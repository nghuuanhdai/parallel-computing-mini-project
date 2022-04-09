//================================================================================
// Name        : calievtdatastore_internal.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Internal interface for inserting and querying Caliper Event data.
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
 *        Caliper Event Data Store which are provided by the
 *        CaliEvtDataStoreImpl class.
 */

#ifndef DCDB_CALIEVTDATASTORE_INTERNAL_H
#define DCDB_CALIEVTDATASTORE_INTERNAL_H

#include "dcdb/calievtdatastore.h"
#include "dcdb/connection.h"

namespace DCDB {

//TODO Deduplicate (duplicate of sensordatastore_internal.h)
//Definition of callback function for Cassandra inserts
//prints debug output on insert failure
void CaliEvtDataStoreImpl_on_result(CassFuture_* future, void* data) {
    /* This result will now return immediately */
    static CassError rcPrev = CASS_OK;
    static uint32_t ctr = 0;
    CassError rc = cass_future_error_code(future);
    if(rc != CASS_OK) {
        if(rc != rcPrev) {
            const char* error_msg;
            size_t error_msg_len;
            cass_future_error_message(future, &error_msg, &error_msg_len);
            std::string msg(error_msg, error_msg_len);
            std::cout << "Cassandra Backend Error (CaliEvt): " << cass_error_desc(rc) << ": " << msg << std::endl;
            ctr = 0;
            rcPrev = rc;
        } else if(++ctr%10000 == 0)
            std::cout << "Cassandra Backend Error (CaliEvt): " << cass_error_desc(rc) << " (" << ctr << " more)" << std::endl;
    }
}

  /**
   * @brief The CaliEvtDataStoreImpl class contains all protected
   *        functions belonging to CaliEvtDataStore which are
   *        hidden from the user of the libdcdb library.
   */
  class CaliEvtDataStoreImpl {
  protected:
    Connection* connection; /**< The Connection object that does the low-level stuff for us. */
    CassSession* session;   /**< The CassSession object given by the connection. */
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
    /* See calievtdatastore.h for documentation */
    void insert(SensorId* sid, uint64_t ts, const std::string& event, int64_t ttl=-1);
    void insert(CaliEvtData& data, int64_t ttl=-1);
    void insertBatch(std::list<CaliEvtData>& datas, int64_t ttl=-1);
    void setTTL(uint64_t ttl);
    void setDebugLog(bool dl);
    void query(std::list<CaliEvtData>& result, SensorId& sid, TimeStamp& start, TimeStamp& end);
    void fuzzyQuery(std::list<CaliEvtData>& result, SensorId& sid, TimeStamp& ts, uint64_t tol_ns=10000000000);
    void queryCB(CaliEvtDataStore::QueryCECbFunc cbFunc, void* userData, SensorId& sid, TimeStamp& start, TimeStamp& end);

    CaliEvtDataStoreImpl(Connection* conn);
    virtual ~CaliEvtDataStoreImpl();
  };

} /* End of namespace DCDB */

#endif /* DCDB_CALIEVTDATASTORE_INTERNAL_H */
