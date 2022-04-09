//================================================================================
// Name        : analyticscontroller.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Wrapper class for OperatorManager.
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

#ifndef PROJECT_ANALYTICSCONTROLLER_H
#define PROJECT_ANALYTICSCONTROLLER_H

#include <list>
#include <vector>
#include <dcdb/sensordatastore.h>
#include <dcdb/sensorconfig.h>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include "../analytics/OperatorManager.h"
#include "sensornavigator.h"
#include "sensorcache.h"
#include "configuration.h"
#include "logging.h"
#include "metadatastore.h"

using namespace std;

/**
* @brief Class implementing a wrapper around the OperatorManager
* 
* @details This class provides a wrapper around many features required for the
*          instantiation of the Operator Manager - namely the instantiation of
*          a SensorNavigator, the creation of a dedicated thread pool, and the
*          insertion of generated sensor values into the Cassandra datastore.
*          Most of these features were already available in DCDBPusher to handle
*          regular sensors, but they had to be ported over to the CollectAgent.
*
* @ingroup ca
*/
class AnalyticsController {

public:

    /**
    * @brief            Class constructor
    * 
    * @param dcdbCfg    SensorConfig object to be used to retrieve sensor meta-data from Cassandra
    * @param dcdbStore  SensorDataStore object to be used to insert sensor readings into Cassandra
    */
    AnalyticsController(DCDB::SensorConfig *dcdbCfg, DCDB::SensorDataStore *dcdbStore, boost::asio::io_context& io)
        : _dcdbCfg(dcdbCfg),
	  _dcdbStore(dcdbStore) {
        _manager = make_shared<OperatorManager>(io);
        _navigator = nullptr;
        _sensorCache = nullptr;
        _metadataStore = nullptr;
        _keepRunning = false;
        _doHalt = false;
        _halted = true;
        _initialized = false;
    }

    /**
    * @brief            Class destructor
    */
    ~AnalyticsController() {}

    /**
    * @brief            Starts the internal thread of the controller
    *
    *                   Initialization must have been performed already at this point.
    */
    void start();

    /**
    * @brief            Stops the internal management thread
    *
    *                   This will also stop and join all threads in the BOOST ASIO pool.
    */
    void stop();
    
    /**
    * @brief            Initializes the data analytics infrastructure
    *
    *                   This method will build a Sensor Navigator by fecthing sensor names from the Cassandra datastore,
    *                   and then create an OperatorManager object, which will take care of instantiating and 
    *                   preparing plugins.
    *                   
    * @param settings   Settings class containing user-specified configuration parameters                 
    * @param configPath Path to the configuration files for the data analytics framework
    * @return           True if successful, false otherwise 
    */
    bool initialize(Configuration& settings);

    /**
    * @brief            Sets the cache to be used for sensors
    *
    *                   This method allows to set the SensorCache object to be used to store sensor readings produced
    *                   by the data analytics framework.
    * @param cache      The SensorCache object to be used as cache
    */
    void setCache(SensorCache* cache)            { _sensorCache = cache; }
    
    /**
    * @brief            Sets the internal MetadataStore object to retrieve sensor information 
    * 
    * @param mStore     A pointer to a valid MetadataStore object
    */
    void setMetadataStore(MetadataStore* mStore) { _metadataStore = mStore; }

    /**
    * @brief            Returns the status of the internal thread
    *
    * @return           True if the controller is currently stopped, false otherwise
    */
    bool isHalted() { return _halted; }

    /**
    * @brief            Triggers a temporary halt of the internal management thread
    * 
    * @param wait       If set to true, the method returns only when the thread has stopped
    */
    void halt(bool wait=false) { 
        _doHalt = true; 
        if(wait) while (!_halted) { sleep(1); }
    }

    /**
    * @brief            Resumes the internal management thread
    */
    void resume() { _doHalt = false; }

    /**
    * @brief            Returns the internal OperatorManager object
    *
    * @return           A shared pointer to the internal OperatorManager object
    */
    shared_ptr<OperatorManager> getManager()    { return _manager; }

    /**
    * @brief            Returns the internal SensorNavigator object
    *
    * @return           A shared pointer to the internal SensorNavigator object
    */
    shared_ptr<SensorNavigator>  getNavigator()  { return _navigator; }

    /**
    * @brief            Returns the SensorCache object used to store readings
    *
    * @return           A pointer to a SensorCache object
    */
    SensorCache*                 getCache()      { return _sensorCache; }

    /**
    * @brief            Returns an insert counter for data analytics readings
    * 
    *                   This counter keeps track of how many inserts were performed into the Cassandra datastore for
    *                   data analytics-related operations since the last call to this method.
    *
    * @return           An insert counter to the Cassandra datastore
    */
    uint64_t                     getReadingCtr() { uint64_t ctr=_readingCtr; _readingCtr=0; return ctr; }

    /**
     * @brief   Rebuilds the internal sensor navigator.
     * 
     *          This method does not rely on a MetadataStore, unlike initialize(), but queries the list of public 
     *          sensors on its own; this was done to prevent race conditions with the collectagent's MetadataStore.
     * 
     * @return  True if successful, false otherwise
     */
    bool                        rebuildSensorNavigator();

private:

    // Method implementing the main loop of the internal thread
    void run();
    // Performs auto-publish of data analytics sensors, if required
    bool publishSensors();

    // Flag to keep track of the thread's status
    bool _keepRunning;
    // Flag to trigger temporary stops to the data analytics controller
    bool _doHalt;
    bool _halted;
    bool _initialized;
    // Objects to connect to the Cassandra datastore
    DCDB::SensorConfig *_dcdbCfg;
    DCDB::SensorDataStore *_dcdbStore;
    // Global sensor cache object for the collectagent
    SensorCache *_sensorCache;
    // Global sensor metadata store object
    MetadataStore *_metadataStore;
    // Sensor navigator
    shared_ptr<SensorNavigator> _navigator;
    // Internal data operator manager object
    shared_ptr<OperatorManager> _manager;
    // Misc configuration attributes
    Configuration _settings;
    // Readings counter
    uint64_t _readingCtr;

    // Main management thread for the analytics controller
    boost::thread _mainThread;
    // Underlying thread pool
    boost::thread_group _threads;
    // Dummy task to keep thread pool alive
    shared_ptr<boost::asio::io_service::work> _keepAliveWork;

    //Logger object
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
};

#endif //PROJECT_ANALYTICSCONTROLLER_H
