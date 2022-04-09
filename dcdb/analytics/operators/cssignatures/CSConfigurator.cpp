//================================================================================
// Name        : CSConfigurator.cpp
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

#include "CSConfigurator.h"

CSConfigurator::CSConfigurator() : OperatorConfiguratorTemplate() {
    _operatorName = "signature";
    _baseName     = "sensor";
}

CSConfigurator::~CSConfigurator() {}

void CSConfigurator::sensorBase(CSSensorBase& s, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "imag"))
            s.setImag(to_bool(val.second.data()));
    }
}

void CSConfigurator::operatorAttributes(CSOperator& op, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if(boost::iequals(val.first, "window"))
            op.setAggregationWindow(stoull(val.second.data()) * 1000000);
        else if(boost::iequals(val.first, "inputPath"))
            op.setInputPath(val.second.data());
        else if(boost::iequals(val.first, "outputPath"))
            op.setOutputPath(val.second.data());
        else if(boost::iequals(val.first, "reuseModel"))
            op.setReuseModel(to_bool(val.second.data()));
        else if(boost::iequals(val.first, "numBlocks"))
            op.setNumBlocks(stoull(val.second.data()));
        else if(boost::iequals(val.first, "trainingSamples"))
            op.setTrainingSamples(stoull(val.second.data())); 
        else if (boost::iequals(val.first, "scalingFactor"))
            op.setScalingFactor(stoull(val.second.data()));
    }
}

bool CSConfigurator::unit(UnitTemplate<CSSensorBase>& u) {
    if(u.isTopUnit()) {
        LOG(error) << "    " << _operatorName << ": This operator type only supports flat units!";
        return false;
    }
    if(u.getOutputs().empty()) {
        LOG(error) << "    " << _operatorName << ": At least one output sensor per unit must be defined!";
        return false;
    }
    return true;
}

bool CSConfigurator::readUnits(CSOperator& op, std::vector<CSSBPtr>& protoInputs, std::vector<CSSBPtr>& protoOutputs, 
        std::vector<CSSBPtr>& protoGlobalOutputs, inputMode_t inputMode) {

    vector <shared_ptr<UnitTemplate<CSSensorBase>>> *units = NULL;
    if(protoOutputs.empty() || !protoGlobalOutputs.empty() || protoOutputs.size() > 2) {
        LOG(error) << this->_operatorName << " " << op.getName() << ": Units must be flat with at most two output sensors!";
        return false;
    }
    
    bool realDone=false, imagDone=false;
    vector<CSSBPtr> trueOutputs;
    
    // Duplicating sensors according to the number of blocks
    for (const auto &s : protoOutputs) {
        if(!s->getImag() && !realDone) {
            realDone = true;
        } else if(s->getImag() && !imagDone) {
            imagDone = true;
        } else {
            continue;
        }
        
        for(size_t i=0; i<op.getNumBlocks(); i++) {
            auto outS = std::make_shared<CSSensorBase>(*s);
            outS->setMqtt(outS->getMqtt() + std::to_string(i));
            outS->setName(outS->getName() + std::to_string(i));
            outS->setBlockID(i);
            trueOutputs.push_back(outS);
        }
    }
    // Replacing sensors
    protoOutputs.clear();
    protoOutputs = trueOutputs;
    trueOutputs.clear();
    
    try {
        units = _unitGen.generateAutoUnit(SensorNavigator::rootKey, std::list<std::string>(), protoGlobalOutputs, protoInputs,
                                          protoOutputs, inputMode, op.getMqttPart(), !op.getStreaming(), op.getEnforceTopics(), op.getRelaxed());
    }
    catch (const std::exception &e) {
        LOG(error) << _operatorName << " " << op.getName() << ": Error when creating units: " << e.what();
        delete units;
        return false;
    }

    for (auto &u: *units) {
        if (op.getStreaming()) {
            if(!constructSensorTopics(*u, op)) {
                op.clearUnits();
                delete units;
                return false;
            }
            if (!unit(*u)) {
                LOG(error) << "    Unit " << u->getName() << " did not pass the final check!";
                op.clearUnits();
                delete units;
                return false;
            } else {
                LOG(debug) << "    Unit " << u->getName() << " generated.";
                op.addUnit(u);
            }
        } else {
            if (unit(*u)) {
                op.addToUnitCache(u);
                LOG(debug) << "    Template unit for on-demand operation " + u->getName() + " generated.";
            } else {
                LOG(error) << "    Template unit " << u->getName() << " did not pass the final check!";
                op.clearUnits();
                delete units;
                return false;
            }
        }
    }
    delete units;
    return true;
}
