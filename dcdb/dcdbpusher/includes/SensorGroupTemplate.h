//================================================================================
// Name        : SensorGroupTemplate.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Interface template for sensor group functionality.
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

#ifndef SENSORGROUPTEMPLATE_H_
#define SENSORGROUPTEMPLATE_H_

#include "SensorGroupInterface.h"

#include "timestamp.h"

#include <memory>
#include <vector>

/**
 * @brief Interface template for sensor group implementations without entities.
 *
 * @details There is a derived template for groups with entities, see
 *          SensorGroupTemplateEntity.
 *          Internal usage of _baseSensors is not synchronized via
 *          acquireSensors()/releaseSensors() as it is only accessed (e.g. in
 *          (de-)constructor) when no other caller is expected to access
 *          _baseSensors.
 *
 * @ingroup pusherplugins
 */
template <class S>
class SensorGroupTemplate : public SensorGroupInterface {
	//the template shall only be instantiated for classes which derive from SensorBase
	static_assert(std::is_base_of<SensorBase, S>::value, "S must derive from SensorBase!");

      protected:
	using S_Ptr = std::shared_ptr<S>;

      public:
	SensorGroupTemplate(const std::string groupName)
	    : SensorGroupInterface(groupName) {}

	SensorGroupTemplate(const SensorGroupTemplate &other)
	    : SensorGroupInterface(other) {

		for (auto s : other._sensors) {
			S_Ptr sensor = std::make_shared<S>(*s);
			_sensors.push_back(sensor);
			_baseSensors.push_back(sensor);
		}
	}

	virtual ~SensorGroupTemplate() {
		if (_keepRunning) {
			stop();
		}

		_sensors.clear();
		_baseSensors.clear();
	}

	SensorGroupTemplate &operator=(const SensorGroupTemplate &other) {
		SensorGroupInterface::operator=(other);
		_sensors.clear();
		_baseSensors.clear();

		for (auto s : other._sensors) {
			S_Ptr sensor = std::make_shared<S>(*s);
			_sensors.push_back(sensor);
			_baseSensors.push_back(sensor);
		}

		return *this;
	}

	/**
     * @brief Initialize the sensor group.
     *
     * @details This method must not be overwritten by plugins. See execOnInit()
     *          if custom actions are required during initialization.
     *          Initializes associated sensors.
     *
     * @param io IO service to initialize the timer with.
     */
	virtual void init(boost::asio::io_service &io) override {
		SensorGroupInterface::init(io);

		for (auto s : _sensors) {
			s->initSensor(_interval, _queueSize);
		}

		this->execOnInit();
	}

	/**
     * @brief Does a busy wait until all dispatched handlers are finished
     *        (_pendingTasks == 0).
     *
     * @details If the wait takes longer than a reasonable amount of time we
     *          return anyway, to not block termination of dcdbpusher.
     */
	virtual void wait() final override {
		uint64_t sleepms = 10, i = 0;
		uint64_t timeout = _interval < 10000 ? 30000 : _interval * 3;

		while (sleepms * i++ < timeout) {
			if (_pendingTasks)
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepms));
			else {
				this->execOnStop();
				LOG(info) << "Sensorgroup " << _groupName << " stopped.";
				return;
			}
		}

		LOG(warning) << "Group " << _groupName << " will not finish! Skipping it";
	}

	/**
     * @brief Start the sensor group (i.e. start collecting data).
     *
     * @details This method must not be overwritten by plugins. See
     *          execOnStart() if custom actions are required during startup.
     */
	virtual void start() override {
		if (_disabled) {
			return;
		}

		if (_keepRunning) {
			//we have been started already
			LOG(debug) << "Sensorgroup " << _groupName << " already running.";
			return;
		}

		if (!this->execOnStart()) {
			LOG(error) << "Sensorgroup " << _groupName << ": Startup failed.";
			return;
		}

		_keepRunning = true;
		_pendingTasks++;
		_timer->async_wait(std::bind(&SensorGroupTemplate::readAsync, this));
		LOG(info) << "Sensorgroup " << _groupName << " started.";
	}

	/**
     * @brief Stop the sensor group (i.e. stop collecting data).
     *
     * @details This method must not be overwritten. See execOnStop() if custom
     *          actions are required during shutdown.
     */
	virtual void stop() final override {
		if (!_keepRunning) {
			LOG(debug) << "Sensorgroup " << _groupName << " already stopped.";
			return;
		}

		_keepRunning = false;
		//cancel any outstanding readAsync()
		_timer->cancel();
	}

	/**
     * @brief Add a sensor to this group.
     *
     * @details Not intended to be overwritten. If a sensor with identical name
     *          is already present, it will be replaced.
     *
     * @param s Shared pointer to the sensor.
     */
	virtual void pushBackSensor(SBasePtr s) final override {
		//check if dynamic cast returns nullptr
		if (S_Ptr dSensor = std::dynamic_pointer_cast<S>(s)) {
			_sensors.push_back(dSensor);
			_baseSensors.push_back(s);
		} else {
			LOG(warning) << "Group " << _groupName << ": Type mismatch when storing sensor! Sensor omitted";
		}
	}

	/**
	 * @brief Get access to all sensors of this group.
	 *
	 * @details Different from acquireSensors(): returns vector with plugin
	 *          specific sensor pointers instead of general SensorBase pointers.
	 *          Does not allow for thread-safe access. Only intended for
	 *          usage by Configurators.
	 *
	 * @return Vector with smart pointers of all sensors associated with this
	 *         group.
	 */
	std::vector<S_Ptr> &getDerivedSensors() { return _sensors; }

	/**
     * @brief Print SensorGroup configuration.
     *
     * @details Always call %SensorGroupTemplate::printConfig() to print
     *          complete configuration of a sensor group. This method takes
     *          care of calling SensorGroupInterface::printConfig() and
     *          derived %printConfig() methods in correct order.
     *
     * @param ll Log severity level to be used from logger.
     */
	virtual void printConfig(LOG_LEVEL ll, unsigned leadingSpaces = 8) final override {
		//print common base attributes
		SensorGroupInterface::printConfig(ll, leadingSpaces);

		//print plugin specific group attributes
		this->printGroupConfig(ll, leadingSpaces + 4);

		//print associated sensors
		std::string leading(leadingSpaces + 4, ' ');
		LOG_VAR(ll) << leading << "Sensors:";
		for (auto s : _sensors) {
			s->SensorBase::printConfig(ll, lg, leadingSpaces + 8);
			s->printConfig(ll, lg, leadingSpaces + 8);
		}
	}

      protected:
	/**
     * @brief Asynchronous callback if _timer expires.
     *
     * @details Issues a read() and sets the timer again if _keepRunning is true.
     */
	virtual void readAsync() {
		this->read();
		if (_timer && _keepRunning && !_disabled) {
			_timer->expires_at(timestamp2ptime(nextReadingTime()));
			_pendingTasks++;
			_timer->async_wait(std::bind(&SensorGroupTemplate::readAsync, this));
		}
		_pendingTasks--;
	}

	std::vector<S_Ptr> _sensors; ///< Store pointers to actual sensor objects in addition to general sensor base pointers
};

#endif /* SENSORGROUPTEMPLATE_H_ */
