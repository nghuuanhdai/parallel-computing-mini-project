//================================================================================
// Name        : EntityInterface.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Abstract interface defining sensor entity functionality.
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

#ifndef DCDBPUSHER_INCLUDES_ENTITYINTERFACE_H_
#define DCDBPUSHER_INCLUDES_ENTITYINTERFACE_H_

#include <memory>

#include <boost/asio.hpp>

#include "logging.h"

/**
 * @brief Abstract interface defining sensor entity functionality.
 *
 * @ingroup pusherplugins
 */
class EntityInterface {
      public:
	using strand = boost::asio::io_service::strand;

	/**
     * @brief Constructor
     *
     * @details Does not initialize _strand.
     *
     * @param name Name of the entity.
     */
	EntityInterface(const std::string &name)
	    : _name(name),
	      _mqttPart(""),
	      _initialized(false),
	      _disabled(false),
	      _strand(nullptr) {}

	/**
     * @brief Copy constructor
     *
     * @details Does not initialize _strand.
     *
     * @param other Entity to copy construct from.
     */
	EntityInterface(const EntityInterface &other)
	    : _name(other._name),
	      _mqttPart(other._mqttPart),
	      _initialized(false),
	      _disabled(other._disabled),
	      _strand(nullptr) {}

	/**
     * @brief Destructor
     */
	virtual ~EntityInterface() {}

	/**
     * @brief Assignment operator
     *
     * @details _strand is uninitialized afterwards.
     *
     * @param other Entity to assign from.
     *
     * @return EntityInterface
     */
	EntityInterface &operator=(const EntityInterface &other) {
		_name = other._name;
		_mqttPart = other._mqttPart;
		_initialized = false;
		_disabled = other._disabled;
		_strand = nullptr;

		return *this;
	}

	///@name Getters
	///@{
	const std::string &            getName() const { return _name; }
	const std::string &            getMqttPart() const { return _mqttPart; }
	const bool &                   isDisabled() const { return _disabled; }
	const std::unique_ptr<strand> &getStrand() const { return _strand; }
	///@}

	///@name Setters
	///@{
	void setName(const std::string &name) { _name = name; }
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
	void setDisabled(const bool disabled) { _disabled = disabled; }
	///@}

	/**
     * @brief Initialize this entity.
     *
     * @details This method must not be overwritten. See execOnInit() if custom
     *          actions are required during initialization. Initializes
     *          associated base class and subsequently calls execOnInit().
     *
     * @param io IO service to initialize _strand with.
     */
	void init(boost::asio::io_service &io) {
		if (_initialized) {
			return;
		}
		_strand.reset(new strand(io));
		this->execOnInit();
		_initialized = true;
	}

	/**
     * @brief Print configuration of this entity.
     *
     * @details Prints configuration of base class and subsequently calls
     *          printEntityConfig().
     *
     * @param ll Log severity level to be used from logger.
     */
	void printConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << _name;
		LOG_VAR(ll) << leading << "    Disabled:     " << (_disabled ? std::string("true") : std::string("false"));
		if (_mqttPart != "") {
			LOG_VAR(ll) << leading << "    MQTT part:    " << _mqttPart;
		}
		this->printEntityConfig(ll, leadingSpaces + 4);
	}

      protected:
	/**
     * @brief Implement plugin specific actions to initialize an entity here.
     *
     * @details If a derived class (i.e. a plugin entity) requires further custom
     *          actions for initialization, this should be implemented here.
     *          %execOnInit() is appropriately called by this template during
     *          init().
     */
	virtual void execOnInit() { /* do nothing if not overwritten */
	}

	/**
     * @brief Print configuration of derived class.
     *
     * @param ll Log severity level to be used from logger.
     */
	virtual void printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) { /* do nothing if not overwritten */
	}

	std::string _name;     ///< Name of the entity
	std::string _mqttPart; ///< Partial MQTT topic identifying this entity

	bool _initialized; ///< An entity should only be initialized once
	bool _disabled;

	std::unique_ptr<strand> _strand; ///< Provides serialized handler execution to avoid race conditions
	LOGGER                  lg;      ///< Logging instance
};

#endif /* DCDBPUSHER_INCLUDES_ENTITYINTERFACE_H_ */
