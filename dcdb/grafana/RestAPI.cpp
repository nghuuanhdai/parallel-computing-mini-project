//================================================================================
// Name        : RestAPI.cpp
// Author      : Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : RESTful API implementation for the Grafana server
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
#include "dcdb/unitconv.h"
#include "timestamp.h"

#include <sstream>
#include <boost/property_tree/json_parser.hpp>

#define stdBind(fun) std::bind(&RestAPI::fun, \
          this, \
          std::placeholders::_1, \
          std::placeholders::_2, \
          std::placeholders::_3)

RestAPI::RestAPI(serverSettings_t settings,
                 hierarchySettings_t hierarchySettings,
                 DCDB::Connection* cassandraConnection,
                 boost::asio::io_context &io) :
                 RESTHttpsServer(settings, io),
                 _connection(cassandraConnection),
                 _updating(false),
                 _timer(io, boost::posix_time::seconds(0)),
                 _publishedSensorsWritetime(0),
                 _hierarchySettings(hierarchySettings) {

    //Configuring endpoints
    addEndpoint("/",            {http::verb::get, stdBind(GET_datasource)});
    addEndpoint("/levels",      {http::verb::post, stdBind(POST_levels)});
    addEndpoint("/search",      {http::verb::post, stdBind(POST_search)});
    addEndpoint("/query",       {http::verb::post, stdBind(POST_query)});
    addEndpoint("/navigator",   {http::verb::put, stdBind(PUT_navigator)});

    _sensorConfig = new DCDB::SensorConfig(_connection);
    _sensorDataStore = new DCDB::SensorDataStore(_connection);
    _separator = _hierarchySettings.separator;
    _smootherRegex = boost::regex(_hierarchySettings.smootherRegex);
    _numRegex = boost::regex("[0-9]+");
}

//Initializes the internal sensor navigator
bool RestAPI::buildTree() {
    std::vector<std::string> topics;
    std::list<DCDB::PublicSensor> publicSensors;

    //Spinlock to ensure that only one sensor navigator update runs at a time
    while(_updating.exchange(true)) {}
    
    LOG(info) << "Retrieving published sensor names and topics...";

    //Get the list of all public sensors and topics.
    if(!_sensorConfig || _sensorConfig->getPublicSensorsVerbose(publicSensors)!=DCDB::SC_OK) {
        LOG(error) << "Unable to fetch list of public sensors!";
        _updating.store(false);
        return false;
    }

    std::shared_ptr<SensorNavigator> newNavigator = std::make_shared<SensorNavigator>();
    std::shared_ptr<MetadataStore> newMetadataStore = std::make_shared<MetadataStore>();
    
    for(auto& s : publicSensors) {
        topics.push_back(s.name);
        newMetadataStore->store(s.name, DCDB::PublicSensor::publicSensorToMetadata(s));
    }

    //Build the tree navigator
    newNavigator->setFilter(_hierarchySettings.filter);
    newNavigator->buildTree(_hierarchySettings.regex, &topics);
    
    //Replacing the old navigator and metadata store
    _navigator = newNavigator;
    _metadataStore = newMetadataStore;
    _sensorConfig->getPublishedSensorsWritetime(_publishedSensorsWritetime);
    _updating.store(false);
    LOG(info) << "Built a sensor navigator of size " << std::to_string(_navigator->getTreeSize()) << " and depth " << std::to_string(_navigator->getTreeDepth()) << ".";
    
    return true;
}

void RestAPI::checkPublishedSensorsAsync() {
    uint64_t t;
    if (_sensorConfig->getPublishedSensorsWritetime(t) == DCDB::SC_OK) {
	if (t > _publishedSensorsWritetime) {
	    LOG(debug) << "Fetching updated sensors";
	    buildTree();
	}
    }
    _timer.expires_at(timestamp2ptime(getTimestamp() + S_TO_NS(60)));
    _timer.async_wait(std::bind(&RestAPI::checkPublishedSensorsAsync, this));
}


//Dummy GET request to create a datasource. All necessary checks that could be peformed here are
//already done by the RESTAPIServer (e.g., user credentials, connectivity to the DB,...).
void RestAPI::GET_datasource(endpointArgs) {
    
    res.body() = "Data Source Added";
    res.result(http::status::ok);
    return;
}

//Returns the depth of the sensor tree in the navigator. Used to precompute the max number of levels
//at the frontend for the UI.
void RestAPI::POST_levels(endpointArgs) {
    
    res.body() = "[" + std::to_string(_navigator->getTreeDepth() + 1) + "]";;
    res.result(http::status::ok);
    return;
}

//Returns to Grafana a list of possible targets to be dispayed in the query drop-down menus of a panel
//(i.e., hierarchical levels of the system and sensors).
void RestAPI::POST_search(endpointArgs) {
    
    res.body() = "[";
    std::set<std::string> *treeOutput = nullptr;
    
    //Get the element from the sensor navigator.
    std::string parentNode = getQuery("node", queries);
    if(parentNode == "") {
        try {
            treeOutput = getQuery("sensors", queries)=="true" ? _navigator->getSensors(0, false) : _navigator->getNodes(0, false);
        } catch(const std::exception &e) {
            treeOutput = new std::set<std::string>();
            treeOutput->insert("");
        }
    } else {
        parentNode += "/";
        // What does this do exactly?
        if(_separator != "")
            boost::replace_all(parentNode, "/", _separator);

        try {
            treeOutput = getQuery("sensors", queries)=="true" ? _navigator->getSensors(parentNode, false) : _navigator->getNodes(parentNode, false);
        } catch(const std::exception &e) {
            res.body() = "Encountered exception: " + std::string(e.what());
            res.result(http::status::bad_request);
            return;
        }
    }
    
    //Format the data for Grafana
    boost::regex exp(parentNode);
    std::string outputElement;
    for (auto& o : *treeOutput) {
        if(getQuery("sensors", queries) != "true") {
            outputElement = boost::regex_replace((o),exp,"");
            outputElement.erase(std::remove(outputElement.begin(), outputElement.end(), '/'), outputElement.end());
        } else {
            outputElement = o;
        }
        
        res.body() += "\"" + outputElement + "\",";
    }
    //De-allocating the set
    delete treeOutput;
    
    if(res.body().back() == ',') {
        res.body().pop_back();
    }
    res.body() += "]";
    
    //Send the response to Grafana
    res.result(http::status::ok);
    return;
}

