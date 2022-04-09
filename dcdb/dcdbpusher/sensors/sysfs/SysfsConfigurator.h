//================================================================================
// Name        : SysfsConfigurator.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Sysfs plugin configurator class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2017-2019 Leibniz Supercomputing Centre
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

#ifndef SYSFSCONFIGURATOR_H_
#define SYSFSCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplate.h"
#include "SysfsSensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup sysfs
 */
class SysfsConfigurator : public ConfiguratorTemplate<SysfsSensorBase, SysfsSensorGroup> {

      public:
	SysfsConfigurator();
	virtual ~SysfsConfigurator();

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(SysfsSensorBase &s, CFG_VAL config) override;
	void sensorGroup(SysfsSensorGroup &s, CFG_VAL config) override;
};

extern "C" ConfiguratorInterface *create() {
	return new SysfsConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* SYSFSCONFIGURATOR_H_ */
