//================================================================================
// Name        : ProcfsSensorBase.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for Procfs plugin.
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
 * @defgroup procfs ProcFS plugin
 * @ingroup  pusherplugins
 *
 * @brief Collect data from the /proc filesystem.
 */

#ifndef PROCFSSENSOR_H_
#define PROCFSSENSOR_H_

#include "sensorbase.h"

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @details Adds two private members, cpuID and metric, to the SensorBase class.
 *
 * @ingroup procfs
 */
class ProcfsSensorBase : public SensorBase {

      public:
	// Constructor and destructor
	ProcfsSensorBase(const std::string &name)
	    : SensorBase(name) {
		_metric = "";
		_perCPU = false;
		_cpuId = -1;
		_deltaMax = ULLONG_MAX;
	}

	ProcfsSensorBase(const std::string &name, const std::string &metric, bool percpu = false, int cpuid = -1)
	    : SensorBase(name) {
		_metric = metric;
		_perCPU = percpu;
		_cpuId = cpuid;
	}

	// Copy constructor
	ProcfsSensorBase(ProcfsSensorBase &other)
	    : SensorBase(other) {
		_metric = other.getMetric();
		_perCPU = other.isPerCPU();
		_cpuId = other.getCPUId();
	}

	virtual ~ProcfsSensorBase() {}

	void        setMetric(std::string m) { _metric = m; }
	void        setPerCPU(bool p) { _perCPU = p; }
	void        setCPUId(int i) { _cpuId = i; }
	std::string getMetric() { return _metric; }
	int         getCPUId() { return _cpuId; }
	bool        isPerCPU() { return _perCPU; }

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << "    Metric:            " << _metric;
		LOG_VAR(ll) << leading << "    CPU Id:            " << _cpuId;
		LOG_VAR(ll) << leading << "    PerCPU:            " << (_perCPU ? "enabled" : "disabled");
	}

      protected:
	// The metric field is used to decouple the sensor's name from its metric within the proc file
	std::string _metric;
	bool        _perCPU;
	int         _cpuId;
};

using ProcfsSBPtr = std::shared_ptr<ProcfsSensorBase>;

#endif /* PROCFSSENSOR_H_ */
