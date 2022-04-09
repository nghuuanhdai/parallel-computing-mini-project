//================================================================================
// Name        : collectagent.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Main code of the CollectAgent
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
 * @defgroup ca Collect Agent
 *
 * @brief MQTT message broker in between pusher and storage backend.
 *
 * @details Collect Agent is a intermediary between one or multiple Pusher
 *          instances and one storage backend. It runs a reduced custom MQTT
 *          message server. Collect Agent receives data from Pusher
 *          via MQTT messages and stores them in the storage via libdcdb.
 */

/**
 * @file collectagent.cpp
 *
 * @brief Main function for the DCDB Collect Agent.
 *
 * @ingroup ca
 */

#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <dcdb/libconfig.h>
#include <dcdb/connection.h>
#include <dcdb/sensordatastore.h>
#include <dcdb/jobdatastore.h>
#include <dcdb/calievtdatastore.h>
#include <dcdb/sensorconfig.h>
#include <dcdb/version.h>
#include <dcdb/sensor.h>
#include "version.h"

#include "CARestAPI.h"
#include "configuration.h"
#include "simplemqttserver.h"
#include "messaging.h"
#include "abrt.h"
#include "dcdbdaemon.h"
#include "sensorcache.h"
#include "analyticscontroller.h"
#include "../analytics/includes/QueryEngine.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

/**
 * Uncomment and recompile to activate CollectAgent's special benchmark mode.
 * In this mode, all received messages will be discarded and no data is stored
 * in the storage backend.
 */
//#define BENCHMARK_MODE

using namespace std;

bool newAutoPub;
int keepRunning;
int retCode = EXIT_SUCCESS;
uint64_t msgCtr;
uint64_t readingCtr;
uint64_t dbQueryCtr;
uint64_t cachedQueryCtr;
uint64_t missesQueryCtr;
SensorCache mySensorCache;
AnalyticsController* analyticsController;
DCDB::Connection* dcdbConn;
DCDB::SensorDataStore *mySensorDataStore;
DCDB::JobDataStore *myJobDataStore;
DCDB::SensorConfig *mySensorConfig;
DCDB::CaliEvtDataStore *myCaliEvtDataStore;
MetadataStore *metadataStore;
CARestAPI* httpsServer = nullptr;
DCDB::SCError err;
QueryEngine& queryEngine = QueryEngine::getInstance();
boost::shared_ptr<boost::asio::io_context::work> keepAliveWork;
logger_t lg;

bool jobQueryCallback(const string& jobId, const uint64_t startTs, const uint64_t endTs, vector<qeJobData>& buffer, const bool rel, const bool range, const string& domainId) {
    std::list<JobData> tempList;
    JobData   tempData;
    qeJobData tempQeData;
    JDError err;
    if(range) {
        // Getting a list of jobs in the given time range
        uint64_t now = getTimestamp();
        uint64_t startTsInt = rel ? now - startTs : startTs;
        uint64_t endTsInt = rel ? now - endTs : endTs;
        DCDB::TimeStamp start(startTsInt), end(endTsInt);
        err = myJobDataStore->getJobsInIntervalRunning(tempList, start, end, domainId);
        if(err != JD_OK) return false;
    } else {
        // Getting a single job by id
        err = myJobDataStore->getJobById(tempData, jobId, domainId);
        if(err != JD_OK) return false;
        tempList.push_back(tempData);
    }
    
    for(auto& jd : tempList) {
        tempQeData.domainId = jd.domainId;
        tempQeData.jobId = jd.jobId;
        tempQeData.userId = jd.userId;
        tempQeData.startTime = jd.startTime.getRaw();
        tempQeData.endTime = jd.endTime.getRaw();
        tempQeData.nodes = jd.nodes;
        buffer.push_back(tempQeData);
    }
    return true;
}

