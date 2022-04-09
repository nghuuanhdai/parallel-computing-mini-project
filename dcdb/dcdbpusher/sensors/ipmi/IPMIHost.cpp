//================================================================================
// Name        : IPMIHost.cpp
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for IPMIHost class.
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

#include "IPMIHost.h"
#include "timestamp.h"
#include "LenovoXCC.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <freeipmi/api/ipmi-api.h>
#include <freeipmi/freeipmi.h>
#include <freeipmi/sensor-read/ipmi-sensor-read.h>

#define RETRIES 2

IPMIHost::IPMIHost(const std::string &name)
    : EntityInterface(name),
      _ipmiCtx(nullptr),
      _sensorReadCtx(nullptr),
      _userName("admin"),
      _password("admin"),
      _cache(""),
      _auth(IPMI_AUTHENTICATION_TYPE_MD5),
      _priv(IPMI_PRIVILEGE_LEVEL_ADMIN),
      _cipher(3),
      _ipmiVersion(1),
      _sessionTimeout(0),
      _retransmissionTimeout(0),
      _xcc(nullptr),
      _errorCount(0),
      _nextConnectAfter(0) {}

IPMIHost::IPMIHost(const IPMIHost &other)
    : EntityInterface(other),
      _ipmiCtx(nullptr),
      _sensorReadCtx(nullptr),
      _userName(other._userName),
      _password(other._password),
      _cache(other._cache),
      _auth(other._auth),
      _priv(other._priv),
      _cipher(other._cipher),
      _ipmiVersion(other._ipmiVersion),
      _sessionTimeout(other._sessionTimeout),
      _retransmissionTimeout(other._retransmissionTimeout),
      _xcc(other._xcc),
      _errorCount(other._errorCount),
      _nextConnectAfter(other._nextConnectAfter) {}

IPMIHost::~IPMIHost() {
	if (_xcc != nullptr) {
		delete _xcc;
	}
}

IPMIHost &IPMIHost::operator=(const IPMIHost &other) {
	EntityInterface::operator=(other);
	_ipmiCtx = nullptr;
	_sensorReadCtx = nullptr;
	_userName = other._userName;
	_password = other._password;
	_cache = other._cache;
	_auth = other._auth;
	_priv = other._priv;
	_cipher = other._cipher;
	_ipmiVersion = other._ipmiVersion;
	_sessionTimeout = other._sessionTimeout;
	_retransmissionTimeout = other._retransmissionTimeout;
	_xcc = other._xcc;
	_errorCount = other._errorCount;
	_nextConnectAfter = other._nextConnectAfter;

	return *this;
}

int IPMIHost::connect() {
	if (getTimestamp() < _nextConnectAfter) {
		return 1;
	}

	if (!(_ipmiCtx = ipmi_ctx_create())) {
		_errorCount++;
		LOG(error) << _name << " Error creating IPMI context (" + std::string(strerror(errno)) + ")";
	} else {
		int workaround_flags = 0;
		int flags = IPMI_FLAGS_DEFAULT;
		int rc;
		if (_ipmiVersion == 1) {
			rc = ipmi_ctx_open_outofband(_ipmiCtx, _name.c_str(), _userName.c_str(), _password.c_str(), _auth, _priv, _sessionTimeout, _retransmissionTimeout, workaround_flags, flags);
		} else {
			rc = ipmi_ctx_open_outofband_2_0(_ipmiCtx, _name.c_str(), _userName.c_str(), _password.c_str(), NULL, 0, _priv, _cipher, _sessionTimeout, _retransmissionTimeout, workaround_flags, flags);
		}
		
		if (rc < 0) {
			_errorCount++;
			LOG(error) << _name << " Error opening IPMI connection (" + std::string(ipmi_ctx_errormsg(_ipmiCtx)) + ")";
			ipmi_ctx_close(_ipmiCtx);
			ipmi_ctx_destroy(_ipmiCtx);
			_ipmiCtx = NULL;
		} else {
			_errorCount = 0;
			return 0;
		}
	}
	
	/* There was an error, check whether we should delay the next connect */
	uint64_t delay = 0;
	if (_errorCount >= 50) {
		delay = 600;
	} else if (_errorCount >= 10) {
		delay = 300;
	} else if (_errorCount >= 5) {
		delay = 60;
	}
	if (delay > 0) {
		LOG(debug) << _name << " Delaying next re-connect for " << delay << "s (errors=" << _errorCount << ")";
		_nextConnectAfter = getTimestamp() + S_TO_NS(delay);
	}
	
	return 2;
}

int IPMIHost::disconnect() {
	if (_sensorReadCtx) {
		ipmi_sensor_read_ctx_destroy(_sensorReadCtx);
		_sensorReadCtx = NULL;
	}

	if (_ipmiCtx) {
		ipmi_ctx_close(_ipmiCtx);
		ipmi_ctx_destroy(_ipmiCtx);
		_ipmiCtx = NULL;
		return 0;
	}
	return 1;
}

