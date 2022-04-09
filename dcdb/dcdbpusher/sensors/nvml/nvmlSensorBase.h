//================================================================================
// Name        : nvmlSensorBase.h
// Author      : Weronika Filinger, EPCC @ The University of Edinburgh
// Contact     :
// Copyright   : 
// Description : Sensor base class for nvml plugin.
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

/**
 * @defgroup nvml nvml plugin
 * @ingroup  pusherplugins
 *
 * Collect data from the nvml interface
 */

#ifndef NVML_NVMLSENSORBASE_H_
#define NVML_NVMLSENSORBASE_H_

#include "sensorbase.h"

enum GPU_FEATURE {
	GPU_ENERGY              = 0,
	GPU_POWER               = 1,
	GPU_TEMP		= 2,
	GPU_FAN			= 3,
	GPU_MEM_USED		= 4,
	GPU_MEM_TOT		= 5,
	GPU_MEM_FREE		= 6,
	GPU_CLK_GP              = 7,
	GPU_CLK_SM		= 8,
	GPU_CLK_MEM		= 9,
	GPU_UTL_MEM             = 10,
	GPU_UTL_GPU             = 11,
	GPU_ECC_ERR		= 13,
	GPU_PCIE_THRU           = 14,
	GPU_RUN_PRCS            = 15,
};

/**
 * @brief 
 *
 *
 * @ingroup nvml
 */
class nvmlSensorBase : public SensorBase {
	public:
		nvmlSensorBase(const std::string& name) :
			SensorBase(name), _featureType(static_cast<GPU_FEATURE>(999)) {
			}

		nvmlSensorBase(const nvmlSensorBase &other)
			: SensorBase(other),
			_featureType(other._featureType) {}


		virtual ~nvmlSensorBase() {}

		int getFeatureType() const {
			return _featureType;
		}

		void setFeatureType(int featureType){
			_featureType = static_cast<GPU_FEATURE>(featureType);

		} 


		nvmlSensorBase& operator=(const nvmlSensorBase& other) {
			SensorBase::operator=(other);
			_featureType = other._featureType;

			return *this;
		}


		void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {

			std::string leading(leadingSpaces, ' ');
			std::string feature("unknown");
			switch (_featureType) {
				case GPU_ENERGY:
					feature = "GPU_ENERGY";
					break;
				case GPU_POWER:
					feature = "GPU_POWER";
					break;
				case GPU_TEMP:
					feature = "GPU_TEMP";
					break;
				case GPU_FAN:
					feature = "GPU_FAN";
					break;
				case GPU_MEM_USED:
					feature = "GPU_MEM_USED";
					break;
				case GPU_MEM_TOT:
					feature = "GPU_MEM_TOT";
					break;
				case GPU_MEM_FREE:
					feature = "GPU_MEM_FREE";
					break;
				case GPU_CLK_GP:
					feature = "GPU_CLK_GP";
					break;
				case GPU_CLK_SM:
					feature = "GPU_CLK_SM";
					break;
				case GPU_CLK_MEM:
					feature = "GPU_CLK_MEM";
					break;
				case GPU_UTL_MEM:
					feature = "GPU_UTL_MEM";
					break;
				case GPU_UTL_GPU:
					feature = "GPU_UTL_GPU";
					break;
				case GPU_ECC_ERR:
					feature = "GPU_ECC_ERR";
					break;
				case GPU_PCIE_THRU:
					feature = "GPU_PCIE_THRU";
					break;
				case GPU_RUN_PRCS:
					feature = "GPU_RUN_PRCS";
					break;
			}  
			LOG_VAR(ll) << leading << "    Feature type:  " << feature;    
		}

	protected:
		GPU_FEATURE _featureType;

};

#endif /* NVML_NVMLSENSORBASE_H_ */
