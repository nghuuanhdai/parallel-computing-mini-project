//================================================================================
// Name        : SMUCNGPerfConfigurator.h
// Author      : Carla Guillen
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

#ifndef ANALYTICS_SMUCNGPERF_SMUCNGPERFCONFIGURATOR_H_
#define ANALYTICS_SMUCNGPERF_SMUCNGPERFCONFIGURATOR_H_

#include "../../includes/OperatorConfiguratorTemplate.h"
#include "SMUCNGPerfOperator.h"

class SMUCNGPerfConfigurator : virtual public OperatorConfiguratorTemplate<SMUCNGPerfOperator, SMUCSensorBase>{
public:
	SMUCNGPerfConfigurator();
	virtual ~SMUCNGPerfConfigurator();
private:
    void sensorBase(SMUCSensorBase& s, CFG_VAL config) override;
    void operatorAttributes(SMUCNGPerfOperator& op, CFG_VAL config) override;
    bool unit(UnitTemplate<SMUCSensorBase>& u) override;
    std::map<SMUCSensorBase::Metric_t, unsigned int> _metricToPosition;
    std::map<std::string, SMUCSensorBase::Metric_t> _metricMap;
    int _vector_position;
};

extern "C" OperatorConfiguratorInterface* create() {
    return new SMUCNGPerfConfigurator;
}

extern "C" void destroy(OperatorConfiguratorInterface* c) {
    delete c;
}

#endif /* ANALYTICS_SMUCNGPERF_SMUCNGPERFCONFIGURATOR_H_ */
