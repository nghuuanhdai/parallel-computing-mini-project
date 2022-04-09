//================================================================================
// Name        : sensordatastore.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for inserting and querying DCDB sensor data.
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
 * @page libdcdb libdcdb
 * The libdcdb library is a dynamic runtime library providing
 * functions to initialize and access the DCDB data store. It
 * is being used by the CollectAgent to handle insertion of
 * data and can be used by tools responsible for data analysis.
 *
 * Its main class is the DCDB::SensorDataStore class which
 * provides functions to connect to the data store, initialize
 * an empty data base and to retrieve data.
 *
 * For its internal handling, DCDB::SensorDataStore relies on
 * the DCDB::SensorDataStoreImpl class (which hides all private
 * member functions belonging to the SensorDataStore class from
 * the header that is used by programmers who link against this
 * library). Raw database functionality is abstracted into the
 * CassandraBackend class (to easy switching to other
 * key-value style databases in the future).
 *
 * To use the library in your client application, simply
 * include the sensordatastore.h header file and initialize
 * an object of the SensorDataStore class.
 */

#include <string>
#include <iostream>
#include <cstdint>
#include <cinttypes>

#include "cassandra.h"

#include "dcdb/sensordatastore.h"
#include "sensordatastore_internal.h"
#include "dcdb/connection.h"
#include "dcdbglobals.h"

#include "dcdbendian.h"

using namespace DCDB;

SensorDataStoreReading::SensorDataStoreReading() {
}

SensorDataStoreReading::SensorDataStoreReading(const SensorId& sid, const uint64_t ts, const int64_t value) {
  this->sensorId = sid;
  this->timeStamp = TimeStamp(ts);
  this->value = value;
}

SensorDataStoreReading::~SensorDataStoreReading() {
}

/**
 * @details
 * Since we want high-performance inserts, we prepare the
 * insert CQL query in advance and only bind it on the actual
 * insert.
 */
void SensorDataStoreImpl::prepareInsert(const uint64_t ttl)
{
    CassError rc = CASS_OK;
    CassFuture* future = NULL;
    const char* query;
    
    /*
    * Free the old prepared if necessary.
    */
    if (preparedInsert) {
      cass_prepared_free(preparedInsert);
    }
    
    query = "INSERT INTO dcdb.sensordata (sid, ws, ts, value) VALUES (?, ?, ?, ?) USING TTL ? ;";
    future = cass_session_prepare(session, query);
    cass_future_wait(future);
    
    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
      connection->printError(future);
    } else {
      preparedInsert = cass_future_get_prepared(future);
    }
    cass_future_free(future);

    // Preparing the insert statement without a TTL clause
    if (preparedInsert_noTTL) {
        cass_prepared_free(preparedInsert_noTTL);
    }
    
    query = "INSERT INTO dcdb.sensordata (sid, ws, ts, value) VALUES (?, ?, ?, ?);";
    future = cass_session_prepare(session, query);
    cass_future_wait(future);
    
    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
    } else {
        preparedInsert_noTTL = cass_future_get_prepared(future);
    }
    cass_future_free(future);
        
    defaultTTL = ttl;
}

/**
 * @details
 * To insert a sensor reading, the Rsvd field of the SensorId must
 * be filled with a time component that ensures that the maximum
 * number of 2^32 columns per key is not exceeded while still
 * allowing relatively easy retrieval of data.
 *
 * We achieve this by using a "week-stamp" (i.e. number of weeks
 * since Unix epoch) within the Rsvd field of the SensorId before
 * calling the Cassandra Backend to do the raw insert.
 *
 * Applications should not call this function directly, but
 * use the insert function provided by the SensorDataStore class.
 */
