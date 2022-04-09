//================================================================================
// Name        : Configuration.cpp
// Author      : Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Class responsible for reading grafana server specific configuration.
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
    if (boost::iequals(global.first, "tempdir")) {
         tempdir = global.second.data();
    } else {
        return false;
    }
    return true;
}

void Configuration::readAdditionalBlocks(boost::property_tree::iptree& cfg) {
    
    //----- READING HIERARCHY SETTINGS -----
    BOOST_FOREACH(boost::property_tree::iptree::value_type &global, cfg.get_child("hierarchy")) {
        
        if (boost::iequals(global.first, "separator")) {
            hierarchySettings.separator = global.second.data();
        } else if (boost::iequals(global.first, "regex")) {
            hierarchySettings.regex = global.second.data();
        } else if (boost::iequals(global.first, "filter")) {
            hierarchySettings.filter = global.second.data();
        } else if (boost::iequals(global.first, "smootherRegex")) {
            hierarchySettings.smootherRegex = global.second.data();
        } else {
            LOG(warning) << "  Value \"" << global.first << "\" not recognized. Omitting";
        }
    }
    
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
}
