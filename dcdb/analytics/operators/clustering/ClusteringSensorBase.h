//================================================================================
// Name        : ClusteringSensorBase.h
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

/**
 * @defgroup clustering Clustering operator plugin.
 * @ingroup operator
 *
 * @brief Clustering operator plugin.
 */

#ifndef PROJECT_CLUSTERINGSENSORBASE_H
#define PROJECT_CLUSTERINGSENSORBASE_H

#include "sensorbase.h"

/**
 * @brief Sensor base for clustering plugin
 *
 * @ingroup clustering
 */
class ClusteringSensorBase : public SensorBase {

public:
    
    // Constructor and destructor
    ClusteringSensorBase(const std::string& name) : SensorBase(name) {}

    // Copy constructor
    ClusteringSensorBase(ClusteringSensorBase& other) : SensorBase(other) {}

    virtual ~ClusteringSensorBase() {}
    
    void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {
        SensorBase::printConfig(ll, lg, leadingSpaces);
        std::string leading(leadingSpaces, ' ');
    }
    
};

using ClusteringSBPtr = std::shared_ptr<ClusteringSensorBase>;

#endif //PROJECT_CLUSTERINGSENSORBASE_H
