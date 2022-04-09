//================================================================================
// Name        : TesterSensorBase.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for Tester plugin.
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
 * @defgroup tester Tester plugin
 * @ingroup  pusherplugins
 *
 * @brief Dummy plugin for (performance) testing.
 */

#ifndef TESTER_TESTERSENSORBASE_H_
#define TESTER_TESTERSENSORBASE_H_

#include "sensorbase.h"

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup tester
 */
class TesterSensorBase : public SensorBase {
      public:
	TesterSensorBase(const std::string &name)
	    : SensorBase(name) {}
	virtual ~TesterSensorBase() {}

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) { /* nothing to print */
	}
};

#endif /* TESTER_TESTERSENSORBASE_H_ */
