//================================================================================
// Name        : RestAPI.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : RESTful API implementation for dcdb-pusher
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

#include "RestAPI.h"

#include <sstream>
#include <string>
#include <signal.h>
#include "version.h"

#include <boost/property_tree/ptree.hpp>

#define stdBind(fun) std::bind(&RestAPI::fun,         \
			       this,                  \
			       std::placeholders::_1, \
			       std::placeholders::_2, \
			       std::placeholders::_3)

RestAPI::RestAPI(serverSettings_t         settings,
		 PluginManager *          pluginManager,
		 MQTTPusher *             mqttPusher,
		 OperatorManager *        manager,
		 boost::asio::io_context &io)
    : RESTHttpsServer(settings, io),
      _pluginManager(pluginManager),
      _mqttPusher(mqttPusher),
      _manager(manager) {

	addEndpoint("/help", {http::verb::get, stdBind(GET_help)});
	addEndpoint("/version", {http::verb::get, stdBind(GET_version)});
	addEndpoint("/plugins", {http::verb::get, stdBind(GET_plugins)});
	addEndpoint("/sensors", {http::verb::get, stdBind(GET_sensors)});
	addEndpoint("/average", {http::verb::get, stdBind(GET_average)});

	addEndpoint("/quit", {http::verb::put, stdBind(PUT_quit)});
	addEndpoint("/load", {http::verb::put, stdBind(PUT_load)});
	addEndpoint("/unload", {http::verb::put, stdBind(PUT_unload)});
	addEndpoint("/reload", {http::verb::put, stdBind(PUT_reload)});

	addEndpoint("/start", {http::verb::post, stdBind(POST_start)});
	addEndpoint("/stop", {http::verb::post, stdBind(POST_stop)});

	_manager->addRestEndpoints(this);

	addEndpoint("/analytics/reload", {http::verb::put, stdBind(PUT_analytics_reload)});
	addEndpoint("/analytics/load", {http::verb::put, stdBind(PUT_analytics_load)});
	addEndpoint("/analytics/unload", {http::verb::put, stdBind(PUT_analytics_unload)});
	addEndpoint("/analytics/navigator", {http::verb::put, stdBind(PUT_analytics_navigator)});
}

void RestAPI::GET_help(endpointArgs) {
	res.body() = restCheatSheet + _manager->restCheatSheet;
	res.result(http::status::ok);
}

void RestAPI::GET_version(endpointArgs) {
    res.body() = "dcdbpusher " + std::string(VERSION);
    res.result(http::status::ok);
}

void RestAPI::GET_plugins(endpointArgs) {
	std::ostringstream data;
	if (getQuery("json", queries) == "true") {
		boost::property_tree::ptree root, plugins;
		for (const auto &p : _pluginManager->getPlugins()) {
			plugins.put(p.id, "");
		}
		root.add_child("plugins", plugins);
		boost::property_tree::write_json(data, root, true);
	} else {
		for (const auto &p : _pluginManager->getPlugins()) {
			data << p.id << "\n";
		}
	}
	res.body() = data.str();
	res.result(http::status::ok);
}

void RestAPI::GET_sensors(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);

	if (!hasPlugin(plugin, res)) {
		return;
	}

	for (const auto &p : _pluginManager->getPlugins()) {
		if (p.id == plugin) {
			std::ostringstream data;
			if (getQuery("json", queries) == "true") {
				boost::property_tree::ptree root, sensors;

				for (const auto &g : p.configurator->getSensorGroups()) {
					boost::property_tree::ptree group;
					for (const auto &s : g->acquireSensors()) {
						group.push_back(boost::property_tree::ptree::value_type("", boost::property_tree::ptree(s->getMqtt())));
					}
					g->releaseSensors();
					sensors.add_child(g->getGroupName(), group);
				}
				root.add_child(p.id, sensors);
				boost::property_tree::write_json(data, root, true);
			} else {
				for (const auto &g : p.configurator->getSensorGroups()) {
					for (const auto &s : g->acquireSensors()) {
						data << g->getGroupName() << "::" << s->getMqtt() << "\n";
					}
					g->releaseSensors();
				}
			}
			res.body() = data.str();
			res.result(http::status::ok);
			return;
		}
	}
}

