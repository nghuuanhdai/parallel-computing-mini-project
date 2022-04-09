//================================================================================
// Name        : sensorcache.h
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description :
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2016-2019 Leibniz Supercomputing Centre
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

#ifndef COLLECTAGENT_SENSORCACHE_H_
#define COLLECTAGENT_SENSORCACHE_H_

#include <map>
#include <atomic>
#include <dcdb/sensorid.h>
#include <dcdb/timestamp.h>
#include <dcdb/sensordatastore.h>
#include "cacheentry.h"

using namespace DCDB;

typedef std::map<SensorId, CacheEntry> sensorCache_t;

/**
 * @brief
 *
 * @ingroup ca
 */
class SensorCache {
public:
	SensorCache(uint64_t maxHistory=60000000000);
	virtual ~SensorCache();

	/**
	* @brief        Returns a reference to the internal sensor cache map.
	**/
	sensorCache_t& getSensorMap();

	/**
	* @brief        Store a sensor reading in the SensorCache.
	*
	* @param sid    The SensorId of the sensor to be cached.
    * @param ts     The timestamp of the sensor reading.
    * @param val    The actual sensor reading.
	* @return       Returns true if the topic string was valid and the data field of the object was populated.
	**/
	void storeSensor(SensorId sid, uint64_t ts, int64_t val);

	/**
	* @brief        Store a sensor reading in the SensorCache.
	*
	* @param s    The SensorDataStoreReading object of the sensor data to be cached.
	**/
	void storeSensor(const SensorDataStoreReading& s);
	
	/**
    * @brief        Return a sensor reading or the average of the last readings
    *               from the SensorCache.
    *
    * @param sid    The SensorId of the sensor to be looked up in the cache.
    * @param avg    If avg > 0: denotes the length of the average aggregation window in nanoseconds.
    * @return       If avg == 0 :The sensor reading of the corresponding cache entry.
    *               If avg > 0 the average of the last readings is returned.
    * @throws       std::invalid_argument if the SensorId doesn't exist in the SensorCache.
    * @throws       std::out_of_range if the sid was found in the cache entry but is outdated.
    **/
	int64_t getSensor(SensorId sid, uint64_t avg=0);

	/**
	* @brief        Return a sensor reading or the average of the last readings
    *               from the SensorCache.
	*
	* @param topic  The topic of the sensor to be looked up in the cache. May contain wildcards.
	* @param avg    If avg > 0: denotes the length of the average aggregation window in nanoseconds.
	* @return       If avg == 0 :The sensor reading of the corresponding cache entry.
    *               If avg > 0 the average of the last readings is returned.
	* @throws       std::invalid_argument if the topic couldn't be found in the SensorCache.
	* @throws       std::out_of_range if the topic was found in the cache entry but is outdated.
	**/
//	int64_t getSensor(std::string topic, uint64_t avg=0);

	/**
    * @brief        Dump the contents of the SensorCache to stdout.
    **/
	void dump();

	/**
	* @brief        Removes all obsolete entries from the cache
	*
	* @details      entries in the cache whose latest sensor reading is older than "now" - t nanoseconds are
	*				removed.
	*
	* @param t		The threshold in nanoseconds for entries that must be removed
	* @return		The number of purged cache entries
	**/
	uint64_t clean(uint64_t t);

	/**
	* @brief               Waits for internal updates to finish.
	*/
	const void wait() {
		while(_updating.load()) {}
		++_access;
	}

	/**
	* @brief               Reduces the internal reading counter.
	*/
	const void release() {
		--_access;
	}

    /**
    * @brief  Set a new maximum cache length.
    *
    * @param maxHistory: new sensor cache length value.
    **/
	void setMaxHistory(uint64_t maxHistory) { this->_maxHistory = maxHistory; }

    /**
    * @brief  Returns the current maximum sensor cache length
    *
    * @return Current maximum sensor cache length
    */
	uint64_t getMaxHistory() { return this->_maxHistory; }

private:
	// Map containing the single sensor caches
	sensorCache_t sensorCache;
	// Global maximum allowed time frame for the sensor caches
	uint64_t _maxHistory;
	// Spinlock to regulate map modifications
	std::atomic<bool> _updating;
	std::atomic<int> _access;

};

#endif /* COLLECTAGENT_SENSORCACHE_H_ */
