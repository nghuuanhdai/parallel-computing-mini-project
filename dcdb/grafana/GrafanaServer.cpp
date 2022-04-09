//================================================================================
// Name        : GrafanaServer.cpp
// Author      : Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : An HTTPS server that processes requests from a Grafana frontend.
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
#include "Configuration.h"
#include "RestAPI.h"
#include "version.h"
#include "dcdbdaemon.h"
#include "abrt.h"

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <iostream>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/algorithm/string.hpp>

Configuration*      _configuration;
RestAPI*            _httpsServer;
DCDB::Connection*   _cassandraConnection;

boost::shared_ptr<boost::asio::io_service::work> keepAliveWork;

void printSyntax()
{
    /*
     1         2         3         4         5         6         7         8
     012345678901234567890123456789012345678901234567890123456789012345678901234567890
     */
    std::cout << "Usage:" << std::endl;
    std::cout << "  grafanaserver [-d] [-c<host:port>] [-u<username>] [-p<password>] [-t<number>] [-v<level>] [-w<path>] <config>" << std::endl;
    std::cout << "  grafanaserver -h" << std::endl;
    std::cout << std::endl;

    std::cout << "Options:" << std::endl;
    std::cout << "  -c <host:port>  Cassandra host and port.    [default: " << DEFAULT_CASSANDRAHOST << ":" << DEFAULT_CASSANDRAPORT << "]" << endl;
	std::cout << "  -u<username>    Cassandra username          [default: none]" << endl;
	std::cout << "  -p<password>    Cassandra password          [default: none]" << endl;
    std::cout << "  -t <number>     Thread count.               [default: " << DEFAULT_THREADS << "]" << std::endl;
    std::cout << "  -v <level>      Set verbosity of output.    [default: " << DEFAULT_LOGLEVEL << "]" << std::endl
    <<           "                  Can be a number between 5 (all) and 0 (fatal)." << std::endl;
    std::cout << "  -w <path>       Writable temp dir.          [default: " << DEFAULT_TEMPDIR << "]" << endl;
    std::cout << std::endl;
    std::cout << "  -d              Daemonize." << std::endl;
    std::cout << "  -h              This help page." << std::endl;
    std::cout << std::endl;
}

void sigHandler(int sig) {
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
    if( sig == SIGINT )
        LOG(fatal) << "Received SIGINT";
    else if( sig == SIGTERM )
        LOG(fatal) << "Received SIGTERM";
    
    //Stop https server
    LOG(info) << "Stopping REST API Server...";
    _httpsServer->stop();

	//Stop io service by killing keepAliveWork
	keepAliveWork.reset();

    //Stop the Cassandra connection.
    LOG(info) << "Closing Cassandra connection...";
    _cassandraConnection->disconnect();
}


