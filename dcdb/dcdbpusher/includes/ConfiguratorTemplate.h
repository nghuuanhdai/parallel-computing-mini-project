//================================================================================
// Name        : ConfiguratorTemplate.h
// Author      : Micha Mueller, Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Interface template for plugin configurators.
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

#ifndef SRC_CONFIGURATORTEMPLATE_H_
#define SRC_CONFIGURATORTEMPLATE_H_

#include <map>
#include <set>

#include "ConfiguratorInterface.h"
#include "SensorGroupTemplate.h"
#include "sensorbase.h"
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

/**
 * @brief Interface template for plugin configurator implementations without
 *        entities.
 *
 * @details There is a derived template class for plugins with entities, see
 *          ConfiguratorTemplateEntity.
 *
 * @ingroup pusherplugins
 */
template <class SBase, class SGroup>
class ConfiguratorTemplate : public ConfiguratorInterface {
	//the template shall only be instantiated for classes which derive from SensorBase/SensorGroup
	static_assert(std::is_base_of<SensorBase, SBase>::value, "SBase must derive from SensorBase!");
	static_assert(std::is_base_of<SensorGroupInterface, SGroup>::value, "SGroup must derive from SensorGroupInterface!");

      protected:
	//TODO use smart pointers
	typedef std::map<std::string, SBase *>  sBaseMap_t;
	typedef std::map<std::string, SGroup *> sGroupMap_t;

	using SB_Ptr = std::shared_ptr<SBase>;
	using SG_Ptr = std::shared_ptr<SGroup>;

      public:
	ConfiguratorTemplate()
	    : ConfiguratorInterface(),
	      _groupName("INVALID"),
	      _baseName("INVALID") {}

	virtual ~ConfiguratorTemplate() {
		for (auto tb : _templateSensorBases) {
			delete tb.second;
		}
		for (auto tg : _templateSensorGroups) {
			delete tg.second;
		}
		_sensorGroups.clear();
		_templateSensorBases.clear();
		_templateSensorGroups.clear();
	}

