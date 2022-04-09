//================================================================================
// Name        : CARestAPI.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : RESTful API implementation for collectagent.
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

#include "CARestAPI.h"
#include <dcdb/version.h>
#include "version.h"

#include <boost/beast/http/field.hpp>

#define stdBind(fun) std::bind(&CARestAPI::fun, \
          this, \
          std::placeholders::_1, \
          std::placeholders::_2, \
	  std::placeholders::_3)

CARestAPI::CARestAPI(serverSettings_t settings, 
                     influx_t* influxSettings,
                     SensorCache* sensorCache, 
                     SensorDataStore* sensorDataStore,
                     SensorConfig *sensorConfig,
                     AnalyticsController* analyticsController,
                     SimpleMQTTServer* mqttServer,
                     boost::asio::io_context& io) :
                     RESTHttpsServer(settings, io),
                     _influxSettings(influxSettings),
                     _sensorCache(sensorCache),
                     _sensorDataStore(sensorDataStore),
                     _sensorConfig(sensorConfig),
                     _analyticsController(analyticsController),
                     _mqttServer(mqttServer) {
    _influxCounter = 0;
			 
    addEndpoint("/help",    {http::verb::get, stdBind(GET_help)});
    addEndpoint("/version", {http::verb::get, stdBind(GET_version)});
    addEndpoint("/hosts",   {http::verb::get, stdBind(GET_hosts)});
    addEndpoint("/average", {http::verb::get, stdBind(GET_average)});
    addEndpoint("/quit", {http::verb::put, stdBind(PUT_quit)});

    addEndpoint("/ping", {http::verb::get, stdBind(GET_ping)});
    addEndpoint("/query", {http::verb::post, stdBind(POST_query)});
    addEndpoint("/write", {http::verb::post, stdBind(POST_write)});

    _analyticsController->getManager()->addRestEndpoints(this);

    addEndpoint("/analytics/reload", {http::verb::put, stdBind(PUT_analytics_reload)});
    addEndpoint("/analytics/load", {http::verb::put, stdBind(PUT_analytics_load)});
    addEndpoint("/analytics/unload", {http::verb::put, stdBind(PUT_analytics_unload)});
    addEndpoint("/analytics/navigator", {http::verb::put, stdBind(PUT_analytics_navigator)});
}

void CARestAPI::GET_help(endpointArgs) {
    res.body() = caRestCheatSheet + _analyticsController->getManager()->restCheatSheet;
    res.result(http::status::ok);
}

void CARestAPI::GET_version(endpointArgs) {
    res.body() = "CollectAgent " + std::string(VERSION) + " (libdcdb " + DCDB::Version::getVersion() + ")";
    res.result(http::status::ok);
}

void CARestAPI::GET_hosts(endpointArgs) {
    if (!_mqttServer) {
        res.body() = "The MQTT server is not initialized!";
        res.result(http::status::internal_server_error);
        return;
    }
    std::ostringstream data;
    data << "address,clientID,lastSeen" << std::endl;
    std::map<std::string, hostInfo_t> hostsVec = _mqttServer->collectLastSeen();
    for(auto &el : hostsVec) {
        data << el.second.address <<  "," << el.second.clientId << "," << std::to_string(el.second.lastSeen) << std::endl;
    }
    res.body() = data.str();
    res.result(http::status::ok);
}

