//================================================================================
// Name        : cacheentry.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Class defining a sensor cache entry.
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

#ifndef CACHEENTRY_H_
#define CACHEENTRY_H_

#include <vector>
#include <utility>
#include <exception>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "timestamp.h"

/**
 * @brief Defines a signed reading.
 *
 * @ingroup common
 */
typedef struct {
	int64_t  value;
	uint64_t timestamp;
} reading_t;

/**
 * @brief Defines a unsigned reading.
 *
 * @ingroup common
 */
typedef struct {
	uint64_t value;
	uint64_t timestamp;
} ureading_t;

/**
 * @brief Class defining a sensor cache entry.
 *
 * @ingroup common
 */
class CacheEntry {
public:
	CacheEntry(uint64_t maxHistory=300000000000) {
		_maxHistory = maxHistory;
		_stable = false;
		_cacheIndex = -1;
		_batchSize = -1.0;
		//We pre-allocate the cache to a initial guess of 600 elements - 10 minutes at 1s sampling period
		_cache.reserve(600);
	}

	CacheEntry(uint64_t maxHistory, uint64_t size) {
		_maxHistory = maxHistory;
		_cache.resize(size);
		_stable = true;
		_cacheIndex = -1;
		_batchSize = -1.0;
	}

	~CacheEntry() {
		_cache.clear();
	}


	/**
	* @brief         Updates the internal batch size value
	* 
	*				 A 0.05 learning rate is used to update the internal batch size value. 
	* 
	* @param newsize The new observed batch size
	* @param enforce If true, the batch size is updated ignoring the learning rate 
	**/
	void updateBatchSize(uint64_t newsize, bool enforce=false) {
		if(!enforce)
			_batchSize = _batchSize < 0.0 ? (float)newsize : _batchSize*0.95 + (float)newsize*0.05;
		else
			_batchSize = (float)newsize;
	}

	/**
	* @brief        Returns the current batch size value.
	**/
	uint64_t getBatchSize() const { return (uint64_t)_batchSize; }
	
    /**
    * @brief        Returns the time frame (in nanoseconds) covered by the cache.
    **/
	uint64_t getMaxHistory() const { return _maxHistory; }

	/**
	* @brief        Returns a reference to the internal cache vector.
	**/
	const std::vector<reading_t>* getRaw() const { return &_cache; }

    /**
    * @brief        Returns the element with index i in the cache.
    **/
	reading_t get(unsigned i) const { return _cache[i]; }

	/**
	* @brief        	Stores a sensor reading in the cache.
	*
	* @param reading    reading_t struct containing the latest sensor reading.
	**/
	void store(reading_t reading) {
		// Sensor caches have two operating phases: first, the sensor cache vector expands until the maximum allowed time
		// range is covered. After this "stable" size is reached, the sensor vector is used like a circular array, and its
		// size does not change anymore
		//std::cout << "Value: " << reading.value << " at time: " << reading.timestamp << " size: " << _cache.size() << std::endl;
		_cacheIndex = _stable ? (_cacheIndex + 1) % _cache.size() : (_cacheIndex + 1);
		if(!_stable) {
			_cache.push_back(reading);
			if(_cache.front().timestamp + _maxHistory <= reading.timestamp) {
				_stable = true;
				//We shrink the cache capacity, if necessary, to its actual size
				_cache.shrink_to_fit();
			}
		} else
			_cache[_cacheIndex] = reading;
	}

