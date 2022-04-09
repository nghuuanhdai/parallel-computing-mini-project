//================================================================================
// Name        : ProcfsConfigurator.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Procfs plugin configurator class.
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

#ifndef PROCFSCONFIGURATOR_H_
#define PROCFSCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplate.h"
#include "ProcfsSensorGroup.h"
#include <vector>

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @details Configures and instantiates Procfs plugin, using config files.
 *
 * @ingroup procfs
 */
class ProcfsConfigurator : public ConfiguratorTemplate<ProcfsSensorBase, ProcfsSensorGroup> {

      public:
	// Constructor and destructor
	ProcfsConfigurator();
	virtual ~ProcfsConfigurator();

      protected:
	// There are no sensor entities to be read from the config file, only groups
	// Therefore, we only deal with the sensorGroup and sensorBase methods from ConfiguratorTemplate
	void sensorGroup(ProcfsSensorGroup &sGroup, CFG_VAL config) override;
	void sensorBase(ProcfsSensorBase &s, CFG_VAL config) override;
};

// Functions required to correctly generate Configurator objects from linked libraries

extern "C" ConfiguratorInterface *create() {
	return new ProcfsConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* PROCFSCONFIGURATOR_H_ */
