//================================================================================
// Name        : SensorGroupInterface.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Abstract interface defining sensor group functionality.
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
 * @defgroup pusherplugins Pusher Plugins
 * @ingroup  pusher
 *
 * @brief Collection of plugin interfaces, plugin templates summarizing common
 *        logic and the plugins itself.
 */

#ifndef SENSORGROUPINTERFACE_H_
#define SENSORGROUPINTERFACE_H_

#include <atomic>
#include <boost/asio.hpp>
#include <memory>

#include "logging.h"
#include "sensorbase.h"

/**
 * @brief Abstract interface defining sensor group functionality.
 *
 * @details Sensor groups should not implement this interface themselves but
 *          inherit the SensorGroupTemplate instead.
 *
 * @ingroup pusherplugins
 */
class SensorGroupInterface {
      public:
	SensorGroupInterface(const std::string &groupName)
	    : _groupName(groupName),
	      _mqttPart(""),
	      _sync(true),
	      _keepRunning(false),
	      _disabled(false),
	      _minValues(1),
	      _interval(1000),
	      _queueSize(1024),
	      _pendingTasks(0),
	      _timer(nullptr) {
	}

	SensorGroupInterface(const SensorGroupInterface &other)
	    : _groupName(other._groupName),
	      _mqttPart(other._mqttPart),
	      _sync(other._sync),
	      _keepRunning(false),
	      _disabled(other._disabled),
	      _minValues(other._minValues),
	      _interval(other._interval),
	      _queueSize(other._queueSize),
	      _pendingTasks(0),
	      _timer(nullptr) {
	}

	virtual ~SensorGroupInterface() {}

	SensorGroupInterface &operator=(const SensorGroupInterface &other) {
		_groupName = other._groupName;
		_mqttPart = other._mqttPart;
		_sync = other._sync;
		_keepRunning = false;
		_disabled = other._disabled;
		_minValues = other._minValues;
		_interval = other._interval;
		_queueSize = other._queueSize;
		_pendingTasks.store(0);
		_timer = nullptr;

		return *this;
	}

	///@name Getters
	///@{
	const std::string &getGroupName() const { return _groupName; }
	const std::string &getMqttPart() const { return _mqttPart; }
	bool               getSync() const { return _sync; }
	virtual bool       isDisabled() const { return _disabled; }
	unsigned           getMinValues() const { return _minValues; }
	unsigned           getInterval() const { return _interval; }
	unsigned           getQueueSize() const { return _queueSize; }
	///@}

	///@name Setters
	///@{
	void setGroupName(const std::string &groupName) { _groupName = groupName; }
	void setMqttPart(const std::string &mqttPart) {
		_mqttPart = mqttPart;
		//sanitize mqttPart into uniform /xxxx format
		if (_mqttPart.front() != '/') {
			_mqttPart.insert(0, "/");
		}
		if (_mqttPart.back() == '/') {
			_mqttPart.erase(_mqttPart.size() - 1);
		}
	}
	void setSync(bool sync) { _sync = sync; }
	void setDisabled(const bool disabled) { _disabled = disabled; }
	void setMinValues(unsigned minValues) { _minValues = minValues; }
	void setInterval(unsigned interval) { _interval = interval; }
	void setQueueSize(unsigned queueSize) { _queueSize = queueSize; }
	///@}

	///@name Publicly accessible interface methods (implemented in SensorGroupTemplate)
	///@{
	/**
     * @brief Initialize the sensor group.
     *
     * @details Derived classes overwriting this method must ensure to either
     *          call this method or reproduce its functionality.
     *
     * @param io IO service to initialize the timer with.
     */
	virtual void init(boost::asio::io_service &io) {
		_timer.reset(new boost::asio::deadline_timer(io, boost::posix_time::seconds(0)));
	}

	/**
     * @brief Waits for the termination of the sensor group.
     */
	virtual void wait() = 0;

	/**
     * @brief Start the sensor group (i.e. start collecting data).
     */
	virtual void start() = 0;

	/**
     * @brief Stop the sensor group (i.e. stop collecting data).
     * 
     * @details Must be followed by a call to the wait() method.
     */
	virtual void stop() = 0;

	/**
     * @brief Add a sensor to this group.
     *
     * @param s Shared pointer to the sensor.
     */
	virtual void pushBackSensor(SBasePtr s) = 0;

	/**
     * @brief Acquire access to all sensors of this group.
     *
     * @details Always release sensor access via releaseSensors() afterwards to
     *          ensure multi-threading safety!
     *          acquireSensors() and releaseSensors() allow for thread-safe
     *          (locked) access to _baseSensors. However, as most plugins do not
     *          require synchronized access (and we want to avoid the induced
     *          lock overhead for them) the actual locking mechanism is only
     *          implemented in plugins actually requiring thread-safe access.
     *          Every caller which may require thread safe access to
     *          _baseSensors should use acquireSensors() and releaseSensors().
     *          If a plugin does not implement a locking mechanism because it
     *          does not require synchronized access releaseSensors() can be
     *          omitted.
     *          See the Caliper plugin for a SensorGroup which implements locked
     *          access to _baseSensors.
     *
     * @return A std::vector with shared pointers to all associated sensors.
     */
	virtual std::vector<SBasePtr> &acquireSensors() { return _baseSensors; }

