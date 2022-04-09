//================================================================================
// Name        : UnitGenerator.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Helper template to generate Operator Units.
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

#ifndef PROJECT_UNITGENERATOR_H
#define PROJECT_UNITGENERATOR_H

#include <set>
#include <list>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include "sensornavigator.h"
#include "UnitTemplate.h"
#include "mqttchecker.h"
#include "logging.h"

using namespace std;

/**
 * @brief Helper template to generate Operator Units
 *
 * @details This template decouples the unit-related configuration logic from
 *          AnalyerConfiguratorTemplate, so that users can override such
 *          template without losing access to the unit system.
 *
 * @ingroup operator
 */
template <class SBase=SensorBase>
class UnitGenerator {
    // The template shall only be instantiated for classes which derive from SensorBase
    static_assert(std::is_base_of<SensorBase, SBase>::value, "SBase must derive from SensorBase!");

public:

    /**
    * @brief            Class constructor
    */
    UnitGenerator() {}

    /**
    * @brief            Class constructor
    *
    * @param navi       SensorNavigator object to be used internally
    */
    UnitGenerator(shared_ptr<SensorNavigator> navi) {
        _navi = navi;
    }

    /**
    * @brief            Class destructor
    */
    ~UnitGenerator() {}

    /**
    * @brief            Sets the internal SensorNavigator object
    *
    *                   Note that a SensorNavigator must be set before the UnitGenerator can be used
    *
    * @param navi       Shared pointer to a SensorNavigator object
    */
    void setNavigator(shared_ptr<SensorNavigator> navi) { _navi = navi; }

    /**
    * @brief            Parses a string encoding a tree level
    *
    *                   This method serves to parse strings that are used to express hierarchy levels in config files
    *                   of the data analytics framework. These strings are in the format "<topdown X>.*" or
    *                   "<bottomup X>.*", and signify "sensors that are in nodes X levels down from level 0 in the
    *                   sensor tree", or "sensors that are in nodes X levels up from the deepest level in the sensor
    *                   tree" respectively. As such, the method returns the depth level of sensors represented by the
    *                   input string.
    *
    * @param s          String to be parsed
    * @return           Absolute depth level in the sensor tree that is encoded in the string
    */
    int  parseNodeLevelString(const string& s) {
        if(!_navi || !_navi->treeExists())
            throw runtime_error("UnitGenerator: SensorNavigator tree not initialized!");

        int _treeDepth = _navi->getTreeDepth();
        bool topDown = false;
        if(boost::regex_search(s.c_str(), _match, _blockRx)) {
            string blockMatch = _match.str(0);
            if(boost::regex_search(blockMatch.c_str(), _match, _topRx))
                topDown = true;
            else if(boost::regex_search(blockMatch.c_str(), _match, _bottomRx))
                topDown = false;
            else
                throw runtime_error("UnitGenerator: Syntax error in configuration!");
            blockMatch = _match.str(0);
            int lv;
            if(topDown)
                lv = !boost::regex_search(blockMatch.c_str(), _match, _numRx) ? 0 : (int)stoi(_match.str(0));
            else
                lv = !boost::regex_search(blockMatch.c_str(), _match, _numRx) ? _treeDepth : _treeDepth - (int)stoi(_match.str(0));

            if( lv < 0 ) lv = 0;
            else if( lv > _treeDepth ) lv = _treeDepth;

            return lv;
        }
        else
            return -1;
    }

