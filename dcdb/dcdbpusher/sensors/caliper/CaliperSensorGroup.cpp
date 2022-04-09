//================================================================================
// Name        : CaliperSensorGroup.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Caliper sensor group class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
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

#include "CaliperSensorGroup.h"

#include <algorithm>
#include <chrono>
#include <errno.h>
#include <stdio.h>
#include <thread>
#include <utility>

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include "timestamp.h"

CaliperSensorGroup::CaliperSensorGroup(const std::string &name)
    : SensorGroupTemplate(name),
      _maxSensorNum(500),
      _timeout(15),
      _socket(-1),
      _connection(-1),
      _globalMqttPrefix("") {
	_mqttPart = "/caliper";
	_buf = static_cast<char *>(malloc(MSGQ_SIZE));
	_lock.clear();
}

CaliperSensorGroup::CaliperSensorGroup(const CaliperSensorGroup &other)
    : SensorGroupTemplate(other),
      _maxSensorNum(other._maxSensorNum),
      _timeout(other._timeout),
      _socket(-1),
      _connection(-1),
      _globalMqttPrefix(other._globalMqttPrefix) {
	_buf = static_cast<char *>(malloc(MSGQ_SIZE));
	_lock.clear();

	//SensorGroupTemplate already copy constructed _sensor
	for (auto &s : _sensors) {
		_sensorIndex.insert(std::make_pair(s->getName(), s));
	}
}

CaliperSensorGroup::~CaliperSensorGroup() {
	_sensorIndex.clear();
	free(_buf);

	for (auto &it : _processes) {
		munmap(it.shm, SHM_SIZE);
		close(it.shmFile);
	}
}

CaliperSensorGroup &CaliperSensorGroup::operator=(const CaliperSensorGroup &other) {
	SensorGroupTemplate::operator=(other);
	_maxSensorNum = other._maxSensorNum;
	_timeout = other._timeout;
	_socket = -1;
	_connection = -1;
	_globalMqttPrefix = other._globalMqttPrefix;
	_lock.clear();

	//SensorGroupTemplate already copied _sensor
	for (auto &s : _sensors) {
		_sensorIndex.insert(std::make_pair(s->getName(), s));
	}

	return *this;
}

bool CaliperSensorGroup::execOnStart() {
	_socket = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);

	if (_socket == -1) {
		LOG(error) << _groupName << ": Failed to open socket: " << strerror(errno);
		return false;
	}

	sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));

	addr.sun_family = AF_UNIX;
	snprintf(&addr.sun_path[1], 91, SOCK_NAME);

	if (bind(_socket, (struct sockaddr *)&addr, sizeof(addr))) {
		LOG(error) << _groupName << ": Failed to bind socket: " << strerror(errno);
		close(_socket);
		_socket = -1;
		return false;
	}

	if (listen(_socket, 1)) {
		LOG(error) << _groupName << ": Can not listen on socket: " << strerror(errno);
		close(_socket);
		_socket = -1;
		return false;
	}

	return true;
}

void CaliperSensorGroup::execOnStop() {
	if (_connection != -1) {
		close(_connection);
		_connection = -1;
	}
	if (_socket != -1) {
		close(_socket);
		_socket = -1;
	}
}

