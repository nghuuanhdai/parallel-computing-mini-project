//================================================================================
// Name        : SNMPSensorBase.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for SNMP plugin.
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
 * @defgroup snmp SNMP plugin
 * @ingroup  pusherplugins
 *
 * @brief Retrieve data from devices running a SNMP agent via the SNMP protocol.
 */

#ifndef SNMP_SNMPSENSORBASE_H_
#define SNMP_SNMPSENSORBASE_H_

#include "SNMPConnection.h"

#include "sensorbase.h"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup snmp
 */
class SNMPSensorBase : public SensorBase {
      public:
	SNMPSensorBase(const std::string &name)
	    : SensorBase(name),
	      _oidLen(0) {
		memset(_oid, 0x0, sizeof(oid) * MAX_OID_LEN);
	}

	SNMPSensorBase(const SNMPSensorBase &other)
	    : SensorBase(other),
	      _oidLen(other._oidLen),
	      _oidSuffix(other._oidSuffix) {
		memset(_oid, 0x0, sizeof(oid) * MAX_OID_LEN);
		memcpy(_oid, other._oid, other._oidLen);
	}

	virtual ~SNMPSensorBase() {}

	SNMPSensorBase operator=(const SNMPSensorBase &other) {
		SensorBase::operator=(other);
		memset(_oid, 0x0, sizeof(oid) * MAX_OID_LEN);
		memcpy(_oid, other._oid, _oidLen);
		_oidLen = other._oidLen;

		return *this;
	}

	void setOIDSuffix(const std::string &suffix) {
		_oidSuffix = suffix;
	}

	void setOID(const std::string &oid) {
		size_t newoidnamelen = MAX_OID_LEN;
		if (read_objid(oid.c_str(), _oid, &newoidnamelen)) {
			_oidLen = newoidnamelen;
		} else {
			_oidLen = 0;
			//LOG(error) << _name << ": Cannot convert OID string: " << oid;
		}
	}

	size_t            getOIDLen() const { return _oidLen; }
	const oid *const  getOID() const { return _oid; }
	const std::string getOIDString() {
		char buf[255];
		int  len = snprint_objid(buf, 255, _oid, _oidLen);
		if (len == -1) {
			//LOG(error) << _name << ": getOIDString buffer not large enough!";
			return std::string("");
		}
		return std::string(buf, len);
	}
	const std::string getOIDSuffix() const { return _oidSuffix; }

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << "    OIDsuffix:         " << getOIDSuffix();
	}

      protected:
	oid         _oid[MAX_OID_LEN];
	size_t      _oidLen;
	std::string _oidSuffix;
};

#endif /* SNMP_SNMPSENSORBASE_H_ */
