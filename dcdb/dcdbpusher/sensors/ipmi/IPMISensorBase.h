//================================================================================
// Name        : IPMISensorBase.h
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for IPMI plugin.
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

/**
 * @defgroup ipmi IPMI plugin
 * @ingroup  pusherplugins
 *
 * @brief Collect data from devices via the IPMI protocol.
 */

#ifndef SRC_SENSORS_IPMI_IPMISENSORBASE_H_
#define SRC_SENSORS_IPMI_IPMISENSORBASE_H_

#include "sensorbase.h"

#include "IPMIHost.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <sstream>
#include <vector>

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup ipmi
 */
class IPMISensorBase : public SensorBase {
      public:
	IPMISensorBase(const std::string &name)
	    : SensorBase(name),
	      _type(undefined),
	      _recordId(0),
	      _lsb(0),
	      _msb(0) {}

	IPMISensorBase(const IPMISensorBase &other)
	    : SensorBase(other),
	      _type(other._type),
	      _recordId(other._recordId),
	      _sdrRecord(other._sdrRecord),
	      _rawCmd(other._rawCmd),
	      _lsb(other._lsb),
	      _msb(other._msb) {}

	virtual ~IPMISensorBase() {}

	IPMISensorBase &operator=(const IPMISensorBase &other) {
		SensorBase::operator=(other);
		_type = other._type;
		_recordId = other._recordId;
		_sdrRecord = other._sdrRecord;
		_rawCmd = other._rawCmd;
		_lsb = other._lsb;
		_msb = other._msb;

		return *this;
	}

	enum sensorType {
		undefined = 0,
		raw,
		sdr,
		xccDatastorePower,
		xccSingleEnergy,
		xccBulkPower,
		xccBulkEnergy
	};

	uint16_t                    getRecordId() const { return _recordId; }
	const std::vector<uint8_t> &getSdrRecord() const { return _sdrRecord; }
	const std::vector<uint8_t> &getRawCmd() const { return _rawCmd; }
	std::string                 getRawCmdString() const {
                std::stringstream ss;
                ss << "0x";
                for (auto i : _rawCmd) {
                        ss << std::setfill('0') << std::setw(2) << std::hex << (int)i << " ";
                }
                return ss.str().substr(0, _rawCmd.size() * 3 + 1);
	}
	uint8_t     getLsb() const { return _lsb; }
	uint8_t     getMsb() const { return _msb; }
	sensorType  getType() const { return _type; }
	std::string getTypeString() const {
		switch (_type) {
			case raw:
				return std::string("raw");
			case sdr:
				return std::string("sdr");
			case xccDatastorePower:
				return std::string("xccDatastorePower");
			case xccSingleEnergy:
				return std::string("xccSingleEnergy");
			case xccBulkPower:
				return std::string("xccBulkPower");
			case xccBulkEnergy:
				return std::string("xccBulkEnergy");
			default:
				return std::string("undefined");
		}
	}

	void setRecordId(const std::string &recordId) {
		_recordId = stoul(recordId);
		_type = sdr;
	}
	void setSdrRecord(const std::vector<uint8_t> &sdrRecord) { _sdrRecord = sdrRecord; }
	void setRawCmd(std::string &rawCmd) {
		boost::regex                                       expr("(?:0x)?([0-9a-fA-F]+)");
		boost::regex_token_iterator<std::string::iterator> it{rawCmd.begin(), rawCmd.end(), expr, 1};
		boost::regex_token_iterator<std::string::iterator> end;
		while (it != end) {
			_rawCmd.push_back(stoi(*it++, NULL, 16));
		}
		_type = raw;
	}
	void setLsb(const std::string &lsb) { _lsb = stoi(lsb); }
	void setLsb(uint8_t lsb) { _lsb = lsb; }
	void setMsb(const std::string &msb) { _msb = stoi(msb); }
	void setMsb(uint8_t msb) { _msb = msb; }
	void setType(const std::string &type) {
		if (boost::iequals(type, "raw")) {
			_type = raw;
		} else if (boost::iequals(type, "sdr")) {
			_type = sdr;
		} else if (boost::iequals(type, "xccDatastorePower")) {
			_type = xccDatastorePower;
		} else if (boost::iequals(type, "xccSingleEnergy")) {
			_type = xccSingleEnergy;
		} else if (boost::iequals(type, "xccBulkPower")) {
			_type = xccBulkPower;
		} else if (boost::iequals(type, "xccBulkEnergy")) {
			_type = xccBulkEnergy;
		} else {
			_type = undefined;
		}
	}

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) override {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << "    Type:              " << getTypeString();
		switch (_type) {
			case raw:
				LOG_VAR(ll) << leading << "    Raw Cmd:           " << getRawCmdString();
				LOG_VAR(ll) << leading << "    lsb:               " << (int)_lsb;
				LOG_VAR(ll) << leading << "    msb:               " << (int)_msb;
				break;
			case sdr:
				LOG_VAR(ll) << leading << "    Record Id:         " << _recordId;
				break;
			default:
				break;
		}
	}

      protected:
	sensorType           _type;
	uint16_t             _recordId;
	std::vector<uint8_t> _sdrRecord;

	std::vector<uint8_t> _rawCmd;
	uint8_t              _lsb;
	uint8_t              _msb;
};

#endif /* SRC_SENSORS_IPMI_IPMISENSORBASE_H_ */
