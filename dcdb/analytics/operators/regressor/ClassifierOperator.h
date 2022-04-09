//================================================================================
// Name        : ClassifierOperator.h
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

#ifndef PROJECT_CLASSIFIEROPERATOR_H
#define PROJECT_CLASSIFIEROPERATOR_H


#include "RegressorOperator.h"

/**
 * @brief Classifier operator plugin.
 *
 * @ingroup classifier
 */
class ClassifierOperator : virtual public RegressorOperator {

public:

    ClassifierOperator(const std::string& name);
    ClassifierOperator(const ClassifierOperator& other);

    virtual ~ClassifierOperator();
    
    void printConfig(LOG_LEVEL ll) override;

protected:

    void compute(U_Ptr unit)	 override;

    int _currentClass;

};

#endif //PROJECT_CLASSIFIEROPERATOR_H