//================================================================================
// Name        : SmoothingOperator.cpp
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

#include "SmoothingOperator.h"

SmoothingOperator::SmoothingOperator(const std::string& name) : OperatorTemplate(name) {
    _separator = "-";
    _exclude = "";
}

SmoothingOperator::SmoothingOperator(const SmoothingOperator &other) : OperatorTemplate(other) {
    _separator = other._separator;
    _exclude = "";
}

SmoothingOperator::~SmoothingOperator() {}

void SmoothingOperator::printConfig(LOG_LEVEL ll) {
    OperatorTemplate<SmoothingSensorBase>::printConfig(ll);
}

void SmoothingOperator::compute(U_Ptr unit) {
    // Clearing the buffer
    _buffer.clear();
    SmoothingSBPtr sIn=unit->getInputs()[0], sOut=unit->getOutputs()[0];
    uint64_t endTs = getTimestamp();
    uint64_t startTs = sOut->getTimestamp() ? sOut->getTimestamp() : endTs;

    // Throwing an error does not make sense here - the query will often fail depending on insert batching
    if(!_queryEngine.querySensor(sIn->getName(), startTs, endTs, _buffer, false))
        return;
    
    for(const auto& v : _buffer) {
        if (v.timestamp > sOut->getTimestamp()) {
            for(const auto& s : unit->getOutputs())
                s->smoothAndStore(v);
        } else {
            continue;
        }
    }
}

bool SmoothingOperator::execOnStart() {
    // Setting the reference initial timestamp
    for(const auto& u : _units)
        for(const auto& s : u->getOutputs()) {
            s->setTimestamp(0);
            s->setValue(0.0);
        }
        
    return true;
}
