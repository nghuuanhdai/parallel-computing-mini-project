//================================================================================
// Name        : BACnetConfigurator.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for BACnet plugin configurator class.
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

#ifndef BACNETCONFIGURATOR_H_
#define BACNETCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplate.h"

#include "BACnetClient.h"
#include "BACnetSensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup bacnet
 */
class BACnetConfigurator : public ConfiguratorTemplate<BACnetSensorBase, BACnetSensorGroup> {

      public:
	BACnetConfigurator();
	virtual ~BACnetConfigurator();

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(BACnetSensorBase &s, CFG_VAL config) override;
	void sensorGroup(BACnetSensorGroup &s, CFG_VAL config) override;

	void global(CFG_VAL config) override;

	void printConfiguratorConfig(LOG_LEVEL ll) override;

      private:
	std::shared_ptr<BACnetClient> _bacClient; /**< Only one BACnetClient should be instantiated at all times,
	                                               therefore we do not leverage the entity functionality but
	                                               instead manage the single object ourselves */
};

extern "C" ConfiguratorInterface *create() {
	return new BACnetConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* BACNETCONFIGURATOR_H_ */
