//================================================================================
// Name        : OperatorManager.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Management class implementation for the data analytics framework.
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

#include "OperatorManager.h"
#include "timestamp.h"

void OperatorManager::clear() {
    for(const auto& p : _plugins)
        p.destroy(p.configurator);
    _plugins.clear();
    _status = CLEAR;
}

bool OperatorManager::probe(const string& path, const string& globalFile) {
    std::string cfgPath = path;
    boost::property_tree::iptree cfg;

    if (cfgPath != "" && cfgPath[cfgPath.length() - 1] != '/')
        cfgPath.append("/");

    try {
        boost::property_tree::read_info(cfgPath + globalFile, cfg);
    } catch (boost::property_tree::info_parser_error &e) {
        return false;
    }

    if(cfg.find("operatorPlugins") == cfg.not_found()) 
        return false;

    int pluginCtr = 0;
    BOOST_FOREACH(boost::property_tree::iptree::value_type &plugin, cfg.get_child("operatorPlugins")) {
        if (boost::iequals(plugin.first, "operatorPlugin"))
            pluginCtr++;
    }

    if( pluginCtr == 0) 
        return false;
    else
        return true;
}

bool OperatorManager::load(const string& path, const string& globalFile, const pluginSettings_t& pluginSettings) {
    //The load code is pretty much the same as in Configuration.cpp to load pusher plugins
    _configPath = path;
    _pluginSettings = pluginSettings;
    boost::property_tree::iptree cfg;

    if (_configPath != "" && _configPath[_configPath.length()-1] != '/')
        _configPath.append("/");
    else if(_configPath.empty())
        _configPath = "./";

    try {
        boost::property_tree::read_info(_configPath + globalFile, cfg);
    } catch (boost::property_tree::info_parser_error& e) {
        LOG(error) << "Error when reading operator plugins from " << globalFile << ": " << e.what();
        return false;
    }

    if(cfg.find("operatorPlugins") == cfg.not_found()) {
        LOG(warning) << "No operatorPlugins block found, skipping data analytics initialization!";
        _status = LOADED;
        return true;
    }

    //Reading plugins
    BOOST_FOREACH(boost::property_tree::iptree::value_type &plugin, cfg.get_child("operatorPlugins")) {
        if (boost::iequals(plugin.first, "operatorPlugin")) {
            if (!plugin.second.data().empty()) {
                string pluginConfig="", pluginPath="";
                BOOST_FOREACH(boost::property_tree::iptree::value_type &val, plugin.second) {
                    if (boost::iequals(val.first, "path")) {
                        pluginPath = val.second.data();
                    } else if (boost::iequals(val.first, "config")) {
                        pluginConfig = val.second.data();
                    } else {
                        LOG(warning) << "  Value \"" << val.first << "\" not recognized. Omitting";
                    }
                }
                if(!loadPlugin(plugin.second.data(), pluginPath, pluginConfig))
                    return false;
            }
        }
    }
    _status = LOADED;
    return true;
}

bool OperatorManager::loadPlugin(const string& name, const string& pluginPath, const string& config) {
    LOG(info) << "Loading operator plugin " << name << "...";
    std::string pluginConfig; //path to config file for plugin
    std::string pluginLib = "libdcdboperator_" + name;
#if __APPLE__
    pluginLib+= ".dylib";
#else
    pluginLib+= ".so";
#endif
    
    std::string iPath = pluginPath;
    // If path not specified we will look up in the default lib-directories (usr/lib and friends)
    if (iPath != "") {
        if (iPath[iPath.length()-1] != '/')
            iPath.append("/");
        pluginLib = iPath + pluginLib;
    }
    
    pluginConfig = config;
    // If config-path not specified we will look for pluginName.conf in the global conf directory
    if (pluginConfig == "")
        pluginConfig = _configPath + name + ".conf";
    else if(pluginConfig[0] != '/')
        pluginConfig = _configPath + pluginConfig;
        
    // Open dl-code based on http://tldp.org/HOWTO/C++-dlopen/thesolution.html
    if (FILE *file = fopen(pluginConfig.c_str(), "r")) {
        fclose(file);
        op_dl_t dynLib;
        dynLib.id = name;
        dynLib.DL = NULL;
        dynLib.configurator = NULL;

        // If plugin.conf exists, open libdcdboperator_pluginName.so and read config
        LOG(info) << pluginConfig << " found";
        dynLib.DL = dlopen(pluginLib.c_str(), RTLD_NOW);
        if(!dynLib.DL) {
            LOG(error) << "Cannot load " << dynLib.id << "-library: " << dlerror();
            return false;
        }
        dlerror();

        // Set dynLib op_dl_t struct, load create and destroy symbols
        dynLib.create = (op_create_t*) dlsym(dynLib.DL, "create");
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            LOG(error) << "Cannot load symbol create for " << dynLib.id << ": " << dlsym_error;
            return false;
        }

        dynLib.destroy = (op_destroy_t*) dlsym(dynLib.DL, "destroy");
        dlsym_error = dlerror();
        if (dlsym_error) {
            LOG(error) << "Cannot load symbol destroy for " << dynLib.id << ": " << dlsym_error;
            return false;
        }

        dynLib.configurator = dynLib.create();
        dynLib.configurator->setGlobalSettings(_pluginSettings);
        // Read the operator plugin configuration
        if (!(dynLib.configurator->readConfig(pluginConfig))) {
            LOG(error) << "Plugin " << dynLib.id << " could not read configuration!";
            return false;
        }

        // Returning an empty vector may indicate problems with the config file
        if(dynLib.configurator->getOperators().size() == 0) {
            LOG(warning) << "Plugin " << dynLib.id << " created no operators!";
        } else if(!checkTopics(dynLib)) {
            LOG(error) << "Problematic MQTT topics or sensor names, please check your config files!";
            return false;
        }
        //save dl-struct
        _plugins.push_back(dynLib);
        LOG(info) << "Plugin " << dynLib.id << " " << dynLib.configurator->getVersion() << " loaded!";
    } else {
        LOG(info) << pluginConfig << " not found. Omitting";
        return false;
    }
    return true;
}

