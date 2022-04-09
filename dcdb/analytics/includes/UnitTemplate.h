//================================================================================
// Name        : UnitTemplate.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Template implementing features to use Units in Operators.
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

#ifndef PROJECT_UNITTEMPLATE_H
#define PROJECT_UNITTEMPLATE_H

#include "UnitInterface.h"

/**
 * @brief Template that implements features needed to use Units in Operators.
 *
 * @details The template accepts any class derived from SensorBase, allowing
 *          users to add their own attributes to input and output sensors. Users
 *          should employ this template in their plugins if possible, and change
 *          it only if absolutely necessary.
 *
 * @ingroup operator
 */
template <class S=SensorBase>
class UnitTemplate : public UnitInterface {
    // The template shall only be instantiated for classes which derive from SensorBase
    static_assert(std::is_base_of<SensorBase, S>::value, "S must derive from SensorBase!");

protected:

    // For readability
    using S_Ptr = std::shared_ptr<S>;

public:
    
    /**
     * @brief           Class constructor
     * 
     * @param name      Name of this unit 
     */
    UnitTemplate(const std::string& name) :
            UnitInterface(),
            _name(name),
            _inputMode(SELECTIVE),
            _parent(nullptr) {}
    
    /**
    * @brief            Class constructor
    *
    * @param name       The name of this unit
    * @param inputs     The vector of input sensors associated to this unit
    * @param outputs    The vector of outputs sensors associated to this unit
    */
    UnitTemplate(const std::string& name, const std::vector<S_Ptr>& inputs, const std::vector<S_Ptr>& outputs) :
            UnitInterface(),
            _name(name),
            _inputMode(SELECTIVE),
            _inputs(inputs),
            _outputs(outputs),
            _parent(nullptr) {

        // base inputs and outputs vectors are constructed using iterators
        _baseInputs = std::vector<SBasePtr>(_inputs.begin(), _inputs.end());
        _baseOutputs = std::vector<SBasePtr>(_outputs.begin(), _outputs.end());
    }

    /**
    * @brief            Copy constructor
    */
    UnitTemplate(const UnitTemplate& other) :
            _name(other._name),
            _inputMode(other._inputMode),
            _parent(other._parent) {

        for(auto s : other._inputs) {
            _inputs.push_back(s);
            _baseInputs.push_back(s);
        }

        for(auto s : other._outputs) {
            _outputs.push_back(s);
            _baseOutputs.push_back(s);
        }
        
        for(auto u : other._subUnits) {
            _subUnits.push_back(u);
        }
    }

    /**
    * @brief            Assignment operator
    */
    UnitTemplate& operator=(const UnitTemplate& other) {
        _name = other._name;
        _inputMode = other._inputMode;
        _parent = other._parent;

        _inputs.clear();
        _baseInputs.clear();
        for(auto s : other._inputs) {
            _inputs.push_back(s);
            _baseInputs.push_back(s);
        }

        _outputs.clear();
        _baseOutputs.clear();
        for(auto s : other._outputs) {
            _outputs.push_back(s);
            _baseOutputs.push_back(s);
        }
        
        _subUnits.clear();
        for(auto u : other._subUnits) {
            _subUnits.push_back(u);
        }

        return *this;
    }

    /**
     * @brief           Class destructor
     */
    virtual ~UnitTemplate() { clear(); }
    
    /**
     * @brief           Clears this unit
     */
    void clear() {
        _baseInputs.clear();
        _inputs.clear();
        _baseOutputs.clear();
        _outputs.clear();
        _subUnits.clear();
        _parent = nullptr;
    }

    /**
    * @brief            Tells whether this is a sub-unit in a hierarchical unit with no further sub-units
    * 
    * @return           True if this is a terminal sub-unit, false otherwise
    */
    bool isLeafUnit()    { return _parent && _subUnits.empty(); }
    
    /**
    * @brief            Tells whether this is a sub-unit in a hierarchical unit 
    * 
    * @return           True if this is a sub-unit, false otherwise
    */
    bool isSubUnit()    { return _parent && !_subUnits.empty(); }

    /**
    * @brief            Tells whether this is the top entity of a hierarchical unit 
    * 
    * @return           True if this is a "top" unit, false otherwise
    */
    bool isTopUnit()    { return !_parent && !_subUnits.empty(); }

    /**
    * @brief            Initializes the sensors in the unit
    *
    * @param interval   Sampling interval in milliseconds
    */
    void init(unsigned int interval, unsigned int queueSize) override {
        for(const auto &s : _outputs)
            if (!s->isInit())
                s->initSensor(interval, queueSize);
        for (const auto &su : _subUnits)
            for (const auto &s : su->getOutputs())
                if (!s->isInit())
                    s->initSensor(interval, queueSize);
    }

    /**
    * @brief            Sets the name of this unit
    *
    * @param name       The name of this unit
    */
    void setName(const std::string& name) override { _name = name; }

    /**
    * @brief            Get the name of this unit
    *
    *                   A unit's name points to the logical entity that it represents; for example, it could be
    *                   "hpcsystem1.node44", or "node44.cpu10". All the outputs of the unit are then associated to its
    *                   entity, and all of the input are related to it as well.
    *
    * @return           The unit's name
    */
    std::string& getName() override                     { return _name; }

    /**
    * @brief            Sets the input mode of this unit
    *
    * @param iMode      The input mode that was used for this unit
    */
    void setInputMode(const inputMode_t iMode) override { _inputMode=iMode; }

    /**
    * @brief            Get the input mode of this unit
    *
    * @return           The unit's input mode
    */
    inputMode_t getInputMode()  override              { return _inputMode; }

