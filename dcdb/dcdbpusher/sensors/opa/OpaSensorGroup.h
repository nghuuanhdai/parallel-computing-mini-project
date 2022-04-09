//================================================================================
// Name        : OpaSensorGroup.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Opa sensor group class.
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

#ifndef OPA_OPASENSORGROUP_H_
#define OPA_OPASENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"
#include "OpaSensorBase.h"

#include <inttypes.h>
#include <opamgt/opamgt.h>
#include <opamgt/opamgt_pa.h>

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup opa
 */
class OpaSensorGroup : public SensorGroupTemplate<OpaSensorBase> {
      public:
	OpaSensorGroup(const std::string name);
	OpaSensorGroup(const OpaSensorGroup &other);
	virtual ~OpaSensorGroup();
	OpaSensorGroup &operator=(const OpaSensorGroup &other);

	bool execOnStart() final override;
	void execOnStop() final override;

	int32_t getHfiNum() const { return _hfiNum; }
	uint8_t getPortNum() const { return _portNum; }

	void setHfiNum(const std::string &hfiNum) { _hfiNum = stoi(hfiNum); }
	void setPortNum(const std::string &portNum) { _portNum = stoull(portNum); }

	void printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) final override;

      private:
	void read() final override;

	int32_t _hfiNum;
	uint8_t _portNum;

	struct omgt_port *     _port;
	STL_PA_IMAGE_ID_DATA   _imageId;
	STL_PA_IMAGE_INFO_DATA _imageInfo;
};

#endif /* OPA_OPASENSORGROUP_H_ */
