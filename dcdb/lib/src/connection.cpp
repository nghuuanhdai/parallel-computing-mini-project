//================================================================================
// Name        : connection.cpp
// Author      : Axel Auweter, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for libdcdb connections
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

#include <algorithm>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "cassandra.h"

#include "dcdb/connection.h"
#include "connection_internal.h"
#include "dcdbglobals.h"

using namespace DCDB;

/*
 * Definition of the public Connection functions.
 * All calls forward calls of all Connection functions
 * to their counterparts in the ConnectionImpl class.
 */
void Connection::printError(CassFuture* future) {
  impl->printError(future);
}

void Connection::setNumThreadsIo(uint32_t n) {
    impl->setNumThreadsIo(n);
}

void Connection::setQueueSizeIo(uint32_t s)  {
    impl->setQueueSizeIo(s);
}

uint32_t Connection::getQueueSizeIo()  {
    return impl->getQueueSizeIo();
}

void Connection::setBackendParams(uint32_t* p) {
    impl->setBackendParams(p);
}

void Connection::setHostname(std::string hostname) {
  impl->setHostname(hostname);
}

std::string Connection::getHostname() {
  return impl->getHostname();
}

void Connection::setPort(uint16_t port) {
  impl->setPort(port);
}

uint16_t Connection::getPort() {
  return impl->getPort();
}

void Connection::setUsername(std::string username) {
  impl->setUsername(username);
}

std::string Connection::getUsername() {
  return impl->getUsername();
}

void Connection::setPassword(std::string password) {
  impl->setHostname(password);
}

std::string Connection::getPassword() {
  return impl->getPassword();
}

bool Connection::connect() {
  return impl->connect();
}

void Connection::disconnect() {
  impl->disconnect();
}

bool Connection::initSchema() {
  return impl->initSchema();
}

CassSession* Connection::getSessionHandle() {
  return impl->getSessionHandle();
}

/*
 * Connection constructors & destructor.
 * Upon object creation, allocate a corresponding impl class
 * and free it on object deallocation.
 */
Connection::Connection() {
  impl = new ConnectionImpl();
}

Connection::Connection(std::string hostname, uint16_t port) {
  impl = new ConnectionImpl();
  impl->setHostname(hostname);
  impl->setPort(port);
}

Connection::Connection(std::string hostname, uint16_t port, std::string username, std::string password) {
  impl = new ConnectionImpl();
  impl->setHostname(hostname);
  impl->setPort(port);
  impl->setUsername(username);
  impl->setPassword(password);
}

Connection::~Connection() {
  delete impl;
}


/*
 * Definitions of the protected ConnectionImpl class functions
 */

/**
 * @details
 * The validation for alphanumeric is implemented by using C++ STL's
 * find_if function. Pretty nice piece of C++!
 */
bool ConnectionImpl::validateName(std::string name)
{
    /*
     * Make sure name only consists of alphabetical characters or underscores (super ugly)...
     */
    class {
    public:
        static bool check(char c) {
            return !isalpha(c) && !(c == '_');
        }
    } isNotAlpha;

    if (find_if(name.begin(), name.end(), isNotAlpha.check) == name.end())
        return true;
    else
        return false;
}

/**
 * @details
 * This function updates the local copy of the data
 * store schema information. It is called whenever
 * an existsKeyspace query is performed to take note
 * of recently added keyspaces.
 */
void ConnectionImpl::updateSchema()
{
    /* Free the memory of the currently known schema definition. */
    if (schema)
      cass_schema_meta_free(schema);

    /* Get the new schema */
    schema = cass_session_get_schema_meta(session);
}

/**
 * @details
 * This function tries to retrieve schema information
 * about a given keyspace to determine if the keyspace
 * exists
 */