void OperatorManager::unloadPlugin(const string& id) {
    for (auto it = _plugins.begin(); it != _plugins.end(); ++it) {
        if(it->id == id || id == "") {
            for (const auto& op : it->configurator->getOperators())
                op->stop();

            for (const auto& op : it->configurator->getOperators())
                op->wait();

            removeTopics(*it);

            if (it->configurator)
                it->destroy(it->configurator);
            
            //if (it->DL)
            //    dlclose(it->DL);

            if (id != "") {
                _plugins.erase(it);
                return;
            }
        }
    }

    if (id == "")
        _plugins.clear();
}

bool OperatorManager::init(const string& plugin) {
    if(_status != LOADED) {
        LOG(error) << "Cannot init, OperatorManager is not loaded!";
        return false;
    }
    bool out=false;
    for (const auto &p : _plugins)
        //Actions always affect either one or all plugins, and always all operators within said plugin
        if(plugin=="" || plugin==p.id) {
            out = true;
            LOG(info) << "Init " << p.id << " operator plugin";
            for (const auto &op : p.configurator->getOperators())
                op->init(_io);
        }
    return out;
}

bool OperatorManager::reload(const string& plugin) {
    if(_status != LOADED) {
        LOG(error) << "Cannot reload, OperatorManager is not loaded!";
        return false;
    }
    bool out=false;
    for (const auto &p : _plugins)
        if(plugin=="" || plugin==p.id) {
            LOG(info) << "Reload " << p.id << " operator plugin";
            out = true;
            //Removing obsolete MQTT topics
            removeTopics(p);
            //Reloading plugin
            if(!p.configurator->reReadConfig())
                return false;
            //Checking new MQTT topics
            if(!checkTopics(p)) {
                removeTopics(p);
                p.configurator->clearConfig();
                return false;
            } else
                for (const auto &op : p.configurator->getOperators())
                    op->init(_io);
        }
    return out;
}

void OperatorManager::removeTopics(op_dl_t p) {
    MQTTChecker& mqttCheck = MQTTChecker::getInstance();
    for(const auto& op : p.configurator->getOperators()) {
        mqttCheck.removeGroup(op->getName());
        if (op->getStreaming()) {
            for (const auto &u : op->getUnits())
                for (const auto &o: u->getBaseOutputs()) {
                    mqttCheck.removeTopic(o->getMqtt());
                    mqttCheck.removeName(o->getName());
                }
            op->releaseUnits();
        }
    }
}

bool OperatorManager::checkTopics(op_dl_t p) {
    MQTTChecker& mqttCheck = MQTTChecker::getInstance();
    bool validTopics=true;
    for(const auto& op : p.configurator->getOperators()) {
        if (!(op->getStreaming() && op->getDuplicate()) && !mqttCheck.checkGroup(op->getName()))
            validTopics = false;
        if (op->getStreaming()) {
            for (const auto &u : op->getUnits())
                for (const auto &o: u->getBaseOutputs())
                    if (!mqttCheck.checkTopic(o->getMqtt()) || !mqttCheck.checkName(o->getName()))
                        validTopics = false;
            op->releaseUnits();
        }
    }
    return validTopics;
}

