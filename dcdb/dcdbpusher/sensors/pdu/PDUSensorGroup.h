//================================================================================
// Name        : PDUSensorGroup.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for PDU sensor group class.
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

#ifndef SRC_SENSORS_PDU_PDUSENSORGROUP_H_
#define SRC_SENSORS_PDU_PDUSENSORGROUP_H_

#include "../../includes/SensorGroupTemplateEntity.h"
#include "PDUSensorBase.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup pdu
 */
class PDUSensorGroup : public SensorGroupTemplateEntity<PDUSensorBase, PDUUnit> {

      public:
	PDUSensorGroup(const std::string &name);
	PDUSensorGroup(const PDUSensorGroup &other);
	virtual ~PDUSensorGroup();
	PDUSensorGroup &operator=(const PDUSensorGroup &other);

	void               setRequest(const std::string &request) { _request = request; }
	const std::string &getRequest() { return _request; }

	void printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) final override;

      private:
	void read() final override;

	std::string _request;
};

#endif /* SRC_SENSORS_PDU_PDUSENSORGROUP_H_ */
