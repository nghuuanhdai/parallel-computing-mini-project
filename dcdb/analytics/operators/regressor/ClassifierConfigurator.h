//================================================================================
// Name        : ClassifierConfigurator.h
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

#ifndef PROJECT_CLASSIFIERCONFIGURATOR_H
#define PROJECT_CLASSIFIERCONFIGURATOR_H

#include "../../includes/OperatorConfiguratorTemplate.h"
#include "ClassifierOperator.h"

/**
 * @brief Configurator for the classifier plugin.
 *
 * @ingroup classifier
 */
class ClassifierConfigurator : virtual public OperatorConfiguratorTemplate<ClassifierOperator, RegressorSensorBase> {

public:
    ClassifierConfigurator();
    virtual ~ClassifierConfigurator();

private:
    void sensorBase(RegressorSensorBase& s, CFG_VAL config) override;
    void operatorAttributes(ClassifierOperator& op, CFG_VAL config) override;
    bool unit(UnitTemplate<RegressorSensorBase>& u) override;

};

extern "C" OperatorConfiguratorInterface* create() {
    return new ClassifierConfigurator;
}

extern "C" void destroy(OperatorConfiguratorInterface* c) {
    delete c;
}


#endif //PROJECT_CLASSIFIERCONFIGURATOR_H
