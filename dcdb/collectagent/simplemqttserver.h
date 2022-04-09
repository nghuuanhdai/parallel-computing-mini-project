//================================================================================
// Name        : simplemqttserver.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header for the implementation of a simple MQTT message server
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2019 Leibniz Supercomputing Centre
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//================================================================================

/**
 * @defgroup mqttserver Simple MQTT server
 * @ingroup ca
 *
 * @brief Simplified custom MQTT message server.
 */

#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>

#include <exception>
#include <stdexcept>

#include <boost/ptr_container/ptr_list.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include "logging.h"

#ifndef SIMPLEMQTTSERVER_H_
#define SIMPLEMQTTSERVER_H_

//#define SimpleMQTTVerbose 100

/**
 * Define the MQTT connection queue length
 *
 * @ingroup mqttserver
 */
#ifndef SimpleMQTTConnectionsQueueLength
#define SimpleMQTTConnectionsQueueLength 4
#endif

/*
 * Define the maximum backlog size for listen().
 *
 * @ingroup mqttserver
 */
#ifndef SimpleMQTTBacklog
#define SimpleMQTTMaxBacklog 100
#endif

/*
 * Define the standard wait time for poll() calls.
 *
 * @ingroup mqttserver
 */
#ifndef SimpleMQTTPollTimeout
#define SimpleMQTTPollTimeout 100
#endif

/*
 * Define the standard size for the buffer used in read() calls.
 *
 * @ingroup mqttserver
 */
#ifndef SimpleMQTTReadBufferSize
#define SimpleMQTTReadBufferSize 1024
#endif

/*
 * Enable verbose output of the MQTT server.
 *
 */
//#define SimpleMQTTVerbose

#include "simplemqttservermessage.h"
typedef int (*SimpleMQTTMessageCallback)(SimpleMQTTMessage*);
#include "simplemqttserverthread.h"

/**
 * @brief %SimpleMQTTServer Class Definition
 *
 * @details Usage:
 * @code
 * // Create a simple MQTT server instance listening on localhost, port 1883 (default):
 * SimpleMQTTServer s();
 *
 * // Create a simple MQTT server instance listening on 127.0.0.1 (IPv4 only), port 1234:
 * SimpleMQTTServer ss("127.0.0.1", 1234);
 *
 * s.start();    // Start server
 * s.stop();     // Stop server
 * @endcode
 *
 * @ingroup mqttserver
 */
class SimpleMQTTServer
{
protected:
  uint64_t _maxThreads;
  uint64_t _maxConnPerThread;
  std::string listenAddress;
  std::string listenPort;
  boost::ptr_vector<int> listenSockets;
  boost::ptr_list<SimpleMQTTServerAcceptThread> acceptThreads;
  SimpleMQTTMessageCallback messageCallback;

  logger_t lg;

  void init(std::string addr, std::string port);
  void initSockets(void);

public:
  void start();
  void stop();
  void setMessageCallback(SimpleMQTTMessageCallback callback);
  std::map<std::string, hostInfo_t> collectLastSeen();

  SimpleMQTTServer();
  SimpleMQTTServer(std::string addr, std::string port, uint64_t maxThreads=128, uint64_t maxConnPerThread=16);
  virtual
  ~SimpleMQTTServer();
};

#endif /* SIMPLEMQTTSERVER_H_ */
