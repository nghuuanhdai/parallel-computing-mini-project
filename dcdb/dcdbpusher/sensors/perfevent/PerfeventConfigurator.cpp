//================================================================================
// Name        : PerfeventConfigurator.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Perfevent plugin configurator class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2017-2019 Leibniz Supercomputing Centre
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

#include "PerfeventConfigurator.h"

#include <linux/perf_event.h>
#include <sys/sysinfo.h>
#include <unistd.h>

using namespace std;

PerfeventConfigurator::PerfeventConfigurator() {
	_groupName = "group";
	_baseName = "counter";

	//set up enum-maps to map string from cfgFile to an enum value defined in linux/perf_event.h
	_enumType["PERF_TYPE_HARDWARE"] = PERF_TYPE_HARDWARE;
	_enumType["PERF_TYPE_SOFTWARE"] = PERF_TYPE_SOFTWARE;
	_enumType["PERF_TYPE_TRACEPOINT"] = PERF_TYPE_TRACEPOINT;
	_enumType["PERF_TYPE_HW_CACHE"] = PERF_TYPE_HW_CACHE;
	_enumType["PERF_TYPE_RAW"] = PERF_TYPE_RAW;
	_enumType["PERF_TYPE_BREAKPOINT"] = PERF_TYPE_BREAKPOINT;

	//if type==PERF_TYPE_HARDWARE
	_enumConfig["PERF_COUNT_HW_CPU_CYCLES"] = PERF_COUNT_HW_CPU_CYCLES;
	_enumConfig["PERF_COUNT_HW_INSTRUCTIONS"] = PERF_COUNT_HW_INSTRUCTIONS;
	_enumConfig["PERF_COUNT_HW_CACHE_REFERENCES"] = PERF_COUNT_HW_CACHE_REFERENCES;
	_enumConfig["PERF_COUNT_HW_CACHE_MISSES"] = PERF_COUNT_HW_CACHE_MISSES;
	_enumConfig["PERF_COUNT_HW_BRANCH_INSTRUCTIONS"] = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
	_enumConfig["PERF_COUNT_HW_BRANCH_MISSES"] = PERF_COUNT_HW_BRANCH_MISSES;
	_enumConfig["PERF_COUNT_HW_BUS_CYCLES"] = PERF_COUNT_HW_BUS_CYCLES;
	_enumConfig["PERF_COUNT_HW_STALLED_CYCLES_FRONTEND"] = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND;
	_enumConfig["PERF_COUNT_HW_STALLED_CYCLES_BACKEND"] = PERF_COUNT_HW_STALLED_CYCLES_BACKEND;
	_enumConfig["PERF_COUNT_HW_REF_CPU_CYCLES"] = PERF_COUNT_HW_REF_CPU_CYCLES;

	//if type==PERF_TYPE_SOFTWARE
	_enumConfig["PERF_COUNT_SW_CPU_CLOCK"] = PERF_COUNT_SW_CPU_CLOCK;
	_enumConfig["PERF_COUNT_SW_TASK_CLOCK"] = PERF_COUNT_SW_TASK_CLOCK;
	_enumConfig["PERF_COUNT_SW_PAGE_FAULTS"] = PERF_COUNT_SW_PAGE_FAULTS;
	_enumConfig["PERF_COUNT_SW_CONTEXT_SWITCHES"] = PERF_COUNT_SW_CONTEXT_SWITCHES;
	_enumConfig["PERF_COUNT_SW_CPU_MIGRATIONS"] = PERF_COUNT_SW_CPU_MIGRATIONS;
	_enumConfig["PERF_COUNT_SW_PAGE_FAULTS_MIN"] = PERF_COUNT_SW_PAGE_FAULTS_MIN;
	_enumConfig["PERF_COUNT_SW_PAGE_FAULTS_MAJ"] = PERF_COUNT_SW_PAGE_FAULTS_MAJ;
	_enumConfig["PERF_COUNT_SW_ALIGNMENT_FAULTS"] = PERF_COUNT_SW_ALIGNMENT_FAULTS;
	_enumConfig["PERF_COUNT_SW_EMULATION_FAULTS"] = PERF_COUNT_SW_EMULATION_FAULTS;
	_enumConfig["PERF_COUNT_SW_DUMMY"] = PERF_COUNT_SW_DUMMY;

	//TODO set up map for rest of config enum
}

PerfeventConfigurator::~PerfeventConfigurator() {}

