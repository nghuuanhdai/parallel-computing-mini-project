//================================================================================
// Name        : SensorGroupTemplateEntity.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Interface template for sensor group functionality with entities.
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

#ifndef DCDBPUSHER_INCLUDES_SENSORGROUPTEMPLATEENTITY_H_
#define DCDBPUSHER_INCLUDES_SENSORGROUPTEMPLATEENTITY_H_

#include "SensorGroupTemplate.h"

#include "EntityInterface.h"

/**
 * @brief Interface template for sensor group implementations with entities.
 *
 * @details This is a derived template of SensorGroupTemplate.
 *          Internal usage of _baseSensors is not synchronized via
 *          acquireSensors()/releaseSensors() as it is only accessed (e.g. in
 *          (de-)constructor) when no other caller is expected to access
 *          _baseSensors.
 *
 * @ingroup pusherplugins
 */
template <class S, class E>
class SensorGroupTemplateEntity : public SensorGroupTemplate<S> {
	//the template shall only be instantiated for classes which derive from EntityInterface
	static_assert(std::is_base_of<EntityInterface, E>::value, "E must derive from EntityInterface!");

      protected:
	//mention all required parent attributes and functions here to avoid compiler errors
	using SensorGroupInterface::_disabled;
	using SensorGroupInterface::_groupName;
	using SensorGroupInterface::_interval;
	using SensorGroupInterface::_queueSize;
	using SensorGroupInterface::_keepRunning;
	using SensorGroupInterface::_pendingTasks;
	using SensorGroupInterface::_timer;
	using SensorGroupInterface::nextReadingTime;
	using SensorGroupTemplate<S>::_sensors;

      public:
	SensorGroupTemplateEntity(const std::string groupName)
	    : SensorGroupTemplate<S>(groupName),
	      _entity(nullptr) {}

	SensorGroupTemplateEntity(const SensorGroupTemplateEntity &other)
	    : SensorGroupTemplate<S>(other),
	      _entity(other._entity) {}

	virtual ~SensorGroupTemplateEntity() {}

	SensorGroupTemplateEntity &operator=(const SensorGroupTemplateEntity &other) {
		SensorGroupTemplate<S>::operator=(other);

		_entity = other._entity;

		return *this;
	}
	bool     isDisabled() const final override { return _disabled || _entity->isDisabled(); };
	
	void     setEntity(E *entity) { _entity = entity; }
	E *const getEntity() const { return _entity; }

	/**
     * @brief Initialize the sensor group.
     *
     * @details This method must not be overwritten. See execOnInit() if custom
     *          actions are required during initialization. Initializes
     *          associated sensors and entity.
     *
     * @param io IO service to initialize the timer with.
     */
	virtual void init(boost::asio::io_service &io) final override {
		if (!_entity) {
			LOG(error) << "No entity set for group " << _groupName << "! Cannot initialize group";
			return;
		}

		SensorGroupInterface::init(io);

		for (auto s : _sensors) {
			s->initSensor(_interval, _queueSize);
		}

		_entity->init(io);

		this->execOnInit();
	}

	/**
     * @brief Start the sensor group (i.e. start collecting data).
     *
     * @details This method must not be overwritten. See execOnStart() if custom
     *          actions are required during startup.
     */
	virtual void start() final override {
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

		if (_entity) {
			if (!_entity->isDisabled()) {
				_keepRunning = true;
				_pendingTasks++;
				_timer->async_wait(_entity->getStrand()->wrap(std::bind(&SensorGroupTemplateEntity::readAsync, this)));
				LOG(info) << "Sensorgroup " << _groupName << " started.";
			}
		} else {
			LOG(error) << "No entity set for group " << _groupName << "! Cannot start polling.";
		}
	}

      protected:
	/**
     * @brief Asynchronous callback if _timer expires.
     *
     * @details Issues a read() and sets the timer again if _keepRunning is true.
     */
	void readAsync() final override {
		this->read();
		if (_timer && _keepRunning && !_disabled && !_entity->isDisabled()) {
			_timer->expires_at(timestamp2ptime(nextReadingTime()));
			_pendingTasks++;
			_timer->async_wait(_entity->getStrand()->wrap(std::bind(&SensorGroupTemplateEntity::readAsync, this)));
		}
		_pendingTasks--;
	}

	LOGGER lg;
	E *_entity; ///< Entity this group is associated to
};

#endif /* DCDBPUSHER_INCLUDES_SENSORGROUPTEMPLATEENTITY_H_ */
