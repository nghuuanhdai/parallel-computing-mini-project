//================================================================================
// Name        : sensordatastore.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for inserting and querying DCDB sensor data.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2019 Leibniz Supercomputing Centre
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//================================================================================

/**
 * @defgroup libdcdb libdcdb
 *
 * @brief Library to access DCDB's storage backend.
 *
 * @details The libdcdb library is a dynamic runtime library providing
 * 	        functions to initialize and access the DCDB data store. It
 * 	        is being used by the CollectAgent to handle insertion of
 * 	        data and can be used by tools responsible for data analysis.
 */

/**
 * @file sensordatastore.h
 *
 * @brief This file contains the public API for the libdcdb library.
 * It contains the class definition of the library's main class
 * SensorDataStore as well as the helper struct typedefs SensorId,
 * DeviceLocation, and DeviceSensorId.
 *
 * @ingroup libdcdb
 */

#include <string>
#include <list>
#include <cstdint>

#include "sensorid.h"
#include "timestamp.h"
#include "connection.h"

#ifndef DCDB_SENSORDATASTORE_H
#define DCDB_SENSORDATASTORE_H

namespace DCDB {

typedef enum {
  SDS_OK,
  SDS_EMPTYSET
} SDSQueryResult;
  
typedef enum {
  AGGREGATE_NONE = 0,
  AGGREGATE_MIN,
  AGGREGATE_MAX,
  AGGREGATE_AVG,
  AGGREGATE_SUM,
  AGGREGATE_COUNT
} QueryAggregate;

/* Forward-declaration of the implementation-internal classes */
class SensorDataStoreImpl;

/**
 * @brief %SensorDataStoreReading is the class for a single
 * sensor-timestamp-value entry in the database.
 */
class SensorDataStoreReading
{
public:
  SensorId sensorId;
  TimeStamp timeStamp;
  int64_t value;

#if 0
  inline bool operator == (const SensorDataStoreReading& rhs) const {
    return (sensorId == rhs.sensorId) &&
        (timeStamp == rhs.timeStamp) &&
        (value == rhs.value);}
#endif

  inline bool operator < (const SensorDataStoreReading& rhs) const {
    return (value < rhs.value);
  }

  SensorDataStoreReading();
  SensorDataStoreReading(const SensorId& sid, const uint64_t ts, const int64_t value);
  virtual ~SensorDataStoreReading();
};

/**
 * @brief %SensorDataStore is the class of the libdcdb library
 * to write and read sensor data.
 */
class SensorDataStore
{
private:
    SensorDataStoreImpl* impl;
    
public:
    /**
     * @brief This function inserts a single sensor reading into
     *        the database.
     * @param sid      A SensorId object
     * @param ts       The timestamp of the sensor reading
     * @param value    The value of the sensor reading
     * @param ttl      Time to live (in seconds) for the inserted reading
     */
    void insert(const SensorId& sid, const uint64_t ts, const int64_t value, const int64_t ttl=-1);

    /**
     * @brief This function inserts a single sensor reading into
     *        the database.
     * @param reading  A SensorDataStoreReading object
     * @param ttl      Time to live (in seconds) for the inserted reading
     */
    void insert(const SensorDataStoreReading& reading, const int64_t ttl=-1);
    
    /**
     * @brief This function inserts a single sensor reading into
     *        the database.
     * @param readings  A list of SensorDataStoreReading object
     * @param ttl       Time to live (in seconds) for the inserted readings
     */
    void insertBatch(const std::list<SensorDataStoreReading>& readings, const int64_t ttl=-1);
    
    /**
     * @brief Set the TTL for newly inserted sensor data.
     * @param ttl      The TTL for the sensor data in seconds.
     */
    void setTTL(const uint64_t ttl);

    /**
     * @brief Enables or disables logging of Cassandra insert errors
     * @param dl        true to enable logging, false otherwise
     */
    void setDebugLog(const bool dl);

    /**
     * @brief This function queries a sensor's values in
     *        the given time range.
     * @param result   The list where the results will be stored.
     * @param sid      The SensorId to query.
     * @param start    Start of the time series.
     * @param end      End of the time series.
     */
    void query(std::list<SensorDataStoreReading>& result, const SensorId& sid, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate=AGGREGATE_NONE);

    /**
     * @brief This function queries a the values of
     *        a set of sensors in the given time range.
     *        The weekstamp being used is in this case that of the sensor ID 
     *        at the front of the input list.
     * @param result   The list where the results will be stored.
     * @param sids     The list of SensorIds to query.
     * @param start    Start of the time series.
     * @param end      End of the time series.
     * @param storeSid If true, Sensor IDs will be retrieved for each reading.
     */
    void query(std::list<SensorDataStoreReading>& result, const std::list<SensorId>& sids, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate=AGGREGATE_NONE, const bool storeSids=true);
    
    /**
     * @brief This function performs a fuzzy query and returns the 
     *        closest sensor reading to the input timestamp.
     * @param result   The list where the results will be stored.
     * @param sid      The SensorId to query.
     * @param ts       The target timestamp.
     * @param tol_ns   Tolerance of the fuzzy query in nanoseconds.
     */
    void fuzzyQuery(std::list<SensorDataStoreReading>& result, const SensorId& sid, const TimeStamp& ts, const uint64_t tol_ns=3600000000000);

    /**
     * @brief This function performs a fuzzy query and returns the closest 
     *        sensor reading to the input timestamp, one per queried sensor.
     *        The weekstamp being used is in this case that of the sensor ID 
     *        at the front of the input list.
     * @param result   The list where the results will be stored.
     * @param sids     The list of SensorIds to query.
     * @param ts       The target timestamp.
     * @param tol_ns   Tolerance of the fuzzy query in nanoseconds.
     * @param storeSid If true, Sensor IDs will be retrieved for each reading.
     */
    void fuzzyQuery(std::list<SensorDataStoreReading>& result, const std::list<SensorId>& sids, const TimeStamp& ts, const uint64_t tol_ns=3600000000000, const bool storeSids=true);

    typedef void (*QueryCbFunc)(SensorDataStoreReading& reading, void* userData);
    /**
     * @brief This function queries a sensor's values in
     *        the given time range.
     * @param cbFunc   A function to called for each reading.
     * @param userData Pointer to user data that will be returned in the callback.
     * @param sid      The SensorId to query.
     * @param start    Start of the time series.
     * @param end      End of the time series.
     */
    void queryCB(QueryCbFunc cbFunc, void* userData, const SensorId& sid, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate=AGGREGATE_NONE);

    /**
     * @brief This function truncates all sensor data that is older than
     *        the specified week.
     * @param weekStamp   The 16-bit weekstamp generated from a cut-off date
     */
    void truncBeforeWeek(const uint16_t weekStamp);

    /**
     * @brief A shortcut constructor for a SensorDataStore object
     *        that allows accessing the data store through a
     *        connection that is already established.
     * @param conn     The Connection object of an established
     *                 connection to Cassandra.
     */
    SensorDataStore(Connection* conn);

    /**
     * @brief The standard destructor for a SensorDatStore object.
     */
    virtual ~SensorDataStore();
};

} /* End of namespace DCDB */

#endif /* DCDB_SENSORDATASTORE_H */