    /**
    * @brief        	Returns a view of the sensor cache corresponding to a certain time frame.
    *
    *                   Starting from the sensor cache, a new vector is built using readings contained in the time
    *                   frame specified by the startTs and endTs input arguments.
    *
    * @param startTs    Starting timestamp of the desired view
    * @param endTs      End timestamp of the desired view
    * @param buffer     Reference to a vector to be used to store the view.
    * @param rel        If true, startTs and endTs are interpreted as relative timestamps against "the most recent sensor reading"
    * @param tol        Maximum tolerance (in ns) for returned timestamps. Limited by the internal staleness threshold.   
    * @return           True if successful, false otherwise
    **/
	bool getView(uint64_t startTs, uint64_t endTs, std::vector<reading_t>& buffer, bool rel=false, uint64_t tol=3600000000000) const {
	    // We add the estimated batch size in the computation of the stale threshold (set to 1 if not used)
		uint64_t cacheSize = _cache.size()>1 ? _cache.size()-1 : 1;
		uint64_t staleThreshold = (_maxHistory / cacheSize) * (uint64_t)_batchSize * 4;
        uint64_t now = getTimestamp();
        
        if(tol < staleThreshold)
            staleThreshold = tol;
        
        if(startTs!=endTs) {
            //Converting relative offsets to absolute timestamp for staleness checking
            uint64_t startTsInt = rel ? now - startTs : startTs;
            uint64_t endTsInt = rel ? now - endTs : endTs;
            //Getting the cache indexes to access sensor data
            int64_t startIdx = rel ? getOffset(startTs) : searchTimestamp(startTs, true);
            int64_t endIdx = rel ? getOffset(endTs) : searchTimestamp(endTs, false);
    
            //Managing invalid time offsets
            if(startIdx < 0 || endIdx < 0 || startIdx-endIdx==1)
                return false;
            //Managing obsolete data
            if(tsAbs(startTsInt, _cache[startIdx].timestamp) > staleThreshold || tsAbs(endTsInt, _cache[endIdx].timestamp) > staleThreshold)
                return false;
            if(startIdx <= endIdx)
                buffer.insert(buffer.end(), _cache.begin() + startIdx, _cache.begin() + endIdx + 1);
            else {
                buffer.insert(buffer.end(), _cache.begin() + startIdx, _cache.end());
                buffer.insert(buffer.end(), _cache.begin(), _cache.begin() + endIdx + 1);
            }
        }
        else {
            // If start and end timestamps are equal we retrieve only one value
            uint64_t tsInt = rel ? now - startTs : startTs;
            int64_t intIdx = rel ? getOffset(startTs) : searchTimestamp(startTs, false);
            if(intIdx < 0 || tsAbs(tsInt, _cache[intIdx].timestamp) > staleThreshold)
                return false;
            buffer.push_back(_cache[intIdx]);
        }
        return true;
	}

	/**
	* @brief        	Ensures that the cache is still valid.
	*
	*					The cache is considered valid if it is not outdated, that is, the latest reading is not
	*					older than 5 times the average sampling rate.
	* @param live       If true, more strict staleness checks are enforced
	* @return 			True if the cache is still valid, False otherwise
	**/
	bool checkValid(bool live=false) const {
		if(!_stable || _cache.empty())
			return false;
		uint64_t cacheSize = _cache.size()>1 ? _cache.size()-1 : 1;
		uint64_t staleThreshold = (_maxHistory / cacheSize) * (live ? 4 : (uint64_t)_batchSize * 4);
		return ((getTimestamp() - getLatest().timestamp) <= staleThreshold);
	}
	
	/**
	* @brief        	Returns an average of recent sensor readings.
	*
	*					Only the sensor readings pushed in the last "avg" nanoseconds are used in the average
	*					computation.
	*
	* @param avg    	length of the average aggregation window in nanoseconds.
	* @return 			Average value of the last sensor readings.
	**/
	int64_t getAverage(uint64_t avg) const {
		uint64_t ts = getTimestamp();

		if (_cache.size() > 0) {
			if (ts - getOldest().timestamp < avg) {
				throw std::out_of_range("Not enough data");
			}
			else if (ts - getLatest().timestamp > avg && avg>0) {
				throw std::out_of_range("Sensor outdated");
			}

			double sum = 0;
			int64_t it, prev;
			prev = _cacheIndex;
			it = older(prev);
			// We compute the weighted average of elements in the cache that fall within the specified window
			while ((it != _cacheIndex) && ((ts - _cache[it].timestamp) <= avg)) {
				uint64_t deltaT = (_cache[prev].timestamp - _cache[it].timestamp);
				sum += ((_cache[it].value + _cache[prev].value) / 2) * deltaT;
				//std::cout << "SensorCache::getAverage sum=" << sum << " deltaT=" <<deltaT << " it=(" << it->timestamp << "," <<it->val <<") prev=(" <<  prev->timestamp << "," << prev->val <<") " << (ts.getRaw() - it->timestamp) << std::endl;
				prev = it;
				it = older(it);
			}

			//std::cout << "SensorCache::getAverage (" << prev->timestamp << "," <<prev->val <<") (" <<  entry.back().timestamp << "," << entry.back().value << ") sum=" << sum << " deltaT=" << entry.back().timestamp - prev->timestamp << std::endl;
			// If prev points to the cache head, there was only one element in the aggregation window
			if (prev == _cacheIndex || avg==0) {
				return getLatest().value;
			} else {
				return sum/(getLatest().timestamp - _cache[prev].timestamp);
			}
		}
		throw std::invalid_argument("Sensor not found");
	}