bool OperatorManager::start(const string& plugin, const string& operatorN) {
    if(_status != LOADED) {
        LOG(error) << "Cannot start, OperatorManager is not loaded!";
        return false;
    }
    bool out=false;
    for (const auto &p : _plugins)
        if(plugin=="" || plugin==p.id) {
            LOG(info) << "Start " << p.id << " operator plugin";
            for (const auto &op : p.configurator->getOperators())
                // Only streaming operators can be started
                if(op->getStreaming() && (operatorN=="" || operatorN==op->getName())) {
                    op->start();
                    out=true;
                }
        }
    return out;
}

bool OperatorManager::stop(const string& plugin, const string& operatorN) {
    if(_status != LOADED) {
        LOG(error) << "Cannot stop, OperatorManager is not loaded!";
        return false;
    }
    bool out=false;
    for (const auto &p : _plugins)
        if(plugin=="" || plugin==p.id) {
            LOG(info) << "Stop " << p.id << " operator plugin";
            for (const auto &op : p.configurator->getOperators())
                // Only streaming operators can be stopped
                if(op->getStreaming() && (operatorN=="" || operatorN==op->getName())) {
                    op->stop();
                    out=true;
                }
            for (const auto &op : p.configurator->getOperators())
                if(op->getStreaming() && (operatorN=="" || operatorN==op->getName()))
                    op->wait();
        }
    return out;
}

/******************************************************************************/
/*      Rest API endpoint methods                                             */
/******************************************************************************/

#define stdBind(fun) std::bind(&OperatorManager::fun, \
          this, \
          std::placeholders::_1, \
          std::placeholders::_2, \
	  std::placeholders::_3)

void OperatorManager::addRestEndpoints(RESTHttpsServer* restServer) {
    restServer->addEndpoint("/analytics/help",      {http::verb::get, stdBind(GET_analytics_help)});
    restServer->addEndpoint("/analytics/plugins",   {http::verb::get, stdBind(GET_analytics_plugins)});
    restServer->addEndpoint("/analytics/sensors",   {http::verb::get, stdBind(GET_analytics_sensors)});
    restServer->addEndpoint("/analytics/units",     {http::verb::get, stdBind(GET_analytics_units)});
    restServer->addEndpoint("/analytics/operators", {http::verb::get, stdBind(GET_analytics_operators)});

    restServer->addEndpoint("/analytics/start",    {http::verb::post, stdBind(POST_analytics_start)});
    restServer->addEndpoint("/analytics/stop",     {http::verb::post, stdBind(POST_analytics_stop)});
    restServer->addEndpoint("/analytics/compute",  {http::verb::put, stdBind(PUT_analytics_compute)});
    restServer->addEndpoint("/analytics/operator", {http::verb::put, stdBind(PUT_analytics_operator)});
}

void OperatorManager::GET_analytics_help(endpointArgs){
    if (!managerLoaded(res)) {
        return;
    }
    res.body() = restCheatSheet;
    res.result(http::status::ok);
}

void OperatorManager::GET_analytics_plugins(endpointArgs) {
    if (!managerLoaded(res)) {
        return;
    }
    std::ostringstream data;
    if (getQuery("json", queries) == "true") {
        boost::property_tree::ptree root, plugins;
        for(const auto& p : _plugins) {
            plugins.put(p.id, "");
        }
        root.add_child("plugins", plugins);
        boost::property_tree::write_json(data, root, true);
    } else {
        for(const auto& p : _plugins) {
            data << p.id << "\n";
        }
    }
    res.body() = data.str();
    res.result(http::status::ok);
}

