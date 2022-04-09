//================================================================================
// Name        : ClusteringConfigurator.h
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

#ifndef PROJECT_CLUSTERINGCONFIGURATOR_H
#define PROJECT_CLUSTERINGCONFIGURATOR_H

#include "../../includes/OperatorConfiguratorTemplate.h"
#include "ClusteringOperator.h"

/**
 * @brief Configurator for the clustering plugin.
 *
 * @ingroup clustering
 */
class ClusteringConfigurator : virtual public OperatorConfiguratorTemplate<ClusteringOperator, ClusteringSensorBase> {

public:
    ClusteringConfigurator();
    virtual ~ClusteringConfigurator();

private:
    void sensorBase(ClusteringSensorBase& s, CFG_VAL config) override;
    void operatorAttributes(ClusteringOperator& op, CFG_VAL config) override;
    bool unit(UnitTemplate<ClusteringSensorBase>& u) override;
    bool readUnits(ClusteringOperator& op, std::vector<ClusteringSBPtr>& protoInputs, std::vector<ClusteringSBPtr>& protoOutputs, 
                   std::vector<ClusteringSBPtr>& protoGlobalOutputs, inputMode_t inputMode) override;

};

extern "C" OperatorConfiguratorInterface* create() {
    return new ClusteringConfigurator;
}

extern "C" void destroy(OperatorConfiguratorInterface* c) {
    delete c;
}


#endif //PROJECT_CLUSTERINGCONFIGURATOR_H