bool sensorGroupQueryCallback(const std::vector<string>& names, const uint64_t startTs, const uint64_t endTs, std::vector<reading_t>& buffer, const bool rel, const uint64_t tol) {
    // Returning NULL if the query engine is being updated
    if(queryEngine.updating.load()) return false;
    ++queryEngine.access;
    
    std::list<DCDB::SensorId> topics;
    std::string topic;
    sensorCache_t& sensorMap = mySensorCache.getSensorMap();
    size_t successCtr = 0;
    for(const auto& name : names) {
        // Getting the topic of the queried sensor from the Navigator
        // If not found, we try to use the input name as topic
        try {
            topic = queryEngine.getNavigator()->getNodeTopic(name);
        } catch (const std::domain_error &e) { topic = name; }
        DCDB::SensorId sid;
        // Creating a SID to perform the query
        if (sid.mqttTopicConvert(topic)) {
            mySensorCache.wait();
            if (sensorMap.count(sid) > 0 && sensorMap[sid].getView(startTs, endTs, buffer, rel, tol)) {
                // Data was found, can continue to next SID
                successCtr++;
            } else {
                // This happens only if no data was found in the local cache
                topics.push_back(sid);
            }
            mySensorCache.release();
        }
    }
    if (successCtr) {
	cachedQueryCtr+= buffer.size();
    }
    
    // If we are here then some sensors were not found in the cache - we need to fetch data from Cassandra
    if(!topics.empty()) {
        try {
            std::list <DCDB::SensorDataStoreReading> results;
            uint64_t now = getTimestamp();
            //Converting relative timestamps to absolute
            uint64_t startTsInt = rel ? now - startTs : startTs;
            uint64_t endTsInt = rel ? now - endTs : endTs;
            DCDB::TimeStamp start(startTsInt), end(endTsInt);
            uint16_t startWs=start.getWeekstamp(), endWs=end.getWeekstamp();
            // If timestamps are equal we perform a fuzzy query
            if(startTsInt == endTsInt) {
                topics.front().setRsvd(startWs);
                mySensorDataStore->fuzzyQuery(results, topics, start, tol, false);
            }
            // Else, we iterate over the weekstamps (if more than one) and perform range queries
            else {
                for(uint16_t currWs=startWs; currWs<=endWs; currWs++) {
                    topics.front().setRsvd(currWs);
                    mySensorDataStore->query(results, topics, start, end, DCDB::AGGREGATE_NONE, false);
                }
            }

            if (!results.empty()) {
                successCtr++;
                reading_t reading;
		dbQueryCtr+= results.size();
                for (const auto &r : results) {
                    reading.value = r.value;
                    reading.timestamp = r.timeStamp.getRaw();
                    buffer.push_back(reading);
                }
            } else {
                missesQueryCtr += topics.size();
            }
        }
        catch (const std::exception &e) {}
    }
    
    --queryEngine.access;
    return successCtr>0;
}

bool sensorQueryCallback(const string& name, const uint64_t startTs, const uint64_t endTs, std::vector<reading_t>& buffer, const bool rel, const uint64_t tol) {
    // Returning NULL if the query engine is being updated
    if(queryEngine.updating.load()) return false;
    std::vector<std::string> nameWrapper;
    nameWrapper.push_back(name);
    return sensorGroupQueryCallback(nameWrapper, startTs, endTs, buffer, rel, tol);
}

bool metadataQueryCallback(const string& name, SensorMetadata& buffer) {
    // Returning NULL if the query engine is being updated
    if(queryEngine.updating.load()) return false;
    ++queryEngine.access;
    std::string topic=name;
    // Getting the topic of the queried sensor from the Navigator
    // If not found, we try to use the input name as topic
    try {
        topic = queryEngine.getNavigator()->getNodeTopic(name);
    } catch(const std::domain_error& e) {}
    
    bool local = false;
    metadataStore->wait();
    if(metadataStore->getMap().count(topic)) {
        buffer = metadataStore->get(topic);
        local = true;
    }
    metadataStore->release();
        
    if(!local) {
        // If we are here then the sensor was not found in the cache - we need to fetch data from Cassandra
        try {
            DCDB::PublicSensor publicSensor;
            if (mySensorConfig->getPublicSensorByName(publicSensor, topic.c_str()) != SC_OK) {
                --queryEngine.access;
                return false;
            }
            buffer = DCDB::PublicSensor::publicSensorToMetadata(publicSensor);
        }
        catch (const std::exception &e) {
            --queryEngine.access;
            return false;
        }
    }
    --queryEngine.access;
    return true;
}

/* Normal termination (SIGINT, CTRL+C) */
void sigHandler(int sig)
{
  boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
  if( sig == SIGINT ) {
      LOG(fatal) << "Received SIGINT";
      retCode = EXIT_SUCCESS;
  } else if( sig == SIGTERM ) {
      LOG(fatal) << "Received SIGTERM";
      retCode = EXIT_SUCCESS;
  } else if( sig == SIGUSR1 ) {
      LOG(fatal) << "Received SIGUSR1 via REST API";
      retCode = !httpsServer ? EXIT_SUCCESS : httpsServer->getReturnCode();
  }

  keepAliveWork.reset();
  keepRunning = 0;
}

/* Crash */
void abrtHandler(int sig)
{
  abrt(EXIT_FAILURE, SIGNAL);
}

