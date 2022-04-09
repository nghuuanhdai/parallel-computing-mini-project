//================================================================================
// Name        : PerfSensorGroup.cpp
// Author      : Micha Mueller, Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Perfevent sensor group class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
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

#include "PerfSensorGroup.h"

#include <algorithm>
#include <functional>
#include <limits.h>
#include <unistd.h>

#include <asm/unistd.h>

#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>

#include <sys/ioctl.h>
#include <sys/sysinfo.h>

//the read group data will have this format
struct read_format {
	uint64_t nr;
	uint64_t time_enabled; /* if PERF_FORMAT_TOTAL_TIME_ENABLED */
	uint64_t time_running; /* if PERF_FORMAT_TOTAL_TIME_RUNNING */
	struct {
		uint64_t value;
		uint64_t id;
	} values[];
};

PerfSensorGroup::PerfSensorGroup(const std::string &name)
    : SensorGroupTemplate(name),
      _htAggregation(0),
      _maxCorrection(20),
      _buf(nullptr),
      _bufSize(0) {
}

PerfSensorGroup::PerfSensorGroup(const PerfSensorGroup &other)
    : SensorGroupTemplate(other),
      _htAggregation(other._htAggregation),
      _maxCorrection(other._maxCorrection),
      _buf(nullptr),
      _bufSize(0) {
}

PerfSensorGroup::~PerfSensorGroup() {
	if (_buf) {
		delete[] _buf;
	}
}

PerfSensorGroup &PerfSensorGroup::operator=(const PerfSensorGroup &other) {
	SensorGroupTemplate::operator=(other);
	_htAggregation = other._htAggregation;
	_maxCorrection = other._maxCorrection;
	_buf = nullptr;
	_bufSize = 0;

	return *this;
}

void PerfSensorGroup::execOnInit() {
	//clear vectors in case this method gets called multiple times, although it shouldn't
	_sensorBins.clear();
	_cpuBinMapping.clear();

	for (int i = 0; i < get_nprocs(); i++) {
		_cpuBinMapping.push_back(-1);
	}

	//Sort sensors into bins. Every bin equals an perf-event group
	for (auto s : _sensors) {
		int cpu = s->getCpu();
		int bin = _cpuBinMapping[cpu];

		if (bin != -1) {
			_sensorBins[bin].sensors.push_back(s);
		} else {
			sensorBin bin(s, cpu);
			_sensorBins.push_back(bin);
			_cpuBinMapping[cpu] = _sensorBins.size() - 1;
		}
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
			if (lhs->getType() == rhs->getType()) {
				return lhs->getConfig() < rhs->getConfig();
			} else {
				return lhs->getType() < rhs->getType();
			}
		});
	}

	_sensorBins.shrink_to_fit();
	_cpuBinMapping.shrink_to_fit();

	/* Allocate buffer to read in later. Reading struct has the following format:
	 *
	 *	struct read_format {
	 *		u64 nr;            // The number of events
	 *		u64 time_enabled;  // if PERF_FORMAT_TOTAL_TIME_ENABLED
	 *		u64 time_running;  // if PERF_FORMAT_TOTAL_TIME_RUNNING
	 *		struct {
	 *			u64 value;     // The value of the event
	 *			u64 id;        // if PERF_FORMAT_ID
	 *		} values[nr];
	 *	};
	 *
	 *	Therefore we require 16 byte per sensor plus an additional 8*3 byte
	 */
	std::size_t bufSize = binSensorSize * 16 + 24;
	if (!_buf) {
		_buf = new char[bufSize];
		_bufSize = bufSize;
	} else if (bufSize > _bufSize) {
		delete _buf;
		_buf = new char[bufSize];
		_bufSize = bufSize;
	}

	if (!_htAggregation) {
		return;
	}

	//set up convenience aggregator flags
	for (auto &b : _sensorBins) {
		int cpu = b.cpu;
		int mod = cpu % _htAggregation;

		//search bin with smallest multiple of mod as CPU. This bin will then aggregate us
		for (int agg = mod; agg < get_nprocs(); agg += _htAggregation) {
			int bin = _cpuBinMapping[agg];

			if (bin != -1) {
				//found bin aggregating us (could be ourselves)
				b.aggregator = false;
				_sensorBins[bin].aggregator = true;
				if(cpu != mod) {
					for(auto &sensor: b.sensors){
						sensor->setPublish(false);
					}
				}
				break;
			}
		}
	}
}

