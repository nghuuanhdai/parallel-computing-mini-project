//================================================================================
// Name        : dcdbpusher.cpp
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Main function for the DCDB MQTT Pusher.
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
 * @defgroup pusher Pusher
 *
 * @brief Data harvester for DCDB.
 *
 * @details Pusher is the DCDB building block responsible for gathering data from
 * 	        hard-/software and pushing the values to the collect agent. The data
 * 	        collection capabilities of pusher stem from its plugins.
 */

/**
 * @file dcdbpusher.cpp
 *
 * @brief Main function for the DCDB MQTT Pusher.
 *
 * @ingroup pusher
 */

#include <dcdbdaemon.h>
#include <functional>
#include <string>
#include <vector>

#include "Configuration.h"
#include "MQTTPusher.h"
#include "PluginManager.h"
#include "RestAPI.h"
#include "version.h"
#include "abrt.h"

#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

using namespace std;

int retCode = 0;
bool restAPIEnabled = false;
Configuration *  _configuration;
MQTTPusher *     _mqttPusher;
PluginManager *  _pluginManager;
RestAPI *        _httpsServer;
OperatorManager *_operatorManager;
QueryEngine &    _queryEngine = QueryEngine::getInstance();

boost::shared_ptr<boost::asio::io_service::work> keepAliveWork;

bool sensorQueryCallback(const string &name, const uint64_t startTs, const uint64_t endTs, std::vector<reading_t> &buffer, const bool rel, const uint64_t tol) {
	// Returning NULL if the query engine is being updated
	if (_queryEngine.updating.load())
		return false;
	bool found = false;
	++_queryEngine.access;
	shared_ptr<map<string, SBasePtr>> sensorMap = _queryEngine.getSensorMap();
	
	if (sensorMap != nullptr && sensorMap->count(name) > 0) {
		SBasePtr sensor = sensorMap->at(name);
		found = sensor->isInit() ? sensor->getCache()->getView(startTs, endTs, buffer, rel, tol) : false;
	}
	--_queryEngine.access;
	return found;
}

bool sensorGroupQueryCallback(const std::vector<string> &names, const uint64_t startTs, const uint64_t endTs, std::vector<reading_t> &buffer, const bool rel, const uint64_t tol) {
	// Returning NULL if the query engine is being updated
	if (_queryEngine.updating.load())
		return false;
	bool outcome = false;
	for(const auto& name : names) {
        outcome = sensorQueryCallback(name, startTs, endTs, buffer, rel, tol) || outcome;
    }
	return outcome;
}

bool metadataQueryCallback(const string &name, SensorMetadata &buffer) {
	// Returning NULL if the query engine is being updated
	if (_queryEngine.updating.load())
		return false;
	bool found = false;
	++_queryEngine.access;
	shared_ptr<map<string, SBasePtr>> sensorMap = _queryEngine.getSensorMap();

	if (sensorMap != nullptr && sensorMap->count(name) > 0) {
		SBasePtr sensor = sensorMap->at(name);
		if (!sensor->getMetadata()) {
			found = false;
		} else {
			found = true;
			buffer = *sensor->getMetadata();
		}
	}
	--_queryEngine.access;
	return found;
}

void sigHandler(int sig) {
	boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
	if (sig == SIGINT) {
		LOG(fatal) << "Received SIGINT";
		retCode = 0;
	} else if (sig == SIGTERM) {
		LOG(fatal) << "Received SIGTERM";
		retCode = 0;
	} else if( sig == SIGUSR1 ) {
		LOG(fatal) << "Received SIGUSR1 via REST API";
		retCode = !_httpsServer ? EXIT_SUCCESS : _httpsServer->getReturnCode();
	}

	LOG(info) << "Stopping sensors...";
	//Stop data analytics plugins and operators
	_operatorManager->stop();

	//Stop all sensors
	_pluginManager->stopPlugin();

	if (restAPIEnabled) {
		//Stop https server
		LOG(info) << "Stopping REST API Server...";
		_httpsServer->stop();
	}

	//Stop io service by killing keepAliveWork
	keepAliveWork.reset();

	//Stop MQTTPusher
	LOG(info) << "Flushing MQTT queues...";
	_mqttPusher->stop();
}

