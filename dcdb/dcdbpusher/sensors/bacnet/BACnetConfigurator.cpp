//================================================================================
// Name        : BACnetConfigurator.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for BACnet plugin configurator class.
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

#include "BACnetConfigurator.h"

#include <boost/optional.hpp>
#include <iostream>

BACnetConfigurator::BACnetConfigurator() {
	_bacClient = nullptr;

	_groupName = "group";
	_baseName = "property";
}

BACnetConfigurator::~BACnetConfigurator() {}

void BACnetConfigurator::sensorBase(BACnetSensorBase &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("objectInstance", setObjectInstance);
		ATTRIBUTE("objectType", setObjectType);
		ATTRIBUTE("id", setPropertyId);
	}
}

void BACnetConfigurator::sensorGroup(BACnetSensorGroup &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("deviceInstance", setDeviceInstance);
	}
	s.setEntity(_bacClient.get());
}

void BACnetConfigurator::global(CFG_VAL config) {
	_bacClient = std::make_shared<BACnetClient>("BACnetClient");

	std::string address_cache, interface;
	unsigned    port = 47808, timeout = 1000, apdu_timeout = 200, apdu_retries = 0;

	ADD {
		SETTING("address_cache") {
			address_cache = val.second.data();
			LOG(debug) << "  Address Cache: " << address_cache;
		}
		SETTING("interface") {
			interface = val.second.data();
			LOG(debug) << "  Interface " << interface;
		}
		SETTING("port") {
			port = stoul(val.second.data());
			LOG(debug) << "  Port " << port;
		}
		SETTING("timeout") {
			timeout = stoul(val.second.data());
			LOG(debug) << "  Timeout " << timeout;
		}
		SETTING("apdu_timeout") {
			apdu_timeout = stoul(val.second.data());
			LOG(debug) << "  apdu_timeout " << apdu_timeout;
		}
		SETTING("apdu_retries") {
			apdu_retries = stoul(val.second.data());
			LOG(debug) << "  apdu_retries " << apdu_retries;
		}
	}

	try {
		_bacClient->init(interface, address_cache, port, timeout, apdu_timeout, apdu_retries);
	} catch (const std::exception &e) {
		LOG(error) << "Could not initialize BACnetClient: " << e.what();
		_bacClient = nullptr;
		return;
	}
}
void BACnetConfigurator::printConfiguratorConfig(LOG_LEVEL ll) {
	if (_bacClient) {
		_bacClient->printConfig(ll, 8);
	} else {
		LOG_VAR(ll) << "        No BACClient present!";
	}
}
