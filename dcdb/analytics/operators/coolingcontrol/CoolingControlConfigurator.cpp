//================================================================================
// Name        : CoolingControlConfigurator.cpp
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

#include "CoolingControlConfigurator.h"

CoolingControlConfigurator::CoolingControlConfigurator() : OperatorConfiguratorTemplate() {
    _operatorName = "controller";
    _baseName     = "sensor";
}

CoolingControlConfigurator::~CoolingControlConfigurator() {}

void CoolingControlConfigurator::sensorBase(CoolingControlSensorBase& s, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "hotThreshold")) {
            s.setHotThreshold(std::stoull(val.second.data()));
        } else if (boost::iequals(val.first, "critThreshold")) {
            s.setCriticalThreshold(std::stoull(val.second.data()));
        }
    }
}

void CoolingControlConfigurator::operatorAttributes(CoolingControlOperator& op, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "OIDSuffix"))
            op.getController().setOIDSuffix(val.second.data());
        else if (boost::iequals(val.first, "OIDPrefix"))
            op.getController().setOIDPrefix(val.second.data());
        else if (boost::iequals(val.first, "Community"))
            op.getController().setSNMPCommunity(val.second.data());
        else if (boost::iequals(val.first, "Version"))
            op.getController().setVersion(val.second.data());
        else if (boost::iequals(val.first, "Host"))
            op.getController().setHost(val.second.data());
        else if (boost::iequals(val.first, "Username"))
            op.getController().setUsername(val.second.data());
        else if (boost::iequals(val.first, "SecLevel"))
            op.getController().setSecurityLevel(val.second.data());
        else if (boost::iequals(val.first, "AuthProto"))
            op.getController().setAuthProto(val.second.data());
        else if (boost::iequals(val.first, "PrivProto"))
            op.getController().setPrivProto(val.second.data());
        else if (boost::iequals(val.first, "AuthKey"))
            op.getController().setAuthKey(val.second.data());
        else if (boost::iequals(val.first, "PrivKey"))
            op.getController().setPrivKey(val.second.data());
        else if (boost::iequals(val.first, "maxTemperature"))
            op.setMaxTemp(std::stoull(val.second.data()));
        else if (boost::iequals(val.first, "minTemperature"))
            op.setMinTemp(std::stoull(val.second.data()));
        else if (boost::iequals(val.first, "bins"))
            op.setBins(std::stoull(val.second.data()));
        else if (boost::iequals(val.first, "window"))
            op.setWindow(std::stoull(val.second.data()) * 1000000);
        else if (boost::iequals(val.first, "hotPercentage"))
            op.setHotPerc(std::stoull(val.second.data()));
        else if (boost::iequals(val.first, "strategy")) {
            if (val.second.data() == "continuous" || val.second.data() == "stepped") {
                op.setStrategy(val.second.data());
            }
        }
    }
}

bool CoolingControlConfigurator::unit(UnitTemplate<CoolingControlSensorBase>& u) {
    if(u.isTopUnit()) {
        LOG(error) << "    " << _operatorName << ": This operator type only supports flat units!";
        return false;
    }
    if(!u.getOutputs().empty()) {
        LOG(error) << "    " << _operatorName << ": This is a cooling control sink, no output sensors can be defined!";
        return false;
    }
    return true;
}
