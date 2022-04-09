//================================================================================
// Name        : analyticscontroller.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Wrapper class implementation for AnalyticsManager.
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

#include "analyticscontroller.h"

void AnalyticsController::start() {
    _keepRunning = true;
    _readingCtr = 0;
    _mainThread = boost::thread(bind(&AnalyticsController::run, this));
}

void AnalyticsController::stop() {
    LOG(info) << "Stopping data analytics management thread...";
    _keepRunning = false;
    _mainThread.join();
    LOG(info) << "Stopping sensors...";
    _manager->stop();
    _manager->clear();
    LOG(info) << "Stopping worker threads...";
    _keepAliveWork.reset();
    _threads.join_all();
    _initialized = false;
}

bool AnalyticsController::initialize(Configuration& settings) {
    _settings = settings;
    _navigator = make_shared<SensorNavigator>();

    // A sensor navigator is only built if operator plugins are expected to be instantiated
    QueryEngine &_queryEngine = QueryEngine::getInstance();
    if(_manager->probe(settings.cfgFilePath, settings.cfgFileName)) {
        vector<string> topics;
        _metadataStore->wait();
        for(const auto& kv : _metadataStore->getMap())
            if(kv.second.isValid())
                topics.push_back(*kv.second.getPattern());
        _metadataStore->release();

        // Building the sensor navigator
        try {
            _navigator->setFilter(_settings.analyticsSettings.filter);
            _navigator->buildTree(_settings.analyticsSettings.hierarchy, &topics);
            LOG(info) << "Built a sensor hierarchy tree of size " << _navigator->getTreeSize() << " and depth " << _navigator->getTreeDepth() << ".";
        } catch (const std::invalid_argument &e) {
            _navigator->clearTree();
            LOG(error) << e.what();
            LOG(error) << "Failed to build sensor hierarchy tree!";
        }
        topics.clear();

        // Assigning the newly-built sensor navigator to the QueryEngine (even if uninitialized)
        _queryEngine.setNavigator(_navigator);
    }

    //TODO: find a better solution to disable the SensorBase default cache
    _settings.pluginSettings.cacheInterval = 0;
    if(!_manager->load(_settings.cfgFilePath, _settings.cfgFileName, _settings.pluginSettings)) {
        LOG(fatal) << "Failed to load data analytics manager!";
        return false;
    }

    if(!_queryEngine.updating.is_lock_free())
        LOG(warning) << "This machine does not support lock-free atomics. Performance may be degraded.";

    _initialized = true;
    return true;
}

void AnalyticsController::run() {
    // If initialization of data analytics fails, the thread terminates
    if(!_initialized)
        return;

    LOG(info) << "Init operators...";
    _manager->init();
    LOG(info) << "Starting operators...";
    _manager->start();
    LOG(info) << "Sensors started!";
    
    publishSensors();

    vector<op_dl_t>& _analyticsPlugins = _manager->getPlugins();
    
    DCDB::SensorId sid;
    list<DCDB::SensorDataStoreReading> readings;
    boost::lockfree::spsc_queue<reading_t> *sensorQueue;
    reading_t readingBuf;
    while (_keepRunning) {
        if (_doHalt) {
            _halted = true;
            sleep(2);
            continue;
        }
        _halted = false;
        int64_t realTTL = 0;
        // Push output analytics sensors
        for (auto &p : _analyticsPlugins) {
            if (_doHalt) break;
            for (const auto &op : p.configurator->getOperators())
                if(op->getStreaming()) {
                    for (const auto &u : op->getUnits())
                        for (const auto &s : u->getBaseOutputs())
                            if (s->getSizeOfReadingQueue() >= op->getMinValues() && sid.mqttTopicConvert(s->getMqtt())) {
                                readings.clear();
                                sensorQueue = s->getReadingQueue();
                                while (sensorQueue->pop(readingBuf)) {
                                    readings.push_back(DCDB::SensorDataStoreReading(sid, readingBuf.timestamp, readingBuf.value));
                                }
                                // The readings of operators that are dynamic (e.g., job operators) are not cached
                                if(!op->getDynamic()) {
                                    for(const auto &r : readings) {
                                        _sensorCache->storeSensor(sid, r.timeStamp.getRaw(), r.value);
                                    }
                                    _sensorCache->getSensorMap()[sid].updateBatchSize(op->getMinValues());
                                }
                                // Dynamic sensors (e.g., job aggregator) may have a TTL defined even if they are
                                // not published in the metadatastore
                                realTTL = _metadataStore->getTTL(s->getMqtt());
                                if( realTTL < 0 && s->getMetadata() && s->getMetadata()->getTTL() ) {
                                    realTTL = *(s->getMetadata()->getTTL()) / 1000000000;
                                }
                                _dcdbStore->insertBatch(readings, realTTL);
                                _readingCtr += readings.size();
                            }
                    op->releaseUnits();
                }
        }
        sleep(1);
    }
}

