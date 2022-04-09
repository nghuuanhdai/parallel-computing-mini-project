//================================================================================
// Name        : ClusteringConfigurator.cpp
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

#include "ClusteringConfigurator.h"

ClusteringConfigurator::ClusteringConfigurator() : OperatorConfiguratorTemplate() {
    _operatorName = "clustering";
    _baseName     = "sensor";
}

ClusteringConfigurator::~ClusteringConfigurator() {}

void ClusteringConfigurator::sensorBase(ClusteringSensorBase& s, CFG_VAL config) {}

void ClusteringConfigurator::operatorAttributes(ClusteringOperator& op, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if(boost::iequals(val.first, "window"))
            op.setAggregationWindow(stoull(val.second.data()) * 1000000);
        else if(boost::iequals(val.first, "lookbackWindow"))
            op.setLookbackWindow(stoull(val.second.data()) * 1000000);
        else if(boost::iequals(val.first, "inputPath"))
            op.setInputPath(val.second.data());
        else if(boost::iequals(val.first, "outputPath"))
            op.setOutputPath(val.second.data());
        else if(boost::iequals(val.first, "numComponents"))
            op.setNumComponents(stoull(val.second.data()));
        else if(boost::iequals(val.first, "outlierCut"))
            op.setOutlierCut(stod(val.second.data()));
        else if(boost::iequals(val.first, "reuseModel"))
            op.setReuseModel(to_bool(val.second.data()));
    }
}

bool ClusteringConfigurator::unit(UnitTemplate<ClusteringSensorBase>& u) {
    if(!u.isTopUnit()) {
        LOG(error) << "    " << _operatorName << ": This operator type only supports hierarchical units!";
        return false;
    }
    if(u.getSubUnits().empty()) {
        LOG(error) << "    " << _operatorName << ": No sub-units were instantiated!";
        return false;
    }
    
    for(const auto& su : u.getSubUnits())
        if(su->getOutputs().size()!=1) {
            LOG(error) << "    " << _operatorName << ": Only one output sensor per unit is allowed!";
            return false;
        }
    return true; 
}

bool ClusteringConfigurator::readUnits(ClusteringOperator& op, std::vector<ClusteringSBPtr>& protoInputs, std::vector<ClusteringSBPtr>& protoOutputs,
                       std::vector<ClusteringSBPtr>& protoGlobalOutputs, inputMode_t inputMode) {
    
    if(op.getDuplicate()) {
        LOG(warning) << this->_operatorName << " " << op.getName() << ": The units of this operator cannot be duplicated.";
        op.setDuplicate(false);
    }
    shared_ptr<UnitTemplate<ClusteringSensorBase>> un = nullptr;
    try {
        un = _unitGen.generateHierarchicalUnit(SensorNavigator::rootKey, std::list<std::string>(), protoGlobalOutputs, protoInputs, 
                protoOutputs, inputMode, op.getMqttPart(), !op.getStreaming(), op.getEnforceTopics(), op.getRelaxed());
    }
    catch (const std::exception &e) {
        LOG(error) << _operatorName << " " << op.getName() << ": Error when creating units: " << e.what();
        return false;
    }
    
    if (op.getStreaming()) {
        if(!constructSensorTopics(*un, op)) {
            op.clearUnits();
            return false;
        }
        if (!unit(*un)) {
            LOG(error) << "    Unit " << un->getName() << " did not pass the final check!";
            op.clearUnits();
            return false;
        } else {
            LOG(debug) << "    Unit " << un->getName() << " generated.";
            op.addUnit(un);
        }
    } else {
        if (unit(*un)) {
            op.addToUnitCache(un);
            LOG(debug) << "    Template unit for on-demand operation " + un->getName() + " generated.";
        } else {
            LOG(error) << "    Template unit " << un->getName() << " did not pass the final check!";
            op.clearUnits();
            return false;
        }
    }
    return true;
}
