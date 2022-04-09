//================================================================================
// Name        : RESTConfigurator.cpp
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for REST plugin configurator class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2021 Leibniz Supercomputing Centre
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

#include "RESTConfigurator.h"

#include <iostream>
#include <sstream>

RESTConfigurator::RESTConfigurator() {
	_entityName = "host";
	_groupName = "group";
	_baseName = "sensor";
}

RESTConfigurator::~RESTConfigurator() {}

void RESTConfigurator::sensorBase(RESTSensorBase &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("path", setXMLPath);
	}
}

void RESTConfigurator::sensorGroup(RESTSensorGroup &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("endpoint", setEndpoint);
		ATTRIBUTE("request", setRequest);
	}
}

void RESTConfigurator::sensorEntity(RESTUnit &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("baseurl", setBaseURL);
		ATTRIBUTE("authendpoint", setAuthEndpoint);
		ATTRIBUTE("authdata", setAuthData);
	}
}