void RestAPI::GET_average(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);
	const std::string sensor = getQuery("sensor", queries);
	const std::string interval = getQuery("interval", queries);

	if (plugin == "" || sensor == "") {
		res.body() = "Request malformed: plugin or sensor query missing\n";
		res.result(http::status::bad_request);
		return;
	}

	uint64_t time = 0;

	if (interval != "") {
		try {
			time = std::stoul(interval);
		} catch (const std::exception &e) {
			RESTAPILOG(warning) << "Bad interval query: " << e.what();
			res.body() = "Bad interval query!\n";
			res.result(http::status::bad_request);
			return;
		}
	}

	res.body() = "Plugin not found!\n";
	res.result(http::status::not_found);

	for (const auto &p : _pluginManager->getPlugins()) {
		if (p.id == plugin) {
			res.body() = "Sensor not found!\n";
			for (const auto &g : p.configurator->getSensorGroups()) {
				for (const auto &s : g->acquireSensors()) {
					if (s->getName() == sensor && s->isInit()) {
						int64_t avg = 0;
						try {
							avg = s->getCache()->getAverage(S_TO_NS(time));
						} catch (const std::exception &e) {
							res.body() = "Unable to compute average: ";
							res.body() += e.what();
							res.result(http::status::internal_server_error);
							g->releaseSensors();
							return;
						}
						res.body() = plugin + "::" + sensor + " Average of last " +
							     std::to_string(time) + " seconds is " + std::to_string(avg) + "\n";
						res.result(http::status::ok);
						g->releaseSensors();
						return;
					}
				}
				g->releaseSensors();
			}
		}
	}

	for (auto &p : _manager->getPlugins()) {
		if (p.id == plugin) {
			res.body() = "Sensor not found!\n";
			for (const auto &op : p.configurator->getOperators()) {
				if (op->getStreaming()) {
					for (const auto &u : op->getUnits()) {
						for (const auto &s : u->getBaseOutputs()) {
							if (s->getName() == sensor && s->isInit()) {
								int64_t avg = 0;
								try {
									avg = s->getCache()->getAverage(S_TO_NS(time));
								} catch (const std::exception &e) {
									res.body() = "Unable to compute average: ";
									res.body() += e.what();
									res.result(http::status::internal_server_error);
									return;
								}
								res.body() = plugin + "::" + sensor + " Average of last " +
									     std::to_string(time) + " seconds is " + std::to_string(avg) + "\n";
								res.result(http::status::ok);
								return;
							}
						}
					}
					op->releaseUnits();
				}
			}
		}
	}
}

void RestAPI::PUT_quit(endpointArgs) {
	int retCode = getQuery("code", queries)=="" ? 0 : std::stoi(getQuery("code", queries));
	if(retCode<0 || retCode>255)
		retCode = 0;
	_retCode = retCode;
	raise(SIGUSR1);
	res.body() = "Quitting with return code " + std::to_string(retCode) + ".\n";
	res.result(http::status::ok);
}

void RestAPI::PUT_load(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);
	const std::string path = getQuery("path", queries);
	const std::string config = getQuery("config", queries);

	if (!hasPlugin(plugin, res)) {
		return;
	}

	//before modifying the plugin we need to ensure that we have exclusive access
	//therefore pause the only other concurrent user (MQTTPusher)
	if (!_mqttPusher->halt()) {
		res.body() = "Could not load plugin (Timeout while waiting).\n";
		res.result(http::status::internal_server_error);
		return;
	}

	unloadQueryEngine();

	if (_pluginManager->loadPlugin(plugin, path, config)) {
		res.body() = "Plugin " + plugin + " successfully loaded!\n";
		res.result(http::status::ok);

		_pluginManager->initPlugin(plugin);
	} else {
		res.body() = "Failed to load plugin " + plugin + "!\n";
		res.result(http::status::internal_server_error);
	}

	//continue MQTTPusher
	_mqttPusher->cont();
	reloadQueryEngine();
}

