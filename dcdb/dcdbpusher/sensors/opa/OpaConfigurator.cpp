//================================================================================
// Name        : OpaConfigurator.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Opa plugin configurator class.
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

#include "OpaConfigurator.h"

OpaConfigurator::OpaConfigurator() {
	//set up enum-map to map string from cfgFile to an enum value defined in OpaSensorBase.h
	_enumCntData["portXmitData"] = portXmitData;
	_enumCntData["portRcvData"] = portRcvData;
	_enumCntData["portXmitPkts"] = portXmitPkts;
	_enumCntData["portRcvPkts"] = portRcvPkts;
	_enumCntData["portMulticastXmitPkts"] = portMulticastXmitPkts;
	_enumCntData["portMulticastRcvPkts"] = portMulticastRcvPkts;
	_enumCntData["localLinkIntegrityErrors"] = localLinkIntegrityErrors;
	_enumCntData["fmConfigErrors"] = fmConfigErrors;
	_enumCntData["portRcvErrors"] = portRcvErrors;
	_enumCntData["excessiveBufferOverruns"] = excessiveBufferOverruns;
	_enumCntData["portRcvConstraintErrors"] = portRcvConstraintErrors;
	_enumCntData["portRcvSwitchRelayErrors"] = portRcvSwitchRelayErrors;
	_enumCntData["portXmitDiscards"] = portXmitDiscards;
	_enumCntData["portXmitConstraintErrors"] = portXmitConstraintErrors;
	_enumCntData["portRcvRemotePhysicalErrors"] = portRcvRemotePhysicalErrors;
	_enumCntData["swPortCongestion"] = swPortCongestion;
	_enumCntData["portXmitWait"] = portXmitWait;
	_enumCntData["portRcvFECN"] = portRcvFECN;
	_enumCntData["portRcvBECN"] = portRcvBECN;
	_enumCntData["portXmitTimeCong"] = portXmitTimeCong;
	_enumCntData["portXmitWastedBW"] = portXmitWastedBW;
	_enumCntData["portXmitWaitData"] = portXmitWaitData;
	_enumCntData["portRcvBubble"] = portRcvBubble;
	_enumCntData["portMarkFECN"] = portMarkFECN;
	_enumCntData["linkErrorRecovery"] = linkErrorRecovery;
	_enumCntData["linkDowned"] = linkDowned;
	_enumCntData["uncorrectableErrors"] = uncorrectableErrors;

	_groupName = "group";
	_baseName = "sensor";
}

OpaConfigurator::~OpaConfigurator() {}

void OpaConfigurator::sensorBase(OpaSensorBase &s, CFG_VAL config) {
	/**
	 * Custom code because we need to use _enumCntData
	 */
	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
		if (boost::iequals(val.first, "cntData")) {
			enumMap_t::iterator it = _enumCntData.find(val.second.data());
			if (it != _enumCntData.end()) {
				s.setCounterData(it->second);
			} else {
				LOG(warning) << "  cntData \"" << val.second.data() << "\" not known.";
			}
		} else if (boost::iequals(val.first, "delta")) {
			//it is explicitly stated to be off --> set it to false
			s.setDelta(!(val.second.data() == "off"));
		}
	}
}

void OpaConfigurator::sensorGroup(OpaSensorGroup &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("hfiNum", setHfiNum);
		ATTRIBUTE("portNum", setPortNum);
	}
}
