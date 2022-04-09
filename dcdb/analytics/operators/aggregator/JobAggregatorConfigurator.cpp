//================================================================================
// Name        : JobAggregatorConfigurator.cpp
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

#include "JobAggregatorConfigurator.h"

JobAggregatorConfigurator::JobAggregatorConfigurator() : JobOperatorConfiguratorTemplate() {
    _operatorName = "aggregator";
    _baseName     = "sensor";
}

JobAggregatorConfigurator::~JobAggregatorConfigurator() {}

void JobAggregatorConfigurator::sensorBase(AggregatorSensorBase& s, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "operation")) {
            std::string opName = val.second.data();
            if (opName == "sum")
                s.setOperation(AggregatorSensorBase::SUM);
            else if (opName == "average")
                s.setOperation(AggregatorSensorBase::AVG);
            else if (opName == "maximum")
                s.setOperation(AggregatorSensorBase::MAX);
            else if (opName == "minimum")
                s.setOperation(AggregatorSensorBase::MIN);
            else if (opName == "std")
                s.setOperation(AggregatorSensorBase::STD);
            else if (opName == "percentiles")
                s.setOperation(AggregatorSensorBase::QTL);
            else if (opName == "observations")
                s.setOperation(AggregatorSensorBase::OBS);
        } else if (boost::iequals(val.first, "percentile")) {
            size_t quantile = stoull(val.second.data());
            if( quantile>0 && quantile<100 )
                s.setPercentile(quantile);
        }
    }
}

void JobAggregatorConfigurator::operatorAttributes(JobAggregatorOperator& op, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "window"))
            op.setWindow(stoull(val.second.data()) * 1000000);
        else if (boost::iequals(val.first, "goBack"))
            op.setGoBack(stoull(val.second.data()) * 1000000);
    }
}

bool JobAggregatorConfigurator::unit(UnitTemplate<AggregatorSensorBase>& u) {
    if(!u.isTopUnit()) {
        LOG(error) << "    " << _operatorName << ": This operator type only supports hierarchical units!";
        return false;
    }
    if(u.getOutputs().empty()) {
        LOG(error) << "    " << _operatorName << ": At least one output sensor per unit must be defined!";
        return false;
    }
    return true; 
}