void RestAPI::PUT_unload(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);

	if (!hasPlugin(plugin, res)) {
		return;
	}

	if (!_mqttPusher->halt()) {
		res.body() = "Could not unload plugin (Timeout while waiting).\n";
		res.result(http::status::internal_server_error);
		return;
	}

	unloadQueryEngine();

	_pluginManager->unloadPlugin(plugin);
	res.body() = "Plugin " + plugin + " unloaded.\n";
	res.result(http::status::ok);

	//continue MQTTPusher
	_mqttPusher->cont();
	reloadQueryEngine();
}

void RestAPI::POST_start(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);

	if (!hasPlugin(plugin, res)) {
		return;
	}

	if (_pluginManager->startPlugin(plugin)) {
		res.body() = "Plugin " + plugin + ": Sensors started\n";
		res.result(http::status::ok);
		return;
	}
}

void RestAPI::POST_stop(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);

	if (!hasPlugin(plugin, res)) {
		return;
	}

	if (_pluginManager->stopPlugin(plugin)) {
		res.body() = "Plugin " + plugin + ": Sensors stopped\n";
		res.result(http::status::ok);
		return;
	}
}

void RestAPI::PUT_reload(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);

	if (!hasPlugin(plugin, res)) {
		return;
	}

	//before modifying the plugin we need to ensure that we have exclusive access
	//therefore pause the only other concurrent user (MQTTPusher)
	if (!_mqttPusher->halt()) {
		res.body() = "Could not reload plugin (Timeout while waiting).\n";
		res.result(http::status::internal_server_error);
		return;
	}

	unloadQueryEngine();

	if (_pluginManager->reloadPluginConfig(plugin)) {
		res.body() = "Plugin " + plugin + ": Configuration reloaded.\n";
		res.result(http::status::ok);

		_pluginManager->initPlugin(plugin);
		_pluginManager->startPlugin(plugin);
	} else {
		res.body() = "Could not reload plugin (Plugin not found or invalid config file).\n";
		res.result(http::status::internal_server_error);
	}

	//continue MQTTPusher
	_mqttPusher->cont();
	reloadQueryEngine();
}

void RestAPI::PUT_analytics_reload(endpointArgs) {
	if (_manager->getStatus() != OperatorManager::LOADED) {
		res.body() = "OperatorManager is not loaded!\n";
		res.result(http::status::internal_server_error);
		return;
	}

	const std::string plugin = getQuery("plugin", queries);

	// Wait until MQTTPusher is paused in order to reload plugins
	if (_mqttPusher->halt()) {

		unloadQueryEngine();

		if (!_manager->reload(plugin)) {
			res.body() = "Plugin not found or reload failed, please check the config files and MQTT topics!\n";
			res.result(http::status::not_found);
		} else if (!_manager->start(plugin)) {
			res.body() = "Plugin cannot be restarted!\n";
			res.result(http::status::internal_server_error);
		} else {
			res.body() = "Plugin " + plugin + ": Sensors reloaded\n";
			res.result(http::status::ok);
		}

		_mqttPusher->cont();
		reloadQueryEngine();
	} else {
		res.body() = "Could not reload plugins (Timeout while waiting).\n";
		res.result(http::status::internal_server_error);
	}
}

