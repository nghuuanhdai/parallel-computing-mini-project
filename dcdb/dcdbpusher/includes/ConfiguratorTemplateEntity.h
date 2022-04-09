//================================================================================
// Name        : ConfiguratorTemplateEntity.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Interface template for plugin configurators with entities.
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

#ifndef DCDBPUSHER_INCLUDES_CONFIGURATORTEMPLATEENTITY_H_
#define DCDBPUSHER_INCLUDES_CONFIGURATORTEMPLATEENTITY_H_

#include "ConfiguratorTemplate.h"
#include "EntityInterface.h"

/**
 * @brief Interface template for plugin configurator implementations with
 *        entities.
 *
 * @details Inherits from the ConfiguratorTemplate class and extends it for
 *          entity support
 *
 * @ingroup pusherplugins
 */
template <class SBase, class SGroup, class SEntity>
class ConfiguratorTemplateEntity : public ConfiguratorTemplate<SBase, SGroup> {
	//the template shall only be instantiated for entities which derive from EntityInterface
	static_assert(std::is_base_of<EntityInterface, SEntity>::value, "SEntity must derive from EntityInterface!");

      protected:
	//mention all required parent attributes and functions here to avoid compiler errors
	using ConfiguratorInterface::_cfgPath;
	using ConfiguratorInterface::_mqttPrefix;
	using ConfiguratorInterface::readGlobal;
	using ConfiguratorTemplate<SBase, SGroup>::_baseName;
	using ConfiguratorTemplate<SBase, SGroup>::_groupName;
	using ConfiguratorTemplate<SBase, SGroup>::_sensorGroups;
	using ConfiguratorTemplate<SBase, SGroup>::_templateSensorBases;
	using ConfiguratorTemplate<SBase, SGroup>::_templateSensorGroups;
	using ConfiguratorTemplate<SBase, SGroup>::readSensorBase;
	using ConfiguratorTemplate<SBase, SGroup>::readSensorGroup;
	using ConfiguratorTemplate<SBase, SGroup>::storeSensorGroup;

	//TODO use smart pointers
	typedef std::map<std::string, SEntity *> sEntityMap_t;

	using SB_Ptr = std::shared_ptr<SBase>;
	using SG_Ptr = std::shared_ptr<SGroup>;

      public:
	ConfiguratorTemplateEntity()
	    : ConfiguratorTemplate<SBase, SGroup>(),
	      _entityName("INVALID") {}

