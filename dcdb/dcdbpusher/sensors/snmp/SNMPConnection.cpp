//================================================================================
// Name        : SNMPConnection.cpp
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for SNMPConnection class.
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

#include "SNMPConnection.h"

#include <string>

SNMPConnection::SNMPConnection(const std::string &name)
    : EntityInterface(name),
      _snmpCommunity(""),
      _oidPrefix(""),
      _username(""),
      _authKey(""),
      _privKey(""),
      _authProto(NULL),
      _privProto(NULL),
      _authProtoLen(0),
      _privProtoLen(0),
      _securityLevel(-1),
      _version(-1),
      _snmpSessp(NULL) {
	_snmpSession.peername = NULL;
	_snmpSession.community_len = 0;
	_snmpSession.securityNameLen = 0;
}

SNMPConnection::SNMPConnection(const SNMPConnection &other)
    : EntityInterface(other),
      _snmpCommunity(other._snmpCommunity),
      _oidPrefix(other._oidPrefix),
      _username(other._username),
      _authKey(other._authKey),
      _privKey(other._privKey),
      _authProto(other._authProto),
      _privProto(other._privProto),
      _authProtoLen(other._authProtoLen),
      _privProtoLen(other._privProtoLen),
      _securityLevel(other._securityLevel),
      _version(other._version),
      _snmpSession(other._snmpSession),
      _snmpSessp(NULL) {
}

SNMPConnection::~SNMPConnection() {
	if (_snmpSession.peername) {
		free(_snmpSession.peername);
	}
	if (_version == SNMP_VERSION_2c) {
		if (_snmpSession.community_len) {
			free(_snmpSession.community);
			_snmpSession.community_len = 0;
		}
	} else if (_version == SNMP_VERSION_3) {
		free(_snmpSession.securityName);
	}
}

SNMPConnection &SNMPConnection::operator=(const SNMPConnection &other) {
	EntityInterface::operator=(other);
	_snmpCommunity = other._snmpCommunity;
	_oidPrefix = other._oidPrefix;
	_username = other._username;
	_authKey = other._authKey;
	_privKey = other._privKey;
	_authProto = other._authProto;
	_privProto = other._privProto;
	_authProtoLen = other._authProtoLen;
	_privProtoLen = other._privProtoLen;
	_securityLevel = other._securityLevel;
	_version = other._version;
	_snmpSessp = NULL;
	_snmpSession = other._snmpSession;

	return *this;
}

void SNMPConnection::execOnInit() {
	snmp_sess_init(&_snmpSession);
	int   liberr, syserr;
	char *errstr;
	snmp_error(&_snmpSession, &liberr, &syserr, &errstr);
	if (liberr || syserr) {
		LOG(error) << "SNMP: Error initializing session: " + std::string(errstr);
	}
    free(errstr);
	_snmpSession.version = _version;
	_snmpSession.peername = strdup(_name.c_str());

	if (_version == SNMP_VERSION_2c) {
		_snmpSession.community = (u_char *)strdup(_snmpCommunity.c_str());
		_snmpSession.community_len = _snmpCommunity.length();
	} else if (_version == SNMP_VERSION_3) {
		_snmpSession.community = NULL;
		_snmpSession.community_len = 0;

		/* set the SNMPv3 user name */
		_snmpSession.securityName = strdup(_username.c_str());
		_snmpSession.securityNameLen = _username.length();

		/* set the security level */
		_snmpSession.securityLevel = _securityLevel;

		if (_snmpSession.securityLevel == SNMP_SEC_LEVEL_NOAUTH) {
			// no authentication, no privacy
			// nothing to do
		} else if (_snmpSession.securityLevel == SNMP_SEC_LEVEL_AUTHNOPRIV || _snmpSession.securityLevel == SNMP_SEC_LEVEL_AUTHPRIV) {
			// authentication...
			_snmpSession.securityAuthKeyLen = USM_AUTH_KU_LEN;

			//set the authentication method
			_snmpSession.securityAuthProto = _authProto;
			_snmpSession.securityAuthProtoLen = _authProtoLen; //sizeof(_authProto)/sizeof(oid);

			/* set the authentication key to a hashed version of our
			_authKey (which must be at least 8 characters long) */
			if (generate_Ku(_snmpSession.securityAuthProto, _snmpSession.securityAuthProtoLen,
					(u_char *)_authKey.c_str(), _authKey.length(), _snmpSession.securityAuthKey,
					&_snmpSession.securityAuthKeyLen) != SNMPERR_SUCCESS) {
				LOG(error) << "SNMP: Error generating Ku from authentication key";
			}

			if (_snmpSession.securityLevel == SNMP_SEC_LEVEL_AUTHPRIV) {
				//...with privacy
				_snmpSession.securityPrivKeyLen = USM_PRIV_KU_LEN;

				//set the privacy method (aka encryption)
				_snmpSession.securityPrivProto = _privProto;
				_snmpSession.securityPrivProtoLen = _privProtoLen;

				/* set the privacy key to a hashed version of our
				_privKey (which must be at least 8 characters long) */
				if (generate_Ku(_snmpSession.securityAuthProto, _snmpSession.securityAuthProtoLen,
						(u_char *)_privKey.c_str(), _privKey.length(), _snmpSession.securityPrivKey,
						&_snmpSession.securityPrivKeyLen) != SNMPERR_SUCCESS) {
					LOG(error) << "SNMP: Error generating Ku from privacy key";
				}
			}
		} else {
			LOG(warning) << "SNMP security level unknown!";
		}
	} else {
		LOG(warning) << "SNMP Version " << _version << " not supported!";
	}
}

