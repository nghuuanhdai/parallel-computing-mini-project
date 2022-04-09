//================================================================================
// Name        : simplemqttserverthread.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Headers of classes for running MQTT server threads
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

#ifndef SIMPLEMQTTSERVERTHREAD_H_
#define SIMPLEMQTTSERVERTHREAD_H_

#include "logging.h"
#include "timestamp.h"
#include <vector>

typedef struct {
    uint64_t lastSeen;
    std::string address;
    std::string clientId;
} hostInfo_t;

/**
 * @brief Simple MQTT server thread.
 *
 * @ingroup mqttserver
 */
class SimpleMQTTServerThread
{
protected:
  pthread_t t;
  bool terminate;

  boost::mutex cleanupMtx;

  logger_t lg;

  static void* launch(void* thisPtr);
  static void yield();
  virtual void run() = 0;

public:
  SimpleMQTTServerThread();
  virtual
  ~SimpleMQTTServerThread();
};

/**
 * @brief Simple MQTT server message thread.
 *
 * @ingroup mqttserver
 */
class SimpleMQTTServerMessageThread : SimpleMQTTServerThread
{
protected:
  uint64_t _maxConnPerThread;
  unsigned numConnections;
  struct pollfd* fds;
  SimpleMQTTMessage** msg;
  std::pair<int, std::string> *fdQueue;
  hostInfo_t *lastSeen;
  int fdQueueReadPos;
  int fdQueueWritePos;
  SimpleMQTTMessageCallback messageCallback;

  void run();
  int sendAck(int connectionId);

public:
  int queueConnection(int newsock, std::string addr);
  void assignConnections();
  void releaseConnection(int connectionId);
  bool hasCapacity();
  void setMessageCallback(SimpleMQTTMessageCallback callback);
  hostInfo_t *getLastSeen() { return lastSeen; }

  SimpleMQTTServerMessageThread(SimpleMQTTMessageCallback callback, uint64_t maxConnPerThread=16);
  virtual
  ~SimpleMQTTServerMessageThread();
};

/**
 * @brief Simple MQTT server accept thread.
 *
 * @ingroup mqttserver
 */
class SimpleMQTTServerAcceptThread : SimpleMQTTServerThread
{
protected:
  uint64_t _maxThreads;
  uint64_t _maxConnPerThread;
  int socket;
  unsigned int threadCtr;
  std::vector<SimpleMQTTServerMessageThread*> messageThreads;
  SimpleMQTTMessageCallback messageCallback;

  void run();

public:
  void setMessageCallback(SimpleMQTTMessageCallback callback);
  std::vector<hostInfo_t> collectLastSeen();

  SimpleMQTTServerAcceptThread(int listenSock, SimpleMQTTMessageCallback callback, uint64_t maxThreads=128, uint64_t maxConnPerThread=16);
  virtual~SimpleMQTTServerAcceptThread();
};

#endif /* SIMPLEMQTTSERVERTHREAD_H_ */