    /**
    * @brief            Resolves a string encoding a tree level starting from a given node
    *
    *                   This method takes as input strings in the format specified for parseNodeLevelString(). It then
    *                   takes as input also the name of a node in the sensor tree. The method will then return the set
    *                   of sensors expressed by "s", that belong to nodes encoded in its hierarchy level and that are
    *                   related to "node", either as ancestors or descendants. If a filter was included in the unit
    *                   clause, e.g. <bottomup 1, filter cpu>freq, then only sensors in nodes matching the filter
    *                   regular expression will be returned.
    *
    * @param s          String to be parsed
    * @param node       Name of the target node
    * @param replace    If False only the names of the resolved units will be returned, without the sensor name
    * @return           Set of sensors encoded in "s" that are associated with "node"
    */
    set<string> *resolveNodeLevelString(const string& s, const string& node, const bool replace=true) {
        int level = parseNodeLevelString(s);
        if(!_navi->nodeExists(node))
            throw invalid_argument("UnitGenerator: Node " + node + " does not exist!");

        set<string> *sensors = new set<string>();
        if( level <= -1 )
            sensors->insert(replace ? s : SensorNavigator::rootKey);
        else {
            //Ensuring that only the "filter" clause matches is enough, since we already checked for the entire
            // < > configuration block in parseNodeLevelString
            boost::regex filter = boost::regex(boost::trim_copy(!boost::regex_search(s.c_str(), _match, _filterRx) ? ".*" : _match.str(0)));
            set<string> *nodes = _navi->navigate(node, level - _navi->getNodeDepth(node));
            // Filtering is performed here, as node names always include all upper levels of the hierarchy
            for(const auto& n : *nodes) {
                if (boost::regex_search(n.c_str(), _match, filter))
                    sensors->insert(replace ? boost::regex_replace(s, _blockRx, n) : n);
            }
            delete nodes;
        }
        return sensors;
    }
    
    /**
    * @brief                   Computes and instantiates a single unit
    *
    *                          Please refer to the generateUnits() method.
    *
    * @param u                 Name of the unit which must be instantiated
    * @param inputs	           The vector of "prototype" sensor objects for inputs
    * @param outputs           The vector of "prototype" sensor objects for outputs
    * @param inputMode         Defines the method with which input sensors are instantiated for each unit
    * @param mqttPrefix        MQTT prefix to use for output sensors
    * @param ondemand          If True, no unit resolution is performed and a raw unit template is stored
    * @param enforceTopics     If True, output sensors of units will have "mqttPrefix" pre-pended to their MQTT topics 
    * @param relaxed           If True, checks on the existence of input sensors are ignored
    * @return	               A shared pointer to the generated unit object
    */
    shared_ptr<UnitTemplate<SBase>> generateUnit(const string& u, vector<shared_ptr<SBase>>& inputs, vector<shared_ptr<SBase>>& outputs,
            inputMode_t inputMode, string mqttPrefix="", bool ondemand=false, bool enforceTopics=false, bool relaxed=false) {

        list<string> names;
        names.push_back(u);
        vector<shared_ptr<UnitTemplate<SBase>>> *units = generateUnits(names, inputs, outputs, inputMode, 
                mqttPrefix, ondemand, enforceTopics, relaxed);
        shared_ptr<UnitTemplate<SBase>> finalUnit = units->at(0);
        delete units;
        return finalUnit;
    }
    
    /**
    * @brief                   Computes and instantiates a single hierarchical unit
    *
    *                          Please refer to the generateHierarchicalUnits() method.
    *
    * @param u                 Name of the unit which must be instantiated
    * @param subNames          List of node identifiers used to build the sub-units for each top unit
    * @param outputs           The vector of "prototype" sensor objects for the top level unit's outputs
    * @param subInputs	       The vector of "prototype" sensor objects for the sub-units' inputs
    * @param subOutputs        The vector of "prototype" sensor objects for the sub-units' outputs
    * @param inputMode         Defines the method with which input sensors are instantiated for each unit
    * @param mqttPrefix        MQTT prefix to use for output sensors
    * @param ondemand          If True, no unit resolution is performed and a raw unit template is stored
    * @param enforceTopics     If True, output sensors of units will have "mqttPrefix" pre-pended to their MQTT topics   
    * @param relaxed           If True, checks on the existence of input sensors are ignored 
    * @return	               A shared pointer to the generated hierarchical unit object
    */
    shared_ptr<UnitTemplate<SBase>> generateHierarchicalUnit(const string& u, const list<std::string>& subNames,
            vector<shared_ptr<SBase>>& outputs, vector<shared_ptr<SBase>>& subInputs, vector<shared_ptr<SBase>>& subOutputs,
            inputMode_t inputMode, string mqttPrefix="", bool ondemand=false, bool enforceTopics=false, bool relaxed=false) {

        list<string> names;
        names.push_back(u);
        vector<shared_ptr<UnitTemplate<SBase>>> *units = generateHierarchicalUnits(names, subNames, outputs, subInputs, 
                subOutputs, inputMode, mqttPrefix, ondemand, enforceTopics, relaxed);
        shared_ptr<UnitTemplate<SBase>> finalUnit = units->at(0);
        delete units;
        return finalUnit;
    }

