//================================================================================
// Name        : SensorBase.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : General sensor base class.
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

#ifndef SRC_SENSORBASE_H_
#define SRC_SENSORBASE_H_

#include <fstream>
#include <memory>
#include <string>
#include <limits.h>
#include <boost/lockfree/spsc_queue.hpp>
#include "logging.h"
#include "cacheentry.h"
#include "metadatastore.h"

/**
 * @brief General sensor base class.
 *
 * @ingroup common
 */
class SensorBase {
public:
	SensorBase(const std::string& name) :
		_name(name),
		_mqtt(""),
		_skipConstVal(false),
		_publish(true),
		_cacheInterval(900000),
		_subsamplingFactor(1),
		_subsamplingIndex(0),
		_factor(1),
		_cache(nullptr),
		_delta(false),
		_deltaMax(LLONG_MAX),
		_firstReading(true),
		_readingQueue(nullptr),
		_metadata(nullptr) {

        _lastRawUValue.timestamp = 0;
        _lastRawUValue.value     = 0;
        _lastRawValue.timestamp = 0;
        _lastRawValue.value     = 0;
        _latestValue.timestamp	= 0;
        _latestValue.value		= 0;
        _lastSentValue.timestamp= 0;
        _lastSentValue.value	= 0;
        _accumulator.timestamp  = 0;
        _accumulator.value		= 0;
	}

	SensorBase(const SensorBase& other) :
		_name(other._name),
		_mqtt(other._mqtt),
		_skipConstVal(other._skipConstVal),
		_publish(other._publish),
		_cacheInterval(other._cacheInterval),
		_subsamplingFactor(other._subsamplingFactor),
		_subsamplingIndex(0),
		_factor(other._factor),
		_cache(nullptr),
		_delta(other._delta),
		_deltaMax(other._deltaMax),
		_firstReading(true),
		_lastRawUValue(other._lastRawUValue),
		_lastRawValue(other._lastRawValue),
		_latestValue(other._latestValue),
        _lastSentValue(other._lastSentValue),
        _accumulator(other._accumulator),
		_readingQueue(nullptr) {
	    
        _metadata.reset(other._metadata.get() ? new SensorMetadata(*other._metadata) : nullptr);
	}

	virtual ~SensorBase() {}

	SensorBase& operator=(const SensorBase& other) {
		_name = other._name;
		_mqtt = other._mqtt;
		_skipConstVal = other._skipConstVal;
		_publish = other._publish;
		_cacheInterval = other._cacheInterval;
		_subsamplingFactor = other._subsamplingFactor;
		_subsamplingIndex = 0;
		_factor = other._factor;
		_cache.reset(nullptr);
		_delta = other._delta;
		_deltaMax = other._deltaMax;
		_firstReading = true;
		_lastRawUValue.timestamp = other._lastRawUValue.timestamp;
        _lastRawUValue.value     = other._lastRawUValue.value;
		_lastRawValue.timestamp = other._lastRawValue.timestamp;
        _lastRawValue.value     = other._lastRawValue.value;
		_latestValue.timestamp	= other._latestValue.timestamp;
		_latestValue.value		= other._latestValue.value;
        _lastSentValue.timestamp= other._lastSentValue.timestamp;
        _lastSentValue.value	= other._lastSentValue.value;
        _accumulator.timestamp  = other._accumulator.timestamp;
        _accumulator.value      = other._accumulator.value;
		_readingQueue.reset(nullptr);
        _metadata.reset(other._metadata.get() ? new SensorMetadata(*other._metadata) : nullptr);

		return *this;
	}

	const bool 				isDelta()			const 	{ return _delta;}
	const uint64_t			getDeltaMaxValue()  const 	{ return _deltaMax; }
	const std::string& 		getName() 			const	{ return _name; }
	const std::string&		getMqtt() 			const	{ return _mqtt; }
	bool					getSkipConstVal()	const	{ return _skipConstVal; }
	bool					getPublish()		const	{ return _publish; }
        bool					getDelta()		const	{ return _delta; }
	unsigned				getCacheInterval()	const	{ return _cacheInterval; }
	int 					getSubsampling()	const   { return _subsamplingFactor; }
	double					getFactor()		const	{ return _factor; }

