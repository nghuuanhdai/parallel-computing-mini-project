//================================================================================
// Name        : JobAggregatorOperator.cpp
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

#include "JobAggregatorOperator.h"

JobAggregatorOperator::JobAggregatorOperator(const std::string& name) :
    OperatorTemplate(name),
    AggregatorOperator(name),
    JobOperatorTemplate(name) {}

JobAggregatorOperator::JobAggregatorOperator(const JobAggregatorOperator& other) : OperatorTemplate(other._name), AggregatorOperator(other), JobOperatorTemplate(other._name) {}

JobAggregatorOperator::~JobAggregatorOperator() {}

void JobAggregatorOperator::compute(U_Ptr unit, qeJobData& jobData) {
    uint64_t now = getTimestamp() - _goBack;
    // Too early to fetch job data
    if(now < jobData.startTime) {
        return;
    }
    // Making sure that the aggregation boundaries do not go past the job start/end time
    uint64_t jobEnd   = jobData.endTime!=0 && now > jobData.endTime ? jobData.endTime : now;
    uint64_t jobStart = jobEnd-_window < jobData.startTime ? jobData.startTime : jobEnd-_window;
    // Clearing the buffer
    _buffer.clear();
    std::vector<std::string> sensorNames;
    // Job units are hierarchical, and thus we iterate over all sub-units associated to each single node
    for(const auto& subUnit : unit->getSubUnits()) {
        for (const auto &in : subUnit->getInputs()) {
            sensorNames.push_back(in->getName());
        }
    }
    if (!_queryEngine.querySensor(sensorNames, jobStart, jobEnd, _buffer, false)) {
        LOG(debug) << "Job Operator " << _name << ": cannot read from any sensor for unit " + unit->getName() + "!";
        return;
    }
    compute_internal(unit, _buffer);
}