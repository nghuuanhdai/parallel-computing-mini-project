//================================================================================
// Name        : Configuration.cpp
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

#include "Configuration.h"
#include <string>
#include <unistd.h>

using namespace std;

bool Configuration::readAdditionalValues(boost::property_tree::iptree::value_type &global) {
	// ----- READING ADDITIONAL GLOBAL SETTINGS -----
	if (boost::iequals(global.first, "mqttBroker")) {
		brokerHost = parseNetworkHost(global.second.data());
		brokerPort = parseNetworkPort(global.second.data()) == "" ? DEFAULT_BROKERPORT : stoi(parseNetworkPort(global.second.data()));
	} else if (boost::iequals(global.first, "qosLevel")) {
		qosLevel = stoi(global.second.data());
		if (qosLevel > 2 || qosLevel < 0)
			qosLevel = 1;
	} else if (boost::iequals(global.first, "maxInflightMsgNum")) {
		maxInflightMsgNum = stoull(global.second.data());
	} else if (boost::iequals(global.first, "maxQueuedMsgNum")) {
		maxQueuedMsgNum = stoull(global.second.data());
	} else if (boost::iequals(global.first, "maxMsgNum")) {
		maxMsgNum = stoi(global.second.data());
	} else {
		return false;
	}
	return true;
}

bool Configuration::readPlugins(PluginManager &pluginManager) {
	std::string globalConfig = cfgFilePath;
	globalConfig.append(cfgFileName);

	boost::property_tree::iptree cfg;
	try {
		boost::property_tree::read_info(globalConfig, cfg);
	} catch (boost::property_tree::info_parser_error &e) {
		LOG(error) << "Error while parsing plugins from " << globalConfig << ": " << e.what();
		return false;
	}

	pluginManager.setCfgFilePath(cfgFilePath);
	//read plugins
	BOOST_FOREACH (boost::property_tree::iptree::value_type &plugin, cfg.get_child("plugins")) {
		if (boost::iequals(plugin.first, "plugin")) {
			if (!plugin.second.data().empty()) {
				std::string pluginName = plugin.second.data();
				std::string pluginConfig = ""; // path to config file for plugin
				std::string pluginPath = "";   // path to plugin

				//LOG(info) << "Read plugin " << pluginName << "...";

				BOOST_FOREACH (boost::property_tree::iptree::value_type &val, plugin.second) {
					if (boost::iequals(val.first, "path")) {
						pluginPath = val.second.data();
					} else if (boost::iequals(val.first, "config")) {
						pluginConfig = val.second.data();
					} else {
						LOG(warning) << "  Value \"" << val.first << "\" not recognized. Omitting";
					}
				}

				if (!pluginManager.loadPlugin(pluginName, pluginPath, pluginConfig)) {
					LOG(error) << "Could not load plugin " << pluginName;
					pluginManager.unloadPlugin();
					return false;
				}
			}
		}
	}
	return true;
}
