//================================================================================
// Name        : globalconfiguration.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Common functionality implementation for reading in configuration files.
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

#include "globalconfiguration.h"
#include "RESTHttpsServer.h"
#include "boost/filesystem.hpp"

GlobalConfiguration::GlobalConfiguration(const std::string& cfgFilePath, const std::string& cfgFileName) {
    boost::filesystem::path p(cfgFilePath);
    if (boost::filesystem::exists(p)) {
	if (boost::filesystem::is_directory(p)) {
	    this->cfgFileName = cfgFileName;
	    this->cfgFilePath = cfgFilePath;
	} else {
	    this->cfgFileName = p.filename().native();
	    this->cfgFilePath = p.parent_path().native();
	}
	if (this->cfgFilePath[this->cfgFilePath.length()-1] != '/')
	    this->cfgFilePath.append("/");
    } else {
	throw std::runtime_error(cfgFilePath + " does not exist");
    }
}

void GlobalConfiguration::readConfig() {
    // Open file
    std::string globalConfig = cfgFilePath;
    globalConfig.append(cfgFileName);
    boost::property_tree::iptree cfg;
    // Parse to property_tree
    try {
        boost::property_tree::read_info(globalConfig, cfg);
    } catch (boost::property_tree::info_parser_error& e) {
        throw std::runtime_error("Error when parsing " + globalConfig + ": " + e.what());
    }
    
    BOOST_FOREACH(boost::property_tree::iptree::value_type &global, cfg.get_child("global")) {
        // ----- READING PLUGIN SETTINGS -----
        if (boost::iequals(global.first, "mqttprefix")) {
            pluginSettings.mqttPrefix = MQTTChecker::formatTopic(global.second.data());
        } else if (boost::iequals(global.first, "autoPublish")) {
            pluginSettings.autoPublish = to_bool(global.second.data());
        } else if (boost::iequals(global.first, "tempdir")) {
            pluginSettings.tempdir = global.second.data();
            if (pluginSettings.tempdir[pluginSettings.tempdir.length() - 1] != '/')
                pluginSettings.tempdir.append("/");
        } else if (boost::iequals(global.first, "cacheInterval")) {
            pluginSettings.cacheInterval = stoul(global.second.data()) * 1000;
        // ----- READING GLOBAL SETTINGS -----
        } else if (boost::iequals(global.first, "threads")) {
            threads = stoi(global.second.data());
        } else if (boost::iequals(global.first, "daemonize")) {
            daemonize = to_bool(global.second.data());
        } else if (boost::iequals(global.first, "validateConfig")) {
            validateConfig = to_bool(global.second.data());
        } else if (boost::iequals(global.first, "verbosity")) {
            logLevelFile = stoi(global.second.data());
        } else if (boost::iequals(global.first, "statisticsInterval")) {
            statisticsInterval = stoi(global.second.data());
	} else if (boost::iequals(global.first, "statisticsMqttPart")) {
	    statisticsMqttPart = MQTTChecker::formatTopic(global.second.data());
        } else if (!readAdditionalValues(global)) {
            LOG(warning) << "  Value \"" << global.first << "\" not recognized. Omitting";
        }
    }

    // ----- READING ANALYTICS SETTINGS -----
    if(cfg.find("analytics") != cfg.not_found()) {
        BOOST_FOREACH(boost::property_tree::iptree::value_type & global, cfg.get_child("analytics")) {
            if (boost::iequals(global.first, "hierarchy")) {
                analyticsSettings.hierarchy = global.second.data();
            } else if (boost::iequals(global.first, "filter")) {
                analyticsSettings.filter = global.second.data();
            } else if (boost::iequals(global.first, "jobFilter")) {
                analyticsSettings.jobFilter = global.second.data();
            } else if (boost::iequals(global.first, "jobMatch")) {
                analyticsSettings.jobMatch = global.second.data();
            } else if (boost::iequals(global.first, "jobIdFilter")) {
                analyticsSettings.jobIdFilter = global.second.data();
            } else if (boost::iequals(global.first, "jobDomainId")) {
                analyticsSettings.jobDomainId = global.second.data();
            } else {
                LOG(warning) << "  Value \"" << global.first << "\" not recognized. Omitting";
            }
        }
    }

    // ----- READING REST API SETTINGS -----
    if(cfg.find("restAPI") != cfg.not_found()) {
	restAPISettings.enabled = true;
        BOOST_FOREACH(boost::property_tree::iptree::value_type & global, cfg.get_child("restAPI")) {
            if (boost::iequals(global.first, "address")) {
                restAPISettings.host = global.second.data();
                size_t pos = restAPISettings.host.find(":");
                if (pos != std::string::npos) {
                    restAPISettings.port = restAPISettings.host.substr(pos + 1);
                    restAPISettings.host.erase(pos);
                }
            } else if (boost::iequals(global.first, "certificate")) {
                restAPISettings.certificate = global.second.data();
            } else if (boost::iequals(global.first, "privateKey")) {
                restAPISettings.privateKey = global.second.data();
            } else if (boost::iequals(global.first, "user")) {
                //Avoids unnecessary "Value not recognized" message
            } else {
                LOG(warning) << "  Value \"" << global.first << "\" not recognized. Omitting";
            }
        }
    }

    readAdditionalBlocks(cfg);
}

bool GlobalConfiguration::readRestAPIUsers(RESTHttpsServer* server) {
    //open file
    std::string globalConfig = cfgFilePath;
    globalConfig.append(cfgFileName);

    boost::property_tree::iptree cfg;
    //parse to property_tree
    try {
        boost::property_tree::read_info(globalConfig, cfg);
    } catch (boost::property_tree::info_parser_error& e) {
        LOG(error) << "Error when reading users from " << cfgFileName << ":" << e.what();
        return false;
    }

    //read users
    BOOST_FOREACH(boost::property_tree::iptree::value_type &global, cfg.get_child("restAPI")) {
        if (boost::iequals(global.first, "user")) {
            std::string username = global.second.data();
            userAttributes_t attributes;
#ifdef DEBUG
            LOG(info) << "Username: \"" << username << "\"";
#endif
            BOOST_FOREACH(boost::property_tree::iptree::value_type &perm, global.second) {
                if (boost::iequals(perm.first, "GET")) {
#ifdef DEBUG
                    LOG(info) << "  Permission \"GET\"";
#endif
                    attributes.second[GET] = true;
                } else if (boost::iequals(perm.first, "PUT")) {
#ifdef DEBUG
                    LOG(info) << "  Permission \"PUT\"";
#endif
                    attributes.second[PUT] = true;
                } else if (boost::iequals(perm.first, "POST")) {
#ifdef DEBUG
                    LOG(info) << "  Permission \"POST\"";
#endif
                    attributes.second[POST] = true;
                } else if (boost::iequals(perm.first, "password")) {
                    attributes.first = perm.second.data();
#ifdef DEBUG
                    LOG(info) << "  Password: \"" << attributes.first << "\"";
#endif
                } else {
#ifdef DEBUG
                    LOG(warning) << "Permission \"" << perm.first << "\" not recognized. Omitting";
#endif
                }
            }
	    if (attributes.first.size() != 40) {
		LOG(warning) << "User " << username << "'s password does not appear to be a sha1 hash!";
	    } else if (server->addUser(username, attributes)) {
                LOG(warning) << "User " << username << " already existed and was overwritten!";
            }
        } else {
            // do nothing and continue search for the user section
        }
    }
    return true;
}
