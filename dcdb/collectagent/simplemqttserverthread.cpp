//================================================================================
// Name        : simplemqttserverthread.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Implementation of classes for running MQTT server threads
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

#include "simplemqttserver.h"
#include "abrt.h"
#include "messaging.h"

using namespace std;
using namespace boost::system;

#ifdef SimpleMQTTVerbose
static boost::mutex coutMtx;
#endif

static boost::mutex launchMtx;

SimpleMQTTServerThread::SimpleMQTTServerThread()
{
  /*
   * Start the thread at the 'launch' function. The 'launch'
   * function will take care of calling the child-specific
   * 'run' function which contains the thread's main loop.
   *
   * Special care must be taken not to call the run function
   * before the child has gone through its constructor. Thus,
   * we lock a mutex (launchMtx) that will cause the 'launch'
   * function to  wait until the mutex is released by the
   * child's constructor.
   *
   * A second mutex (cleanupMtx) is held to allow others to
   * wait for a thread to return from its child-specific
   * 'run' function. This is necessary for cases where the
   * child class has local objects that can only be destroyed
   * after the 'run' function has finished (to avoid access
   * to these objects in the 'run'function after destroying
   * them). In such cases, the child class destructor can try
   * to obtain the mutex in the destructor.
   */
  launchMtx.lock();
  cleanupMtx.lock();

  terminate = false;
  if (pthread_create(&t, NULL, launch, this) != 0) {
      LOG(error) << "Error creating new MQTT server thread.";
      abrt(EXIT_FAILURE, INTERR);
  }

#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Started Thread (" << hex << t << ") of class (" << this << ")...\n";
  coutMtx.unlock();
#endif
}

SimpleMQTTServerThread::~SimpleMQTTServerThread()
{
  /*
   * Terminate the thread and join it to ensure proper
   * thread termination before the class is destroyed.
   */
  terminate = true;
  if (pthread_join(t, NULL) != 0) {
      LOG(error) << "Error joining thread.";
      abrt(EXIT_FAILURE, INTERR);
  }

#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Terminated Thread (" << t << ") of class (" << this << ")...\n";
  coutMtx.unlock();
#endif
}

void* SimpleMQTTServerThread::launch(void *selfPtr)
{
#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Running launcher for class (" << selfPtr << ")...\n";
  coutMtx.unlock();
#endif

  /*
   * The following lines guard the run function to be called
   * before the constructor of the child class has finished.
   * The lock is released at the end of the child's constructor.
   */
  launchMtx.lock();
  launchMtx.unlock();

  /*
   * Call the child's run function...
   */
  ((SimpleMQTTServerThread*)selfPtr)->run();

  /*
   * The thread will terminate. Unlock the cleanupMtx...
   */
  ((SimpleMQTTServerThread*)selfPtr)->cleanupMtx.unlock();

  return NULL;
}

void SimpleMQTTServerThread::yield()
{
  /*
   * Yield this thread's execution to give way to other threads.
   * This is being used in the SimpleMQTTServer to allow the
   * accept thread acquiring the fdsMtx lock. Otherwise, in
   * particular on single-CPU systems (or when the acceptThread
   * and the messageThread share a core), waiting connections
   * may have to wait a long time before the lock will be given
   * to the acceptThread.
   */
  boost::this_thread::yield();
}