	/**
     * @brief Read in the given configuration
     *
     * @details Overwriting this method is only required if a custom logic is really necessary!
     *
     * @param   cfgPath Path to the config-file
     *
     * @return  True on success, false otherwise
     */
	bool readConfig(std::string cfgPath) override {
		_cfgPath = cfgPath;

		boost::property_tree::iptree cfg;
		boost::property_tree::read_info(cfgPath, cfg);

		//read global variables (if present overwrite those from global.conf)
		readGlobal(cfg);

		//read groups and templates for groups. If present also template stuff
		BOOST_FOREACH (boost::property_tree::iptree::value_type &val, cfg) {
			//template group
			if (boost::iequals(val.first, "template_" + _groupName)) {
				LOG(debug) << "Template " << _groupName << " \"" << val.second.data() << "\"";
				if (!val.second.empty()) {
					SGroup *group = new SGroup(val.second.data());
					if (readSensorGroup(*group, val.second, true)) {
						auto ret = _templateSensorGroups.insert(std::pair<std::string, SGroup *>(val.second.data(), group));
						if (!ret.second) {
							LOG(warning) << "Template " << _groupName << " "
								     << val.second.data() << " already exists! Omitting...";
							delete group;
						}
					} else {
						LOG(warning) << "Template " << _groupName << " \""
							     << val.second.data() << "\" has bad values! Ignoring...";
						delete group;
					}
				}
				//template base
			} else if (boost::iequals(val.first, "template_" + _baseName)) {
				LOG(debug) << "Template " << _baseName << " \"" << val.second.data() << "\"";
				if (!val.second.empty()) {
					SBase *base = new SBase(val.second.data());
					if (readSensorBase(*base, val.second, true)) {
						auto ret = _templateSensorBases.insert(std::pair<std::string, SBase *>(val.second.data(), base));
						if (!ret.second) {
							LOG(warning) << "Template " << _baseName << " "
								     << val.second.data() << " already exists! Omitting...";
							delete base;
						}
					} else {
						LOG(warning) << "Template " << _baseName << " \""
							     << val.second.data() << "\" has bad values! Ignoring...";
						delete base;
					}
				}
				//template single sensor
			} else if (boost::iequals(val.first, "template_single_" + _baseName)) {
				LOG(debug) << "Template single " << _baseName << " \"" << val.second.data() << "\"";
				if (!val.second.empty()) {
					SGroup *group = new SGroup(val.second.data());
					if (readSensorGroup(*group, val.second, true)) {
						//group which consists of only one sensor
						SB_Ptr sensor = std::make_shared<SBase>(val.second.data());
						if (readSensorBase(*sensor, val.second, true)) {
							group->pushBackSensor(sensor);
							auto ret = _templateSensorGroups.insert(std::pair<std::string, SGroup *>(val.second.data(), group));
							if (!ret.second) {
								LOG(warning) << "Template single " << _baseName << " "
									     << val.second.data() << " already exists! Omitting...";
								delete group;
							}
						} else {
							LOG(warning) << "Template single " << _baseName << " "
								     << val.second.data() << " could not be read! Omitting";
							delete group;
						}
					} else {
						LOG(warning) << "Template single " << _baseName << " \""
							     << val.second.data() << "\" has bad values! Ignoring...";
						delete group;
					}
				}
				//group
			} else if (boost::iequals(val.first, _groupName)) {
				LOG(debug) << _groupName << " \"" << val.second.data() << "\"";
				if (!val.second.empty()) {
					SG_Ptr group = std::make_shared<SGroup>(val.second.data());
					if (readSensorGroup(*group, val.second)) {
						storeSensorGroup(group);
					} else {
						LOG(warning) << _groupName << " \""
							     << val.second.data() << "\" has bad values! Ignoring...";
					}
				}
				//single sensor
			} else if (boost::iequals(val.first, "single_" + _baseName)) {
				LOG(debug) << "Single " << _baseName << " \"" << val.second.data() << "\"";
				if (!val.second.empty()) {
					SG_Ptr group = std::make_shared<SGroup>(val.second.data());
					if (readSensorGroup(*group, val.second)) {
						//group which consists of only one sensor
						SB_Ptr sensor;
						//perhaps one sensor is already present because it was copied from the template group
						if (group->getDerivedSensors().size() != 0) {
							sensor = group->getDerivedSensors()[0];
							sensor->setName(val.second.data());
							if (readSensorBase(*sensor, val.second)) {
								storeSensorGroup(group);
							} else {
								LOG(warning) << "Single " << _baseName << " "
									     << val.second.data() << " could not be read! Omitting";
							}
						} else {
							sensor = std::make_shared<SBase>(val.second.data());
							if (readSensorBase(*sensor, val.second)) {
								group->pushBackSensor(sensor);
								storeSensorGroup(group);
							} else {
								LOG(warning) << "Single " << _baseName << " "
									     << val.second.data() << " could not be read! Omitting";
							}
						}
					} else {
						LOG(warning) << "Single " << _baseName << " \""
							     << val.second.data() << "\" has bad values! Ignoring...";
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

	/**
     * @brief Clear internal storage and return plugin in unconfigured state.
     */
	void clearConfig() override {
		ConfiguratorInterface::clearConfig();
		// Bring everything to a halt
		for (auto g : _sensorGroups)
			g->stop();
		// Wait for stop
		for (auto g : _sensorGroups)
			g->wait();

		//clean up sensors/groups and templates
		for (auto tb : _templateSensorBases) {
			delete tb.second;
		}
		for (auto tg : _templateSensorGroups) {
			delete tg.second;
		}
		_sensorGroups.clear();
		_templateSensorBases.clear();
		_templateSensorGroups.clear();
	}

	/**
     * @brief Print configuration.
     *
     * @param ll Log severity level to be used from logger.
     */
	void printConfig(LOG_LEVEL ll) override {
		ConfiguratorInterface::printConfig(ll);

		//prints plugin specific configurator attributes and entities if present
		printConfiguratorConfig(ll);

		LOG_VAR(ll) << "    " << _groupName << "s:";
		for (auto g : _sensorGroups) {
			g->printConfig(ll);
		}
	}

      protected:
	///@name Template-internal use only
	///@{
	/**
     * @brief Read common values of a sensorbase.
     *
     * @details Reads and sets the common base values of a sensor base
     *          (currently none), then calls sensorBase() to read plugin
     *          specific values.
     *
     * @param sBase      The sensor base for which to set the values.
     * @param config     A boost property (sub-)tree containing the values.
     * @param isTemplate Are we parsing a template or a regular sensor?
     *
     * @return  True on success, false otherwise
     */
	bool readSensorBase(SBase &sBase, CFG_VAL config, bool isTemplate = false) {
		sBase.setCacheInterval(_cacheInterval);
		if (!isTemplate) {
			boost::optional<boost::property_tree::iptree &> def = config.get_child_optional("default");
			if (def) {
				//we copy all values from default
				LOG(debug) << "  Using \"" << def.get().data() << "\" as default.";
				auto it = _templateSensorBases.find(def.get().data());
				if (it != _templateSensorBases.end()) {
					sBase = *(it->second);
					sBase.setName(config.data());
				} else {
					LOG(warning) << "Template " << _baseName << "\" "
						     << def.get().data() << "\" not found! Using standard values.";
				}
			}
		}

		BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
			if (boost::iequals(val.first, "mqttsuffix")) {
				sBase.setMqtt(val.second.data());
			} else if (boost::iequals(val.first, "skipConstVal")) {
				sBase.setSkipConstVal(to_bool(val.second.data()));
			} else if (boost::iequals(val.first, "delta")) {
				sBase.setDelta(to_bool(val.second.data()));
			} else if (boost::iequals(val.first, "deltaMax")) {
				sBase.setDeltaMaxValue(std::stoull(val.second.data()));
			} else if (boost::iequals(val.first, "subSampling")) {
				sBase.setSubsampling(std::stoi(val.second.data()));
			} else if (boost::iequals(val.first, "factor")) {
				sBase.setFactor(std::stod(val.second.data()));
			} else if (boost::iequals(val.first, "publish")) {
				sBase.setPublish(to_bool(val.second.data()));
			} else if (boost::iequals(val.first, "metadata")) {
				SensorMetadata sm;
				if(sBase.getMetadata())
					sm = *sBase.getMetadata();
				try {
					sm.parsePTREE(val.second);
					sBase.setMetadata(sm);
				} catch (const std::exception &e) {
					LOG(error) << "  Metadata parsing failed for sensor " << sBase.getName() << "!" << std::endl;
				}
			}
		}
		if (sBase.getMqtt().size() == 0) {
			sBase.setMqtt(sBase.getName());
		}
		sensorBase(sBase, config);
		return true;
	}

	/**
     * @brief Read common values of a sensorGroup.
     *
     * @details Reads and sets the common base values of a sensor group
     *          (currently none), then calls sensorGroup() to read plugin
     *          specific values.
     *
     * @param sGroup     The sensor group for which to set the values.
     * @param config     A boost property (sub-)tree containing the values.
     * @param isTemplate Are we parsing a template or a regular group?
     *
     * @return  True on success, false otherwise
     */
	bool readSensorGroup(SGroup &sGroup, CFG_VAL config, bool isTemplate = false) {
		//first check if default group is given, unless we are reading a template already
		if (!isTemplate) {
			boost::optional<boost::property_tree::iptree &> def = config.get_child_optional("default");
			if (def) {
				//we copy all values from default (including copy constructing its sensors)
				//if own sensors are specified they are appended
				LOG(debug) << "  Using \"" << def.get().data() << "\" as default.";
				auto it = _templateSensorGroups.find(def.get().data());
				if (it != _templateSensorGroups.end()) {
					sGroup = *(it->second);
					sGroup.setGroupName(config.data());
				} else {
					LOG(warning) << "Template " << _groupName << "\" "
						     << def.get().data() << "\" not found! Using standard values.";
				}
			}
		}

		//read in values inherited from SensorGroupInterface
		BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
			if (boost::iequals(val.first, "interval")) {
				sGroup.setInterval(stoull(val.second.data()));
			} else if (boost::iequals(val.first, "queueSize")) {
				sGroup.setQueueSize(stoull(val.second.data()));
			} else if (boost::iequals(val.first, "minValues")) {
				sGroup.setMinValues(stoull(val.second.data()));
			} else if (boost::iequals(val.first, "mqttPart")) {
				sGroup.setMqttPart(val.second.data());
			} else if (boost::iequals(val.first, "sync")) {
				sGroup.setSync(to_bool(val.second.data()));
			} else if (boost::iequals(val.first, "disabled")) {
				sGroup.setDisabled(to_bool(val.second.data()));
			} else if (boost::iequals(val.first, _baseName)) {
				if (!isTemplate) {
					LOG(debug) << "  " << _baseName << " " << val.second.data();
				}

				SB_Ptr sensor = std::make_shared<SBase>(val.second.data());
				bool   overwriting = false;

				//Check if sensor with equal name is already present.
				//If yes, we do not want a new sensor but instead overwrite the
				//existing one.
				for (const auto &s : sGroup.getDerivedSensors()) {
					if (s->getName() == sensor->getName()) {
						sensor = s;
						overwriting = true;
						break;
					}
				}
				if (readSensorBase(*sensor, val.second, false)) {
					if (!overwriting) {
						sGroup.pushBackSensor(sensor);
					}
				} else if (!isTemplate) {
					LOG(warning) << _baseName << " " << sGroup.getGroupName() << "::"
						     << sensor->getName() << " could not be read! Omitting";
				}
			}
		}

		sensorGroup(sGroup, config);
		return true;
	}
	///@}

	///@name Overwrite in plugin
	///@{
	/**
     * Method responsible for reading plugin-specific sensor base values.
     *
     * @param s         The sensor base for which to set the values
     * @param config    A boost property (sub-)tree containing the sensor values
     */
	virtual void sensorBase(SBase &s, CFG_VAL config) = 0;

	/**
     * Method responsible for reading plugin-specific sensor group values.
     *
     * @param s         The sensor group for which to set the values
     * @param config    A boost property (sub-)tree containing the group values
     */
	virtual void sensorGroup(SGroup &s, CFG_VAL config) = 0;
	///@}

	///@name Utility
	///@{
	/**
     * @brief Store a sensor group internally.
     *
     * @details If a group with identical name is already present, it will be
     *          replaced.
     *
     * @param sGroup Group to store.
     */
	void storeSensorGroup(SG_Ptr sGroup) {
		_sensorGroups.push_back(sGroup);
		_sensorGroupInterfaces.push_back(sGroup);
	}

	/**
     * @brief Adjusts the names of the sensors in generated groups.
     *
     * @return True if successful, false otherwise.
     */
	virtual bool constructSensorTopics() {
		// Sensor names are adjusted according to the respective MQTT topics
		for (auto &g : _sensorGroups) {
			for (auto &s : g->acquireSensors()) {
				s->setMqtt(MQTTChecker::formatTopic(_mqttPrefix) +
					   MQTTChecker::formatTopic(g->getMqttPart()) +
					   MQTTChecker::formatTopic(s->getMqtt()));
				s->setName(s->getMqtt());
				SensorMetadata *sm = s->getMetadata();
				if (sm) {
					sm->setPublicName(s->getMqtt());
					sm->setPattern(s->getMqtt());
					sm->setIsVirtual(false);
					if(!sm->getInterval())
						sm->setInterval((uint64_t)g->getInterval() * 1000000);
					sm->setDelta(s->getDelta());
				}
			}
			g->releaseSensors();
		}
		return true;
	}
	///@}

	std::string _groupName;
	std::string _baseName;

	std::vector<SG_Ptr> _sensorGroups;
	sBaseMap_t          _templateSensorBases;
	sGroupMap_t         _templateSensorGroups;
};

#endif /* SRC_CONFIGURATORTEMPLATE_H_ */
