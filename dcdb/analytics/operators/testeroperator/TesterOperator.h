//================================================================================
// Name        : TesterOperator.h
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

#ifndef PROJECT_TESTEROPERATOR_H
#define PROJECT_TESTEROPERATOR_H

#include "../../includes/OperatorTemplate.h"
#include "sensorbase.h"

#define TESTERAN_OFFSET 1000000000

/**
 * @brief Tester operator plugin.
 *
 * @ingroup testeroperator
 */
class TesterOperator : virtual public OperatorTemplate<SensorBase> {

public:

    TesterOperator(const std::string& name);
    TesterOperator(const TesterOperator& other);
    
    virtual ~TesterOperator();

    void setWindow(unsigned long long w)        { _window = w; }
    void setNumQueries(unsigned long long q)    { _numQueries = q;}
    void setRelative(bool r)                    { _relative = r; }
    
    unsigned long long getWindow()              { return _window; }
    unsigned long long getNumQueries()          { return _numQueries; }
    bool getRelative()                          { return _relative; }
    
    void printConfig(LOG_LEVEL ll) override;

protected:

    virtual void compute(U_Ptr unit)	 override;
    uint64_t compute_internal(U_Ptr unit);
    
    vector<reading_t> _buffer;
    unsigned long long _window;
    unsigned long long _numQueries;
    bool _relative;
    
};

#endif //PROJECT_AGGREGATOROPERATOR_H