bool ConnectionImpl::existsKeyspace(std::string name)
{
    updateSchema();

    const CassKeyspaceMeta* keyspaceMeta = cass_schema_meta_keyspace_by_name(schema, name.c_str());
    if (keyspaceMeta != NULL)
      return true;
    else
      return false;
}

/**
 * @details
 * This function creates a new keyspace with a given name and
 * replication factor.
 */
void ConnectionImpl::createKeyspace(std::string name, int replicationFactor)
{
    std::string query;
    std::string rFact = boost::lexical_cast<std::string>(replicationFactor);

    if(validateName(name)) {
        query = "CREATE KEYSPACE " + name + " WITH replication = { 'class': 'SimpleStrategy', 'replication_factor': '" + rFact + "' };";
        if(executeSimpleQuery(query) != CASS_OK) {
            std::cerr << "Failed to create keyspace " << name << "!" << std::endl;
        }
    }
}

/**
 * @details
 * This function sets the current keyspace by issuing a CQL USE
 * statement and setting the currentKeySpace class member to point
 * to the newly selected keyspace.
 */
void ConnectionImpl::selectKeyspace(std::string name)
{
    std::string query;

    if (validateName(name) && existsKeyspace(name)) {
        query = "USE " + name + ";";
        if (executeSimpleQuery(query) != CASS_OK) {
            std::cerr << "Error selecting keyspace " << name << "!" << std::endl;
        }
        currentKeyspace = name;
    }
}

/**
 * @details
 * If no keyspace was selected the returned string is empty.
 */
std::string ConnectionImpl::getActiveKeyspace()
{
    return currentKeyspace;
}

/**
 * @details
 * This function tries to query meta information for a given
 * column family (table) to check whether the column family
 * exists.
 */
bool ConnectionImpl::existsColumnFamily(std::string name)
{
    updateSchema();

    const CassKeyspaceMeta* keyspaceMeta = cass_schema_meta_keyspace_by_name(schema, currentKeyspace.c_str());
    if (keyspaceMeta == NULL) {
        /* It is a bit misleading to return false if the keyspace doesn't even exist... */
        return false;
    }

    const CassTableMeta* tableMeta = cass_keyspace_meta_table_by_name(keyspaceMeta, name.c_str());
    if (tableMeta == NULL) {
        return false;
    }

    return true;
}

/**
 * @details
 * This functions assembles the parameters into a CQL CREATE
 * TABLE query and submits the query to the server.
 */
void ConnectionImpl::createColumnFamily(std::string name, std::string fields, std::string primaryKey, std::string options)
{
    std::stringstream query;

    /* FIXME: Secure this and use proper types for fields, primaryKey, and options. */
    query << "CREATE TABLE " << name
        << " ( " << fields << ", PRIMARY KEY (" << primaryKey << "))";

    if (options != "") {
      query << " WITH " << options;
    }

    query << ";";

    executeSimpleQuery(query.str());
}

/**
 * @details
 * This functions assembles the parameters into a CQL CREATE
 * TABLE query and submits the query to the server.
 */
void ConnectionImpl::createMaterializedView(std::string name, std::string select, std::string fromTable, 
        std::string where, std::string primaryKey, std::string options)
{
    std::stringstream query;

    query << "CREATE MATERIALIZED VIEW " << name << " AS"
          << " SELECT " << select << " FROM " << fromTable
          << " WHERE " << where << " PRIMARY KEY (" << primaryKey << ")";

    if (options != "") {
        query << " WITH " << options;
    }

    query << ";";

    executeSimpleQuery(query.str());
}

/*
 * Definitions of the public ConnectionImpl class functions
 */

void ConnectionImpl::printError(CassFuture* future)
{
  const char *message;
  size_t      length;
  cass_future_error_message(future, &message, &length);

  std::cerr << "Cassandra Backend Error: " << std::string(message, length) << std::endl;
}

void ConnectionImpl::setNumThreadsIo(uint32_t n) {
    if(!connected)
        numThreadsIo = n;
}

