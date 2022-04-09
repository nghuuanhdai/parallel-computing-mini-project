//================================================================================
// Name        : CoolingControlOperator.cpp
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

#include "CoolingControlOperator.h"

CoolingControlOperator::CoolingControlOperator(const std::string& name) : OperatorTemplate(name) {
    _strategy = "stepped";
    _maxTemp = 450;
    _minTemp = 350;
    _window = 0;
    _bins = 4;
    _hotPerc = 20;
    _currTemp = 0;
}

CoolingControlOperator::CoolingControlOperator(const CoolingControlOperator& other) : OperatorTemplate(other) {
    _strategy = other._strategy;
    _maxTemp = other._maxTemp;
    _minTemp = other._minTemp;
    _window = other._window;
    _bins = other._bins;
    _hotPerc = other._hotPerc;
    _currTemp = 0;
}

CoolingControlOperator::~CoolingControlOperator() {}

void CoolingControlOperator::printConfig(LOG_LEVEL ll) {
    LOG_VAR(ll) << "            Strategy:        " << _strategy;
    LOG_VAR(ll) << "            Max Temperature: " << _maxTemp;
    LOG_VAR(ll) << "            Min Temperature: " << _minTemp;
    LOG_VAR(ll) << "            Window:          " << _window;
    LOG_VAR(ll) << "            Bins:            " << _bins;
    LOG_VAR(ll) << "            Hot Percentage:  " << _hotPerc;
    _controller.printEntityConfig(ll, 12);
    OperatorTemplate<CoolingControlSensorBase>::printConfig(ll);
}

restResponse_t CoolingControlOperator::REST(const string& action, const unordered_map<string, string>& queries) {
    restResponse_t resp;
    if(action=="status") {
        resp.response = "Using cooling control strategy " + _strategy + " with temperature " + std::to_string(_currTemp) + ".\n";
    } else
        throw invalid_argument("Unknown plugin action " + action + " requested!");
    return resp;
}

void CoolingControlOperator::execOnInit() {
    _controller.execOnInit();
    _dummySensor.setOID(_controller.getOIDPrefix() + _controller.getOIDSuffix());
}

bool CoolingControlOperator::execOnStart() {
    // Resetting the internal temperature setting at each plugin start
    _currTemp = 0;
    return true;
}

void CoolingControlOperator::compute(U_Ptr unit) {
    // Querying input data
    std::vector<std::vector<reading_t>> buffer;
    for(const auto& in : unit->getInputs()) {
        std::vector<reading_t> bufferInt;
        if (!_queryEngine.querySensor(in->getName(), _window, 0, bufferInt, true) || bufferInt.empty()) {
            LOG(debug) << "Operator " + _name + ": cannot read from sensor " + in->getName() + "!";
        }
        buffer.push_back(bufferInt);
    }
    
    // Establishing the new setting - needs to be a 32-bit signed integer
    int newSetting=-1;
    if( _strategy == "continuous" ) {
        newSetting = continuousControl(buffer, unit);
    } else if( _strategy == "stepped") {
        newSetting = steppedControl(buffer, unit);
    }
    
    // LOG(debug) << "New setting: " << newSetting;
    // Enacting control
    if( newSetting >= 0 && _controller.open() ) {
        try {
            _controller.set(_dummySensor.getOID(), _dummySensor.getOIDLen(), ASN_INTEGER, (void *) &newSetting, sizeof(newSetting));
        } catch (const std::runtime_error& e) {
            LOG(error) << "Operator " << _name << ": SNMP set request failed!";
        }
        _controller.close();
    }
    
    return;
}

int CoolingControlOperator::continuousControl(std::vector<std::vector<reading_t>> &readings, U_Ptr unit) {
    if( _currTemp == 0 ) {
        _currTemp = (_maxTemp + _minTemp) / 2;
    } else {
        // If there are less hot nodes than our hot threshold, we increase the inlet temperature - and vice versa
        uint64_t percHotNodes =  (getNumHotNodes(readings, unit) * 100) / readings.size();
        _currTemp = (int64_t)_currTemp + ((int64_t)_hotPerc - (int64_t)percHotNodes) * ((int64_t)_maxTemp - (int64_t)_minTemp) / 100;
        _currTemp = clipTemperature(_currTemp);
    }
    return (int)_currTemp;
}

int CoolingControlOperator::steppedControl(std::vector<std::vector<reading_t>> &readings, U_Ptr unit) {
    uint64_t oldTemp = _currTemp;
    int ret = -1;
    if(_currTemp == 0) {
        _currTemp = (_maxTemp + _minTemp) / 2;
        return _currTemp;
    } else {
        // If there are less hot nodes than our hot threshold, we increase the inlet temperature - and vice versa
        uint64_t percHotNodes =  (getNumHotNodes(readings, unit) * 100) / readings.size();
        _currTemp = (int64_t)_currTemp + ((int64_t)_hotPerc - (int64_t)percHotNodes) * ((int64_t)_maxTemp - (int64_t)_minTemp) / 100;
        _currTemp = clipTemperature(_currTemp);
        // We're crossing to a new bin - assigning the new setting
        if(getBinForValue(oldTemp) != getBinForValue(_currTemp)) {
            uint64_t binStep = (_maxTemp - _minTemp) / _bins;
            _currTemp = _minTemp + getBinForValue(_currTemp) * binStep + binStep / 2;
            ret = (int)_currTemp;
        }
    }
    return ret;
}

uint64_t CoolingControlOperator::getNumHotNodes(std::vector<std::vector<reading_t>> &readings, U_Ptr unit) {
    uint64_t numHotNodes = 0;
    for(size_t idx=0; idx<readings.size(); idx++) {
        if (!readings[idx].empty()) {
            bool hotNode = true;
            for (const auto &r : readings[idx]) {
                // If a single reading in a single component exceeds it critical temperature,
                // we immediately trigger a steep cooling temperature decrease by counting all components as hot
                if (unit->getInputs()[idx]->getCriticalThreshold() != 0 && r.value >= (int64_t)unit->getInputs()[idx]->getCriticalThreshold()) {
                    return readings.size();
                } else if (r.value < (int64_t)unit->getInputs()[idx]->getHotThreshold()) {
                    hotNode = false;
                    break;
                }
            }
            if (hotNode) {
                numHotNodes++;
            }
        // Missing data automatically results in a hot node being counted, as a failsafe measure
        } else {
            numHotNodes++;
        }
    }
    return numHotNodes;
}

uint64_t CoolingControlOperator::getBinForValue(uint64_t temp) {
    if(temp <= _minTemp) {
        return 0;
    } else if(temp >= _maxTemp) {
        return _bins - 1;
    } else {
        uint64_t binStep = (_maxTemp - _minTemp) / _bins;
        return (temp - _minTemp) / binStep;
    }
}

uint64_t CoolingControlOperator::clipTemperature(uint64_t temp) {
    if(temp > _maxTemp) {
        return _maxTemp;
    } else if (temp < _minTemp) {
        return _minTemp;
    } else {
        return temp;
    }
}
