//================================================================================
// Name        : SNMPController.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description :
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
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

#ifndef PROJECT_SNMPCONTROLLER_H
#define PROJECT_SNMPCONTROLLER_H

#include "../../../dcdbpusher/sensors/snmp/SNMPConnection.h"

class SNMPController : public SNMPConnection {

public:

    SNMPController(const std::string &name = "SNMPCont") : SNMPConnection(name) {}
    SNMPController(const SNMPConnection &other) : SNMPConnection(other) {}
    SNMPController &operator=(const SNMPConnection &other) { SNMPConnection::operator=(other); return *this; }
    
    virtual ~SNMPController() {}

    void set(const oid *const OID, size_t OIDLen, unsigned char type, const void* const val, size_t valLen);
    
    const std::string &getOIDSuffix()      { return _oidSuffix; }

    void setOIDSuffix(const std::string &oidSuffix) {
        _oidSuffix = oidSuffix;
        if(_oidSuffix.length() > 0) {
            if (_oidSuffix[0]!='.') {
                _oidSuffix = '.' + _oidSuffix;
            }
            if (_oidSuffix.at(_oidSuffix.length() - 1) == '.') {
                _oidSuffix.resize(_oidSuffix.size() - 1);
            }
        }
    }

    void printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) override;
    
protected:
    
    std::string _oidSuffix;
};


#endif //PROJECT_SNMPCONTROLLER_H
