//================================================================================
// Name        : MSRSensorGroup.cpp
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for MSR sensor group class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//================================================================================

#include "MSRSensorGroup.h"

#include <atomic>
#include <boost/log/core/record.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/parameter/keyword.hpp>
#include <exception>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "Types.h"
#include "logging.h"
#include "timestamp.h"
#include <iomanip>
#include <sstream>
#include <thread>


MSRSensorGroup::MSRSensorGroup(const std::string &name)
    : SensorGroupTemplate(name), _htAggregation(0) {
	_total_number_cpus = std::thread::hardware_concurrency();
}

MSRSensorGroup::~MSRSensorGroup() {
}

void MSRSensorGroup::groupInBins() {
	for (auto s : _sensors) {
		_sensorBins[s->getCpu()].addMsrSensorBin(s);
	}
	//sanity check: all bins should have the same number of sensors
	if (_sensorBins.size() == 0) {
		LOG(error) << "Sensorgroup " << _groupName << " failed to sort sensors!";
		return;
	}
	size_t binSensorSize = _sensorBins.front().sensors.size();
	for (auto &b : _sensorBins) {
		if (b.sensors.size() != binSensorSize) {
			LOG(error) << "Sensorgroup " << _groupName << " sensor number missmatch!";
			return;
		}
	}

	//sort bins, so that the sensor ordering is equal in every bin (useful in case of hyper-threading aggregation
	for (auto &b : _sensorBins) {
		std::sort(b.sensors.begin(), b.sensors.end(), [](const S_Ptr &lhs, const S_Ptr &rhs) {
			return lhs->getMetric() < rhs->getMetric();
		});
	}

	_sensorBins.shrink_to_fit();

	if (_htAggregation) {
		for (unsigned int cpu = _htAggregation; cpu < _total_number_cpus; ++cpu) {
			for (unsigned int m = 0; m < _sensorBins[cpu].sensors.size(); ++m) {
				_sensorBins[cpu].sensors[m]->setPublish(false);
			}
		}
	}
}

bool MSRSensorGroup::execOnStart() {
	for (unsigned int cpu = 0; cpu < _sensorBins.size(); ++cpu) {
		if (!_sensorBins[cpu].isActive()) {
			continue;
		}
		const std::size_t BUF_LEN = 200;
		char              path[BUF_LEN];
		snprintf(path, BUF_LEN, "/dev/cpu/%d/msr", cpu);
		int handle = open(path, O_RDWR);
		if (handle < 0) { // try msr_safe
			snprintf(path, BUF_LEN, "/dev/cpu/%d/msr_safe", cpu);
			handle = open(path, O_RDWR);
		}
		if (handle < 0) {
			LOG(error) << "Can't open msr device " << path;
			continue;
		}
		_sensorBins[cpu].setFd(handle);
	}
	program_fixed();

	return true;
}

void MSRSensorGroup::execOnStop() {
	//close file descriptors and leave counters running freely
	for (unsigned int cpu = 0; cpu < _sensorBins.size(); ++cpu) {
		if (_sensorBins[cpu].isActive()) {
			close(_sensorBins[cpu].getFd());
			_sensorBins[cpu].setFd(-1);
		}
	}
}

void MSRSensorGroup::read() {
	ureading_t reading;
	reading.timestamp = getTimestamp();

	try {
		for (auto s : _sensors) {
			if (checkStatus(s->getMetric(), s->getCpu()) == 0) {
				auto ret_val = msr_read(s->getMetric(), &reading.value, s->getCpu());
				if (ret_val != -1) {
					s->storeReading(reading, 1, !_htAggregation); //1 is no correction...
#ifdef DEBUG
					LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
				}
			} else {
				LOG(debug) << _groupName << "::" << s->getName() << " has been disabled, ignoring reading";
				s->setFirstReading(true);
			}
		}

		if (_htAggregation) {
			for (unsigned int cpu = 0; cpu < _htAggregation; ++cpu) { // loop through all cpus until the aggregation
				for (unsigned int m = 0; m < _sensorBins[cpu].sensors.size(); ++m) { //loop through the group's sensors
					reading_t aggregation;
					aggregation.value = 0;
					aggregation.timestamp = reading.timestamp;
					// starting at the cpu we find all the cpus which will be aggregated here
					for (unsigned int agg = cpu; agg + _htAggregation < _total_number_cpus; agg += _htAggregation) {
						if (_sensorBins[agg].isActive()) {
							aggregation.value += _sensorBins[agg].sensors[m]->getLatestValue().value;
						}
					}
					_sensorBins[cpu].sensors[m]->storeReadingGlobal(aggregation);
				}
			}
		}

	} catch (const std::exception &e) {
		LOG(error) << "Sensorgroup " << _groupName << " could not read value: " << e.what();
	}

	program_fixed();
}

