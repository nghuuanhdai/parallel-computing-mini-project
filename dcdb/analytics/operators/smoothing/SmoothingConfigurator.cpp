//================================================================================
// Name        : SmoothingConfigurator.cpp
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

#include "SmoothingConfigurator.h"

SmoothingConfigurator::SmoothingConfigurator() : OperatorConfiguratorTemplate() {
    _operatorName = "smoother";
    _baseName     = "sensor";
}

SmoothingConfigurator::~SmoothingConfigurator() {}

void SmoothingConfigurator::operatorAttributes(SmoothingOperator& op, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "separator")) {
            op.setSeparator(val.second.data());
        } else if (boost::iequals(val.first, "exclude")) {
            op.setExclude(val.second.data());
        }
    }
}

void SmoothingConfigurator::sensorBase(SmoothingSensorBase& s, CFG_VAL config) {
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
        if (boost::iequals(val.first, "range")) {
            s.setRange(std::stoull(val.second.data()));
        }
    }
}

bool SmoothingConfigurator::unit(UnitTemplate<SmoothingSensorBase>& u) {
    if(u.isTopUnit()) {
        LOG(error) << "    " << _operatorName << ": This operator type only supports flat units!";
        return false;
    }
    if(u.getInputs().size() != 1) {
        LOG(error) << "    " << _operatorName << ": Exactly one input sensor per unit must be defined!";
        return false;
    }
    if(u.getOutputs().empty()) {
        LOG(error) << "    " << _operatorName << ": At least one output sensor per unit must be defined!";
        return false;
    }
    return true;
}

bool SmoothingConfigurator::readUnits(SmoothingOperator& op, std::vector<SmoothingSBPtr>& protoInputs, std::vector<SmoothingSBPtr>& protoOutputs,
               std::vector<SmoothingSBPtr>& protoGlobalOutputs, inputMode_t inputMode) {
    vector<shared_ptr<UnitTemplate<SmoothingSensorBase>>> *units = NULL;
    vector<SmoothingSBPtr> realInputs, realOutputs;

    // Printing a warning if declaring global outputs or selecting on-demand mode
    if(!protoGlobalOutputs.empty()) {
        LOG(warning) << _operatorName << " " << op.getName() << ": Global outputs will be ignored.";
        protoGlobalOutputs.clear();
    }
    if(!op.getStreaming()) {
        LOG(warning) << _operatorName << " " << op.getName() << ": This operator does not support on-demand mode.";
        op.setStreaming(true);
    }

    // If no inputs are specified, we pick all sensors present in the sensor tree
    if(protoInputs.empty() && inputMode==SELECTIVE) {
        set <string> *inputNames = NULL;
        try {
            inputNames = _queryEngine.getNavigator()->getSensors(SensorNavigator::rootKey, true);
        } catch(const std::exception &e) {
            LOG(error) << _operatorName << " " << op.getName() << ": Error when creating units: " << e.what();
            return false;
        }
        for(const auto& n : *inputNames) {
            SmoothingSensorBase ssb(n);
            ssb.setMqtt(n);
            protoInputs.push_back(std::make_shared<SmoothingSensorBase>(ssb));
        }
        delete inputNames;
    // Resolving all input sensors beforehand if some were defined
    } else if(!protoInputs.empty()) {
        vector<SmoothingSBPtr> tempInputs;
        for(const auto & sIn : protoInputs) {
            set<string> *inputNames = _unitGen.resolveNodeLevelString(sIn->getName(), SensorNavigator::rootKey);
            for (const auto &n : *inputNames) {
                SmoothingSensorBase ssb(n);
                ssb.setMqtt(n);
                tempInputs.push_back(std::make_shared<SmoothingSensorBase>(ssb));
            }
            delete inputNames;
        }
        // Replacing the original inputs
        protoInputs.clear();
        protoInputs.insert(protoInputs.end(), tempInputs.begin(), tempInputs.end());
        tempInputs.clear();
    }

    boost::cmatch match;
    boost::regex excludeReg(op.getExclude());
    // Generating one separate unit for each input sensor
    for(const auto& pIn : protoInputs) {
        if (op.getExclude()=="" || !boost::regex_search(pIn->getMqtt().c_str(), match, excludeReg)) {
            realInputs.push_back(pIn);
            for (const auto &sOut : protoOutputs) {
                // Removing patterns from the output prototype sensors
                SmoothingSensorBase ssb(*sOut);
                ssb.setMqtt(MQTTChecker::formatTopic(pIn->getMqtt()) + _stripTopic(ssb.getMqtt(), op.getSeparator()));
                ssb.setName(ssb.getMqtt());
                realOutputs.push_back(std::make_shared<SmoothingSensorBase>(ssb));
            }
            try {
                units = _unitGen.generateAutoUnit(SensorNavigator::rootKey, std::list<std::string>(), protoGlobalOutputs,
                        realInputs, realOutputs, inputMode, "", !op.getStreaming(), op.getEnforceTopics(), op.getRelaxed());
            }
            catch (const std::exception &e) {
                LOG(error) << _operatorName << " " << op.getName() << ": Error when creating units: " << e.what();
                delete units;
                return false;
            }

            if(units->size() > 1) {
                LOG(error) << _operatorName << " " << op.getName() << ": Unexpected number of units created.";
                delete units;
                return false;
            }
            
            for (auto &u: *units) {
                u->setName(pIn->getMqtt());
                if (!constructSensorTopics(*u, op)) {
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
            }
            realInputs.clear();
            realOutputs.clear();
            delete units;
        }
    }
    return true;
}

std::string SmoothingConfigurator::_stripTopic(const std::string& topic, const std::string& separator) {
    if(topic.empty()) return topic;
    std::string newTopic = topic;
    // Stripping off all forward slashes
    while(!newTopic.empty() && newTopic.front()==MQTT_SEP) newTopic.erase(0, 1);
    while(!newTopic.empty() && newTopic.back()==MQTT_SEP)  newTopic.erase(newTopic.size()-1, 1);
    // Adding a front separator
    newTopic = separator + newTopic;
    return newTopic;
}
