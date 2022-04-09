//================================================================================
// Name        : IPMIConfigurator.h
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for IPMI plugin configurator class.
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

#ifndef IPMICONFIGURATOR_H_
#define IPMICONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplateEntity.h"

#include "IPMIHost.h"
#include "IPMISensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup ipmi
 */
class IPMIConfigurator : public ConfiguratorTemplateEntity<IPMISensorBase, IPMISensorGroup, IPMIHost> {

	typedef struct {
		uint32_t sessionTimeout;
		uint32_t retransmissionTimeout;
	} globalHost_t;

      public:
	IPMIConfigurator();
	virtual ~IPMIConfigurator();
	bool readConfig(std::string cfgPath) override;

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(IPMISensorBase &s, CFG_VAL config) override;
	void sensorGroup(IPMISensorGroup &s, CFG_VAL config) override;
	void sensorEntity(IPMIHost &s, CFG_VAL config) override;

	void global(CFG_VAL config) override;

	void derivedSetGlobalSettings(const pluginSettings_t &pluginSettings) override { _tempdir = pluginSettings.tempdir; }

	void printConfiguratorConfig(LOG_LEVEL ll) override;

      private:
	std::string  _tempdir;
	globalHost_t _globalHost;
};

extern "C" ConfiguratorInterface *create() {
	return new IPMIConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* IPMICONFIGURATOR_H_ */
