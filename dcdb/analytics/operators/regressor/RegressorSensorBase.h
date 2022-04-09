//================================================================================
// Name        : RegressorSensorBase.h
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
 * @defgroup regressor Regressor operator plugin.
 * @ingroup operator
 *
 * @brief Regressor operator plugin.
 */

#ifndef PROJECT_REGRESSORSENSORBASE_H
#define PROJECT_REGRESSORSENSORBASE_H

#include "sensorbase.h"

/**
 * @brief Sensor base for regressor plugin
 *
 * @ingroup regressor
 */
class RegressorSensorBase : public SensorBase {

public:
    
    // Constructor and destructor
    RegressorSensorBase(const std::string& name) : SensorBase(name) {
        _trainingTarget = false;
    }

    // Copy constructor
    RegressorSensorBase(RegressorSensorBase& other) : SensorBase(other) {
        _trainingTarget = other._trainingTarget;
    }

    virtual ~RegressorSensorBase() {}

    void setTrainingTarget(bool t)      { _trainingTarget=t; }

    bool getTrainingTarget()             { return _trainingTarget; }

    void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {
        SensorBase::printConfig(ll, lg, leadingSpaces);
        std::string leading(leadingSpaces, ' ');
        //LOG_VAR(ll) << leading << "    Training target: " << (_trainingTarget ? std::string("on") : std::string("off"));
    }

protected:
    
    bool _trainingTarget;
};

using RegressorSBPtr = std::shared_ptr<RegressorSensorBase>;

#endif //PROJECT_REGRESSORSENSORBASE_H