int MSRSensorGroup::checkStatus(uint64_t msr_number, unsigned int cpu) {
	if (!_sensorBins[cpu].isActive()) { // CPU is not active, so it won't be programmed
		return 0;
	}

	struct FixedEventControlRegister ctrl_reg;
	if (msr_read(IA32_CR_FIXED_CTR_CTRL, &ctrl_reg.value, cpu) != -1) {
		if ((msr_number == INST_RETIRED_ANY_ADDR) && !(ctrl_reg.fields.os0 && ctrl_reg.fields.usr0))
			return 1;
		else if ((msr_number == CPU_CLK_UNHALTED_THREAD_ADDR) && !(ctrl_reg.fields.os1 && ctrl_reg.fields.usr1))
			return 1;
		else if ((msr_number == CPU_CLK_UNHALTED_REF_ADDR) && !(ctrl_reg.fields.os2 && ctrl_reg.fields.usr2))
			return 1;
	}

	return 0;
}
	
int32_t MSRSensorGroup::msr_read(uint64_t msr_number, uint64_t *value, unsigned int cpu) {
	return pread(_sensorBins[cpu].getFd(), (void *)value, sizeof(uint64_t), msr_number);
}

int32_t MSRSensorGroup::msr_write(uint64_t msr_number, uint64_t value, unsigned int cpu) {
	return pwrite(_sensorBins[cpu].getFd(), (const void *)&value, sizeof(uint64_t), msr_number);
}

/**
 * Program the fixed MSR as required for this plugin.
 *
 * @return  True if counters programmed successfully, false otherwise, e.g.
 *          because the counters are already in use.
 */
void MSRSensorGroup::program_fixed() {
	unsigned int freeRunning = 0;
	for (unsigned int cpu = 0; cpu < _sensorBins.size(); ++cpu) {
		if (!_sensorBins[cpu].isActive()) { // CPU is not active, so it won't be programmed
			continue;
		}
		// program core counters

		//we do not want to interrupt other services already doing measurements with MSRs
		//therefore check if any fixed counter is currently enabled
		struct FixedEventControlRegister ctrl_reg;

		msr_read(IA32_CR_FIXED_CTR_CTRL, &ctrl_reg.value, cpu);
		//are they all enabled?
		if (ctrl_reg.fields.os0 && ctrl_reg.fields.usr0 && ctrl_reg.fields.os1 && ctrl_reg.fields.usr1 && ctrl_reg.fields.os2 && ctrl_reg.fields.usr2) {
			//yes! Free running counters were set by someone else => we don't need to program them, just read them.
			freeRunning++;
			continue;
		}
		//not all of them (or none) are enabled => we program them again

		// disable counters while programming
		msr_write(IA32_CR_PERF_GLOBAL_CTRL, 0, cpu);

		ctrl_reg.fields.os0 = 1;
		ctrl_reg.fields.usr0 = 1;
		ctrl_reg.fields.any_thread0 = 0;
		ctrl_reg.fields.enable_pmi0 = 0;

		ctrl_reg.fields.os1 = 1;
		ctrl_reg.fields.usr1 = 1;
		ctrl_reg.fields.any_thread1 = 0;
		ctrl_reg.fields.enable_pmi1 = 0;

		ctrl_reg.fields.os2 = 1;
		ctrl_reg.fields.usr2 = 1;
		ctrl_reg.fields.any_thread2 = 0;
		ctrl_reg.fields.enable_pmi2 = 0;

		ctrl_reg.fields.reserved1 = 0;

		// program them
		msr_write(IA32_CR_FIXED_CTR_CTRL, ctrl_reg.value, cpu);

		// start counting, enable 3 fixed counters (enable also the programmables counters)
		uint64_t value = (1ULL << 0) + (1ULL << 1) + (1ULL << 2) + (1ULL << 3) + (1ULL << 32) + (1ULL << 33) + (1ULL << 34);
		//uint64_t value = (1ULL << 32) + (1ULL << 33) + (1ULL << 34);
		msr_write(IA32_CR_PERF_GLOBAL_CTRL, value, cpu);
	}
	
	if (freeRunning != _sensorBins.size()) {
		LOG(debug) << "Programmed fixed counters on " << _sensorBins.size()-freeRunning << " CPUs, " << freeRunning << " were already free runnning";
	}
}

void MSRSensorGroup::addCpu(unsigned int cpu) {
	if (cpu + 1 > _sensorBins.size()) {
		_sensorBins.resize(cpu + 1);
	}
	_sensorBins[cpu].setActive();
}

std::vector<unsigned> MSRSensorGroup::getCpus() {
	std::vector<unsigned> cpus;
	for (unsigned int cpu = 0; cpu < _sensorBins.size(); ++cpu) {
		if (_sensorBins[cpu].isActive()) {
			cpus.push_back(cpu);
		}
	}
	return cpus;
}

void MSRSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::stringstream ss;
	const char *      separator = "";
	for (unsigned int cpu = 0; cpu < _sensorBins.size(); ++cpu) {
		if (_sensorBins[cpu].isActive()) {
			ss << separator << cpu;
			separator = ", ";
		}
	}
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "CPUs:  " << ss.str();
}
