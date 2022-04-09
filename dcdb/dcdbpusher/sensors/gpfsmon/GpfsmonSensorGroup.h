//================================================================================
// Name        : GpfsmonSensorGroup.h
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Gpfsmon sensor group class.
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

#ifndef GPFSMON_GPFSMONSENSORGROUP_H_
#define GPFSMON_GPFSMONSENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"

#include "GpfsmonSensorBase.h"

using Gpfs_SB = std::shared_ptr<GpfsmonSensorBase>;

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup gpfsmon
 */
class GpfsmonSensorGroup : public SensorGroupTemplate<GpfsmonSensorBase> {
      public:
	GpfsmonSensorGroup(const std::string &name);
	virtual ~GpfsmonSensorGroup();

	void execOnInit() final override;

	GpfsmonSensorGroup &operator=(const GpfsmonSensorGroup &other);
	GpfsmonSensorGroup(const GpfsmonSensorGroup &other);

      private:
	void read() final override;

	bool fileExists(const char *filename);
	void createTempFile();
	bool parseLine(std::string &&toparse);

	const std::string _cmd_io = "sudo /usr/lpp/mmfs/bin/mmpmon -p -i /tmp/gpfsmon"; //todo change to real command

	std::vector<uint64_t> _data;             //!< temp variable in parseLine defined here for efficiency.
	constexpr static int  BUFFER_SIZE = 255; //!< constant buffer that is used to parse line by line (from popen)
	char const *          TMP_GPFSMON = "/tmp/gpfsmon";
};

#endif /* GPFSMON_GPFSMONSENSORGROUP_H_ */
