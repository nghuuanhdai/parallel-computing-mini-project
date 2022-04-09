//================================================================================
// Name        : BACnetSensorBase.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for BACnet plugin.
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
 * @defgroup bacnet BACnet plugin
 * @ingroup  pusherplugins
 *
 * @brief Collect data from devices via the BACnet protocol.
 */

#ifndef SRC_SENSORS_BACNET_BACNETSENSORBASE_H_
#define SRC_SENSORS_BACNET_BACNETSENSORBASE_H_

#include "sensorbase.h"

#include "BACnetClient.h"

#include "bacnet/bacenum.h"

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup bacnet
 */
class BACnetSensorBase : public SensorBase {
      public:
	BACnetSensorBase(const std::string &name)
	    : SensorBase(name),
	      _objectInstance(0) {
		_objectType = OBJECT_DEVICE;
		_propertyId = PROP_PRESENT_VALUE;
		_objectIndex = BACNET_ARRAY_ALL;
	}

	BACnetSensorBase(const BACnetSensorBase &other)
	    : SensorBase(other),
	      _objectInstance(other._objectInstance),
	      _objectType(other._objectType),
	      _propertyId(other._propertyId),
	      _objectIndex(other._objectIndex) {}

	virtual ~BACnetSensorBase() {}

	BACnetSensorBase &operator=(const BACnetSensorBase &other) {
		SensorBase::operator=(other);
		_objectInstance = other._objectInstance;
		_objectType = other._objectType;
		_propertyId = other._propertyId;
		_objectIndex = other._objectIndex;

		return *this;
	}

	uint32_t           getObjectInstance() const { return _objectInstance; }
	BACNET_OBJECT_TYPE getObjectType() const { return _objectType; }
	BACNET_PROPERTY_ID getPropertyId() const { return _propertyId; }
	int32_t            getObjectIndex() const { return _objectIndex; }

	void setObjectInstance(const std::string &objectInstance) { _objectInstance = stoul(objectInstance); }
	void setObjectType(const std::string &objectType) { _objectType = static_cast<BACNET_OBJECT_TYPE>(stoul(objectType)); }
	void setPropertyId(const std::string &property) { _propertyId = static_cast<BACNET_PROPERTY_ID>(stoul(property)); }
	void setObjectIndex(int32_t objectIndex) { _objectIndex = objectIndex; }

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) override {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << "    objectInstance: " << _objectInstance;
		LOG_VAR(ll) << leading << "    objectType:     " << _objectType;
		LOG_VAR(ll) << leading << "    propertyId:     " << _propertyId;
		LOG_VAR(ll) << leading << "    objectIndex:    " << _objectIndex;
	}

      protected:
	uint32_t           _objectInstance;
	BACNET_OBJECT_TYPE _objectType;
	BACNET_PROPERTY_ID _propertyId;
	int32_t            _objectIndex;
};

#endif /* SRC_SENSORS_BACNET_BACNETSENSORBASE_H_ */
