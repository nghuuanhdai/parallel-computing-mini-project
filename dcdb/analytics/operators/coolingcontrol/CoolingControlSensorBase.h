//================================================================================
// Name        : CoolingControlSensorBase.h
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
 * @defgroup coolingcontrol Cooling Control operator plugin.
 * @ingroup operator
 *
 * @brief Cooling Control operator plugin.
 */

#ifndef PROJECT_COOLINGCONTROLSENSORBASE_H
#define PROJECT_COOLINGCONTROLSENSORBASE_H

#include "../../../dcdbpusher/sensors/snmp/SNMPSensorBase.h"

/**
 * @brief Sensor base for Cooling Control plugin
 *
 * @ingroup coolingcontrol
 */
class CoolingControlSensorBase : public SNMPSensorBase {

public:

    // Constructor and destructor
    CoolingControlSensorBase(const std::string& name) : SNMPSensorBase(name) {
        _hotThreshold = 70;
        _critThreshold = 0;
    }

    // Copy constructor
    CoolingControlSensorBase(CoolingControlSensorBase& other) : SNMPSensorBase(other) {
        _hotThreshold = other._hotThreshold;
        _critThreshold = other._critThreshold;
    }

    virtual ~CoolingControlSensorBase() {}

    void setHotThreshold(uint64_t t)        { _hotThreshold = t; }
    void setCriticalThreshold(uint64_t t)   { _critThreshold = t; }

    uint64_t getHotThreshold()              { return _hotThreshold; }
    uint64_t getCriticalThreshold()         { return _critThreshold; }

    void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {
        SNMPSensorBase::printConfig(ll, lg, leadingSpaces);
        std::string leading(leadingSpaces, ' ');
        LOG_VAR(ll) << leading << "    Hot Threshold:     " << _hotThreshold;
        if( _critThreshold != 0 ) {
            LOG_VAR(ll) << leading << "    Crit Threshold:    " << _critThreshold;
        }
    }

protected:

    uint64_t _hotThreshold;
    uint64_t _critThreshold;

};

using CCSBPtr = std::shared_ptr<CoolingControlSensorBase>;

#endif //PROJECT_COOLINGCONTROLSENSORBASE_H

