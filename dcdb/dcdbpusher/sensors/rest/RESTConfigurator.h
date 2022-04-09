//================================================================================
// Name        : RESTConfigurator.h
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for REST plugin configurator class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2021 Leibniz Supercomputing Centre
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

#ifndef RESTCONFIGURATOR_H_
#define RESTCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplateEntity.h"

#include "RESTSensorGroup.h"
#include "RESTUnit.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup rest
 */
class RESTConfigurator : public ConfiguratorTemplateEntity<RESTSensorBase, RESTSensorGroup, RESTUnit> {

      public:
	RESTConfigurator();
	virtual ~RESTConfigurator();

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(RESTSensorBase &s, CFG_VAL config) override;
	void sensorGroup(RESTSensorGroup &s, CFG_VAL config) override;
	void sensorEntity(RESTUnit &s, CFG_VAL config) override;

      private:
	/**
	 * Split the given string into multiple parts which are later required to find the sensor value in the boost property tree.
	 * Store the individual parts into the sensors _xmlPathVector
	 * @param sensor		Sensor where the path belongs to
	 * @param pathString	Complete unrefined string as read from the config file
	 */
	void parsePathString(RESTSensorBase &sensor, const std::string &pathString);
};

extern "C" ConfiguratorInterface *create() {
	return new RESTConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* RESTCONFIGURATOR_H_ */