void printSyntax() {
	/*
                       1         2         3         4         5         6         7         8
             012345678901234567890123456789012345678901234567890123456789012345678901234567890
*/
	cout << "Usage:" << endl;
	cout << "  dcdbpusher [-d] [-x] [-a] [-b<host>] [-m<string>] [-w<path>] [-v<level>] <config>" << endl;
	cout << "  dcdbpusher -h" << endl;
	cout << endl;

	cout << "Options:" << endl;
	cout << "  -b <host>       MQTT broker                  [default: " << DEFAULT_BROKERHOST << ":" << DEFAULT_BROKERPORT << "]" << endl;
	cout << "  -m <string>     MQTT topic prefix            [default: none]" << endl;
	cout << "  -w <path>       Writable temp dir            [default: " << DEFAULT_TEMPDIR << "]" << endl;
	cout << "  -v <level>      Set verbosity of output      [default: " << DEFAULT_LOGLEVEL << "]" << endl
	     << "                  Can be a number between 5 (all) and 0 (fatal)." << endl;
	cout << endl;
	cout << "  -d              Daemonize" << endl;
	cout << "  -x              Parse and print the config but do not actually start dcdbpusher" << endl;
	cout << "  -a			   Enable sensor auto-publish" << endl;
	cout << "  -h              This help page" << endl;
	cout << endl;
}