bool SNMPConnection::open() {
	_snmpSessp = snmp_sess_open(&_snmpSession);
	int   liberr, syserr;
	char *errstr;
	snmp_error(&_snmpSession, &liberr, &syserr, &errstr);
	if (liberr || syserr) {
		LOG(error) << "SNMP-open: " << errstr;
		free(errstr);
		return false;
	}
    free(errstr);
	return true;
}

void SNMPConnection::close() {
	if (_snmpSessp) {
		snmp_sess_close(_snmpSessp);
		_snmpSessp = NULL;
	}
}

int64_t SNMPConnection::get(const oid *const OID, size_t OIDLen) {
	struct snmp_pdu *     pdu, *response;
	struct variable_list *vp;
	int                   status;
	int64_t               ret = 0;

	if (!_snmpSessp) {
		open();
	}

	pdu = snmp_pdu_create(SNMP_MSG_GET);

	snmp_add_null_var(pdu, OID, OIDLen);

	status = snmp_sess_synch_response(_snmpSessp, pdu, &response);

	if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
		vp = response->variables;
		if (vp) {
			if (vp->type == ASN_INTEGER) {
				ret = *vp->val.integer;
			} else if (vp->type == 0x43) { /* Timeticks: No idea why there is no definition for this in the libray */
				ret = *vp->val.integer;
			} else {
				LOG(warning) << "Non-Integer and non-Timetick SNMP data received (type=" << std::to_string((int)vp->type) << "):";
				char buf[1024];
				while (vp) {
					snprint_variable(buf, sizeof(buf), vp->name, vp->name_length, vp);
					LOG(warning) << (&_snmpSession)->peername << ": " << buf;
					vp = vp->next_variable;
				}
			}
		}
	} else {
		if (status == STAT_SUCCESS) {
			LOG(error) << "Error in packet: " << snmp_errstring(response->errstat);
		} else {
			char *err;
			snmp_error(&_snmpSession, NULL, NULL, &err);
			LOG(error) << "SNMP-get: " << err;
			free(err);
		}

		throw std::runtime_error("Request failed!");
	}

	if (response) {
		snmp_free_pdu(response);
	}
	return ret;
}

void SNMPConnection::printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "OIDPrefix:    " << getOIDPrefix();
	std::string version("unknown");
	switch (getVersion()) {
		case SNMP_VERSION_1:
			version = "1";
			break;
		case SNMP_VERSION_2c:
			version = "2c";
			break;
		case SNMP_VERSION_3:
			version = "3";
			break;
	}
	LOG_VAR(ll) << leading << "Version:      " << version;
	if (getVersion() < SNMP_VERSION_3) {
		LOG_VAR(ll) << leading << "Community:    " << getSNMPCommunity();
	} else {
		LOG_VAR(ll) << leading << "Username:     " << getUsername();
		LOG_VAR(ll) << leading << "SecLevel:     " << getSecurityLevelString();
		if (_securityLevel != SNMP_SEC_LEVEL_NOAUTH) {
			LOG_VAR(ll) << leading << "AuthProto:    " << getAuthProtoString();
			LOG_VAR(ll) << leading << "AuthKey:      " << getAuthKey();
		}
		if (_securityLevel == SNMP_SEC_LEVEL_AUTHPRIV) {
			LOG_VAR(ll) << leading << "PrivProto:    " << getPrivProtoString();
			LOG_VAR(ll) << leading << "PrivKey:      " << getPrivKey();
		}
	}
}
