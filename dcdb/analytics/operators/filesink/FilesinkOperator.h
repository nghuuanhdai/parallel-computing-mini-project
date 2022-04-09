//================================================================================
// Name        : FilesinkOperator.h
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

#ifndef PROJECT_FILESINKOPERATOR_H
#define PROJECT_FILESINKOPERATOR_H


#include "../../includes/OperatorTemplate.h"
#include "FilesinkSensorBase.h"
#include <math.h>
#include <algorithm>

/**
 * @brief Filesink operator plugin.
 *
 * @ingroup filesink
 */
class FilesinkOperator : virtual public OperatorTemplate<FilesinkSensorBase> {

public:

    FilesinkOperator(const std::string& name);
    FilesinkOperator(const FilesinkOperator& other);

    virtual ~FilesinkOperator();
    
    void setAutoName(bool a)                { _autoName = a; }
    
    bool getAutoName()                      { return _autoName; }
    
    void printConfig(LOG_LEVEL ll) override;
    void execOnStop() override;

protected:

    virtual void compute(U_Ptr unit)	 override;
    std::string adjustPath(FilesinkSBPtr s);
    
    bool _autoName;
    vector<reading_t> _buffer;
};


#endif //PROJECT_FILESINKOPERATOR_H