int mqttCallback(SimpleMQTTMessage *msg)
{
  /*
   * Increment the msgCtr for statistics.
   */
  msgCtr++;

#ifndef BENCHMARK_MODE

  uint64_t len;
  /*
   * Decode the message and put into the database.
   */
  if (msg->isPublish()) {
      const char *topic = msg->getTopic().c_str();
      // We check whether the topic includes the /DCDB_MAP/ keyword, indicating that the payload will contain the
      // sensor's name. In that case, we set the mappingMessage flag to true, and filter the keyword out of the prefix
      // We use strncmp as it is the most efficient way to do it
      if (strncmp(topic, DCDB_MAP, DCDB_MAP_LEN) == 0) {
          if ((len = msg->getPayloadLength()) == 0) {
              LOG(error) << "Empty sensor publish message received!";
              return 1;
          }

          string payload((char *) msg->getPayload(), len);
          //If the topic includes the extended /DCDB_MAP/METADATA/ keyword, we assume a JSON metadata packet is encoded
          if(strncmp(topic, DCDB_MET, DCDB_MET_LEN) == 0) {
              SensorMetadata sm;
              try {
                  sm.parseJSON(payload);
              } catch (const std::exception &e) {
                  LOG(error) << "Invalid metadata packed received!";
                  return 1;
              }
              if(sm.isValid()) {
                  err = mySensorConfig->publishSensor(sm);
                  metadataStore->store(*sm.getPattern(), sm);
              }
          } else {
              err = mySensorConfig->publishSensor(payload.c_str(), topic + DCDB_MAP_LEN);
          }

          // PublishSensor does most of the error checking for us
          switch (err) {
              case DCDB::SC_INVALIDPATTERN:
                  LOG(error) << "Invalid sensor topic : " << msg->getTopic();
                  return 1;
              case DCDB::SC_INVALIDPUBLICNAME:
                  LOG(error) << "Invalid sensor public name.";
                  return 1;
              case DCDB::SC_INVALIDSESSION:
                  LOG(error) << "Cannot reach sensor data store.";
                  return 1;
              default:
                  break;
          }
          
          newAutoPub = true;
      } else if (strncmp(topic, DCDB_CALIEVT, DCDB_CALIEVT_LEN) == 0) {
          /*
           * Special message case. This message contains a Caliper Event data
           * string that is encoded in the MQTT topic. Its payload consists of
           * usual timestamp-value pairs.
           * Data from this messages will be stored in a separate table managed
           * by CaliEvtDataStore class.
           */
          std::string topicStr(msg->getTopic());
          mqttPayload buf, *payload;

          len = msg->getPayloadLength();
          //TODO support case that no timestamp is given?
          //retrieve timestamp and value from the payload
          if ((len % sizeof(mqttPayload) == 0) && (len > 0)) {
              payload = (mqttPayload *) msg->getPayload();
          } else {
              //this message is malformed -> ignore
              LOG(error) << "Message malformed";
              return 1;
          }

          /*
           * Decode message topic in actual sensor topic and string data.
           * "/:/" is used as delimiter between topic and data.
           */
          topicStr.erase(0, DCDB_CALIEVT_LEN);
          size_t pos = topicStr.find("/:/");
          if (pos == std::string::npos) {
              // topic is malformed -> ignore
              LOG(error) << "CaliEvt topic malformed";
              return 1;
          }
          const std::string data(topicStr, pos+3);
          topicStr.erase(pos);

          /*
           * We use the same MQTT-topic/SensorId infrastructure as actual sensor
           * data readings to sort related events.
           * Check if we can decode the event topic into a valid SensorId. If
           * successful, store the record in the database.
           */
          DCDB::SensorId sid;
          if (sid.mqttTopicConvert(topicStr)) {
              //std::list<DCDB::CaliEvtData> events;
              DCDB::CaliEvtData e;
              e.eventId   = sid;
              e.event     = data;
              for (uint64_t i = 0; i < len / sizeof(mqttPayload); i++) {
                  e.timeStamp = payload[i].timestamp;

                  /**
                   * We want an exhaustive list of all events ordered by their
                   * time of occurrence. Payload values should always be
                   * one. Other values currently indicate a malformed message.
                   *
                   * In the future, the value field could be used to aggregate
                   * multiple equal events that occurred in the same plugin
                   * read cycle.
                   */
                  if(payload[i].value != 1) {
                      LOG(error) << "CaliEvt message malformed. Value != 1";
                      return 1;
                  }

                  myCaliEvtDataStore->insert(e, metadataStore->getTTL(topicStr));
                  //events.push_back(e);
              }
              //myCaliEvtDataStore->insertBatch(events, metadataStore->getTTL(topicStr));
          } else {
              LOG(error) << "Topic could not be converted to SID";
          }
      } else if (strncmp(topic, DCDB_JOBDATA, DCDB_JOBDATA_LEN) == 0) {
	      /**
           * This message contains Slurm job data in JSON format. We need to
           * parse the payload and store it within the JobDataStore.
           */
	      if ((len = msg->getPayloadLength()) == 0) {
		      LOG(error) << "Empty job data message received!";
		      return 1;
	      }

	      //parse JSON into JobData object
	      string        payload((char *)msg->getPayload(), len);
	      DCDB::JobData jd;
	      try {
		      boost::property_tree::iptree config;
		      std::istringstream           str(payload);
		      boost::property_tree::read_json(str, config);
		      BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
			      if (boost::iequals(val.first, "jobid")) {
				      jd.jobId = val.second.data();
			      } else if (boost::iequals(val.first, "domainid")) {
                      jd.domainId = val.second.data();
                  } else if (boost::iequals(val.first, "userid")) {
				      jd.userId = val.second.data();
			      } else if (boost::iequals(val.first, "starttime")) {
				      jd.startTime = DCDB::TimeStamp((uint64_t)stoull(val.second.data()));
			      } else if (boost::iequals(val.first, "endtime")) {
				      jd.endTime = DCDB::TimeStamp((uint64_t)stoull(val.second.data()));
			      } else if (boost::iequals(val.first, "nodes")) {
				      BOOST_FOREACH (boost::property_tree::iptree::value_type &node, val.second) {
					      jd.nodes.push_back(node.second.data());
				      }
			      }
		      }
	      } catch (const std::exception &e) {
		      LOG(error) << "Invalid job data packet received!";
		      return 1;
	      }

#ifdef DEBUG
	      LOG(debug) << "JobId  = " << jd.jobId;
	      LOG(debug) << "UserId = " << jd.userId;
	      LOG(debug) << "Start  = " << jd.startTime.getString();
	      LOG(debug) << "End    = " << jd.endTime.getString();
	      LOG(debug) << "Nodes: ";
	      for (const auto &n : jd.nodes) {
		      LOG(debug) << "    " << n;
	      }
#endif

	      //store JobData into Storage Backend
	      //dcdbslurmjob start inserts the endTime as startTime + max job length + 1, i.e. the last digit will be 1 
	      if ((jd.endTime == DCDB::TimeStamp((uint64_t)0)) || ((jd.endTime.getRaw() & 0x1) == 1)) {
		      //starting job data
		      if (myJobDataStore->insertJob(jd) != DCDB::JD_OK) {
			      LOG(error) << "Job data insert for job " << jd.jobId << " failed!";
			      return 1;
		      }
	      } else {
		      //ending job data
		      DCDB::JobData tmp;
		      if (myJobDataStore->getJobById(tmp, jd.jobId, jd.domainId) != DCDB::JD_OK) {
			      LOG(error) << "Could not retrieve job " << jd.jobId << " to be updated!";
			      return 1;
		      }

		      if (myJobDataStore->updateEndtime(tmp.jobId, tmp.startTime, jd.endTime, jd.domainId) != DCDB::JD_OK) {
			      LOG(error) << "Could not update end time of job " << tmp.jobId << "!";
			      return 1;
		      }
	      }
      } else {
	      mqttPayload buf, *payload;

	      len = msg->getPayloadLength();
	      //In the 64 bit message case, the collect agent provides a timestamp
	      if (len == sizeof(uint64_t)) {
		      payload = &buf;
		      payload->value = *((int64_t *)msg->getPayload());
		      payload->timestamp = Messaging::calculateTimestamp();
		      len = sizeof(uint64_t) * 2;
	      }
	      //...otherwise it just retrieves it from the MQTT message payload.
	      else if ((len % sizeof(mqttPayload) == 0) && (len > 0)) {
		      payload = (mqttPayload *)msg->getPayload();
	      }
	      //...otherwise this message is malformed -> ignore...
	      else {
		      LOG(error) << "Message malformed";
		      return 1;
	      }

	      /*
           * Check if we can decode the message topic
           * into a valid SensorId. If successful, store
           * the record in the database.
           */
	      DCDB::SensorId sid;
	      if (sid.mqttTopicConvert(msg->getTopic())) {
#if 0
              cout << "Topic decode successful:" << endl
                  << "  Raw:            " << hex << setw(16) << setfill('0') << sid.getRaw()[0] << hex << setw(16) << setfill('0') << sid.getRaw()[1] << endl
                  << "  DeviceLocation: " << hex << setw(16) << setfill('0') << sid.getDeviceLocation() << endl
                  << "  device_id:      " << hex << setw(8) << setfill('0') << sid.getDeviceSensorId().device_id << endl
                  << "  sensor_number:  " << hex << setw(4) << setfill('0') << sid.getDeviceSensorId().sensor_number << endl << dec;

              cout << "Payload ("  << len/sizeof(mqttPayload) << " messages):"<< endl;
              for (uint64_t i=0; i<len/sizeof(mqttPayload); i++) {
                cout << "  " << i << ": ts=" << payload[i].timestamp << " val=" << payload[i].value << endl;
              }
              cout << endl;
#endif
	      std::list<DCDB::SensorDataStoreReading> readings;
          for (uint64_t i = 0; i < len / sizeof(mqttPayload); i++) {
              DCDB::SensorDataStoreReading r(sid, payload[i].timestamp, payload[i].value);
              readings.push_back(r);
              mySensorCache.storeSensor(sid, payload[i].timestamp, payload[i].value);
          }
          mySensorCache.getSensorMap()[sid].updateBatchSize(uint64_t(len / sizeof(mqttPayload)));
	      mySensorDataStore->insertBatch(readings, metadataStore->getTTL(msg->getTopic()));
	      readingCtr+= readings.size();

              //mySensorCache.dump();
          } else {
              LOG(error) << "Message with empty topic received";
          }
      }
  }