void ConnectionImpl::setQueueSizeIo(uint32_t s) {
    if(!connected)
        queueSizeIo = s;
}

uint32_t ConnectionImpl::getQueueSizeIo() {
    return queueSizeIo;
}

void ConnectionImpl::setBackendParams(uint32_t* p) {
    if(!connected) {
        coreConnPerHost = p[0];
    }
}

void ConnectionImpl::setHostname(std::string hostname) {
  if (!connected)
    hostname_ = hostname;
}

std::string ConnectionImpl::getHostname() {
  return hostname_;
}

void ConnectionImpl::setPort(uint16_t port) {
  if (!connected)
    port_ = port;
}

uint16_t ConnectionImpl::getPort() {
  return port_;
}

void ConnectionImpl::setUsername(std::string username) {
  if (!connected)
    username_ = username;
}

std::string ConnectionImpl::getUsername() {
  return username_;
}

void ConnectionImpl::setPassword(std::string password) {
  if (!connected)
    password_ = password;
}

std::string ConnectionImpl::getPassword() {
  return password_;
}

/**
 * @details
 * This function connects to the selected Cassandra
 * front end node using the CQL API.
 */
bool ConnectionImpl::connect() {
  if (connected)
    return false;

  /* Set hostname and port */
  cass_cluster_set_contact_points(cluster, hostname_.c_str());
  cass_cluster_set_port(cluster, port_);
  if (username_.size() && password_.size()) {
    cass_cluster_set_credentials(cluster, username_.c_str(), password_.c_str());
  }

  cass_cluster_set_num_threads_io(cluster, numThreadsIo);
  cass_cluster_set_queue_size_io(cluster, queueSizeIo);
  cass_cluster_set_core_connections_per_host(cluster, coreConnPerHost);
  cass_cluster_set_request_timeout(cluster, 60000);

  /* Force protcol version to 4 */
  cass_cluster_set_protocol_version(cluster, 4);

  /* Connect to the server */
  CassError rc = CASS_OK;
  CassFuture* future = cass_session_connect(session, cluster);

  /* Wait for successful connection */
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
      printError(future);
      cass_future_free(future);
      return false;
  }

  cass_future_free(future);
  connected = true;
  return true;
}

void ConnectionImpl::disconnect() {
  if (!connected)
    return;

  CassFuture* future;
  future = cass_session_close(session);
  cass_future_wait(future);
  cass_future_free(future);
  connected = false;
}

CassSession* ConnectionImpl::getSessionHandle() {
  return session;
}

CassError ConnectionImpl::executeSimpleQuery(std::string query)
{
  CassError rc = CASS_OK;
  CassFuture* future = NULL;
  CassStatement* statement = cass_statement_new(query.c_str(), 0);

  future = cass_session_execute(session, statement);
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
      printError(future);
  }

  cass_future_free(future);
  cass_statement_free(statement);

  return rc;
}