    /**
    * @brief                   Computes and instantiates a single (or multiple) unit automatically
    *
    *                          Please refer to the generateAutoUnits() method. This variant takes only one string as
    *                          identifier for the top-level unit. If hierarchical units are chosen (outputs vector
    *                          is non-empty) then only one output unit is produced. Otherwise, a vector of units, one
    *                          for each element in the domains of the subOutputs sensors, are produced.
    *
    * @param u                 Name of the (top-level) unit which must be instantiated
    * @param subNames          List of node identifiers used to build the sub-units (or plain units)
    * @param outputs           The vector of "prototype" sensor objects for the top level unit's outputs
    * @param subInputs	       The vector of "prototype" sensor objects for the sub-units' inputs
    * @param subOutputs        The vector of "prototype" sensor objects for the sub-units' outputs
    * @param inputMode         Defines the method with which input sensors are instantiated for each unit
    * @param mqttPrefix        MQTT prefix to use for output sensors
    * @param ondemand          If True, no unit resolution is performed and a raw unit template is stored
    * @param enforceTopics     If True, output sensors of units will have "mqttPrefix" pre-pended to their MQTT topics 
    * @param relaxed           If True, checks on the existence of input sensors are ignored
    * @return	               A shared pointers to the generated hierarchical unit object
    */
    vector<shared_ptr<UnitTemplate<SBase>>> *generateAutoUnit(const string& u, const list<std::string>& subNames,
            vector<shared_ptr<SBase>>& outputs, vector<shared_ptr<SBase>>& subInputs, vector<shared_ptr<SBase>>& subOutputs,
            inputMode_t inputMode, string mqttPrefix="", bool ondemand=false, bool enforceTopics=false, bool relaxed=false) {

        list<string> names;
        names.push_back(u);
        return generateAutoUnits(names, subNames, outputs, subInputs, subOutputs, inputMode, mqttPrefix, ondemand, enforceTopics, relaxed);
    }

    /**
    * @brief                   Computes and instantiates a single unit from a template
    *
    *                          This method generates a unit starting from an input "template" unit object. The resulting
    *                          unit will be plain or hierarchical depending on how the template is structured. Contrary
    *                          to generateAutoUnits(), here "u" is prioritized and always describes the name of the unit,
    *                          hierarchical or not. The "subNames" vector, on the other hand, is used only if the template
    *                          "tUnit" is hierarchical itself.
    *
    * @param tUnit             Template unit object to be used
    * @param u                 Name of the unit which must be instantiated
    * @param subNames          List of node identifiers used to build the sub-units for each top unit
    * @param mqttPrefix        MQTT prefix to use for output sensors
    * @param enforceTopics     If True, output sensors of units will have "mqttPrefix" pre-pended to their MQTT topics
    * @param relaxed           If True, checks on the existence of input sensors are ignored
    * @return	               A vector of shared pointers to the generated hierarchical (or plain) unit objects
    */
    shared_ptr<UnitTemplate<SBase>> generateFromTemplate(shared_ptr<UnitTemplate<SBase>> tUnit, const string& u,
            const list<std::string>& subNames, string mqttPrefix="", bool enforceTopics=false, bool relaxed=false) {

        if (tUnit->isTopUnit()) {
            if (tUnit->getSubUnits().size() != 1)
                throw invalid_argument("UnitGenerator: hierarchical template unit is malformed!");
            if(subNames.empty() && u!=SensorNavigator::rootKey)
                throw invalid_argument("UnitGenerator: only root unit is supported for this template type!");

            shared_ptr <UnitTemplate<SBase>> subUnit = tUnit->getSubUnits()[0];
            return generateHierarchicalUnit(u, subNames, tUnit->getOutputs(), subUnit->getInputs(),
                    subUnit->getOutputs(), tUnit->getInputMode(), mqttPrefix, false, enforceTopics, relaxed);
        } else {
            return generateUnit(u, tUnit->getInputs(), tUnit->getOutputs(), tUnit->getInputMode(), 
                    mqttPrefix, false, enforceTopics, relaxed);
        }
    }
    
