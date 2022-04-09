//================================================================================
// Name        : HealthCheckerOperator.h
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

#ifndef PROJECT_HEALTHCHECKEROPERATOR_H
#define PROJECT_HEALTHCHECKEROPERATOR_H


#include "../../includes/OperatorTemplate.h"
#include "HealthCheckerSensorBase.h"

#define HC_MSG_MARKER "%s"
#define HC_LEN_MARKER 2

/**
 * @brief Health checker operator plugin.
 *
 * @ingroup healthchecker
 */
class HealthCheckerOperator : virtual public OperatorTemplate<HealthCheckerSensorBase> {

public:

    HealthCheckerOperator(const std::string& name);
    HealthCheckerOperator(const HealthCheckerOperator& other);

    virtual ~HealthCheckerOperator();

    void setCooldown(uint64_t c)               { _cooldown = c; }
    void setWindow(uint64_t w)                 { _window = w; }
    void setLog(bool l)                        { _log = l; }
    void setCommand(std::string c)             { _command = isCommandValid(c) ? c : ""; }
    void setShell(std::string s)               { _shell = s; }

    uint64_t getCooldown()                     { return _cooldown; }
    uint64_t getWindow()                       { return _window; }
    std::string getCommand()                   { return _command; }
    std::string getShell()                     { return _shell; }
    bool getLog()                              { return _log; }

    void printConfig(LOG_LEVEL ll) override;
    
    bool isCommandValid(std::string c) {
        // Command must contain the marker to be replaced with the message, and a space (i.e., more than 1 argument) 
        return c.find(HC_MSG_MARKER) != std::string::npos && c.find(" ") != std::string::npos;
    }
    
protected:

    virtual void compute(U_Ptr unit)	 override;
    
    std::string _shell;
    std::string _command;
    uint64_t    _cooldown;
    uint64_t    _window;
    bool        _log;
    
};


#endif //PROJECT_HEALTHCHECKEROPERATOR_H