void CARestAPI::GET_average(endpointArgs) {
    const std::string sensor = getQuery("sensor", queries);
    const std::string interval = getQuery("interval", queries);

    if (sensor == "") {
        res.body() = "Request malformed: sensor query missing";
        res.result(http::status::bad_request);
        return;
    }

    uint64_t time = 0;

    if (interval != "") {
        try {
            time = std::stoul(interval);
        } catch (const std::exception& e) {
            RESTAPILOG(warning) << "Bad interval query: " << e.what();
            res.body() = "Bad interval query!\n";
            res.result(http::status::bad_request);
            return;
        }
    }

    //try getting the latest value
    try {
        int64_t val = _sensorCache->getSensor(sensor, (uint64_t) time * 1000000000);
        res.body() = "collectagent::" + sensor + " Average of last " +
                     std::to_string(time) + " seconds is " + std::to_string(val);
        res.result(http::status::ok);
        //std::ostringstream data;
        //data << val << "\n";
        //data << "Sid : " << sid.toString() << ", Value: " << val << "." << std::endl;
        //res.body() = data.str();
    } catch (const std::invalid_argument &e) {
        res.body() = "Error: Sensor id not found.\n";
        res.result(http::status::not_found);
    } catch (const std::out_of_range &e) {
        res.body() = "Error: Sensor unavailable.\n";
        res.result(http::status::no_content);
    } catch (const std::exception &e) {
        res.body() = "Internal server error.\n";
        res.result(http::status::internal_server_error);
        RESTAPILOG(warning) << "Internal server error: " << e.what();
    }
}

void CARestAPI::GET_ping(endpointArgs) {
    res.body() = "";
    res.result(http::status::ok);
}

void CARestAPI::POST_query(endpointArgs) {
    res.set(http::field::content_type, "application/json");
    res.body() = "{results: [{statement_id: 0}]}";
    res.result(http::status::ok);
}

void CARestAPI::POST_write(endpointArgs) {
    std::istringstream body(req.body());
    std::string line;
    while (std::getline(body, line)) {
	// Regex to split line into measurement, tags, fields, timestamp
	boost::regex r1("^([^,]*)(,[^ ]*)? ([^ ]*)( .*)?$", boost::regex::extended);
	// Regex to split comma-separated tags and fields into individual entries
	boost::regex r2(",?([^,=]*)=([^,]*)", boost::regex::extended);

	boost::smatch m1, m2;
	if (boost::regex_search(line, m1, r1)) {
	    std::string measurement = m1[1].str();
	    auto m = _influxSettings->measurements.find(measurement);
	    if (m != _influxSettings->measurements.end()) {
		influx_measurement_t influx = m->second;
		
		// Parse tags into a map
		std::map<std::string, std::string> tags;
		std::string tagList = m1[2].str();
		while (boost::regex_search(tagList, m2, r2)) {
		    tags[m2[1].str()] = m2[2].str();
		    tagList = m2.suffix().str();
		}

		auto t = tags.find(influx.tag);
		if (t != tags.end()) {
		    std::string tagName = t->second;
		    // Perform pattern filter or substitution via regex on tag
		    if (!influx.tagRegex.empty()) {
			std::string input(tagName);
			tagName = "";
			boost::regex_replace(std::back_inserter(tagName), input.begin(), input.end(), influx.tagRegex, influx.tagSubstitution.c_str(), boost::regex_constants::format_sed | boost::regex_constants::format_no_copy);
			if (tagName.size() == 0) {
			    // There was no match
			    break;
			}
		    }
		    std::map<std::string, std::string> fields;
		    std::string fieldList = m1[3].str();
		    while (boost::regex_search(fieldList, m2, r2)) {
			fields[m2[1].str()] = m2[2].str();
			fieldList = m2.suffix().str();
		    }
		    
		    DCDB::TimeStamp ts;
		    try {
			ts = TimeStamp(m1[4].str());
		    } catch (...) {
		    }

		    for (auto &f: fields) {
			// If no fields were defined, we take any field
			if (influx.fields.empty() || (influx.fields.find(f.first) != influx.fields.end())) {
			    std::string mqttTopic = _influxSettings->mqttPrefix + m->second.mqttPart + "/" + tagName + "/" + f.first;
			    uint64_t value = 0;
			    try {
				value = stoull(f.second);
			    } catch (...) {
				break;
			    }

			    DCDB::SensorId sid;
			    if (sid.mqttTopicConvert(mqttTopic)) {
				_sensorCache->storeSensor(sid, ts.getRaw(), value);
				_sensorDataStore->insert(sid, ts.getRaw(), value);
				_influxCounter++;
				
				if (_influxSettings->publish && (_influxSensors.find(sid.getId()) == _influxSensors.end())) {
				    _influxSensors.insert(sid.getId());
				    _sensorConfig->publishSensor(sid.getId().c_str(), sid.getId().c_str());
				}
			    }
#ifdef DEBUG
			    LOG(debug) << "influx insert: " << mqttTopic << " " << ts.getRaw() << " " << value;
#endif
			}
		    }
		}
	    } else {
		LOG(debug) << "influx: unknown measurement " << measurement;
#ifdef DEBUG
		LOG(debug) << "influx line: " << line;
#endif
	    }
	}
    }
    res.body() = "";
    res.result(http::status::no_content);
}

