//================================================================================
// Name        : TesterSensorGroup.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Tester sensor group class.
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

#include "TesterSensorGroup.h"
#include "timestamp.h"
#include <cstring>
#include <functional>

using namespace std;

TesterSensorGroup::TesterSensorGroup(const std::string &name)
    : SensorGroupTemplate(name),
      _value(0),
      _numSensors(0) {}

// Overriding assignment operator so that sensors are not copy-constructed
TesterSensorGroup &TesterSensorGroup::operator=(const TesterSensorGroup &other) {
	SensorGroupInterface::operator=(other);
	_sensors.clear();
	_baseSensors.clear();

	_numSensors = other._numSensors;
	_value = other._value;

	return *this;
}

TesterSensorGroup::~TesterSensorGroup() {}

void TesterSensorGroup::read() {

	// We don't care too much if _value overflows
	reading_t reading;
	reading.value = _value++;
	reading.timestamp = getTimestamp();

	for (auto s : _sensors) {
#ifdef DEBUG
		LOG(debug) << _groupName << "::" << s->getName() << ": \"" << reading.value << "\"";
#endif
		//to keep the _cacheIndex uniform for all sensors store value in every case
		s->storeReading(reading);
	}
}

void TesterSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "Value:        " << _value;
	LOG_VAR(ll) << leading << "Num Sensors:  " << _numSensors;
}
