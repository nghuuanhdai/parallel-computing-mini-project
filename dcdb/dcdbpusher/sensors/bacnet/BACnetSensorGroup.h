//================================================================================
// Name        : BACnetSensorGroup.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for BACnet sensor group class.
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

#ifndef BACNETSENSORGROUP_H_
#define BACNETSENSORGROUP_H_

#include "../../includes/SensorGroupTemplateEntity.h"
#include "BACnetSensorBase.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup bacnet
 */
class BACnetSensorGroup : public SensorGroupTemplateEntity<BACnetSensorBase, BACnetClient> {

      public:
	BACnetSensorGroup(const std::string &name);
	BACnetSensorGroup(const BACnetSensorGroup &other);
	virtual ~BACnetSensorGroup();
	BACnetSensorGroup &operator=(const BACnetSensorGroup &other);

	void     setDeviceInstance(const std::string &deviceInstance) { _deviceInstance = stoul(deviceInstance); }
	uint32_t getDeviceInstance() const { return _deviceInstance; }

	void printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) final override;

      private:
	void read() final override;

	uint32_t _deviceInstance;
};

#endif /* BACNETSENSORGROUP_H_ */