void SensorDataStoreImpl::insert(const SensorId& sid, const uint64_t ts, const int64_t value, const int64_t ttl)
{
#if 0
  std::cout << "Inserting@SensorDataStoreImpl (" << sid->raw[0] << " " << sid->raw[1] << ", " << ts << ", " << value << ")" << std::endl;
#endif

  /* Calculate and insert week number */
  uint16_t week = ts / 604800000000000;
  int64_t ttlReal = (ttl<0 ? defaultTTL : ttl);
  std::string sidStr = sid.getId();
  
  CassStatement* statement = cass_prepared_bind(ttlReal<=0 ? preparedInsert_noTTL : preparedInsert);
  cass_statement_bind_string_by_name(statement, "sid", sidStr.c_str());
  cass_statement_bind_int16_by_name(statement, "ws", week);
  cass_statement_bind_int64_by_name(statement, "ts", ts);
  cass_statement_bind_int64_by_name(statement, "value", value);
  if(ttlReal>0)
      cass_statement_bind_int32(statement, 4, ttlReal);
  
  CassFuture* future = cass_session_execute(session, statement);
  cass_statement_free(statement);

  /* Don't wait for the future, just free it to make the call truly asynchronous */
  cass_future_free(future);
}

void SensorDataStoreImpl::insert(const SensorDataStoreReading& reading, const int64_t ttl) {
  insert(reading.sensorId, reading.timeStamp.getRaw(), reading.value, ttl);
}

void SensorDataStoreImpl::insertBatch(const std::list<SensorDataStoreReading>& readings, const int64_t ttl) {
  CassBatch* batch = cass_batch_new(CASS_BATCH_TYPE_UNLOGGED);
  
  int64_t ttlReal = (ttl<0 ? defaultTTL : ttl);
  
  for (auto r: readings) {
    /* Calculate week number */
    uint16_t week = r.timeStamp.getRaw() / 604800000000000;
    
    /* Add insert statement to batch */
    CassStatement* statement = cass_prepared_bind(ttlReal<=0 ? preparedInsert_noTTL : preparedInsert);
    cass_statement_bind_string_by_name(statement, "sid", r.sensorId.getId().c_str());
    cass_statement_bind_int16_by_name(statement, "ws", week);
    cass_statement_bind_int64_by_name(statement, "ts", r.timeStamp.getRaw());
    cass_statement_bind_int64_by_name(statement, "value", r.value);
    if(ttlReal>0)
        cass_statement_bind_int32(statement, 4, ttlReal);
    cass_batch_add_statement(batch, statement);
    cass_statement_free(statement);
  }
  
  /* Execute batch */
  CassFuture *future = cass_session_execute_batch(session, batch);
  cass_batch_free(batch);

  if(debugLog)
    cass_future_set_callback(future, DataStoreImpl_on_result, NULL);
  
  /* Don't wait for the future, just free it to make the call truly asynchronous */
  cass_future_free(future);
}

/**
 * @details
 * This function updates the prepared statement for inserts
 * with the new TTL value.
 */
void SensorDataStoreImpl::setTTL(const uint64_t ttl)
{
  prepareInsert(ttl);
}

/**
 * @brief Enables or disables logging of Cassandra insert errors
 * @param dl        true to enable logging, false otherwise
 */
void SensorDataStoreImpl::setDebugLog(const bool dl)
{
    debugLog = dl;
}

/**
 * @details
 * This function issues a regular query to the data store
 * and creates a SensorDataStoreReading object for each
 * entry which is stored in the result list.
 */
void SensorDataStoreImpl::query(std::list<SensorDataStoreReading>& result, const SensorId& sid, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate) {
  CassError rc = CASS_OK;
  CassStatement* statement = NULL;
  CassFuture *future = NULL;
  const CassPrepared* prepared = nullptr;
  
  std::string query = std::string("SELECT ts,");
  if (aggregate == AGGREGATE_NONE) {
    query.append("value");
  } else {
    query.append(AggregateString[aggregate] + std::string("(value) as value"));
  }
  query.append(" FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? AND ws = ? AND ts >= ? AND ts <= ? ;");
  future = cass_session_prepare(session, query.c_str());
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    cass_future_free(future);
    return;
  }

  prepared = cass_future_get_prepared(future);
  cass_future_free(future);
  
#if 0
  std::cout << "Query: " << query << std::endl << "sid: " << sid.toString() << " ts1: " << start.getRaw() << " ts2: " << end.getRaw() << std::endl;
