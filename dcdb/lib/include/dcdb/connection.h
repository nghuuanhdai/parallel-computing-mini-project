//================================================================================
// Name        : connection.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ Application Programming Interface for libdcdb connections
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
 * @file connection.h
 * @brief This file contains parts of the public API for the
 * libdcdb library.
 * It contains the class definition of the Connection class,
 * that handles connections to the data store and schema
 * initialization.
 *
 * @ingroup libdcdb
 */

#include <string>
#include <cstdint>

#include "cassandra.h"

#ifndef DCDB_CONNECTION_H
#define DCDB_CONNECTION_H

namespace DCDB {

/* Forward-declaration of the implementation-internal classes */
class ConnectionImpl;

class Connection
{
private:
  ConnectionImpl* impl;     /**< The object which implements the core functionality of this class */

public:
  /**
   * @brief This function prints Cassandra CQL-specific
   *        error messages from a CassFuture object.
   * @param future  The future object which caused the error.
   *
   * @details
   * Once a request to Cassandra completes, we get a CassFuture
   * object which holds the return to our request. In case the
   * request caused an error, this function prints a human-readable
   * error message.
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
   * @param p         A vector of unsigned integers containing a specific number of
   *                  implementation-specific parameters
   */
  void setBackendParams(uint32_t* p);

  /**
   * @brief Set the hostname for the connection.
   * @param hostname  Hostname of a Cassandra front end node.
   */
  void setHostname(std::string hostname);

  /**
   * @brief Return the current hostname of the connection.
   * @return The hostname of a Cassandra front end node to which this object will connect.
   */
  std::string getHostname();

  /**
   * @brief Set the port for the connection.
   * @param port  Port on which the Cassandra front end node accepts CQL native protocol clients.
   */
  void setPort(uint16_t port);

  /**
   * @brief Return the current port of the connection.
   * @return The port on which the Cassandra front end node accepts CQL native protocol clients.
   */
  uint16_t getPort();

  /**
   * @brief Set the username for the connection.
   * @param username  Username for connecting to the Cassandra front end node.
   */
  void setUsername(std::string username);
  
  /**
   * @brief Return the current username of the connection.
   * @return The username for connecting to the Cassandra front end node.
   */
  std::string getUsername();
  
  /**
   * @brief Set the password for the connection.
   * @param password  The password for connecting to the Cassandra front end node.
   */
  void setPassword(std::string password);
  
  /**
   * @brief Return the current password of the connection.
   * @return The password for connecting to the Cassandra front end node.
   */
  std::string getPassword();
  
  /**
   * @brief Establish a connection to the Cassandra database.
   * @return True if the connection was successfully established, false otherwise.
   */
  bool connect();

  /**
   * @brief Disconnect an existing connection to the Cassandra database.
   */
  void disconnect();

  /**
   * @brief Get the session handle of the connection.
   * @return Pointer to this connection's session object.
   */
  CassSession* getSessionHandle();

  /**
   * @brief This function executes a simple raw CQL query.
   * @param query  The CQL query string.
   * @return       Returns a CassError type to indicate success or failure
   */
  CassError executeSimpleQuery(std::string query);

  /**
   * @brief Initialize the database schema for DCDB.
   * @return True if the schema was successfully initialized.
   */
  bool initSchema();

  /**
   * @brief Standard constructor for Connections.
   *
   * If not set explicitly, hostname and port will default to localhost and 9042.
   */
  Connection();

  /**
   * @brief Construct a Connection to the specific host and port without authentication
   */
  Connection(std::string hostname, uint16_t port);

  /**
   * @brief Construct a Connection to the specific host and port and authenticate with given username and password.
   */
  Connection(std::string hostname, uint16_t port, std::string username, std::string password);
  
  /**
   * @brief Standard destructor for Connections.
   */
  virtual ~Connection();
};

} /* End of namespace DCDB */

#endif /* DCDB_CONNECTION_H */