    /**
    * @brief                   Computes and instantiates units associated to the input operator
    *
    *                          This will compute the list of units that must be instantiated, starting from the
    *                          outputs of the operator. The inputs for each unit are then computed, and the units
    *                          finalized. The method takes as input two vectors of "prototype" input and output sensors
    *                          that will be used as "templates" to generate the units. Such prototype sensors must
    *                          be in the type of the template, and must have names containing the <unit> construct,
    *                          which will be use to resolve the actual units.
    *
    * @param uNames            List of sensor tree node names from which units must be generated. If empty, the list of
     *                         nodes will be inferred from the domains of the output sensors
    * @param inputs	           The vector of "prototype" sensor objects for inputs
    * @param outputs           The vector of "prototype" sensor objects for outputs
    * @param inputMode         Defines the method with which input sensors are instantiated for each unit
    * @param mqttPrefix        MQTT prefix to use for output sensors
    * @param ondemand          If True, no unit resolution is performed and a raw unit template is stored
    * @param enforceTopics     If True, output sensors of units will have "mqttPrefix" pre-pended to their MQTT topics
    * @param relaxed           If True, checks on the existence of input sensors are ignored
    * @return	               A vector of shared pointers to the generated unit objects
    */
    vector<shared_ptr<UnitTemplate<SBase>>> *generateUnits(const list<std::string>& uNames, vector<shared_ptr<SBase>>& inputs, 
            vector<shared_ptr<SBase>>& outputs, inputMode_t inputMode, string mqttPrefix="", bool ondemand=false, 
            bool enforceTopics=false, bool relaxed=false) {
        
        // If no inputs are defined, no units can be instantiated
        if(inputs.empty() && inputMode==SELECTIVE)
            throw invalid_argument("UnitGenerator: Invalid inputs!");

        // Output sensors must share the same unit pattern
        if(!outputs.empty() && !isConsistent(outputs))
            throw invalid_argument("UnitGenerator: Incoherent output levels!");
        
        set<string>* units = NULL;
        
        // No list of units to be built was supplied - we infer ourselves
        if(uNames.empty()) {
            // We iterate over the outputs, and compute their depth level in the current sensor tree. From such depth 
            // level, the list of units is defined, consisting in all nodes in the sensor tree at the level of outputs
            int unitLevel = outputs.empty() ? -1 : parseNodeLevelString(outputs[0]->getName());
            if (unitLevel > -1)
                units = resolveNodeLevelString(outputs[0]->getName(), SensorNavigator::rootKey, false);
                // If no depth level was found (output sensor names do not contain any <unit> block) we assume that
                // everything relates to root
            else if (unitLevel == -1) {
                units = new set<string>();
                units->insert(SensorNavigator::rootKey);
            }

            if (!units || units->empty())
                throw invalid_argument("UnitGenerator: Invalid output level or unit specification!");
        }
        else {
            units = new set<string>();
            for(const auto &u : uNames) {
                // The unit specified as input must belong to the domain of the outputs
                if (!outputs.empty() && !nodeBelongsToPattern(u, outputs[0]->getName()))
                    LOG(debug) << "UnitGenerator: Node " + u + " does not belong to this unit domain!";
                else
                    units->insert(u);
            }
            if(units->empty()) {
                delete units;
                throw invalid_argument("UnitGenerator: All input nodes do not belong to this unit domain!");
            }
        }
        
        // We iterate over the units, and resolve their inputs and outputs starting from the prototype definitions
        vector<shared_ptr<UnitTemplate<SBase>>> *unitObjects = new vector<shared_ptr<UnitTemplate<SBase>>>();
        if(!ondemand)
            for(const auto& u : *units) {
                try {
                    unitObjects->push_back(_generateUnit(u, inputs, outputs, inputMode, mqttPrefix, enforceTopics, relaxed));
                } catch(const exception& e) {
                    if(units->size()>1) {
                        LOG(debug) << e.what();
                        LOG(debug) << "UnitGenerator: cannot build unit " << u << "!";
                        continue;
                    } else {
                        delete units;
                        delete unitObjects;
                        throw;
                    }
                }
            }
        else {
            shared_ptr<UnitTemplate<SBase>> unPtr = make_shared<UnitTemplate<SBase>>(SensorNavigator::templateKey, inputs, outputs);
            unPtr->setInputMode(inputMode);
            unitObjects->push_back(unPtr);
        }

        delete units;
        if(unitObjects->empty()) {
            delete unitObjects;
            throw invalid_argument("UnitGenerator: No units were created!");
        }
        
        return unitObjects;
    }
    
