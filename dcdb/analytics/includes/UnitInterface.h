//================================================================================
// Name        : UnitInterface.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Interface for Units used by Operators.
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

#ifndef PROJECT_UNITINTERFACE_H
#define PROJECT_UNITINTERFACE_H

#include <vector>
#include "sensorbase.h"

// Defines how inputs must be constructed for the specified unit
typedef enum inputMode_t { SELECTIVE = 1, ALL = 2, ALL_RECURSIVE = 3 } inputMode_t;

/**
 * @brief Interface for Units used by Operators to perform data analytics.
 *
 * @details An Unit represents a logical entity on which an Operator operates,
 *          and is identified by its name, inputs and outputs.
 *
 * @ingroup operator
 */
class UnitInterface {

public:

    /**
    * @brief            Class constructor
    */
    UnitInterface() {}

    /**
    * @brief            Class destructor
    */
    virtual ~UnitInterface() {}

    /**
    * @brief            Initializes the sensors in the unit
    *
    * @param interval   Sampling interval in milliseconds 
    */
    virtual void init(unsigned int interval, unsigned int queueSize) = 0;

    /**
    * @brief            Sets the name of this unit
    *
    * @param name       The name of this unit
    */
    virtual void setName(const std::string& name) = 0;

    /**
    * @brief            Get the name of this unit
    *
    *                   A unit's name points to the logical entity that it represents; for example, it could be
    *                   "hpcsystem1.node44", or "node44.cpu10". All the outputs of the unit are then associated to its
    *                   entity, and all of the input are related to it as well.
    *
    * @return           The unit's name
    */
    virtual std::string& getName() = 0;

    /**
    * @brief            Sets the input mode of this unit
    *
    * @param iMode      The input mode that was used for this unit
    */
    virtual void setInputMode(const inputMode_t iMode) = 0;

    /**
    * @brief            Get the input mode of this unit
    *
    * @return           The unit's input mode
    */
    virtual inputMode_t getInputMode() = 0;

    /**
    * @brief            Get the (base) input sensors of this unit
    *
    * @return           A vector of pointers to SensorBase objects that constitute this unit's input
    */
    virtual std::vector<SBasePtr>& getBaseInputs()  = 0;

    /**
    * @brief            Get the (base) output sensors of this unit
    *
    * @return           A vector of pointers to SensorBase objects that constitute this unit's output
    */
    virtual std::vector<SBasePtr>& getBaseOutputs() = 0;

    /**
    * @brief            Prints the current unit configuration
    *
    * @param ll             Logging level at which the configuration is printed
    * @param lg             Logger object to be used
    * @param leadingSpaces  Number of leading spaces to pre-pend
    */
    virtual void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16)       = 0;

};

//for better readability
using UnitPtr = std::shared_ptr<UnitInterface>;

#endif //PROJECT_UNITINTERFACE_H
