//================================================================================
// Name        : simplemqttserver.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Implementation of a simple MQTT message server
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

using namespace std;

void SimpleMQTTServer::initSockets(void)
{
  struct addrinfo hints;
  struct addrinfo *ainfo_head, *ainfo_cur;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = PF_UNSPEC;

  if (getaddrinfo(listenAddress.c_str(), listenPort.c_str(), &hints, &ainfo_head))
    throw new runtime_error("Error initializing socket.");

  ainfo_cur = ainfo_head;
  for (ainfo_cur = ainfo_head; ainfo_cur; ainfo_cur = ainfo_cur->ai_next) {

#ifdef SimpleMQTTVerbose
      /*
       * Print some details about the current addrinfo.
       */
      char buf[INET_ADDRSTRLEN+1], buf6[INET6_ADDRSTRLEN+1];
      if (ainfo_cur->ai_family == AF_INET) {
          buf[INET_ADDRSTRLEN] = 0;
          inet_ntop(AF_INET, &((struct sockaddr_in *)ainfo_cur->ai_addr)->sin_addr, buf, INET_ADDRSTRLEN);
          cout << "Initializing IPv4 socket:\n"
              << "\tAddress: " << buf << "\n"
              << "\tPort: " << ntohs(((struct sockaddr_in *)ainfo_cur->ai_addr)->sin_port) << "\n";
      }
      else if (ainfo_cur->ai_family == AF_INET6) {
          buf6[INET6_ADDRSTRLEN] = 0;
          inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ainfo_cur->ai_addr)->sin6_addr, buf6, INET6_ADDRSTRLEN);
          cout << "Initializing IPv6 socket...\n"
              << "\tAddress: " << buf6 << "\n"
              << "\tPort: " << ntohs(((struct sockaddr_in6 *)ainfo_cur->ai_addr)->sin6_port) << "\n";
      }
      else {
          cout << "Unclear socket type.\n";
      }
#endif

      /*
       * Open the socket.
       */
      int sock = socket(ainfo_cur->ai_family, ainfo_cur->ai_socktype, ainfo_cur->ai_protocol);
      if (sock == -1) {
#ifdef SimpleMQTTVerbose
          cout << "Error: could not create socket.\n";
#endif
          continue;
      }

      /*
       * Set socket options.
       */
      int sopt = 1;
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sopt, sizeof(sopt));
      sopt = 1;
      setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &sopt, sizeof(sopt));
#if 1
      sopt = fcntl(sock, F_GETFL, 0);
      if (sopt == -1) {
          cout << "Warning: could not get socket options, ignoring socket.\n";
          close(sock);
          continue;
      }
      sopt |= O_NONBLOCK;
      if (fcntl(sock, F_SETFL, sopt) == -1) {
          cout << "Warning: could not set socket options, ignoring socket.\n";
          close(sock);
          continue;
      }
#endif
      /*
       * Bind and listen on socket.
       */
      if (::bind(sock, ainfo_cur->ai_addr, ainfo_cur->ai_addrlen) == -1) {
          LOG(warning) << "Could not bind to socket, ignoring socket.";
          close(sock);
          continue;
      }
      if (listen(sock, SimpleMQTTMaxBacklog) == -1) {
          LOG(warning) << "Could not listen on socket, ignoring socket.";
          close(sock);
          continue;
      }

      listenSockets.push_back(new int(sock));
  }

  freeaddrinfo(ainfo_head);

#ifdef SimpleMQTTVerbose
  cout << "Opened " << listenSockets.size() << " network socket(s) for MQTT connections.\n";
#endif
}

void SimpleMQTTServer::start()
{
  /*
   * Check if at least one socket is open.
   */
  if (listenSockets.size() == 0) {
      throw new invalid_argument("Failed to establish a listen socket with the given configuration.");
  }

  /*
   * Start one accept thread per socket.
   */
  for (unsigned int i=0; i<listenSockets.size(); i++)
    acceptThreads.push_back(new SimpleMQTTServerAcceptThread(listenSockets[i], messageCallback, this->_maxThreads, this->_maxConnPerThread));
}

void SimpleMQTTServer::stop()
{
  /*
   * Terminate all running server threads.
   */
  acceptThreads.clear();
}

void SimpleMQTTServer::setMessageCallback(SimpleMQTTMessageCallback callback)
{
  /*
   * Set the function that will be called for each received
   * MQTT message and propagate to all accept threads.
   */
  messageCallback = callback;
  for (boost::ptr_list<SimpleMQTTServerAcceptThread>::iterator i = acceptThreads.begin(); i != acceptThreads.end(); i++) {
      (*i).setMessageCallback(callback);
  }
}


void SimpleMQTTServer::init(string addr, string port)
{
  /*
   * Assign all class internal variables with sane values and
   * do some simple validation checks.
   */
  if (addr.empty() || addr.length() == 0)
    throw new invalid_argument("The listen address cannot be empty.");
  listenAddress = addr;

  if (port.empty() || (strtol(port.c_str(), NULL, 10) == 0))
    throw new invalid_argument("Network port is not numeric.");
  listenPort = port;

  messageCallback = NULL;

  /*
   * Set up the sockets.
   */
  initSockets();
}

std::map<std::string, hostInfo_t> SimpleMQTTServer::collectLastSeen() {
    std::map<std::string, hostInfo_t> hosts;
    for(auto &t : acceptThreads) {
        std::vector<hostInfo_t> tempHosts = t.collectLastSeen();
	for(auto &h1 : tempHosts) {
	    auto h2 = hosts.find(h1.clientId);
	    if (h2 != hosts.end()) {
		if (h2->second.lastSeen < h1.lastSeen) {
		    h2->second = h1;
		}
	    } else {
		hosts[h1.clientId] = h1;
	    }
	}
    }
    return hosts;
}

SimpleMQTTServer::SimpleMQTTServer()
{
  /*
   * Initialize server with default settings.
   */
  init("localhost", "1883");
}

SimpleMQTTServer::SimpleMQTTServer(std::string addr, std::string port, uint64_t maxThreads, uint64_t maxConnPerThread)
{
  /*
   * Initialize server to listen on specified address and port.
   */
  this->_maxThreads = maxThreads;
  this->_maxConnPerThread = maxConnPerThread;
  init(addr, port);
}

SimpleMQTTServer::~SimpleMQTTServer()
{
  /*
   * Stop the server operation.
   */
  stop();

  /*
   * Close all listen sockets.
   */
  for (unsigned int i=0; i<listenSockets.size(); i++)
    close(listenSockets[i]);

  listenSockets.clear();
}

