//================================================================================
// Name        : MSRSensorGroup.h
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for MSR sensor group class.
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

#ifndef MSR_MSRSENSORGROUP_H_
#define MSR_MSRSENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "MSRSensorBase.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup msr
 */
class MSRSensorGroup : public SensorGroupTemplate<MSRSensorBase> {
      public:
	MSRSensorGroup(const std::string &name);
	MSRSensorGroup(const MSRSensorGroup &other) = default;
	virtual ~MSRSensorGroup();
	MSRSensorGroup &operator=(const MSRSensorGroup &other) = default;

	bool                  execOnStart() final override;
	void                  execOnStop() final override;
	void                  addCpu(unsigned int cpu);
	std::vector<unsigned> getCpus();

	void printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) final override;

	void setHtAggregation(unsigned int htAggregation) {
		_htAggregation = htAggregation;
	}

	void groupInBins();

      private:
	void read() final override;

	struct msrSensorBin {               /**< A bin holds all sensors with same cpu. Therefore
	                         all sensors of a bin belong to the same msr
	                         group */
		int                fd;      /**< File descriptor to read all events in this cpu */
		std::vector<S_Ptr> sensors; /**< Sensors in this bin */

		void addMsrSensorBin(const S_Ptr &s) {
			sensors.push_back(s);
		}

		void setActive() {
			if (fd == -2) {
				fd = -1;
			}
		}

		bool isActive() {
			return (fd == -2 ? false : true);
		}

		int getFd() {
			return fd;
		}

		void setFd(int filedescriptor) {
			fd = filedescriptor;
		}

		msrSensorBin()
		    : fd(-2) {
		}
	};

	void program_fixed();
	int checkStatus(uint64_t msr_number, unsigned int cpu);

	unsigned int _htAggregation; /**< Value for hyper-threading aggregation. Zero indicates disabled HT agg. */
	//int _number_metrics_per_cpu;
	unsigned int              _total_number_cpus;
	std::vector<msrSensorBin> _sensorBins; /**< Bins to sort sensors according to their _cpu. */
	int32_t                   msr_read(uint64_t msr_number, uint64_t *value, unsigned int cpu);
	int32_t                   msr_write(uint64_t msr_number, uint64_t value, unsigned int cpu);
};

#endif /* MSR_MSRSENSORGROUP_H_ */
