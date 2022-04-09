//================================================================================
// Name        : nvmlSensorGroup.cpp
// Author      : Fiona Reid, Weronika Filinger, EPCC @ The University of Edinburgh
// Contact     :
// Copyright   : 
// Description : Source file for nvml sensor group class.
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

#include "nvmlSensorGroup.h"

#include "timestamp.h"

// Used to ensure we get a sensible value of energy by computing the difference 
// between calls to the read function
static int isfirsttime=0; 

struct env_t {
	nvmlDevice_t device;
} env; 

// counters storing the GPU metrics queried from the NVML library
struct counters_t {
	unsigned long long energy_initial;
	unsigned long long energy_current;
	unsigned long long energy_previous;
	unsigned int temperature;
	unsigned int fanspeed;
	unsigned int clockspeed_graphics;
	unsigned int clockspeed_sm;
	unsigned int clockspeed_mem;
	nvmlMemory_t memory;
	unsigned int power;
	unsigned long long ecc_counts;
	nvmlUtilization_t utilization;
	unsigned int pcie_throughput;
	unsigned int procs_cnt;
} counters;

nvmlSensorGroup::nvmlSensorGroup(const std::string& name) :
	SensorGroupTemplate(name) {
	}

nvmlSensorGroup::nvmlSensorGroup(const nvmlSensorGroup& other) :
	SensorGroupTemplate(other) {
	}

nvmlSensorGroup::~nvmlSensorGroup() {}

nvmlSensorGroup& nvmlSensorGroup::operator=(const nvmlSensorGroup& other) {
	SensorGroupTemplate::operator=(other);

	return *this;
}

void nvmlSensorGroup::execOnInit() {
	nvmlReturn_t err;
	err = nvmlInit();
	err = nvmlDeviceGetHandleByIndex(0,&(env.device));
	err = nvmlDeviceGetTotalEnergyConsumption(env.device,&(counters.energy_initial));
	
	if(err != NVML_SUCCESS) {
		LOG(warning) << "Sensorgroup " << _groupName << ": NVML error during initialization!";
	}
}

bool nvmlSensorGroup::execOnStart() {
	cudaProfilerStart();
	return true;
}

void nvmlSensorGroup::execOnStop() {
	cudaProfilerStop();
}

void nvmlSensorGroup::read() {
	reading_t reading;
	reading.timestamp = getTimestamp();
	reading.value = 0;
	nvmlReturn_t err = NVML_SUCCESS;     
	unsigned long long temp;
	try {
		for(auto s : _sensors) {
			switch(s->getFeatureType()){
				case(GPU_ENERGY):
					// Need to measure the difference in energy used between calls to the read function
					if (isfirsttime==0){
						// First time through we use the initial value to set previous and get the new energy into current
						counters.energy_previous = counters.energy_initial;
						err = nvmlDeviceGetTotalEnergyConsumption(env.device,&(counters.energy_current));
						isfirsttime=1;
					}
					else {
						// Otherwise, set previous energy to whatever it was before and get the new value
						counters.energy_previous=counters.energy_current;
						err = nvmlDeviceGetTotalEnergyConsumption(env.device,&(counters.energy_current));
					}
					temp=counters.energy_current - counters.energy_previous; // Take difference and compute energy in millijoules 
					reading.value = temp;
					break;
				case(GPU_POWER):
					err = nvmlDeviceGetPowerUsage(env.device,&(counters.power));
					reading.value = counters.power;
					break;
				case(GPU_TEMP):
					err = nvmlDeviceGetTemperature(env.device,NVML_TEMPERATURE_GPU,&(counters.temperature));
					reading.value = counters.temperature;
					break;
				case(GPU_FAN):
					err = nvmlDeviceGetFanSpeed(env.device,&(counters.fanspeed));
					reading.value = counters.fanspeed;
					break;
				case(GPU_MEM_USED):
					err = nvmlDeviceGetMemoryInfo (env.device, &(counters.memory));
					reading.value = counters.memory.used;
					break;
				case(GPU_MEM_TOT):
					err = nvmlDeviceGetMemoryInfo (env.device, &(counters.memory));
					reading.value = counters.memory.total;
					break;
				case(GPU_MEM_FREE):
					err = nvmlDeviceGetMemoryInfo (env.device, &(counters.memory));
					reading.value = counters.memory.free;
					break;
				case(GPU_CLK_GP):
					err = nvmlDeviceGetClock (env.device, NVML_CLOCK_GRAPHICS,NVML_CLOCK_ID_CURRENT,&(counters.clockspeed_graphics));
					reading.value = counters.clockspeed_graphics;
					break;
				case(GPU_CLK_SM):
					err = nvmlDeviceGetClock (env.device, NVML_CLOCK_SM,NVML_CLOCK_ID_CURRENT,&(counters.clockspeed_sm));
					reading.value = counters.clockspeed_sm;
					break;
				case(GPU_CLK_MEM):
					err = nvmlDeviceGetClock (env.device, NVML_CLOCK_MEM,NVML_CLOCK_ID_CURRENT,&(counters.clockspeed_mem));
					reading.value = counters.clockspeed_mem;
					break;
				case(GPU_UTL_MEM):
					err = nvmlDeviceGetUtilizationRates (env.device, &(counters.utilization));
					reading.value = counters.utilization.memory;
					break;
				case(GPU_UTL_GPU):
					err = nvmlDeviceGetUtilizationRates (env.device, &(counters.utilization));
					reading.value = counters.utilization.gpu;
					break;
				case(GPU_ECC_ERR):
					err = nvmlDeviceGetTotalEccErrors (env.device, NVML_MEMORY_ERROR_TYPE_CORRECTED,NVML_VOLATILE_ECC,&(counters.ecc_counts));
					reading.value = counters.ecc_counts;
					break;
				case(GPU_PCIE_THRU):
					err = nvmlDeviceGetPcieThroughput (env.device, NVML_PCIE_UTIL_COUNT,&(counters.pcie_throughput));
					reading.value = counters.pcie_throughput;
					break;
				case(GPU_RUN_PRCS):
					err = nvmlDeviceGetComputeRunningProcesses (env.device,&(counters.procs_cnt),NULL);
					reading.value = counters.procs_cnt;
					break;
			}
			s->storeReading(reading);
			if(err != NVML_SUCCESS) {
				LOG(debug) << "Sensorgroup " << _groupName << " could not read " << s->getName() << ": NVML error!";
			}
#ifdef DEBUG
			LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
		}
	} catch (const std::exception& e) {
		LOG(error) << "Sensorgroup " << _groupName << " could not read value: " << e.what();
	}
}

void nvmlSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {

	LOG_VAR(ll) << "            NumSpacesAsIndention: " << 12;
}