	const CacheEntry* const	getCache() 			const	{ return _cache.get(); }
	const reading_t&		getLatestValue()	const	{ return _latestValue; }
	const bool				isInit()			const 	{ return _cache && _readingQueue; }

	// Exposing the reading queue is necessary for publishing sensor data from the collectagent
	boost::lockfree::spsc_queue<reading_t>* getReadingQueue() { return _readingQueue.get(); }
	SensorMetadata* getMetadata()						      { return _metadata.get(); }
	
	void	clearMetadata()									{ _metadata.reset(nullptr); }
	void	setMetadata(SensorMetadata& s)					{ _metadata.reset(new SensorMetadata(s)); }
	void	setSkipConstVal(bool skipConstVal)				{ _skipConstVal = skipConstVal;	}
	void	setPublish(bool pub)							{ _publish = pub; }
	void	setDelta(const bool delta) 						{ _delta = delta; }
	void	setDeltaMaxValue(const uint64_t maxv)			{ _deltaMax = maxv; }
	void	setName(const std::string& name)				{ _name = name; }
	void	setMqtt(const std::string& mqtt)				{ _mqtt = mqtt; }
	void 	setCacheInterval(unsigned cacheInterval)		{ _cacheInterval = cacheInterval; }
	void	setSubsampling(int factor)						{ _subsamplingFactor = factor; }
	void	setFactor(const double &factor)			{ _factor = factor; }
	void    setLastRaw(int64_t raw)                         { _lastRawValue.value = raw; }
	void    setLastURaw(uint64_t raw)                       { _lastRawUValue.value = raw; }

	const std::size_t	getSizeOfReadingQueue() const { return _readingQueue->read_available(); }
	std::size_t 		popReadingQueue(reading_t *reads, std::size_t max) const	{ return _readingQueue->pop(reads, max); }
	void 				clearReadingQueue() const	{ reading_t buf; while(_readingQueue->pop(buf)) {} }
	void				pushReadingQueue(reading_t *reads, std::size_t count) const	{ _readingQueue->push(reads, count); }

	virtual void initSensor(unsigned interval, unsigned queueLen) {
		uint64_t cacheSize = _cacheInterval / interval + 1;
		if(!_cache) {
			//TODO: have all time-related configuration parameters use the same unit (e.g. milliseconds)
			_cache.reset(new CacheEntry( (uint64_t)_cacheInterval * 1000000, cacheSize));
			_cache->updateBatchSize(1, true);
		}
		if(!_readingQueue) {
			_readingQueue.reset(new boost::lockfree::spsc_queue<reading_t>(queueLen));
		}
	}

	/**
	 * Store a reading, in order to get it pushed to the data base eventually.
	 * Also this method takes care of other optional reading post-processing,
	 * e.g. delta computation, subsampling, caching, scaling, etc.
	 *
	 * This is the primary storeReading() and should be used whenever possible.
	 *
	 * @param rawReading  Reading struct with value and timestamp to be stored.
	 * @param factor      Scaling factor, which is applied to the reading value (optional)
	 * @param storeGlobal Store reading in reading queue, so that it can get pushed.
	 */
	void storeReading(reading_t rawReading, double factor=1.0, bool storeGlobal=true) {
        reading_t reading = rawReading;
        if( _delta ) {
            if (!_firstReading) {
                if (rawReading.value < _lastRawValue.value)
                    reading.value = (rawReading.value + ((int64_t)_deltaMax - _lastRawValue.value)) * factor * _factor;
                else
                    reading.value = (rawReading.value - _lastRawValue.value) * factor * _factor;
            } else {
                _firstReading = false;
                _lastRawValue = rawReading;
                return;
            }
            _lastRawValue = rawReading;
        }
        else
            reading.value = rawReading.value * factor * _factor;

		storeReadingLocal(reading);
		if (storeGlobal) {
		    storeReadingGlobal(reading);
		}
	}

