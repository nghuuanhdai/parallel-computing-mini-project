//================================================================================
// Name        : IPMIHost.h
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for IPMIHost class.
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

#ifndef IPMIHOST_H_
#define IPMIHOST_H_

#include "../../includes/EntityInterface.h"

#include <freeipmi/freeipmi.h>
#include <list>
#include <string>

class LenovoXCC;

/**
 * @brief Handles all connections to the same IPMI host.
 *
 * @ingroup ipmi
 */
class IPMIHost : public EntityInterface {
      public:
	IPMIHost(const std::string &name = "");
	IPMIHost(const IPMIHost &other);
	virtual ~IPMIHost();
	IPMIHost &operator=(const IPMIHost &other);

	/* Open/close connection to BMC (sets/destroys _ipmiCtx) */
	int connect();
	int disconnect();

	/* Translate recordId to SDR record */
	void getSdrRecord(uint16_t recordId, std::vector<uint8_t> &record);
	/* Send raw command to BMC. Returns sensor reading as responded by BMC. */
	int sendRawCmd(const uint8_t *rawCmd, unsigned int rawCmdLen, void *buf, unsigned int bufLen);
	/* Read the sensor specified by its record id. Returns sensor reading. */
	double readSensorRecord(std::vector<uint8_t> &record);

	void setAuth(uint8_t auth) { _auth = auth; }
	void setHostName(const std::string &hostName) { _name = hostName; }
	void setPassword(const std::string &password) { _password = password; }
	void setCache(const std::string &cacheDir) { _cache = cacheDir + ".ipmiPluginSdrCache." + _name; }
	void setPriv(uint8_t priv) { _priv = priv; }
	void setCipher(const std::string &cipher) { _cipher = stoi(cipher); }
	void setIpmiVersion(const std::string &ipmiVersion) { _ipmiVersion = stoi(ipmiVersion); }
	void setUserName(const std::string &userName) { _userName = userName; }
	void setSessionTimeout(uint32_t sessionTimeout) { _sessionTimeout = sessionTimeout; }
	void setRetransmissionTimeout(uint32_t retransmissionTimeout) { _retransmissionTimeout = retransmissionTimeout; }
	void setXCC(LenovoXCC* xcc) { _xcc = xcc; }

	uint8_t            getAuth() const { return _auth; }
	const std::string &getHostName() const { return _name; }
	const std::string &getPassword() const { return _password; }
	const std::string &getCache() const { return _cache; }
	uint8_t            getPriv() const { return _priv; }
	uint8_t            getCipher() const { return _cipher; }
	uint8_t            getIpmiVersion() const { return _ipmiVersion; }
	const std::string &getUserName() const { return _userName; }
	LenovoXCC*         getXCC() { return _xcc; }

	void printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) final override;

      private:
	/* Open and create/destroy SDR cache (sets/destroys _sdrCtx and _sensorReadCtx) */
	void createSdrCache();
	void destroySdrCache();

	/* Various context structs, required to make use of FreeIPMI */
	ipmi_ctx_t             _ipmiCtx;
	ipmi_sensor_read_ctx_t _sensorReadCtx;

	std::string       _userName;
	std::string       _password;
	std::string       _cache;
	uint8_t           _auth;
	uint8_t           _priv;
	uint8_t           _cipher;
	uint8_t           _ipmiVersion;
	uint32_t          _sessionTimeout;
	uint32_t          _retransmissionTimeout;
	LenovoXCC*        _xcc;
	uint32_t          _errorCount;
	uint64_t          _nextConnectAfter;
};

#endif /* IPMIHOST_H_ */