	/**
	* @brief        	Searches for the input timestamp in the cache.
	*
	*					Binary search is used the search for the "t" timestamp within the sensor cache, and its
	*					index is returned if successful.
	*
	* @param t    		Timestamp to be searched, in nanoseconds.
	* @param leftmost	If true, leftmost binary search is performed. Rightmost otherwise.
	*
	* @return 			The index of the closest sensor reading to t, or -1 if out of bounds.
	**/
	int64_t searchTimestamp(uint64_t t, bool leftmost=true) const {
		// Cache is empty or has only one element
		if(!_stable || _cache.empty())
			return -1;
		// Target timestamp (relative or absolute) is outside of the time frame contained in the cache
		//else if(t > _cache[_cacheIndex].timestamp || t < _cache[(_cacheIndex+1) % _cache.size()].timestamp)
		//	return -1;

		int64_t pivot = 0, pivotReal = 0, aPoint = 0, bPoint = _cache.size(), fixIndex=_cacheIndex;
		if(leftmost) {
			// Attention! aPoint and bPoint are linearized indexes, and do not take into account the presence of cacheIndex
			// When computing the position of the pivot, we map it to the actual index in the circular array
			// Standard binary search algorithm below
			while (aPoint < bPoint) {
				pivot = (aPoint + bPoint) / 2;
				pivotReal = (fixIndex + 1 + pivot) % _cache.size();
				if (t <= _cache[pivotReal].timestamp)
					bPoint = pivot;
				else
					aPoint = pivot + 1;
			}
			return (fixIndex + 1 + aPoint) % _cache.size();
		}
		else {
			while (aPoint < bPoint) {
				pivot = (aPoint + bPoint) / 2;
				pivotReal = (fixIndex + 1 + pivot) % _cache.size();
				if (t < _cache[pivotReal].timestamp)
					bPoint = pivot;
				else
					aPoint = pivot + 1;
			}
			return (fixIndex + aPoint) % _cache.size();
		}
	}

	/**
	* @brief        	Returns the index of the cache element that is older than the latest entry by "t".
	*
	*					Unlike searchTimestamp, this method does not perform an actual search, but simply computes
	*					the number of elements that cover a time interval of "t" nanoseconds. This value is then
	*					used together with _cacheIndex to compute the starting index of the most recent cache
	*					portion that covers such time interval.
	*
	* @param t    		Length of the time frame in nanoseconds.
	*
	* @return 			Index of element in the sensor cache.
	**/
	int64_t getOffset(uint64_t t) const {
		if(!_stable || _cache.empty())
			return -1;
		else {
			int64_t cacheSize = _cache.size();
			int64_t offset = _maxHistory==0 ? 0 : (( cacheSize * (int64_t)t ) / ((int64_t)_maxHistory));
			if(offset > cacheSize)
				return -1;
			return (cacheSize + _cacheIndex - offset) % cacheSize;
		}
	}

	/**
	* @brief        	Returns the latest sensor reading in the cache.
	**/
	reading_t getLatest() const {
		if(_cacheIndex==-1) {
			reading_t s = {0,0};
			return s;
		} else
			return _cache[_cacheIndex];
	}

	/**
	* @brief        	Returns the oldest sensor reading in the cache.
	**/
	reading_t getOldest() const {
		if(_cacheIndex==-1) {
			reading_t s = {0,0};
			return s;
		} else
			return _cache[(_cacheIndex + 1) % _cache.size()];
	}

private:
	
	inline
	uint64_t tsAbs(uint64_t a, uint64_t b) const {
		return a<=b ? b-a : a-b;
	}

	/**
	* @brief        	Returns the index of the immediately newer element with respect to input index "ind".
	**/
	uint64_t newer(uint64_t ind) const { return (ind + 1) % _cache.size(); }

	/**
	* @brief        	Returns the index of the immediately older element with respect to input index "ind".
	**/
	uint64_t older(uint64_t ind) const { return ind == 0 ? _cache.size() - 1 : ind - 1; }

	// Internal cache vector
	std::vector<reading_t> _cache;
	// Flag to signal cache status
	bool _stable;
	// Head of the cache in the circular array
	int64_t _cacheIndex;
	// Time frame in nanoseconds covered by the cache
	uint64_t _maxHistory;
	// Batch size for the sensor represented by this cache entry
	float _batchSize;
};

#endif /* CACHEENTRY_H_ */
