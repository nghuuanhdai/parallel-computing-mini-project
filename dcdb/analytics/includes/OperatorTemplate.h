//================================================================================
// Name        : OperatorTemplate.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Template implementing features needed by Operators.
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

#ifndef PROJECT_OPERATORTEMPLATE_H
#define PROJECT_OPERATORTEMPLATE_H

#include "OperatorInterface.h"
#include "UnitTemplate.h"
#include "UnitGenerator.h"
#include "timestamp.h"
#include "QueryEngine.h"

#include <vector>
#include <map>
#include <memory>

using namespace std;

/**
 * @brief Template that implements features needed by Operators and complying
 *        to OperatorInterface.
 *
 * @details The template accepts any class derived from SensorBase, allowing
 *          users to add their own attributes to input and output sensors. This
 *          template also employs UnitTemplates, which are instantiated
 *          according to the input sensor type.
 *
 * @ingroup operator
 */
template <typename S>
class OperatorTemplate : public OperatorInterface {
    // The template shall only be instantiated for classes which derive from SensorBase
    static_assert(is_base_of<SensorBase, S>::value, "S must derive from SensorBase!");

protected:
    // For readability
    using S_Ptr = shared_ptr<S>;
    using U_Ptr = shared_ptr< UnitTemplate<S> >;

public:
    
    /**
    * @brief            Class constructor
    *
    * @param name       Name of the operator
    */
    OperatorTemplate(const string name) :
            OperatorInterface(name),
            _unitCache(nullptr),
            _insertionLUT(nullptr),
            _queryEngine(QueryEngine::getInstance()),
            _scheduledTime(0) {}

    /**
    * @brief            Copy constructor
    *
    *                   Achtung! The vectors of units will be copied in shallow fashion, and the underlying output
    *                   sensors across different copied units will point to the same location. This is intended,
    *                   as "duplicated" units share the same view of the data analytics module.
    *
    */
    OperatorTemplate(const OperatorTemplate& other) :
            OperatorInterface(other),
            _unitCache(nullptr),
            _insertionLUT(nullptr),
            _queryEngine(QueryEngine::getInstance()),
            _scheduledTime(0) {

        for(auto u : other._units) {
            _units.push_back(u);
            _baseUnits.push_back(u);
        }
    }

    /**
    * @brief            Assignment operator
    *
    *                   Here, the same considerations done for the copy constructor apply.
    *
    */
    OperatorTemplate& operator=(const OperatorTemplate& other) {
        OperatorInterface::operator=(other);
        _units.clear();
        _scheduledTime = 0;

        for(auto u : other._units) {
            _units.push_back(u);
            _baseUnits.push_back(u);
        }

        return *this;
    }

    /**
    * @brief            Class destructor
    */
    virtual ~OperatorTemplate() {
        _units.clear();
        _baseUnits.clear();

        if(_unitCache) {
            _unitCache->clear();
            delete _unitCache;
        }
        if(_insertionLUT) {
            _insertionLUT->clear();
            delete _insertionLUT;
        }
    }

    /**
    * @brief            Prints the current operator configuration
    *
    * @param ll         Logging level at which the configuration is printed
    */
    virtual void printConfig(LOG_LEVEL ll) override {
        if(_mqttPart!="")
            LOG_VAR(ll) << "            MQTT prefix:     " << _mqttPart;
        LOG_VAR(ll) << "            Disabled:        " << (_disabled ? "true" : "false");
        LOG_VAR(ll) << "            Sync readings:   " << (_sync ? "enabled" : "disabled");
        LOG_VAR(ll) << "            Streaming mode:  " << (_streaming ? "enabled" : "disabled");
        LOG_VAR(ll) << "            Duplicated mode: " << (_duplicate ? "enabled" : "disabled");
        LOG_VAR(ll) << "            MinValues:       " << _minValues;
        LOG_VAR(ll) << "            Interval:        " << _interval;
        LOG_VAR(ll) << "            Interval Delay:  " << _delayInterval;
		LOG_VAR(ll) << "            QueueSize:       " << _queueSize;
        LOG_VAR(ll) << "            Unit Cache Size: " << _unitCacheLimit;
        if(!_units.empty()) {
            LOG_VAR(ll) << "            Units:";
            if(_unitID<0)
                for (auto u : _units)
                    u->printConfig(ll, lg);
            else
                _units[_unitID]->printConfig(ll, lg);
        } else
            LOG_VAR(ll) << "            Units:           none";
    }