#endif

  statement = cass_prepared_bind(prepared);
  cass_statement_set_paging_size(statement, PAGING_SIZE);
  cass_statement_bind_string(statement, 0, sid.getId().c_str());
  cass_statement_bind_int16(statement, 1, sid.getRsvd());
  cass_statement_bind_int64(statement, 2, start.getRaw());
  cass_statement_bind_int64(statement, 3, end.getRaw());
  
  bool morePages = false;
  do {
      future = cass_session_execute(session, statement);
      cass_future_wait(future);

      if (cass_future_error_code(future) == CASS_OK) {
          const CassResult *cresult = cass_future_get_result(future);
          CassIterator *rows = cass_iterator_from_result(cresult);

          SensorDataStoreReading entry;

          while (cass_iterator_next(rows)) {
              const CassRow *row = cass_iterator_get_row(rows);

              cass_int64_t ts, value;
              cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &ts);
              cass_value_get_int64(cass_row_get_column_by_name(row, "value"), &value);

              entry.sensorId = sid;
              entry.timeStamp = (uint64_t) ts;
              entry.value = (int64_t) value;

              result.push_back(entry);
#if 0
              if (localtime) {
                  t.convertToLocal();
              }
              if (raw) {
                  std::cout << sensorName << "," << std::dec << t.getRaw() << "," << std::dec << value << std::endl;
              }
              else {
                  std::cout << sensorName << "," << t.getString() << "," << std::dec << value << std::endl;
              }
#endif
          }
          
          if((morePages = cass_result_has_more_pages(cresult)))
              cass_statement_set_paging_state(statement, cresult);

          cass_iterator_free(rows);
          cass_result_free(cresult);
      } else {
          morePages = false;
      }
      
      cass_future_free(future);
  } 
  while(morePages);

  cass_statement_free(statement);
  cass_prepared_free(prepared);
  result.reverse();
}

/**
 * @details
 * This function issues a regular query to the data store,
 * queries an arbitrary number of sensors simultaneously
 * and creates a SensorDataStoreReading object for each
 * entry which is stored in the result list.
 */
void SensorDataStoreImpl::query(std::list<SensorDataStoreReading>& result, const std::list<SensorId>& sids, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate, const bool storeSids) {
    if(sids.empty())
        return;
    
    CassError rc = CASS_OK;
    CassStatement* statement = NULL;
    CassFuture *future = NULL;
    const CassPrepared* prepared = nullptr;

    std::string query = std::string(storeSids ? "SELECT sid,ts," : "SELECT ts,");
    if (aggregate == AGGREGATE_NONE) {
        query.append("value");
    } else {
        query.append(AggregateString[aggregate] + std::string("(value) as value"));
    }
    query.append(" FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? AND ws = ? AND ts >= ? AND ts <= ? ;");
    future = cass_session_prepare(session, query.c_str());
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    // Paged asynchronous queries require keeping track of statements
    std::list<std::pair<CassStatement*, CassFuture*>> futures;
    auto sidIt = sids.begin();
    size_t sidCtr = 0;

    // Limiting the amount of concurrent requests with small queues
    uint32_t realGroupLimit = connection->getQueueSizeIo()/10 < QUERY_GROUP_LIMIT ? connection->getQueueSizeIo()/10 : QUERY_GROUP_LIMIT;

    while(sidIt != sids.end()) {
        futures.clear();
        sidCtr = 0;
        
        // Breaking up the original list of sids in chunks
        while(sidIt != sids.end() && sidCtr < realGroupLimit) {
            statement = cass_prepared_bind(prepared);
            cass_statement_set_paging_size(statement, PAGING_SIZE);
            cass_statement_bind_string(statement, 0, sidIt->getId().c_str());
            cass_statement_bind_int16(statement, 1, sids.front().getRsvd());
            cass_statement_bind_int64(statement, 2, start.getRaw());
            cass_statement_bind_int64(statement, 3, end.getRaw());
            futures.push_back(std::pair<CassStatement*, CassFuture*>(statement, cass_session_execute(session, statement)));
            
            ++sidIt;
            ++sidCtr;
        }

        do {
            // Keeps track of outstanding pages from current queries
            std::list<std::pair<CassStatement *, CassFuture *>> futurePages;
            for (auto &fut : futures) {
                bool morePages = false;
                cass_future_wait(fut.second);

                if (cass_future_error_code(fut.second) == CASS_OK) {
                    const CassResult *cresult = cass_future_get_result(fut.second);
                    CassIterator *rows = cass_iterator_from_result(cresult);
                    SensorDataStoreReading entry;
                    cass_int64_t ts, value;
                    const char *name;
                    size_t name_len;

                    while (cass_iterator_next(rows)) {
                        const CassRow *row = cass_iterator_get_row(rows);

                        cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &ts);
                        cass_value_get_int64(cass_row_get_column_by_name(row, "value"), &value);
                        entry.timeStamp = (uint64_t) ts;
                        entry.value = (int64_t) value;

                        if (storeSids) {
                            cass_value_get_string(cass_row_get_column_by_name(row, "sid"), &name, &name_len);
                            entry.sensorId = SensorId(std::string(name, name_len));
                        }

                        result.push_back(entry);
#if 0
                        if (localtime) {
                          t.convertToLocal();
                      }
                      if (raw) {
                          std::cout << sensorName << "," << std::dec << t.getRaw() << "," << std::dec << value << std::endl;
                      }
                      else {
                          std::cout << sensorName << "," << t.getString() << "," << std::dec << value << std::endl;
                      }
#endif
                    }
                    
                    if ((morePages = cass_result_has_more_pages(cresult))) {
                        cass_statement_set_paging_state(fut.first, cresult);
                        futurePages.push_back(std::pair<CassStatement *, CassFuture *>(fut.first, cass_session_execute(session, fut.first)));
                    } else {
                        cass_statement_free(fut.first);
                    }

                    cass_iterator_free(rows);
                    cass_result_free(cresult);
                } else {
                    cass_statement_free(fut.first);
                    morePages = false;
                }
                cass_future_free(fut.second);
            }
            futures.clear();
            futures = futurePages;
            futurePages.clear();
        } while(!futures.empty());
    }
    
    cass_prepared_free(prepared);
    result.reverse();
}

