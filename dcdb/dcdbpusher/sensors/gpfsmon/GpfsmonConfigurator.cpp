//================================================================================
// Name        : GpfsmonConfigurator.cpp
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Gpfsmon plugin configurator class.
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

#include "GpfsmonConfigurator.h"

GpfsmonConfigurator::GpfsmonConfigurator() {
	_metricMap["TIMESTAMP_GPFS"] = TIMESTAMP_GPFS;
	_metricMap["IOBYTESREAD"] = IOBYTESREAD;
	_metricMap["IOBYTESWRITE"] = IOBYTESWRITE;
	_metricMap["IOOPENS"] = IOOPENS;
	_metricMap["IOCLOSES"] = IOCLOSES;
	_metricMap["IOREADS"] = IOREADS;
	_metricMap["IOWRITES"] = IOWRITES;
	_metricMap["READDIR"] = READDIR;
	_metricMap["INODE_UPDATES"] = INODE_UPDATES;
	_groupName = "group";
	_baseName = "sensor";
}

GpfsmonConfigurator::~GpfsmonConfigurator() {}

void GpfsmonConfigurator::sensorBase(GpfsmonSensorBase &s, CFG_VAL config) {
	ADD {
		if (boost::iequals(val.first, "metric")) {
			auto it = _metricMap.find(val.second.data());

			if (it != _metricMap.end()) {
				if (it->second == TIMESTAMP_GPFS) {
					LOG(warning) << "  metric \"" << val.second.data() << "\" not supported.";
				} else {
					s.setMetricType(it->second);
				}
			} else {
				LOG(warning) << "  metric \"" << val.second.data() << "\" not known.";
			}
		}
	}
}

void GpfsmonConfigurator::sensorGroup(GpfsmonSensorGroup &s, CFG_VAL config) {
	ADD {
		//ToDo do I need something here?
	}
}