void OperatorManager::GET_analytics_sensors(endpointArgs) {
    if (!managerLoaded(res)) {
        return;
    }

    const std::string plugin   = getQuery("plugin", queries);
    const std::string oper     = getQuery("operator", queries);

    if (!hasPlugin(plugin, res)) {
        return;
    }

    bool found = false;
    std::ostringstream data;

    for (const auto& p : _plugins) {
        if (p.id == plugin) {
            if (getQuery("json", queries) == "true") {
                boost::property_tree::ptree root, sensors;

                // In JSON mode, sensors are arranged hierarchically by plugin->operator->sensor
                for (const auto& op : p.configurator->getOperators()) {
                    if (op->getStreaming() && (oper == "" || oper == op->getName())) {
                        found = true;
                        boost::property_tree::ptree group;
                        for (const auto& u : op->getUnits()) {
                            for (const auto& s : u->getBaseOutputs()) {
                                // Explicitly adding nodes to the ptree as to prevent BOOST from performing
                                // parsing on the node names
                                group.push_back(boost::property_tree::ptree::value_type("", boost::property_tree::ptree(s->getMqtt())));
                            }
                        }
                        op->releaseUnits();
                        sensors.add_child(op->getName(), group);
                    }
                }
                root.add_child(p.id, sensors);
                boost::property_tree::write_json(data, root, true);
            } else {
                for (const auto& op : p.configurator->getOperators()) {
                    if (op->getStreaming() && (oper == "" || oper == op->getName())) {
                        found = true;
                        for (const auto& u : op->getUnits()) {
                            for (const auto& s : u->getBaseOutputs()) {
                                data << op->getName() << "::" << s->getMqtt() << "\n";
                            }
                        }
                        op->releaseUnits();
                    }
                }
            }
            res.body() = data.str();
            res.result(http::status::ok);
            break;
        }
    }
    if (!found) {
        res.body() = "Plugin or operator not found!\n";
        res.result(http::status::not_found);
    }
}

void OperatorManager::GET_analytics_units(endpointArgs) {
    if (!managerLoaded(res)) {
        return;
    }

    const std::string plugin   = getQuery("plugin", queries);
    const std::string oper     = getQuery("operator", queries);

    if (!hasPlugin(plugin, res)) {
        return;
    }

    bool found = false;
    std::ostringstream data;

    for (const auto& p : _plugins) {
        if (p.id == plugin) {
            if (getQuery("json", queries) == "true") {
                boost::property_tree::ptree root, units;

                // In JSON mode, sensors are arranged hierarchically by plugin->operator->sensor
                for (const auto& op : p.configurator->getOperators())
                    if (op->getStreaming() && (oper == "" || oper == op->getName())) {
                        found = true;
                        boost::property_tree::ptree group;
                        for (const auto& u : op->getUnits()) {
                            group.push_back(boost::property_tree::ptree::value_type("", boost::property_tree::ptree(u->getName())));
                        }
                        op->releaseUnits();
                        units.add_child(op->getName(), group);
                    }
                root.add_child(p.id, units);
                boost::property_tree::write_json(data, root, true);
            } else {
                for (const auto& op : p.configurator->getOperators()) {
                    if (op->getStreaming() && (oper == "" || oper == op->getName())) {
                        found = true;
                        for (const auto& u : op->getUnits()) {
                            data << op->getName() << "::" << u->getName() << "\n";
                        }
                        op->releaseUnits();
                    }
                }
            }
            res.body() = data.str();
            res.result(http::status::ok);
            break;
        }
    }
    if (!found) {
        res.body() = "Plugin or operator not found!\n";
        res.result(http::status::not_found);
    }
}

void OperatorManager::GET_analytics_operators(endpointArgs) {
    if (!managerLoaded(res)) {
        return;
    }

    const std::string plugin   = getQuery("plugin", queries);

    if (!hasPlugin(plugin, res)) {
        return;
    }

    std::ostringstream data;

    for (const auto& p : _plugins) {
        if (p.id == plugin) {
            if (getQuery("json", queries) == "true") {
                boost::property_tree::ptree root, operators;

                // For each operator, we output its type as well
                for (const auto& op : p.configurator->getOperators()) {
                    operators.push_back(boost::property_tree::ptree::value_type(op->getName(), boost::property_tree::ptree(op->getStreaming() ? "streaming" : "on-demand")));
                }
                root.add_child(p.id, operators);
                boost::property_tree::write_json(data, root, true);
            } else {
                for (const auto& op : p.configurator->getOperators()) {
                    data << op->getName() << " " << (op->getStreaming() ? "streaming\n" : "on-demand\n");
                }
            }
            res.body() = data.str();
            res.result(http::status::ok);
            return;
        }
    }
}


void OperatorManager::POST_analytics_start(endpointArgs) {
    if (!managerLoaded(res)) {
        return;
    }

    const std::string plugin   = getQuery("plugin", queries);
    const std::string oper     = getQuery("operator", queries);

    if (start(plugin, oper)) {
        res.body() = "Plugin " + plugin + " " + oper + ": Sensors started!\n";
        res.result(http::status::ok);
    } else {
        res.body() = "Plugin or operator not found!\n";
        res.result(http::status::not_found);
    }
}

void OperatorManager::POST_analytics_stop(endpointArgs) {
    if (!managerLoaded(res)) {
        return;
    }

    const std::string plugin   = getQuery("plugin", queries);
    const std::string oper     = getQuery("operator", queries);

    if (stop(plugin, oper)) {
        res.body() = "Plugin " + plugin + " " + oper + ": Sensors stopped!\n";
        res.result(http::status::ok);
    } else {
        res.body() = "Plugin or operator not found!\n";
        res.result(http::status::not_found);
    }
}

