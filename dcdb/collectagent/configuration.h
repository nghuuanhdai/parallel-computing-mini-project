//================================================================================
// Name        : configuration.h
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

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <string>
#include <unistd.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>


#include "logging.h"
#include "globalconfiguration.h"
#include "metadatastore.h"
#include <dcdb/sensorconfig.h>

#define DEFAULT_LISTENHOST "localhost"
#define DEFAULT_LISTENPORT "1883"
#define DEFAULT_RESTAPIHOST "0.0.0.0"
#define DEFAULT_RESTAPIPORT "8080"

using namespace std;

/**
 * Wrapper class for Cassandra-specific settings
 *
 * @ingroup ca
 */
class cassandra_t {
public:
    cassandra_t() {}
    std::string host = string(DEFAULT_CASSANDRAHOST);
    std::string port = string(DEFAULT_CASSANDRAPORT);
    std::string username = "";
    std::string password = "";
    uint32_t ttl = DEFAULT_CASSANDRATTL;
    uint32_t numThreadsIo = 1;
    uint32_t queueSizeIo = 4096;
    uint32_t coreConnPerHost = 1;
    bool debugLog = false;
};

class influx_measurement_t {
public:
    influx_measurement_t() {}
    std::string           mqttPart;
    std::string           tag;
    boost::regex          tagRegex;
    std::string           tagSubstitution;
    std::set<std::string> fields;
};


class influx_t {
public:
    influx_t() {}
    std::string           mqttPrefix;
    bool                  publish = false;
    std::map<std::string, influx_measurement_t> measurements;
};

/**
 * @brief Class responsible for reading collect agent specific configuration.
 *
 * @ingroup ca
 */
class Configuration : public GlobalConfiguration {

public:
    
    /**
     * @brief               Create new Configuration. Sets global config file to read from to cfgFile.
     * 
     * @param cfgFilePath	Path to where all config-files are located
     * @param cfgFileName   Name of the file containing the config
     */
    Configuration(const std::string& cfgFilePath, const std::string& cfgFileName) : GlobalConfiguration(cfgFilePath, cfgFileName) {
        restAPISettings.port = string(DEFAULT_RESTAPIPORT);
        restAPISettings.host = string(DEFAULT_RESTAPIHOST);
    }
    Configuration() {}
    
    virtual ~Configuration() {}
    
    // Additional configuration parameters to be parsed within the global block
    std::string mqttListenHost = string(DEFAULT_LISTENHOST);
    std::string mqttListenPort = string(DEFAULT_LISTENPORT);
    uint64_t cleaningInterval = 86400;
    uint64_t messageThreads = 128;
    uint64_t messageSlots = 16;
    cassandra_t cassandraSettings;
    influx_t influxSettings;
    
protected:
    void readAdditionalBlocks(boost::property_tree::iptree& cfg) override;
    bool readAdditionalValues(boost::property_tree::iptree::value_type &global) override;
};

#endif /* CONFIGURATION_H_ */

