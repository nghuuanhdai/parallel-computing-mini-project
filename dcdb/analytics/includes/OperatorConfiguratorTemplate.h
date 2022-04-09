//================================================================================
// Name        : OperatorConfiguratorTemplate.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Template implementing a standard OperatorConfiguratorInterface.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
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

#ifndef PROJECT_OPERATORCONFIGURATORTEMPLATE_H
#define PROJECT_OPERATORCONFIGURATORTEMPLATE_H

#include <map>
#include <set>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include "OperatorTemplate.h"
#include "OperatorConfiguratorInterface.h"
#include "sensorbase.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#define CFG_VAL	boost::property_tree::iptree&

/**
 * @brief Template that implements a standard OperatorConfiguratorInterface.
 *
 * @details Users should employ this template whenever possible, and create
 *          their own configurators only when strictly necessary.
 *
 * @ingroup operator
 */
template <class Operator, class SBase = SensorBase>
class OperatorConfiguratorTemplate : public OperatorConfiguratorInterface {

    // Verifying the types of input classes
    static_assert(std::is_base_of<SensorBase, SBase>::value, "SBase must derive from SensorBase!");
    static_assert(std::is_base_of<OperatorInterface, Operator>::value, "Operator must derive from OperatorInterface!");

protected:

    // For readability
    using O_Ptr = std::shared_ptr<Operator>;

    // Some wildcard characters
    const char COMMA = ',';
    const char OPEN_SQBRKET = '[';
    const char CLOSE_SQBRKET = ']';
    const char DASH = '-';

    // Keywords used to identify input and output sensor blocks
    const string INPUT_BLOCK_LEGACY = "input";
    const string OUTPUT_BLOCK_LEGACY = "output";
    const string INPUT_BLOCK = "unitInput";
    const string OUTPUT_BLOCK = "unitOutput";
    const string GLOBAL_OUTPUT_BLOCK = "globalOutput";
    const string ALL_CLAUSE = "all";
    const string ALL_REC_CLAUSE = "all-recursive";
    
public:

    /**
    * @brief            Class constructor
    */
    OperatorConfiguratorTemplate() :
            _queryEngine(QueryEngine::getInstance()),
            _operatorName("INVALID"),
            _baseName("INVALID"),
            _cfgPath(""),
            _mqttPrefix(""),
            _cacheInterval(900000) {}

    /**
    * @brief            Copy constructor is not available
    */
    OperatorConfiguratorTemplate(const OperatorConfiguratorTemplate&) = delete;

    /**
    * @brief            Assignment operator is not available
    */
    OperatorConfiguratorTemplate& operator=(const OperatorConfiguratorTemplate&) = delete;

    /**
    * @brief            Class destructor
    */
    virtual ~OperatorConfiguratorTemplate() {
        for (auto ta : _templateOperators)
            delete ta.second;
        for (auto ts : _templateSensors)
            delete ts.second;
        _templateOperators.clear();
        _templateSensors.clear();
        _templateProtoInputs.clear();
        _operatorInterfaces.clear();
        _operators.clear();
    }

    /**
    * @brief                    Sets default global settings for operators
    *
    *                           This method should be called once after constructing a configurator and before reading
    *                           the configuration, so that it has access to the default global settings (which can
    *                           be overridden.
    *
    * @param pluginSettings	    struct with global default settings for the plugins.
    */
    virtual void setGlobalSettings(const pluginSettings_t& pluginSettings) final {
        _mqttPrefix = pluginSettings.mqttPrefix;
        _cacheInterval = pluginSettings.cacheInterval;
    }

    /**
    * @brief                    Print configuration as read in.
    *
    * @param ll                 Logging level to log with
    */
    void printConfig(LOG_LEVEL ll) final {
        LOG_VAR(ll) << "    General: ";
        LOG_VAR(ll) << "          MQTT-Prefix:    " << (_mqttPrefix != "" ? _mqttPrefix : std::string("DEFAULT"));
        LOG_VAR(ll) << "          CacheInterval: " << _cacheInterval/1000 << " [s]";

        //prints plugin specific configurator attributes and entities if present
        printConfiguratorConfig(ll);

        LOG_VAR(ll) << "    Operators: ";
        for(auto a : _operators) {
            LOG_VAR(ll) << "        Operator: " << a->getName();
            a->printConfig(ll);
        }
    }