/**
 * @details
 * This function performs a fuzzy query on the datastore,
 * picking a single sensor readings that is closest to
 * the one given as input
 */
void SensorDataStoreImpl::fuzzyQuery(std::list<SensorDataStoreReading>& result, const SensorId& sid, const TimeStamp& ts, const uint64_t tol_ns) {
    /* Find the readings before time t */
    CassError rc = CASS_OK;
    CassStatement* statement = NULL;
    CassFuture *future = NULL;
    const CassPrepared* prepared = nullptr;
    const char* queryBefore = "SELECT sid,ts,value FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? AND ws = ? AND ts <= ? ORDER BY ws DESC, ts DESC LIMIT 1";

    future = cass_session_prepare(session, queryBefore);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);
    cass_statement_bind_string(statement, 0, sid.getId().c_str());
    cass_statement_bind_int16(statement, 1, sid.getRsvd());
    cass_statement_bind_int64(statement, 2, ts.getRaw());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);
    SensorDataStoreReading r;

    if (cass_future_error_code(future) == CASS_OK) {
        const CassResult* cresult = cass_future_get_result(future);
        CassIterator* rows = cass_iterator_from_result(cresult);

        while (cass_iterator_next(rows)) {
            const CassRow* row = cass_iterator_get_row(rows);

            cass_int64_t tsInt, value;
            cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &tsInt);
            cass_value_get_int64(cass_row_get_column_by_name(row, "value"), &value);

            if(ts.getRaw() - (uint64_t)tsInt < tol_ns) {
                r.sensorId = sid;
                r.timeStamp = (uint64_t) tsInt;
                r.value = (int64_t) value;
                result.push_back(r);
            }
        }
        cass_iterator_free(rows);
        cass_result_free(cresult);
    }

    cass_statement_free(statement);
    cass_future_free(future);
    cass_prepared_free(prepared);
    result.reverse();
}

/**
 * @details
 * This function performs a fuzzy query on the datastore,
 * picking readings from a set of sensors that are closest to
 * the timestamp given as input
 */