    /**
    * @brief                   Computes and instantiates a set of hierarchical units
    *
    *                          Hierarchical units differ from conventional units. First, they have two levels: a top
    *                          unit which contains all "global" outputs that are eventually propagated. This unit does 
    *                          not have inputs, as in, sensors. Then, there are as many children units as specified 
    *                          tree nodes: for each of these, all of the inputs that were specified are present, 
    *                          together with node-related outputs. 
    *
    * @param uNames            List of names from which units must be generated. Must not be empty
    * @param subNames          List of node identifiers used to build the sub-units for each top unit
    * @param outputs           The vector of "prototype" sensor objects for the top level unit's outputs
    * @param subInputs	       The vector of "prototype" sensor objects for the sub-units' inputs
    * @param subOutputs        The vector of "prototype" sensor objects for the sub-units' outputs
    * @param inputMode         Defines the method with which input sensors are instantiated for each unit
    * @param mqttPrefix        MQTT prefix to use for output sensors
    * @param ondemand          If True, no unit resolution is performed and a raw unit template is stored
    * @param enforceTopics     If True, output sensors of units will have "mqttPrefix" pre-pended to their MQTT topics
    * @param relaxed           If True, checks on the existence of input sensors are ignored
    * @return	               A vector of shared pointers to the generated hierarchical unit objects
    */
    vector<shared_ptr<UnitTemplate<SBase>>> *generateHierarchicalUnits(const list<std::string>& uNames, const list<std::string>& subNames, 
            vector<shared_ptr<SBase>>& outputs, vector<shared_ptr<SBase>>& subInputs, vector<shared_ptr<SBase>>& subOutputs,
            inputMode_t inputMode, string mqttPrefix="", bool ondemand=false, bool enforceTopics=false, bool relaxed=false) {

        // Checking if unit names were actually supplied
        if(uNames.empty())
            throw invalid_argument("HierarchicalUnitGenerator: No unit names were supplied!");
            
        // If no inputs or outputs are defined, no units can be instantiated
        if((subInputs.size()==0 && inputMode==SELECTIVE))
            throw invalid_argument("HierarchicalUnitGenerator: Invalid inputs or outputs!");

        // Output sensors must share the same unit pattern
        if(!subOutputs.empty() && !isConsistent(subOutputs))
            throw invalid_argument("HierarchicalUnitGenerator: Incoherent output levels!");
        
        vector<shared_ptr<UnitTemplate<SBase>>> *unitObjects = new vector<shared_ptr<UnitTemplate<SBase>>>();
        
        if(!ondemand)
            for(const auto& u : uNames) {
                shared_ptr <UnitTemplate<SBase>> topUnit = make_shared<UnitTemplate<SBase>>(u);
                std::string effPrefix;
                if(u==SensorNavigator::rootKey) {
                    effPrefix = MQTTChecker::formatTopic(mqttPrefix);
                } else {
                    effPrefix = MQTTChecker::formatTopic(mqttPrefix) + MQTTChecker::formatTopic(u);
                    topUnit->setName(MQTTChecker::formatTopic(u) + "/");
                }
                try {
                    vector<shared_ptr<UnitTemplate<SBase>>>* units = generateUnits(subNames, subInputs, subOutputs, inputMode, effPrefix, ondemand, enforceTopics, relaxed);
                    topUnit->setSubUnits(*units);
                    delete units;
                } catch (const invalid_argument &e) {
                    topUnit->clear();
                    if(uNames.size()>1) {
                        LOG(debug) << e.what();
                        LOG(debug) << "HierarchicalUnitGenerator: cannot build unit " << u << "!";
                        continue;
                    } else {
                        delete unitObjects;
                        throw;
                    }
                }
                
                // Mapping outputs
                for (const auto &out : outputs) {
                    shared_ptr <SBase> uOut = make_shared<SBase>(*out);
                    uOut->setMqtt(effPrefix + MQTTChecker::formatTopic(uOut->getMqtt()));
                    uOut->setName(uOut->getMqtt());
                    topUnit->addOutput(uOut);
                }
                
                unitObjects->push_back(topUnit);
            }
        else {
            shared_ptr<UnitTemplate<SBase>> unPtr = make_shared<UnitTemplate<SBase>>(SensorNavigator::templateKey, vector<shared_ptr<SBase>>(), outputs);
            shared_ptr<UnitTemplate<SBase>> subPtr = make_shared<UnitTemplate<SBase>>(SensorNavigator::templateKey, subInputs, subOutputs);
            unPtr->setInputMode(inputMode);
            subPtr->setInputMode(inputMode);
            unPtr->addSubUnit(subPtr);
            unitObjects->push_back(unPtr);
        }
        
        if(unitObjects->empty()) {
            delete unitObjects;
            throw invalid_argument("HierarchicalUnitGenerator: No units were created!");
        }
        
        return unitObjects;
    }

