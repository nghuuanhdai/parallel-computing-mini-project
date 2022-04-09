//================================================================================
// Name        : OperatorInterface.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Interface to data operators.
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

/**
 * @defgroup operator Operator Plugins
 * @ingroup analytics
 *
 * @brief Operator for the analytics plugin.
 */
#ifndef PROJECT_OPERATORINTERFACE_H
#define PROJECT_OPERATORINTERFACE_H

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>
#include <map>
#include <boost/asio.hpp>

#include "logging.h"
#include "UnitInterface.h"

using namespace std;

// Struct defining a response to a REST request
typedef struct {
    string response;
    string data;
} restResponse_t;

/**
 * @brief Interface to data operators
 *
 * @details This interface supplies methods to instantiate, start and retrieve
 *          data from operators, which are "modules" that* implement data
 *          analytics models and are loaded by DCDB data analytics plugins. For
 *          it to be used in the DCDB data analytics framework, the operator
 *          must comply to this interface.
 *
 *          An operator acts on "units", which are logical entities represented
 *          by certain inputs and outputs. An unit can be, for example, a node,
 *          a CPU or a rack in a HPC system.
 *
 * @ingroup operator
 */
class OperatorInterface {
public:

    /**
    * @brief            Class constructor
    *
    * @param name       Name of the operator
    */
    OperatorInterface(const string& name) :
            _name(name),
            _mqttPart(""),
            _isTemplate(false),
            _relaxed(false),
            _enforceTopics(false),
            _duplicate(false),
            _streaming(true),
            _sync(true),
            _dynamic(false),
            _disabled(false),
            _unitID(-1),
            _keepRunning(0),
            _minValues(1),
            _interval(1000),
            _queueSize(1024),
            _cacheInterval(900000),
            _unitCacheLimit(1000),
            _cacheSize(1),
            _delayInterval(10),
            _pendingTasks(0),
            _onDemandLock(false),
            _timer(nullptr) {}

    /**
    * @brief            Copy constructor
    */
    OperatorInterface(const OperatorInterface& other) :
            _name(other._name),
            _mqttPart(other._mqttPart),
            _isTemplate(other._isTemplate),
            _relaxed(other._relaxed),
            _enforceTopics(other._enforceTopics),
            _duplicate(other._duplicate),
            _streaming(other._streaming),
            _sync(other._sync),
            _dynamic(other._dynamic),
            _disabled(other._disabled),
            _unitID(other._unitID),
            _keepRunning(other._keepRunning),
            _minValues(other._minValues),
            _interval(other._interval),
            _queueSize(other._queueSize),
            _cacheInterval(other._cacheInterval),
            _unitCacheLimit(other._unitCacheLimit),
            _cacheSize(other._cacheSize),
            _delayInterval(other._delayInterval),
            _pendingTasks(0),
            _onDemandLock(false),
            _timer(nullptr) {}

    /**
    * @brief            Class destructor
    */
    virtual ~OperatorInterface() {}

    /**
    * @brief            Assignment operator
    */
    OperatorInterface& operator=(const OperatorInterface& other) {
        _name = other._name;
        _mqttPart = other._mqttPart;
        _isTemplate = other._isTemplate;
        _relaxed = other._relaxed;
        _enforceTopics = other._enforceTopics;
        _unitID = other._unitID;
        _duplicate = other._duplicate;
        _streaming = other._streaming;
        _sync = other._sync;
        _dynamic = other._dynamic;
        _disabled = other._disabled;
        _keepRunning = other._keepRunning;
        _minValues = other._minValues;
        _interval = other._interval;
        _cacheInterval = other._cacheInterval;
        _queueSize = other._queueSize;
        _unitCacheLimit = other._unitCacheLimit;
        _cacheSize = other._cacheSize;
        _delayInterval = other._delayInterval;
        _pendingTasks.store(0);
        _onDemandLock.store(false);
        _timer = nullptr;

        return *this;
    }
    