    /**
    * @brief              Adds an unit to this operator
    *
    * @param u            Shared pointer to a UnitInterface object
    */
    virtual void addUnit(UnitPtr u)  override {
        // Since the OperatorInterface method accepts UnitInterface objects, we must cast the input argument
        // to its actual type, which is UnitTemplate<S>
        if (U_Ptr dUnit = dynamic_pointer_cast< UnitTemplate<S> >(u)) {
            _units.push_back(dUnit);
            _baseUnits.push_back(u);
            if(dUnit->isTopUnit())
                for(auto& subUnit : dUnit->getSubUnits()) {
                    if (U_Ptr dSubUnit = dynamic_pointer_cast< UnitTemplate<S> >(subUnit))
                        _baseUnits.push_back(dSubUnit);
                    else
                        LOG(error) << "Operator " << _name << ": Type mismatch when storing sub-unit! Will be omitted";
                }
        }
        else
            LOG(error) << "Operator " << _name << ": Type mismatch when storing unit! Will be omitted";
    }

    /**
    * @brief              Returns the units of this operator
    *
    *                     The units returned by this method are of the UnitInterface type. The actual units, in their
    *                     derived type, are used internally. If the operator uses a lock to regulate unit access,
    *                     this will be acquired and must be released through the releaseUnits() method.
    *                     
    * @return             The vector of UnitInterface objects of this operator
    */
    virtual vector<UnitPtr>& getUnits() override	{ return _baseUnits; }
    
    /**
     * @brief             Releases the internal lock for unit access
     * 
     *                    This method is meant to regulate concurrent access to units which are generated dynamically.
     *                    In this specific implementation, the method does not perform anything as units in standard
     *                    operators are static and never modified.
     * 
     */
    virtual void releaseUnits() override {}

    /**
     * @brief              Clears all the units contained in this operator
     */
    virtual void clearUnits() override { _units.clear(); _baseUnits.clear(); _unitID = -1; }

	/**
	 * @brief              Returns the number of messages this operator will
	 *                     generate per second
	 *
	 * @return             Messages/s
	 */
	virtual float getMsgRate() override {
		float val = 0.0f;
        for (const auto &u : this->getUnits())
            for(const auto &s : u->getBaseOutputs()) {
                if (s->getSubsampling() > 0)
                    val += 1.0f / ((float) s->getSubsampling());
            }
        this->releaseUnits();
        return val * (1000.0f / (float)_interval) / (float)_minValues;
	}

    /**
    * @brief              Initializes this operator
    *
    *                     This method performs additional initialization compared to OperatorInterface. Specifically,
    *                     all output sensors in units are initialized, and their caches instantiated.
    *
    * @param io           Boost ASIO service to be used
    */
    virtual void init(boost::asio::io_service& io) final override {
        OperatorInterface::init(io);

        for(const auto& u : _units)
            u->init(_interval, _queueSize);

        this->execOnInit();
    }

