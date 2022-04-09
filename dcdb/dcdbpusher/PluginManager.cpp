//================================================================================
// Name        : PluginManager.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Class bundling all logic related to dcdb-pusher plugins.
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

#include "PluginManager.h"

#include <dlfcn.h>

#include "mqttchecker.h"

using namespace std;

PluginManager::PluginManager(boost::asio::io_context &io, const pluginSettings_t &pluginSettings)
    : _pluginSettings(pluginSettings),
      _cfgFilePath("./"),
	  _io(io) {}

PluginManager::~PluginManager() {
	for (const auto &p : _plugins)
		p.destroy(p.configurator);
	_plugins.clear();
}

bool PluginManager::loadPlugin(const string &name,
			       const string &pluginPath,
			       const string &config) {

	LOG(info) << "Loading plugin " << name << "...";
	string pluginLib;    //path to the plugin-lib
	string pluginConfig; //path to config file for plugin

	// build plugin path
	pluginLib = "libdcdbplugin_" + name; //TODO add version information?
#if __APPLE__
	pluginLib += ".dylib";
#else
	pluginLib += ".so";
#endif
	if (pluginPath != "") {
		if (pluginPath[pluginPath.length() - 1] != '/') {
			pluginLib = "/" + pluginLib;
		}
		pluginLib = pluginPath + pluginLib;
	}

	//if config-path not specified we will look for pluginName.conf in the dcdbpusher.conf directory
	if (config == "") {
		pluginConfig = _cfgFilePath + name + ".conf";
	} else {
		if (config[0] == '/') {
			pluginConfig = config;
		} else {
			pluginConfig = _cfgFilePath + config;
		}
	}

	//open plugin
	//dl-code based on http://tldp.org/HOWTO/C++-dlopen/thesolution.html

	//TODO switch to C++17 std::filesystem::exists
	//check if config file exists
	FILE *file = NULL;
	if (!(file = fopen(pluginConfig.c_str(), "r"))) {
		LOG(warning) << pluginConfig << " not found. Omitting";
		return false;
	}
	fclose(file);

	pusherPlugin_t plugin;
	plugin.id = name;
	plugin.DL = NULL;
	plugin.configurator = NULL;

	//plugin.conf exists --> open libdcdbplugin_NAME.so and read config
	LOG(info) << pluginConfig << " found";
	plugin.DL = dlopen(pluginLib.c_str(), RTLD_NOW);
	if (!plugin.DL) {
		LOG(error) << "Cannot load " << plugin.id << "-library: " << dlerror();
		return false;
	}
	//reset errors
	dlerror();

	//set dynLib-struct
	plugin.create = (create_t *)dlsym(plugin.DL, "create");
	const char *dlsym_error = dlerror();
	if (dlsym_error) {
		LOG(error) << "Cannot load symbol create for " << plugin.id << ": " << dlsym_error;
		return false;
	}

	plugin.destroy = (destroy_t *)dlsym(plugin.DL, "destroy");
	dlsym_error = dlerror();
	if (dlsym_error) {
		LOG(error) << "Cannot load symbol destroy for " << plugin.id << ": " << dlsym_error;
		return false;
	}

	plugin.configurator = plugin.create();
	//set prefix to global prefix (may be overwritten)
	plugin.configurator->setGlobalSettings(_pluginSettings);
	//read in config
	if (!(plugin.configurator->readConfig(pluginConfig))) {
		LOG(error) << "Plugin " << plugin.id << " could not read configuration!";
		return false;
	}

	// returning an empty vector may indicate problems with the config file
	if (plugin.configurator->getSensorGroups().size() == 0) {
		LOG(warning) << "Plugin " << plugin.id << " created no sensors!";
	}

	//check if an MQTT-suffix was assigned twice
	if (!checkTopics(plugin)) {
		LOG(error) << "Problematic MQTT topics or sensor names, please check your config files!";
		removeTopics(plugin);
		return false;
	}

	//save dl-struct
	_plugins.push_back(plugin);
	LOG(info) << "Plugin " << plugin.id << " " << plugin.configurator->getVersion() << " loaded!";

	return true;
}

