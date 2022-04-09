//================================================================================
// Name        : OpaSensorGroup.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Opa sensor group class.
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

#include "OpaSensorGroup.h"

OpaSensorGroup::OpaSensorGroup(const std::string name)
    : SensorGroupTemplate(name),
      _hfiNum(0),
      _portNum(0),
      _port(nullptr) {
	_imageId = {0};
	_imageInfo = {0};
}

OpaSensorGroup::OpaSensorGroup(const OpaSensorGroup &other)
    : SensorGroupTemplate(other),
      _hfiNum(other._hfiNum),
      _portNum(other._portNum),
      _port(nullptr) {
	_imageId = {0};
	_imageInfo = {0};
}

OpaSensorGroup::~OpaSensorGroup() {
	if (_port) {
		omgt_close_port(_port);
	}
}

OpaSensorGroup &OpaSensorGroup::operator=(const OpaSensorGroup &other) {
	SensorGroupTemplate::operator=(other);
	_hfiNum = other._hfiNum;
	_portNum = other._portNum;
	_port = nullptr;
	_imageId = {0};
	_imageInfo = {0};

	return *this;
}

bool OpaSensorGroup::execOnStart() {
	if (omgt_open_port_by_num(&_port, _hfiNum, _portNum, NULL) != OMGT_STATUS_SUCCESS) {
		LOG(error) << "Sensorgroup " << _groupName << " failed to open port or initialize PA connection";
		_port = nullptr;
		return false;
	}

	if (omgt_pa_get_image_info(_port, _imageId, &_imageInfo)) {
		LOG(error) << "Sensorgroup " << _groupName << " failed to get PA image";
		omgt_close_port(_port);
		_port = nullptr;
		return false;
	}

	return true;
}

void OpaSensorGroup::execOnStop() {
	if (_port) {
		omgt_close_port(_port);
		_port = nullptr;
	}
}

void OpaSensorGroup::read() {
	ureading_t reading;
	reading.timestamp = getTimestamp();

	STL_PORT_COUNTERS_DATA portCounters;
	try {
		if (omgt_pa_get_port_stats2(_port, _imageId, 1, _portNum, &_imageId, &portCounters, NULL, 0, 1)) {
			throw std::runtime_error("Failed to get port counters");
		}
	} catch (const std::exception &e) {
		LOG(error) << "Sensorgroup" << _groupName << " could not read value: " << e.what();
		return;
	}

	for (const auto &s : _sensors) {
		switch (s->getCounterData()) {
			case (portXmitData):
				reading.value = portCounters.portXmitData;
				break;
			case (portRcvData):
				reading.value = portCounters.portRcvData;
				break;
			case (portXmitPkts):
				reading.value = portCounters.portXmitPkts;
				break;
			case (portRcvPkts):
				reading.value = portCounters.portRcvPkts;
				break;
			case (portMulticastXmitPkts):
				reading.value = portCounters.portMulticastXmitPkts;
				break;
			case (portMulticastRcvPkts):
				reading.value = portCounters.portMulticastRcvPkts;
				break;
			case (localLinkIntegrityErrors):
				reading.value = portCounters.localLinkIntegrityErrors;
				break;
			case (fmConfigErrors):
				reading.value = portCounters.fmConfigErrors;
				break;
			case (portRcvErrors):
				reading.value = portCounters.portRcvErrors;
				break;
			case (excessiveBufferOverruns):
				reading.value = portCounters.excessiveBufferOverruns;
				break;
			case (portRcvConstraintErrors):
				reading.value = portCounters.portRcvConstraintErrors;
				break;
			case (portRcvSwitchRelayErrors):
				reading.value = portCounters.portRcvSwitchRelayErrors;
				break;
			case (portXmitDiscards):
				reading.value = portCounters.portXmitDiscards;
				break;
			case (portXmitConstraintErrors):
				reading.value = portCounters.portXmitConstraintErrors;
				break;
			case (portRcvRemotePhysicalErrors):
				reading.value = portCounters.portRcvRemotePhysicalErrors;
				break;
			case (swPortCongestion):
				reading.value = portCounters.swPortCongestion;
				break;
			case (portXmitWait):
				reading.value = portCounters.portXmitWait;
				break;
			case (portRcvFECN):
				reading.value = portCounters.portRcvFECN;
				break;
			case (portRcvBECN):
				reading.value = portCounters.portRcvBECN;
				break;
			case (portXmitTimeCong):
				reading.value = portCounters.portXmitTimeCong;
				break;
			case (portXmitWastedBW):
				reading.value = portCounters.portXmitWastedBW;
				break;
			case (portXmitWaitData):
				reading.value = portCounters.portXmitWaitData;
				break;
			case (portRcvBubble):
				reading.value = portCounters.portRcvBubble;
				break;
			case (portMarkFECN):
				reading.value = portCounters.portMarkFECN;
				break;
			case (linkErrorRecovery):
				reading.value = portCounters.linkErrorRecovery;
				break;
			case (linkDowned):
				reading.value = portCounters.linkDowned;
				break;
			case (uncorrectableErrors):
				reading.value = portCounters.uncorrectableErrors;
				break;
			default:
				LOG(error) << _groupName << "::" << s->getName() << " could not read value!";
				continue;
				break;
		}
		s->storeReading(reading);
#ifdef DEBUG
		LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
	}
}

void OpaSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "HFI Num:  " << _hfiNum;
	LOG_VAR(ll) << leading << "Port Num: " << _portNum;
}
