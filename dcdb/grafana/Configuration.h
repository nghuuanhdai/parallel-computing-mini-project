//================================================================================
// Name        : Configuration.h
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

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "globalconfiguration.h"

#include <set>

#define DEFAULT_GRAFANAHOST "127.0.0.1"
#define DEFAULT_GRAFANAPORT "8081"

/**
 * Struct defining the Cassandra settings.
 * It holds connection settings of the Apache Cassandra server (hostname and port).
 */
typedef struct {
    std::string host;
    std::string port;
    std::string username;
    std::string password;
    uint32_t numThreadsIo;
    uint32_t queueSizeIo;
    uint32_t coreConnPerHost;
    bool debugLog;
} cassandraSettings_t;

/**
 * Struct defining the hierarchy settings.
 * It holds the regular expressions defining the different elements of the datacenter.
 */
typedef struct {
    std::string separator;
    std::string regex;
    std::string filter;
    std::string smootherRegex;
} hierarchySettings_t;

/**
 * @brief Class responsible for reading grafanaserver specific configuration.
 *
 * @ingroup pusher
 */
class Configuration : public GlobalConfiguration {

public:
	
    cassandraSettings_t cassandraSettings;
    hierarchySettings_t hierarchySettings;
    
    // Additional configuration parameter to be parsed within the global block
    std::string tempdir = DEFAULT_TEMPDIR;
    
	/**
     * Create new Configuration. Sets global config file to read from to cfgFile.
     * 
     * @param cfgFilePath	Path to where all config-files are located
     * @param cfgFileName   Name of the file containing the config
     */
	Configuration(const std::string& cfgFilePath, const std::string& cfgFileName) : GlobalConfiguration(cfgFilePath, cfgFileName) {
        
        restAPISettings.host = std::string(DEFAULT_GRAFANAHOST);
        restAPISettings.port = std::string(DEFAULT_GRAFANAPORT);
        
        cassandraSettings.host = std::string(DEFAULT_CASSANDRAHOST);
        cassandraSettings.port = std::string(DEFAULT_CASSANDRAPORT);
        cassandraSettings.username = "";
        cassandraSettings.password = "";
        cassandraSettings.numThreadsIo = 1;
        cassandraSettings.queueSizeIo = 4096;
        cassandraSettings.coreConnPerHost = 1;
        cassandraSettings.debugLog = false;
        
        hierarchySettings.separator = "";
        hierarchySettings.regex = "";
        hierarchySettings.filter = "";
        hierarchySettings.smootherRegex = "";
    }
	
	virtual ~Configuration() {}
	
protected:
    void readAdditionalBlocks(boost::property_tree::iptree& cfg) override;
    bool readAdditionalValues(boost::property_tree::iptree::value_type &global) override;
};

#endif /* CONFIGURATION_H_ */
