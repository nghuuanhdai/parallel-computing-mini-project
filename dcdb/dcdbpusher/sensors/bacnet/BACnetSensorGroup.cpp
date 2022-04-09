//================================================================================
// Name        : BACnetSensorGroup.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for BACnet sensor group class.
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

#include "BACnetSensorGroup.h"

#include <functional>

BACnetSensorGroup::BACnetSensorGroup(const std::string &name)
    : SensorGroupTemplateEntity(name), _deviceInstance(0) {
}

BACnetSensorGroup::BACnetSensorGroup(const BACnetSensorGroup &other)
    : SensorGroupTemplateEntity(other),
      _deviceInstance(other._deviceInstance) {}

BACnetSensorGroup::~BACnetSensorGroup() {}

BACnetSensorGroup &BACnetSensorGroup::operator=(const BACnetSensorGroup &other) {
	SensorGroupTemplate::operator=(other);
	_deviceInstance = other._deviceInstance;

	return *this;
}

void BACnetSensorGroup::read() {
	reading_t reading;
	reading.timestamp = getTimestamp();

	for (const auto &s : _sensors) {
		try {
			reading.value = _entity->readProperty(getDeviceInstance(), s->getObjectInstance(), s->getObjectType(), s->getPropertyId());
#ifdef DEBUG
			LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
			s->storeReading(reading);
		} catch (const std::exception &e) {
			LOG(error) << _groupName << "::" << s->getName() << " could not read value: " << e.what();
			continue;
		}
	}
}

void BACnetSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "deviceInstance: " << _deviceInstance;
}
