//================================================================================
// Name        : TesterOperatorConfigurator.cpp
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

#include "TesterOperatorConfigurator.h"

TesterOperatorConfigurator::TesterOperatorConfigurator() : OperatorConfiguratorTemplate() {
        _operatorName = "operator";
        _baseName     = "sensor";
}

TesterOperatorConfigurator::~TesterOperatorConfigurator() {}

void TesterOperatorConfigurator::sensorBase(SensorBase& s, CFG_VAL config) {}

void TesterOperatorConfigurator::operatorAttributes(TesterOperator& op, CFG_VAL config) {
        BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
        {
                if (boost::iequals(val.first, "window"))
                        op.setWindow(stoull(val.second.data()) * 1000000);
                else if (boost::iequals(val.first, "queries"))
                        op.setNumQueries(stoull(val.second.data()));
                else if (boost::iequals(val.first, "relative"))
                        op.setRelative(to_bool(val.second.data()));
        }
}

bool TesterOperatorConfigurator::unit(UnitTemplate<SensorBase>& u) {
    if(u.getOutputs().size()!=1) {
        LOG(error) << "    " << _operatorName << ": Only one output sensor in the top unit is allowed!";
        return false;
    }

    if(u.isTopUnit())
        for(const auto& subUnit : u.getSubUnits())
            if(subUnit->getOutputs().size()!=1) {
                    LOG(error) << "    " << _operatorName << ": Only one output sensor per sub unit is allowed!";
                    return false;
            } 
    return true;
}
