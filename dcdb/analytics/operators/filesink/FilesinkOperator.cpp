//================================================================================
// Name        : FilesinkOperator.cpp
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

#include "FilesinkOperator.h"

FilesinkOperator::FilesinkOperator(const std::string& name) : OperatorTemplate(name) {
    _autoName = false;
}

FilesinkOperator::FilesinkOperator(const FilesinkOperator& other) : OperatorTemplate(other) {
    _autoName = other._autoName;
}

FilesinkOperator::~FilesinkOperator() {}

void FilesinkOperator::printConfig(LOG_LEVEL ll) {
    LOG_VAR(ll) << "            Auto naming:     " << (_autoName ? "enabled" : "disabled");
    OperatorTemplate<FilesinkSensorBase>::printConfig(ll);
}

void FilesinkOperator::execOnStop() {
    for(const auto& u : _units)
        for(const auto& in : u->getInputs())
            in->closeFile();
}

void FilesinkOperator::compute(U_Ptr unit) {
    for(const auto& in : unit->getInputs()) {
        if(!in->isOpen()) {
            in->setPath(adjustPath(in));
            if(!in->openFile()) {
                LOG(error) << "Operator " + _name + ": failed to open file for sensor " << in->getName() << "!";
                continue;
            }
        }
        
        // Clearing the buffer
        _buffer.clear();
        if(!_queryEngine.querySensor(in->getName(), 0, 0, _buffer) || _buffer.empty())
            LOG(debug) << "Operator " + _name + ": cannot read from sensor " + in->getName() + "!";
        else if(!in->writeFile(_buffer[_buffer.size()-1]))
            LOG(error) << "Operator " + _name + ": failed file write for sensor " << in->getName() << "!";
    }
}

std::string FilesinkOperator::adjustPath(FilesinkSBPtr s) {
    std::string adjPath = s->getPath();
    if(this->_autoName || adjPath.empty()) {
        // If no path is specified, we fall back to the current directory
        if(adjPath.empty())
            adjPath = "./";
        size_t lastSep = adjPath.find_last_of('/');
        // If no separator can be found in the path, we append one
        if(lastSep == std::string::npos)
            adjPath += '/';
        // Else, we pick the path up to the directory and remove the last segment
        else
            adjPath = adjPath.substr(0, lastSep+1);
        adjPath += MQTTChecker::topicToName(s->getMqtt());
    }
    return adjPath;
}