    /**
    * @brief            Read a config file and instantiate operators accordingly
    *
    *                   This method supplies standard algorithms to instantiate operators and parse units from config
    *                   files accordingly. It should be overridden only if strictly necessary, which generally should
    *                   not happen.
    *
    * @param cfgPath    Path to the config file
    * @return	        True if successful, false otherwise
    */
    bool readConfig(std::string cfgPath) {
        _cfgPath = cfgPath;
        _unitGen.setNavigator(_queryEngine.getNavigator());

        boost::property_tree::iptree cfg;
        boost::property_tree::read_info(cfgPath, cfg);

        // Read global variables (if present overwrite those from global.conf)
        readGlobal(cfg);

        // Reading operators and template operators
        BOOST_FOREACH(boost::property_tree::iptree::value_type &val, cfg) {
            // In this block templates are parsed and read
            if (boost::iequals(val.first, "template_" + _operatorName)) {
                LOG(debug) << "Template " << _operatorName << " \"" << val.second.data() << "\"";
                if (!val.second.empty()) {
                    Operator* op = new Operator(val.second.data());
                    op->setTemplate(true);
                    if (!readOperator(*op, val.second)) {
                        LOG(warning) << "Template " << _operatorName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
                        delete op;
                    }
                }
            // Sensor templates are read
            } else if (boost::iequals(val.first, "template_" + _baseName)) {
                LOG(debug) << "Template " << _baseName << " \"" << val.second.data() << "\"";
                if (!val.second.empty()) {
                    SBase* base = new SBase(val.second.data());
                    if (!readSensorBase(*base, val.second, true)) {
                        LOG(warning) << "Template " << _baseName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
                        delete base;
                    }
                }
            // Here we read and instantiate operators
            } else if (boost::iequals(val.first, _operatorName)) {
                LOG(debug) << _operatorName << " \"" << val.second.data() << "\"";
                if (!val.second.empty()) {
                    O_Ptr op = std::make_shared<Operator>(val.second.data());
                    if (readOperator(*op, val.second)) {
                        // If the operator must be duplicated for each compute unit, we copy-construct identical
                        // instances that have different unit IDs
                        unsigned numUnits = op->getUnits().size();
                        op->releaseUnits();
                        if(op->getDuplicate() && numUnits>1) {
                            for(unsigned int i=0; i < numUnits; i++) {
                                O_Ptr opCopy = std::make_shared<Operator>(*op);
                                opCopy->setUnitID(i);
                                //opCopy->setName(opCopy->getName() + "@" + op->getUnits()[i]->getName());
                                opCopy->collapseUnits();
                                storeOperator(opCopy);
                            }
                        } else
                            storeOperator(op);
                    } else {
                        LOG(warning) << _operatorName << " \"" << val.second.data() << "\" has bad values! Ignoring...";
                    }
                }
            } else if( !boost::iequals(val.first, "global") ) {
                LOG(error) << "\"" << val.first << "\": unknown construct!";
                return false;
            }
        }
        return true;
    }

    /**
    * @brief            Clears the plugin configuration
    *
    *                   This will stop any operators that have been created, destroy them and return the plugin to
    *                   its uninitialized state.
    *
    */
    void clearConfig() final {
        // Stop all operators
        for(auto op : _operators)
            op->stop();

        // Wait for all operators to finish
        for(auto op : _operators)
            op->wait();

        // First of all, delete all template operators and sensors
        for (auto to : _templateOperators)
            delete to.second;
        for (auto ts : _templateSensors)
            delete ts.second;

        // Clear all operators
        _operatorInterfaces.clear();
        _operators.clear();
        _templateOperators.clear();
        _templateSensors.clear();
        _templateProtoInputs.clear();
    }

    /**
    * @brief            Clear all instantiated operators and read the configuration again
    *
    *                   This will stop any operators that have been created, destroy them and finally create new ones
    *                   from a new configuration read pass.
    *
    * @return	        True if successful, false otherwise
    */
    bool reReadConfig() final {
        clearConfig();

        // Reading the configuration once again
        return readConfig(_cfgPath);
    }

