//================================================================================
// Name        : c_api.h
// Author      : Axel Auweter, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C Application Programming Interface for libdcdb
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
 * @file c_api.h
 * @brief This file contains a reduced public API for the
 * libdcdb library using C instead of C++ bindings.
 *
 * @ingroup libdcdb
 */

#include <stdint.h>
#include <time.h>

#include "dcdb/jobdatastore.h"
#include "dcdb/connection.h"

#ifndef DCDB_C_API_H
#define DCDB_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enum type for representing the outcome of a DCDB C API operation.
 */
typedef enum {
  DCDB_C_OK,                    /**< Everything went fine. */
  DCDB_C_CONNERR,               /**< The connection to the database could not be made. */
  DCDB_C_SENSORNOTFOUND,        /**< The requested sensor cannot be found in the database's list of public sensors. */
  DCDB_C_EMPTYSET,              /**< The query into the database resulted in an empty set. */
  DCDB_C_NOSENSOR,              /**< The caller did not specify a sensor to be queried. */
  DCDB_C_BADPARAMS,             /**< The provided function parameters are malformed or incomplete */
  DCDB_C_UNKNOWN                /**< An unknown error occurred. */
} DCDB_C_RESULT;

/**
 * Type for passing various options to C API operations (bitmask style).
 *
 * So far, only the DCDB_C_LOCALTIME option exists.
 */
typedef uint32_t DCDB_C_OPTIONS;
#define DCDB_C_LOCALTIME 0x1    /**< Treat time stamps passed to the query as being in local time instead of UTC */

/*****************************************************************************/
/* Following are C-API functions to insert job information.                  */
/* Intended to be called from python-script.                                 */
/*                                                                           */
/* Expected order:                                                           */
/* 1. Create a database-connection via connectToDatabase().                  */
/* 2. Create a JobDataStore object with the previous connection.             */
/* 3. Insert all jobs via insertJobIntoDatabase and the previous JobDataStore*/
/* 4. If finished, destroy JobDataStore and Connection object with their     */
/*    corresponding destruct/disconnect methods.                             */
/*****************************************************************************/

/**
 * @brief Construct a new DCDB::Connection object and connect it to database.
 *
 * @param hostname  Hostname of database node
 * @param port      TCP port to use for connecting to the database node
 *                  (for Cassandra, this is usually 9042).
 * @return  Pointer to the new Connection object, or NULL if an error occurred.
 *
 * @details
 * Constructs a new Connection object via < b>new< \b>. Then tries to connect
 * it to the database. If the connection attempt fails, the connection is
 * destroyed and NULL is returned instead.
 */
DCDB::Connection* connectToDatabase(const char* hostname, uint16_t port);

/**
 * @brief Disconnect and destroy a DCDB::Connection object.
 *
 * @param conn  Pointer to the Connection, which shall be destroyed.
 * @return  Always returns DCDB_C_OK.
 *
 * @details
 * First, if the Connection object pointed to by conn is still connected, it is
 * disconnected from the database. Afterwards the object is deconstructed via
 * < b>delete< \b>. This function is guaranteed to always succeed.
 *
 */
DCDB_C_RESULT disconnectFromDatabase(DCDB::Connection* conn);

/**
 * @brief Construct a new DCDB::JobDataStore object.
 *
 * @param conn  Database connection, required to access the database.
 *
 * @return  Pointer to the new JobDataStore object, or NULL if an error
 *          occurred.
 *
 * @details
 * Construct a new JobDataStore object via < b>new< \b>. The
 * given Connection object must already be connected to the database,
 * otherwise later JobDataStore operations will fail.
 */
DCDB::JobDataStore* constructJobDataStore(DCDB::Connection* conn);

/**
 * @brief Insert a starting job into the database.
 *
 * @param jds       Pointer to JobDataStore object, which shall be used to
 *                  insert the job.
 * @param jid       SLURM id of the job.
 * @param uid       SLURM user id of the job owner.
 * @param startTs   Start time of the job (in ns since Unix epoch).
 * @param nodes     String array of node names used by the job.
 * @param nodeSize  Size of the nodes array.
 *
 * @return  DCDB_C_OK if the job was successfully inserted. DCDB_BAD_PARAMS if
 *          the given parameters were illogical. DCDB_C_UNKNOWN otherwise.
 *
 * @details
 * Builds a JobData struct from (jid, uid, startTs, endTs, nodes, nodeSize) and
 * then tries to insert it by calling the corresponding insert function of jds.
 */
DCDB_C_RESULT insertJobStart(DCDB::JobDataStore* jds, DCDB::JobId jid,
                             DCDB::UserId uid, uint64_t startTs,
                             const char ** nodes, unsigned nodeSize);

/**
 * @brief Update the end time of the most recent job with Id jid.
 *
 * @param jds   Pointer to JobDataStore object, which shall be used to insert
 *              the job.
 * @param jid   SLURM id of the job.
 * @param endTs End time of the job (in ns since Unix epoch).
 *
 * @return  DCDB_C_OK if the job was successfully updated. DCDB_BAD_PARAMS if
 *          no job with the given JobId exists. DCDB_C_UNKNOWN otherwise.
 */
DCDB_C_RESULT updateJobEnd(DCDB::JobDataStore* jds, DCDB::JobId jid,
                           uint64_t endTs);

/**
 * @brief For Debugging. Print the jobdata or an appropriate error message.
 *
 * @param jds   Pointer to JobDataStore object, which to query for the JobId.
 * @param jid   SLURM id of the job to be printed.
 *
 * @return  DCDB_C_OK.
 *
 * @details
 * For fast testing if a job was inserted correctly, this method allows to
 * query the datastore for the job and print its data. If the job could not
 * be found or another error was encountered, an appropriate error message is
 * printed.
 */
DCDB_C_RESULT printJob(DCDB::JobDataStore* jds, DCDB::JobId jid);

/**
 * @brief Destroy a DCDB::JobDataStore object.
 *
 * @param jds  Pointer to the JobDataStore, which shall be destroyed.
 * @return  Always returns DCDB_C_OK.
 *
 * @details
 * The object is deconstructed via < b>delete< \b>. This function is guaranteed
 * to always succeed. NOTE: Does not delete the connection object, which was
 * given to the JobDataStore at construction time. Use disconnectFromDatabase()
 * to destroy the Connection object.
 *
 */
DCDB_C_RESULT destructJobDataStore(DCDB::JobDataStore* jds);

#ifdef __cplusplus
}
#endif

#endif /* DCDB_C_API_H */
