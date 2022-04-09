//================================================================================
// Name        : IPMIConfigurator.cpp
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for IPMI plugin configurator class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2017-2019 Leibniz Supercomputing Centre
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

#include "IPMIConfigurator.h"

#include <boost/optional.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>

IPMIConfigurator::IPMIConfigurator() {
	_globalHost.sessionTimeout = 0;
	_globalHost.retransmissionTimeout = 0;

	_entityName = "host";
	_groupName = "group";
	_baseName = "sensor";
}

IPMIConfigurator::~IPMIConfigurator() {}

bool IPMIConfigurator::readConfig(std::string cfgPath) {
	if (ConfiguratorTemplateEntity<IPMISensorBase, IPMISensorGroup, IPMIHost>::readConfig(cfgPath)) {
		for (auto &g : _sensorGroups) {
			if (!g->checkConfig()) {
				return false;
			}
		}
		return true;
	} else {
		return false;
	}
}

void IPMIConfigurator::sensorBase(IPMISensorBase &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("cmd", setRawCmd);
		ATTRIBUTE("lsb", setLsb);
		ATTRIBUTE("msb", setMsb);
		ATTRIBUTE("recordId", setRecordId);
		ATTRIBUTE("type", setType);
	}

	if (s.getLsb() < s.getMsb()) {
		if (s.getMsb() - s.getLsb() >= 8) {
			s.setMsb(s.getLsb() + 7);
			LOG(warning) << "Maximum length of IPMI raw response interval is 8 bytes, setting msb offset to " << (int)s.getMsb();
		}
	} else {
		if (s.getLsb() - s.getMsb() >= 8) {
			s.setLsb(s.getMsb() + 7);
			LOG(warning) << "Maximum length of IPMI raw response interval is 8 bytes, setting lsb offset to " << (int)s.getLsb();
		}
	}
}

void IPMIConfigurator::sensorGroup(IPMISensorGroup &s, CFG_VAL config) {
	ADD {
		//no group attributes currently
	}
}

void IPMIConfigurator::sensorEntity(IPMIHost &s, CFG_VAL config) {
	s.setSessionTimeout(_globalHost.sessionTimeout);
	s.setRetransmissionTimeout(_globalHost.retransmissionTimeout);
	s.setCache(_tempdir);
	ADD {
		ATTRIBUTE("username", setUserName);
		ATTRIBUTE("password", setPassword);
		ATTRIBUTE("cipher", setCipher);
		ATTRIBUTE("ipmiVersion", setIpmiVersion);
	}
}

void IPMIConfigurator::global(CFG_VAL config) {
	ADD {
		SETTING("SessionTimeout") {
			_globalHost.sessionTimeout = stoi(val.second.data());
		}
		SETTING("RetransmissionTimeout") {
			_globalHost.retransmissionTimeout = stoi(val.second.data());
		}
	}
}

void IPMIConfigurator::printConfiguratorConfig(LOG_LEVEL ll) {
	LOG_VAR(ll) << "        Session Timeout:        " << _globalHost.sessionTimeout;
	LOG_VAR(ll) << "        Retransmission Timeout: " << _globalHost.retransmissionTimeout;
	LOG_VAR(ll) << "        Temporal write dir:     " << _tempdir;
}