    /**
    * @brief                   Return all instantiated operators
    *
    * @return	               Vector containing pointers to all operator interfaces of this plugin
    */
    std::vector<OperatorPtr>& getOperators() final {
        return _operatorInterfaces;
    }

protected:

    /**
    * @brief           Reads any derived operator attributes
    *
    *                  Pure virtual interface method, responsible for reading plugin-specific operator attributes.
    *
    * @param op		   The operator for which derived attributes must be set
    * @param config	   A Boost property (sub-)tree containing the config attributes
    */
    virtual void operatorAttributes(Operator& op, CFG_VAL config) = 0;

    /**
    * @brief           Reads any derived sensor attributes
    *
    *                  Pure virtual interface method, responsible for reading plugin-specific sensor attributes.
    *
    * @param s		   The sensor for which derived attributes must be set
    * @param config	   A Boost property (sub-)tree containing the config attributes
    */
    virtual void sensorBase(SBase& s, CFG_VAL config) = 0;

    /**
    * @brief           Performs additional checks on instantiated units
    *
    *                  Pure virtual interface method, responsible for performing user-specified checks on units.
    *
    * @param u		   The unit that has been created
    * @return          True if the unit is valid, False otherwise
    */
    virtual bool unit(UnitTemplate<SBase>& u) = 0;

    /**
    * @brief           Reads additional global attributes on top of the default ones
    *
    *                  Virtual interface method, responsible for reading plugin-specific global attributes.
    *
    * @param config	   A Boost property (sub-)tree containing the global values
    */
    virtual void global(CFG_VAL config) {}

    /**
    * @brief            Print information about configurable configurator attributes.
    *
    *                   This method is virtual and can be overridden on a per-plugin basis.
    *
    * @param ll         Severity level to log with
    */
    virtual void printConfiguratorConfig(LOG_LEVEL ll) {
        LOG_VAR(ll) << "          No other plugin-specific general parameters defined";
    }

    /**
    * @brief                   Store an operator in the internal vectors
    *
    * @param op                Shared pointer to a OperatorInterface object
    */
    void storeOperator(O_Ptr op) {
        _operators.push_back(op);
        _operatorInterfaces.push_back(op);
    }

