//================================================================================
// Name        : SNMPConnection.h
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for SNMPConnection class.
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

#ifndef SNMPCONNECTION_H_
#define SNMPCONNECTION_H_

#include "../../includes/EntityInterface.h"

#include <boost/algorithm/string.hpp>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#define AUTH_MD5 "MD5"
#define AUTH_SHA1 "SHA1"
#define PRIV_DES "DES"
#define PRIV_AES "AES"
#define SEC_NOAUTHNOPRIV "noAuthNoPriv"
#define SEC_AUTHNOPRIV "authNoPriv"
#define SEC_AUTHPRIV "authPriv"

/**
 * @brief This class handles a SNMP connection.
 *
 * @ingroup snmp
 */
class SNMPConnection : public EntityInterface {

      public:
	SNMPConnection(const std::string &name = "SNMPConn");
	SNMPConnection(const SNMPConnection &other);
	virtual ~SNMPConnection();
	SNMPConnection &operator=(const SNMPConnection &other);

	void setSNMPCommunity(const std::string &community) { _snmpCommunity = community; }
	void setOIDPrefix(const std::string &oidPrefix) {
		/* Ensure that the oidPrefix does not contain a trailing . */
		_oidPrefix = oidPrefix;
		if (_oidPrefix.length() > 0) {
			if (_oidPrefix.at(_oidPrefix.length() - 1) == '.') {
				_oidPrefix.resize(_oidPrefix.size() - 1);
			}
		}
	}
	void setUsername(const std::string &username) { _username = username; }
	void setAuthKey(const std::string &authKey) { _authKey = authKey; }
	void setPrivKey(const std::string &privKey) { _privKey = privKey; }
	void setAuthProto(const std::string &authProto) {
		if (boost::iequals(authProto, AUTH_MD5)) {
			_authProto = usmHMACMD5AuthProtocol;
			_authProtoLen = USM_AUTH_PROTO_MD5_LEN;
		} else if (boost::iequals(authProto, AUTH_SHA1)) {
			_authProto = usmHMACSHA1AuthProtocol;
			_authProtoLen = USM_AUTH_PROTO_SHA_LEN;
		}
	}
	void setPrivProto(const std::string &privProto) {
		if (boost::iequals(privProto, PRIV_DES)) {
			_privProto = snmp_duplicate_objid(usmDESPrivProtocol, USM_PRIV_PROTO_DES_LEN);
			_privProtoLen = USM_PRIV_PROTO_DES_LEN;
		} else if (boost::iequals(privProto, PRIV_AES)) {
			_privProto = snmp_duplicate_objid(usmAESPrivProtocol, USM_PRIV_PROTO_AES_LEN);
			_privProtoLen = USM_PRIV_PROTO_AES_LEN;
		}
	}
	void setHost(const std::string &host) { _name = host; }
	void setSecurityLevel(const std::string &securityLevel) {
		if (boost::iequals(securityLevel, SEC_NOAUTHNOPRIV)) {
			_securityLevel = SNMP_SEC_LEVEL_NOAUTH;
		} else if (boost::iequals(securityLevel, SEC_AUTHNOPRIV)) {
			_securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
		} else if (boost::iequals(securityLevel, SEC_AUTHPRIV)) {
			_securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;
		}
	}
	void setVersion(const std::string &version) {
		int vers = stoi(version);
		switch (vers) {
			case 1:
				_version = SNMP_VERSION_1;
				break;
			case 2:
				_version = SNMP_VERSION_2c;
				break;
			case 3:
				_version = SNMP_VERSION_3;
				break;
			default:
				_version = -1;
		}
	}

	const std::string &getSNMPCommunity() const { return _snmpCommunity; }
	const std::string &getOIDPrefix() const { return _oidPrefix; }
	const std::string &getUsername() const { return _username; }
	const std::string &getAuthKey() const { return _authKey; }
	const std::string &getPrivKey() const { return _privKey; }
	uint64_t           getAuthProto() const { return *_authProto; }
	size_t             getAuthProtoLen() const { return _authProtoLen; }
	const std::string  getAuthProtoString() const {
                if (_authProto == usmHMACMD5AuthProtocol) {
                        return std::string(AUTH_MD5);
                } else if (_authProto == usmHMACSHA1AuthProtocol) {
                        return std::string(AUTH_SHA1);
                } else {
                        return std::string("unknown");
                }
	}
	uint64_t          getPrivProto() const { return *_privProto; }
	size_t            getPrivProtoLen() const { return _privProtoLen; }
	const std::string getPrivProtoString() const {
		if (std::memcmp(_privProto, usmDESPrivProtocol, _privProtoLen * sizeof(oid)) == 0) {
			return std::string(PRIV_DES);
		} else if (std::memcmp(_privProto, usmAESPrivProtocol, _privProtoLen * sizeof(oid)) == 0) {
			return std::string(PRIV_AES);
		} else {
			return std::string("unknown");
		}
	}
	const std::string &getHost() const { return _name; }
	int                getSecurityLevel() const { return _securityLevel; }
	const std::string  getSecurityLevelString() const {
                if (_securityLevel == SNMP_SEC_LEVEL_NOAUTH) {
                        return std::string(SEC_NOAUTHNOPRIV);
                } else if (_securityLevel == SNMP_SEC_LEVEL_AUTHNOPRIV) {
                        return std::string(SEC_AUTHNOPRIV);
                } else if (_securityLevel == SNMP_SEC_LEVEL_AUTHPRIV) {
                        return std::string(SEC_AUTHPRIV);
                } else {
                        return std::string("unknown");
                }
	}
	long int getVersion() const { return _version; }

	void printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) override;

	/**
	 * Initializes the connection. Must be called once before the connection can be used
	 */
	void execOnInit() override;

	/**
	 *  Open SNMP Session. Must be called once before issuing a get() call.
	 */
	bool open();

	/**
	 *  Close SNMP Session.
	 */
	void close();

	/**
	 * Issues a get request to _host for the value specified by the OID.
	 *
	 * @param OID		Pointer to the OID buffer which identifies the value to request
	 * @param OIDLen	Lenght of the OID buffer
	 *
	 * @return		The requested value
	 */
	int64_t get(const oid *const OID, size_t OIDLen);

      protected:
	std::string         _snmpCommunity;
	std::string         _oidPrefix;
	std::string         _username;
	std::string         _authKey;
	std::string         _privKey;
	oid *               _authProto;
	oid *               _privProto;
	size_t              _authProtoLen;
	size_t              _privProtoLen;
	int                 _securityLevel;
	long int            _version;
	struct snmp_session _snmpSession;
	void *              _snmpSessp;
};

#endif /* SNMPCONNECTION_H_ */