    /**
    * @brief                   Computes and instantiates a set of units automatically
    *
    *                          This method detects whether a set of units needs to be hierarchical or not, and 
    *                          arranges them accordingly. If at least one "global" output sensor (outputs vector) is
    *                          defined, a hierarchical unit is created. Otherwise, units are arranged in a plain 
    *                          fashion, as a vector, as described by the "subNames" vector.
    *
    * @param uNames            List of names from which top-level units must be generated. Must not be empty
    * @param subNames          List of node identifiers used to build the sub-units for each top unit (or plain units)
    * @param outputs           The vector of "prototype" sensor objects for the top level unit's outputs
    * @param subInputs	       The vector of "prototype" sensor objects for the sub-units' inputs
    * @param subOutputs        The vector of "prototype" sensor objects for the sub-units' outputs
    * @param inputMode         Defines the method with which input sensors are instantiated for each unit
    * @param mqttPrefix        MQTT prefix to use for output sensors
    * @param ondemand          If True, no unit resolution is performed and a raw unit template is stored
    * @param enforceTopics     If True, output sensors of units will have "mqttPrefix" pre-pended to their MQTT topics
    * @param relaxed           If True, checks on the existence of input sensors are ignored
    * @return	               A vector of shared pointers to the generated hierarchical (or plain) unit objects
    */
    vector<shared_ptr<UnitTemplate<SBase>>> *generateAutoUnits(const list<std::string>& uNames, const list<std::string>& subNames,
            vector<shared_ptr<SBase>>& outputs, vector<shared_ptr<SBase>>& subInputs, vector<shared_ptr<SBase>>& subOutputs,
            inputMode_t inputMode, string mqttPrefix="", bool ondemand=false, bool enforceTopics=false, bool relaxed=false) {
        
        if(outputs.empty())
            return generateUnits(subNames, subInputs, subOutputs, inputMode, mqttPrefix, ondemand, enforceTopics, relaxed);
        else
            return generateHierarchicalUnits(uNames, subNames, outputs, subInputs, subOutputs, inputMode, mqttPrefix, ondemand, enforceTopics, relaxed);
    }
    
protected:
    
    /**
    *                       This private method will resolve all nodes in the current sensor tree that satisfy 
    *                       the "unit" pattern, and then verify whether "node" belongs to this set or not.
    *                                      
    * @param node           Sensor tree node name to be checked
    * @param unit           Pattern unit expression to generate a domain
    * @return               True if "node" belongs to the domain of "unit", false otherwise
    */
    bool nodeBelongsToPattern(const string& node, const string& unit) {
        set<string>* units = resolveNodeLevelString(unit, SensorNavigator::rootKey, false);
        if(units->count(node)) {
            delete units;
            return true;
        }
        else {
            delete units;
            return false;
        }
    }
    
    // Private method that checks if all output sensors in the given vector contain the same unit pattern
    bool isConsistent(vector<shared_ptr<SBase>>& outputs) {
        if(outputs.empty())
            return false;

        string pattern = boost::regex_search(outputs[0]->getName().c_str(), _match, _blockRx) ? _match.str(0) : "";
        string otherPattern;
        for(const auto& out : outputs) {
            otherPattern = boost::regex_search(out->getName().c_str(), _match, _blockRx) ? _match.str(0) : "";
            if (pattern != otherPattern)
                return false;
        }
        return true;
    }