//Resolves a query given a time range and a list of sensors and returns it to Grafana.
void RestAPI::POST_query(endpointArgs) {
    
    std::string startTime, endTime;
    std::stringstream bodyss;
    std::list<std::string> sensors;
    boost::property_tree::ptree root;
    
    res.body() = "[";
    
    //Parse the body of the request, extracting time range and requested sensors.
    bodyss << req.body().c_str();
    boost::property_tree::read_json(bodyss, root);

    //Parse the time range.
    BOOST_FOREACH(boost::property_tree::ptree::value_type & range, root.get_child("range")) {
        if (boost::iequals(range.first, "from")) {
            startTime = range.second.data();
        } else if (boost::iequals(range.first, "to"))
            endTime = range.second.data();
    }
    startTime[startTime.find("T")] = ' ';
    startTime.pop_back();
    endTime[endTime.find("T")] = ' ';
    endTime.pop_back();
    
    //Parse the sensors.
    BOOST_FOREACH(boost::property_tree::ptree::value_type & target, root.get_child("targets")) {
        for(auto s = target.second.begin(); s!= target.second.end(); ++s) {
            if(s->first == "target")
                sensors.push_back(s->second.get_value<std::string>());
        }
    }
    
    //Prepare query
    DCDB::TimeStamp start, end;
    SensorMetadata sm;
    start = DCDB::TimeStamp(startTime, false);
    end   = DCDB::TimeStamp(endTime, false);
    std::list<DCDB::SensorDataStoreReading> results;
    
    for(auto& sensorName : sensors) {
        results.clear();
        try {
            sm = _metadataStore->get(sensorName);
        } catch(const std::invalid_argument &e) {}
        
        uint64_t sInterval = sm.getInterval() ? *sm.getInterval() : 0;
        double sScale = sm.getScale() ? *sm.getScale() : 1.0;
        std::string bestSmoother = "";
        
        if(sInterval && sm.getOperations() && !sm.getOperations()->empty()) {
            // Estimate the number of requested datapoints, for all sensors' data plotted in the panel. 
            // The number is calculated assuming all sensors plotted on the same panel have the same sampling period. 
            // If we exceed the maximum, then apply smoothing.
            uint64_t grafanaInterval = end.getRaw() - start.getRaw();
            int64_t bestRate = LLONG_MAX, smoothRate = LLONG_MAX;
            
            if((int)((grafanaInterval / sInterval) * sensors.size()) > MAX_DATAPOINTS) {
                for(const auto &op : *sm.getOperations()) {
                    //Looking for the smoother identifier string and getting its sampling interval in seconds
                    if(boost::regex_search(op.c_str(), _match, _smootherRegex) && boost::regex_search(op.c_str(), _match, _numRegex)) {
                        // Calculating the number of points for this smoother and updating the best one
                        smoothRate = (int64_t)(grafanaInterval / (stoull(_match.str(0)) * 1000000000)) - (int64_t)MAX_DATAPOINTS;
                        // Getting the smoother that is the closest to MAX_DATAPOINTS, and possibly smaller
                        if(sign(smoothRate) < sign(bestRate) || abs(smoothRate) < abs(bestRate)) {
                            bestRate = smoothRate;
                            bestSmoother = op;
                        }
                    }
                }
                //LOG(debug) << "Picking smoother " << bestSmoother << " for sensor " << sensorName << ".";
            }
        }
	
        //We need the Sensor ID, since smoothed sensors are not published.
        DCDB::SensorId sid(sensorName + bestSmoother);
        uint16_t startWs=start.getWeekstamp(), endWs=end.getWeekstamp();
        
        //Shoot the query for this sensor.
        for(uint16_t currWs=startWs; currWs<=endWs; currWs++) {
            sid.setRsvd(currWs);
            _sensorDataStore->query(results, sid, start, end, DCDB::AGGREGATE_NONE);
        }

        //Format the output for the response to Grafana.
        std::string datapoints = "[";
        for (auto& r : results) {
	    double val = (double) r.value;
	    DCDB::UnitConv::convertToBaseUnit(val, DCDB::UnitConv::fromString(*sm.getUnit()));
            datapoints += "[" + std::to_string(val * sScale) + "," + std::to_string(r.timeStamp.getRaw()/1000000) + "],";
	}
        if(datapoints.back() == ',')
            datapoints.pop_back();
        datapoints += "]";
        res.body() += "{\"target\":\"" + sensorName + "\",\"datapoints\":" + datapoints + "},";
    }
    
    if(res.body().back() == ',')
        res.body().pop_back();
    
    //Return the results to Grafana.
    res.body() += "]";
    res.result(http::status::ok);
    return;
}

void RestAPI::PUT_navigator(endpointArgs) {
    if(!buildTree()) {
        res.body() = "Sensor navigator could not be rebuilt.";
        res.result(http::status::internal_server_error);
    } else {
        res.body() = "Built a sensor navigator of size " + std::to_string(_navigator->getTreeSize()) + " and depth " + std::to_string(_navigator->getTreeDepth()) + ".";
        res.result(http::status::ok);
    }
}
