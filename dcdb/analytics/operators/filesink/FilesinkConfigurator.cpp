//================================================================================
// Name        : FilesinkConfigurator.cpp
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

#include "FilesinkConfigurator.h"

FilesinkConfigurator::FilesinkConfigurator() : OperatorConfiguratorTemplate() {
    _operatorName = "sink";
    _baseName     = "sensor";
}

FilesinkConfigurator::~FilesinkConfigurator() {}

void FilesinkConfigurator::sensorBase(FilesinkSensorBase& s, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "path")) {
            s.setPath(val.second.data());
        } 
    }
}

void FilesinkConfigurator::operatorAttributes(FilesinkOperator& op, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "autoName"))
            op.setAutoName(to_bool(val.second.data()));
    }
}

bool FilesinkConfigurator::unit(UnitTemplate<FilesinkSensorBase>& u) {
    if(u.isTopUnit()) {
        LOG(error) << "    " << _operatorName << ": This operator type only supports flat units!";
        return false;
    }
    if(!u.getOutputs().empty()) {
        LOG(error) << "    " << _operatorName << ": This is a file sink, no output sensors can be defined!";
        return false;
    }
    return true; 
}