    /**
    * @brief            Sets the parent of this unit
    *
    * @param p          Pointer to a UnitTemplate object
    */
    void setParent(UnitTemplate<S>* p)                { _parent = p; }

    /**
    * @brief            Get the pointer to this unit's parent
    *
    * @return           The unit's parent
    */
    UnitTemplate<S> *getParent()                      { return _parent; }

    /**
    * @brief            Get the (base) input sensors of this unit
    *
    * @return           A vector of pointers to SensorBase objects that constitute this unit's input
    */
    std::vector<SBasePtr>& getBaseInputs() override  { return _baseInputs; }

    /**
    * @brief            Get the derived input sensors of this unit
    *
    * @return           A vector of pointers to objects derived from SensorBase that constitute this unit's input
    */
    std::vector<S_Ptr>& getInputs()                  { return _inputs; }

    /**
    * @brief            Get the (base) output sensors of this unit
    *
    * @return           A vector of pointers to SensorBase objects that constitute this unit's output
    */
    std::vector<SBasePtr>& getBaseOutputs() override { return _baseOutputs; }

    /**
    * @brief            Get the derived output sensors of this unit
    *
    * @return           A vector of pointers to objects derived from SensorBase that constitute this unit's output
    */
    std::vector<S_Ptr>& getOutputs()                 { return _outputs; }

    /**
     * @brief           Get the internal vector of sub-units of this unit
     * 
     * @return          A reference to the internal vector of sub-unit pointers 
     */
    std::vector<std::shared_ptr<UnitTemplate<S>>>& getSubUnits()  { return _subUnits; }

    /**
    * @brief            Set the inputs of this unit
    *
    *                   Note that these setter methods will replicate the vectors given as input, but the copies
    *                   are shallow (i.e., vectors of pointers will refer to the same objects). That is intended, in
    *                   order to save memory space, and does not lead to race conditions.
    *
    * @param inputs     A vector of pointers to objects derived from SensorBase that will be this unit's input
    */
    void setInputs(const std::vector<S_Ptr>& inputs) {
        _inputs = inputs;
        _baseInputs = std::vector<SBasePtr>(_inputs.begin(), _inputs.end());
    }

    /**
    * @brief            Set the outputs of this unit
    *
    *                   The same considerations done for setInputs apply here as well.
    *
    * @param outputs    A vector of pointers to objects derived from SensorBase that will be this unit's output
    */
    void setOutputs(const std::vector<S_Ptr>& outputs) {
        _outputs = outputs;
        _baseOutputs = std::vector<SBasePtr>(_outputs.begin(), _outputs.end());
    }

    /**
    * @brief            Set the outputs of this unit
    *
    *                   The same considerations done for setInputs apply here as well.
    *
    * @param outputs    A vector of pointers to objects derived from SensorBase that will be this unit's output
    */
    void setSubUnits(const std::vector<std::shared_ptr<UnitTemplate<S>>>& sUnits) {
        _subUnits = sUnits;
        for(const auto& subUnit : _subUnits)
            subUnit->setParent(this);
    }

    /**
    * @brief            Add a single input sensor to this unit
    *
    * @param input      A pointer to an object derived from SensorBase that will be added to this unit's inputs
    */
    void addInput(const S_Ptr input) { _inputs.push_back(input); _baseInputs.push_back(input); }

    /**
    * @brief            Add a single output sensor to this unit
    *
    * @param output     A pointer to an object derived from SensorBase that will be added to this unit's outputs
    */
    void addOutput(const S_Ptr output) { _outputs.push_back(output); _baseOutputs.push_back(output); }


    /**
    * @brief            Adds a sub-unit to this unit
    *
    * @param sUnit     A shared pointer to a UnitTemplate object
    */
    void addSubUnit(const std::shared_ptr<UnitTemplate<S>> sUnit) { _subUnits.push_back(sUnit); sUnit->setParent(this); }
    
    /**
    * @brief            Prints the current unit configuration
    *
    * @param ll             Logging level at which the configuration is printed
    * @param lg             Logger object to be used
    * @param leadingSpaces  Number of leading spaces to pre-pend
    */
    virtual void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) override {
        if(leadingSpaces>30) return;
        std::string leading(leadingSpaces, ' ');
        LOG_VAR(ll) << leading << "Unit: " << _name;
        LOG_VAR(ll) << leading << "Inputs: ";
        for (const auto &i : _inputs)
            LOG_VAR(ll) << leading << "    " << i->getName();
        LOG_VAR(ll) << leading << "Outputs: ";
        for (const auto &o : _outputs)
            o->printConfig(ll, lg, leadingSpaces+4);
        if(_subUnits.size()>0) {
            LOG_VAR(ll) << leading << "Sub-units: ";
            for (const auto &u : _subUnits)
                u->printConfig(ll, lg, leadingSpaces+4);
        }
    }

protected:

    // Name corresponds to the output unit we are addressing
    std::string _name;
    // Input mode associated to this unit
    inputMode_t _inputMode;
    // Vector of sensor objects that make up inputs
    std::vector<S_Ptr> _inputs;
    // Base version that uses SensorBase is needed to expose units externally
    std::vector<SBasePtr> _baseInputs;
    // Vector of Sensor objects that make up outputs
    std::vector<S_Ptr> _outputs;
    // Vector of sub-units that are associated to this unit
    std::vector<std::shared_ptr<UnitTemplate<S>>> _subUnits;
    // Pointer to the unit's parent (if it is a sub-unit)
    UnitTemplate<S>* _parent;
    // Same as baseInputs
    std::vector<SBasePtr> _baseOutputs;
};

#endif //PROJECT_UNITTEMPLATE_H