bool PerfSensorGroup::execOnStart() {
	//setup
	int                    fd, lfd;
	uint64_t               id;
	S_Ptr                  pc;
	struct perf_event_attr pe;

	memset(&pe, 0, sizeof(struct perf_event_attr));
	pe.size = sizeof(struct perf_event_attr);
	pe.exclude_kernel = 0;
	pe.exclude_hv = 0;
	pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;

	//open perfevent groups
	for (const auto &b : _sensorBins) {
		//perf_event_open() first sensor for cpu as group leader
		pc = b.sensors.front();

		pe.type = pc->getType();
		pe.config = pc->getConfig();
		pe.disabled = 1;

		fd = syscall(__NR_perf_event_open, &pe, -1, pc->getCpu(), -1, 0);
		if (fd == -1) {
			LOG(error) << "Failed to open performance-counter group \"" << _groupName << "\":" << strerror(errno);
			this->stop();
			return false;
		}
		ioctl(fd, PERF_EVENT_IOC_ID, &id);
		//store fd and id to make sensor distinguishable when reading
		pc->setFd(fd);
		pc->setId(id);

		lfd = fd;

		LOG(debug) << "  " << _groupName << "::" << pc->getName() << " opened with ID " << std::to_string(id);

		pe.disabled = 0;
		//open all other counters for the same cpu and attach them to group leader
		for (unsigned i = 1; i < b.sensors.size(); i++) {
			pc = b.sensors[i];

			pe.type = pc->getType();
			pe.config = pc->getConfig();

			fd = syscall(__NR_perf_event_open, &pe, -1, pc->getCpu(), lfd, 0);
			//store id, so that we can match counters with values later (see read())
			if (fd != -1) {
				pc->setFd(fd);
				int rc;
				if ((rc = ioctl(fd, PERF_EVENT_IOC_ID, &id)) == 0) {
					pc->setId(id);
					LOG(debug) << "  " << _groupName << "::" << pc->getName() << " opened with ID " << std::to_string(id);
				} else {
					LOG(debug) << "  " << _groupName << "::" << pc->getName() << " error obtaining ID: " << strerror(rc);
				}
			} else {
				LOG(debug) << "  " << _groupName << "::" << pc->getName() << " error opening perf file descriptor: " << strerror(errno);
			}
		}
	}

	for (const auto &b : _sensorBins) {
		ioctl(b.sensors.front()->getFd(), PERF_EVENT_IOC_RESET, 0);
		ioctl(b.sensors.front()->getFd(), PERF_EVENT_IOC_ENABLE, 0);
	}
	return true;
}

void PerfSensorGroup::execOnStop() {
	for (auto s : _sensors) {
		if (s->getFd() != -1) {
			close(s->getFd());
		}
		s->setFd(-1);
		s->setId(0);
	}
}

void PerfSensorGroup::read() {
	ureading_t reading;
	reading.timestamp = getTimestamp();

	struct read_format *rf = (struct read_format *)_buf;

	for (auto &bin : _sensorBins) {

		if (::read(bin.sensors.front()->getFd(), _buf, _bufSize) < 0) {
			LOG(error) << "Sensorgroup" << _groupName << " could not read value";
			return;
		}

		double   correction = 1;
		bool     validMeasurement = true;
		uint64_t time_enabled = calculateIntervalValue(bin.latest_time_enabled, rf->time_enabled, ULLONG_MAX);
		uint64_t time_running = calculateIntervalValue(bin.latest_time_running, rf->time_running, ULLONG_MAX);

		//store latest times
		bin.latest_time_enabled = rf->time_enabled;
		bin.latest_time_running = rf->time_running;

		if (time_running) {
			correction = static_cast<double>(time_enabled) / time_running;
		} else {
			LOG(debug) << "PerfSensorGroup: Group: " << _groupName << "::CPU" << bin.cpu << " could not be measured. Time running==0";
			validMeasurement = false;
		}
		if (correction > _maxCorrection || correction < 1) {
			LOG(debug) << "PerfSensorGroup: Group: " << _groupName << "::CPU" << bin.cpu << " could not be measured. Correction factor ==" << correction;
			validMeasurement = false;
		}

		if (!validMeasurement) {
			bin.lastValid = false;
			bin.latestValueValid = false;
			continue;
		}
		//iterate over all values returned by ::read()
		for (size_t i = 0; i < rf->nr; i++) {
			reading.value = rf->values[i].value;

			//iterate over all counters and find the one with matching id
			for (const auto &s : bin.sensors) {
				if (rf->values[i].id == s->getId()) {
#ifdef DEBUG
					LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
					if (bin.lastValid) {
						//storeReading takes care of delta computation and applies correction value on the result
						s->storeReading(reading, correction, !_htAggregation);
						bin.latestValueValid = true;
					} else {
						//Before we can compute correct values again after an invalid reading we have to update the lastRawValue first
						s->setLastURaw(reading.value);
					}
					break;
				}
			}
		}
		if (validMeasurement) { //set valid for the next time to read
			bin.lastValid = true;
		}
	}

	//hyper-threading aggregation logic
	if (_htAggregation) {
		for (const auto &bin : _sensorBins) {
			if (!(bin.aggregator)) {
				//this cpu bin gets aggregated
				continue;
			}

			for (size_t i = 0; i < bin.sensors.size(); i++) {
				reading_t reading;
				reading.value = 0;
				reading.timestamp = 0;

				for (int j = bin.cpu; j < get_nprocs(); j += _htAggregation) {
					int b = _cpuBinMapping[j];
					if (b != -1) {
						S_Ptr &sensor2 = _sensorBins[b].sensors[i];
						if (_sensorBins[b].latestValueValid) {
							reading.value += sensor2->getLatestValue().value;
							reading.timestamp = sensor2->getLatestValue().timestamp;
						}
					}
				}

				if (reading.timestamp != 0) {
					bin.sensors[i]->storeReadingGlobal(reading);
				}
			}
		}
	}
}

uint64_t PerfSensorGroup::calculateIntervalValue(uint64_t previous, uint64_t current, uint64_t maxValue) {
	if (previous > current) { //overflow
		return current + (maxValue - previous);
	}
	return current - previous;
}

void PerfSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "HT aggregation: " << (_htAggregation ? "true" : "false");
	LOG_VAR(ll) << leading << "maxCorrection: " << _maxCorrection;
}
