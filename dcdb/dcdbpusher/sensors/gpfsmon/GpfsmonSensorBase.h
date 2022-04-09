//================================================================================
// Name        : GpfsmonSensorBase.h
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for Gpfsmon plugin.
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

/**
 * @defgroup gpfsmon Gpfsmon plugin
 * @ingroup  pusherplugins
 *
 * @brief
 */

#ifndef GPFSMON_GPFSMONSENSORBASE_H_
#define GPFSMON_GPFSMONSENSORBASE_H_

#include "sensorbase.h"

#include <regex>

enum GPFS_METRIC {
	TIMESTAMP_GPFS = 0,
	IOBYTESREAD = 1,
	IOBYTESWRITE = 2,
	IOOPENS = 3,
	IOCLOSES = 4,
	IOREADS = 5,
	IOWRITES = 6,
	READDIR = 7,
	INODE_UPDATES = 8,
	SIZE = INODE_UPDATES + 1
};

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup gpfsmon
 */
class GpfsmonSensorBase : public SensorBase {
      public:
	GpfsmonSensorBase(const std::string &name)
	    : SensorBase(name), _metric_type(GPFS_METRIC::SIZE) {
		setDelta(true);
		setDeltaMaxValue(ULLONG_MAX);
	}

	virtual ~GpfsmonSensorBase() {
	}

	GPFS_METRIC getMetricType() const {
		return _metric_type;
	}

	void setMetricType(GPFS_METRIC metricType) {
		_metric_type = metricType;
	}

	GpfsmonSensorBase(const GpfsmonSensorBase &) = default;
	GpfsmonSensorBase &operator=(const GpfsmonSensorBase &) = default;

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		std::string metric("unknown");
		switch (_metric_type) {
			case TIMESTAMP_GPFS:
				metric = "TIMESTAMP_GPFS";
				break;
			case IOBYTESREAD:
				metric = "IOBYTESREAD";
				break;
			case IOBYTESWRITE:
				metric = "IOBYTESWRITE";
				break;
			case IOOPENS:
				metric = "IOOPENS";
				break;
			case IOCLOSES:
				metric = "IOCLOSES";
				break;
			case IOREADS:
				metric = "IOREADS";
				break;
			case IOWRITES:
				metric = "IOWRITES";
				break;
			case READDIR:
				metric = "READDIR";
				break;
			case INODE_UPDATES:
				metric = "INODE_UPDATES";
				break;
			case SIZE:
				metric = "SIZE";
				break;
		}
		LOG_VAR(ll) << leading << "    Metric type:  " << metric;
	}

      protected:
	GPFS_METRIC _metric_type;
};

#endif /* GPFSMON_GPFSMONSENSORBASE_H_ */