    /**
    * @brief              Waits for the operator to complete its tasks
    *
    *                     Does a busy wait until all dispatched handlers are finished (_pendingTasks == 0).
    */
    virtual void wait() final override {
        uint64_t sleepms=10, i=0;
        uint64_t timeout = _interval<10000 ? 30000 : _interval*3;

        while(sleepms*i++ < timeout) {
            if (_pendingTasks)
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepms));
            else {
                this->execOnStop();
                LOG(info) << "Operator " << _name << " stopped.";
                return;
            }
        }

        LOG(warning) << "Operator " << _name << " will not finish! Skipping it";
    }

    /**
    * @brief              Starts this operator
    */
    virtual void start() final override {
        if(_keepRunning) {
            LOG(debug) << "Operator " << _name << " already running.";
            return;
        } else if(!_streaming) {
            LOG(error) << "On-demand operator " << _name << " cannot be started.";
            return;
        } else if(_disabled)
            return;

        if (!this->execOnStart()) {
            LOG(error) << "Operator " << _name << ": startup failed.";
            return;
        }

        _keepRunning = 1;
        _pendingTasks++;
        _timer->async_wait(bind(&OperatorTemplate<S>::computeAsync, this));
        LOG(info) << "Operator " << _name << " started.";
    }

    /**
    * @brief              Stops this operator
    */
    virtual void stop() final override {
        if(_keepRunning == 0 || !_streaming) {
            LOG(debug) << "Operator " << _name << " already stopped.";
            return;
        }

        _keepRunning = 0;
        //cancel any outstanding readAsync()
        _timer->cancel();
    }

    /**
    * @brief              Perform a REST-triggered PUT action
    *
    *                     This is a dummy implementation that can be overridden in user plugins. Any thrown
    *                     exceptions will be reported in the response string.
    *
    * @param action       Name of the action to be performed
    * @param queries      Vector of queries (key-value pairs)
    *
    * @return             Response to the request as a <response, data> pair
    */
    virtual restResponse_t REST(const string& action, const unordered_map<string, string>& queries) override {
        throw invalid_argument("Unknown plugin action " + action + " requested!");
    }

    /**
    * @brief              Performs an on-demand compute task
    *
    *                     Unlike the protected computeAsync and compute methods, computeOnDemand allows to interactively
    *                     perform data analytics queries on the operator, which must have the _streaming attribute set
    *                     to false. A unit is generated on the fly, corresponding to the input node given as input,
    *                     and results are returned in the form of a map.
    *
    * @param node         Unit name for which the query must be performed
    * @return             a map<string, reading_t> containing the output of the query
    */
    virtual map<string, reading_t> computeOnDemand(const string& node="__root__") override {
        map<string, reading_t> outMap;
        if( !_streaming && !_disabled ) {
            shared_ptr<SensorNavigator> navi = _queryEngine.getNavigator();
            UnitGenerator<S> unitGen(navi);
            // We check whether the input node belongs to this operator's unit domain
            if(!_unitCache)
                throw std::runtime_error("Initialization error in operator " + _name + "!");

            // Getting exclusive access to the operator
            while( _onDemandLock.exchange(true) ) {}

            // If the ondemand template unit refers to root it means it has been completely resolved already,
            //  and therefore we can use such unit without doing any resolution
            try {
                U_Ptr tempUnit = nullptr;
                if (_unitCache->count(node)) {
                    LOG(debug) << "Operator " << _name << ": cache hit for unit " << node << ".";
                    tempUnit = _unitCache->at(node);
                } else {
                    if (!_unitCache->count(SensorNavigator::templateKey))
                        throw std::runtime_error("No template unit in operator " + _name + "!");
                    LOG(debug) << "Operator " << _name << ": cache miss for unit " << node << ".";
                    U_Ptr uTemplate = _unitCache->at(SensorNavigator::templateKey);
                    tempUnit = unitGen.generateFromTemplate(uTemplate, node, list<string>(), _mqttPart, _enforceTopics, _relaxed);
                    addToUnitCache(tempUnit);
                }
                // Initializing sensors if necessary
                tempUnit->init(_interval, _queueSize);
                compute(tempUnit);
                retrieveAndFlush(outMap, tempUnit);
            } catch(const exception& e) {
                _onDemandLock.store(false);
                throw;
            }
            _onDemandLock.store(false);
        } else if( _keepRunning && !_disabled ) {
            bool found = false;
            if(!_duplicate) {
                for(const auto &u : _units)
                    if(u->getName() == node) {
                        found = true;
                        retrieveAndFlush(outMap, u, false);
                    }
            } else if(_unitID>=0 && node==_units[_unitID]->getName()) {
                found = true;
                retrieveAndFlush(outMap, _units[_unitID], false);
            }

            if(!found)
                throw std::domain_error("Node " + node + " does not belong to the domain of " + _name + "!");
        } else
            throw std::runtime_error("Operator " + _name + ": not available for on-demand query!");
        return outMap;
    }

    /**
    * @brief              Adds an unit to the internal cache of units
    *
    *                     The cache is used to speed up response times to queries of on-demand operators, and reduce
    *                     overall overhead. The cache has a limited size: once this size is reached, at every insertion
    *                     the oldest entry in the cache is removed.
    *
    * @param unit         Shared pointer to the Unit object to be added to the cache
    */
    void addToUnitCache(U_Ptr unit) {
        if(!_unitCache) {
            _unitCache = new map<string, U_Ptr>();
            _insertionLUT = new map<uint64_t, U_Ptr>();
        }

        if(_unitCache->size() >= _unitCacheLimit) {
            auto oldest = _insertionLUT->begin();
            _unitCache->erase(oldest->second->getName());
            _insertionLUT->erase(oldest->first);
        }
        _unitCache->insert(make_pair(unit->getName(), unit));
        // The template unit must never be deleted, even if the cache is full; therefore, we omit its entry from
        // the insertion time LUT, so that it is never picked for deletion
        if(unit->getName() != SensorNavigator::templateKey)
            _insertionLUT->insert(make_pair(getTimestamp(), unit));
    }

    /**
    * @brief              Clears the internal baseUnits vector and only preserves the currently active unit
    *
    *                     This method is used for duplicated operators that share the same unit vector, with different
    *                     unit IDs. In order to expose distinct units to the outside, this method allows to keep
    *                     in the internal baseUnits vector (which is exposed through getUnits) only the unit that is
    *                     assigned to this specific operator, among those of the duplicated group. Access to the
    *                     other units is preserved through the _units vector, that can be accessed from within this
    *                     operator object.
    *
    */
    virtual void collapseUnits() {
        if (_unitID < 0 || _units.empty()) {
            LOG(error) << "Operator " << _name << ": Cannot collapse units!";
            return;
        }
        _baseUnits.clear();
        _baseUnits.push_back(_units[_unitID]);
        // If the unit is hierarchical, we add its subunits as well
        if(_units[_unitID]->isTopUnit())
            for(auto& subUnit : _units[_unitID]->getSubUnits())
                _baseUnits.push_back(subUnit);
    }