bool AnalyticsController::publishSensors() {
    // Performing auto-publish (if required) for the sensors instantiated by the data analytics framework
    if(!_settings.pluginSettings.autoPublish) return false;
    
    DCDB::SCError err;
    vector<op_dl_t>& _analyticsPlugins = _manager->getPlugins();
    bool failedPublish = false;
    uint64_t publishCtr = 0;
    for (auto &p : _analyticsPlugins)
        for (const auto &op : p.configurator->getOperators())
            if(op->getStreaming() && !op->getDynamic()) {
                for (const auto &u : op->getUnits())
                    for (const auto &s : u->getBaseOutputs()) {
                        if (s->getPublish()) {
                            if (s->getMetadata() && s->getMetadata()->isValid()) {
                                err = _dcdbCfg->publishSensor(*s->getMetadata());
                                _metadataStore->store(*s->getMetadata()->getPattern(), *s->getMetadata());
                            } else {
                                err = _dcdbCfg->publishSensor(s->getName().c_str(), s->getMqtt().c_str());
                            }
                            switch (err) {
                                case DCDB::SC_OK:
                                    publishCtr++;
                                    break;
                                case DCDB::SC_INVALIDPATTERN:
                                    LOG(error) << "Invalid sensor topic : " << s->getMqtt();
                                    failedPublish = true;
                                    break;
                                case DCDB::SC_INVALIDPUBLICNAME:
                                    LOG(error) << "Invalid sensor public name: " << s->getName();
                                    failedPublish = true;
                                    break;
                                case DCDB::SC_INVALIDSESSION:
                                    LOG(error) << "Cannot reach sensor data store.";
                                    failedPublish = true;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                op->releaseUnits();
            }

    if(failedPublish)
        LOG(error) << "Issues during sensor name auto-publish! Only " << publishCtr << " sensors were published.";
    else
        LOG(info) << "Sensor name auto-publish performed for " << publishCtr << " sensors!";
    return true;
}

bool AnalyticsController::rebuildSensorNavigator() {
    QueryEngine &qEngine = QueryEngine::getInstance();
    // Locking access to the QueryEngine
    qEngine.lock();

    _navigator = std::make_shared<SensorNavigator>();
    vector<string> topics;
    list<DCDB::PublicSensor> publicSensors;
    SensorMetadata sBuf;
    // Fetching sensor names and topics from the Cassandra datastore
    if(_dcdbCfg->getPublicSensorsVerbose(publicSensors)!=SC_OK)
        LOG(error) << "Failed to retrieve public sensors. Sensor Navigator will be empty.";
    for (const auto &s : publicSensors)
        if (!s.is_virtual) {
            sBuf = DCDB::PublicSensor::publicSensorToMetadata(s);
            if(sBuf.isValid())
                topics.push_back(*sBuf.getPattern());
        }
    publicSensors.clear();

    // Building the sensor navigator
    try {
        _navigator->setFilter(_settings.analyticsSettings.filter);
        _navigator->buildTree(_settings.analyticsSettings.hierarchy, &topics);
    } catch (const std::invalid_argument &e) {
        _navigator->clearTree();
        qEngine.getNavigator()->clearTree();
        qEngine.unlock();
        return false;
    }
    topics.clear();

    // Assigning the newly-built sensor navigator to the QueryEngine
    qEngine.setNavigator(_navigator);
    // Unlocking the QueryEngine
    qEngine.unlock();
    return true;
}
