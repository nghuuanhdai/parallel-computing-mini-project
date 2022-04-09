//================================================================================
// Name        : PDUConfigurator.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for PDU plugin configurator class.
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

#ifndef SRC_SENSORS_PDU_PDUCONFIGURATOR_H_
#define SRC_SENSORS_PDU_PDUCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplateEntity.h"

#include "PDUSensorGroup.h"
#include "PDUUnit.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup pdu
 */
class PDUConfigurator : public ConfiguratorTemplateEntity<PDUSensorBase, PDUSensorGroup, PDUUnit> {

      public:
	PDUConfigurator();
	virtual ~PDUConfigurator();

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(PDUSensorBase &s, CFG_VAL config) override;
	void sensorGroup(PDUSensorGroup &s, CFG_VAL config) override;
	void sensorEntity(PDUUnit &s, CFG_VAL config) override;

      private:
	/**
	 * Split the given string into multiple parts which are later required to find the sensor value in the boost property tree.
	 * Store the individual parts into the sensors _xmlPathVector
	 * @param sensor		Sensor where the path belongs to
	 * @param pathString	Complete unrefined string as read from the config file
	 */
	void parsePathString(PDUSensorBase &sensor, const std::string &pathString);
};

extern "C" ConfiguratorInterface *create() {
	return new PDUConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* SRC_SENSORS_PDU_PDUCONFIGURATOR_H_ */
