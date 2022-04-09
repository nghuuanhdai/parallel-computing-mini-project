//================================================================================
// Name        : IPMISensorGroup.h
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for IPMI sensor group class.
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

#ifndef IPMISENSORGROUP_H_
#define IPMISENSORGROUP_H_

#include "../../includes/SensorGroupTemplateEntity.h"
#include "IPMISensorBase.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup ipmi
 */
class IPMISensorGroup : public SensorGroupTemplateEntity<IPMISensorBase, IPMIHost> {

      public:
	IPMISensorGroup(const std::string &name);
	IPMISensorGroup(const IPMISensorGroup &other);
	virtual ~IPMISensorGroup();
	IPMISensorGroup &operator=(const IPMISensorGroup &other);
	float getMsgRate() final override;
	uint64_t         nextReadingTime() override;
	bool             checkConfig();
	void execOnInit() final override;

      private:
	void     read() final override;
	uint64_t readRaw(const std::vector<uint8_t> &rawCmd, uint8_t lsb, uint8_t msb);
};

#endif /* IPMISENSORGROUP_H_ */
