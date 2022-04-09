//================================================================================
// Name        : TesterOperator.cpp
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

#include "TesterOperator.h"

TesterOperator::TesterOperator(const std::string& name) : OperatorTemplate(name) { 
    _window = 0;
    _numQueries = 1;
    _relative = true;
}

TesterOperator::TesterOperator(const TesterOperator& other) : OperatorTemplate(other) {
    _numQueries = other._numQueries;
    _window = other._window;
    _relative = other._relative;
}

TesterOperator::~TesterOperator() {}

void TesterOperator::printConfig(LOG_LEVEL ll) {
    LOG_VAR(ll) << "            Window:          " << _window;
    LOG_VAR(ll) << "            Queries:         " << _numQueries;
    LOG_VAR(ll) << "            Relative mode:   " << (_relative ? "enabled" : "disabled");
    OperatorTemplate<SensorBase>::printConfig(ll);
}

void TesterOperator::compute(U_Ptr unit) {
    if(unit->isTopUnit()) {
        reading_t outR;
        outR.timestamp = getTimestamp();
        uint64_t globCtr=0;
        for(const auto& subUnit : unit->getSubUnits())
            globCtr += compute_internal(subUnit);
        outR.value = (int64_t)globCtr;
        unit->getOutputs()[0]->storeReading(outR);
    } else
        compute_internal(unit);
}

uint64_t TesterOperator::compute_internal(U_Ptr unit) {
    bool errorLog=false;
    reading_t outR;
    outR.timestamp = getTimestamp();
    uint64_t elCtr=0, queryCtr=0;
    uint64_t startTs = _relative ? _window : outR.timestamp-_window-TESTERAN_OFFSET;
    uint64_t endTs   = _relative ? 0 : outR.timestamp-TESTERAN_OFFSET;
    // Looping to the desired number of queries
    while(queryCtr < _numQueries) {
        for (const auto &in : unit->getInputs()) {
            // Clearing the buffer
            _buffer.clear();
            if (!_queryEngine.querySensor(in->getName(), startTs, endTs, _buffer, _relative) || _buffer.empty())
                errorLog = true;
            else
                elCtr += _buffer.size();
            if(++queryCtr >= _numQueries)
                break;
        }
    }
    if(errorLog)
        LOG(debug) << "Operator " << _name << ": could not read from one or more sensors!";
    outR.value = (int64_t)elCtr;
    unit->getOutputs()[0]->storeReading(outR);
    return elCtr;
}
