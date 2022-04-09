//================================================================================
// Name        : CoolingControlOperator.h
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

#ifndef PROJECT_COOLINGCONTROLOPERATOR_H
#define PROJECT_COOLINGCONTROLOPERATOR_H

#include "../../includes/OperatorTemplate.h"
#include "CoolingControlSensorBase.h"
#include "SNMPController.h"

/**
 * @brief Aggregator operator plugin.
 *
 * @ingroup aggregator
 */
class CoolingControlOperator : virtual public OperatorTemplate<CoolingControlSensorBase> {

public:

    CoolingControlOperator(const std::string& name);
    CoolingControlOperator(const CoolingControlOperator& other);

    virtual ~CoolingControlOperator();

    virtual restResponse_t REST(const string& action, const unordered_map<string, string>& queries) override;

    virtual void execOnInit() override;
    virtual bool execOnStart() override;
    
    void setStrategy(std::string strat)         { _strategy = strat; }
    void setMaxTemp(uint64_t mt)                { _maxTemp = mt; }
    void setMinTemp(uint64_t mt)                { _minTemp = mt; }
    void setWindow(uint64_t w)                  { _window = w; }
    void setBins(uint64_t b)                    { _bins = b; }
    void setHotThreshold(uint64_t ht)           { _hotThreshold = ht; }
    void setHotPerc(uint64_t hp)                { _hotPerc = hp; }
    
    SNMPController &getController()             { return _controller; }
    std::string getStrategy()                   { return _strategy; }
    uint64_t getMaxTemp()                       { return _maxTemp; }
    uint64_t getMinTemp()                       { return _minTemp; }
    uint64_t getWindow()                        { return _window; }
    uint64_t getBins()                          { return _bins; }
    uint64_t getHotThreshold()                  { return _hotThreshold; }
    uint64_t getHotPerc()                       { return _hotPerc; }

    void printConfig(LOG_LEVEL ll) override;

protected:

    virtual void compute(U_Ptr unit)	 override;
    
    int continuousControl(std::vector<std::vector<reading_t>> &readings, U_Ptr unit);
    int steppedControl(std::vector<std::vector<reading_t>> &readings, U_Ptr unit);
    
    uint64_t getNumHotNodes(std::vector<std::vector<reading_t>> &readings, U_Ptr unit);
    uint64_t getBinForValue(uint64_t temp);
    uint64_t clipTemperature(uint64_t temp);
    
    // Control parameters
    std::string _strategy;
    uint64_t    _currTemp;
    uint64_t    _maxTemp;
    uint64_t    _minTemp;
    uint64_t    _window;
    uint64_t    _bins;
    uint64_t    _hotThreshold;
    uint64_t    _hotPerc;
    SNMPController _controller  = SNMPController("SNMPControl");
    SNMPSensorBase _dummySensor = SNMPSensorBase("SNMPSensor");
};

#endif //PROJECT_COOLINGCONTROLOPERATOR_H
