//================================================================================
// Name        : PerfSensorBase.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for Perfevent plugin.
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
 * @defgroup perf Perf plugin
 * @ingroup  pusherplugins
 *
 * @brief Collect data from a CPU's performance (perf) counters.
 */

#ifndef PERFEVENT_PERFSENSORBASE_H_
#define PERFEVENT_PERFSENSORBASE_H_

#include "sensorbase.h"
#include <limits.h>

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup perf
 */
class PerfSensorBase : public SensorBase {
      public:
	static constexpr uint64_t MAXCOUNTERVALUE = LLONG_MAX; //ToDo check if it is not LLONG_MAX, or make it configurable!

	PerfSensorBase(const std::string &name)
	    : SensorBase(name),
	      _type(0),
	      _config(0),
	      _cpu(-1),
	      _fd(-1),
	      _id(0) {
		//default delta to true, as perfevent has only monotonic sensors usually
		_delta = true;
		_deltaMax = MAXCOUNTERVALUE;
	}

	PerfSensorBase(const PerfSensorBase &other)
	    : SensorBase(other),
	      _type(other._type),
	      _config(other._config),
	      _cpu(other._cpu),
	      _fd(-1),
	      _id(0) {}

	virtual ~PerfSensorBase() {}

	PerfSensorBase &operator=(const PerfSensorBase &other) {
		SensorBase::operator=(other);
		_type = other._type;
		_config = other._config;
		_cpu = other._cpu;
		_fd = -1;
		_id = 0;

		return *this;
	}

	unsigned getType() const { return _type; }
	unsigned getConfig() const { return _config; }
	int      getCpu() const { return _cpu; }
	int      getFd() const { return _fd; }
	uint64_t getId() const { return _id; }

	void setType(unsigned type) { _type = type; }
	void setConfig(unsigned config) { _config = config; }
	void setCpu(int cpu) { _cpu = cpu; }
	void setFd(int fd) { _fd = fd; }
	void setId(uint64_t id) { _id = id; }

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << "    Type:   " << std::showbase << std::hex << _type;
		LOG_VAR(ll) << leading << "    Config: " << std::showbase << std::hex << _config;
		LOG_VAR(ll) << leading << "    CPU:    " << _cpu;
	}

      protected:
	unsigned int _type;
	unsigned int _config;
	int          _cpu;
	int          _fd;
	uint64_t     _id;
};

#endif /* PERFEVENT_PERFSENSORBASE_H_ */
