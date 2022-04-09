//================================================================================
// Name        : AggregatorOperator.cpp
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

#include "AggregatorOperator.h"
#include "../../includes/CommonStatistics.h"

AggregatorOperator::AggregatorOperator(const std::string& name) : OperatorTemplate(name) { 
    _window = 0;
    _goBack = 0;
    _relative = true;
}

AggregatorOperator::AggregatorOperator(const AggregatorOperator& other) : OperatorTemplate(other) {
    _window = other._window;
    _goBack = other._goBack;
    _relative = other._relative;
}

AggregatorOperator::~AggregatorOperator() {}

void AggregatorOperator::printConfig(LOG_LEVEL ll) {
    LOG_VAR(ll) << "            Window:          " << _window;
    LOG_VAR(ll) << "            Go Back:         " << _goBack;
    LOG_VAR(ll) << "            Relative mode:   " << (_relative ? "enabled" : "disabled");
    OperatorTemplate<AggregatorSensorBase>::printConfig(ll);
}

void AggregatorOperator::compute(U_Ptr unit) {
    uint64_t startTs=0, endTs=0, now=getTimestamp();
    startTs = _relative ? (_window + _goBack) : (now - _window - _goBack);
    endTs   = _relative ? (_goBack) : (now - _goBack);
    // Clearing the buffer
    _buffer.clear();
    std::vector<std::string> sensorNames;
    for(const auto& in : unit->getInputs()) {
        sensorNames.push_back(in->getName());
    }
    if(!_queryEngine.querySensor(sensorNames, startTs, endTs, _buffer, _relative)) {
        LOG(debug) << "Operator " + _name + ": cannot read from any sensor for unit " + unit->getName() + "!";
        return;
    }
    compute_internal(unit, _buffer);
}

void AggregatorOperator::compute_internal(U_Ptr unit, vector<reading_t>& buffer) {
    _percentileSensors.clear();
    _percentiles.clear();
    reading_t reading;
    AggregatorSensorBase::aggregationOps_t op;
    reading.timestamp = getTimestamp() - _goBack;
    // Performing the actual aggregation operation
    for(const auto& out : unit->getOutputs()) {
        op = out->getOperation();
        if(op!=AggregatorSensorBase::QTL) {
            switch (op) {
                case AggregatorSensorBase::SUM:
                    reading.value = computeSum(buffer);
                    break;
                case AggregatorSensorBase::AVG:
                    reading.value = computeAvg(buffer);
                    break;
                case AggregatorSensorBase::MIN:
                    reading.value = computeMin(buffer);
                    break;
                case AggregatorSensorBase::MAX:
                    reading.value = computeMax(buffer);
                    break;
                case AggregatorSensorBase::STD:
                    reading.value = computeStd(buffer);
                    break;
                case AggregatorSensorBase::OBS:
                    reading.value = computeObs(buffer);
                    break;
                default:
                    LOG(warning) << _name << ": Encountered unknown operation!";
                    reading.value = 0;
                    break;
            }
            out->storeReading(reading);
        } else {
            _percentileSensors.push_back(out);
            _percentiles.push_back(out->getPercentile());
        }
    }
    if(!_percentileSensors.empty()) {
        computePercentiles(buffer, _percentiles, _percentileResult);
        for(unsigned idx=0; idx<_percentileResult.size(); idx++) {
            reading.value = _percentileResult[idx];
            _percentileSensors[idx]->storeReading(reading);
        }
    }
}