    /**
    * @brief                   Reads a single operator configuration block
    *
    *                          Non-virtual interface method for class-internal use only. This will configure an
    *                          Operator object, and instantiate all units associated to it. All derived attributes
    *                          and additional configuration must be performed in the operatorAttributes() virtual method.
    *
    * @param op	               The operator that must be configured
    * @param config	           A boost property (sub-)tree containing the operator values
    * @return	               True if successful, false otherwise
    */
    bool readOperator(Operator& op, CFG_VAL config) {
        // Vectors containing "prototype" inputs and outputs to be modified with the actual compute units
        std::vector<shared_ptr<SBase>> protoInputs, protoOutputs, protoGlobalOutputs;
        inputMode_t inputMode = SELECTIVE;
        // Check for the existence of a template definition to initialize the operator
        boost::optional<boost::property_tree::iptree&> def = config.get_child_optional("default");
        if(def) {
            LOG(debug) << "  Using \"" << def.get().data() << "\" as default.";
            auto it = _templateOperators.find(def.get().data());
            if(it != _templateOperators.end()) {
                op = *(it->second);
                op.setName(config.data());
                op.setTemplate(false);
                // Operators instantiated from templates DO NOT share the same units and output sensors. 
                // This would lead to too much naming ambiguity and is generally just not needed
                op.clearUnits();
                // The input sensors defined in the template are on the other hand preserved; this is meant as a 
                // workaround to shorten certain configurations 
                protoInputs = _templateProtoInputs[def.get().data()];
            } else {
                LOG(warning) << "Template " << _operatorName << "\"" << def.get().data() << "\" not found! Using standard values.";
            }
        }
        // Reading attributes associated to OperatorInterface
        BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
        {
            if (boost::iequals(val.first, "interval")) {
                op.setInterval(stoull(val.second.data()));
	    } else if (boost::iequals(val.first, "queueSize")) {
		op.setQueueSize(stoull(val.second.data()));
            } else if (boost::iequals(val.first, "minValues")) {
                op.setMinValues(stoull(val.second.data()));
            } else if (boost::iequals(val.first, "mqttPart")) {
                op.setMqttPart(val.second.data());
            } else if (boost::iequals(val.first, "enforceTopics")) {
                op.setEnforceTopics(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "sync")) {
                op.setSync(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "disabled")) {
                op.setDisabled(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "delay")) {
                op.setDelayInterval(stoull(val.second.data()));
            } else if (boost::iequals(val.first, "duplicate")) {
                op.setDuplicate(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "relaxed")) {
                op.setRelaxed(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "unitCacheLimit")) {
                op.setUnitCacheLimit(stoull(val.second.data()));
            } else if (boost::iequals(val.first, "streaming")) {
                op.setStreaming(to_bool(val.second.data()));
            } else if (isInputBlock(val.first) || isOutputBlock(val.first) || isGlobalOutputBlock(val.first)) {
                // Instantiating all sensors contained within the "unitInput", "unitOutput" or "globalOutput" block
                BOOST_FOREACH(boost::property_tree::iptree::value_type &valInner, val.second)
                {
                    if (boost::iequals(valInner.first, _baseName)) {
                        LOG(debug) << "    I/O " << _baseName << " " << valInner.second.data();
                        SBase sensor(valInner.second.data());
                        if (readSensorBase(sensor, valInner.second, false)) {
                            shared_ptr<SBase> sensorPtr = make_shared<SBase>(sensor);
                            if(isInputBlock(val.first))
                                protoInputs.push_back(sensorPtr);
                            else if(isOutputBlock(val.first))
                                protoOutputs.push_back(sensorPtr);
                            else
                                protoGlobalOutputs.push_back(sensorPtr);
                        } else {
                            LOG(warning) << "I/O " << _baseName << " " << op.getName() << "::" << sensor.getName() << " could not be read! Omitting";
                        }
                    // An "all" or "all-recursive" statement in the input block causes all sensors related to the specific
                    // unit to be picked
                    } else if (isInputBlock(val.first) && (boost::iequals(valInner.first, ALL_CLAUSE) || boost::iequals(valInner.first, ALL_REC_CLAUSE))) {
                        inputMode = boost::iequals(valInner.first, ALL_CLAUSE) ? ALL : ALL_RECURSIVE;
                    } else {
                        LOG(error) << "\"" << valInner.first << "\": unknown I/O construct!";
                        return false;
                    }
                }
            }
        }
        
        // Reading all derived attributes, if any
        operatorAttributes(op, config);
        // Instantiating units
        if(!op.getTemplate()) {
            op.setMqttPart(MQTTChecker::formatTopic(_mqttPrefix) + MQTTChecker::formatTopic(op.getMqttPart()));
            return readUnits(op, protoInputs, protoOutputs, protoGlobalOutputs, inputMode);
        } else {
            // If the operator is a template, we add it to the related map
            auto ret = _templateOperators.insert(std::pair<std::string, Operator*>(op.getName(), &op));
            if(!ret.second) {
                LOG(warning) << "Template " << _operatorName << " " << op.getName() << " already exists! Omitting...";
                return false;
            }
            _templateProtoInputs.insert(std::pair<std::string, std::vector<shared_ptr<SBase>>>(op.getName(), protoInputs));
        }
        
        return true;
    }

