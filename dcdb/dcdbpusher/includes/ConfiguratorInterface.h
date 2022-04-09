//================================================================================
// Name        : ConfiguratorInterface.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Abstract interface defining plugin configurator functionality.
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

#ifndef SRC_CONFIGURATORINTERFACE_H_
#define SRC_CONFIGURATORINTERFACE_H_

#include <string>
#include <vector>

#include "SensorGroupTemplate.h"
#include "globalconfiguration.h"
#include "version.h"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>

//#define STRCMP(node,str) boost::iequals(node.first,str) //DEPRECATED
#define CFG_VAL boost::property_tree::iptree &

#define ADD BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config)
#define ATTRIBUTE(name, setter)                        \
	do {                                           \
		if (boost::iequals(val.first, name)) { \
			s.setter(val.second.data());   \
		}                                      \
	} while (0)
#define SETTING(name) if (boost::iequals(val.first, name))

#define DEFAULT_CACHE_INTERVAL 900000

/**
 * @brief Abstract interface defining plugin configurator functionality.
 *
 * @details Plugin configurator should not implement this interface themselves
 *          but inherit the ConfiguratorTemplate instead.
 *
 * @ingroup pusherplugins
 */
class ConfiguratorInterface {

      protected:
	const char COMMA = ',';
	const char OPEN_SQBRKET = '[';
	const char CLOSE_SQBRKET = ']';
	const char DASH = '-';

      public:
	ConfiguratorInterface()
	    : _cfgPath(""),
	      _mqttPrefix(""),
	      _cacheInterval(DEFAULT_CACHE_INTERVAL) {}

	ConfiguratorInterface(const ConfiguratorInterface &) = delete;

	virtual ~ConfiguratorInterface() {
		_sensorGroupInterfaces.clear();
	}

	ConfiguratorInterface &operator=(const ConfiguratorInterface &) = delete;

	/**
	 * @brief Read in plugin configuration.
	 *
	 * @param cfgPath Path + file name of configuration file to read from.
	 */
	virtual bool readConfig(std::string cfgPath) = 0;

	/**
	 * @brief Clear internal storage and return plugin in unconfigured state.
	 */
	virtual void clearConfig() {
		_sensorGroupInterfaces.clear();
	}

	/**
     * @brief Clear internal storage and read in the configuration again.
     *
     * @return True on success, false otherwise
     */
	bool reReadConfig() {
		clearConfig();

		//back to the very beginning
		return readConfig(_cfgPath);
	}

	/**
     * @brief Sets internal variables with the ones provided by pluginSettings.
     *        This method should be called once after constructing a configurator
     *        to provide him with the global default values.
     *
     * @param pluginSettings Struct with global default settings for the plugins.
     */
	void setGlobalSettings(const pluginSettings_t &pluginSettings) {
		_mqttPrefix = pluginSettings.mqttPrefix;
		_cacheInterval = pluginSettings.cacheInterval;
		derivedSetGlobalSettings(pluginSettings);
	}

	/**
     * @brief Get all sensor groups.
     *
     * @return Vector containing pointers to all sensor groups of this plugin.
     */
	std::vector<SGroupPtr> &getSensorGroups() {
		return _sensorGroupInterfaces;
	}

	/**
     * @brief Get current DCDB version.
     *
     * @return Version number as string.
     */
	std::string getVersion() {
		return std::string(VERSION);
	}

	/**
     * @brief Print configuration.
     *
     * @param ll Log severity level to be used from logger.
     */
	virtual void printConfig(LOG_LEVEL ll) {
		LOG_VAR(ll) << "    General: ";
		if (_mqttPrefix != "") {
			LOG_VAR(ll) << "        MQTT-Prefix: " << _mqttPrefix;
		} else {
			LOG_VAR(ll) << "        MQTT-Prefix: DEFAULT";
		}
		if (_cacheInterval != DEFAULT_CACHE_INTERVAL) {
			LOG_VAR(ll) << "        Cache interval: " << _cacheInterval << " ms";
		} else {
			LOG_VAR(ll) << "        Cache interval: DEFAULT";
		}
	}

      protected:
	///@name Overwrite on demand
	///@{
	/**
     * @brief Set global values specifically for plugin.
     *
     * @param pluginSettings The struct with global default plugin settings
     */
	virtual void derivedSetGlobalSettings(const pluginSettings_t &pluginSettings) {}