void CaliperSensorGroup::read() {
	//check if there are pending connections from applications who want to send
	//us their PIDs
	while (true) {
		_connection = accept(_socket, NULL, NULL);
		if (_connection == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				//no pending connections
				break;
			}
			LOG(error) << _groupName << ": Accept failed: " << strerror(errno);
			continue;
		}

		//got new incoming connection
		ssize_t      nrec;
		const size_t bufSize = 64;
		char         buf[bufSize];

		//try to receive message three times. Should leave the sender with enough time to send
		for (unsigned short i = 0; i < 3; ++i) {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
			//receive message with PID
			nrec = recv(_connection, (void *)buf, bufSize, MSG_DONTWAIT);
			if (nrec != -1 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
				break;
			}
		}

		close(_connection);
		_connection = -1;

		if (nrec <= 0) {
			LOG(error) << _groupName << ": Connection accepted but got no message";
			continue;
		}

		std::string  pidStr(buf);
		caliInstance ci;
		ci.shm = nullptr;
		ci.shmFile = -1;
		ci.shmFailCnt = 0;

		LOG(debug) << _groupName << ": PID " << pidStr << " connected.";

		//set up shared memory for newly connected process
		ci.shmFile = shm_open((STR_PREFIX + pidStr).c_str(), O_RDWR, 0666);
		if (ci.shmFile == -1) {
			LOG(error) << _groupName << ": Failed to open shmFile from PID " << pidStr;
			continue;
		}
		ci.shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ci.shmFile, 0);
		if (ci.shm == (void *)-1) {
			LOG(error) << _groupName << ": Failed to mmap shmFile for PID " << pidStr;
			close(ci.shmFile);
			continue;
		}

		if (_processes.size() == 0) {
			//Currently no previous processes are connected. We use this chance to
			//clear all previous sensors (we cannot clear them process-wise as
			//there is no possibility to tell which sensors belongs to which process).
			//If their values have not been pushed by now they are lost.
			_sensorIndex.clear();
			acquireSensors();
			_sensors.clear();
			_baseSensors.clear();
			releaseSensors();
		}

		_processes.push_back(std::move(ci));
	}

	//clean up sensors if required
	//If they still contain unpushed values, they will get lost
	if (_sensorIndex.size() > _maxSensorNum) {
		_sensorIndex.clear();
		acquireSensors();
		_sensors.clear();
		_baseSensors.clear();
		releaseSensors();
	}

	bool doCleanUp = false;
	for (auto &it : _processes) {
		//get snapshot data from message queue in shared memory
		size_t r_index;
		size_t w_index;

		sem_t *r_sem;
		sem_t *w_sem;
		sem_t *u_sem;

		char *msg_queue;

		r_sem = reinterpret_cast<sem_t *>(static_cast<char *>(it.shm) + 2 * sizeof(size_t));
		w_sem = r_sem + 1;
		msg_queue = reinterpret_cast<char *>(w_sem + 1);

		//TODO atomic load/stores instead of semaphore locking?
		if (sem_wait(r_sem)) {
			continue;
		}
		r_index = *(reinterpret_cast<size_t *>(static_cast<char *>(it.shm)));
		sem_post(r_sem);

		if (sem_wait(w_sem)) {
			continue;
		}
		w_index = *(reinterpret_cast<size_t *>(static_cast<char *>(it.shm) + sizeof(size_t)));
		sem_post(w_sem);

		//are new elements there at all?
		if (r_index == w_index) {
			++it.shmFailCnt;
			if (it.shmFailCnt > _timeout) {
				LOG(debug) << _groupName << ": Removing process (Timeout)";
				//"Timeout". We assume that the application terminated
				sem_destroy(r_sem);
				sem_destroy(w_sem);

				munmap(it.shm, SHM_SIZE);
				it.shm = nullptr;
				close(it.shmFile);
				it.shmFile = -1;
				doCleanUp = true;
			}
			continue;
		}

		it.shmFailCnt = 0;
		size_t bufSize = 0;

		if (r_index < w_index) {
			bufSize = w_index - r_index;
			memcpy(_buf, &msg_queue[r_index + 1], bufSize);
		} else {
			bufSize = MSGQ_SIZE - r_index + w_index;
			size_t sep = MSGQ_SIZE - r_index - 1;
			memcpy(_buf, &msg_queue[r_index + 1], sep);
			memcpy(&_buf[sep], msg_queue, bufSize - sep);
		}

		//update r_index in shm
		if (sem_wait(r_sem)) {
			continue;
		}
		*(reinterpret_cast<size_t *>(static_cast<char *>(it.shm))) = w_index;
		sem_post(r_sem);

		//LOG(debug) << _groupName << "Processing " << bufSize << " bytes";

		std::unordered_map<std::string, std::pair<S_Ptr, reading_t>> cache;
		for (size_t bufIdx = 0; bufIdx < bufSize;) {
			reading_t reading;
			reading.value = 1;
			reading.timestamp = *(reinterpret_cast<uint64_t *>(&_buf[bufIdx]));
			bufIdx += sizeof(uint64_t);

			std::string data(&_buf[bufIdx]);
			bufIdx += data.size() + 1;

			const bool event = (data[0] == 'E' ? true : false);
			data.erase(0, 1);

			// '#' and '+' are not allowed in MQTT topics. Get rid of them here
			std::replace(data.begin(), data.end(), '#', '/');
			std::replace(data.begin(), data.end(), '+', '.');

			std::string cpu = "/" + data.substr(0, data.find_first_of('/'));
			std::string top = data.substr(data.find_first_of('/'));

			//store in sensors
			S_Ptr s;
			auto  it = _sensorIndex.find(data);
			if (it != _sensorIndex.end()) {
				//we encountered this function or event already
				s = it->second;
			} else {
				//unknown function or event --> create a new sensor
				s = std::make_shared<CaliperSensorBase>(data);
				if (event) {
					//Events shall be stored in a separate table. For
					//identification they get a unique topic prefix. Also,
					//data will be encoded in the topic but separated so it can
					//be split again in the CollectAgent
					s->setMqtt("/DCDB_CE/" + _globalMqttPrefix + cpu + _mqttPart + "/:/" + top);
				} else {
					s->setMqtt(_globalMqttPrefix + cpu + _mqttPart + top);
				}
				s->setName(s->getMqtt());
				s->initSensor(_interval);

				acquireSensors();
				_sensors.push_back(s);
				_baseSensors.push_back(s);
				releaseSensors();
				_sensorIndex.insert(std::make_pair(data, s));
			}

			// temporarily store and aggregate value in the cache
			// currently only done for Sample data
			if (!event) {
				if (cache.find(data) == cache.end()) {
					//no cache entry yet
					cache[data] = std::make_pair(s, reading);
				} else {
					//update cache entry. Use timestamp of last aggregated value
					cache[data].second.value += reading.value;
					if (reading.timestamp > cache[data].second.timestamp) {
						cache[data].second.timestamp = reading.timestamp;
					}
				}
			} else {
				//NOTE: if event data shall ever be cached and aggregated, too,
				//CollectAgent's message processing has to be adapted!
				s->storeReading(reading);
#ifdef DEBUG
				LOG(debug) << _groupName << "::" << s->getName() << " (E) raw reading: \"" << reading.value << "\"";
#endif
			}
		}

		//flush cache
		for (auto &it : cache) {
			it.second.first->storeReading(it.second.second);
#ifdef DEBUG
			LOG(debug) << _groupName << "::" << it.second.first->getName() << " (S) raw reading: \"" << it.second.second.value << "\"";
#endif
		}
	}

	if (doCleanUp) {
		//do another full iteration and remove all terminated instances
		auto it = _processes.begin();
		while (it != _processes.end()) {
			if (it->shm == nullptr) {
				it = _processes.erase(it);
			} else {
				++it;
			}
		}
	}
}

void CaliperSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "Timeout:  " << _timeout;
	//nothing special to print
}
