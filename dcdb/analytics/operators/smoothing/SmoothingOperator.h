//================================================================================
// Name        : SmoothingOperator.h
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

#ifndef PROJECT_SMOOTHINGOPERATOR_H
#define PROJECT_SMOOTHINGOPERATOR_H


#include "../../includes/OperatorTemplate.h"
#include "SmoothingSensorBase.h"

/**
 * @brief Smoothing operator plugin.
 *
 * @ingroup smoothing
 */
class SmoothingOperator : virtual public OperatorTemplate<SmoothingSensorBase> {

public:

    SmoothingOperator(const std::string& name);
    SmoothingOperator(const SmoothingOperator& other);

    virtual ~SmoothingOperator();
    
    void setSeparator(std::string& s)               { _separator = s; }
    void setExclude(std::string& e)                 { _exclude = e; }
    
    std::string getSeparator()                      { return _separator; }
    std::string getExclude()                        { return _exclude; }
    
    void printConfig(LOG_LEVEL ll) override;
    
    bool execOnStart() override;
    
protected:

    virtual void compute(U_Ptr unit)	 override;
    
    vector<reading_t> _buffer;
    std::string _separator;
    std::string _exclude;
    
};


#endif //PROJECT_SMOOTHINGOPERATOR_H
