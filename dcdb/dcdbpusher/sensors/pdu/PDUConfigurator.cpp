//================================================================================
// Name        : PDUConfigurator.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for PDU plugin configurator class.
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

#include "PDUConfigurator.h"

#include <iostream>
#include <sstream>

PDUConfigurator::PDUConfigurator() {
	_entityName = "pdu";
	_groupName = "group";
	_baseName = "sensor";
}

PDUConfigurator::~PDUConfigurator() {}

void PDUConfigurator::sensorBase(PDUSensorBase &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("path", setXMLPath);
	}
}

void PDUConfigurator::sensorGroup(PDUSensorGroup &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("request", setRequest);
	}
}

void PDUConfigurator::sensorEntity(PDUUnit &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("host", setHost);
	}
}
