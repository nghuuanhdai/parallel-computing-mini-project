//================================================================================
// Name        : SNMPController.cpp
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

#include "SNMPController.h"

void SNMPController::set(const oid *const OID, size_t OIDLen, unsigned char type, const void* const val, size_t valLen) {
    struct snmp_pdu *pdu, *response;
    struct variable_list *vp;
    int                   status;

    if (!_snmpSessp) {
        open();
    }

    pdu = snmp_pdu_create(SNMP_MSG_SET);
    snmp_pdu_add_variable(pdu, OID, OIDLen, type, val, valLen);
    
    status = snmp_sess_synch_response(_snmpSessp, pdu, &response);

    if( status != STAT_SUCCESS || response->errstat != SNMP_ERR_NOERROR ) {
        if (status == STAT_SUCCESS) {
            LOG(error) << "Error in packet: " << snmp_errstring(response->errstat);
        } else {
            char *err;
            snmp_error(&_snmpSession, NULL, NULL, &err);
            LOG(error) << "SNMP-set: " << err;
            free(err);
        }

        throw std::runtime_error("Request failed!");
    }

    if (response) {
        snmp_free_pdu(response);
    }
}

void SNMPController::printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
    std::string leading(leadingSpaces, ' ');
    LOG_VAR(ll) << leading << "OIDSuffix:    " << getOIDSuffix();
    SNMPConnection::printEntityConfig(ll, leadingSpaces);
}