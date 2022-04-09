//================================================================================
// Name        : OpaSensorBase.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for Opa plugin.
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

/**
 * @defgroup opa OPA plugin
 * @ingroup  pusherplugins
 *
 * @brief Collect data from Intel Omni-Path Architecture (OPA) interfaces.
 */

#ifndef OPA_OPASENSORBASE_H_
#define OPA_OPASENSORBASE_H_

#include "sensorbase.h"

enum PORT_COUNTER_DATA {
	portXmitData = 0,
	portRcvData = 1,
	portXmitPkts = 2,
	portRcvPkts = 3,
	portMulticastXmitPkts = 4,
	portMulticastRcvPkts = 5,
	localLinkIntegrityErrors = 6,
	fmConfigErrors = 7,
	portRcvErrors = 8,
	excessiveBufferOverruns = 9,
	portRcvConstraintErrors = 10,
	portRcvSwitchRelayErrors = 11,
	portXmitDiscards = 12,
	portXmitConstraintErrors = 13,
	portRcvRemotePhysicalErrors = 14,
	swPortCongestion = 15,
	portXmitWait = 16,
	portRcvFECN = 17,
	portRcvBECN = 18,
	portXmitTimeCong = 19,
	portXmitWastedBW = 20,
	portXmitWaitData = 21,
	portRcvBubble = 22,
	portMarkFECN = 23,
	linkErrorRecovery = 24,
	linkDowned = 25,
	uncorrectableErrors = 26
};

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup opa
 */
class OpaSensorBase : public SensorBase {
      public:
	OpaSensorBase(const std::string &name)
	    : SensorBase(name),
	      _counterData(static_cast<PORT_COUNTER_DATA>(999)) {
		//default delta to true, as opa has only monotonic sensors usually
		_delta = true;
		_deltaMax = ULLONG_MAX;
	}

	OpaSensorBase(const OpaSensorBase &other)
	    : SensorBase(other),
	      _counterData(other._counterData) {}

	virtual ~OpaSensorBase() {}

	OpaSensorBase &operator=(const OpaSensorBase &other) {
		SensorBase::operator=(other);
		_counterData = other._counterData;

		return *this;
	}

	int getCounterData() const { return _counterData; }

	void setCounterData(int counterData) { _counterData = static_cast<PORT_COUNTER_DATA>(counterData); }

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		std::string cntData("unknown");
		switch (_counterData) {
			case portXmitData:
				cntData = "portXmitData";
				break;
			case portRcvData:
				cntData = "portRcvData";
				break;
			case portXmitPkts:
				cntData = "portXmitPkts";
				break;
			case portRcvPkts:
				cntData = "portRcvPkts";
				break;
			case portMulticastXmitPkts:
				cntData = "portMulticastXmitPkts";
				break;
			case portMulticastRcvPkts:
				cntData = "portMulticastRcvPkts";
				break;
			case localLinkIntegrityErrors:
				cntData = "localLinkIntegrityErrors";
				break;
			case fmConfigErrors:
				cntData = "fmConfigErrors";
				break;
			case portRcvErrors:
				cntData = "portRcvErrors";
				break;
			case excessiveBufferOverruns:
				cntData = "excessiveBufferOverruns";
				break;
			case portRcvConstraintErrors:
				cntData = "portRcvConstraintErrors";
				break;
			case portRcvSwitchRelayErrors:
				cntData = "portRcvSwitchRelayErrors";
				break;
			case portXmitDiscards:
				cntData = "portXmitDiscards";
				break;
			case portXmitConstraintErrors:
				cntData = "portXmitConstraintErrors";
				break;
			case portRcvRemotePhysicalErrors:
				cntData = "portRcvRemotePhysicalErrors";
				break;
			case swPortCongestion:
				cntData = "swPortCongestion";
				break;
			case portXmitWait:
				cntData = "portXmitWait";
				break;
			case portRcvFECN:
				cntData = "portRcvFECN";
				break;
			case portRcvBECN:
				cntData = "portRcvBECN";
				break;
			case portXmitTimeCong:
				cntData = "portXmitTimeCong";
				break;
			case portXmitWastedBW:
				cntData = "portXmitWastedBW";
				break;
			case portXmitWaitData:
				cntData = "portXmitWaitData";
				break;
			case portRcvBubble:
				cntData = "portRcvBubble";
				break;
			case portMarkFECN:
				cntData = "portMarkFECN";
				break;
			case linkErrorRecovery:
				cntData = "linkErrorRecovery";
				break;
			case linkDowned:
				cntData = "linkDowned";
				break;
			case uncorrectableErrors:
				cntData = "uncorrectableErrors";
				break;
		}
		LOG_VAR(ll) << leading << "    Counter data:  " << cntData;
	}

      protected:
	PORT_COUNTER_DATA _counterData;
};

#endif /* OPA_OPASENSORBASE_H_ */
