//================================================================================
// Name        : calievtdatastore.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for inserting and querying Caliper event data.
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
 * @file
 * @brief This file contains actual implementations of the calievtdatastore
 * interface. The interface itself forwards all functions to the internal
 * CaliEvtDataStoreImpl. The real logic is implemented in the CaliEvtDataStoreImpl.
 */

#include <cinttypes>
#include <iostream>
#include <list>
#include <string>

#include "cassandra.h"

#include "dcdb/calievtdatastore.h"
#include "calievtdatastore_internal.h"
#include "dcdb/connection.h"
#include "dcdbglobals.h"

using namespace DCDB;

/**
 * @details
 * Since we want high-performance inserts, we prepare the insert CQL query in
 * advance and only bind it on the actual insert.
 */
void CaliEvtDataStoreImpl::prepareInsert(uint64_t ttl) {
    CassError rc = CASS_OK;
    CassFuture* future = NULL;
    const char* query;

    /* Free the old prepared if necessary. */
    if (preparedInsert) {
        cass_prepared_free(preparedInsert);
    }

    query = "INSERT INTO " CED_KEYSPACE_NAME "." CF_CALIEVTDATA
          " (sid, ws, ts, value) VALUES (?, ?, ?, ?) USING TTL ? ;";
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
    query = "INSERT INTO " CED_KEYSPACE_NAME "." CF_CALIEVTDATA
          " (sid, ws, ts, value) VALUES (?, ?, ?, ?);";
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
 * To insert a Caliper Event, the Rsvd field of the SensorId must
 * be filled with a time component that ensures that the maximum
 * number of 2^32 columns per key is not exceeded while still
 * allowing relatively easy retrieval of data.
 *
 * We achieve this by using a "week-stamp" (i.e. number of weeks
 * since Unix epoch) within the Rsvd field of the SensorId before
 * calling the Cassandra Backend to do the raw insert.
 *
 * Applications should not call this function directly, but
 * use the insert function provided by the CaliEvtDataStore class.
 */
void CaliEvtDataStoreImpl::insert(SensorId* sid, uint64_t ts, const std::string& event, int64_t ttl) {
    /* Calculate and insert week number */
    uint16_t week = ts / 604800000000000;
    sid->setRsvd(week);

    int64_t ttlReal = (ttl<0 ? defaultTTL : ttl);

    CassStatement* statement = cass_prepared_bind(ttlReal<=0 ? preparedInsert_noTTL : preparedInsert);
    cass_statement_bind_string_by_name(statement, "sid", sid->getId().c_str());
    cass_statement_bind_int16_by_name(statement, "ws", week);
    cass_statement_bind_int64_by_name(statement, "ts", ts);
    cass_statement_bind_string_by_name(statement, "value", event.c_str());
    if(ttlReal>0)
        cass_statement_bind_int32(statement, 4, ttlReal);

    /* Execute statement */
    CassFuture* future = cass_session_execute(session, statement);
    cass_statement_free(statement);

    if(debugLog) {
        cass_future_set_callback(future, CaliEvtDataStoreImpl_on_result, NULL);
    }

    /* Don't wait for the future, just free it to make the call truly asynchronous */
    cass_future_free(future);
}

/**
 * @details
 * Forward to previous insert() implementation.
 */
void CaliEvtDataStoreImpl::insert(CaliEvtData& data, int64_t ttl) {
    insert(&data.eventId, data.timeStamp.getRaw(), data.event, ttl);
}

void CaliEvtDataStoreImpl::insertBatch(std::list<CaliEvtData>& datas, int64_t ttl) {
    CassBatch* batch = cass_batch_new(CASS_BATCH_TYPE_UNLOGGED);

    int64_t ttlReal = (ttl<0 ? defaultTTL : ttl);

    for (auto& d: datas) {
        /* Calculate and insert week number */
        uint16_t week = d.timeStamp.getRaw() / 604800000000000;
        d.eventId.setRsvd(week);

        /* Add insert statement to batch */
        CassStatement* statement = cass_prepared_bind(ttlReal<=0 ? preparedInsert_noTTL : preparedInsert);
        cass_statement_bind_string_by_name(statement, "sid", d.eventId.getId().c_str());
        cass_statement_bind_int16_by_name(statement, "ws", week);
        cass_statement_bind_int64_by_name(statement, "ts", d.timeStamp.getRaw());
        cass_statement_bind_string_by_name(statement, "value", d.event.c_str());
        if(ttlReal>0)
          cass_statement_bind_int32(statement, 4, ttlReal);
        cass_batch_add_statement(batch, statement);
        cass_statement_free(statement);
    }

    /* Execute batch */
    CassFuture *future = cass_session_execute_batch(session, batch);
    cass_batch_free(batch);

    if(debugLog) {
        cass_future_set_callback(future, CaliEvtDataStoreImpl_on_result, NULL);
    }

    /* Don't wait for the future, just free it to make the call truly asynchronous */
    cass_future_free(future);
}

/**
 * @details
 * This function updates the prepared statement for inserts
 * with the new TTL value.
 */
void CaliEvtDataStoreImpl::setTTL(uint64_t ttl) {
    prepareInsert(ttl);
}

/**
 * @brief Enables or disables logging of Cassandra insert errors
 * @param dl        true to enable logging, false otherwise
 */
void CaliEvtDataStoreImpl::setDebugLog(bool dl) {
    debugLog = dl;
}

/**
 * @details
 * This function issues a regular query to the data store
 * and creates a CaliEvtData object for each
 * entry which is stored in the result list.
 */
void CaliEvtDataStoreImpl::query(std::list<CaliEvtData>& result, SensorId& sid, TimeStamp& start, TimeStamp& end) {
    CassError rc = CASS_OK;
    CassStatement* statement = NULL;
    CassFuture *future = NULL;
    const CassPrepared* prepared = nullptr;

    std::string query = std::string("SELECT ts, value FROM " CED_KEYSPACE_NAME "." CF_CALIEVTDATA
                                    " WHERE sid = ? AND ws = ? AND ts >= ? AND ts <= ? ;");
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
    
            CaliEvtData entry;
    
            while (cass_iterator_next(rows)) {
                const CassRow* row = cass_iterator_get_row(rows);
    
                cass_int64_t ts;
                const char* eventStr;
                size_t eventStr_len;
                cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &ts);
                cass_value_get_string(cass_row_get_column_by_name(row, "value"), &eventStr, &eventStr_len);
    
                entry.eventId = sid;
                entry.timeStamp = (uint64_t)ts;
                entry.event = std::string(eventStr, eventStr_len);
    
                result.push_back(entry);
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
 * This function performs a fuzzy query on the datastore,
 * picking a single sensor readings that is closest to
 * the one given as input
 */
void CaliEvtDataStoreImpl::fuzzyQuery(std::list<CaliEvtData>& result, SensorId& sid, TimeStamp& ts, uint64_t tol_ns) {
    size_t elCtr = result.size();
    /* Issue a standard query */
    query(result, sid, ts, ts);
    // We got a sensor reading, and return
    if(elCtr < result.size())
        return;

    /* Find the readings before and after time t */
    CassError rc = CASS_OK;
    CassStatement* statement = NULL;
    CassFuture *future = NULL;
    const CassPrepared* prepared = nullptr;
    const char* queryBefore = "SELECT * FROM " CED_KEYSPACE_NAME "." CF_CALIEVTDATA " WHERE sid = ? AND ws = ? AND ts <= ? ORDER BY ws DESC, ts DESC LIMIT 1";
    const char* queryAfter = "SELECT * FROM " CED_KEYSPACE_NAME "." CF_CALIEVTDATA " WHERE sid = ? AND ws = ? AND ts > ? LIMIT 1";

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
    CaliEvtData d;
    uint64_t distNow=tol_ns, minDist=tol_ns;

    if (cass_future_error_code(future) == CASS_OK) {
        const CassResult* cresult = cass_future_get_result(future);
        CassIterator* rows = cass_iterator_from_result(cresult);

        while (cass_iterator_next(rows)) {
            const CassRow* row = cass_iterator_get_row(rows);

            cass_int64_t tsInt;
            const char* eventStr;
            size_t eventStr_len;
            cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &tsInt);
            cass_value_get_string(cass_row_get_column_by_name(row, "value"), &eventStr, &eventStr_len);

            distNow = (uint64_t)tsInt < ts.getRaw() ? ts.getRaw()-(uint64_t)tsInt : (uint64_t)tsInt-ts.getRaw();
            if(distNow<minDist) {
                minDist = distNow;
                d.eventId = sid;
                d.timeStamp = (uint64_t) tsInt;
                d.event = std::string(eventStr, eventStr_len);
            }
        }
        cass_iterator_free(rows);
        cass_result_free(cresult);
    }

    cass_statement_free(statement);
    cass_future_free(future);
    cass_prepared_free(prepared);

    /* Query after... */
    future = cass_session_prepare(session, queryAfter);
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

    if (cass_future_error_code(future) == CASS_OK) {
        const CassResult* cresult = cass_future_get_result(future);
        CassIterator* rows = cass_iterator_from_result(cresult);

        while (cass_iterator_next(rows)) {
            const CassRow* row = cass_iterator_get_row(rows);

            cass_int64_t tsInt;
            const char* eventStr;
            size_t eventStr_len;
            cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &tsInt);
            cass_value_get_string(cass_row_get_column_by_name(row, "value"), &eventStr, &eventStr_len);

            distNow = (uint64_t)tsInt < ts.getRaw() ? ts.getRaw()-(uint64_t)tsInt : (uint64_t)tsInt-ts.getRaw();
            if(distNow<minDist) {
                minDist = distNow;
                d.eventId = sid;
                d.timeStamp = (uint64_t) tsInt;
                d.event = std::string(eventStr, eventStr_len);
            }
        }

        cass_iterator_free(rows);
        cass_result_free(cresult);
    }

    cass_statement_free(statement);
    cass_future_free(future);
    cass_prepared_free(prepared);

    if(minDist < tol_ns)
        result.push_back(d);
}