void PluginManager::unloadPlugin(const string &id) {
	for (auto it = _plugins.begin(); it != _plugins.end(); ++it) {
		if (it->id == id || id == "") {
			for (const auto &g : it->configurator->getSensorGroups())
				g->stop();
			for (const auto &g : it->configurator->getSensorGroups())
				g->wait();

			removeTopics(*it);

			if (it->configurator) {
				it->destroy(it->configurator);
			}
			//if (it->DL) {
			//    dlclose(it->DL);
			//}

			if (id != "") {
				//only erase if we immediately return or otherwise our iterator would be invalidated
				_plugins.erase(it);
				return;
			}
		}
	}

	if (id == "") {
		_plugins.clear();
	}
}

bool PluginManager::initPlugin(const string &id) {
	bool found = false;

	for (const auto &p : _plugins) {
		if (p.id == id || id == "") {
			LOG(info) << "Init " << p.id << " plugin";
			for (const auto &g : p.configurator->getSensorGroups()) {
				found = true;
				g->init(_io);
			}
		}
	}

	if (!found) {
		LOG(warning) << "Could not find plugin " << id << " to initialize!";
	}
	return found;
}

bool PluginManager::startPlugin(const string &id) {
	bool found = false;

	for (const auto &p : _plugins) {
		if (p.id == id || id == "") {
			LOG(info) << "Start " << p.id << " plugin";
			for (const auto &g : p.configurator->getSensorGroups()) {
				found = true;
				g->start();
			}
		}
	}

	if (!found) {
		LOG(warning) << "Could not find plugin " << id << " to start!";
	}
	return found;
}

bool PluginManager::stopPlugin(const string &id) {
	bool found = false;

	for (const auto &p : _plugins) {
		if (p.id == id || id == "") {
			LOG(info) << "Stop " << p.id << " plugin";
			// Issuing stop command
			for (const auto &g : p.configurator->getSensorGroups()) {
				found = true;
				g->stop();
			}
			// Waiting for sensor group termination
			for (const auto &g : p.configurator->getSensorGroups()) {
                g->wait();
            }
		}
	}

	if (!found) {
		LOG(warning) << "Could not find plugin " << id << " to stop!";
	}
	return found;
}

bool PluginManager::reloadPluginConfig(const string &id) {
	for (const auto &p : _plugins) {
		if (p.id == id) {
			// Removing obsolete MQTT topics
			removeTopics(p);
			if (p.configurator->reReadConfig()) {
				// Perform checks on MQTT topics
				if (!checkTopics(p)) {
					LOG(warning) << "Plugin " + id + ": problematic MQTT topics or sensor names, please check your config files!";
					removeTopics(p);
					p.configurator->clearConfig();
					return false;
				} else {
					LOG(info) << "Plugin " + id + ": Configuration reloaded.";
					return true;
				}
			} else {
				LOG(warning) << "Plugin " << id << ": Could not reload configuration!";
				return false;
			}
		}
	}
	return false;
}

bool PluginManager::checkTopics(const pusherPlugin_t &p) {
	MQTTChecker &mqttCheck = MQTTChecker::getInstance();
	bool         validTopics = true;

	for (const auto &g : p.configurator->getSensorGroups()) {
		if (!g->isDisabled()) {
			if (!mqttCheck.checkGroup(g->getGroupName())) {
				validTopics = false;
			}
			for (const auto &s : g->acquireSensors()) {
				if (!mqttCheck.checkTopic(s->getMqtt()) || !mqttCheck.checkName(s->getName())) {
					validTopics = false;
				}
			}
			g->releaseSensors();
		}
	}

	return validTopics;
}

void PluginManager::removeTopics(const pusherPlugin_t &p) {
	MQTTChecker &mqttCheck = MQTTChecker::getInstance();
	for (const auto &g : p.configurator->getSensorGroups()) {
		mqttCheck.removeGroup(g->getGroupName());
		for (const auto &s : g->acquireSensors()) {
			mqttCheck.removeTopic(s->getMqtt());
			mqttCheck.removeName(s->getName());
		}
		g->releaseSensors();
	}
}
