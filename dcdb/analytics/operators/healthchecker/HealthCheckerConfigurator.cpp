//================================================================================
// Name        : HealthCheckerConfigurator.cpp
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

#include "HealthCheckerConfigurator.h"

HealthCheckerConfigurator::HealthCheckerConfigurator() : OperatorConfiguratorTemplate() {
    _operatorName = "healthchecker";
    _baseName     = "sensor";
}

HealthCheckerConfigurator::~HealthCheckerConfigurator() {}

void HealthCheckerConfigurator::sensorBase(HealthCheckerSensorBase& s, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "threshold")) {
            s.setThreshold(std::stoull(val.second.data()));
        } else if (boost::iequals(val.first, "condition")) {
            HealthCheckerSensorBase::HCCond c = s.stringToCond(val.second.data());
            s.setCondition(c);
            if (c == HealthCheckerSensorBase::HC_INVALID) {
                LOG(error) << "    " << _operatorName << ": Invalid alarm condition specified!";
            }
        }
    }
}

void HealthCheckerConfigurator::operatorAttributes(HealthCheckerOperator& op, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "cooldown")) {
            op.setCooldown(std::stoull(val.second.data()) * 1000000);
        } else if (boost::iequals(val.first, "window")) {
            op.setWindow(std::stoull(val.second.data()) * 1000000);
        } else if (boost::iequals(val.first, "log")) {
            op.setLog(to_bool(val.second.data()));
        } else if (boost::iequals(val.first, "command")) {
            op.setCommand(val.second.data());
            if(!op.isCommandValid(val.second.data())) {
                LOG(error) << "    " << _operatorName << ": Invalid command specified!";
            }
        } /*else if (boost::iequals(val.first, "shell")) {
            op.setShell(val.second.data());
        } */
    }
}

bool HealthCheckerConfigurator::unit(UnitTemplate<HealthCheckerSensorBase>& u) {
    if(u.isTopUnit()) {
        LOG(error) << "    " << _operatorName << ": This operator type only supports flat units!";
        return false;
    }
    if(!u.getOutputs().empty()) {
        LOG(error) << "    " << _operatorName << ": This is an health checker, no output sensors can be defined!";
        return false;
    }
    return true;
}