#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    cout << "grafanaserver " << VERSION << endl << endl;
    boost::asio::io_service io;
    boost::thread_group threads;
    
    if (argc <= 1) {
        cout << "Please specify a path to the config-directory or a config-file" << endl << endl;
        printSyntax();
        return 1;
    }
    
    //define allowed command-line options once
    const char opts[] = "c:u:p:t:v:w:dh";
    
    //check if help flag specified
    char c;
    while ((c = getopt(argc, argv, opts)) != -1) {
        switch (c)
        {
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
		_configuration = new Configuration(argv[argc-1], "grafana.conf");
		
		//Read global variables from config file
		_configuration->readConfig();
		
		//read global settings
		Configuration& globalSettings = *_configuration;
		cassandraSettings_t& cassandraSettings = _configuration->cassandraSettings;
		serverSettings_t& restAPISettings = _configuration->restAPISettings;
		hierarchySettings_t& hierarchySettings = _configuration->hierarchySettings;
		
		//reset getopt()
		optind = 1;
		//read in options (overwrite dcdbpusher.conf settings if necessary)
		while ((c = getopt(argc, argv, opts)) != -1) {
			switch (c)
			{
			case 'c':
				cassandraSettings.host = parseNetworkHost(optarg);
				cassandraSettings.port = parseNetworkPort(optarg) == "" ? DEFAULT_CASSANDRAPORT : parseNetworkPort(optarg);
				break;
			case 'u':
				cassandraSettings.username = optarg;
				break;
			case 'p': {
				cassandraSettings.password = optarg;
				// What does this do? Mask the password?
				size_t pwdLen = strlen(optarg);
				memset(optarg, 'x', (pwdLen >= 3) ? 3 : pwdLen);
				if (pwdLen > 3) {
					memset(optarg+3, 0, pwdLen-3);
				}
				break;
			}
			case 't':
				globalSettings.threads = stoul(optarg);
			case 'v':
				globalSettings.logLevelCmd = stoi(optarg);
				break;
			case 'd':
				globalSettings.daemonize = 1;
				break;
			case 'w':
				globalSettings.tempdir = optarg;
				if (globalSettings.tempdir[globalSettings.tempdir.length()-1] != '/') {
				globalSettings.tempdir.append("/");
				}
				break;
			case 'h':
				printSyntax();
				return 1;
			default:
				if (c != '?') cerr << "Unknown parameter: " << c << endl;
				return 1;
			}
		}
		
		//we now should know where the writable tempdir is
		//set up logger to file
		if (globalSettings.logLevelFile >= 0) {
			auto fileSink = setupFileLogger(globalSettings.tempdir, std::string("grafanaserver"));
			fileSink->set_filter(boost::log::trivial::severity >= translateLogLevel(globalSettings.logLevelFile));
		}
		
		//severity level may be overwritten (per option or config-file) --> set it according to globalSettings
		if (globalSettings.logLevelCmd >= 0) {
			cmdSink->set_filter(boost::log::trivial::severity >= translateLogLevel(globalSettings.logLevelCmd));
		}
		
		LOG(info) << "Logging setup complete";
		
		//print configuration to give some feedback
		//config of plugins is only printed if the config shall be validated or to debug level otherwise
		LOG_LEVEL vLogLevel = LOG_LEVEL::debug;
		LOG_VAR(vLogLevel) << "-----  Configuration  -----";
		
		//print global settings in either case
		LOG(info) << "Global Settings:";
		LOG(info) << "    Threads:            " << globalSettings.threads;
		LOG(info) << "    Daemonize:          " << (globalSettings.daemonize ? "Enabled" : "Disabled");
		LOG(info) << "    Write-Dir:          " << globalSettings.tempdir;
		
		LOG(info) << "Grafana Settings:";
		LOG(info) << "    Grafana Server:     " << restAPISettings.host << ":" << restAPISettings.port;
		LOG(info) << "    Certificate:        " << restAPISettings.certificate;
		LOG(info) << "    Private key file:   " << restAPISettings.privateKey;
		
		LOG(info) << "Cassandra Settings:";
		LOG(info) << "    Address:            " << cassandraSettings.host << ":" << cassandraSettings.port;
		LOG(info) << "    NumThreadsIO:       " << cassandraSettings.numThreadsIo;
		LOG(info) << "    QueueSizeIO:        " << cassandraSettings.queueSizeIo;
		LOG(info) << "    CoreConnPerHost:    " << cassandraSettings.coreConnPerHost;
		LOG(info) << "    DebugLog:           " << (cassandraSettings.debugLog ? "Enabled" : "Disabled");
		
		LOG(info) << "Hierarchy Settings:";
		LOG(info) << "    Regex:              " << (hierarchySettings.regex != "" ? hierarchySettings.regex : "none");
		LOG(info) << "    Separator:          " << (hierarchySettings.separator != "" ? hierarchySettings.separator : "none");
		LOG(info) << "    Filter:             " << (hierarchySettings.filter!="" ? hierarchySettings.filter : "none");
		LOG(info) << "    Smoother Regex:     " << (hierarchySettings.smootherRegex!="" ? hierarchySettings.smootherRegex : "none");
		
		LOG_VAR(vLogLevel) << "-----  End Configuration  -----";
		
		//Setting up the connection with the Cassandra DB
		LOG(info) << "Connecting to the Cassandra database...";
		_cassandraConnection = new DCDB::Connection(cassandraSettings.host, atoi(cassandraSettings.port.c_str()),
										cassandraSettings.username, cassandraSettings.password);
		_cassandraConnection->setNumThreadsIo(cassandraSettings.numThreadsIo);
		_cassandraConnection->setQueueSizeIo(cassandraSettings.queueSizeIo);
		uint32_t params[1] = {cassandraSettings.coreConnPerHost};
		_cassandraConnection->setBackendParams(params);
		
		if (!_cassandraConnection->connect()) {
			throw std::runtime_error("Failed to connect to the Cassandra database!");
		}
		
		_httpsServer = new RestAPI(restAPISettings, hierarchySettings, _cassandraConnection, io);
		_configuration->readRestAPIUsers(_httpsServer);
		
		_httpsServer->checkPublishedSensorsAsync();
		
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
		for(size_t i = 0; i < globalSettings.threads; i++) {
			threads.create_thread(bind(static_cast< size_t (boost::asio::io_service::*) () >(&boost::asio::io_service::run), &io));
		}
		
		LOG(info) << "Threads created!";
		
		LOG(info) << "Starting RestAPI Https Server...";
		_httpsServer->start();
		
		LOG(info) << "Registering signal handlers...";
		signal(SIGINT, sigHandler);    //Handle Strg+C
		signal(SIGTERM, sigHandler);    //Handle termination
		LOG(info) << "Signal handlers registered!";
		
		LOG(info) << "Cleaning up...";
		delete _configuration;
		
		LOG(info) << "Setup complete!";
		
		LOG(trace) << "Running...";
		
		//Run until Strg+C
		threads.join_all();
		
		//will only continue if interrupted by SIGINT and threads were stopped
		LOG(info) << "Tearing down objects...";
		delete _cassandraConnection;
		delete _httpsServer;
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
    return 0;
}