void PerfeventConfigurator::sensorBase(PerfSensorBase &s, CFG_VAL config) {
	/*
	 * Custom code, as perf-event is an extra special plugin
	 */
	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
		if (boost::iequals(val.first, "type")) {
			try {
				uint64_t type = stoull(val.second.data(), 0, 0);
				LOG(debug) << "  Type:      0x" << hex << type << dec;
				s.setType(type);
			} catch (const std::invalid_argument &e) {
				enumMap_t::iterator it = _enumType.find(val.second.data());
				if (it != _enumType.end()) {
					s.setType(it->second);
					LOG(debug) << "  Type:      " << val.second.data() << " (= " << s.getType() << ")";
				} else {
					LOG(warning) << "  Type \"" << val.second.data() << "\" not known and could not be parsed as integer type.";
				}
			} catch (const std::exception &e) {
				LOG(warning) << "  Error parsing event type \"" << val.second.data() << "\": " << e.what();
			}
		} else if (boost::iequals(val.first, "config")) {
			if (s.getType() == PERF_TYPE_BREAKPOINT) {
				//leave config zero
			} else if (s.getType() == PERF_TYPE_RAW || s.getType() > PERF_TYPE_MAX) {
				//read in custom hex-value
				unsigned long config = stoull(val.second.data(), 0, 0);
				s.setConfig(config);
				LOG(debug) << "  Config:    Raw value: 0x" << hex << s.getConfig() << dec;
			} else {
				enumMap_t::iterator it = _enumConfig.find(val.second.data());
				if (it != _enumConfig.end()) {
					s.setConfig(it->second);
					LOG(debug) << "  Config:    " << val.second.data() << " (= " << s.getConfig() << ")";
				} else {
					LOG(warning) << "  Config \"" << val.second.data() << "\" not known.";
				}
			}
		} else if (boost::iequals(val.first, "delta")) {
			//it is explicitly stated to be off --> set it to false
			s.setDelta(!(val.second.data() == "off"));
		}
	}
}

void PerfeventConfigurator::sensorGroup(PerfSensorGroup &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("maxCorrection", setMaxCorrection);
		ATTRIBUTE("htVal", setHtAggregation);
	}
}

bool PerfeventConfigurator::readConfig(std::string cfgPath) {
	/*
	 * Custom code, as perf-event is an extra special plugin
	 */
	_cfgPath = cfgPath;

	boost::property_tree::iptree cfg;
	boost::property_tree::read_info(cfgPath, cfg);

	//read global variables (if present overwrite those from global.conf)
	readGlobal(cfg);

	//read groups and templates for groups
	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, cfg) {
		if (boost::iequals(val.first, "template_" + _groupName)) {
			LOG(debug) << "Template " << _groupName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				PerfSensorGroup *group = new PerfSensorGroup(val.second.data());
				if (readSensorGroup(*group, val.second)) {
					//check if cpus-list is given for this template group
					boost::optional<boost::property_tree::iptree &> cpus = val.second.get_child_optional("cpus");
					if (cpus) {
						LOG(debug) << "Reading CPUs for \"" << val.second.data() << "\"";
						std::set<int> cpuVec = parseCpuString(cpus.get().data());
						_templateCpus.insert(templateCpuMap_t::value_type(val.second.data(), cpuVec));
					}

					//check if hyper-thread aggregation value is given for this template group
					boost::optional<boost::property_tree::iptree &> ht = val.second.get_child_optional("htVal");
					if (ht) {
						unsigned htVal = std::stoul(ht.get().data());
						_templateHTs.insert(templateHTMap_t::value_type(val.second.data(), htVal));
						LOG(debug) << "HT value of " << htVal << " given for \"" << val.second.data() << "\"";
					}

					auto ret = _templateSensorGroups.insert(std::pair<std::string, PerfSensorGroup *>(val.second.data(), group));
					if (!ret.second) {
						LOG(warning) << "Template " << _groupName << " " << val.second.data() << " already exists! Omitting...";
						delete group;
					}
				} else {
					LOG(warning) << "Template " << _groupName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
					delete group;
				}
			}
			//template single sensor
		} else if (boost::iequals(val.first, "template_single_" + _baseName)) {
			LOG(debug) << "Template single " << _baseName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				PerfSensorGroup *group = new PerfSensorGroup(val.second.data());
				if (readSensorGroup(*group, val.second)) {
					//check if cpus-list is given for this template single counter
					boost::optional<boost::property_tree::iptree &> cpus = val.second.get_child_optional("cpus");
					if (cpus) {
						LOG(debug) << "Reading CPUs for \"" << val.second.data() << "\"";
						std::set<int> cpuVec = parseCpuString(cpus.get().data());
						_templateCpus.insert(templateCpuMap_t::value_type(val.second.data(), cpuVec));
					}

					//check if hyper-thread aggregation value is given for this template group
					boost::optional<boost::property_tree::iptree &> ht = val.second.get_child_optional("htVal");
					if (ht) {
						unsigned htVal = std::stoul(ht.get().data());
						LOG(debug) << "HT value of " << htVal << " given for \"" << val.second.data() << "\"";
						_templateHTs.insert(templateHTMap_t::value_type(val.second.data(), htVal));
					}

					//group which consists of only one sensor
					std::shared_ptr<PerfSensorBase> sensor = std::make_shared<PerfSensorBase>(val.second.data());
					if (readSensorBase(*sensor, val.second)) {
						group->pushBackSensor(sensor);

						auto ret = _templateSensorGroups.insert(std::pair<std::string, PerfSensorGroup *>(val.second.data(), group));
						if (!ret.second) {
							LOG(warning) << "Template single " << _baseName << " " << val.second.data() << " already exists! Omitting...";
							delete group;
						}
					} else {
						LOG(warning) << "Template single " << _baseName << " " << val.second.data() << " could not be read! Omitting";
						delete group;
					}
				} else {
					LOG(warning) << "Template single " << _baseName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
					delete group;
				}
			}
		} else if (boost::iequals(val.first, _groupName)) {
			LOG(debug) << _groupName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				PerfSensorGroup group(val.second.data());
				if (readSensorGroup(group, val.second)) {
					customizeAndStore(group, val.second);
				} else {
					LOG(warning) << _groupName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
				}
			}
			//single sensor
		} else if (boost::iequals(val.first, "single_" + _baseName)) {
			LOG(debug) << "Single " << _baseName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				PerfSensorGroup group(val.second.data());
				if (readSensorGroup(group, val.second)) {
					//group which consists of only one sensor
					std::shared_ptr<PerfSensorBase> sensor;
					//perhaps one sensor is already present because it was copied from the template group
					if (group.getDerivedSensors().size() != 0) {
						sensor = group.getDerivedSensors()[0];
						sensor->setName(val.second.data());
						if (readSensorBase(*sensor, val.second)) {
							customizeAndStore(group, val.second);
						} else {
							LOG(warning) << "Single " << _baseName << " " << val.second.data() << " could not be read! Omitting";
						}
					} else {
						sensor = std::make_shared<PerfSensorBase>(val.second.data());
						if (readSensorBase(*sensor, val.second)) {
							group.pushBackSensor(sensor);
							customizeAndStore(group, val.second);
						} else {
							LOG(warning) << "Single " << _baseName << " " << val.second.data() << " could not be read! Omitting";
						}
					}
				} else {
					LOG(warning) << "Single " << _baseName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
				}
			}
		} else if (!boost::iequals(val.first, "global")) {
			LOG(error) << "\"" << val.first << "\": unknown construct!";
			return false;
		}
	}
	//we do not need them anymore
	_templateCpus.clear();
	//read of config finished. Now we build the mqtt-topic for every sensor
	return constructSensorTopics();
}

