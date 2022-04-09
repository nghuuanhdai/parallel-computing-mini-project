//================================================================================
// Name        : MSRConfigurator.h
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for MSR plugin configurator class.
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

#ifndef MSR_MSRCONFIGURATOR_H_
#define MSR_MSRCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplate.h"
#include "MSRSensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup msr
 */
class MSRConfigurator : public ConfiguratorTemplate<MSRSensorBase, MSRSensorGroup> {

      public:
	MSRConfigurator();
	virtual ~MSRConfigurator();

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(MSRSensorBase &s, CFG_VAL config) override;
	void sensorGroup(MSRSensorGroup &s, CFG_VAL config) override;
	bool readConfig(std::string cfgPath) override;

      private:
	/**
     *  Takes a MSRSensorGroup and duplicates its sensors for every CPU.
     *  Assigns one CPU value to every newly constructed sensor and stores the
     *  group afterwards.
     *
     *  @param g    MSRSensorGroup which is to be customized for every CPU
     */
	void customizeAndStore(SG_Ptr g);
};

extern "C" ConfiguratorInterface *create() {
	return new MSRConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* MSR_MSRCONFIGURATOR_H_ */
