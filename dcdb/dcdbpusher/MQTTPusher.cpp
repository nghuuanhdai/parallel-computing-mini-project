//================================================================================
// Name        : MQTTPusher.cpp
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Collects values from the sensors and pushes them to the database.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2017-2019 Leibniz Supercomputing Centre
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

#include "MQTTPusher.h"
#include "timestamp.h"
#include <iostream>
#include <string>
#include <unistd.h>

MQTTPusher::MQTTPusher(int brokerPort, const std::string &brokerHost, const bool autoPublish, int qosLevel,
		       pusherPluginStorage_t &plugins, op_pluginVector_t &oPlugins, int maxNumberOfMessages, unsigned int maxInflightMsgNum, unsigned int maxQueuedMsgNum, unsigned int statisticsInterval, std::string statisticsMqttTopic)
    : _qosLevel(qosLevel),
      _brokerPort(brokerPort),
      _brokerHost(brokerHost),
      _autoPublish(autoPublish),
      _plugins(plugins),
      _operatorPlugins(oPlugins),
      _connected(false),
      _keepRunning(true),
      _msgCap(DISABLED),
      _doHalt(false),
      _halted(false),
      _maxNumberOfMessages(maxNumberOfMessages),
      _maxInflightMsgNum(maxInflightMsgNum),
      _maxQueuedMsgNum(maxQueuedMsgNum),
      _statisticsInterval(statisticsInterval),
      _statisticsMqttTopic(statisticsMqttTopic) {

	//first print some info
	int mosqMajor, mosqMinor, mosqRevision;
	mosquitto_lib_version(&mosqMajor, &mosqMinor, &mosqRevision);
	LOGM(info) << mosqMajor << "." << mosqMinor << "." << mosqRevision;
	char hostname[256];
	if (gethostname(hostname, 255) != 0) {
		LOG(fatal) << "Cannot get hostname";
		exit(EXIT_FAILURE);
	}
	hostname[255] = '\0';
	LOG(info) << "Hostname: " << hostname;
	//enough information

	//init mosquitto-struct
	mosquitto_lib_init();
	_mosq = mosquitto_new(hostname, false, NULL);
	if (!_mosq) {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	mosquitto_threaded_set(_mosq, true);
	mosquitto_max_inflight_messages_set(_mosq, _maxInflightMsgNum);
	mosquitto_max_queued_messages_set(_mosq, _maxQueuedMsgNum);
}

MQTTPusher::~MQTTPusher() {
	if (_connected) {
		mosquitto_disconnect(_mosq);
	}
	mosquitto_destroy(_mosq);
	mosquitto_lib_cleanup();
}

void MQTTPusher::push() {
	int      mosqErr;
	uint64_t idleTime = 0;
	int      connectTimer = 0;

	//connect to broker (if necessary)
	while (_keepRunning && !_connected) {
		if (mosquitto_connect(_mosq, _brokerHost.c_str(), _brokerPort, 1000) != MOSQ_ERR_SUCCESS) {
			if (connectTimer == 0) {
				LOGM(warning) << "Could not connect to MQTT broker " << _brokerHost << ":" << _brokerPort;
			}
			connectTimer++;
			sleep(5);
		} else {
			_connected = true;
			LOGM(info) << "Connection to MQTT broker " << _brokerHost << ":" << _brokerPort << " established!";
			connectTimer = 0;
		}
	}

	//Performing auto-publish if necessary
	sendMappings();

	computeMsgRate();
	//collect sensor-data
	reading_t * reads = new reading_t[1024];
	std::size_t totalCount = 0; //number of messages
	uint64_t msgCtr = 0;
	uint64_t readingCtr = 0;
	uint64_t lastStats = getTimestamp();
	while (_keepRunning || totalCount) {
		if (_doHalt) {
			_halted = true;
			sleep(2);
			continue;
		}
		_halted = false;

		//there was a (unintended) disconnect in the meantime --> reconnect
		if (!_connected) {
			if (connectTimer == 0) {
				LOGM(info) << "Lost connection. Reconnecting...";
			}
			if (mosquitto_reconnect(_mosq) != MOSQ_ERR_SUCCESS) {
//				LOGM(debug) << "Could not reconnect to MQTT broker " << _brokerHost << ":" << _brokerPort << std::endl;
				connectTimer++;
				sleep(5);
			} else {
				_connected = true;
				LOGM(info) << "Connection to MQTT broker " << _brokerHost << ":" << _brokerPort << " established!";
				connectTimer = 0;
			}
		}

		if (_connected) {
			if (getTimestamp() - idleTime >= PUSHER_IDLETIME) {
				idleTime = getTimestamp();
				readingCtr+= totalCount;
				totalCount = 0;
				// Push sensor data
				for (auto &p : _plugins) {
					if (_doHalt) {
						//for faster response
						break;
					}
					for (const auto &g : p.configurator->getSensorGroups()) {
						for (const auto &s : g->acquireSensors()) {
							if (s->getSizeOfReadingQueue() >= g->getMinValues()) {
								if (_msgCap == DISABLED || totalCount < (unsigned)_maxNumberOfMessages) {
									if (sendReadings(*s, reads, totalCount) ==  0) {
										msgCtr++;
									} else {
										break;
									}
								} else {
									break; //ultimately we will go to sleep 1 second
								}
							}
						}
						g->releaseSensors();
					}
				}
				// Push output analytics sensors
				for (auto &p : _operatorPlugins) {
					if (_doHalt) {
						break;
					}
					for (const auto &op : p.configurator->getOperators()) {
						if (op->getStreaming()) {
							for (const auto &u : op->getUnits()) {
								for (const auto &s : u->getBaseOutputs()) {
									if (s->getSizeOfReadingQueue() >= op->getMinValues()) {
										if (_msgCap == DISABLED || totalCount < (unsigned)_maxNumberOfMessages) {
											if (sendReadings(*s, reads, totalCount) ==  0) {
												msgCtr++;
											} else {
												break;
											}
										} else {
											break;
										}
									}
								}
							}
							op->releaseUnits();
						}
					}
				}
			}

			if ((mosqErr = mosquitto_loop(_mosq, -1, 1)) != MOSQ_ERR_SUCCESS) {
				if (mosqErr == MOSQ_ERR_CONN_LOST) {
					LOGM(info) << "Disconnected.";
					_connected = false;
				} else {
					LOGM(error) << "Error in mosquitto_loop: " << mosquitto_strerror(mosqErr);
				}
			}
			
			if (_statisticsInterval > 0) {
				uint64_t ts = getTimestamp();
				uint64_t elapsed = ts - lastStats;
				if (NS_TO_S(elapsed) > _statisticsInterval) {
					LOG(info) << "Statistics: " << (float) msgCtr*1000/(NS_TO_MS(elapsed)) << " messages/s, " << (float) readingCtr*1000/(NS_TO_MS(elapsed)) << " readings/s";
					if (_statisticsMqttTopic.size() != 0) {
						reading_t r = { (int64_t) msgCtr, ts };
						int rc = MOSQ_ERR_SUCCESS;
						rc+= mosquitto_publish(_mosq, NULL, std::string(_statisticsMqttTopic+"/msgsSent").c_str(), sizeof(reading_t), &r, _qosLevel, false);
						r = { (int64_t) readingCtr, ts };
						rc+= mosquitto_publish(_mosq, NULL, std::string(_statisticsMqttTopic+"/readingsSent").c_str(), sizeof(reading_t), &r, _qosLevel, false);
						if (rc != MOSQ_ERR_SUCCESS) {
							LOGM(info) << "Error sending statistics via MQTT: " << mosquitto_strerror(rc);
							_connected = false;
						}
					}
					lastStats = ts;
					msgCtr = 0;
					readingCtr = 0;
				}
			}
		}
	}
	delete[] reads;
	mosquitto_disconnect(_mosq);
}

int MQTTPusher::sendReadings(SensorBase &s, reading_t *reads, std::size_t &totalCount) {
	//get all sensor values out of its queue
	std::size_t count = s.popReadingQueue(reads, 1024);
	totalCount+= count;
//	totalCount += 1;
#ifdef DEBUG
	LOGM(debug) << "Sending " << count << " values from " << s.getName();
#endif

#if DEBUG
	for (std::size_t i = 0; i < count; i++) {
		LOG(debug) << "  " << reads[i].timestamp << " " << reads[i].value;
	}
#endif
	//try to send them to the broker
	int rc;
	if ((rc = mosquitto_publish(_mosq, NULL, s.getMqtt().c_str(), sizeof(reading_t) * count, reads, _qosLevel, false)) != MOSQ_ERR_SUCCESS) {
		//could not send them --> push the sensor values back into the queue
		if (rc == MOSQ_ERR_NOMEM) {
			LOGM(info) << "Can\'t queue additional messages";
		} else {
			LOGM(debug) << "Could not send message: " << mosquitto_strerror(rc) << " Trying again later";
			_connected = false;
		}
		s.pushReadingQueue(reads, count);
		totalCount -= count;
//		totalCount -= 1;
		return 1;
	}
	return 0;
}

bool MQTTPusher::sendMappings() {
	if (!_autoPublish)
		return false;

	std::string  topic, payload;
	unsigned int publishCtr = 0;
	// Performing auto-publish for sensors
	for (auto &p : _plugins) {
		for (auto &g : p.configurator->getSensorGroups()) {
			for (auto &s : g->acquireSensors()) {
				if (s->getPublish()) {
					if (!s->getMetadata()) {
						topic = std::string(DCDB_MAP) + s->getMqtt();
						payload = s->getName();
					} else {
						topic = std::string(DCDB_MET) + s->getMqtt();
						try {
							payload = s->getMetadata()->getJSON();
						} catch (const std::exception &e) {
							LOGM(error) << "Malformed metadata for sensor " << s->getName() << "!";
							continue;
						}
					}

					// Try to send mapping to the broker
					if (mosquitto_publish(_mosq, NULL, topic.c_str(), payload.length(), payload.c_str(), _qosLevel, false) != MOSQ_ERR_SUCCESS) {
						LOGM(error) << "Broker not reachable! Only " << publishCtr << " sensors were published.";
						_connected = false;
						return true;
					} else
						publishCtr++;
				}
			}
			g->releaseSensors();
		}
	}

	// Performing auto-publish for analytics output sensors
	for (auto &p : _operatorPlugins)
		for (auto &op : p.configurator->getOperators())
			if (op->getStreaming() && !op->getDynamic()) {
				for (auto &u : op->getUnits())
					for (auto &s : u->getBaseOutputs()) {
						if (s->getPublish()) {
							if (!s->getMetadata()) {
								topic = std::string(DCDB_MAP) + s->getMqtt();
								payload = s->getName();
							} else {
								topic = std::string(DCDB_MET) + s->getMqtt();
								try {
									payload = s->getMetadata()->getJSON();
								} catch (const std::exception &e) {
									LOGM(error) << "Malformed metadata for sensor " << s->getName() << "!";
									continue;
								}
							}

							// Try to send mapping to the broker
							if (mosquitto_publish(_mosq, NULL, topic.c_str(), payload.length(), payload.c_str(), _qosLevel, false) != MOSQ_ERR_SUCCESS) {
								LOGM(error) << "Broker not reachable! Only " << publishCtr << " sensors were published.";
								_connected = false;
								return true;
							} else
								publishCtr++;
						}
					}
				op->releaseUnits();
			}
	LOGM(info) << "Sensor name auto-publish performed for " << publishCtr << " sensors!";
	return true;
}

bool MQTTPusher::halt(unsigned short timeout) {
	_doHalt = true;

	for (unsigned short i = 1; i <= timeout; i++) {
		if (_halted) {
			return true;
		} else {
			LOGM(info) << "Waiting for push cycle to pause... (" << i << "/" << timeout << ")";
			sleep(1);
		}
	}

	cont();
	LOGM(info) << "Timeout: push cycle did not pause. Continuing...";
	return false;
}

void MQTTPusher::computeMsgRate() {
	// Computing number of sent MQTT messages per second
	float msgRate = 0;
	bool  dynWarning = false;
	for (auto &p : _plugins) {
		for (const auto &g : p.configurator->getSensorGroups()) {
		    msgRate += g->getMsgRate();
		}
	}
	for (auto &p : _operatorPlugins)
		for (const auto &op : p.configurator->getOperators()) {
			if (op->getStreaming() && !op->getDynamic()) {
			    msgRate += op->getMsgRate();
			} else if (op->getDynamic())
				dynWarning = true;
		}
	// The formula below assumes the pusher's sleep time is 1 sec; if not, change accordingly
	if (_maxNumberOfMessages >= 0 && _msgCap != MINIMUM) {
		_msgCap = _maxNumberOfMessages == 0 || msgRate > _maxNumberOfMessages ? DISABLED : ENABLED;
		if (_msgCap == DISABLED && _maxNumberOfMessages > 0)
			LOGM(warning) << "Cannot enforce max rate of " << _maxNumberOfMessages << " msg/s lower than actual " << msgRate << " msg/s!";
		else if (_maxNumberOfMessages > 0)
			LOGM(info) << "Enforcing message cap of " << _maxNumberOfMessages << " msg/s against actual " << msgRate << " msg/s.";
		else
			LOGM(info) << "No message cap enforced. Predicted message rate is " << msgRate << " msg/s.";
	} else {
		_msgCap = MINIMUM;
		_maxNumberOfMessages = msgRate + 10;
		LOGM(info) << "Enforcing message cap of " << _maxNumberOfMessages << " msg/s against actual " << msgRate << " msg/s.";
	}
	if (_msgCap != DISABLED && dynWarning)
		LOGM(warning) << "Attention! The computed message rate does not account for analyzers with dynamically-generated sensors.";
}
