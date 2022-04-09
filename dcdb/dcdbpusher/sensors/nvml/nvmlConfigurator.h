//================================================================================
// Name        : nvmlConfigurator.h
// Author      : Weronika Filinger, EPCC @ The University of Edinburgh
// Contact     :
// Copyright   :
// Description : Header file for nvml plugin configurator class.
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

#ifndef NVML_NVMLCONFIGURATOR_H_
#define NVML_NVMLCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplate.h"
#include "nvmlSensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup nvml
 */
class nvmlConfigurator : public ConfiguratorTemplate<nvmlSensorBase, nvmlSensorGroup> {

	typedef std::map<std::string, unsigned int> gpuFeatureMap_t;

	public:
	nvmlConfigurator();
	virtual ~nvmlConfigurator();

	protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(nvmlSensorBase& s, CFG_VAL config) override;
	void sensorGroup(nvmlSensorGroup& s, CFG_VAL config) override;


	virtual void printConfiguratorConfig(LOG_LEVEL ll) final override;

	private:
	gpuFeatureMap_t  _gpuFeatureMAP;

};

extern "C" ConfiguratorInterface* create() {
	return new nvmlConfigurator;
}

extern "C" void destroy(ConfiguratorInterface* c) {
	delete c;
}

#endif /* NVML_NVMLCONFIGURATOR_H_ */
