//================================================================================
// Name        : PerfSensorGroup.h
// Author      : Micha Mueller, Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Perfevent sensor group class.
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

#ifndef PERFSENSORGROUP_H_
#define PERFSENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"
#include "PerfSensorBase.h"

class PerfSensorGroup;

using PerfSGPtr = std::shared_ptr<PerfSensorGroup>;

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @details Leverages perfevent's group feature. A perfevent group is limited to
 *          one single cpu, therefore a PerfSensorGroup manages multiple equal
 *          perfevent groups which only differ in their cpu.
 *
 * @ingroup perf
 */
class PerfSensorGroup : public SensorGroupTemplate<PerfSensorBase> {
      public:
	PerfSensorGroup(const std::string &name);
	PerfSensorGroup(const PerfSensorGroup &other);
	virtual ~PerfSensorGroup();
	PerfSensorGroup &operator=(const PerfSensorGroup &other);

	void execOnInit() final override;
	bool execOnStart() final override;
	void execOnStop() final override;

	void setHtAggregation(std::string htAggregation) { _htAggregation = std::stoul(htAggregation); }
	void setMaxCorrection(std::string maxCorrection) { _maxCorrection = std::stod(maxCorrection); }

	void printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) final override;

	std::vector<S_Ptr> getPerfSensors() { return _sensors; }

      private:
	void read() final override;

	struct sensorBin {                              /**< A bin holds all sensors with same cpu. Therefore
	                         all sensors of a bin belong to the same perf-event
	                         group */
		int                cpu;                 /**< Cpu of this bin */
		bool               aggregator;          /**< Flag if this bin aggregates or gets aggregated */
		bool               lastValid;           /**< Flag if last reading was valid */
		bool               latestValueValid;    /**< Flag if latest raw reading was valid */
		uint64_t           latest_time_enabled; /**< see perf documentation */
		uint64_t           latest_time_running; /**< see perf documentation */
		std::vector<S_Ptr> sensors;             /**< Sensors in this bin */

		sensorBin(const S_Ptr &s, int cpuVal)
		    : cpu(cpuVal),
		      aggregator(false),
		      lastValid(true),
		      latestValueValid(false),
		      latest_time_enabled(0),
		      latest_time_running(0) {
			sensors.push_back(s);
		}
		sensorBin() = delete;
	};

	std::vector<sensorBin> _sensorBins;    /**< Bins to sort sensors according to their _cpu. */
	std::vector<int>       _cpuBinMapping; /**< _cpuBinMapping[_sensorBins[i].cpu] == i */

	unsigned _htAggregation; /**< Value for hyper-threading aggregation. Zero indicates disabled HT agg. */

	double _maxCorrection; /**< Maximum allowed correction value (defaults to 20).
	                            Prevents calculation of absurd correction values and instead skips the reading */

	char *      _buf;     /**< Member buffer to temporarily read in to avoid costly memory allocation every read-cycle */
	std::size_t _bufSize; /**< Size of _buf */

	uint64_t calculateIntervalValue(uint64_t previous, uint64_t current, uint64_t maxValue);
};
#endif /* PERFSENSORGROUP_H_ */