void SimpleMQTTServerAcceptThread::run()
{
#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Running SimpleMQTTServerAcceptThread for socket " << socket << "...\n";
  coutMtx.unlock();
#endif

  int newsock = -1;
  struct pollfd fd;

  while (!terminate) {
      /*
       * Wait for something to happen on the socket...
       */
      fd.fd = socket;
      fd.events = POLLIN | POLLPRI;
      fd.revents = 0;
      if(poll(&fd, 1, SimpleMQTTPollTimeout) > 0 && (fd.revents & (POLLIN | POLLPRI))) {
          /*
           * Take the next incoming connection.
           */
#ifdef SimpleMQTTVerbose
          coutMtx.lock();
          cout << "Thread (" << this << ") waiting in accept()...\n";
          coutMtx.unlock();
#endif
          struct sockaddr_in addr;
          socklen_t socklen = sizeof(addr);
          newsock = accept(socket, (struct sockaddr*)&addr, &socklen);
          if (newsock != -1) {
              int opt = fcntl(newsock, F_GETFL, 0);
              if (opt == -1 || fcntl(newsock, F_SETFL, opt | O_NONBLOCK)==-1) {
#ifdef SimpleMQTTVerbose
                coutMtx.lock();
                cout << "Setting socket to non-blocking in thread (" << this << ") failed for socket " << newsock << "...\n";
                coutMtx.unlock();
#endif
                close(newsock);
              } else {
#ifdef SimpleMQTTVerbose
                    coutMtx.lock();
                    cout << "Successfully set socket " << newsock << " non-blocking in thread (" << this << ")...\n";
                    coutMtx.unlock();
#endif
                  if (messageThreads.size() < this->_maxThreads) {
                      // Spawning new threads, if not exceeding maximum thread pool size
                      messageThreads.push_back(new SimpleMQTTServerMessageThread(messageCallback, this->_maxConnPerThread));
                      messageThreads.back()->queueConnection(newsock, std::string(inet_ntoa(addr.sin_addr)) + ":" + std::to_string(addr.sin_port));
                  }
                  else {
                      // If thread pool is full, we cycle through it to find any available threads to connect to
                      unsigned int ctr=0;
                      do {
                          // Rotating the thread queue to ensure round-robin scheduling
                          threadCtr = (threadCtr + 1) % messageThreads.size();
                      } while(ctr++ < messageThreads.size() && messageThreads[threadCtr]->queueConnection(newsock, inet_ntoa(addr.sin_addr)));

                      if(ctr > messageThreads.size()) {
                          LOG(warning) << "Socket " << socket << " cannot accept more connections.";
                          // FIXME: There must be nicer ways to handle such situations...
                          close(newsock);
                      }
#ifdef SimpleMQTTVerbose
                      else {
                          coutMtx.lock();
                          cout << "Found a message thread with capacity: " << messageThreads.front() << "\n";
                          coutMtx.unlock();
                      }
#endif
                  }
              }
            }
#ifdef SimpleMQTTVerbose
            else {
              coutMtx.lock();
              cout << "Accept() in thread (" << this << ") returned -1...\n";
              coutMtx.unlock();
            }
#endif
      }
  }
}

void SimpleMQTTServerAcceptThread::setMessageCallback(SimpleMQTTMessageCallback callback)
{
  /*
   * Set the function that will be called for each received
   * MQTT message and propagate to all message threads.
   */
  messageCallback = callback;
  for (std::vector<SimpleMQTTServerMessageThread*>::iterator i = messageThreads.begin(); i != messageThreads.end(); i++) {
      (*i)->setMessageCallback(callback);
  }
}

std::vector<hostInfo_t> SimpleMQTTServerAcceptThread::collectLastSeen() {
    std::vector<hostInfo_t> hosts;
    for(const auto &m : messageThreads) {
        hostInfo_t *lastSeenVec = m->getLastSeen();
        if(lastSeenVec) {
            for(size_t idx=0; idx<this->_maxConnPerThread; idx++) {
                if(lastSeenVec[idx].lastSeen != 0) {
                    hosts.push_back(lastSeenVec[idx]);
                }
            }
        }
    }
    return hosts;
}

SimpleMQTTServerAcceptThread::SimpleMQTTServerAcceptThread(int listenSock, SimpleMQTTMessageCallback callback, uint64_t maxThreads, uint64_t maxConnPerThread)
{
  /*
   * Assign socket and message callback.
   */
  this->_maxThreads = maxThreads;
  this->_maxConnPerThread = maxConnPerThread;
  socket = listenSock;
  messageCallback = callback;
  threadCtr = 0;

  /*
   * Release the lock to indicate that the constructor has
   * finished. This causes the launcher to call the run function.
   */
  launchMtx.unlock();
}

