//================================================================================
// Name        : PerSystSqlConfigurator.h
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Template implementing features to use Units in Operators.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
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

#ifndef ANALYTICS_OPERATORS_PERSYSTSQL_PERSYSTSQLCONFIGURATOR_H_
#define ANALYTICS_OPERATORS_PERSYSTSQL_PERSYSTSQLCONFIGURATOR_H_

#include "../../includes/JobOperatorConfiguratorTemplate.h"
#include "../../includes/UnitTemplate.h"
#include "../aggregator/AggregatorSensorBase.h"
#include "../aggregator/JobAggregatorOperator.h"
#include "../smucngperf/SMUCSensorBase.h"
#include "PerSystSqlOperator.h"

class PerSystSqlConfigurator: public JobOperatorConfiguratorTemplate<PerSystSqlOperator, AggregatorSensorBase>  {
public:
	PerSystSqlConfigurator();
	virtual ~PerSystSqlConfigurator();
private:
	std::map<std::string, MariaDB::Rotation_t> _rotation_map;
    void sensorBase(AggregatorSensorBase& s, CFG_VAL config) override;
    void operatorAttributes(PerSystSqlOperator& op, CFG_VAL config) override;
    bool unit(UnitTemplate<AggregatorSensorBase>& u) override;
    bool readUnits(PerSystSqlOperator& op, std::vector<shared_ptr<AggregatorSensorBase>>& protoInputs, std::vector<shared_ptr<AggregatorSensorBase>>& protoOutputs, 
            std::vector<shared_ptr<AggregatorSensorBase>>& protoGlobalOutputs, inputMode_t inputMode) override;
};

extern "C" OperatorConfiguratorInterface* create() {
    return new PerSystSqlConfigurator;
}

extern "C" void destroy(OperatorConfiguratorInterface* c) {
    delete c;
}

#endif /* ANALYTICS_OPERATORS_PERSYSTSQL_PERSYSTSQLCONFIGURATOR_H_ */
