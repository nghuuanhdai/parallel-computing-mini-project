//================================================================================
// Name        : IPMISensorGroup.cpp
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for IPMI sensor group class.
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

#include "IPMISensorGroup.h"
#include "IPMIHost.h"
#include "LenovoXCC.h"

#include <chrono>
#include <exception>
#include <iostream>

IPMISensorGroup::IPMISensorGroup(const std::string &name)
    : SensorGroupTemplateEntity(name) {
}

IPMISensorGroup::IPMISensorGroup(const IPMISensorGroup &other)
    : SensorGroupTemplateEntity(other) {
}

IPMISensorGroup::~IPMISensorGroup() {}

IPMISensorGroup &IPMISensorGroup::operator=(const IPMISensorGroup &other) {
	SensorGroupTemplate::operator=(other);

	return *this;
}

float IPMISensorGroup::getMsgRate()  {
	float msgRate = 0;
	for (const auto &s : _sensors) {
		switch(s->getType()) {
			case IPMISensorBase::sensorType::xccDatastorePower:
				msgRate+= 3000.0f / (s->getSubsampling() * _minValues);
				break;
			case IPMISensorBase::sensorType::xccBulkPower:
				msgRate+= 100.0f / (s->getSubsampling() * _minValues);
				break;
			case IPMISensorBase::sensorType::xccBulkEnergy:
				msgRate+= 101.0f / (s->getSubsampling() * _minValues);
				break;
			default:
				msgRate+= SensorGroupTemplate::getMsgRate();
		}
	}
	return msgRate;
}

uint64_t IPMISensorGroup::readRaw(const std::vector<uint8_t> &rawCmd, uint8_t lsb, uint8_t msb) {
	uint8_t buf[256];
	int     len = -1;

	try {
		len = _entity->sendRawCmd(&rawCmd[0], rawCmd.size(), (void *)buf, sizeof(buf));
	} catch (const std::exception &e) {
		throw e;
		return 0;
	}

	std::stringstream ss;
	if (msb > len) {
		ss << "Error processing IPMI raw data: msb=" << msb << " > len=" << len;
	} else if (lsb > len) {
		ss << "Error processing IPMI raw data: lsb=" << lsb << " > len=" << len;
	}
	if (ss.gcount() > 0) {
		throw std::runtime_error(ss.str());
		return 0;
	}

	int64_t val = 0;
	int     i;
	if (msb > lsb) {
		for (i = lsb; i <= msb; i++) {
			val |= ((int64_t)buf[i]) << (msb - i) * 8;
		}
	} else {
		for (i = lsb; i >= msb; i--) {
			val |= ((int64_t)buf[i]) << (i - msb) * 8;
		}
	}

	return val;
}

void IPMISensorGroup::read() {
	reading_t reading;
	reading.value = 0;
	reading.timestamp = getTimestamp();

	if (_entity->connect() == 0) {
		for (const auto &s : _sensors) {
			try {
				switch (s->getType()) {
					case IPMISensorBase::sensorType::xccDatastorePower: {
						std::vector<reading_t> readings;
						LenovoXCC xcc(_entity);
						if (_entity->getXCC()->getDatastorePower(readings) == 0) {
							for (unsigned int i=0; i<readings.size(); i++) {
								s->storeReading(readings[i]);
							}
							reading = readings.back();
						}
						break;
					}
					case IPMISensorBase::sensorType::xccSingleEnergy: {
						if (_entity->getXCC()->getSingleEnergy(reading) == 0) {
							s->storeReading(reading);
						}
						break;
					}
					case IPMISensorBase::sensorType::xccBulkPower: {
						std::vector<reading_t> readings;
						if (_entity->getXCC()->getBulkPower(readings) == 0) {
							for (unsigned int i=0; i<readings.size(); i++) {
								s->storeReading(readings[i]);
							}
							reading = readings.back();
						}
						break;
					}
					case IPMISensorBase::sensorType::xccBulkEnergy: {
						std::vector<reading_t> readings;
						if (_entity->getXCC()->getBulkEnergy(readings) == 0) {
							for (unsigned int i=0; i<readings.size(); i++) {
								s->storeReading(readings[i]);
							}
							reading = readings.back();
						}
						break;
					}
					case IPMISensorBase::sensorType::sdr: {
						std::vector<uint8_t> sdrRecord = s->getSdrRecord();
						if (sdrRecord.size() == 0) {
							_entity->getSdrRecord(s->getRecordId(), sdrRecord);
							s->setSdrRecord(sdrRecord);
						}
						reading.value = _entity->readSensorRecord(sdrRecord);
						s->storeReading(reading);
						break;
					}
					case IPMISensorBase::sensorType::raw: {
						reading.value = readRaw(s->getRawCmd(), s->getLsb(), s->getMsb());
						s->storeReading(reading);
						break;
					}
					default:
						LOG(error) << _groupName << "::" << s->getName() << " undefined sensor type";
						break;
				}
#ifdef DEBUG
				LOG(debug) << s->getName() << " reading: ts=" << prettyPrintTimestamp(reading.timestamp) << " val=" << reading.value;
#endif
			} catch (const std::exception &e) {
				LOG(error) << _entity->getName() << " " << e.what() << ": " << s->getName();
				continue;
			}
		}
		_entity->disconnect();
	}
}

uint64_t IPMISensorGroup::nextReadingTime() {
	if ((_sensors.size() == 1) && (_sensors.front()->getType() == IPMISensorBase::sensorType::xccDatastorePower)) {
		reading_t r = _sensors.front()->getLatestValue();
		uint64_t  now = getTimestamp();

		if (r.timestamp < now - S_TO_NS(35)) {
			// There was no reading yet or it is too old, so schedule next in 30s
			return now + S_TO_NS(30);
		} else {
			// The first reading of the next 30s block is 10ms after the last reading in the current block.
			// A block becomes available 30s after its first reading, adding 2s grace period
			return r.timestamp + MS_TO_NS(32010);
		}
	}
	return SensorGroupInterface::nextReadingTime();
}

bool IPMISensorGroup::checkConfig() {
	if (_sensors.size() > 1) {
		for (const auto &s : _sensors) {
			if (s->getType() == IPMISensorBase::sensorType::xccDatastorePower) {
				LOG(error) << _groupName << " contains an XCC sensor among others, this is not possible";
				return false;
			} 
		}
	}
	
	for (const auto &s : _sensors) {
		if (s->getType() == IPMISensorBase::sensorType::undefined) {
			LOG(error) << _groupName << "::" << s->getName() << " has an undefined sensor type";
			return false;
		}
		if (s->getType() == IPMISensorBase::sensorType::xccDatastorePower) {
			if (_queueSize < 3000) {
				LOG(info) << _groupName << "::" << s->getName() << " increasing queueSize to 3000 to store all data store readings (was " << _queueSize << ")";
				_queueSize = 3000;
			}
		}
	}
	return true;
}

void IPMISensorGroup::execOnInit() {
	for (const auto &s : _sensors) {
		switch(s->getType()) {
			case IPMISensorBase::sensorType::xccDatastorePower:
			case IPMISensorBase::sensorType::xccSingleEnergy:
			case IPMISensorBase::sensorType::xccBulkPower:
			case IPMISensorBase::sensorType::xccBulkEnergy:
				if (_entity->getXCC() == nullptr) {
					_entity->setXCC(new LenovoXCC(_entity));
				}
				break;
			default:
				break;
		}
	}
}