	/**
     * Store an unsigned reading, in order to get it pushed to the data base eventually.
     * Also this method takes care of other optional reading post-processing,
     * e.g. delta computation, subsampling, caching, scaling, etc.
     *
     * This is a variant of the primary storeReading() for monotonically increasing
     * sensors reading unsigned 64bit values which may require more than the 63bit
     * offered by a signed reading_t. The readings are still stored as signed int64
     * in the database, therefore all such sensors should enable storage of deltas!
     *
     * This variant only adapts the delta computation for ureading_t actually.
     *
     * @param rawReading  Reading struct with (usigned) value and timestamp to be stored.
     * @param factor      Scaling factor, which is applied to the reading value (optional)
     * @param storeGlobal Store reading in reading queue, so that it can get pushed.
     */
	void storeReading(ureading_t rawReading, double factor=1.0, bool storeGlobal=true) {
        reading_t reading;
        reading.timestamp = rawReading.timestamp;
        if( _delta ) {
            if (!_firstReading) {
                if (rawReading.value < _lastRawUValue.value)
                    reading.value = (rawReading.value + (_deltaMax - _lastRawUValue.value)) * factor * _factor;
                else
                    reading.value = (rawReading.value - _lastRawUValue.value) * factor * _factor;
            } else {
                _firstReading = false;
                _lastRawUValue = rawReading;
                return;
            }
            _lastRawUValue = rawReading;
        }
        else
            reading.value = rawReading.value * factor * _factor;

        storeReadingLocal(reading);
        if (storeGlobal) {
            storeReadingGlobal(reading);
        }
    }

    /**
     * Store reading within the sensor, but do not put it in the readingQueue
     * so the reading does not get pushed but the caches are still updated.
     */
    inline
    void storeReadingLocal(reading_t reading) {
        _cache->store(reading);
        _latestValue = reading;
    }

    /**
     * Store the reading in the readingQueue so it can get pushed.
     */
    inline
    void storeReadingGlobal(reading_t reading) {
        if( _delta )
            // If in delta mode, _accumulator acts as a buffer, summing all deltas for the subsampling period
            _accumulator.value += reading.value;
        else
            _accumulator.value = reading.value;

        if (_subsamplingFactor>0 && _subsamplingIndex++%_subsamplingFactor==0) {
            _accumulator.timestamp = reading.timestamp;
            //TODO: if sensor starts with values of 0, these won't be pushed. This should be fixed
            if( !(_skipConstVal && (_accumulator.value == _lastSentValue.value)) ) {
                _readingQueue->push(_accumulator);
                _lastSentValue = _accumulator;
            }
            // We reset the accumulator's value for the correct accumulation of deltas
            _accumulator.value = 0;
        }
    }
    
    virtual void printConfig(LOG_LEVEL ll, LOGGER& lg, unsigned leadingSpaces=16) {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << _name;
		if (getSubsampling() != 1) {
			LOG_VAR(ll) << leading << "    SubSampling:      " << getSubsampling();
		}
		LOG_VAR(ll) << leading << "    Factor:            " << _factor;
		LOG_VAR(ll) << leading << "    Skip const values: " << (_skipConstVal ? "true" : "false");
		LOG_VAR(ll) << leading << "    Store delta only:  " << (_delta ? "true" : "false");
		if(_delta) {
			LOG_VAR(ll) << leading << "    Maximum value:     " << _deltaMax;
		}
		LOG_VAR(ll) << leading << "    Publish:           " << (_publish ? "true" : "false");
    }

protected:

	std::string _name;
	std::string _mqtt;
	bool _skipConstVal;
	bool _publish;
	unsigned int _cacheInterval;
	int _subsamplingFactor;
	unsigned int _subsamplingIndex;
	double _factor;
	std::unique_ptr<CacheEntry> _cache;
	bool _delta;
	uint64_t _deltaMax;
	bool _firstReading;
	ureading_t _lastRawUValue;
	reading_t _lastRawValue;
	reading_t _latestValue;
	reading_t _lastSentValue;
	reading_t _accumulator;
	std::unique_ptr<boost::lockfree::spsc_queue<reading_t>> _readingQueue;
	std::unique_ptr<SensorMetadata> _metadata;
};

//for better readability
using SBasePtr = std::shared_ptr<SensorBase>;

#endif /* SRC_SENSORBASE_H_ */
