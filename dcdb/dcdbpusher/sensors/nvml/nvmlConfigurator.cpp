//================================================================================
// Name        : nvmlConfigurator.cpp
// Author      : Weronika Filinger, EPCC @ The University of Edinburgh
// Contact     :
// Copyright   :
// Description : Source file for nvml plugin configurator class.
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

#include "nvmlConfigurator.h"

nvmlConfigurator::nvmlConfigurator() {

	_gpuFeatureMAP["GPU_ENERGY"] = GPU_ENERGY;
	_gpuFeatureMAP["GPU_POWER"] = GPU_POWER;
	_gpuFeatureMAP["GPU_TEMP"] = GPU_TEMP;
	_gpuFeatureMAP["GPU_FAN"] = GPU_FAN;
	_gpuFeatureMAP["GPU_MEM_USED"] = GPU_MEM_USED;
	_gpuFeatureMAP["GPU_MEM_FREE"] = GPU_MEM_FREE;
	_gpuFeatureMAP["GPU_MEM_TOT"] = GPU_MEM_TOT;
	_gpuFeatureMAP["GPU_CLK_GP"] = GPU_CLK_GP;	
	_gpuFeatureMAP["GPU_CLK_SM"] = GPU_CLK_SM;
	_gpuFeatureMAP["GPU_CLK_MEM"] = GPU_CLK_MEM;
	_gpuFeatureMAP["GPU_UTL_MEM"] = GPU_UTL_MEM;
	_gpuFeatureMAP["GPU_UTL_GPU"] = GPU_UTL_GPU;
	_gpuFeatureMAP["GPU_ECC_ERR"] = GPU_ECC_ERR;
	_gpuFeatureMAP["GPU_PCIE_THRU"] = GPU_PCIE_THRU;
	_gpuFeatureMAP["GPU_RUN_PRCS"] = GPU_RUN_PRCS;

	_groupName = "group";
	_baseName = "sensor";
}

nvmlConfigurator::~nvmlConfigurator() {}

void nvmlConfigurator::sensorBase(nvmlSensorBase& s, CFG_VAL config) {

	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
		if (boost::iequals(val.first, "feature")) {
			gpuFeatureMap_t::iterator it = _gpuFeatureMAP.find(val.second.data());
			if (it != _gpuFeatureMAP.end()) {
				s.setFeatureType(it->second);
			} else {
				LOG(warning) << "  feature \"" << val.second.data() << "\" not known.";
			}
		}
	}                                          
}

void nvmlConfigurator::sensorGroup(nvmlSensorGroup& s, CFG_VAL config) {}


void nvmlConfigurator::printConfiguratorConfig(LOG_LEVEL ll) {

	LOG_VAR(ll) << "  NumSpacesAsIndention: " << 2;
}
