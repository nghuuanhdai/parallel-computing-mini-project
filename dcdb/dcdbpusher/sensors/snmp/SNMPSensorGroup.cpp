//================================================================================
// Name        : SNMPSensorGroup.cpp
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for SNMP sensor group class.
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

#include "SNMPSensorGroup.h"

SNMPSensorGroup::SNMPSensorGroup(const std::string &name)
    : SensorGroupTemplateEntity(name) {
}

SNMPSensorGroup::SNMPSensorGroup(const SNMPSensorGroup &other)
    : SensorGroupTemplateEntity(other) {
}

SNMPSensorGroup::~SNMPSensorGroup() {}

SNMPSensorGroup &SNMPSensorGroup::operator=(const SNMPSensorGroup &other) {
	SensorGroupTemplate::operator=(other);

	return *this;
}

void SNMPSensorGroup::execOnInit() {
	for (auto s : _sensors) {
		s->setOID(_entity->getOIDPrefix() + s->getOIDSuffix());
	}
}

void SNMPSensorGroup::read() {
	reading_t reading;
	reading.timestamp = getTimestamp();

	if (_entity->open()) {
		for (const auto &s : _sensors) {
			try {
				reading.value = _entity->get(s->getOID(), s->getOIDLen());
#ifdef DEBUG
				LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
				s->storeReading(reading);
			} catch (const std::exception &e) {
				LOG(error) << _groupName << "::" << s->getName() << " could not read value: " << e.what();
				continue;
			}
		}
		_entity->close();
	}
}