#endif
  return 0;
}




/*
 * Print usage information
 */
void usage() {
  /*
             1         2         3         4         5         6         7         8
   012345678901234567890123456789012345678901234567890123456789012345678901234567890
   */
  cout << "Usage:" << endl;
  cout << "  collectagent [-d] [-s] [-x] [-a] [-m<host>] [-c<host>] [-u<username>] [-p<password>] [-t<ttl>] [-v<verbosity>] <config>" << endl;
  cout << "  collectagent -h" << endl;
  cout << endl;
  
  cout << "Options:" << endl;
  cout << "  -m<host>      MQTT listen address     [default: " << DEFAULT_LISTENHOST << ":" << DEFAULT_LISTENPORT << "]" << endl;
  cout << "  -c<host>      Cassandra host          [default: " << DEFAULT_CASSANDRAHOST << ":" << DEFAULT_CASSANDRAPORT << "]" << endl;
  cout << "  -u<username>  Cassandra username      [default: none]" << endl;
  cout << "  -p<password>  Cassandra password      [default: none]" << endl;
  cout << "  -t<ttl>       Cassandra insert TTL    [default: " << DEFAULT_CASSANDRATTL << "]" << endl;
  cout << "  -v<level>     Set verbosity of output [default: " << DEFAULT_LOGLEVEL << "]" << endl
       << "                Can be a number between 5 (all) and 0 (fatal)." << endl;
  cout << endl;
  cout << "  -d            Daemonize" << endl;
  cout << "  -s            Print message stats" <<endl;
  cout << "  -x            Parse and print the config but do not actually start collectagent" << endl;
  cout << "  -a			   Enable sensor auto-publish" << endl;
  cout << "  -h            This help page" << endl;
  cout << endl;
}

