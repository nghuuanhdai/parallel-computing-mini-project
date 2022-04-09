//================================================================================
// Name        : configuration.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Class responsible for reading collectagent specific configuration.
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

#include "configuration.h"

bool Configuration::readAdditionalValues(boost::property_tree::iptree::value_type &global) {
    // ----- READING ADDITIONAL GLOBAL SETTINGS -----
    if (boost::iequals(global.first, "mqttListenAddress")) {
        mqttListenHost = parseNetworkHost(global.second.data());
        mqttListenPort = parseNetworkPort(global.second.data());
        if(mqttListenPort=="") mqttListenPort = string(DEFAULT_LISTENPORT);
    } else if (boost::iequals(global.first, "cleaningInterval")) {
        cleaningInterval = stoul(global.second.data());
    } else if (boost::iequals(global.first, "messageThreads")) {
        messageThreads = stoul(global.second.data());
    } else if (boost::iequals(global.first, "messageSlots")) {
        messageSlots = stoul(global.second.data());
    } else {
        return false;
    }
    return true;
}

void Configuration::readAdditionalBlocks(boost::property_tree::iptree& cfg) {
    // ----- READING CASSANDRA SETTINGS -----
    if(cfg.find("cassandra") != cfg.not_found()) {
        BOOST_FOREACH(boost::property_tree::iptree::value_type & global, cfg.get_child("cassandra")) {
            if (boost::iequals(global.first, "address")) {
                cassandraSettings.host = parseNetworkHost(global.second.data());
                cassandraSettings.port = parseNetworkPort(global.second.data());
                if(cassandraSettings.port=="") cassandraSettings.port = string(DEFAULT_CASSANDRAPORT);
            } else if (boost::iequals(global.first, "username")) {
                cassandraSettings.username = global.second.data();
            } else if (boost::iequals(global.first, "password")) {
                cassandraSettings.password = global.second.data();
            } else if (boost::iequals(global.first, "ttl")) {
                cassandraSettings.ttl = stoul(global.second.data());
            } else if (boost::iequals(global.first, "numThreadsIo")) {
                cassandraSettings.numThreadsIo = stoul(global.second.data());
            } else if (boost::iequals(global.first, "queueSizeIo")) {
                cassandraSettings.queueSizeIo = stoul(global.second.data());
            } else if (boost::iequals(global.first, "coreConnPerHost")) {
                cassandraSettings.coreConnPerHost = stoul(global.second.data());
            } else if (boost::iequals(global.first, "debugLog")) {
                cassandraSettings.debugLog = to_bool(global.second.data());
            } else {
                LOG(warning) << "  Value \"" << global.first << "\" not recognized. Omitting";
            }
        }
    }
    
    // ----- READING INFLUXDB LINE PROTOCOL SETTINGS -----
    if (cfg.find("influx") != cfg.not_found()) {
	BOOST_FOREACH(boost::property_tree::iptree::value_type & global, cfg.get_child("influx")) {
	    if (boost::iequals(global.first, "mqttprefix")) {
		influxSettings.mqttPrefix = MQTTChecker::formatTopic(global.second.data());
	    } else if (boost::iequals(global.first, "publish")) {
		influxSettings.publish = to_bool(global.second.data());
	    } else if (boost::iequals(global.first, "measurement")) {
		influx_measurement_t measurement;
		BOOST_FOREACH(boost::property_tree::iptree::value_type &m, global.second) {
		    if (boost::iequals(m.first, "tag")) {
			measurement.tag = m.second.data();
		    } else if (boost::iequals(m.first, "tagfilter")) {
			//check if input has sed format of "s/.../.../" for substitution
			boost::regex  checkSubstitute("s([^\\\\]{1})([\\S|\\s]*)\\1([\\S|\\s]*)\\1");
			boost::smatch matchResults;
			
			if (regex_match(m.second.data(), matchResults, checkSubstitute)) {
			    //input has substitute format
			    measurement.tagRegex = boost::regex(matchResults[2].str(), boost::regex_constants::extended);
			    measurement.tagSubstitution = matchResults[3].str();
			} else {
			    //input is only a regex
			    measurement.tagRegex = boost::regex(m.second.data(), boost::regex_constants::extended);
			    measurement.tagSubstitution = "&";
			}
		    } else if (boost::iequals(m.first, "mqttpart")) {
			measurement.mqttPart = MQTTChecker::formatTopic(m.second.data());
		    } else if (boost::iequals(m.first, "fields")) {
			std::stringstream ss(m.second.data());
			while (ss.good()) {
			    std::string s;
			    getline(ss, s, ',');
			    measurement.fields.insert(s);
			}
		    }
		}
		if (measurement.mqttPart.size() == 0) {
		    // if no mqttpart is given, we use the measurement
		    measurement.mqttPart = MQTTChecker::formatTopic(global.second.data());
		}
		influxSettings.measurements[global.second.data()] = measurement;
	    }
	}
    }
}
