//================================================================================
// Name        : SNMPConfigurator.h
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for SNMP plugin configurator class.
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

#ifndef SNMPCONFIGURATOR_H_
#define SNMPCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplateEntity.h"

#include "SNMPConnection.h"
#include "SNMPSensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup snmp
 */
class SNMPConfigurator : public ConfiguratorTemplateEntity<SNMPSensorBase, SNMPSensorGroup, SNMPConnection> {

      public:
	SNMPConfigurator();
	virtual ~SNMPConfigurator();

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(SNMPSensorBase &s, CFG_VAL config) override;
	void sensorGroup(SNMPSensorGroup &s, CFG_VAL config) override;
	void sensorEntity(SNMPConnection &s, CFG_VAL config) override;
};

extern "C" ConfiguratorInterface *create() {
	return new SNMPConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* SNMPCONFIGURATOR_H_ */
