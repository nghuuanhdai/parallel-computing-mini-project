//================================================================================
// Name        : CaliperSensorBase.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for Caliper plugin.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
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
 * @defgroup cali Caliper plugin
 * @ingroup  pusherplugins
 *
 * @brief Listen for data sent by Caliper framework.
 *
 * @details This plugin is an odd one out to accommodate Caliper. It uses UNIX
 *          sockets to receive data (function or event names and corresponding
 *          timestamp). It does not periodically poll data but instead checks if
 *          new data is available at the socket. If data is produced faster by
 *          Caliper than it can be consumed by this plugin at its current
 *          frequency data will be consequently lost. Sensors are not
 *          constructed at program start (during configuration) but during
 *          runtime. For each new encountered function/event name a new sensor
 *          is created whose name identifies the function/event. On subsequent
 *          encounters a value of 1 is pushed for the corresponding sensor.
 *          Aggregated numbers of encountered function/event names can be
 *          evaluated afterwards from the database.
 */

#ifndef CALIPER_CALIPERSENSORBASE_H_
#define CALIPER_CALIPERSENSORBASE_H_

#include "sensorbase.h"

/**
* @brief SensorBase specialization for this plugin.
*
* @ingroup cali
*/
class CaliperSensorBase : public SensorBase {
      public:
	CaliperSensorBase(const std::string &name)
	    : SensorBase(name) {
	}

	CaliperSensorBase(const CaliperSensorBase &other)
	    : SensorBase(other) {
	}

	virtual ~CaliperSensorBase() {
	}

	CaliperSensorBase &operator=(const CaliperSensorBase &other) {
		SensorBase::operator=(other);

		return *this;
	}

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		/* nothing to print */
	}

      protected:
	// no specific attributes
};

#endif /* CALIPER_CALIPERSENSORBASE_H_ */
