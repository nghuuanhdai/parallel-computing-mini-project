//================================================================================
// Name        : mqttchecker.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Class that manages constraints for MQTT topic formatting.
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

#ifndef PROJECT_MQTTCHECKER_H
#define PROJECT_MQTTCHECKER_H

#include <set>
#include <algorithm>
#include "logging.h"

#define MQTT_SEP '/'
#define NAME_SEP '.'
#define JOB_STR "job"

/**
 * @brief Class that manages constraint for MQTT topic formatting
 *
 * @details This class is implemented as a singleton. It maintains an internal
 *          set containing the currently used MQTT topics. Each used sensor
 *          topic should be checked with the "check" method.
 *
 * @ingroup common
 */
class MQTTChecker {

public:
    
    /**
    * @brief            Returns an instance to a MQTTChecker object
    *
    *                   The MQTTChecker class is implemented as a singleton: therefore, all entities calling the
    *                   getInstance method will share the same instance of the MQTTChecker class.
    *
    * @returns          Reference to a MQTTChecker object
    */
    static MQTTChecker& getInstance() {
        static MQTTChecker m;
        return m;
    }

    /**
     * @brief           Converts a numerical job ID to its internal MQTT topic representation
     * 
     * @param jobId     The job ID value to be processed
     * @return          The processed MQTT topic
     */
    static std::string jobToTopic(std::string jobId) {
        return "/job" + jobId + "/";
    }

    /**
     * @brief           Converts an MQTT topic to its corresponding numerical job ID, if applicable
     * 
     * @param topic     The topic to be processed 
     * @return          The numerical job ID
     */
    static std::string topicToJob(const std::string& topic) {
        std::string jobKey(JOB_STR), jobId = topic;
        jobId.erase(std::remove(jobId.begin(), jobId.end(), MQTT_SEP), jobId.end());
        size_t pos = jobId.find(jobKey);
        if (pos != std::string::npos) 
            jobId.erase(pos, jobKey.length());
        return jobId;
    }
    
    /**
     * @brief           Converts an MQTT topic to a filesystem path representation
     * 
     * @param topic     The MQTT topic to be processed 
     * @param path      The base destination path
     * @return          A processed file system path
     */
    static std::string topicToFile(const std::string& topic, const std::string& path="") {
        std::string tPath = path;
        if(tPath.empty())
            tPath = "./";
        else if(tPath[tPath.size()-1]!='/')
            tPath += '/';
        return tPath + topicToName(topic);
    }

    /**
     * @brief           Converts a sensor name to its internal MQTT topic representation
     * 
     * @param name      The sensor name to be processed 
     * @return          The processed MQTT topic
     */
    static std::string nameToTopic(const std::string& name) {
        if(name.empty()) return name;
        std::string newTopic = name;
        std::replace(newTopic.begin(), newTopic.end(), NAME_SEP, MQTT_SEP);
        if(!newTopic.empty() && newTopic[0] != MQTT_SEP) newTopic.insert(0, 1, MQTT_SEP);
        return newTopic;
    }

    /**
     * @brief           Converts an MQTT topic to the name representation exposed to users
     * 
     * @param topic     The topic to be processed 
     * @return          The processed sensor name
     */
    static std::string topicToName(const std::string& topic) {
        if(topic.empty()) return topic;
        std::string newName = topic;
        std::replace(newName.begin(), newName.end(), MQTT_SEP, NAME_SEP);
        if(!newName.empty() && newName[0] == NAME_SEP) newName.erase(0, 1);
        return newName;
    }
    
    /**
     * @brief           Sanitizes and formats a MQTT topic or suffix
     * 
     * @param topic     The topic or suffix to be processed 
     * @param cpuID     The cpu ID associated to this topic (if any)
     * @return          The processed MQTT topic or suffix
     */
    static std::string formatTopic(const std::string& topic, int cpuID=-1) {
        if(topic.empty()) return topic;
        std::string newTopic = topic;
        // Stripping off all forward slashes
        while(!newTopic.empty() && newTopic.front()==MQTT_SEP) newTopic.erase(0, 1);
        while(!newTopic.empty() && newTopic.back()==MQTT_SEP)  newTopic.erase(newTopic.size()-1, 1);
        // Adding the one front forward slash that we need
        newTopic.insert(0, 1, MQTT_SEP);
        return cpuID<0 ? newTopic : "/cpu" + std::to_string(cpuID) + newTopic;
    }