SimpleMQTTServerAcceptThread::~SimpleMQTTServerAcceptThread() {
    terminate = true;

    cleanupMtx.lock();
    cleanupMtx.unlock();

    //De-allocating and destroying running message threads
    for(auto t : messageThreads)
        delete t;
    messageThreads.clear();
}

int SimpleMQTTServerMessageThread::sendAck(int connectionId) {
  uint8_t buf[] = {0, 0, 0, 0};
  switch(msg[connectionId]->getType()) {
    case MQTT_CONNECT: {
      buf[0] = MQTT_CONNACK << 4;
      buf[1] = 2;
      break;
    }
    case MQTT_PUBLISH: {
      if (msg[connectionId]->getQoS() == 0) {
        return 1;
      } else {
        if (msg[connectionId]->getQoS() == 1) {
          buf[0] = MQTT_PUBACK << 4;
        } else {
          buf[0] = MQTT_PUBREC << 4;
        }
        buf[1] = 2;
        uint16_t msgId = htons(msg[connectionId]->getMsgId());
        buf[2] = ((uint8_t*)&msgId)[0];
        buf[3] = ((uint8_t*)&msgId)[1];
      }
      break;
    }
    case MQTT_PUBREL: {
      buf[0] = MQTT_PUBCOMP << 4;
      buf[1] = 2;
      uint16_t msgId = htons(msg[connectionId]->getMsgId());
      buf[2] = ((uint8_t*)&msgId)[0];
      buf[3] = ((uint8_t*)&msgId)[1];
      break;
    }
    case MQTT_PINGREQ: {
      buf[0] = MQTT_PINGRESP << 4;
      buf[1] = 0;
      break;
    }
    default:
      return 1;
  }
  return !(write(fds[connectionId].fd, buf, buf[1]+2) == buf[1]+2);
}

