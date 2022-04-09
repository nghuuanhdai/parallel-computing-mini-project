//================================================================================
// Name        : calievtdatastore.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for inserting and querying Caliper event data.
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
 * @file calievtdatastore.h
 * @brief This file contains parts of the public API for the libdcdb library.
 * It contains the class definition of the CaliEvtDataStore class, that handles
 * database operations for Caliper Event data.
 *
 * @ingroup libdcdb
 */

#ifndef DCDB_CALIEVTDATASTORE_H
#define DCDB_CALIEVTDATASTORE_H

#include <cstdint>
#include <list>
#include <string>

#include "cassandra.h"
#include "connection.h"
#include "sensorid.h"
#include "timestamp.h"

namespace DCDB {

  /* Forward-declaration of the implementation-internal classes */
  class CaliEvtDataStoreImpl;

  /**
   * @brief This struct is a container for the information DCDB keeps about
   * a Caliper Event.
   */
  struct CaliEvtData {
    SensorId eventId;   /**< We abuse the SensorId to identify events that
                             occurred on the same CPU. */
    TimeStamp timeStamp;/**< Time when the event occurred. */
    std::string event;  /**< String representation of the event that occurred. */
  };

  /**
   * @brief %CaliEvtDataStore is the class of the libdcdb library
   * to write and read Caliper Event data. The SensorId class is abused as
   * identifier for Caliper Events.
   */
  class CaliEvtDataStore {
    private:
      CaliEvtDataStoreImpl* impl;

    public:
      /**
       * @brief This function inserts a single event into the database.
       *
       * @param sid      A SensorId object
       * @param ts       The timestamp of the Caliper Event
       * @param event    String representation of the encountered event
       * @param ttl      Time to live (in seconds) for the inserted reading
       */
      void insert(SensorId* sid, uint64_t ts, const std::string& event, int64_t ttl=-1);

      /**
       * @brief This function inserts a single event into the database.
       *
       * @param data    A CaliEvtData object
       * @param ttl      Time to live (in seconds) for the inserted reading
       */
      void insert(CaliEvtData& data, int64_t ttl=-1);

      /**
       * @brief This function inserts a batch of Caliper Events into
       *        the database.
       * @param readings  A list of CaliEvtData objects
       * @param ttl       Time to live (in seconds) for the inserted readings
       */
      void insertBatch(std::list<CaliEvtData>& datas, int64_t ttl=-1);

      /**
       * @brief Set the TTL for newly inserted event data.
       * @param ttl      The TTL for the event data in seconds.
       */
      void setTTL(uint64_t ttl);

      /**
       * @brief Enables or disables logging of Cassandra insert errors
       * @param dl        true to enable logging, false otherwise
       */
      void setDebugLog(bool dl);

      /**
       * @brief This function queries Caliper event data in the given time
       *        range.
       * @param result   The list where the results will be stored.
       * @param sid      The SensorId to query.
       * @param start    Start of the time series.
       * @param end      End of the time series.
       */
      void query(std::list<CaliEvtData>& result, SensorId& sid, TimeStamp& start, TimeStamp& end);

      /**
       * @brief This function performs a fuzzy query and returns the
       *        closest Caliper event data to the input timestamp.
       * @param result   The list where the results will be stored.
       * @param sid      The SensorId to query.
       * @param ts       The target timestamp.
       * @param tol_ns   Tolerance of the fuzzy query in nanoseconds.
       */
      void fuzzyQuery(std::list<CaliEvtData>& result, SensorId& sid, TimeStamp& ts, uint64_t tol_ns=10000000000);

      typedef void (*QueryCECbFunc)(CaliEvtData& data, void* userData);
      /**
       * @brief This function queries Caliper event data in the given time
       *        range and calls a function for each reading.
       * @param cbFunc   A function to called for each reading.
       * @param userData Pointer to user data that will be returned in the callback.
       * @param sid      The SensorId to query.
       * @param start    Start of the time series.
       * @param end      End of the time series.
       */
      void queryCB(QueryCECbFunc cbFunc, void* userData, SensorId& sid, TimeStamp& start, TimeStamp& end);

      /**
       * @brief A shortcut constructor for a CaliEvtDataStore object that allows
       *        accessing the data store through a connection that is already
       *        established.
       * @param conn     The Connection object of an established connection to
       *                 Cassandra.
       */
      CaliEvtDataStore(Connection* conn);

      /**
       * @brief The standard destructor for a CaliEvtDatStore object.
       */
      virtual ~CaliEvtDataStore();
  };

} /* End of namespace DCDB */

#endif /* DCDB_CALIEVTDATASTORE_H */
