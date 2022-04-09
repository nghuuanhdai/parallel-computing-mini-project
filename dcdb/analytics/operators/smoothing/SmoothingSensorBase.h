//================================================================================
// Name        : SmoothingSensorBase.h
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

#ifndef PROJECT_SMOOTHINGSENSORBASE_H
#define PROJECT_SMOOTHINGSENSORBASE_H

#include "sensorbase.h"

/**
 * @brief Sensor base for smoothing plugin
 *
 * @ingroup smoothing
 */
class SmoothingSensorBase : public SensorBase {

public:
    
    // Constructor and destructor
    SmoothingSensorBase(const std::string& name) : SensorBase(name) {
        _range = 300000000000;
        _currValue = 0.0;
        _currTs = 0;
        _last = 0;
        // Declaring the sensor as an operation by default
        SensorMetadata sm;
        sm.setIsOperation(true);
        setMetadata(sm);
    }

    // Copy constructor
    SmoothingSensorBase(SmoothingSensorBase& other) : SensorBase(other) {
        _range = other._range;
        _currValue = 0.0;
        _currTs = 0;
        _last = 0;
    }

    virtual ~SmoothingSensorBase() {}
    
    void setRange(uint64_t r)                   { _range = r * 1000000; }
    void setTimestamp(uint64_t t)               { _currTs = t; }
    void setValue(double v)                     { _currValue = v; }
    
    uint64_t getRange()                         { return _range / 1000000; }
    uint64_t getTimestamp()                     { return _currTs; }
    double getValue()                           { return _currValue; }
    
    void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {
        SensorBase::printConfig(ll, lg, leadingSpaces);
        std::string leading(leadingSpaces, ' ');
        LOG_VAR(ll) << leading << "    Range: " << _range;
    }
    
    void smoothAndStore(reading_t r) {
        double weight = (double)(r.timestamp - _currTs) / (double)_range;
        if (weight > 1.0) {
            weight = 1.0;
        } else if (weight < 0.0) {
            weight = 0.0;
        }
        _currValue = _last ? (_currValue * (1.0 - weight) + (double)r.value * weight) : (double)r.value;
        _currTs = r.timestamp;
        if(r.timestamp - _last > _range) {
            _last = r.timestamp;
            reading_t newVal;
            newVal.timestamp = r.timestamp;
            newVal.value = (uint64_t) _currValue;
            storeReading(newVal);
        }
    }

protected:
    
    uint64_t _range;
    uint64_t _last;
    uint64_t _currTs;
    double   _currValue;
};

using SmoothingSBPtr = std::shared_ptr<SmoothingSensorBase>;

#endif //PROJECT_SMOOTHINGSENSORBASE_H