int main(int argc, char* const argv[]) {
    cout << "CollectAgent " << VERSION << " (libdcdb " << DCDB::Version::getVersion() << ")" << endl << endl;

  try{

      // Checking if path to config is supplied
      if (argc <= 1) {
          cout << "Please specify a path to the config-directory or a config-file" << endl << endl;
          usage();
          exit(EXIT_FAILURE);
      }

      // Defining options
      const char* opts = "m:r:c:C:u:p:t:v:dDsaxh";

      // Same mechanism as in DCDBPusher - checking if help string is requested before loading config
      char ret;
      while ((ret = getopt(argc, argv, opts)) != -1) {
          switch (ret)
          {
              case 'h':
                  usage();
                  exit(EXIT_FAILURE);
                  break;
              default:
                  //do nothing (other options are read later on)
                  break;
          }
      }

      initLogging();
      auto cmdSink = setupCmdLogger();

      Configuration config(argv[argc - 1], "collectagent.conf");
      config.readConfig();
      LOG(info) << "Parse Config";
      // References to shorten access to config parameters
      Configuration& settings = config;
      cassandra_t& cassandraSettings = config.cassandraSettings;
      pluginSettings_t& pluginSettings = config.pluginSettings;
      serverSettings_t& restAPISettings = config.restAPISettings;
      analyticsSettings_t& analyticsSettings = config.analyticsSettings;
      LOG(info) << "check flag";
      optind = 1;
      while ((ret=getopt(argc, argv, opts))!=-1) {
          switch(ret) {
              case 'a':
                  pluginSettings.autoPublish = true;
                  break;
              case 'm':
                  settings.mqttListenHost = parseNetworkHost(optarg);
                  settings.mqttListenPort = parseNetworkPort(optarg);
                  if(settings.mqttListenPort=="") settings.mqttListenPort = string(DEFAULT_LISTENPORT);
                  break;
              case 'c':
                  cassandraSettings.host = parseNetworkHost(optarg);
                  cassandraSettings.port = parseNetworkPort(optarg);
                  if(cassandraSettings.port=="") cassandraSettings.port = string(DEFAULT_CASSANDRAPORT);
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
                  cassandraSettings.ttl = stoul(optarg);
                  break;
              case 'v':
                  settings.logLevelCmd = stoi(optarg);
                  break;
              case 'd':
              case 'D':
                  settings.daemonize = 1;
                  break;
              case 's':
                  settings.statisticsInterval = 1;
                  break;
              case 'x':
                  settings.validateConfig = true;
                  break;
              case 'h':
              default:
                  usage();
                  exit(EXIT_FAILURE);
          }
      }
      LOG(info) << "complete check flag";
      libConfig.init();
      libConfig.setTempDir(pluginSettings.tempdir);

	LOG(info) << "setup logger to file";
      //set up logger to file
      if (settings.logLevelFile >= 0) {
	  auto fileSink = setupFileLogger(pluginSettings.tempdir, std::string("collectagent"));
	  fileSink->set_filter(boost::log::trivial::severity >= translateLogLevel(settings.logLevelFile));
      }
      
	LOG(info) << "globalSettings";
      //severity level may be overwritten (per option or config-file) --> set it according to globalSettings
      if (settings.logLevelCmd >= 0) {
	  cmdSink->set_filter(boost::log::trivial::severity >= translateLogLevel(settings.logLevelCmd));
      }

      /*
       * Catch SIGINT and SIGTERM signals to allow for proper server shutdowns.
       */
      signal(SIGINT, sigHandler);
      signal(SIGTERM, sigHandler);
      signal(SIGUSR1, sigHandler);

      /*
       * Catch critical signals to allow for backtraces
       */
      signal(SIGABRT, abrtHandler);
      signal(SIGSEGV, abrtHandler);
      LOG(info) << "deamonize";
      if(settings.daemonize)
          LOG(info) << "setting deamonize True";
      // Daemonizing the collectagent
      if(settings.daemonize)
          dcdbdaemon();      
      // Setting the size of the sensor cache
      // Conversion from milliseconds to nanoseconds
      mySensorCache.setMaxHistory(uint64_t(pluginSettings.cacheInterval) * 1000000);
      LOG(info) << "allocate and initialize connection to cassandra";
      //Allocate and initialize connection to Cassandra.
      dcdbConn = new DCDB::Connection(cassandraSettings.host, atoi(cassandraSettings.port.c_str()), 
                                      cassandraSettings.username, cassandraSettings.password);
      dcdbConn->setNumThreadsIo(cassandraSettings.numThreadsIo);
      dcdbConn->setQueueSizeIo(cassandraSettings.queueSizeIo);
      uint32_t params[1] = {cassandraSettings.coreConnPerHost};
      dcdbConn->setBackendParams(params);
      
      if (!dcdbConn->connect()) {
          LOG(fatal) << "Cannot connect to Cassandra!";
          exit(EXIT_FAILURE);
      }
      LOG(info) << "connected to cassadra";
      /*
       * Legacy behavior: Initialize the DCDB schema in Cassandra.
       */
      dcdbConn->initSchema();
      
      /*
       * Allocate the SensorDataStore.
       */
      mySensorDataStore = new DCDB::SensorDataStore(dcdbConn);
      mySensorConfig = new DCDB::SensorConfig(dcdbConn);
      myJobDataStore = new DCDB::JobDataStore(dcdbConn);
      myCaliEvtDataStore = new DCDB::CaliEvtDataStore(dcdbConn);

      /*
       * Set TTL for data store inserts if TTL > 0.
       */
      if (cassandraSettings.ttl > 0) {
        mySensorDataStore->setTTL(cassandraSettings.ttl);
        myCaliEvtDataStore->setTTL(cassandraSettings.ttl);
      }
      mySensorDataStore->setDebugLog(cassandraSettings.debugLog);
      myCaliEvtDataStore->setDebugLog(cassandraSettings.debugLog);

      // Fetching public sensor information from the Cassandra datastore
      list<DCDB::PublicSensor> publicSensors;
      if(mySensorConfig->getPublicSensorsVerbose(publicSensors)!=SC_OK) {
	  LOG(error) << "Failed to retrieve public sensors!";
	  exit(EXIT_FAILURE);
      }
      
      metadataStore = new MetadataStore();
      SensorMetadata sBuf;
      for (const auto &s : publicSensors)
          if (!s.is_virtual) {
              sBuf = DCDB::PublicSensor::publicSensorToMetadata(s);
              if(sBuf.isValid())
                  metadataStore->store(*sBuf.getPattern(), sBuf);
          }
      publicSensors.clear();

      boost::asio::io_context io;
      boost::thread_group     threads;

      analyticsController = new AnalyticsController(mySensorConfig, mySensorDataStore, io);
      analyticsController->setCache(&mySensorCache);
      analyticsController->setMetadataStore(metadataStore);
      queryEngine.setFilter(analyticsSettings.filter);
      queryEngine.setJobFilter(analyticsSettings.jobFilter);
      queryEngine.setJobMatch(analyticsSettings.jobMatch);
      queryEngine.setJobIDFilter(analyticsSettings.jobIdFilter);
      queryEngine.setJobDomainId(analyticsSettings.jobDomainId);
      queryEngine.setSensorHierarchy(analyticsSettings.hierarchy);
      queryEngine.setQueryCallback(sensorQueryCallback);
      queryEngine.setGroupQueryCallback(sensorGroupQueryCallback);
      queryEngine.setMetadataQueryCallback(metadataQueryCallback);
      queryEngine.setJobQueryCallback(jobQueryCallback);
      if(!analyticsController->initialize(settings))
          return EXIT_FAILURE;
      
      LOG_LEVEL vLogLevel = settings.validateConfig ? LOG_LEVEL::info : LOG_LEVEL::debug;
      LOG_VAR(vLogLevel) << "-----  Configuration  -----";

      //print global settings in either case
      LOG(info) << "Global Settings:";
      LOG(info) << "    MQTT-listenAddress: " << settings.mqttListenHost << ":" << settings.mqttListenPort;
      LOG(info) << "    CacheInterval:      " << int(pluginSettings.cacheInterval/1000) << " [s]";
      LOG(info) << "    CleaningInterval:   " << settings.cleaningInterval << " [s]";
      LOG(info) << "    Threads:            " << settings.threads;
      LOG(info) << "    MessageThreads:     " << settings.messageThreads;
      LOG(info) << "    MessageSlots:       " << settings.messageSlots;
      LOG(info) << "    Daemonize:          " << (settings.daemonize ? "Enabled" : "Disabled");
      LOG(info) << "    StatisticsInterval: " << settings.statisticsInterval << " [s]";
      LOG(info) << "    StatisticsMqttPart: " << settings.statisticsMqttPart;
      LOG(info) << "    MQTT-prefix:        " << pluginSettings.mqttPrefix;
      LOG(info) << "    Auto-publish:       " << (pluginSettings.autoPublish ? "Enabled" : "Disabled");
      LOG(info) << "    Write-Dir:          " << pluginSettings.tempdir;
      LOG(info) << (settings.validateConfig ? "    Only validating config files." : "    ValidateConfig:     Disabled");

      LOG(info) << "Analytics Settings:";
      LOG(info) << "    Hierarchy:          " << (analyticsSettings.hierarchy!="" ? analyticsSettings.hierarchy : "none");
      LOG(info) << "    Filter:             " << (analyticsSettings.filter!="" ? analyticsSettings.filter : "none");
      LOG(info) << "    Job Filter:         " << (analyticsSettings.jobFilter != "" ? analyticsSettings.jobFilter : "none");
      LOG(info) << "    Job Match:          " << (analyticsSettings.jobMatch != "" ? analyticsSettings.jobMatch : "none");
      LOG(info) << "    Job ID Filter:      " << (analyticsSettings.jobIdFilter != "" ? analyticsSettings.jobIdFilter : "none");
      LOG(info) << "    Job Domain ID:      " << analyticsSettings.jobDomainId;
      
      LOG(info) << "Cassandra Driver Settings:";
      LOG(info) << "    Address:            " << cassandraSettings.host << ":" << cassandraSettings.port;
      LOG(info) << "    TTL:                " << cassandraSettings.ttl;
      LOG(info) << "    NumThreadsIO:       " << cassandraSettings.numThreadsIo;
      LOG(info) << "    QueueSizeIO:        " << cassandraSettings.queueSizeIo;
      LOG(info) << "    CoreConnPerHost:    " << cassandraSettings.coreConnPerHost;
      LOG(info) << "    DebugLog:           " << (cassandraSettings.debugLog ? "Enabled" : "Disabled");
#ifdef SimpleMQTTVerbose
      LOG(info) << "    Username:           " << cassandraSettings.username;
	  LOG(info) << "    Password:           " << cassandraSettings.password;
#else
      LOG(info) << "    Username and password not printed.";
#endif

      if (restAPISettings.enabled) {
	  
	  LOG(info) << "RestAPI Settings:";
	  LOG(info) << "    REST Server: " << restAPISettings.host << ":" << restAPISettings.port;
	  LOG(info) << "    Certificate: " << restAPISettings.certificate;
	  LOG(info) << "    Private key file: " << restAPISettings.privateKey;
	  
	  if (config.influxSettings.measurements.size() > 0) {
	      LOG(info) << "InfluxDB Settings:";
	      LOG(info) << "    MQTT-Prefix:  " << config.influxSettings.mqttPrefix;
	      LOG(info) << "    Auto-Publish: " << (config.influxSettings.publish ? "Enabled" : "Disabled");

	      for (auto &m: config.influxSettings.measurements) {
		  LOG(info) << "    Measurement: " << m.first;
		  LOG(info) << "        MQTT-Part:   " << m.second.mqttPart;
		  LOG(info) << "        Tag:         " << m.second.tag;
		  if ((m.second.tagRegex.size() > 0) && (m.second.tagSubstitution.size() > 0)) {
		      if (m.second.tagSubstitution != "&") {
			  LOG(info) << "        TagFilter:   s/" << m.second.tagRegex.str() << "/" <<  m.second.tagSubstitution << "/";
		      } else {
			  LOG(info) << "    TagFilter:   " << m.second.tagRegex.str();
		      }
		  }
		  if (m.second.fields.size() > 0) {
		      stringstream ss;
		      copy(m.second.fields.begin(), m.second.fields.end(), ostream_iterator<std::string>(ss, ","));
		      string fields = ss.str();
		      fields.pop_back();
		      LOG(info) << "        Fields:      " << fields;
		  }
	      }
	  }
      }
      LOG_VAR(vLogLevel) << "-----  Analytics Configuration  -----";
      for(auto& p : analyticsController->getManager()->getPlugins()) {
          LOG_VAR(vLogLevel) << "Operator Plugin \"" << p.id << "\"";
          p.configurator->printConfig(vLogLevel);
      }
      LOG_VAR(vLogLevel) << "-----  End Configuration  -----";

      if (settings.validateConfig)
          return EXIT_SUCCESS;
      
      LOG(info) << "Creating threads...";
      // Dummy to keep io service alive even if no tasks remain (e.g. because all sensors have been stopped over REST API)
      // Inherited from DCDB Pusher
      keepAliveWork = boost::make_shared<boost::asio::io_context::work>(io);
      // Create pool of threads which handle the sensors
      for(size_t i = 0; i < settings.threads; i++) {
	  threads.create_thread(bind(static_cast< size_t (boost::asio::io_context::*) () >(&boost::asio::io_context::run), &io));
      }
      LOG(info) << "Threads created!";
      
      analyticsController->start();
      LOG(info) << "AnalyticsController running...";
      
      /*
       * Start the MQTT Message Server.
       */
      SimpleMQTTServer ms(settings.mqttListenHost, settings.mqttListenPort, settings.messageThreads, settings.messageSlots);
      
      ms.setMessageCallback(mqttCallback);
      ms.start();

      LOG(info) << "MQTT Server running...";
      
      /*
       * Start the HTTP Server for the REST API
       */
      if (restAPISettings.enabled) {
          httpsServer = new CARestAPI(restAPISettings, &config.influxSettings, &mySensorCache, mySensorDataStore, mySensorConfig, analyticsController, &ms, io);
          config.readRestAPIUsers(httpsServer);
          httpsServer->start();
          LOG(info) <<  "HTTP Server running...";
      }

      /*
       * Run (hopefully) forever...
       */
      newAutoPub = false;
      keepRunning = 1;
      uint64_t start, end;
      float elapsed;
      msgCtr = 0;
      readingCtr = 0;
      dbQueryCtr = 0;
      cachedQueryCtr = 0;
      missesQueryCtr = 0;

      start = getTimestamp();
      uint64_t lastCleanup = start;
      uint64_t sleepInterval = (settings.statisticsInterval > 0) ? settings.statisticsInterval : 60;

      LOG(info) << "Collect Agent running...";
      while(keepRunning) {
	  start = getTimestamp();
	  if(NS_TO_S(start) - NS_TO_S(lastCleanup) > settings.cleaningInterval) {
	      uint64_t purged = mySensorCache.clean(S_TO_NS(settings.cleaningInterval));
	      lastCleanup = start;
	      if(purged > 0)
		  LOG(info) << "Cache: purged " << purged << " obsolete entries";
	  }
	  if(newAutoPub) {
	      newAutoPub = false;
	      mySensorConfig->setPublishedSensorsWritetime(getTimestamp());
	  }
	  
	  sleep(sleepInterval);
	  
	  if((settings.statisticsInterval > 0) && keepRunning) {
	      /* not really thread safe but will do the job */
	      end = getTimestamp();
	      elapsed = (float)(NS_TO_S(end) - NS_TO_S(start));
	      float aIns = ceil(((float)analyticsController->getReadingCtr()) / elapsed);
	      float cacheReq = ceil(((float)cachedQueryCtr) / elapsed);
	      float missesReq = ceil(((float)missesQueryCtr) / elapsed);
	      float dbReq = ceil(((float)dbQueryCtr) / elapsed);
	      float rIns = restAPISettings.enabled ? ceil(((float)httpsServer->getInfluxCounter()) / elapsed) : 0.0f;
	      float mIns = ceil(((float)readingCtr) / elapsed);
	      float mMsg = ceil(((float) msgCtr) / elapsed);
	      LOG(info) << "Performance: MQTT [" << std::fixed << std::setprecision(0) << mIns << " ins/s|" << mMsg << " msg/s]   REST [" << rIns << " ins/s]   Analytics [" << aIns << " ins/s]   Cache [" << cacheReq << " req/s]   DB [" << dbReq << " req/s] Miss [" << missesReq << " req/s]";
	      std::map<std::string, hostInfo_t> lastSeen = ms.collectLastSeen();
	      uint64_t connectedHosts = 0;
	      for (auto h: lastSeen) {
		  if (h.second.lastSeen >= end - S_TO_NS(settings.statisticsInterval)) {
		      connectedHosts++;
		  }
	      }
	      LOG(info) << "Connected hosts: " << connectedHosts;
	      
	      if (settings.statisticsMqttPart.size() > 0) {
		  std::string statisticsMqttTopic = pluginSettings.mqttPrefix + settings.statisticsMqttPart;
		  std::list<SensorDataStoreReading> stats;
		  stats.push_back(SensorDataStoreReading(SensorId(statisticsMqttTopic+"/msgsRcvd"), end, msgCtr));
		  stats.push_back(SensorDataStoreReading(SensorId(statisticsMqttTopic+"/cachedQueries"), end, cachedQueryCtr));
		  stats.push_back(SensorDataStoreReading(SensorId(statisticsMqttTopic+"/missedQueries"), end, missesQueryCtr));
		  stats.push_back(SensorDataStoreReading(SensorId(statisticsMqttTopic+"/dbQueries"), end, dbQueryCtr));
		  stats.push_back(SensorDataStoreReading(SensorId(statisticsMqttTopic+"/readingsRcvd"), end, readingCtr));
		  stats.push_back(SensorDataStoreReading(SensorId(statisticsMqttTopic+"/hosts"), end, connectedHosts));
		  for (auto s: stats) {
		      mySensorDataStore->insert(s);
		      mySensorCache.storeSensor(s);
		  }
	      }
	      
	      msgCtr = 0;
	      cachedQueryCtr = 0;
	      missesQueryCtr = 0;
	      dbQueryCtr = 0;
	      readingCtr = 0;
	  }
      }

      LOG(info) << "Stopping...";
      ms.stop();
      LOG(info) << "MQTT Server stopped...";
      if (restAPISettings.enabled) { 
          httpsServer->stop();
          LOG(info) << "HTTP Server stopped...";
      }
      analyticsController->stop();
      delete mySensorDataStore;
      delete myJobDataStore;
      delete mySensorConfig;
      delete myCaliEvtDataStore;
      dcdbConn->disconnect();
      delete dcdbConn;
      delete metadataStore;
      delete analyticsController;
      if (restAPISettings.enabled) {
          delete httpsServer;
      }
      LOG(info) << "Collect Agent closed. Bye bye...";
  }
  catch (const std::runtime_error& e) {
      LOG(fatal) <<  e.what();
      return EXIT_FAILURE;
  }
  catch (const exception& e) {
      LOG(fatal) << "Exception: " << e.what();
      abrt(EXIT_FAILURE, INTERR);
  }

  return retCode;
}


