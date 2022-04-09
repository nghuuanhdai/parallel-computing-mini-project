//================================================================================
// Name        : AggregatorSensorBase.h
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

/**
 * @defgroup aggregator Aggregator operator plugin.
 * @ingroup operator
 *
 * @brief Aggregator operator plugin.
 */

#ifndef PROJECT_AGGREGATORSENSORBASE_H
#define PROJECT_AGGREGATORSENSORBASE_H

#include "sensorbase.h"

/**
 * @brief Sensor base for aggregator plugin
 *
 * @ingroup aggregator
 */
class AggregatorSensorBase : public SensorBase {

public:
    
    // Enum to identify aggregation operations
    enum aggregationOps_t { SUM = 0, AVG = 1, MAX = 2, MIN = 3, STD = 4, QTL = 5, OBS = 6, AVG_SEV = 7 };
    
    // Constructor and destructor
    AggregatorSensorBase(const std::string& name) : SensorBase(name) {
        _opType = SUM;
        _percentile = 50;
    }
    
    // Copy constructor
    AggregatorSensorBase(AggregatorSensorBase& other) : SensorBase(other) {
        _opType = other._opType;
        _percentile = other._percentile;
    }

    virtual ~AggregatorSensorBase() {}

    void setOperation(aggregationOps_t op)      { _opType = op; }
    void setPercentile(size_t q)                  { _percentile = q; }
    
    aggregationOps_t getOperation()             { return _opType; }
    size_t getPercentile()                        { return _percentile; }

    void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {
        SensorBase::printConfig(ll, lg, leadingSpaces);
        std::string leading(leadingSpaces, ' ');
        LOG_VAR(ll) << leading << "    Operation: " << getOpString(_opType);
        if(_opType==QTL)
            LOG_VAR(ll) << leading << "    Percentile: " << _percentile;
    }
    
protected:

    std::string getOpString(aggregationOps_t op) {
        std::string opString;
        switch(op) {
            case SUM:
                opString = "sum";
                break;
            case MAX:
                opString = "maximum";
                break;
            case MIN:
                opString = "minimum";
                break;
            case AVG:
                opString = "average";
                break;
            case STD:
                opString = "std";
                break;
            case QTL:
                opString = "percentiles";
                break;
            case OBS:
                opString = "observations";
                break;
            default:
                opString = "invalid";
                break;
        }
        return opString;
    }
    
    aggregationOps_t _opType;
    size_t _percentile;
};

using AggregatorSBPtr = std::shared_ptr<AggregatorSensorBase>;

#endif //PROJECT_AGGREGATORSENSORBASE_H
