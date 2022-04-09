//================================================================================
// Name        : nvmlSensorGroup.h
// Author      : Fiona Reid, Weronika Filinger, EPCC @ The University of Edinburgh
// Contact     :
// Copyright   : 
// Description : Header file for nvml sensor group class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019 Leibniz Supercomputing Centre
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

#ifndef NVML_NVMLSENSORGROUP_H_
#define NVML_NVMLSENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"
#include "nvmlSensorBase.h"
#include <nvml.h>
#include <cuda_profiler_api.h>

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup nvml
 */
class nvmlSensorGroup : public SensorGroupTemplate<nvmlSensorBase> {
public:
    nvmlSensorGroup(const std::string& name);
    nvmlSensorGroup(const nvmlSensorGroup& other);
    virtual ~nvmlSensorGroup();
    nvmlSensorGroup& operator=(const nvmlSensorGroup& other);

    void execOnInit()  final override;
    bool execOnStart() final override;
    void execOnStop()  final override;
     
    void printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) final override;

private:
    void read() final override;

};

#endif /* NVML_NVMLSENSORGROUP_H_ */