void SimpleMQTTServerMessageThread::run()
{
#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Running SimpleMQTTServerMessageThread (" << this << ")...\n";
  coutMtx.unlock();
#endif

  int numfds = -1;
  char inbuf[SimpleMQTTReadBufferSize];

  while (!terminate) {
      /*
       * Check for pending connections
       */
      assignConnections();

      /*
       * Check for activity on our sockets...
       */
      numfds = poll(fds, this->_maxConnPerThread, SimpleMQTTPollTimeout);

      if (numfds == -1)
          throw new boost::system::system_error(errno, boost::system::system_category(), "Error in poll().");

      /*
       * Apparently, there is work to do...
       */
      if (numfds > 0) {
          for (unsigned connectionId=0; connectionId<this->_maxConnPerThread; connectionId++) {
              if (fds[connectionId].fd != -1) {
#ifdef SimpleMQTTVerbose
                  coutMtx.lock();
                  cout << "fd(" << fds[connectionId].fd << ") revents: " << hex << fds[connectionId].revents << dec << "\n";
                  coutMtx.unlock();
#endif
                  /* Remote side hung up */
                  if (fds[connectionId].revents & POLLHUP) {
                      releaseConnection(connectionId);
                  }

                  if (fds[connectionId].revents & POLLIN) {
                      char* readPtr;
                      ssize_t rbytes, lbytes, bytes;

                      rbytes = read(fds[connectionId].fd, inbuf, SimpleMQTTReadBufferSize);

                      readPtr = inbuf;
                      lbytes = rbytes;

                      /*
                       * If read() returns 0, the connection was closed on the
                       * remote side. In this case, release it from our list.
                       */
                      if (rbytes == 0) {
                          releaseConnection(connectionId);
                      } else {
                          lastSeen[connectionId].lastSeen = getTimestamp();
                      }

                      while (lbytes > 0) {
                          /*
                           * Allocate new message if there is none.
                           */
                          if (!msg[connectionId]) {
                              msg[connectionId] = new SimpleMQTTMessage();
                              if (!msg[connectionId]) {
                                  LOG(warning) << "Out of memory! Discarding network input!";
                                  continue;
                              }
#ifdef SimpleMQTTVerbose
                              coutMtx.lock();
                              cout << "Allocated new SimpleMQTTMessage (" << msg[connectionId] << ")...\n";
                              coutMtx.unlock();
#endif
                          }

                          /*
                           * Append received data to message.
                           */
                          bytes = msg[connectionId]->appendRawData(readPtr, lbytes);
                          readPtr += bytes;
                          lbytes -= bytes;

                          /*
                           * Check if message is complete.
                           */
                          if (msg[connectionId]->complete()) {
                              /*
                               * Forward message upstream!
                               * If there is a callback function, it is responsible
                               * for freeing the message using delete. Otherwise, we
                               * have to take care of this, here.
                               */
#ifdef SimpleMQTTVerbose
                              coutMtx.lock();
                              cout << "Completed receiving SimpleMQTTMessage (" << msg[connectionId] << ")...\n";
                              coutMtx.unlock();
#endif
                            switch(msg[connectionId]->getType()) {
                              case MQTT_CONNECT: {
				if (msg[connectionId]->getTopic().size() > 0) {
				  // clientId has been pre-populated with the IP address
				  lastSeen[connectionId].clientId = msg[connectionId]->getTopic();
				}
                                sendAck(connectionId);
                                break;
                              }
                              case MQTT_PUBLISH: {
                                if ((messageCallback(msg[connectionId]) == 0) && (msg[connectionId]->getQoS() > 0)) {
                                  sendAck(connectionId);
                                }
                                break;
                              }
                              case MQTT_PUBREL: {
                                sendAck(connectionId);
                                break;
                              }
                              case MQTT_PINGREQ: {
                                sendAck(connectionId);
                                break;
                              }
                              case MQTT_DISCONNECT: {
                                releaseConnection(connectionId);
                                break;
                              }
                              default: {
                                if (messageCallback) {
                                  messageCallback(msg[connectionId]);
                                }
                                else {
                                  LOG(trace) << "Nothing to do..";
                                }
                                break;
                              }
                            }
                            if(msg[connectionId])
                                msg[connectionId]->clear();
                          }
                      }
                  }
              }
          }
      }
  }
}

int SimpleMQTTServerMessageThread::queueConnection(int newsock, std::string addr) {
  if (numConnections >= this->_maxConnPerThread) {
#ifdef SimpleMQTTVerbose
      coutMtx.lock();
      cout << "Maximum number of connections reached, rejecting new connection  (" << this << ", " << newsock << ", " << numConnections << ")...\n";
      coutMtx.unlock();
#endif
    return 2;
  }

  int nextWritePos = (fdQueueWritePos + 1) % SimpleMQTTConnectionsQueueLength;
  if (nextWritePos == fdQueueReadPos) {
#ifdef SimpleMQTTVerbose
    coutMtx.lock();
    cout << "Queue is full, rejecting new connection  (" << this << ", " << newsock << ", " << fdQueueWritePos << ")...\n";
    coutMtx.unlock();
#endif
    return 1;
  }

#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Queued new connection  (" << this << ", " << newsock << ", " << fdQueueWritePos << ")...\n";
  coutMtx.unlock();
#endif
  fdQueue[fdQueueWritePos].first = newsock;
  fdQueue[fdQueueWritePos].second = addr;
  fdQueueWritePos = nextWritePos;
  return 0;
}