void CARestAPI::PUT_quit(endpointArgs) {
    int retCode = getQuery("code", queries)=="" ? 0 : std::stoi(getQuery("code", queries));
    if(retCode<0 || retCode>255)
        retCode = 0;
    _retCode = retCode;
    raise(SIGUSR1);
    res.body() = "Quitting with return code " + std::to_string(retCode) + ".\n";
    res.result(http::status::ok);
}

void CARestAPI::PUT_analytics_reload(endpointArgs) {
    if (_analyticsController->getManager()->getStatus() != OperatorManager::LOADED) {
        res.body() = "OperatorManager is not loaded!\n";
        res.result(http::status::internal_server_error);
        return;
    }

    const std::string plugin   = getQuery("plugin", queries);

    // Wait until controller is paused in order to reload plugins
    _analyticsController->halt(true);

    if (!_analyticsController->getManager()->reload(plugin)) {
        res.body() = "Plugin not found or reload failed, please check the config files and MQTT topics!\n";
        res.result(http::status::not_found);
    } else if (!_analyticsController->getManager()->start(plugin)) {
        res.body() = "Plugin cannot be restarted!\n";
        res.result(http::status::internal_server_error);
    } else {
        res.body() = "Plugin " + plugin + ": Sensors reloaded\n";
        res.result(http::status::ok);
    }

    _analyticsController->resume();
}

void CARestAPI::PUT_analytics_load(endpointArgs) {
    const std::string plugin = getQuery("plugin", queries);
    const std::string path   = getQuery("path", queries);
    const std::string config = getQuery("config", queries);

    if (!hasPlugin(plugin, res)) {
        return;
    }

    // Wait until controller is paused in order to reload plugins
    _analyticsController->halt(true);

    if(_analyticsController->getManager()->loadPlugin(plugin, path, config)) {
        res.body() = "Operator plugin " + plugin + " successfully loaded!\n";
        res.result(http::status::ok);

        _analyticsController->getManager()->init(plugin);
    } else {
        res.body() = "Failed to load operator plugin " + plugin + "!\n";
        res.result(http::status::internal_server_error);
    }

    _analyticsController->resume();
}

void CARestAPI::PUT_analytics_unload(endpointArgs) {
    const std::string plugin = getQuery("plugin", queries);

    if (!hasPlugin(plugin, res)) {
        return;
    }

    // Wait until controller is paused in order to reload plugins
    _analyticsController->halt(true);

    _analyticsController->getManager()->unloadPlugin(plugin);
    res.body() = "Operator plugin " + plugin + " unloaded.\n";
    res.result(http::status::ok);

    _analyticsController->resume();
}

void CARestAPI::PUT_analytics_navigator(endpointArgs) {
    if(!_analyticsController->rebuildSensorNavigator()) {
        res.body() = "Sensor hierarchy tree could not be rebuilt.\n";
        res.result(http::status::internal_server_error);
    } else {
        std::shared_ptr <SensorNavigator> navigator = QueryEngine::getInstance().getNavigator();
        res.body() = "Built a sensor hierarchy tree of size " + std::to_string(navigator->getTreeSize()) + " and depth " + std::to_string(navigator->getTreeDepth()) + ".\n";
        res.result(http::status::ok);
    }
}