void SensorDataStoreImpl::fuzzyQuery(std::list<SensorDataStoreReading>& result, const std::list<SensorId>& sids, const TimeStamp& ts, const uint64_t tol_ns, const bool storeSids) {
    if(sids.empty())
        return;

    /* Find the readings before time t */
    CassError rc = CASS_OK;
    CassStatement *statement = NULL;
    CassFuture *future = NULL;
    const CassPrepared *prepared = nullptr;
    const char *queryBefore;
    if(storeSids) 
        queryBefore = "SELECT sid,ts,value FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? AND ws = ? AND ts <= ? ORDER BY ws DESC, ts DESC LIMIT 1";
    else
        queryBefore = "SELECT ts,value FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? AND ws = ? AND ts <= ? ORDER BY ws DESC, ts DESC LIMIT 1";

    future = cass_session_prepare(session, queryBefore);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);
    
    std::list<CassFuture*> futures;
    auto sidIt = sids.begin();
    size_t sidCtr = 0;
    
    // Limiting the amount of concurrent requests with small queues
    uint32_t realGroupLimit = connection->getQueueSizeIo()/10 < QUERY_GROUP_LIMIT ? connection->getQueueSizeIo()/10 : QUERY_GROUP_LIMIT;
    
    while(sidIt != sids.end()) {
        sidCtr = 0;
        futures.clear();
        
        // Breaking up the original list of sids in chunks
        while(sidIt != sids.end() && sidCtr < realGroupLimit) {
            statement = cass_prepared_bind(prepared);
            cass_statement_set_paging_size(statement, -1);
            cass_statement_bind_string(statement, 0, sidIt->getId().c_str());
            cass_statement_bind_int16(statement, 1, sids.front().getRsvd());
            cass_statement_bind_int64(statement, 2, ts.getRaw());
            futures.push_back(cass_session_execute(session, statement));
            cass_statement_free(statement);
            
            ++sidIt;
            ++sidCtr;
        }
        
        SensorDataStoreReading r;
        for(auto& fut : futures) {
            cass_future_wait(fut);
            if (cass_future_error_code(fut) == CASS_OK) {
                const CassResult *cresult = cass_future_get_result(fut);
                CassIterator *rows = cass_iterator_from_result(cresult);
                cass_int64_t tsInt, value;
                const char *name;
                size_t name_len;

                while (cass_iterator_next(rows)) {
                    const CassRow *row = cass_iterator_get_row(rows);

                    cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &tsInt);
                    cass_value_get_int64(cass_row_get_column_by_name(row, "value"), &value);

                    if (ts.getRaw() - (uint64_t) tsInt < tol_ns) {
                        if(storeSids) {
                            cass_value_get_string(cass_row_get_column_by_name(row, "sid"), &name, &name_len);
                            r.sensorId = SensorId(std::string(name, name_len));
                        }
                        r.timeStamp = (uint64_t) tsInt;
                        r.value = (int64_t) value;
                        result.push_back(r);
                    }
                }
                
                cass_iterator_free(rows);
                cass_result_free(cresult);
            }
            cass_future_free(fut);
        }
    }
    cass_prepared_free(prepared);
    result.reverse();
}


/**
 * @details
 * This function issues a regular query to the data store
 * and calls cbFunc for every reading.
 */
void SensorDataStoreImpl::queryCB(SensorDataStore::QueryCbFunc cbFunc, void* userData, const SensorId& sid, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate)
{
  CassError rc = CASS_OK;
  CassStatement* statement = NULL;
  CassFuture *future = NULL;
  const CassPrepared* prepared = nullptr;
  
  std::string query = std::string("SELECT ts,");
  if (aggregate == AGGREGATE_NONE) {
    query.append("value");
  } else {
    query.append(AggregateString[aggregate] + std::string("(value) as value"));
  }
  query.append(" FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? AND ws = ? AND ts >= ? AND ts <= ? ;");
  future = cass_session_prepare(session, query.c_str());
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    cass_future_free(future);
    return;
  }

  prepared = cass_future_get_prepared(future);
  cass_future_free(future);
  
  statement = cass_prepared_bind(prepared);
  cass_statement_set_paging_size(statement, PAGING_SIZE);
  cass_statement_bind_string(statement, 0, sid.getId().c_str());
  cass_statement_bind_int16(statement, 1, sid.getRsvd());
  cass_statement_bind_int64(statement, 2, start.getRaw());
  cass_statement_bind_int64(statement, 3, end.getRaw());

  bool morePages = false;
  do {
      future = cass_session_execute(session, statement);
      cass_future_wait(future);
    
      if (cass_future_error_code(future) == CASS_OK) {
          const CassResult* cresult = cass_future_get_result(future);
          CassIterator* rows = cass_iterator_from_result(cresult);
    
          SensorDataStoreReading entry;
    
          while (cass_iterator_next(rows)) {
              const CassRow* row = cass_iterator_get_row(rows);
    
              cass_int64_t ts, value;
              cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &ts);
              cass_value_get_int64(cass_row_get_column_by_name(row, "value"), &value);
    
              entry.sensorId = sid;
              entry.timeStamp = (uint64_t)ts;
              entry.value = (int64_t)value;
    
              cbFunc(entry, userData);
          }

          if((morePages = cass_result_has_more_pages(cresult)))
              cass_statement_set_paging_state(statement, cresult);
          
          cass_iterator_free(rows);
          cass_result_free(cresult);
      } else {
          morePages = false;
      }

      cass_future_free(future);

  }
  while(morePages);

  cass_statement_free(statement);
  cass_prepared_free(prepared);
}