    /**
    * @brief              Initializes this operator
    *
    *                     This method initializes the timer used to schedule tasks. It can be overridden by derived
    *                     classes.
    *
    * @param io           Boost ASIO service to be used
    */
    virtual void init(boost::asio::io_service& io) {
        _cacheSize = _cacheInterval / _interval + 1;
        _timer.reset(new boost::asio::deadline_timer(io, boost::posix_time::seconds(0)));
    }

    /**
    * @brief              Perform a REST-triggered PUT action
    *
    *                     This method must be implemented in derived classes. It will perform an action (if any)
    *                     on the operator according to the input action string. Any thrown
    *                     exceptions will be reported in the response string.
    *
    * @param action       Name of the action to be performed
    * @param queries      Vector of queries (key-value pairs)
    *
    * @return             Response to the request as a <response, data> pair
    */
    virtual restResponse_t REST(const string& action, const unordered_map<string, string>& queries) = 0;

    /**
    * @brief              Waits for the operator to complete its tasks
    *
    *                     This method must be implemented in derived classes.
    */
    virtual void wait() = 0;
    
    /**
    * @brief              Starts this operator
    *
    *                     This method must be implemented in derived classes. It will start the operation of the
    *                     operator.
    */
    virtual void start() = 0;

    /**
    * @brief              Stops this operator
    *
    *                     This method must be implemented in derived classes. It will stop the operation of the
    *                     operator.
    */
    virtual void stop() = 0;

    /**
    * @brief              Adds an unit to this operator
    *
    *                     This method must be implemented in derived classes. It must add the input UnitInterface to
    *                     the internal structure storing units in the operator. Said unit must then be used during
    *                     computation.
    *
    * @param u            Shared pointer to a UnitInterface object
    */
    virtual void addUnit(UnitPtr u)             = 0;

    /**
    * @brief              Clears all the units contained in this operator
    *
    *                     This method must be implemented in derived classes.
    */
    virtual void clearUnits()                   = 0;

	/**
	 * @brief              Returns the number of messages this operator will
	 *                     generate per second
	 *
	 *                     This method must be implemented in derived classes.
	 */
	virtual float getMsgRate()                  = 0;

	/**
    * @brief              Performs an on-demand compute task
    *
    *                     Unlike the protected computeAsync and compute methods, computeOnDemand allows to interactively
    *                     perform data analytics queries on the operator, which must have the _streaming attribute set
    *                     to false. A unit is generated on the fly, corresponding to the input node given as input,
    *                     and results are returned in the form of a map.
    *
    * @param node         Sensor tree node that defines the query
    * @return             a map<string, reading_t> containing the output of the query
    */
    virtual map<string, reading_t> computeOnDemand(const string& node="") = 0;

    /**
    * @brief            Prints the current operator configuration
    *
    * @param ll         Logging level at which the configuration is printed
    */
    virtual void printConfig(LOG_LEVEL ll) = 0;

    // Getter methods
    const string& 	    getName()	        const { return _name; }
    const string& 	    getMqttPart()	    const { return _mqttPart; }
    bool 	            getTemplate()	    const { return _isTemplate; }
    bool                getRelaxed()        const { return _relaxed; }
    bool                getEnforceTopics()  const { return _enforceTopics; }
    bool				getSync()		    const { return _sync; }
    bool				getDuplicate()	    const { return _duplicate; }
    bool				getStreaming()	    const { return _streaming; }
    unsigned			getMinValues()	    const { return _minValues; }
    unsigned			getInterval()	    const { return _interval; }
    unsigned			getQueueSize()	    const { return _queueSize; }
    unsigned			getCacheSize()	    const { return _cacheSize; }
    unsigned            getUnitCacheLimit() const { return _unitCacheLimit; }
    unsigned            getDelayInterval()  const { return _delayInterval; }
    int                 getUnitID()         const { return _unitID; }
    bool                getDynamic()        const { return _dynamic; }
    bool                getDisabled()       const { return _disabled; }