	virtual ~ConfiguratorTemplateEntity() {
		for (auto e : _sensorEntitys) {
			delete e;
		}
		for (auto te : _templateSensorEntitys) {
			delete te.second;
		}
		// SensorGroups must be cleared before Entities
		_sensorGroups.clear();
		_sensorEntitys.clear();
		_templateSensorEntitys.clear();
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

		//read entities and template stuff
		BOOST_FOREACH (boost::property_tree::iptree::value_type &val, cfg) {
			//template entity
			if (boost::iequals(val.first, "template_" + _entityName)) {
				LOG(debug) << "Template " << _entityName << " \"" << val.second.data() << "\"";
				if (!val.second.empty()) {
					//name of an entity is only used to identify templates and is not stored otherwise
					SEntity *entity = new SEntity(val.second.data());
					if (readSensorEntity(*entity, val.second, true)) {
						auto ret = _templateSensorEntitys.insert(std::pair<std::string, SEntity *>(val.second.data(), entity));
						if (!ret.second) {
							LOG(warning) << "Template " << _entityName << " "
								     << val.second.data() << " already exists! Omitting...";
							delete entity;
						}
					} else {
						LOG(warning) << "Template " << _entityName << " \""
							     << val.second.data() << "\" has bad values! Ignoring...";
						delete entity;
					}
				}
				//template group
			} else if (boost::iequals(val.first, "template_" + _groupName)) {
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
				//entity
			} else if (boost::iequals(val.first, _entityName)) {
				LOG(debug) << _entityName << " \"" << val.second.data() << "\"";
				if (!val.second.empty()) {
					SEntity *entity = new SEntity(val.second.data());
					if (readSensorEntity(*entity, val.second)) {
						_sensorEntitys.push_back(entity);
					} else {
						LOG(warning) << _entityName << " \""
							     << val.second.data() << "\" has bad values! Ignoring...";
						delete entity;
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
	void clearConfig() final override {
		ConfiguratorTemplate<SBase, SGroup>::clearConfig();

		//clean up remaining entity stuff
		for (auto e : _sensorEntitys) {
			delete e;
		}
		for (auto te : _templateSensorEntitys) {
			delete te.second;
		}
		_sensorEntitys.clear();
		_templateSensorEntitys.clear();
	}

	/**
     * @brief Print configuration.
     *
     * @param ll Log severity level to be used from logger.
     */
	void printConfig(LOG_LEVEL ll) final override {
		ConfiguratorInterface::printConfig(ll);

		//prints plugin specific configurator attributes and entities if present
		this->printConfiguratorConfig(ll);

		LOG_VAR(ll) << "    " << _entityName << "s:";
		if (_sensorEntitys.size() != 0) {
			for (auto e : _sensorEntitys) {
				e->printConfig(ll, 8);
				LOG_VAR(ll) << "            Sensor Groups:";
				for (auto g : _sensorGroups) {
					if (g->getEntity() == e) {
						g->printConfig(ll, 16);
					}
				}
			}
		} else {
			LOG_VAR(ll) << "            No " << _entityName << "s present!";
		}
	}

      protected:
	///@name Template-internal use only
	///@{
	/**
     * @brief Read common values of a sensor entity.
     *
     * @details Reads and sets the common base values of a sensor entity
     *          (currently none), then calls sensorEntity() to read plugin
     *          specific values.
     *
     * @param sEntity    The aggregating entity for which to set the values.
     * @param config     A boost property (sub-)tree containing the values.
     * @param isTemplate Indicate if sEntity is a template. If so, also store
     *                   the corresponding sGroups in the template map.
     *
     * @return  True on success, false otherwise
     */
	bool readSensorEntity(SEntity &sEntity, CFG_VAL config, bool isTemplate = false) {
		//first check if default entity is given
		boost::optional<boost::property_tree::iptree &> def = config.get_child_optional("default");
		if (def) {
			//we copy all values from default
			LOG(debug) << "  Using \"" << def.get().data() << "\" as default.";
			auto it = _templateSensorEntitys.find(def.get().data());
			if (it != _templateSensorEntitys.end()) {
				sEntity = *(it->second);
				sEntity.setName(config.data());
				for (auto g : _templateSensorGroups) {
					if (it->second == g.second->getEntity()) {
						SG_Ptr group = std::make_shared<SGroup>(*(g.second));
						group->setEntity(&sEntity);
						if (group->getGroupName().size() > 0) {
							group->setGroupName(sEntity.getName() + "::" + group->getGroupName());
						} else {
							group->setGroupName(sEntity.getName());
						}
						storeSensorGroup(group);
					}
				}
			} else {
				LOG(warning) << "Template " << _entityName << "\""
					     << def.get().data() << "\" not found! Using standard values.";
			}
		}

		//read in values inherited from EntityInterface
		BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
			if (boost::iequals(val.first, "mqttPart")) {
				sEntity.setMqttPart(val.second.data());
			} else if (boost::iequals(val.first, "disabled")) {
				sEntity.setDisabled(to_bool(val.second.data()));
			}
		}

		sensorEntity(sEntity, config);

		BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
			if (boost::iequals(val.first, _groupName)) {
				LOG(debug) << "  " << _groupName << " " << val.second.data();
				if (!val.second.empty()) {
					if (isTemplate) {
						SGroup *group = new SGroup(val.second.data());
						if (readSensorGroup(*group, val.second)) {
							group->setEntity(&sEntity);
							auto ret = _templateSensorGroups.insert(std::pair<std::string, SGroup *>(sEntity.getName() + "::" + group->getGroupName(), group));
							if (!ret.second) {
								LOG(warning) << "Template " << _groupName << " "
									     << sEntity.getName() + "::" + group->getGroupName() << " already exists! Omitting...";
								delete group;
							}
						} else {
							LOG(warning) << _groupName << " " << group->getGroupName()
								     << " could not be read! Omitting";
							delete group;
						}
					} else {
						SG_Ptr group = std::make_shared<SGroup>(val.second.data());
                        group->setEntity(&sEntity);
						bool   overwriting = false;

						if (group->getGroupName().size() > 0) {
							group->setGroupName(sEntity.getName() + "::" + group->getGroupName());
						} else {
							group->setGroupName(sEntity.getName());
						}

						//Check if group with equal name is already present.
						//If yes, we do not want a new group but instead
						//overwrite the existing one.
						for (const auto &g : _sensorGroups) {
							if ((g->getGroupName() == group->getGroupName()) && (&sEntity == g->getEntity())) {
								group = g;
								overwriting = true;
								break;
							}
						}

						if (readSensorGroup(*group, val.second) && !overwriting) {
							storeSensorGroup(group);
						} else {
							LOG(warning) << _groupName << " " << group->getGroupName()
								     << " could not be read! Omitting";
						}
					}
				}
			} else if (boost::iequals(val.first, "single_" + _baseName)) {
				LOG(debug) << "Single " << _baseName << " \"" << val.second.data() << "\"";
				if (!val.second.empty()) {
					if (isTemplate) {
						SGroup *group = new SGroup(val.second.data());
						//group which consists of only one sensor
						if (readSensorGroup(*group, val.second)) {
							group->setEntity(&sEntity);
							SB_Ptr sensor = std::make_shared<SBase>(val.second.data());
							if (readSensorBase(*sensor, val.second)) {
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
							LOG(warning) << "Single " << _baseName << " \""
								     << val.second.data() << "\" has bad values! Ignoring...";
							delete group;
						}
					} else {
						SG_Ptr group = std::make_shared<SGroup>(val.second.data());
						bool   overwriting = false;

						if (group->getGroupName().size() > 0) {
							group->setGroupName(sEntity.getName() + "::" + group->getGroupName());
						} else {
							group->setGroupName(sEntity.getName());
						}

						//Check if group with equal name is already present.
						//If yes, we do not want a new group but instead
						//overwrite the existing one.
						for (const auto &g : _sensorGroups) {
							if ((g->getGroupName() == group->getGroupName()) && (&sEntity == g->getEntity())) {
								group = g;
								overwriting = true;
								break;
							}
						}

						//group which consists of only one sensor
						if (readSensorGroup(*group, val.second)) {
							group->setEntity(&sEntity);
							SB_Ptr sensor;
							//perhaps one sensor is already present because it was copied from the template group
							if (group->getDerivedSensors().size() != 0) {
								sensor = group->getDerivedSensors()[0];
								sensor->setName(val.second.data());
								if (readSensorBase(*sensor, val.second) && !overwriting) {
									storeSensorGroup(group);
								} else {
									LOG(warning) << "Single " << _baseName << " "
										     << val.second.data() << " could not be read! Omitting";
								}
							} else {
								sensor = std::make_shared<SBase>(val.second.data());
								if (readSensorBase(*sensor, val.second)) {
									group->pushBackSensor(sensor);
									if (!overwriting) {
										storeSensorGroup(group);
									}
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
				}
			}
		}

		return true;
	}
	///@}

	///@name Overwrite in plugin
	///@{
	/**
     * Method responsible for reading plugin-specific sensor entity values.
     *
     * @param s         The sensor entity for which to set the values
     * @param config    A boost property (sub-)tree containing the entity values
     */
	virtual void sensorEntity(SEntity &s, CFG_VAL config) = 0;
	///@}

	///@name Utility
	///@{
	/**
     * @brief Adjusts the names of the sensors in generated groups.
     *
     * @return True if successful, false otherwise.
     */
	virtual bool constructSensorTopics() final override {
		// Sensor names are adjusted according to the respective MQTT topics
		for (auto &g : _sensorGroups) {
			for (auto &s : g->acquireSensors()) {
				std::string mqtt = MQTTChecker::formatTopic(_mqttPrefix);
				if (g->getEntity()) {
					mqtt.append(MQTTChecker::formatTopic(g->getEntity()->getMqttPart()));
				}
				mqtt.append(MQTTChecker::formatTopic(g->getMqttPart()));
				mqtt.append(MQTTChecker::formatTopic(s->getMqtt()));
				s->setMqtt(mqtt);
				s->setName(mqtt);
				SensorMetadata *sm = s->getMetadata();
				if (sm) {
					sm->setPublicName(s->getMqtt());
					sm->setPattern(s->getMqtt());
					sm->setIsVirtual(false);
					if (!sm->getInterval())
						sm->setInterval((uint64_t)g->getInterval() * 1000000);
					sm->setDelta(s->getDelta());

				}
			}
			g->releaseSensors();
		}
		return true;
	}
	///@}

	std::string _entityName;

	LOGGER lg;
	std::vector<SEntity *> _sensorEntitys;
	sEntityMap_t           _templateSensorEntitys;
};

#endif /* DCDBPUSHER_INCLUDES_CONFIGURATORTEMPLATEENTITY_H_ */