/**
 * @details
 * This function deletes all data from the sensordata store
 * that is older than weekStamp-1 weeks.
 */
void SensorDataStoreImpl::truncBeforeWeek(const uint16_t weekStamp)
{
  /* List of rows that should be deleted */
  std::list<SensorId> deleteList;

  /* Query the database to collect all rows */
  CassError rc = CASS_OK;
  CassStatement* statement = nullptr;
  CassFuture* future = nullptr;
  const char* query = "SELECT DISTINCT sid,ws FROM " KEYSPACE_NAME "." CF_SENSORDATA ";";

  statement = cass_statement_new(query, 0);
  future = cass_session_execute(session, statement);
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
      connection->printError(future);
      cass_future_free(future);
      return;
  }

  const CassResult* result = cass_future_get_result(future);
  cass_future_free(future);

  CassIterator* iterator = cass_iterator_from_result(result);

  /* Iterate over all rows and filter out those, that are too old */
  while (cass_iterator_next(iterator)) {
      const CassRow* row = cass_iterator_get_row(iterator);
      const char* res;
      size_t       res_len;
      int16_t     res_ws;
      cass_value_get_string(cass_row_get_column_by_name(row, "sid"), &res, &res_len);
      cass_value_get_int16(cass_row_get_column_by_name(row, "ws"), &res_ws);
      
      SensorId sensor;
      std::string id(res, res_len);
      sensor.setId(id);
      sensor.setRsvd(res_ws);

      /* Check if the sensorId's rsvd field is smaller than the weekStamp */
      if (sensor.getRsvd() < weekStamp) {
          deleteList.push_back(sensor);
      }
  }
  cass_result_free(result);
  cass_iterator_free(iterator);
  cass_statement_free(statement);

  /* Now iterate over all entries in the deleteList and delete them */
  for (std::list<SensorId>::iterator it = deleteList.begin(); it != deleteList.end(); it++) {
      deleteRow(*it);
  }
}

/**
 * @details
 * Deleting entire rows is rather efficient compared to deleting individual columns.
 */
void SensorDataStoreImpl::deleteRow(const SensorId& sid)
{
  CassError rc = CASS_OK;
  CassStatement* statement = NULL;
  CassFuture *future = NULL;
  const CassPrepared* prepared = nullptr;
  const char* query = "DELETE FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? and ws = ?;";

  future = cass_session_prepare(session, query);
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    cass_future_free(future);
    return;
  }

  prepared = cass_future_get_prepared(future);
  cass_future_free(future);
  
  statement = cass_prepared_bind(prepared);
  cass_statement_bind_string(statement, 0, sid.getId().c_str());
  cass_statement_bind_int16(statement, 1, sid.getRsvd());

  future = cass_session_execute(session, statement);
  cass_future_wait(future);

  cass_statement_free(statement);
  cass_future_free(future);
  cass_prepared_free(prepared);
}

/**
 * @details
 * This constructor sets the internal connection variable to
 * the externally provided Connection object and also
 * retrieves the CassSession pointer of the connection.
 */
SensorDataStoreImpl::SensorDataStoreImpl(Connection* conn)
{
  connection = conn;
  session = connection->getSessionHandle();
  debugLog = false;
  defaultTTL = 0;

  preparedInsert = nullptr;
  preparedInsert_noTTL = nullptr;
  prepareInsert(defaultTTL);
}