void PerfeventConfigurator::customizeAndStore(PerfSensorGroup &group, CFG_VAL cfg) {
	//initialize set with cpuIDs
	//default cpuSet: contains all cpuIDs
	std::set<int> cpuSet;
	for (int i = 0; i < get_nprocs(); i++) {
		cpuSet.insert(i);
	}
	//check if (differing) cpus-list is given; if so, overwrite default cpuVec
	boost::optional<boost::property_tree::iptree &> cpus = cfg.get_child_optional("cpus");
	if (cpus) { //cpu list given
		cpuSet = parseCpuString(cpus.get().data());
	} else { //cpu list not given, but perhaps template counter has one
		boost::optional<boost::property_tree::iptree &> def = cfg.get_child_optional("default");
		if (def) {
			templateCpuMap_t::iterator itC = _templateCpus.find(def.get().data());
			if (itC != _templateCpus.end()) {
				cpuSet = itC->second;
			}
		}
	}

	if (cpuSet.empty()) {
		LOG(warning) << _groupName << ": Empty CPU set!";
		return;
	}

	PerfSGPtr                                    SG = std::make_shared<PerfSensorGroup>(group);
	std::vector<std::shared_ptr<PerfSensorBase>> sensors = group.getPerfSensors();

	//duplicate sensors of perfCounterGroup for every CPU
	//set first cpu to already existing sensors
	std::set<int>::iterator it = cpuSet.begin();

	for (const auto &s : SG->getPerfSensors()) {
		s->setCpu(*it);
		s->setMqtt(MQTTChecker::formatTopic(s->getMqtt(), *it));
	}
	it++;

	//create additional sensors
	for (; it != cpuSet.end(); ++it) {
		for (auto s : sensors) {
			std::shared_ptr<PerfSensorBase> sensor = std::make_shared<PerfSensorBase>(*s);
			sensor->setCpu(*it);
			sensor->setMqtt(MQTTChecker::formatTopic(s->getMqtt(), *it));
			SG->pushBackSensor(sensor);
		}
	}

	storeSensorGroup(SG);
}