protected:
    
    /**
    * @brief              Retrieves output values of a unit and puts them in a map
    *  
    * @param outMap       string to reading_t map in which outputs must be stored
    * @param unit         Unit (flat or hierarchical) to be scanned
    * @param flushQueues  If true, the queues of outbound sensor values will be flushed as well 
    */
    void retrieveAndFlush(map<string, reading_t>& outMap, U_Ptr unit, bool flushQueues=true) {
        // Retrieving top-level outputs
        for (const auto &o : unit->getOutputs()) {
            outMap.insert(make_pair(o->getName(), o->getLatestValue()));
            if(flushQueues)
                o->clearReadingQueue();
        }
        // Retrieving sub-unit outputs (if any)
        for (const auto &subUnit : unit->getSubUnits()) {
            for (const auto &o : subUnit->getOutputs()) {
                outMap.insert(make_pair(o->getName(), o->getLatestValue()));
                if(flushQueues)
                    o->clearReadingQueue();
            }
        }
    }

    /**
    * @brief              Returns the timestamp associated with the next compute task
    *
    *                     If the sync option is enabled, the timestamp will be adjusted to be synchronized with the
    *                     other sensor readings.
    *
    * @return             Timestamp of the next compute task
    */
    uint64_t nextReadingTime() {
        uint64_t now = getTimestamp();
        uint64_t next;
        if (_sync) {
            uint64_t interval64 = static_cast<uint64_t>(_interval);
            uint64_t now_ms = now / 1000 / 1000;
            uint64_t waitToStart = interval64 - (now_ms%interval64); //synchronize all measurements with other sensors
            if(!waitToStart ){ // less than 1 ms seconds is too small, so we wait the entire interval for the next measurement
                return (now_ms + interval64 + 10)*1000*1000;
            }
            return (now_ms + waitToStart + _delayInterval)*1000*1000;
        } else {
            return now + MS_TO_NS(_interval);
        }
    }

    /**
    * @brief              Performs a compute task
    *
    *                     This method is tasked with scheduling the next compute task, and invoking the internal
    *                     compute() method, which encapsulates the real logic of the operator. The compute method
    *                     is automatically called over units as required by the Operator's configuration.
    *
    */
    virtual void computeAsync() override {
        if (_duplicate && _unitID >= 0) {
            try {
                compute(_units[_unitID]);
            } catch(const exception& e) {
                LOG(error) << e.what();
            }
        } else {
            for (unsigned i = 0; i < _units.size(); i++) {
                try {
                    compute(_units[i]);
                } catch (const exception &e) {
                    LOG(error) << e.what();
                    continue;
                }
            }
        }

        if (_timer && _keepRunning && !_disabled) {
            _scheduledTime = nextReadingTime();
            _timer->expires_at(timestamp2ptime(_scheduledTime));
            _pendingTasks++;
            _timer->async_wait(bind(&OperatorTemplate::computeAsync, this));
        }
        _pendingTasks--;
    }

    /**
    * @brief              Data analytics computation logic
    *
    *                     This method contains the actual logic used by the analyzed, and is automatically called by
    *                     the computeAsync method.
    *
    * @param unit         Shared pointer to unit to be processed
    */
    virtual void compute(U_Ptr unit) = 0;

    // Cache for frequently used units in ondemand and job modes
    map<string, U_Ptr>* _unitCache;
    // Helper map to keep track of the cache insertion times
    map<uint64_t, U_Ptr>* _insertionLUT;
    // Vector of pointers to the internal units
    vector<U_Ptr>   _units;
    // Vector of pointers to the internal units, casted to UnitInterface - only efficient way to do this in C++
    // unless we use raw arrays
    vector<UnitPtr> _baseUnits;
    // Instance of a QueryEngine object to get sensor data
    QueryEngine&    _queryEngine;
    // Internal timestamp to keep track of the time at which an operator will be invoked
    uint64_t _scheduledTime;
};

#endif //PROJECT_OPERATORTEMPLATE_H