void RestAPI::PUT_analytics_load(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);
	const std::string path = getQuery("path", queries);
	const std::string config = getQuery("config", queries);

	if (!hasPlugin(plugin, res)) {
		return;
	}

	//before modifying the plugin we need to ensure that we have exclusive access
	//therefore pause the only other concurrent user (MQTTPusher)
	if (!_mqttPusher->halt()) {
		res.body() = "Could not load operator plugin (Timeout while waiting).\n";
		res.result(http::status::internal_server_error);
		return;
	}

	unloadQueryEngine();

	if (_manager->loadPlugin(plugin, path, config)) {
		res.body() = "Operator plugin " + plugin + " successfully loaded!\n";
		res.result(http::status::ok);

		_manager->init(plugin);
	} else {
		res.body() = "Failed to load operator plugin " + plugin + "!\n";
		res.result(http::status::internal_server_error);
	}

	//continue MQTTPusher
	_mqttPusher->cont();
	reloadQueryEngine();
}

void RestAPI::PUT_analytics_unload(endpointArgs) {
	const std::string plugin = getQuery("plugin", queries);

	if (!hasPlugin(plugin, res)) {
		return;
	}

	if (!_mqttPusher->halt()) {
		res.body() = "Could not unload operator plugin (Timeout while waiting).\n";
		res.result(http::status::internal_server_error);
		return;
	}

	unloadQueryEngine();

	_manager->unloadPlugin(plugin);
	res.body() = "Operator plugin " + plugin + " unloaded.\n";
	res.result(http::status::ok);

	//continue MQTTPusher
	_mqttPusher->cont();
	reloadQueryEngine();
}

void RestAPI::PUT_analytics_navigator(endpointArgs) {
	unloadQueryEngine();
	if (!reloadQueryEngine(true)) {
		res.body() = "Sensor hierarchy tree could not be rebuilt.\n";
		res.result(http::status::internal_server_error);
	} else {
		std::shared_ptr<SensorNavigator> navigator = QueryEngine::getInstance().getNavigator();
		res.body() = "Built a sensor hierarchy tree of size " + std::to_string(navigator->getTreeSize()) + " and depth " + std::to_string(navigator->getTreeDepth()) + ".\n";
		res.result(http::status::ok);
	}
}

bool RestAPI::reloadQueryEngine(const bool force) {
	//Updating the SensorNavigator on plugin reloads, if operator plugins are currently running
	if (!force && (_manager == NULL || _manager->getPlugins().empty()))
		return false;

	QueryEngine &                                    qEngine = QueryEngine::getInstance();
	std::shared_ptr<std::map<std::string, SBasePtr>> sensorMap = std::make_shared<std::map<std::string, SBasePtr>>();
	std::shared_ptr<SensorNavigator>                 navigator = std::make_shared<SensorNavigator>();
	std::vector<std::string>                         topics;

	for (const auto &p : _pluginManager->getPlugins()) {
		for (const auto &g : p.configurator->getSensorGroups()) {
			for (const auto &s : g->acquireSensors()) {
				topics.push_back(s->getMqtt());
				sensorMap->insert(std::make_pair(s->getName(), s));
			}
			g->releaseSensors();
		}
	}

	// Adding data analytics sensors to the map
	for (auto &p : _manager->getPlugins()) {
		for (const auto &op : p.configurator->getOperators())
			if (op->getStreaming()) {
				for (const auto &u : op->getUnits())
					for (const auto &o : u->getBaseOutputs())
						sensorMap->insert(std::make_pair(o->getName(), o));
				op->releaseUnits();
			}
	}

	try {
		navigator->buildTree(qEngine.getSensorHierarchy(), &topics);
	} catch (const std::exception &e) {
		navigator->clearTree();
		qEngine.getNavigator()->clearTree();
		qEngine.unlock();
		return false;
	}

	qEngine.setSensorMap(sensorMap);
	qEngine.setNavigator(navigator);
	// Unlocking the QueryEngine
	qEngine.unlock();
	return true;
}

void RestAPI::unloadQueryEngine() {
	QueryEngine &qEngine = QueryEngine::getInstance();
	// Locking access to the QueryEngine
	qEngine.lock();
	//qEngine.setNavigator(nullptr);
	qEngine.setSensorMap(nullptr);
}
