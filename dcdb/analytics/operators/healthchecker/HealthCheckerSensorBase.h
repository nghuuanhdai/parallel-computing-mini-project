//================================================================================
// Name        : HealthCheckerSensorBase.h
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

#ifndef PROJECT_HEALTHCHECKERSENSORBASE_H
#define PROJECT_HEALTHCHECKERSENSORBASE_H

#include "sensorbase.h"

/**
 * @brief Sensor base for health checker plugin
 *
 * @ingroup healthchecker
 */
class HealthCheckerSensorBase : public SensorBase {

public:

    typedef enum {
        HC_ABOVE,
        HC_BELOW,
        HC_EQUAL,
        HC_EXISTS,
        HC_INVALID
    } HCCond;

    // Constructor and destructor
    HealthCheckerSensorBase(const std::string &name) : SensorBase(name) {
        _last = 0;
        _threshold = 0;
        _condition = HC_INVALID;
    }

    // Copy constructor
    HealthCheckerSensorBase(HealthCheckerSensorBase &other) : SensorBase(other) {
        _last = 0;
        _threshold = other._threshold;
        _condition = other._condition;
    }

    virtual ~HealthCheckerSensorBase() {}

    void setLast(uint64_t l) { _last = l; }
    void setThreshold(int64_t t) { _threshold = t; }
    void setCondition(HCCond c) { _condition = c; }

    uint64_t getLast() { return _last; }
    int64_t getThreshold() { return _threshold; }
    HCCond getCondition() { return _condition; }

    void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
        SensorBase::printConfig(ll, lg, leadingSpaces);
        std::string leading(leadingSpaces, ' ');
        LOG_VAR(ll) << leading << "    Condition: " << condToString(_condition);
        LOG_VAR(ll) << leading << "    Threshold: " << _threshold;
    }

    std::string condToString(HCCond c) {
        switch (c) {
            case HC_ABOVE:
                return "above";
            case HC_BELOW:
                return "below";
            case HC_EQUAL:
                return "equals";
            case HC_EXISTS:
                return "exists";
            default:
                return "invalid";
        }
    }

    HCCond stringToCond(std::string s) {
        if (boost::iequals(s, "above")) {
            return HC_ABOVE;
        } else if (boost::iequals(s, "below")) {
            return HC_BELOW;
        } else if (boost::iequals(s, "equals")) {
            return HC_EQUAL;
        } else if (boost::iequals(s, "exists")) {
            return HC_EXISTS;
        } else {
            return HC_INVALID;
        }
    }
    
protected:
    
    uint64_t _last;
    int64_t  _threshold;
    HCCond   _condition;
    
};

using HealthCheckerSBPtr = std::shared_ptr<HealthCheckerSensorBase>;

#endif //PROJECT_HEALTHCHECKERSENSORBASE_H
