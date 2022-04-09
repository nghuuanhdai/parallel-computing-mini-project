//================================================================================
// Name        : MSRConfigurator.cpp
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for MSR plugin configurator class.
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

#include "MSRConfigurator.h"
#include <iomanip>

MSRConfigurator::MSRConfigurator() {
	_groupName = "group";
	_baseName = "sensor";
}

MSRConfigurator::~MSRConfigurator() {}

void MSRConfigurator::sensorBase(MSRSensorBase &s, CFG_VAL config) {
	ADD {
		if (boost::iequals(val.first, "metric")) {
			try {
				uint64_t metric = std::stoull(val.second.data(), nullptr, 16);
				s.setMetric(metric);
			} catch (const std::exception &e) {
				LOG(error) << "  Error parsing metric \"" << val.second.data() << "\": " << e.what();
			}
		}
	}
}

void MSRConfigurator::sensorGroup(MSRSensorGroup &s, CFG_VAL config) {
	ADD {
		if (boost::iequals(val.first, "cpus")) {
			std::set<int> cpus = ConfiguratorTemplate::parseCpuString(val.second.data());
			for (int cpu : cpus) {
				s.addCpu(static_cast<unsigned int>(cpu));
			}
		} else if (boost::iequals(val.first, "htVal")) {
			try {
				unsigned int htVal = std::stoul(val.second.data());
				s.setHtAggregation(htVal);
			} catch (const std::exception &e) {
				LOG(error) << "  Error parsing htVal \"" << val.second.data() << "\": " << e.what();
			}
		}
	}
}

/**
 * Custom readConfig, as MSR has to copy sensors for each CPU
 */
bool MSRConfigurator::readConfig(std::string cfgPath) {
	_cfgPath = cfgPath;

	boost::property_tree::iptree cfg;
	boost::property_tree::read_info(cfgPath, cfg);

	//read global variables (if present overwrite those from global.conf)
	readGlobal(cfg);

	//read groups and templates for groups
	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, cfg) {
		//template group
		if (boost::iequals(val.first, "template_" + _groupName)) {
			LOG(debug) << "Template " << _groupName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				MSRSensorGroup *group = new MSRSensorGroup(val.second.data());
				if (readSensorGroup(*group, val.second, true)) {
					auto ret = _templateSensorGroups.insert(std::pair<std::string, MSRSensorGroup *>(val.second.data(), group));
					if (!ret.second) {
						LOG(warning) << "Template " << _groupName << " " << val.second.data() << " already exists! Omitting...";
						delete group;
					}
				} else {
					LOG(warning) << "Template " << _groupName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
					delete group;
				}
			}
			//template base
		} else if (boost::iequals(val.first, "template_" + _baseName)) {
			LOG(debug) << "Template " << _baseName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				MSRSensorBase *base = new MSRSensorBase(val.second.data());
				if (readSensorBase(*base, val.second, true)) {
					auto ret = _templateSensorBases.insert(std::pair<std::string, MSRSensorBase *>(val.second.data(), base));
					if (!ret.second) {
						LOG(warning) << "Template " << _baseName << " " << val.second.data() << " already exists! Omitting...";
						delete base;
					}
				} else {
					LOG(warning) << "Template " << _baseName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
					delete base;
				}
			}
			//template single sensor
		} else if (boost::iequals(val.first, "template_single_" + _baseName)) {
			LOG(debug) << "Template single " << _baseName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				MSRSensorGroup *group = new MSRSensorGroup(val.second.data());
				if (readSensorGroup(*group, val.second, true)) {
					//group which consists of only one sensor
					SB_Ptr sensor = std::make_shared<MSRSensorBase>(val.second.data());
					if (readSensorBase(*sensor, val.second, true)) {
						group->pushBackSensor(sensor);
						auto ret = _templateSensorGroups.insert(std::pair<std::string, MSRSensorGroup *>(val.second.data(), group));
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
			//group
		} else if (boost::iequals(val.first, _groupName)) {
			LOG(debug) << _groupName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				SG_Ptr group = std::make_shared<MSRSensorGroup>(val.second.data());
				if (readSensorGroup(*group, val.second)) {
					customizeAndStore(group);
				} else {
					LOG(warning) << _groupName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
				}
			}
			//single sensor
		} else if (boost::iequals(val.first, "single_" + _baseName)) {
			LOG(debug) << "Single " << _baseName << " \"" << val.second.data() << "\"";
			if (!val.second.empty()) {
				SG_Ptr group = std::make_shared<MSRSensorGroup>(val.second.data());
				if (readSensorGroup(*group, val.second)) {
					//group which consists of only one sensor
					SB_Ptr sensor;
					//perhaps one sensor is already present because it was copied from the template group
					if (group->getDerivedSensors().size() != 0) {
						sensor = group->getDerivedSensors()[0];
						sensor->setName(val.second.data());
						if (readSensorBase(*sensor, val.second)) {
							customizeAndStore(group);
						} else {
							LOG(warning) << "Single " << _baseName << " " << val.second.data() << " could not be read! Omitting";
						}
					} else {
						sensor = std::make_shared<MSRSensorBase>(val.second.data());
						if (readSensorBase(*sensor, val.second)) {
							group->pushBackSensor(sensor);
							customizeAndStore(group);
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
	//read of config finished. Now we build the mqtt-topic for every sensor
	return constructSensorTopics();
}

void MSRConfigurator::customizeAndStore(SG_Ptr g) {
	bool begin = true;

	std::vector<unsigned>           gCpus = g->getCpus();
	std::vector<unsigned>::iterator it = gCpus.begin();
	std::vector<SB_Ptr>             original;

	// Initializing the vector of "reference" sensors and configuring the first CPU
	for (auto s : g->getDerivedSensors()) {
		SB_Ptr sensor = s;
		// Copying the original sensors for reference
		original.push_back(std::make_shared<MSRSensorBase>(*sensor));
		sensor->setCpu(*it);
		s->setMqtt(MQTTChecker::formatTopic(s->getMqtt(), *it));
	}
	it++;

	// Duplicating sensors for the remaining CPUs from the "reference" vector
	for (; it != gCpus.end(); ++it) {
		for (auto s : original) {
			auto s_otherCPUs = std::make_shared<MSRSensorBase>(*s);
			s_otherCPUs->setCpu(*it);
			s_otherCPUs->setMetric(s->getMetric());
			s_otherCPUs->setMqtt(MQTTChecker::formatTopic(s->getMqtt(), *it));
			g->pushBackSensor(s_otherCPUs);
		}
	}
	g->groupInBins();
	storeSensorGroup(g);
}