int main(int argc, char **argv) {
	cout << "dcdbpusher " << VERSION << endl
	     << endl;
	boost::asio::io_service io;
	boost::thread_group     threads;

	if (argc <= 1) {
		cout << "Please specify a path to the config-directory or a config-file" << endl
		     << endl;
		printSyntax();
		return 1;
	}

	//define allowed command-line options once
	const char opts[] = "b:p:m:v:w:dxah";

	//check if help flag specified
	char c;
	while ((c = getopt(argc, argv, opts)) != -1) {
		switch (c) {
			case 'h':
				printSyntax();
				return 1;
			default:
				//do nothing (other options are read later on)
				break;
		}
	}
	
	//init LOGGING
	initLogging();
	
	//set up logger to command line
	auto cmdSink = setupCmdLogger();
	
	//get logger instance
	boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
	//finished logging startup for the moment (file log added later)

	try {
		_configuration = new Configuration(argv[argc - 1], "dcdbpusher.conf");
		
		//Read global variables from config file
		_configuration->readConfig();
		
		//plugin and restAPI settings are actually part of globalSettings
		//use the references as shortcut, so that we do not have to prefix with 'globalSettings.' all the time
		Configuration &      globalSettings = *_configuration;
		pluginSettings_t &   pluginSettings = _configuration->pluginSettings;
		serverSettings_t &   restAPISettings = _configuration->restAPISettings;
		analyticsSettings_t &analyticsSettings = _configuration->analyticsSettings;
		
		//reset getopt()
		optind = 1;
		//read in options (overwrite dcdbpusher.conf settings if necessary)
		while ((c = getopt(argc, argv, opts)) != -1) {
			switch (c) {
				case 'a':
					pluginSettings.autoPublish = true;
					break;
				case 'b':
					globalSettings.brokerHost = parseNetworkHost(optarg);
					globalSettings.brokerPort = parseNetworkPort(optarg) == "" ? DEFAULT_BROKERPORT : stoi(parseNetworkPort(optarg));
					break;
				case 'm':
					pluginSettings.mqttPrefix = MQTTChecker::formatTopic(optarg);
					break;
				case 'v':
					globalSettings.logLevelCmd = stoi(optarg);
					break;
				case 'd':
					globalSettings.daemonize = 1;
					break;
				case 'x':
					globalSettings.validateConfig = true;
					break;
				case 'w':
					pluginSettings.tempdir = optarg;
					if (pluginSettings.tempdir[pluginSettings.tempdir.length() - 1] != '/') {
						pluginSettings.tempdir.append("/");
					}
					break;
				case 'h':
					printSyntax();
					return 1;
				default:
					if (c != '?')
						cerr << "Unknown parameter: " << c << endl;
					return 1;
			}
		}
		
		//we now should know where the writable tempdir is
		//set up logger to file
		if (globalSettings.logLevelFile >= 0) {
			auto fileSink = setupFileLogger(pluginSettings.tempdir, std::string("dcdbpusher"));
			fileSink->set_filter(boost::log::trivial::severity >= translateLogLevel(globalSettings.logLevelFile));
		}
		
		//severity level may be overwritten (per option or config-file) --> set it according to globalSettings
		if (globalSettings.logLevelCmd >= 0) {
			cmdSink->set_filter(boost::log::trivial::severity >= translateLogLevel(globalSettings.logLevelCmd));
		}
		
		LOG(info) << "Logging setup complete";
		
		_pluginManager = new PluginManager(io, pluginSettings);
		//Read in rest of configuration. Also creates all sensors
		if (!_configuration->readPlugins(*_pluginManager)) {
			LOG(fatal) << "Failed to read configuration!";
			return 1;
		}
		
		_operatorManager = new OperatorManager(io);
		// Preparing the SensorNavigator
		if (_operatorManager->probe(globalSettings.cfgFilePath, globalSettings.cfgFileName)) {
			std::shared_ptr<SensorNavigator> navigator = std::make_shared<SensorNavigator>();
			vector<std::string>              topics;
			for (const auto &p : _pluginManager->getPlugins())
				for (const auto &g : p.configurator->getSensorGroups()) {
					for (const auto &s : g->acquireSensors())
						topics.push_back(s->getMqtt());
					g->releaseSensors();
				}
			try {
				navigator->setFilter(analyticsSettings.filter);
				navigator->buildTree(analyticsSettings.hierarchy, &topics);
				topics.clear();
				LOG(info) << "Built a sensor hierarchy tree of size " << navigator->getTreeSize() << " and depth " << navigator->getTreeDepth() << ".";
			} catch (const std::invalid_argument &e) {
				navigator->clearTree();
				LOG(error) << e.what();
				LOG(error) << "Failed to build sensor hierarchy tree!";
			}
			_queryEngine.setNavigator(navigator);
		}
		
		_queryEngine.setFilter(analyticsSettings.filter);
		_queryEngine.setJobFilter(analyticsSettings.jobFilter);
		_queryEngine.setJobMatch(analyticsSettings.jobMatch);
		_queryEngine.setJobIDFilter(analyticsSettings.jobIdFilter);
		_queryEngine.setJobDomainId(analyticsSettings.jobDomainId);
		_queryEngine.setSensorHierarchy(analyticsSettings.hierarchy);
		_queryEngine.setQueryCallback(sensorQueryCallback);
		_queryEngine.setGroupQueryCallback(sensorGroupQueryCallback);
		_queryEngine.setMetadataQueryCallback(metadataQueryCallback);
		
		if (!_operatorManager->load(globalSettings.cfgFilePath, globalSettings.cfgFileName, pluginSettings)) {
			LOG(fatal) << "Failed to load data analytics manager!";
			return 1;
		} else if (!_operatorManager->getPlugins().empty()) {
			// Preparing the sensor map used for the QueryEngine
			std::shared_ptr<std::map<std::string, SBasePtr>> sensorMap = std::make_shared<std::map<std::string, SBasePtr>>();
			
			for (const auto &p : _pluginManager->getPlugins())
				for (const auto &g : p.configurator->getSensorGroups()) {
					for (const auto &s : g->acquireSensors())
						sensorMap->insert(std::make_pair(s->getName(), s));
					g->releaseSensors();
				}
			
			for (auto &p : _operatorManager->getPlugins()) {
				for (const auto &op : p.configurator->getOperators())
					if (op->getStreaming()) {
						for (const auto &u : op->getUnits())
							for (const auto &o : u->getBaseOutputs())
								sensorMap->insert(std::make_pair(o->getName(), o));
						op->releaseUnits();
					}
			}
			_queryEngine.setSensorMap(sensorMap);
		}
		
		//print configuration to give some feedback
		//config of plugins is only printed if the config shall be validated or to debug level otherwise
		LOG_LEVEL vLogLevel = LOG_LEVEL::debug;
		if (globalSettings.validateConfig) {
			vLogLevel = boost::log::trivial::info;
		}
		LOG_VAR(vLogLevel) << "-----  Configuration  -----";
		
		//print global settings in either case
		LOG(info) << "Global Settings:";
		LOG(info) << "    Broker:             " << globalSettings.brokerHost << ":" << globalSettings.brokerPort;
		LOG(info) << "    Threads:            " << globalSettings.threads;
		LOG(info) << "    Daemonize:          " << (globalSettings.daemonize ? "Enabled" : "Disabled");
		LOG(info) << "    MaxMsgNum:          " << globalSettings.maxMsgNum;
		LOG(info) << "    MaxInflightMsgNum:  " << globalSettings.maxInflightMsgNum;
		LOG(info) << "    MaxQueuedMsgNum:    " << globalSettings.maxQueuedMsgNum;
		LOG(info) << "    MQTT-QoS:           " << globalSettings.qosLevel;
		LOG(info) << "    MQTT-prefix:        " << pluginSettings.mqttPrefix;
		LOG(info) << "    Auto-publish:       " << (pluginSettings.autoPublish ? "Enabled" : "Disabled");
		LOG(info) << "    Write-Dir:          " << pluginSettings.tempdir;
		LOG(info) << "    CacheInterval:      " << pluginSettings.cacheInterval / 1000 << " [s]";
		LOG(info) << "    StatisticsInterval: " << globalSettings.statisticsInterval << " [s]";
		LOG(info) << "    StatisticsMqttPart: " << globalSettings.statisticsMqttPart;

		if (globalSettings.validateConfig) {
			LOG(info) << "    Only validating config files.";
		} else {
			LOG(info) << "    ValidateConfig:     Disabled";
		}
		LOG(info) << "Analytics Settings:";
		LOG(info) << "    Hierarchy:          " << (analyticsSettings.hierarchy != "" ? analyticsSettings.hierarchy : "none");
		LOG(info) << "    Filter:             " << (analyticsSettings.filter != "" ? analyticsSettings.filter : "none");
		LOG(info) << "    Job Filter:         " << (analyticsSettings.jobFilter != "" ? analyticsSettings.jobFilter : "none");
		LOG(info) << "    Job Match:          " << (analyticsSettings.jobMatch != "" ? analyticsSettings.jobMatch : "none");
		LOG(info) << "    Job ID Filter:      " << (analyticsSettings.jobIdFilter != "" ? analyticsSettings.jobIdFilter : "none");
		LOG(info) << "    Job Domain ID:      " << analyticsSettings.jobDomainId;
		if (restAPISettings.enabled) {
			LOG(info) << "RestAPI Settings:";
			LOG(info) << "    REST Server: " << restAPISettings.host << ":" << restAPISettings.port;
			LOG(info) << "    Certificate: " << restAPISettings.certificate;
			LOG(info) << "    Private key file: " << restAPISettings.privateKey;
		}
		LOG_VAR(vLogLevel) << "-----  Sampling Configuration  -----";
		for (auto &p : _pluginManager->getPlugins()) {
			LOG_VAR(vLogLevel) << "Sampling Plugin \"" << p.id << "\"";
			p.configurator->printConfig(vLogLevel);
		}
		
		LOG_VAR(vLogLevel) << "-----  Analytics Configuration  -----";
		for (auto &p : _operatorManager->getPlugins()) {
			LOG_VAR(vLogLevel) << "Operator Plugin \"" << p.id << "\"";
			p.configurator->printConfig(vLogLevel);
		}
		
		LOG_VAR(vLogLevel) << "-----  End Configuration  -----";
		
		//MQTTPusher and Https server get their own threads
		std::string statisticsMqttTopic;
		if (globalSettings.statisticsMqttPart.size() > 0) {
			statisticsMqttTopic = pluginSettings.mqttPrefix + globalSettings.statisticsMqttPart;
		}
		_mqttPusher = new MQTTPusher(globalSettings.brokerPort, globalSettings.brokerHost, pluginSettings.autoPublish, globalSettings.qosLevel,
					     _pluginManager->getPlugins(), _operatorManager->getPlugins(), globalSettings.maxMsgNum, globalSettings.maxInflightMsgNum, globalSettings.maxQueuedMsgNum, globalSettings.statisticsInterval,
					         statisticsMqttTopic);
		if (restAPISettings.enabled) {
			_httpsServer = new RestAPI(restAPISettings, _pluginManager, _mqttPusher, _operatorManager, io);
			_configuration->readRestAPIUsers(_httpsServer);
		}
		if (globalSettings.validateConfig) {
			return 0;
		}
		
		//Init all sensors
		LOG(info) << "Init sensors...";
		_pluginManager->initPlugin();
		
		//Start all sensors
		LOG(info) << "Starting sensors...";
		_pluginManager->startPlugin();
		
		if (!_queryEngine.updating.is_lock_free())
			LOG(warning) << "This machine does not support lock-free atomics. Performance may be degraded.";
		
		LOG(info) << "Init operators...";
		_operatorManager->init();
		
		LOG(info) << "Starting operators...";
		_operatorManager->start();
		
		LOG(info) << "Sensors started!";
		
		if (globalSettings.daemonize) {
			//boost.log does not support forking officially.
			//however, just don't touch the sinks after daemonizing and it should work nonetheless
			LOG(info) << "Detaching...";
			
			cmdSink->flush();
			boost::log::core::get()->remove_sink(cmdSink);
			cmdSink.reset();
			
			//daemonize
			dcdbdaemon();
			
			LOG(info) << "Now detached";
		}
		
		LOG(info) << "Creating threads...";
		
		//dummy to keep io service alive even if no tasks remain (e.g. because all sensors have been stopped over REST API)
		keepAliveWork = boost::make_shared<boost::asio::io_service::work>(io);
		
		//Create pool of threads which handle the sensors
		for (size_t i = 0; i < globalSettings.threads; i++) {
			threads.create_thread(bind(static_cast<size_t (boost::asio::io_service::*)()>(&boost::asio::io_service::run), &io));
		}
		
		boost::thread mqttThread(bind(&MQTTPusher::push, _mqttPusher));
		
		LOG(info) << "Threads created!";
		
		if (restAPISettings.enabled) {
			LOG(info) << "Starting RestAPI Https Server...";
			_httpsServer->start();
		}
		
		LOG(info) << "Registering signal handlers...";
		signal(SIGINT, sigHandler);  //Handle Strg+C
		signal(SIGTERM, sigHandler); //Handle termination
		signal(SIGUSR1, sigHandler); // Handle user-requested termination
		LOG(info) << "Signal handlers registered!";
		
		LOG(info) << "Cleaning up...";
		restAPIEnabled = restAPISettings.enabled;
		delete _configuration;
		
		LOG(info) << "Setup complete!";
		
		LOG(trace) << "Running...";
		
		//Run until Strg+C
		threads.join_all();
		
		//will only continue if interrupted by SIGINT and threads were stopped
		
		mqttThread.join();
		LOG(info) << "MQTTPusher stopped.";
		
		LOG(info) << "Tearing down objects...";
		_queryEngine.setNavigator(nullptr);
		_queryEngine.setSensorMap(nullptr);
		delete _mqttPusher;
		delete _operatorManager;
		delete _pluginManager;
        if (restAPIEnabled) {
            delete _httpsServer;
        }
	}
	catch (const std::runtime_error& e) {
		LOG(fatal) <<  e.what();
		return EXIT_FAILURE;
	}
	catch (const exception& e) {
		LOG(fatal) << "Exception: " << e.what();
		abrt(EXIT_FAILURE, INTERR);
	}
	
	LOG(info) << "Exiting...Goodbye!";
	return retCode;
}