	/**
     * @brief Release previously acquired access to sensors of this group.
     *
     * @details Always acquire sensors via acquireSensors() beforehand!
     *          See description of acquireSensors() for an in-depth reasoning
     *          about this method.
     */
	virtual void releaseSensors() { /* do nothing in base implementation */
	}

	/**
	 * @brief Compute message rate of this group.
	 *
	 * @details Computes the message rate based on number of message, interval
	 *          and minValues
	 *
	 * @return The message rate in messages/s.
	 */
	virtual float getMsgRate() {
		float val = 0;
		for (const auto& s: _baseSensors) {
			if(s->getSubsampling() > 0)
				val+= 1.0f / (float)s->getSubsampling();
		}
		return val * (1000.0f / (float)_interval) / (float)_minValues;
	}
 
	/**
     * @brief Print interface configuration.
     *
     * @details Derived classes overwriting this method must ensure to either
     *          call this method or reproduce its functionality.
     *
     * @param ll Log severity level to be used from logger.
     */
	virtual void printConfig(LOG_LEVEL ll, unsigned leadingSpaces = 8) {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << _groupName;
		LOG_VAR(ll) << leading << "    Disabled:     " << (_disabled ? std::string("true") : std::string("false"));
		if (_mqttPart != "") {
			LOG_VAR(ll) << leading << "    MQTT part:    " << _mqttPart;
		}

		LOG_VAR(ll) << leading << "    Synchronized: " << (_sync ? std::string("true") : std::string("false"));
		LOG_VAR(ll) << leading << "    minValues:    " << _minValues;
		LOG_VAR(ll) << leading << "    interval:     " << _interval;
		LOG_VAR(ll) << leading << "    queueSize:    " << _queueSize;
	}
	///@}

      protected:
	///@name Internal methods (implemented in a plugin's SensorGroup)
	///@{
	/**
     * @brief Read data for all sensors once.
     */
	virtual void read() = 0;

	/**
     * @brief Implement plugin specific actions to initialize a group here.
     *
     * @details If a derived class (i.e. a plugin group) requires further custom
     *          actions for initialization, this should be implemented here.
     *          %initGroup() is appropriately called during init().
     */
	virtual void execOnInit() { /* do nothing if not overwritten */
	}

	/**
     * @brief Implement plugin specific actions to start a group here.
     *
     * @details If a derived class (i.e. a plugin group) requires further custom
     *          actions to start polling data (e.g. open a file descriptor),
     *          this should be implemented here. %startGroup() is appropriately
     *          called during start().
     *
     * @return True on success, false otherwise.
     */
	virtual bool execOnStart() { return true; }

	/**
     * @brief Implement plugin specific actions to stop a group here.
     *
     * @details If a derived class (i.e. a plugin group) requires further custom
     *          actions to stop polling data (e.g. close a file descriptor),
     *          this should be implemented here. %stopGroup() is appropriately
     *          called during stop().
     */
	virtual void execOnStop() { /* do nothing if not overwritten */
	}

	/**
     * @brief Print information about plugin specific group attributes (or
     *        nothing if no such attributes are present).
     *
     * @details Only overwrite if necessary.
     *
     * @param ll Severity level to log with
     */
	virtual void printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	}
	///@}

	///@name Utility methods
	///@{
	/**
     * @brief Calculate timestamp for the next reading.
     *
     * @return Timestamp in the future to wait for
     */
	virtual uint64_t nextReadingTime() {
		uint64_t now = getTimestamp();
		uint64_t next;
		if (_sync) {
			uint64_t interval64 = static_cast<uint64_t>(_interval);
			uint64_t now_ms = now / 1000 / 1000;
			//synchronize all measurements with other sensors
			uint64_t waitToStart = interval64 - (now_ms % interval64);
			//less than 1 ms seconds is too small, so we wait the entire interval for the next measurement
			if (!waitToStart) {
				return (now_ms + interval64) * 1000 * 1000;
			}
			return (now_ms + waitToStart) * 1000 * 1000;
		} else {
			return now + MS_TO_NS(_interval);
		}
	}
	///@}

	//TODO unify naming scheme: _groupName --> _name (also refactor getter+setter)
	std::string                                  _groupName;   ///< String name of this group
	std::string                                  _mqttPart;    ///< MQTT part identifying this group
	bool                                         _sync;        ///< Should the timer (i.e. the read cycle of this groups) be synchronized with other groups?
	bool                                         _keepRunning; ///< Continue with next reading cycle (i.e. set timer again after reading)?
	bool                                         _disabled;
	unsigned int                                 _minValues;    ///< Minimum number of values a sensor should gather before they get pushed (to reduce MQTT overhead)
	unsigned int                                 _interval;     ///< Reading interval cycle in milliseconds
	unsigned int                                 _queueSize;    ///< Maximum number of queued readings
	std::atomic_uint                             _pendingTasks; ///< Number of currently outstanding read operations
	std::unique_ptr<boost::asio::deadline_timer> _timer;        ///< Time readings in a periodic interval
	std::vector<SBasePtr>                        _baseSensors;  ///< Vector with sensors associated to this group
	LOGGER                                       lg;            ///< Personal logging instance
};

//for better readability
using SGroupPtr = std::shared_ptr<SensorGroupInterface>;

#endif /* SENSORGROUPINTERFACE_H_ */
