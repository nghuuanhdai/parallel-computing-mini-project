//================================================================================
// Name        : MQTTPusher.h
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

#ifndef MQTTPUSHER_H_
#define MQTTPUSHER_H_

#define DCDB_MAP "/DCDB_MAP/"
#define DCDB_MET "/DCDB_MAP/METADATA/"
#define PUSHER_IDLETIME 1000000000

#include "../analytics/OperatorManager.h"
#include "PluginManager.h"
#include "sensorbase.h"
#include <map>
#include <mosquitto.h>

enum msgCap_t { DISABLED = 1,
		ENABLED = 2,
		MINIMUM = 3 };

/**
 * @brief Collects values from the sensors and pushes them to the database.
 *
 * @ingroup pusher
 */
class MQTTPusher {
      public:
	MQTTPusher(int brokerPort, const std::string &brokerHost, const bool autoPublish, int qosLevel,
		   pusherPluginStorage_t &plugins, op_pluginVector_t &oPlugins, int maxNumberOfMessages, unsigned int maxInflightMsgNum, unsigned int maxQueuedMsgNum, unsigned int statisticsInterval, std::string statisticsMqttTopic);
	virtual ~MQTTPusher();

	/**
	 * @brief MQTTPusher's main execution loop.
	 *
	 * @details If MQTTPusher is started this function runs indefinitely until
	 *          a call to stop(). Execution of the main loop can be halted and
	 *          continued with halt() and cont() respectively.
	 */
	void push();

	/**
	 * @brief
	 *
	 * @return
	 */
	bool sendMappings();

	/**
	 * @brief Start MQTTPusher's push loop.
	 */
	void start() {
		_keepRunning = true;
	}

	/**
	 * @brief Stop MQTTPusher's push loop and terminate its execution.
	 */
	void stop() {
		_keepRunning = false;
	}

	/**
	 * @brief Blocking call to pause MQTTPusher's push loop.
	 *
	 * @details Instructs MQTTPusher to pause its push loop and blocks until
	 *          MQTTPusher is actually paused or a timeout occurred. On a
	 *          timeout execution of MQTTPusher is continued.
	 *
	 * @param timeout Time in seconds to wait for MQTTPusher to finish its
	 *                current push cycle.
	 *
	 * @return True if MQTTPusher was succesfully paused, false if a timeout
	 *         occurred and MQTTPusher still runs.
	 */
	bool halt(unsigned short timeout = 5);

	/**
	 * @brief Continue MQTTPusher's push loop.
	 */
	void cont() {
		computeMsgRate();
		_doHalt = false;
	}

      private:
	int  sendReadings(SensorBase &s, reading_t *reads, std::size_t &totalCount);
	void computeMsgRate();

	int                    _qosLevel;
	int                    _brokerPort;
	std::string            _brokerHost;
	bool                   _autoPublish;
	pusherPluginStorage_t &_plugins;
	op_pluginVector_t &    _operatorPlugins;
	struct mosquitto *     _mosq;
	bool                   _connected;
	bool                   _keepRunning;
	msgCap_t               _msgCap;
	bool                   _doHalt;
	bool                   _halted;
	int                    _maxNumberOfMessages;
	unsigned int           _maxInflightMsgNum;
	unsigned int           _maxQueuedMsgNum;
	unsigned int           _statisticsInterval;
	std::string            _statisticsMqttTopic;

	boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
};

#endif /* MQTTPUSHER_H_ */
