//================================================================================
// Name        : CSSensorBase.h
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
 * @defgroup cs-smoothing CS Smoothing operator plugin.
 * @ingroup operator
 *
 * @brief CS Smoothing operator plugin.
 */

#ifndef PROJECT_CSSENSORBASE_H
#define PROJECT_CSSENSORBASE_H

#include "sensorbase.h"

/**
 * @brief Sensor base for CS Smoothing plugin
 *
 * @ingroup cs-smoothing
 */
class CSSensorBase : public SensorBase {

public:

    // Constructor and destructor
    CSSensorBase(const std::string& name) : SensorBase(name) {
        _blockID = 0;
        _imag = false;
    }

    // Copy constructor
    CSSensorBase(CSSensorBase& other) : SensorBase(other) {
        _blockID = other._blockID;
        _imag = other._imag;
    }

    virtual ~CSSensorBase() {}
    
    void setBlockID(uint64_t id)    { _blockID = id; }
    void setImag(bool im)           { _imag = im; }
    
    uint64_t getBlockID()           { return _blockID; }
    bool getImag()                  { return _imag; }

    void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {
        SensorBase::printConfig(ll, lg, leadingSpaces);
        std::string leading(leadingSpaces, ' ');
        LOG_VAR(ll) << leading << "    Block ID:          " << _blockID;
        LOG_VAR(ll) << leading << "    Imaginary:         " << (_imag ? "true" : "false");
    }
    
protected:
    
    uint64_t _blockID;
    bool _imag;

};

using CSSBPtr = std::shared_ptr<CSSensorBase>;

#endif //PROJECT_CSSENSORBASE_H

