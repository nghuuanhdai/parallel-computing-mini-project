//================================================================================
// Name        : AggregatorConfigurator.h
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

#ifndef PROJECT_AGGREGATORCONFIGURATOR_H
#define PROJECT_AGGREGATORCONFIGURATOR_H

#include "../../includes/OperatorConfiguratorTemplate.h"
#include "AggregatorOperator.h"

/**
 * @brief Configurator for the aggregator plugin.
 *
 * @ingroup aggregator
 */
class AggregatorConfigurator : virtual public OperatorConfiguratorTemplate<AggregatorOperator, AggregatorSensorBase> {

public:
    AggregatorConfigurator();
    virtual ~AggregatorConfigurator();

private:
    void sensorBase(AggregatorSensorBase& s, CFG_VAL config) override;
    void operatorAttributes(AggregatorOperator& op, CFG_VAL config) override;
    bool unit(UnitTemplate<AggregatorSensorBase>& u) override;
    
};

extern "C" OperatorConfiguratorInterface* create() {
    return new AggregatorConfigurator;
}

extern "C" void destroy(OperatorConfiguratorInterface* c) {
    delete c;
}

#endif //PROJECT_AVERAGECONFIGURATOR_H
