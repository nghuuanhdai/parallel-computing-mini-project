//================================================================================
// Name        : HealthCheckerOperator.cpp
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

#include "HealthCheckerOperator.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>

HealthCheckerOperator::HealthCheckerOperator(const std::string& name) : OperatorTemplate(name) {
    _shell = "/bin/sh";
    _command = "";
    _cooldown = 0;
    _window = 0;
    _log = true;
}

HealthCheckerOperator::HealthCheckerOperator(const HealthCheckerOperator &other) : OperatorTemplate(other) {
    _shell = other._shell;
    _command = other._command;
    _cooldown = other._cooldown;
    _window = other._window;
    _log = other._log;
}

HealthCheckerOperator::~HealthCheckerOperator() {}

void HealthCheckerOperator::printConfig(LOG_LEVEL ll) {
    //LOG_VAR(ll) << "            Shell:           " << _shell;
    LOG_VAR(ll) << "            Command:         " << _command;
    LOG_VAR(ll) << "            Cooldown:        " << _cooldown;
    LOG_VAR(ll) << "            Window:          " << _window;
    LOG_VAR(ll) << "            Log:             " << (_log ? "enabled" : "disabled");
    OperatorTemplate<HealthCheckerSensorBase>::printConfig(ll);
}

void HealthCheckerOperator::compute(U_Ptr unit) {
    std::string msg = "The following alarm conditions were detected by the DCDB Health Checker plugin:\n\n";
    bool alrm = false;
    uint64_t endTs = getTimestamp();
    uint64_t startTs = endTs - _window;
    vector<reading_t> buffer;
    
    for (const auto& in : unit->getInputs()) {
        buffer.clear();
        HealthCheckerSensorBase::HCCond cond = in->getCondition();
        _queryEngine.querySensor(in->getName(), startTs, endTs, buffer, false);
        
        std::string tempMsg = "";
        // Checking the existence condition
        if (buffer.empty() && cond == HealthCheckerSensorBase::HC_EXISTS) {
            tempMsg = "    - Sensor " + in->getName() + " is not providing any data.\n";
        // Checking the remaining value conditions
        } else if(cond != HealthCheckerSensorBase::HC_EXISTS) {
            for (const auto& v : buffer) {
                if (v.value > in->getThreshold() && cond == HealthCheckerSensorBase::HC_ABOVE) {
                    tempMsg = "    - Sensor " + in->getName() + " has a reading " + std::to_string(v.value) + " greater than threshold " + std::to_string(in->getThreshold()) + ".\n";
                    break;
                } else if (v.value < in->getThreshold() && cond == HealthCheckerSensorBase::HC_BELOW) {
                    tempMsg = "    - Sensor " + in->getName() + " has a reading " + std::to_string(v.value) + " smaller than threshold " + std::to_string(in->getThreshold()) + ".\n";
                    break;
                } else if (v.value == in->getThreshold() && cond == HealthCheckerSensorBase::HC_EQUAL) {
                    tempMsg = "    - Sensor " + in->getName() + " has a reading equal to threshold " + std::to_string(in->getThreshold()) + ".\n";
                    break;
                }
            }
        }
        
        if (tempMsg != "" && endTs - in->getLast() > _cooldown) {
            alrm = true;
            in->setLast(endTs);
            msg += tempMsg;
        }
    }
    
    // If at least one alarm was raised
    if(alrm) {
        if(_command != "") {
            pid_t pid = fork();
            // Father and son do their work
            if (pid == 0) {
                std::string cmd = _command.replace(_command.find(HC_MSG_MARKER), HC_LEN_MARKER, "\"" + msg + "\"");
                if(execlp("/bin/sh", "sh", "-c", cmd.c_str(), (char *)0) < 0) {
                    LOG(error) << "Operator " << _name << ": could not spawn child process!";
                    exit(0);
                }
            } else {
                // Fixed 100ms sleep cycle for the wait call, with 60s timeout
                struct timespec req = {0, 100000000};
                uint64_t timeout = S_TO_NS(60);
                uint64_t now = getTimestamp();
                int status = 0;
                
                // Waiting until timeout for the process to complete
                while (waitpid(pid, &status, WNOHANG) == 0) {
                    if (getTimestamp() - now > timeout) {
                        LOG(error) << "Operator " << _name << ": child process with PID " << pid << " does not respond. Killing...";
                        kill(pid, SIGKILL);
                        break;
                    } else {
                        nanosleep(&req, NULL);
                    }
                }
            }
        }
        
        // Logging to the standard DCDB log
        if(_log) {
            LOG(warning) << msg;
        }    
    }
}