void IPMIHost::getSdrRecord(uint16_t recordId, std::vector<uint8_t> &record) {
	ipmi_sdr_ctx_t sdrCtx = ipmi_sdr_ctx_create();
	if (!sdrCtx) {
		throw std::runtime_error("Error creating SDR context (" + std::string(strerror(errno)) + ")");
	}

	bool success = false;
	std::string errorMsg;
	if (ipmi_sdr_cache_open(sdrCtx, _ipmiCtx, _cache.c_str()) < 0) {
		if ((ipmi_sdr_ctx_errnum(sdrCtx) == IPMI_SDR_ERR_CACHE_READ_CACHE_DOES_NOT_EXIST) || (ipmi_sdr_ctx_errnum(sdrCtx) == IPMI_SDR_ERR_CACHE_INVALID) || (ipmi_sdr_ctx_errnum(sdrCtx) == IPMI_SDR_ERR_CACHE_OUT_OF_DATE)) {
			if ((ipmi_sdr_ctx_errnum(sdrCtx) == IPMI_SDR_ERR_CACHE_INVALID) || (ipmi_sdr_ctx_errnum(sdrCtx) == IPMI_SDR_ERR_CACHE_OUT_OF_DATE)) {
				LOG(debug) << _name << " Deleting SDR cache " << _cache;
				ipmi_sdr_cache_close(sdrCtx);
				ipmi_sdr_cache_delete(sdrCtx, _cache.c_str());
			}
			if (ipmi_sdr_cache_create(sdrCtx, _ipmiCtx, _cache.c_str(), IPMI_SDR_CACHE_CREATE_FLAGS_DEFAULT, NULL, NULL) == 0) {
				LOG(debug) << _name << " Created new SDR cache " << _cache;
			} else {
				errorMsg = "Error creating new SDR cache " + _cache;
			}
		} else {
			errorMsg = "Error opening SDR cache (" + std::string(ipmi_sdr_ctx_errormsg(sdrCtx)) + ")";
		}
	} else {
		int     recordLength = 0;
		uint8_t recordBuf[IPMI_SDR_MAX_RECORD_LENGTH];
		
		if (ipmi_sdr_cache_search_record_id(sdrCtx, recordId) < 0) {
			errorMsg = "Error searching SDR record (" + std::string(ipmi_sdr_ctx_errormsg(sdrCtx))  + ")";
		} else {
			if ((recordLength = ipmi_sdr_cache_record_read(sdrCtx, recordBuf, IPMI_SDR_MAX_RECORD_LENGTH)) < 0) {
				errorMsg = "Error reading SDR record (" + std::string(ipmi_sdr_ctx_errormsg(sdrCtx)) + ")";
			} else {
				record.insert(record.end(), &recordBuf[0], &recordBuf[recordLength]);
				success = true;
			}
		}
		ipmi_sdr_cache_close(sdrCtx);
	}
	ipmi_sdr_ctx_destroy(sdrCtx);

	if (!success) {
		throw std::runtime_error(errorMsg);
	}
}

int IPMIHost::sendRawCmd(const uint8_t *rawCmd, unsigned int rawCmdLen, void *buf, unsigned int bufLen) {
	if (!IPMI_NET_FN_RQ_VALID(rawCmd[1])) {
		throw std::runtime_error("Error sending IPMI raw command (Invalid netfn value)");
	}

	int len = -1;
	if ((len = ipmi_cmd_raw(_ipmiCtx, rawCmd[0], rawCmd[1], &rawCmd[2], rawCmdLen - 2, buf, bufLen)) < 0) {
		throw std::runtime_error("Error sending IPMI raw command (" + std::string(ipmi_ctx_errormsg(_ipmiCtx)) + ")");
	} else {
#if 0
		std::cout << "IPMIHost::sendRawCmd() received " << len << " bytes: " << std::setw(2) << std::setfill('0') << std::hex;
		for (i = 0; i < len; i++) {
			std::cout << (unsigned) buf[i] << " ";
		}
		std::cout << std::dec << std::endl;
#endif
		return len;
	}
}

double IPMIHost::readSensorRecord(std::vector<uint8_t> &record) {
	uint8_t  rawReading = 0;
	double * reading = NULL;
	uint16_t eventBitmask = 0;

	if (!_sensorReadCtx) {
		_sensorReadCtx = ipmi_sensor_read_ctx_create(_ipmiCtx);
	}
	
	if (_sensorReadCtx) {
		if (ipmi_sensor_read(_sensorReadCtx, &record[0], record.size(), 0, &rawReading, &reading, &eventBitmask) < 0) {
			throw std::runtime_error("Error reading IPMI record (" + std::string(ipmi_sensor_read_ctx_errormsg(_sensorReadCtx)) + ")");
		} else {
			double ret = .0;
			if (reading) {
				ret = *reading;
				free(reading);
			}
			return ret;
		}
	} else {
		throw std::runtime_error("Error creating sensor context (" + std::string(ipmi_ctx_errormsg(_ipmiCtx)) + ")");
	}
}

void IPMIHost::printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "UserName:     " << getUserName();
#ifdef DEBUG
	LOG_VAR(ll) << leading << "Password:     " << getPassword();
#else
	LOG_VAR(ll) << leading << "Password:     <not shown>";
#endif
	LOG_VAR(ll) << leading << "Cache:        " << getCache();
	LOG_VAR(ll) << leading << "IPMI Version: " << (int)getIpmiVersion();
	if (getIpmiVersion() == 1) {
		LOG_VAR(ll) << leading << "Auth:         " << (int)getAuth();
	}
	LOG_VAR(ll) << leading << "Priv:         " << (int)getPriv();
	if (getIpmiVersion() == 2) {
		LOG_VAR(ll) << leading << "Cipher:       " << (int)getCipher();
	}
	LOG_VAR(ll) << leading << "Session Timeout:        " << _sessionTimeout;
	LOG_VAR(ll) << leading << "Retransmission Timeout: " << _retransmissionTimeout;
}
