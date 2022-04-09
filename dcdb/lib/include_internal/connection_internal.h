//================================================================================
// Name        : connection_internal.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Internal Interface for libdcdb connections
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
 *        Connection which are provided by the
 *        ConnectionImpl class.
 */

#include <string>
#include <cstdint>

#include "dcdb/connection.h"

#ifndef DCDB_CONNECTION_INTERNAL_H
#define DCDB_CONNECTION_INTERNAL_H

namespace DCDB {

class ConnectionImpl
{
protected:
  std::string hostname_;        /**< The hostname of a DB front-end node. */
  uint16_t    port_;            /**< The port of the DB front-end node. */
  std::string username_;        /**< The username for connecting to the DB front-end. */
  std::string password_;        /**< The password for connecting to the DB front-end. */
  bool        connected;        /**< Indicates whether a connection has been established. */

  CassCluster* cluster;         /**< The Cassandra Cluster object (contains hostname, port, etc) */
  CassSession* session;         /**< The session object through which we communicate with C* once the connection is established */
  const CassSchemaMeta* schema; /**< The schema object containing the current database schema information */
  std::string  currentKeyspace; /**< The name of the active keyspace */

  uint32_t numThreadsIo;
  uint32_t queueSizeIo;
  uint32_t coreConnPerHost;
  uint32_t maxConnPerHost;
  uint32_t maxConcRequests;

  /**
   * @brief This function validates a name to ensure that
   *        it only consists of alphanumeric characters.
   * @param name   A string to check
   * @return       True if the name is alphanumeric, false otherwise
   */
  bool validateName(std::string name);

  /**
   * @brief Fetch the list of Key Spaces from the Cassandra server.
   */
  void updateSchema();

  /**
   * @brief Check if a keyspace with a given name exists.
   * @param name   The name of the keyspace to look for.
   * @return True if the keyspace exists, false otherwise.
   */
  bool existsKeyspace(std::string name);

  /**
   * @brief Create a new keyspace.
   * @param name               The name of the new keyspace
   * @param replicationFactor  The Cassandra replication factor
   */
  void createKeyspace(std::string name, int replicationFactor);

  /**
   * @brief Specify a keyspace to use in this connection.
   * @param name   The name of the keyspace to select.
   */
  void selectKeyspace(std::string name);

  /**
   * @brief Get the active keyspace's name
   * @return string object containing the keyspace name
   */
  std::string getActiveKeyspace();

  /**
   * @brief Check if a column family with a given name
   *        exists in the currently selected keyspace.
   * @param name   The name of the column family to look for.
   * @return True if the column family exists in the current keyspace, false otherwise.
   */
  bool existsColumnFamily(std::string name);

  /**
   * @brief Create a new column family in the currently
   *        selected keyspace.
   * @param name        The name of the keyspace
   * @param fields      A comma separated list of field names and types
   * @param primaryKey  A primary key definition (one or more fields)
   * @param options     A Cassandra WITH statement for keyspace generation
   */
  void createColumnFamily(std::string name, std::string fields, std::string primaryKey, std::string options = "");

  /**
   * @brief Create a new materialized view in the
   *        selected keyspace.
   *        
   * @param name        The name of the materialized view
   * @param select      A comma-separated list of columns from the original table
   * @param fromTable   Name of the original table from which the view must be created
   * @param where       List of selection clauses (usually of the IS NOT NULL type)
   * @param primaryKey  A primary key definition (one or more fields)
   * @param options     A Cassandra WITH statement for keyspace generation
   */
  void createMaterializedView(std::string name, std::string select, std::string fromTable, 
          std::string where, std::string primaryKey, std::string options);


public:
  /**
   * @brief The implementation function of Connection::printError().
   */
  void printError(CassFuture* future);

  /**
   * @brief Sets the number of IO threads that are spawned
   * @param n         The number of IO threads
   */
  void setNumThreadsIo(uint32_t n);

  /**
   * @brief Sets the maximum size of the outbound requests queue
   * @param s         The maximum size of the requests queue
   */
  void setQueueSizeIo(uint32_t s);

  /**
   * @brief Returns the maximum size of the outbound requests queue.
   * @return The maximum queue size.
   */
  uint32_t getQueueSizeIo();

  /**
   * @brief Sets implementation-specific parameters
   *
   *                  Input is an array of size three of unsigned integers:
   *                      - p[0] contains the number of connections that are associated by default to each IO thread
   *                      - p[1] contains the max number of connections that can be spawned by an IO thread
   *                      - p[2] specifies the number of concurrent requests that triggers a new connection creation
   *
   * @param p         An array of unsigned integers containing three Cassandra-related
   *                  configuration parameters
   */
  void setBackendParams(uint32_t* p);

  /**
   * @brief The implementation function of Connection::setHostname().
   */
  void setHostname(std::string hostname);

  /**
   * @brief The implementation function of Connection::getHostname().
   */
  std::string getHostname();

  /**
   * @brief The implementation function of Connection::setPort().
   */
  void setPort(uint16_t port);

  /**
   * @brief The implementation function of Connection::getPort().
   */
  uint16_t getPort();

  /**
   * @brief The implementation function of Connection::setHostname().
   */
  void setUsername(std::string username);
  
  /**
   * @brief The implementation function of Connection::getHostname().
   */
  std::string getUsername();

  /**
   * @brief The implementation function of Connection::setHostname().
   */
  void setPassword(std::string password);
  
  /**
   * @brief The implementation function of Connection::getHostname().
   */
  std::string getPassword();

  /**
   * @brief The implementation function of Connection::connect().
   */
  bool connect();

  /**
   * @brief The implementation function of Connection::disconnect().
   */
  void disconnect();

  /**
   * @brief The implementation function of Connection::getSessionHandle().
   */
  CassSession* getSessionHandle();

  /**
   * @brief The implementation function of Connection::executeSimpleQuery().
   */
  CassError executeSimpleQuery(std::string query);

  /**
   * @brief The implementation function of Connection::initSchema().
   */
  bool initSchema();

  ConnectionImpl();
  virtual ~ConnectionImpl();
};

} /* End of namespace DCDB */

#endif /* DCDB_CONNECTION_INTERNAL_H */
