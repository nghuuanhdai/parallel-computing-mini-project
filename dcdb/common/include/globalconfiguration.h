//================================================================================
// Name        : globalconfiguration.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Common functionality for reading in configuration files.
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

/**
 * @defgroup globalconf Global Configuration
 * @ingroup common
 *
 * @brief Groups all definitions related to configuring DCDB applications.
 */

#ifndef PROJECT_GLOBALCONFIGURATION_H
#define PROJECT_GLOBALCONFIGURATION_H

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/algorithm/string.hpp>
#include "logging.h"
#include "mqttchecker.h"

#define DEFAULT_LOGLEVEL 3
#define DEFAULT_THREADS 8
#define DEFAULT_TEMPDIR "./"
#define DEFAULT_CASSANDRAHOST "127.0.0.1"
#define DEFAULT_CASSANDRAPORT "9042"
#define DEFAULT_CASSANDRATTL 0


class RESTHttpsServer;

/**
 * @brief Wrapper class for plugin-specific settings
 *
 * @ingroup globalconf
 */
class pluginSettings_t {
public:
    pluginSettings_t() {}
    std::string mqttPrefix = "";
    std::string	tempdir = DEFAULT_TEMPDIR;
    bool autoPublish = false;
    unsigned int cacheInterval = 900000;
};

/**
 * @brief Wrapper class for REST API server-related settings
 *
 * @ingroup globalconf
 */
class serverSettings_t {
public:
    serverSettings_t() {}
    bool enabled = false;
    std::string host = "";/**< Host name/IP address to listen on */
    std::string port = "8000";/**< Port to listen on */
    std::string certificate = "";/**< Certificate chain file in PEM format */
    std::string privateKey = "";/**< Private key file in PEM format */
};

/**
 * @brief Wrapper class for data analytics-related settings
 *
 * @ingroup globalconf
 */
class analyticsSettings_t {
public:
    analyticsSettings_t() {}
    std::string hierarchy = "";
    std::string filter = "";
    std::string jobFilter = "";
    std::string jobMatch = "";
    std::string jobIdFilter = "";
    std::string jobDomainId = "default";
};

/**
 * @brief     Parses a host:port string and returns the host
 *
 * @param str The input string to be parsed
 * @return    The host-related part of the string
 * 
 * @ingroup globalconf
 */
static std::string parseNetworkHost(std::string str) {
    size_t pos = str.find(":");
    if (pos != std::string::npos)
        str.erase(pos);
    return str;
}

/**
 * @brief     Parses a host:port string and returns the port
 *
 * @param str The input string to be parsed
 * @return    The port-related part of the string
 * 
 * @ingroup globalconf
 */
static std::string parseNetworkPort(std::string str) {
    size_t pos = str.find(":");
    if (pos != std::string::npos)
        str = str.substr(pos+1);
    else
        str = "";
    return str;
}

/**
 * @brief   Converts the input string to a boolean
 *
 * @param s The input string
 * @return  The corresponding boolean value
 * 
 * @ingroup globalconf
 */
static bool to_bool(const std::string& s) {
    return s=="true" || s=="on";
}

/**
 * @brief   Converts the input boolean to a string
 *
 * @param b The input boolean
 * @return  The corresponding string value
 * 
 * @ingroup globalconf
 */
static std::string bool_to_str(const bool& b) { 
    return (b ? "true" : "false"); 
}

/**
 * @brief This class contains the logic to parse and store configurations in
 *        the form of BOOST INFO trees. The configuration parameters which are
 *        parsed here are only those shared between several components of DCDB,
 *        i.e. dcdbpusher and collectagent. Further specific parameters can be
 *        parsed by extending this class appropriately.
 *
 * @details All configuration parameters are freely accessible as public
 *          members of the class. While encapsulation properties are lost, this
 *          makes the codes considerably lighter and more readable.
 *
 * @ingroup globalconf
 */
class GlobalConfiguration {
public:
    /**
     * @brief               Class constructor
     * 
     * @param cfgFilePath   Path to the directory containing the configuration file 
     * @param cfgFileName   Name of the configuration file
     */
    GlobalConfiguration(const std::string& cfgFilePath, const std::string& cfgFileName);
    
    /**
     * @brief               Default constructor
     */
    GlobalConfiguration() {}

    /**
     * @brief               Default destructor
     */
    virtual ~GlobalConfiguration() {}

    /**
     * @brief               Reads in the config from the specified file
     * 
     * @return              True if successful, false otherwise 
     */
    void readConfig();
    
    /**
     * @brief Reads user credentials from the config file and accordingly adds
     *        users to a RESTHttpsServer.
     *
     * @param server    The Rest API server where to add the users.
     * @return  True on success, false otherwise.
     */
    bool readRestAPIUsers(RESTHttpsServer* server);

    // Global configuration members directly accessible from the class
    bool validateConfig = false;
    bool daemonize = false;
    int statisticsInterval = 60;
    std::string statisticsMqttPart;
    uint64_t threads = DEFAULT_THREADS;
    int logLevelFile = -1;
    int logLevelCmd = DEFAULT_LOGLEVEL;
    analyticsSettings_t analyticsSettings;
    serverSettings_t restAPISettings;
    pluginSettings_t pluginSettings;
    std::string cfgFilePath;
    std::string cfgFileName;

protected:
    /**
     * @brief               Virtual method to integrate additional configuration blocks
     * 
     *                      Additional configuration blocks are defined as nodes in the INFO configuration tree. These
     *                      are distinct from the global and the predefined blocks.
     * 
     * @param cfg           Reference to a BOOST iptree object
     */
    virtual void readAdditionalBlocks(boost::property_tree::iptree& cfg) {}
    
    /**
     * @brief               Virtual method to integrate additional configuration members
     * 
     *                      The configuration members defined here will be parsed within the "global" block of the
     *                      configuration, and not in separate blocks.
     * 
     * @param global        The BOOST iptree node containing the configuration parameter
     * @return              True if the parameter was recognized, false otherwise
     */
    virtual bool readAdditionalValues(boost::property_tree::iptree::value_type &global) { return false; }
    
    // Logger object
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
};

#endif //PROJECT_GLOBALCONFIGURATION_H
