//================================================================================
// Name        : CaliperConfigurator.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Caliper plugin configurator class.
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

#include "CaliperConfigurator.h"

CaliperConfigurator::CaliperConfigurator() {
	_groupName = "group";
	_baseName = "sensor";
}

CaliperConfigurator::~CaliperConfigurator() {}

void CaliperConfigurator::sensorBase(CaliperSensorBase &s, CFG_VAL config) {
	ADD {
		//no sensor attributes currently
	}
}

void CaliperConfigurator::sensorGroup(CaliperSensorGroup &s, CFG_VAL config) {
	s.setGlobalMqttPrefix(_mqttPrefix);
	ADD {
		ATTRIBUTE("maxSensors", setMaxSensorNum);
		ATTRIBUTE("timeout", setTimeout);
	}
}
