//================================================================================
// Name        : SNMPSensorGroup.h
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for SNMP sensor group class.
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

#ifndef SNMPSENSORGROUP_H_
#define SNMPSENSORGROUP_H_

#include "../../includes/SensorGroupTemplateEntity.h"
#include "SNMPSensorBase.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup snmp
 */
class SNMPSensorGroup : public SensorGroupTemplateEntity<SNMPSensorBase, SNMPConnection> {

      public:
	SNMPSensorGroup(const std::string &name);
	SNMPSensorGroup(const SNMPSensorGroup &other);
	virtual ~SNMPSensorGroup();
	SNMPSensorGroup &operator=(const SNMPSensorGroup &other);

	void execOnInit() final override;

      private:
	void read() final override;
};

#endif /* SNMPSENSORGROUP_H_ */