    /**
    * @brief            Resets the internal topics set
    */
    void reset()           { _topics.clear(); _groups.clear(); }

    /**
    * @brief            Removes a topic from the internal set
    *
    *                   This method should be used to remove obsolete topics from the set of active topics. This
    *                   usually happens when plugins are reloaded in dcdbpusher, and all old sensors are destroyed.
    *
    * @param topic      The topic to be removed
    */
    void removeTopic(const std::string& topic) {
        std::string str(topic);
        str.erase(std::remove(str.begin(), str.end(), MQTT_SEP), str.end());
        _topics.erase(str);
    }

    /**
    * @brief            Performs a check on a certain MQTT topic
    *
    *                   The check is passed if the topic is not present already in the internal set (to prevent duplicate
    *                   topics) and if all other internal checks are cleared.
    *
    * @param topic      An arbitrary MQTT topic to check
    * @return           True if the topic is valid, False otherwise
    */
    bool checkTopic(const std::string& topic) {
        //We remove all '/' characters to detect duplicates
        std::string str(topic);
        str.erase(std::remove(str.begin(), str.end(), MQTT_SEP), str.end());
        auto returnIt = _topics.insert(str);
        if (!returnIt.second) {
            LOG(error) << "MQTT-Topic \"" << topic << "\" used twice!";
            return false;
        }
        return true;
    }

    /**
    * @brief            Removes a name from the internal set of sensor names
    *
    *                   This method should be used to remove obsolete sensor names, e.g. when reloading plugins.
    *                   This is useful only in the case that MQTT topics differ from the actual sensor names.
    *
    * @param name      The name (string) to be removed
    */
    void removeName(const std::string& name) {
        _names.erase(name);
    }

    /**
    * @brief            Performs a check on a certain name
    *
    *                   The check is passed if the name for the group, analyzer or entity is not used already.
    *
    * @param name      An arbitrary name (string) to check
    * @return           True if the name is valid, False otherwise
    */
    bool checkName(const std::string& name) {
        auto returnIt = _names.insert(name);
        if (!returnIt.second) {
            LOG(error) << "Name \"" << name << "\" used twice!";
            return false;
        }
        return true;
    }

    /**
    * @brief            Removes a name from the internal set of entities
    *
    *                   This method should be used to remove obsolete sensor groups, entities or analyzers once they
    *                   are destroyed, e.g. on plugin reload actions.
    *
    * @param name      The name (string) to be removed
    */
    void removeGroup(const std::string& name) {
        _groups.erase(name);
    }

    /**
    * @brief            Performs a check on a certain group name
    *
    *                   The check is passed if the name for the group, analyzer or entity is not used already.
    *
    * @param name      An arbitrary name (string) to check
    * @return           True if the name is valid, False otherwise
    */
    bool checkGroup(const std::string& name) {
        auto returnIt = _groups.insert(name);
        if (!returnIt.second) {
            LOG(error) << "Group name \"" << name << "\" used twice!";
            return false;
        }
        return true;
    }

private:

    /**
    * @brief            Private class constructor
    */
    MQTTChecker() {}

    /**
    * @brief            Copy constructor is not available
    */
    MQTTChecker(MQTTChecker const&)     = delete;

    /**
    * @brief            Assignment operator is not available
    */
    void operator=(MQTTChecker const&)  = delete;

    // Set used to keep track of topics
    std::set<std::string> _topics;
    // Set used to keep track of groups, analyzers and other entities
    std::set<std::string> _groups;
    // Set used to keep track of sensor names (if different from the respective topics)
    std::set<std::string> _names;

    // Logger object to notify MQTT check outcome
    logger_t lg;
};

#endif //PROJECT_MQTTCHECKER_H