    // Private method for building units, that does not perform redundant checks
    shared_ptr<UnitTemplate<SBase>> _generateUnit(const string& u, vector<shared_ptr<SBase>>& inputs, 
            vector<shared_ptr<SBase>>& outputs, inputMode_t inputMode, string mqttPrefix, bool enforceTopics, bool relaxed) {
        
        vector<shared_ptr<SBase>> unitInputs, unitOutputs;
        // AddedSensors keeps track of which sensor names were added already to the input set, to prevent duplicates
        set<string> addedSensors, *sensors;
        // Mapping inputs
        for(const auto& in : inputs) {
            // Depending on the relationship of an input prototype sensor to the output level, it could be
            // mapped to one sensor or more: for example, if output has level <bottomup 1>, and an input sensor
            // has level <bottomup 2>, than the input will be unique, and the sensor associated to the father of the
            // unit. If the other way around, the input will consist of multiple sensors, one for each child of
            // the unit
            sensors = resolveNodeLevelString(in->getName(), u);
            if (sensors->empty()) {
                delete sensors;
                throw invalid_argument("UnitGenerator: String " + in->getName() + " cannot be resolved!");
            } else
                for (const auto &s : *sensors) {
                    if (!addedSensors.count(s)) {
                        SBase uIn(*in);
                        uIn.setMqtt(s);
                        uIn.setName(s);
                        if (!(_navi->sensorExists(uIn.getName()) || relaxed)) {
                            delete sensors;
                            throw invalid_argument("UnitGenerator: Sensor " + uIn.getName() + " does not exist!");
                        }
                        addedSensors.insert(s);
                        unitInputs.push_back(make_shared<SBase>(uIn));
                    }
                }
            delete sensors;
        }

        // If the all or all-recursive clauses were specified, we pick all sensors related to the specific unit
        // This means that when output unit is "root" (or not specified) we get only sensors at the highest level
        if(inputMode != SELECTIVE) {
            sensors = _navi->getSensors(u, inputMode == ALL_RECURSIVE);
            for(const auto& s : *sensors) {
                if( !addedSensors.count(s) ) {
                    SBase uIn(s);
                    unitInputs.push_back(make_shared<SBase>(uIn));
                }
            }
            delete sensors;
        }

        // Mapping outputs
        for(const auto& out : outputs) {
            SBase uOut(*out);
            // Checking if the unit's node exists (throws an exception if it doesn't)
            sensors = resolveNodeLevelString(uOut.getName(), u);
            delete sensors;
            // If we are instantiating output sensors by unit, we generate mqtt topics by using the prefix
            // associated to the respective node in the sensor tree, and the sensor suffix itself
            // If we are not using units (only unit is root, out of the hierarchy) we pre-pend mqttPrefix to suffix
            // Same happens if the enforceTopics flag is enabled
            if(u!=SensorNavigator::rootKey) {
                uOut.setMqtt(_navi->buildTopicForNode(u, uOut.getMqtt()));
                if(enforceTopics)
                    uOut.setMqtt(MQTTChecker::formatTopic(mqttPrefix) + uOut.getMqtt());
            } else
                uOut.setMqtt(MQTTChecker::formatTopic(mqttPrefix) + MQTTChecker::formatTopic(uOut.getMqtt()));
            // Setting the name back to the MQTT topic
            uOut.setName(uOut.getMqtt());
            unitOutputs.push_back(make_shared<SBase>(uOut));
        }
        shared_ptr<UnitTemplate<SBase>> unPtr = make_shared<UnitTemplate<SBase>>(u, unitInputs, unitOutputs);
        if(u!=SensorNavigator::rootKey)
            unPtr->setName(MQTTChecker::formatTopic(u) + "/");
        unPtr->setInputMode(inputMode);
        return unPtr;
    }

    // Logger object
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;

    //Internal SensorNavigator object
    shared_ptr<SensorNavigator> _navi;

    //Regular expressions used in the parseNodeLevelString and resolveNodeLevelString methods
    boost::cmatch          _match;
    //Regex that matches the entire unit configuration block
    const boost::regex     _blockRx       = boost::regex("<.*>");
    const boost::regex     _bottomRx      = boost::regex("(?<=[,<])[ \\t]*bottomup[ \\t]*([ \\t]*[0-9]+[ \\t]*)?(?=[,>])");
    const boost::regex     _topRx         = boost::regex("(?<=[,<])[ \\t]*topdown[ \\t]*([ \\t]*[0-9]+[ \\t]*)?(?=[,>])");
    const boost::regex     _filterRx      = boost::regex("(?<=filter)[ \\t]+[^ \\t,>]+");
    const boost::regex     _numRx         = boost::regex("[0-9]+");

};

#endif //PROJECT_UNITGENERATOR_H