    /**
    * @brief                   Reads a single sensor configuration block
    *
    *                          Non-virtual interface method for class-internal use only. This will configure a
    *                          sensor object. All derived attributes and additional configuration must be performed
    *                          in the sensorBase() virtual method.
    *
    * @param sBase	           The sensor that must be configured
    * @param config	           A boost property (sub-)tree containing the sensor values
    * @param isTemplate        Do we read a template sensor?
    * @return	               True if successful, false otherwise
    */
    bool readSensorBase(SBase& sBase, CFG_VAL config, bool isTemplate=false) {
        sBase.setCacheInterval(_cacheInterval);
        if (!isTemplate) {
            // Copying parameters from the template (if defined)
            boost::optional<boost::property_tree::iptree&> def = config.get_child_optional("default");
            if(def) {
                LOG(debug) << "  Using \"" << def.get().data() << "\" as default.";
                auto it = _templateSensors.find(def.get().data());
                if(it != _templateSensors.end()) {
                    sBase = *(it->second);
                    sBase.setName(config.data());
                } else {
                    LOG(warning) << "Template " << _baseName << "\" " << def.get().data() << "\" not found! Using standard values.";
                }
            }
        }
        // Reading other sensor parameters
        BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config) {
            if (boost::iequals(val.first, "mqttsuffix")) {
                sBase.setMqtt(val.second.data());
            } else if (boost::iequals(val.first, "skipConstVal")) {
                sBase.setSkipConstVal(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "delta")) {
                sBase.setDelta(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "deltaMax")) {
                sBase.setDeltaMaxValue(std::stoull(val.second.data()));
            } else if (boost::iequals(val.first, "subSampling")) {
                sBase.setSubsampling(std::stoi(val.second.data()));
            } else if (boost::iequals(val.first, "publish")) {
                sBase.setPublish(to_bool(val.second.data()));
            } else if (boost::iequals(val.first, "metadata")) {
                SensorMetadata sm;
                if(sBase.getMetadata())
                    sm = *sBase.getMetadata();
                try {
                    sm.parsePTREE(val.second);
                    sBase.setMetadata(sm);
                } catch(const std::exception& e) {
                    LOG(error) << "  Metadata parsing failed for sensor " << sBase.getName() << "!" << std::endl;
                }
            }
        }
        sensorBase(sBase, config);
        