void SimpleMQTTServerMessageThread::assignConnections()
{
  /*
   * Find the first free slot in fds and assign connection.
   */
  while (fdQueueWritePos != fdQueueReadPos) {
    /* There is no free fd slot at the moment, defer.. */
    if (numConnections >= this->_maxConnPerThread) {
      return;
    }

    for (unsigned i=0; i<this->_maxConnPerThread; i++) {
      if (fds[i].fd == -1) {
        fds[i].events = POLLIN | POLLPRI | POLLHUP;
        fds[i].revents = 0;
        fds[i].fd = fdQueue[fdQueueReadPos].first;
        lastSeen[i].lastSeen = getTimestamp();
        lastSeen[i].address = fdQueue[fdQueueReadPos].second;
	lastSeen[i].clientId = lastSeen[i].address;

        numConnections++;
        fdQueueReadPos = (fdQueueReadPos + 1) % SimpleMQTTConnectionsQueueLength;

#ifdef SimpleMQTTVerbose
        coutMtx.lock();
        cout << "Assigned connection  (" << this << ", " << i << ", " << fds[i].fd << ")...\n";
        coutMtx.unlock();
#endif
        break;
      }
    }
  }
}

void SimpleMQTTServerMessageThread::releaseConnection(int connectionId)
{
  /*
   * Close the connection an clean up.
   */
  shutdown(fds[connectionId].fd, SHUT_RDWR);
  close(fds[connectionId].fd);
  fds[connectionId].fd = -1;
  fds[connectionId].events = 0;
  fds[connectionId].revents = 0;
  lastSeen[connectionId].lastSeen = 0;
  lastSeen[connectionId].address = "";
  lastSeen[connectionId].clientId = "";
  
  if (msg[connectionId]) {
      delete msg[connectionId];
      msg[connectionId] = NULL;
  }
  numConnections--;

#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Released connection  (" << this << ", " << connectionId << ")...\n";
  coutMtx.unlock();
#endif
}

void SimpleMQTTServerMessageThread::setMessageCallback(SimpleMQTTMessageCallback callback)
{
  messageCallback = callback;
}

SimpleMQTTServerMessageThread::SimpleMQTTServerMessageThread(SimpleMQTTMessageCallback callback, uint64_t maxConnPerThread)
{
#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Created new MessageThread (" << (void*)this << ")\n";
  coutMtx.unlock();
#endif

  this->_maxConnPerThread = maxConnPerThread;
  this->fds = new struct pollfd[this->_maxConnPerThread];
  this->msg = new SimpleMQTTMessage*[this->_maxConnPerThread];

  /*
   * Clear the fds array. Warning: This will only work when the
   * size of the fds array is determined at compile time.
   */
  memset(fds, -1, sizeof(fds[0]) * this->_maxConnPerThread);

  /*
   * Clear the msg array. Warning: This will only work when the
   * size of the msg array is determined at compile time.
   */
  memset(msg, 0, sizeof(msg[0]) * this->_maxConnPerThread);

  /*
   * Initialize the number of active connections to 0 and set
   * the messageCallback function.
   */
  numConnections = 0;
  messageCallback = callback;
  
  // Initializing the list of address-value pairs for connected hosts
  this->lastSeen = new hostInfo_t[this->_maxConnPerThread];
  for(size_t idx=0; idx<this->_maxConnPerThread; idx++) {
      this->lastSeen[idx].lastSeen = 0;
      this->lastSeen[idx].address = "";
      this->lastSeen[idx].clientId = "";
  }

  /*
   * Initialize the fd queue for new connections
   */
  fdQueue = new(std::pair<int, std::string>[SimpleMQTTConnectionsQueueLength]);
  fdQueueReadPos = 0;
  fdQueueWritePos = 0;

  /*
   * Release the lock to indicate that the constructor has
   * finished. This causes the launcher to call the run function.
   */
  launchMtx.unlock();
}

SimpleMQTTServerMessageThread::~SimpleMQTTServerMessageThread()
{
  /*
   * When this destructor is called externally, we are likely
   * holding the fdsMtx mutex and waiting in poll().
   * Terminate the thread from here and acquire cleanupMtx to
   * ensure that the thread's 'run' function has terminated in
   * order to avoid fdsMtx to be accessed after destruction.
   */
  terminate = true;

  cleanupMtx.lock();
  cleanupMtx.unlock();

  delete[] fdQueue;
  for(unsigned i=0; i<_maxConnPerThread; i++)
      delete msg[i];
  delete[] msg;
  delete[] fds;
  delete[] lastSeen;
}