/**
 * @details
 * Due to the simplicity of the class, the destructor is left empty.
 */
SensorDataStoreImpl::~SensorDataStoreImpl()
{
  connection = nullptr;
  session = nullptr;
  if (preparedInsert) {
      cass_prepared_free(preparedInsert);
  }
  if (preparedInsert_noTTL) {
      cass_prepared_free(preparedInsert_noTTL);
  }
}

/**
 * @details
 * Instead of doing the actual work, this function simply
 * forwards to the insert function of the SensorDataStoreImpl
 * class.
 */
void SensorDataStore::insert(const SensorId& sid, const uint64_t ts, const int64_t value, const int64_t ttl)
{
    impl->insert(sid, ts, value, ttl);
}

void SensorDataStore::insert(const SensorDataStoreReading& reading, const int64_t ttl)
{
  impl->insert(reading, ttl);
}

void SensorDataStore::insertBatch(const std::list<SensorDataStoreReading>& readings, const int64_t ttl) {
  impl->insertBatch(readings, ttl);
}

/**
 * @details
 * Instead of doing the actual work, this function simply
 * forwards to the insert function of the SensorDataStoreImpl
 * class.
 */
void SensorDataStore::setTTL(const uint64_t ttl)
{
    impl->setTTL(ttl);
}

/**
 * @brief Enables or disables logging of Cassandra insert errors
 * @param dl        true to enable logging, false otherwise
 */
void SensorDataStore::setDebugLog(const bool dl)
{
    impl->setDebugLog(dl);
}

/**
 * @details
 * Instead of doing the actual work, this function simply
 * forwards to the insert function of the SensorDataStoreImpl
 * class.
 */
void SensorDataStore::query(std::list<SensorDataStoreReading>& result, const SensorId& sid, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate)
{
    impl->query(result, sid, start, end, aggregate);
}

/**
 * @details
 * Instead of doing the actual work, this function simply
 * forwards to the insert function of the SensorDataStoreImpl
 * class.
 */
void SensorDataStore::query(std::list<SensorDataStoreReading>& result, const std::list<SensorId>& sids, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate, const bool storeSids)
{
    impl->query(result, sids, start, end, aggregate, storeSids);
}

/**
 * @details
 * Instead of doing the actual work, this function simply
 * forwards to the insert function of the SensorDataStoreImpl
 * class.
 */
void SensorDataStore::fuzzyQuery(std::list<SensorDataStoreReading>& result, const SensorId& sid, const TimeStamp& ts, const uint64_t tol_ns) {
    impl->fuzzyQuery(result, sid, ts, tol_ns);
}

/**
 * @details
 * Instead of doing the actual work, this function simply
 * forwards to the insert function of the SensorDataStoreImpl
 * class.
 */
void SensorDataStore::fuzzyQuery(std::list<SensorDataStoreReading>& result, const std::list<SensorId>& sids, const TimeStamp& ts, const uint64_t tol_ns, const bool storeSids) {
    impl->fuzzyQuery(result, sids, ts, tol_ns, storeSids);
}

/**
 * @details
 * Instead of doing the actual work, this function simply
 * forwards to the insert function of the SensorDataStoreImpl
 * class.
 */
void SensorDataStore::queryCB(SensorDataStore::QueryCbFunc cbFunc, void* userData, const SensorId& sid, const TimeStamp& start, const TimeStamp& end, const QueryAggregate aggregate)
{
    return impl->queryCB(cbFunc, userData, sid, start, end, aggregate);
}

/**
 * @details
 * Instead of doing the actual work, this function simply
 * forwards to the insert function of the SensorDataStoreImpl
 * class.
 */
void SensorDataStore::truncBeforeWeek(const uint16_t weekStamp)
{
    return impl->truncBeforeWeek(weekStamp);
}

/**
 * @details
 * This constructor allocates the implementation class which
 * holds the actual implementation of the class functionality.
 */
SensorDataStore::SensorDataStore(Connection* conn)
{
    impl = new SensorDataStoreImpl(conn);
}

/**
 * @details
 * The SensorDataStore desctructor deallocates the
 * SensorDataStoreImpl and CassandraBackend objects.
 */
SensorDataStore::~SensorDataStore()
{
  /* Clean up... */
  if (impl)
    delete impl;
}
