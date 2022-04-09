//================================================================================
// Name        : sensordatastore_internal.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Internal interface for inserting and querying DCDB sensor data.
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

/*
 * @file
 * @brief This file contains the internal functions of the
 *        Sensor Data Store which are provided by the
 *        SensorDataStoreImpl class.
 */

#ifndef DCDB_SENSORDATASTORE_INTERNAL_H
#define DCDB_SENSORDATASTORE_INTERNAL_H

#include <string>

#include "dcdb/sensordatastore.h"
#include "dcdb/connection.h"

#define QUERY_GROUP_LIMIT 1000

namespace DCDB {
static std::string const AggregateString[] = {"", "min", "max", "avg", "sum", "count"};

//Definition of callback function
void DataStoreImpl_on_result(CassFuture_* future, void* data) {
    /* This result will now return immediately */
    static CassError rcPrev = CASS_OK;
    static uint32_t ctr = 0;
    CassError rc = cass_future_error_code(future);
    if(rc != CASS_OK) {
        if(rc != rcPrev) {
            std::cout << "Cassandra Backend Error: " << cass_error_desc(rc) << std::endl;
            ctr = 0;
            rcPrev = rc;
        } else if(++ctr%10000 == 0)
            std::cout << "Cassandra Backend Error: " << cass_error_desc(rc) << " (" << ctr << " more)" << std::endl;
    }
}

/**
 * @brief The SensorDataStoreImpl class contains all protected
 *        functions belonging to SensorDataStore which are
 *        hidden from the user of the libdcdb library.
 */
class SensorDataStoreImpl
{
protected:
  Connection*     connection;     /**< The Connection object that does the low-level stuff for us. */
  CassSession*        session;        /**< The CassSession object given by the connection. */
  const CassPrepared* preparedInsert; /**< The prepared statement for fast insertions. */
  const CassPrepared* preparedInsert_noTTL; /**< The prepared statement for fast insertions, without TTL. */
  bool  debugLog;
  uint64_t defaultTTL;

  /**
   * @brief Prepare for insertions.
   * @param ttl   A TTL that will be set for newly inserted values. Set to 0 to insert without TTL.
   */
  void prepareInsert(uint64_t ttl);

public:
  /**
   * @brief This function inserts a sensor reading into the database
   * @param sid    The SensorId object representing the sensor (typically obtained from topicToSid)
   * @param ts     The timestamp of the reading (nanoseconds since Unix epoch)
   * @param value  The sensor reading as 64-bit integer
   * @param ttl    Time to live (in seconds) for the inserted reading
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
   * @brief This function sets the TTL of newly inserted readings.
   * @param ttl    The TTL to be used for new inserts in seconds.
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
  void query(std::list<SensorDataStoreReading>& result, const SensorId& sid, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate);

  /**
   * @brief This function queries a the values of
   *        a set of sensors in the given time range.
   *        The weekstamp being used is in this case that 
   *        of the sensor ID at the front of the input list.
   * @param result   The list where the results will be stored.
   * @param sids     The list of SensorIds to query.
   * @param start    Start of the time series.
   * @param end      End of the time series.
   * @param storeSid If true, Sensor IDs will be retrieved for each reading.
   */
  void query(std::list<SensorDataStoreReading>& result, const std::list<SensorId>& sids, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate, const bool storeSids);

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

  /**
   * @brief This function queries a sensor's values in
   *        the given time range.
   * @param cbFunc   A function to called for each reading.
   * @param userData Pointer to user data that will be returned in the callback.
   * @param sid      The SensorId to query.
   * @param start    Start of the time series.
   * @param end      End of the time series.
   */
  void queryCB(SensorDataStore::QueryCbFunc cbFunc, void* userData, const SensorId& sid, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate);

  /**
   * @brief This function truncates all sensor data that is older than
   *        the specified week.
   * @param weekStamp   The 16-bit weekstamp generated from a cut-off date
   */
  void truncBeforeWeek(const uint16_t weekStamp);

  /**
   * @brief This function deletes a row from the sensordatastore.
   * @param sid     SensorId object that identifies the row to be deleted.
   */
  void deleteRow(const SensorId& sid);

  /**
   * @brief This is the standard constructor of the SensorDataStoreImpl class.
   * @param csb   A CassandraBackend object to do the raw database access.
   */
  SensorDataStoreImpl(Connection* conn);

  /**
   * @brief The standard desctructor of SensorDataStoreImpl.
   */
  virtual ~SensorDataStoreImpl();
};

} /* End of namespace DCDB */

#endif /* DCDB_SENSORDATASTORE_INTERNAL_H */