void OperatorManager::PUT_analytics_compute(endpointArgs) {
    if (!managerLoaded(res)) {
        return;
    }

    const std::string plugin   = getQuery("plugin", queries);
    const std::string oper     = getQuery("operator", queries);
    std::string unit = getQuery("unit", queries);

    if (plugin == "" || oper == "") {
        res.body() = "Request malformed: plugin or operator query missing\n";
        res.result(http::status::bad_request);
        return;
    }

    if (unit == "") {
        unit = SensorNavigator::rootKey;
    }

    res.body() = "Plugin or operator not found!\n";
    res.result(http::status::not_found);

    std::ostringstream data;

    bool unitFound=false, opFound=false;
    for (const auto &p : _plugins) {
        if (p.id == plugin) {
            for (const auto &op : p.configurator->getOperators()) {
                if( op->getName() == oper ) {
                    opFound = true;
                    std::map<std::string, reading_t> outMap;
                    try {
                        outMap = op->computeOnDemand(unit);
                        unitFound = true;
                    } catch(const domain_error& e) {
                        // In the particular case where an operator is duplicated, it could be that the right
                        // unit is found only after a few tries. Therefore, we handle the domain_error
                        // exception raised in OperatorTemplate, and allow the search to continue
                        if(op->getStreaming() && op->getDuplicate()) {
                            continue;
                        } else {
                            res.body() = string(e.what()) + string("\n");
                            res.result(http::status::not_found);
                            return;
                        }
                    } catch(const exception& e) {
                        res.body() = string(e.what()) + string("\n");
                        res.result(http::status::internal_server_error);
                        return;
                    }
                    if (getQuery("json", queries) == "true") {
                        boost::property_tree::ptree root, outputs;
                        // Iterating through the outputs of the on-demand computation and adding them to a JSON
                        for (const auto& kv : outMap) {
                            boost::property_tree::ptree sensor;
                            sensor.push_back(boost::property_tree::ptree::value_type("timestamp", boost::property_tree::ptree(to_string(kv.second.timestamp))));
                            sensor.push_back(boost::property_tree::ptree::value_type("value", boost::property_tree::ptree(to_string(kv.second.value))));
                            outputs.push_back(boost::property_tree::ptree::value_type(kv.first, sensor));
                        }
                        root.add_child(op->getName(), outputs);
                        boost::property_tree::write_json(data, root, true);
                    } else {
                        for (const auto& kv : outMap) {
                            data << kv.first << " ts: " << kv.second.timestamp << " v: " << kv.second.value << "\n";
                        }
                    }
                    res.body() = data.str();
                    res.result(http::status::ok);
                    return;
                }
            }
        }
    }
    // This if branch is accessed only if the target operator is streaming and duplicated
    if(opFound && !unitFound) {
        res.body() = "Node " + unit + " does not belong to the domain of " + oper + "!\n";
        res.result(http::status::not_found);
    }

}

void OperatorManager::PUT_analytics_operator(endpointArgs) {
    if (!managerLoaded(res)) {
        return;
    }

    const std::string plugin   = getQuery("plugin", queries);
    const std::string oper     = getQuery("operator", queries);
    const std::string action   = getQuery("action", queries);

    if (plugin == "" || action == "") {
        res.body() = "Request malformed: plugin or action query missing.\n";
        res.result(http::status::bad_request);
        return;
    }

    res.body() = "Plugin or operator not found!\n";
    res.result(http::status::not_found);

    // Managing custom REST PUT actions defined at the operator level
    for (const auto &p : _plugins) {
        if (p.id == plugin) {
            for (const auto &op : p.configurator->getOperators()) {
                if (oper == "" || oper == op->getName()) {
                    // Any thrown exception is catched outside in the HTTPserver
                    try {
                        restResponse_t reply = op->REST(action, queries);
                        res.body() = reply.data;
                        res.body() += reply.response;
                        res.result(http::status::ok);
                    } catch(const std::invalid_argument &e) {
                        res.body() = string(e.what()) + string("\n");
                        res.result(http::status::bad_request);
                    } catch(const std::domain_error &e) {
                        res.body() = string(e.what()) + string("\n");
                        res.result(http::status::not_found);
                    } catch(const std::exception &e) {
                        res.body() = string(e.what()) + string("\n");
                        res.result(http::status::internal_server_error);
                    }
                }
            }
        }
    }
}