bool ConnectionImpl::initSchema() {

  if (!connected)
    return false;

  /* Keyspace and column family for published sensors */
  /* FIXME: We should have a good way to determine the number of replicas here */
  if (!existsKeyspace(CONFIG_KEYSPACE_NAME)) {
      std::cout << "Creating Keyspace " << CONFIG_KEYSPACE_NAME << "...\n";
      createKeyspace(CONFIG_KEYSPACE_NAME, 1);
  }

  selectKeyspace(CONFIG_KEYSPACE_NAME);

  if (!(getActiveKeyspace().compare(CONFIG_KEYSPACE_NAME) == 0)) {
      std::cout << "Cannot select keyspace " << CONFIG_KEYSPACE_NAME << "\n";
      return false;
  }

  if (!existsColumnFamily(CF_PUBLISHEDSENSORS)) {
      std::cout << "Creating Column Family " CF_PUBLISHEDSENSORS "...\n";
      createColumnFamily(CF_PUBLISHEDSENSORS,
          "name varchar, "              /* Public name */
          "virtual boolean, "           /* Whether it is a published physical sensor or a virtual sensor */
          "pattern varchar, "           /* In case of physical sensors: pattern for MQTT topics that this sensor matches against */
          "scaling_factor double, "     /* Unused */
          "unit varchar, "              /* Unit of the sensor (e.g. W for Watts) */
          "sensor_mask bigint, "        /* Bit mask that specifies sensor properties. Currently defined ones are:
                                           Integrable: indicates whether the sensor is integrable over time;
                                           Monotonic : indicates whether the collected sensor data is monotonic. */
          "operations set<varchar>, "   /* Operations for the sensor (e.g., avg, stdev,...). */
          "expression varchar, "        /* For virtual sensors: arithmetic expression to derive the virtual sensor's value */
          "vsensorid varchar, "         /* For virtual sensors: Unique sensorId for the sensor in the virtualsensors table */
          "tzero bigint, "              /* For virtual sensors: time of the first reading */
          "interval bigint,"             /* Interval in nanoseconds at which this virtual sensor provides readings */
          "ttl bigint",                 /* Time to live in nanoseconds for readings of this sensor */

          "name",                       /* Make the "name" column the primary key */
          "CACHING = {'keys' : 'ALL'} "); /* Enable compact storage and maximum caching */
  }

  /* Creating simple key-value table for misc metadata */
  if (!existsColumnFamily(CF_MONITORINGMETADATA)) {
        std::cout << "Creating Column Family " CF_MONITORINGMETADATA "...\n";
        createColumnFamily(CF_MONITORINGMETADATA,
                           "name varchar, "                /* Property name */
                           "value varchar",                 /* Property value */
                           "name",                         /* Make the "name" column the primary key */
                           "CACHING = {'keys' : 'ALL'} "); /* Enable compact storage and maximum caching */
    }

  /* Keyspace and column family for raw and virtual sensor data */
  if (!existsKeyspace(KEYSPACE_NAME)) {
      std::cout << "Creating Keyspace " << KEYSPACE_NAME << "...\n";
      createKeyspace(KEYSPACE_NAME, 1);
  }

  selectKeyspace(KEYSPACE_NAME);

  if (!(getActiveKeyspace().compare(KEYSPACE_NAME) == 0)) {
      std::cout << "Cannot select keyspace " << KEYSPACE_NAME << "\n";
      return false;
  }

  if (!existsColumnFamily(CF_SENSORDATA)) {
      std::cout << "Creating Column Family " CF_SENSORDATA "...\n";
      createColumnFamily(CF_SENSORDATA,
          "sid varchar, ws smallint, ts bigint, value bigint",
          "sid, ws, ts",
	  "CLUSTERING ORDER BY (ws DESC, ts DESC) AND "
          "COMPACT STORAGE AND gc_grace_seconds = " SENSORDATA_GC_GRACE_SECONDS
          " AND compaction = " SENSORDATA_COMPACTION);
  }

  if (!existsColumnFamily(CF_VIRTUALSENSORS)) {
      std::cout << "Creating Column Family " CF_VIRTUALSENSORS "...\n";
      createColumnFamily(CF_VIRTUALSENSORS,
          "sid varchar, ws smallint, ts bigint, value bigint",
          "sid, ws, ts",
          "CLUSTERING ORDER BY (ws DESC, ts DESC) AND "
          "COMPACT STORAGE AND gc_grace_seconds = " SENSORDATA_GC_GRACE_SECONDS
          " AND compaction = " SENSORDATA_COMPACTION);
  }

  /* Keyspace and column family for Caliper Event data */
  if (!existsKeyspace(CED_KEYSPACE_NAME)) {
      std::cout << "Creating Keyspace " << CED_KEYSPACE_NAME << "...\n";
      createKeyspace(CED_KEYSPACE_NAME, 1);
  }

  selectKeyspace(CED_KEYSPACE_NAME);

  if (!(getActiveKeyspace().compare(CED_KEYSPACE_NAME) == 0)) {
      std::cout << "Cannot select keyspace " << CED_KEYSPACE_NAME << "\n";
      return false;
  }

  if (!existsColumnFamily(CF_CALIEVTDATA)) {
      std::cout << "Creating Column Family " CF_CALIEVTDATA "...\n";
      createColumnFamily(CF_CALIEVTDATA,
          "sid varchar, "   /* Public name/SensorID */
          "ws smallint, "   /* Weekstamp to ensure that the maximum number of
                               2^32 columns per key is not exceeded */
          "ts bigint, "     /* Timestamp of a Caliper Event */
          "value varchar",  /* String representation of the Event */

          "sid, ws, ts",   /* Make the "sid", "ws", and "ts" columns together
                               the primary key */
          "COMPACT STORAGE" /* Enable compact storage */
          );                /* No further options required */
  }

  /* Keyspace and column family for job data */
  if (!existsKeyspace(JD_KEYSPACE_NAME)) {
      std::cout << "Creating Keyspace " << JD_KEYSPACE_NAME << "...\n";
      createKeyspace(JD_KEYSPACE_NAME, 1);
  }

  selectKeyspace(JD_KEYSPACE_NAME);

  if (!(getActiveKeyspace().compare(JD_KEYSPACE_NAME) == 0)) {
      std::cout << "Cannot select keyspace " << JD_KEYSPACE_NAME << "\n";
      return false;
  }

  if (!existsColumnFamily(CF_JOBDATA)) {
      std::cout << "Creating Column Family " CF_JOBDATA "...\n";
      createColumnFamily(CF_JOBDATA,
          "domain varchar, "    /* Job Domain */
          "jid varchar, "       /* Job Id */
          "uid varchar, "       /* User Id */
          "start_ts bigint, "   /* Start timestamp of the job */
          "end_ts bigint, "     /* End timestamp of the job */
          "nodes set<varchar>", /* Set of nodes used by the job */

          "domain, jid, start_ts",      /* Make start_ts + jid columns the primary key */

          "CLUSTERING ORDER BY (jid DESC, start_ts DESC)" /* Setting the sorting criteria - recent jobs first */ 
      );
      
      std::cout << "Creating Materialized View " CF_JOBDATAVIEW "...\n";
      createMaterializedView(CF_JOBDATAVIEW,
          "domain,jid,start_ts,end_ts,uid,nodes", /* Picking all job data fields */
          "jobdata",                              /* From the jobdata table */
          "domain IS NOT NULL AND end_ts IS NOT NULL AND start_ts IS NOT NULL and jid IS NOT NULL", /* Primary key components are not null */
          "domain,end_ts,start_ts,jid",           /* We use end_ts in the primary key to improve selectivity in range queries */
          "CLUSTERING ORDER BY (end_ts DESC, start_ts DESC)" /* Using descending order to get more recent jobs */
      );
  }

  return true;
}

ConnectionImpl::ConnectionImpl() {
  cluster = nullptr;
  session = nullptr;
  schema = nullptr;

  numThreadsIo = 1;
  queueSizeIo = 4096;
  coreConnPerHost = 1;

  hostname_ = "localhost";
  port_ = 9042;
  connected = false;

  /* Set loglevel to errors since our token() queries will result in unnecessary warnings by the driver */
  cass_log_set_level(CASS_LOG_ERROR);

  /* Create a new cluster object */
  if (!cluster)
    cluster = cass_cluster_new();

  if (!session)
    session = cass_session_new();
}

ConnectionImpl::~ConnectionImpl() {
  /* Clean up... */
  disconnect();
  if (schema)
    cass_schema_meta_free(schema);
  if (session)
    cass_session_free(session);
  if (cluster)
    cass_cluster_free(cluster);

}


