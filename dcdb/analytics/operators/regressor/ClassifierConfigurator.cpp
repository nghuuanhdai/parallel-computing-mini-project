//================================================================================
// Name        : ClassifierConfigurator.cpp
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

#include "ClassifierConfigurator.h"

ClassifierConfigurator::ClassifierConfigurator() {
    _operatorName = "classifier";
    _baseName     = "sensor";
}

ClassifierConfigurator::~ClassifierConfigurator() {}

void ClassifierConfigurator::sensorBase(RegressorSensorBase& s, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "target")) {
            s.setTrainingTarget(to_bool(val.second.data()));
            std::string opName = val.second.data();
        }
    }
}

void ClassifierConfigurator::operatorAttributes(ClassifierOperator& op, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "trainingSamples"))
            op.setTrainingSamples(stoull(val.second.data()));
        else if(boost::iequals(val.first, "window"))
            op.setAggregationWindow(stoull(val.second.data()) * 1000000);
        else if(boost::iequals(val.first, "inputPath"))
            op.setInputPath(val.second.data());
        else if(boost::iequals(val.first, "outputPath"))
            op.setOutputPath(val.second.data());
        else if(boost::iequals(val.first, "getImportances"))
            op.setComputeImportances(to_bool(val.second.data()));
        else if(boost::iequals(val.first, "targetDistance"))
            op.setTargetDistance(stoull(val.second.data()));
        else if(boost::iequals(val.first, "rawMode"))
            op.setRawMode(to_bool(val.second.data()));
    }
}

bool ClassifierConfigurator::unit(UnitTemplate<RegressorSensorBase>& u) {
    if(u.isTopUnit()) {
        LOG(error) << "    " << _operatorName << ": This operator type only supports flat units!";
        return false;
    }
    bool targetSet=false;
    for(const auto& in : u.getInputs())
        if(in->getTrainingTarget()) {
            if(!targetSet)
                targetSet = true;
            else {
                LOG(error) << _operatorName << ": Only one classification target can be specified!";
                return false;
            }
        }
    if(!targetSet) {
        LOG(warning) << "    " << _operatorName << ": No classification target was specified. Online model training will be unavailable.";
    }
    if(u.getInputs().empty() || (targetSet && u.getInputs().size() < 2)) {
        LOG(error) << "    " << _operatorName << ": Insufficient amount of input sensors!";
        return false;
    }
    if(u.getOutputs().size()!=1) {
        LOG(error) << "    " << _operatorName << ": Only one output sensor per unit is allowed!";
        return false;
    }
    return true;
}