	/**
     * @brief Print information about configurable configurator attributes (or
     *        nothing if no such attributes are required).
     *
     * @param ll Severity level to log with
     */
	virtual void printConfiguratorConfig(LOG_LEVEL ll) {
		//Overwrite if necessary
	}

	/**
     * @brief Overwrite to read in plugin-specific global values.
     *
     * @param config A boost property (sub-)tree containing the global values.
     */
	virtual void global(CFG_VAL config) {}
	///@}

	///@name Utility
	///@{
	/**
     * @brief Read in global values.
     *
     * @details Reads in global interface values and then calls global().
     *
     * @param config A boost property (sub-)tree containing the global values.
     */
	bool readGlobal(CFG_VAL config) {
		boost::optional<boost::property_tree::iptree &> globalVals = config.get_child_optional("global");
		if (globalVals) {
			BOOST_FOREACH (boost::property_tree::iptree::value_type &global, config.get_child("global")) {
				if (boost::iequals(global.first, "mqttprefix")) {
					_mqttPrefix = global.second.data();
				} else if (boost::iequals(global.first, "cacheInterval")) {
					_cacheInterval = stoul(global.second.data());
					_cacheInterval *= 1000;
				}
			}
			global(config.get_child("global"));
		}
		return true;
	}

	/**
     * @brief Try to parse the given cpuString as integer numbers.
     *
     * @details On success, the specified numbers will be inserted into a set,
     *          which will be returned. On failure, an empty set is returned. A
     *          set is used to maintain uniqueness and an ascending order among
     *          the numbers although this is not strictly required.
     *
     * @param cpuString String which specifies a range and/or set of numbers
     *        (e.g. "1,2,3-5,7-9,10")
     *
     * @return A set of integers as specified in the cpuString. If the string
     *         could not be parsed the set will be empty.
     */
	std::set<int> parseCpuString(const std::string &cpuString) {
		std::set<int> cpus;
		int           maxCpu = 512;

		std::vector<std::string> subStrings;

		std::stringstream ssComma(cpuString);
		std::string       item;
		while (std::getline(ssComma, item, ',')) {
			subStrings.push_back(item);
		}

		for (auto s : subStrings) {
			if (s.find('-') != std::string::npos) { //range of values (e.g. 1-5) specified
				std::stringstream ssHyphen(s);
				std::string       min, max;
				std::getline(ssHyphen, min, '-');
				std::getline(ssHyphen, max);

				try {
					int minVal = stoi(min);
					int maxVal = stoi(max);

					for (int i = minVal; i <= maxVal; i++) {
						if (i >= 0 && i < maxCpu) {
							cpus.insert((int)i);
						}
					}
				} catch (const std::exception &e) {
					LOG(debug) << "Could not parse values \"" << min << "-" << max << "\"";
				}
			} else { //single value
				try {
					int val = stoi(s);
					if (val >= 0 && val < maxCpu) {
						cpus.insert((int)val);
					}
				} catch (const std::exception &e) {
					LOG(debug) << "Could not parse value \"" << s << "\"";
				}
			}
		}

		if (cpus.empty()) {
			LOG(warning) << "  CPUs could not be parsed!";
		} else {
			std::stringstream sstream;
			sstream << "  CPUS: ";
			for (auto i : cpus) {
				sstream << i << ", ";
			}
			std::string msg = sstream.str();
			msg.pop_back();
			msg.pop_back();
			LOG(debug) << msg;
		}
		return cpus;
	}
	///@}

	std::string  _cfgPath;       ///< Path + name of config file to read from
	std::string  _mqttPrefix;    ///< Global MQTT prefix
	unsigned int _cacheInterval; ///< Time interval in ms all sensors should cache

	std::vector<SGroupPtr> _sensorGroupInterfaces; ///< Sensor group storage

	LOGGER lg; ///< Personal logging instance
};

//typedef for more readable usage of create()- and destroy()-methods, required for dynamic libraries
typedef ConfiguratorInterface *create_t();
typedef void                   destroy_t(ConfiguratorInterface *);

#endif /* SRC_CONFIGURATORINTERFACE_H_ */