    // Setter methods
    void setName(const string& name)	            { _name = name; }
    void setMqttPart(const string& mqttPart)	    { _mqttPart = mqttPart; }
    void setTemplate(bool t)                        { _isTemplate = t; }
    void setRelaxed(bool r)                         { _relaxed = r; }
    void setEnforceTopics(bool e)                   { _enforceTopics = e; }
    void setSync(bool sync)							{ _sync = sync; }
    void setUnitID(int u)                           { _unitID = u; }
    void setStreaming(bool streaming)				{ _streaming = streaming; }
    void setDuplicate(bool duplicate)				{ _duplicate = duplicate; }
    void setDisabled(bool disabled)                 { _disabled = disabled; }
    void setMinValues(unsigned minValues)			{ _minValues = minValues; }
    void setInterval(unsigned interval)				{ _interval = interval; }
    void setQueueSize(unsigned queueSize)			{ _queueSize = queueSize; }
    void setUnitCacheLimit(unsigned uc)             { _unitCacheLimit = uc+1; }
    void setCacheInterval(unsigned cacheInterval)	{ _cacheInterval = cacheInterval; }
    void setDelayInterval(unsigned delayInterval)	{ _delayInterval = delayInterval; }
    virtual vector<UnitPtr>& getUnits()             = 0;
    virtual void    releaseUnits()                  = 0;

protected:

    /**
     * @brief             Implement plugin specific actions to initialize an operator here.
     *
     * @details           If a derived class requires further custom
     *                    actions for initialization, this should be implemented here.
     */
    virtual void execOnInit() {}

    /**
     * @brief             Implement plugin-specific actions to start an operator here.
     *
     * @details           If a derived class (i.e. a plugin group) requires further custom
     *                    actions to start analytics, this should be implemented here.
     *
     * @return            True on success, false otherwise.
     */
    virtual bool execOnStart() { return true; }

    /**
     * @brief             Implement plugin specific actions to stop a group here.
     *
     * @details           If a derived class requires further custom actions 
     *                    to stop operation this should be implemented here.
     */
    virtual void execOnStop() {}

    /**
    * @brief              Performs a compute task
    *
    * @details            This method is tasked with scheduling the next compute task, and invoking the internal
    *                     compute() method, which encapsulates the real logic of the operator. The compute method
    *                     is automatically called over units as required by the Operator's configuration.
    *
    */
    virtual void computeAsync() = 0;
    
    // Name of this operator
    string _name;
    // MQTT part (see docs) of this operator
    string	_mqttPart;

    // To distinguish between templates and actual operators
    bool _isTemplate;
    // If the operator's units must be built in relaxed mode
    bool _relaxed;
    // If true, when building the units of this operator all output sensors will have _mqttPart prepended to them
    bool _enforceTopics;
    // If true, the operator is a duplicate of another
    bool _duplicate;
    // If true, the operator performs computation in streaming
    bool _streaming;
    // If true, the computation intervals are synchronized
    bool _sync;
    // Indicates whether the operator generates units dynamically at runtime, or only at initialization
    bool _dynamic;
    // If true, the operator is initialized but "disabled" and cannot be used
    bool _disabled;
    // ID of the units this operator works on
    int _unitID;
    // Determines if the operator can keep running or must terminate
    int _keepRunning;
    // Minimum number of sensor values to be accumulated before output can be sent
    unsigned int _minValues;
    // Sampling period regulating compute batches
    unsigned int _interval;
	// readingQueue size
    unsigned int _queueSize;
    // Size of the cache in time for the output sensors in this operator
    unsigned int _cacheInterval;
    // Maximum number of units that can be contained in the unit cache
    unsigned int _unitCacheLimit;
    // Real size of the cache, as determined from cacheInterval
    unsigned int _cacheSize;
    // Time in seconds to wait for before starting computation
    unsigned int _delayInterval;
    // Number of pending ASIO tasks
    atomic_uint _pendingTasks;
    // Lock used to serialize access to the ondemand functionality
    atomic_bool _onDemandLock;
    // Timer for scheduling tasks
    unique_ptr<boost::asio::deadline_timer> _timer;

    // Logger object
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
};

//for better readability
using OperatorPtr = shared_ptr<OperatorInterface>;

#endif //PROJECT_OPERATORINTERFACE_H
