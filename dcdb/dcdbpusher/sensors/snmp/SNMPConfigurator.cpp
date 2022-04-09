//================================================================================
// Name        : SNMPConfigurator.cpp
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for SNMP plugin configurator class.
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

#include "SNMPConfigurator.h"

SNMPConfigurator::SNMPConfigurator() {
	/* Initialize SNMP library */
	init_snmp("dcdbpusher_SNMPplugin");

	_entityName = "connection";
	_groupName = "group";
	_baseName = "sensor";
}

SNMPConfigurator::~SNMPConfigurator() {}

void SNMPConfigurator::sensorBase(SNMPSensorBase &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("OIDSuffix", setOIDSuffix);
	}
}

void SNMPConfigurator::sensorGroup(SNMPSensorGroup &s, CFG_VAL config) {
	ADD {
		//no group attributes currently
	}
}

void SNMPConfigurator::sensorEntity(SNMPConnection &s, CFG_VAL config) {
	ADD {
		//ATTRIBUTE("Type", setType) //TODO would be relevant if we would support more than just agent mode...
		//at the moment we just ignore it, as it can be only "Agent" anyways
		ATTRIBUTE("Host", setHost);
		ATTRIBUTE("Community", setSNMPCommunity);
		ATTRIBUTE("OIDPrefix", setOIDPrefix);
		ATTRIBUTE("Version", setVersion);
		ATTRIBUTE("Username", setUsername);
		ATTRIBUTE("SecLevel", setSecurityLevel);
		ATTRIBUTE("AuthProto", setAuthProto);
		ATTRIBUTE("PrivProto", setPrivProto);
		ATTRIBUTE("AuthKey", setAuthKey);
		ATTRIBUTE("PrivKey", setPrivKey);
	}
}
