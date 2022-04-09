//================================================================================
// Name        : MSRSensorBase.h
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for MSR plugin.
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
 * @defgroup msr MSR plugin
 * @ingroup  pusherplugins
 *
 * @brief Retrieve data from model-specific registers (MSRs).
 */

#ifndef MSR_MSRSENSORBASE_H_
#define MSR_MSRSENSORBASE_H_

#include "sensorbase.h"

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup msr
 */
class MSRSensorBase : public SensorBase {
      public:

	static constexpr uint64_t MSR_MAXIMUM_SIZE = 281474976710656; //2^48
	
	MSRSensorBase(const std::string &name)
	    : SensorBase(name), _cpu(0), _metric(0) {
		//default delta to true, as msr has only monotonic sensors usually
		_delta = true;
		_deltaMax = MSR_MAXIMUM_SIZE;
	}

	MSRSensorBase(const MSRSensorBase &other) = default;

	virtual ~MSRSensorBase() {
	}

	MSRSensorBase &operator=(const MSRSensorBase &other) = default;

	unsigned int getCpu() const {
		return _cpu;
	}

	void setCpu(unsigned int cpu) {
		_cpu = cpu;
	}

	uint64_t getMetric() const {
		return _metric;
	}

	void setMetric(uint64_t metric) {
		_metric = metric;
	}
	
	void setFirstReading(bool val) {
		_firstReading = val;
	}
	
	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << "    CPU:               " << _cpu;
		LOG_VAR(ll) << leading << "    Metric:            " << std::showbase << std::hex << _metric;
	}

      protected:
	unsigned int _cpu;
	uint64_t     _metric;
};

#endif /* MSR_MSRSENSORBASE_H_ */
