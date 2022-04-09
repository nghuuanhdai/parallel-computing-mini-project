//================================================================================
// Name        : Configuration.h
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Class responsible for reading dcdb-pusher specific configuration.
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

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "globalconfiguration.h"

#include <set>

#include "PluginManager.h"

#define DEFAULT_BROKERPORT 1883
#define DEFAULT_BROKERHOST "127.0.0.1"

/**
 * @brief Class responsible for reading dcdb-pusher specific configuration.
 *
 * @ingroup pusher
 */
class Configuration : public GlobalConfiguration {

      public:
	/**
     * Create new Configuration. Sets global config file to read from to cfgFile.
     * 
     * @param cfgFilePath	Path to where all config-files are located
     * @param cfgFileName   Name of the file containing the config
     */
	Configuration(const std::string &cfgFilePath, const std::string &cfgFileName)
	    : GlobalConfiguration(cfgFilePath, cfgFileName) {}

	virtual ~Configuration() {}

	/**
	 * Reads the plugin configuration section from global.conf (located at _cfgFilePath).
	 * Triggers the pluginManager to load plugins as required.
	 *
	 * @param pluginManager The pluginManager object to store plugins.
	 *
	 * @return	true on success, false otherwise
	 */
	bool readPlugins(PluginManager &pluginManager);

	// Additional configuration parameters to be parsed and stored in the global block
	int          qosLevel = 1;
	unsigned int maxInflightMsgNum = 20;
	unsigned int maxQueuedMsgNum = 0;
	int          brokerPort = DEFAULT_BROKERPORT;
	std::string  brokerHost = DEFAULT_BROKERHOST;
	int          maxMsgNum = 0;

      protected:
	bool readAdditionalValues(boost::property_tree::iptree::value_type &global) override;
};

#endif /* CONFIGURATION_H_ */