/**
 * @details
 * This function issues a regular query to the data store
 * and calls cbFunc for every reading.
 */
void CaliEvtDataStoreImpl::queryCB(CaliEvtDataStore::QueryCECbFunc cbFunc, void* userData, SensorId& sid, TimeStamp& start, TimeStamp& end) {
    CassError rc = CASS_OK;
    CassStatement* statement = NULL;
    CassFuture *future = NULL;
    const CassPrepared* prepared = nullptr;

    std::string query = std::string("SELECT ts, value FROM " CED_KEYSPACE_NAME "." CF_CALIEVTDATA
                                        " WHERE sid = ? AND ws = ? AND ts >= ? AND ts <= ? ;");
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
    
            CaliEvtData entry;
    
            while (cass_iterator_next(rows)) {
                const CassRow* row = cass_iterator_get_row(rows);
    
                cass_int64_t ts;
                const char* eventStr;
                size_t eventStr_len;
                cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &ts);
                cass_value_get_string(cass_row_get_column_by_name(row, "value"), &eventStr, &eventStr_len);
    
                entry.eventId = sid;
                entry.timeStamp = (uint64_t)ts;
                entry.event = std::string(eventStr, eventStr_len);
    
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
 * This constructor sets the internal connection variable to
 * the externally provided Connection object and also
 * retrieves the CassSession pointer of the connection. Further on, it prepares
 * insert statements for faster actual inserts later on.
 */
CaliEvtDataStoreImpl::CaliEvtDataStoreImpl(Connection* conn) {
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
 * The destructor just resets the internal pointers. Deletion of the pointers
 * (except prepared inserts) is not our responsibility.
 */
CaliEvtDataStoreImpl::~CaliEvtDataStoreImpl() {
    connection = nullptr;
    session = nullptr;
    if (preparedInsert) {
        cass_prepared_free(preparedInsert);
    }
    if (preparedInsert_noTTL) {
        cass_prepared_free(preparedInsert_noTTL);
    }
}

/* ########################################################################## */

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the CaliEvtDataStoreImpl class.
 */
void CaliEvtDataStore::insert(SensorId* sid, uint64_t ts, const std::string& event, int64_t ttl) {
    impl->insert(sid, ts, event, ttl);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the CaliEvtDataStoreImpl class.
 */
void CaliEvtDataStore::insert(CaliEvtData& data, int64_t ttl) {
    impl->insert(data, ttl);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the CaliEvtDataStoreImpl class.
 */
void CaliEvtDataStore::insertBatch(std::list<CaliEvtData>& datas, int64_t ttl) {
    impl->insertBatch(datas, ttl);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the CaliEvtDataStoreImpl class.
 */
void CaliEvtDataStore::setTTL(uint64_t ttl) {
    impl->setTTL(ttl);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the CaliEvtDataStoreImpl class.
 */
void CaliEvtDataStore::setDebugLog(bool dl) {
    impl->setDebugLog(dl);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the CaliEvtDataStoreImpl class.
 */
void CaliEvtDataStore::query(std::list<CaliEvtData>& result, SensorId& sid, TimeStamp& start, TimeStamp& end) {
    impl->query(result, sid, start, end);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the CaliEvtDataStoreImpl class.
 */
void CaliEvtDataStore::fuzzyQuery(std::list<CaliEvtData>& result, SensorId& sid, TimeStamp& ts, uint64_t tol_ns) {
    impl->fuzzyQuery(result, sid, ts, tol_ns);
}

/**
 * @details
 * Instead of doing the actual work, this function simply forwards to the
 * corresponding function of the CaliEvtDataStoreImpl class.
 */
void CaliEvtDataStore::queryCB(CaliEvtDataStore::QueryCECbFunc cbFunc, void* userData, SensorId& sid, TimeStamp& start, TimeStamp& end) {
    impl->queryCB(cbFunc, userData, sid, start, end);
}

/**
 * @details
 * This constructor allocates the implementation class which
 * holds the actual implementation of the class functionality.
 */
CaliEvtDataStore::CaliEvtDataStore(Connection* conn) {
    impl = new CaliEvtDataStoreImpl(conn);
}

/**
 * @details
 * The CaliEvtDataStore desctructor deallocates the
 * CaliEvtDataStoreImpl object.
 */
CaliEvtDataStore::~CaliEvtDataStore() {
    /* Clean up... */
    if (impl) {
        delete impl;
    }
}
