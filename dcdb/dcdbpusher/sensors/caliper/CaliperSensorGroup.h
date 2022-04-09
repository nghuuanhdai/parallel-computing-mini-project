//================================================================================
// Name        : CaliperSensorGroup.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Caliper sensor group class.
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

#ifndef CALIPER_CALIPERSENSORGROUP_H_
#define CALIPER_CALIPERSENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"

#include "CaliperSensorBase.h"

#include <atomic>
#include <semaphore.h>
#include <unordered_map>

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup caliper
 */
class CaliperSensorGroup : public SensorGroupTemplate<CaliperSensorBase> {

/*******************************************************************************
 * Common defines. Keep in sync with DcdbPusher Caliper service
 ******************************************************************************/
#define MSGQ_SIZE 16 * 1024 * 1024
#define STR_PREFIX "/cali_dcdb_"
#define SHM_SIZE (17 * 1024 * 1024)
#define SOCK_NAME "DCDBPusherCaliSocket"

	/*
 * Queue entry layout:
 * {
 *   uint64_t timestamp
 *   char[]   data (NUL terminated string with at most max_dat_size bytes)
 * }
 */

	/*
 * Layout of shared-memory file used to communicate with Caliper service:
 *
 * //Communication queue, aka ring buffer:
 * size_t r_index //points to the last read byte
 * size_t w_index //points to the last written byte
 * sem_t  r_sem; // atomic access to r_index
 * sem_t  w_sem; // atomic access to w_index
 * char[MSGQ_SIZE] //contains variable length entries
 * //TODO do not exceed SHM_SIZE
 */
	/*******************************************************************************
 * End of common defines
 ******************************************************************************/

      public:
	CaliperSensorGroup(const std::string &name);
	CaliperSensorGroup(const CaliperSensorGroup &other);
	virtual ~CaliperSensorGroup();
	CaliperSensorGroup &operator=(const CaliperSensorGroup &other);

	bool execOnStart() final override;
	void execOnStop() final override;

	std::vector<SBasePtr> &acquireSensors() final override {
		while (_lock.test_and_set(std::memory_order_acquire)) {}
		return _baseSensors;
	}

	void releaseSensors() final override {
		_lock.clear(std::memory_order_release);
	}

	void printGroupConfig(LOG_LEVEL ll, unsigned leadingSpaces = 12) final override;

	void setMaxSensorNum(const std::string &maxSensorNum) { _maxSensorNum = stoul(maxSensorNum); }
	void setTimeout(const std::string &timeout) { _timeout = stoul(timeout); }
	void setGlobalMqttPrefix(const std::string &prefix) { _globalMqttPrefix = prefix; }

	unsigned       getMaxSensorNum() const { return _maxSensorNum; }
	unsigned short getTimeout() const { return _timeout; }

      private:
	void read() final override;

	struct caliInstance { /**< Bundle all variables required to read values from
                               one running Caliper instance */
		void * shm;
		int    shmFile;
		size_t shmFailCnt;
	};

	unsigned       _maxSensorNum; ///< Clear associated sensors if we exceed this threshold to limit memory usage
	unsigned short _timeout;      ///< Number of consecutive read cycles with empty queue before Cali process is assumed dead
	char *         _buf;          ///< Local buffer for temporal storage of shm queue contents

	int         _socket;
	int         _connection;
	std::string _globalMqttPrefix;

	std::list<caliInstance>                _processes;
	std::atomic_flag                       _lock;        ///< Lock to synchronize access to associated sensors
	std::unordered_map<std::string, S_Ptr> _sensorIndex; ///< Additional sensor storage for fast lookup
};

#endif /* CALIPER_CALIPERSENSORGROUP_H_ */
