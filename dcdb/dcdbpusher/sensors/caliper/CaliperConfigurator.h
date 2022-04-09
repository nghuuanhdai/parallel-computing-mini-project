//================================================================================
// Name        : CaliperConfigurator.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Caliper plugin configurator class.
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

#ifndef CALIPER_CALIPERCONFIGURATOR_H_
#define CALIPER_CALIPERCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplate.h"
#include "CaliperSensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup cali
 */
class CaliperConfigurator : public ConfiguratorTemplate<CaliperSensorBase, CaliperSensorGroup> {

      public:
	CaliperConfigurator();
	virtual ~CaliperConfigurator();

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(CaliperSensorBase &s, CFG_VAL config) override;
	void sensorGroup(CaliperSensorGroup &s, CFG_VAL config) override;
};

extern "C" ConfiguratorInterface *create() {
	return new CaliperConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* CALIPER_CALIPERCONFIGURATOR_H_ */