        if(isTemplate) {
            auto ret = _templateSensors.insert(std::pair<std::string, SBase*>(sBase.getName(), &sBase));
            if(!ret.second) {
                LOG(warning) << "Template " << _baseName << " " << sBase.getName() << " already exists! Omitting...";
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * @brief               Instantiates all necessary units for a single operator
     * 
     *                      This method will create and assign all unit objects for a single operator, given a set
     *                      of prototype input sensors, prototype output sensors and an input mode. This method is
     *                      virtual such as to allow for flexibility in case specific operators require different
     *                      assignment policies (such as job operators).
     * 
     * @param op                  The operator whose units must be created
     * @param protoInputs         The vector of prototype input sensors
     * @param protoOutputs        The vector of prototype output sensors
     * @param protoGlobalOutputs  The vector of prototype global output sensors, if any
     * @param inputMode           Input mode to be used (selective, all or all_recursive)
     * @return                    True if successful, false otherwise
     */
    virtual bool readUnits(Operator& op, std::vector<shared_ptr<SBase>>& protoInputs, std::vector<shared_ptr<SBase>>& protoOutputs,
            std::vector<shared_ptr<SBase>>& protoGlobalOutputs, inputMode_t inputMode) {
        vector <shared_ptr<UnitTemplate<SBase>>> *units = NULL;
        if(protoOutputs.empty())
            LOG(debug) << "    No output specified, generating sink unit.";
        // If we employ a hierarchical unit (which will be the root unit) we disable duplication 
        if(!protoGlobalOutputs.empty())
            op.setDuplicate(false);
        
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

    /**
    * @brief                   Reads the global configuration block
    *
    *                          Non-virtual interface method for class-internal use only. This will read the "global"
    *                          configuration block in a file, overwriting any default settings on a per-plugin base.
    *                          Any derived or additional attributes must be added through the global() virtual method.
    *
    * @param config	           A Boost property (sub-)tree containing the global block
    * @return	               True if successful, false otherwise
    */
    bool readGlobal(CFG_VAL config) {
        boost::optional<boost::property_tree::iptree&> globalVals = config.get_child_optional("global");
        if (globalVals) {
            BOOST_FOREACH(boost::property_tree::iptree::value_type &global, config.get_child("global")) {
                if (boost::iequals(global.first, "mqttprefix")) {
                    _mqttPrefix = global.second.data();
                    LOG(debug) << "  Using own MQTT-Prefix " << _mqttPrefix;
                } else if (boost::iequals(global.first, "cacheInterval")) {
                    _cacheInterval = stoul(global.second.data());
                    LOG(debug) << "  Using own caching interval " << _cacheInterval << " [s]";
                    _cacheInterval *= 1000;
                } 
            }
            global(config.get_child("global"));
        }
        return true;
    }

    /**
    * @brief                   Adjusts the topics and names of the sensors
    *
    *                          Names are set according to the corresponding topic.
    *
    * @return                  true if successful, false otherwise
    */
    bool constructSensorTopics(UnitTemplate<SBase>& u, Operator& op) {
        // Performing name construction
        for(auto& s: u.getOutputs()) {
            adjustSensor(s, op, u);
        }
        for(auto& subUnit: u.getSubUnits())
            for(auto& s : subUnit->getOutputs()) {
                adjustSensor(s, op, u);
            }
        return true;
    }

    /**
    * @brief                   Adjusts a single sensor
    *
    */
    void adjustSensor(std::shared_ptr<SBase> s, Operator& op, UnitTemplate<SBase>& u) {
        s->setName(s->getMqtt());
        SensorMetadata* sm = s->getMetadata();
        if(sm) {
            if(sm->getIsOperation() && *sm->getIsOperation()) {
                s->clearMetadata();
                if(u.getInputs().size() != 1) {
                    LOG(error) << _operatorName << " " << op.getName() << ": Ambiguous operation field for sensor " << s->getName();
                    return;
                }
                // Replacing the metadata to publish the sensor as an operation of its corresponding input
                SensorMetadata smNew;
                smNew.setPublicName(u.getInputs()[0]->getMqtt());
                smNew.setPattern(u.getInputs()[0]->getMqtt());
                smNew.addOperation(s->getMqtt());
                s->setMetadata(smNew);
            } else {
                sm->setPublicName(s->getMqtt());
                sm->setPattern(s->getMqtt());
                sm->setIsVirtual(false);
                if (!sm->getInterval())
                    sm->setInterval((uint64_t)op.getInterval() * 1000000);
            }
        }
    }
    
    /**
    * @brief                   Returns true if the input string describes an input block
    * 
    * @param s                 The string to be checked 
    * @return                  True if s is a input block, false otherwise
    */
    bool isInputBlock(const std::string& s ) {
        return boost::iequals(s, INPUT_BLOCK) || boost::iequals(s, INPUT_BLOCK_LEGACY);
    }

    /**
    * @brief                   Returns true if the input string describes an output block
    * 
    * @param s                 The string to be checked 
    * @return                  True if s is a output block, false otherwise
    */
    bool isOutputBlock(const std::string& s ) {
        return boost::iequals(s, OUTPUT_BLOCK) || boost::iequals(s, OUTPUT_BLOCK_LEGACY);
    }

    /**
    * @brief                   Returns true if the input string describes a global output block
    * 
    * @param s                 The string to be checked 
    * @return                  True if s is a global output block, false otherwise
    */
    bool isGlobalOutputBlock(const std::string& s ) {
        return boost::iequals(s, GLOBAL_OUTPUT_BLOCK);
    }

    // Instance of a QueryEngine object
    QueryEngine&     _queryEngine;
    // UnitGenerator object used to create units
    UnitGenerator<SBase>    _unitGen;

    // Keyword used to identify operator blocks in config files
    std::string		_operatorName;
    // Keyword used to identify sensors in config files
    std::string     _baseName;

    // Path of the configuration file that must be used
    std::string 	_cfgPath;
    // Default MQTT prefix to be used when creating output sensors
    std::string		_mqttPrefix;
    // Interval in seconds for the cache of each sensor
    unsigned int	_cacheInterval;
    // The vector of operators, in the form of pointers to OperatorInterface objects
    std::vector<OperatorPtr> 	_operatorInterfaces;
    // Like the above, but containing the operators in their actual types
    std::vector<O_Ptr>		_operators;
    // Map of the template operators that were defined in the config file - used for easy retrieval and instantiation
    std::map<std::string, Operator*> _templateOperators;
    // Map of the template sensors that were defined
    std::map<std::string, SBase*> _templateSensors;
    // Map of the protoinputs belonging to template operators
    std::map<std::string, std::vector<shared_ptr<SBase>>> _templateProtoInputs;
};

#endif //PROJECT_OPERATORCONFIGURATORTEMPLATE_H
