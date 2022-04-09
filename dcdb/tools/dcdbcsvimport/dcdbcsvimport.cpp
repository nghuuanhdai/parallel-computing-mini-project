//================================================================================
// Name        : dcdbcsvimport.cpp
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Command line utility for importing CSV data into DCDB
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

#include <iostream>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <iomanip> 
#include <sstream> 
#include <vector> 
#include <boost/tokenizer.hpp>
#include <algorithm>
#include <exception>
#include <set>
#include <map>
#include <string>


#include <pthread.h>

#include <dcdb/connection.h>
#include <dcdb/sensorconfig.h>
#include <dcdb/sensordatastore.h>
#include <dcdb/sensorid.h>
#include <dcdb/timestamp.h>
#include <dcdb/c_api.h>
#include "dcdb/version.h"
#include "version.h"

int verbose = 0;

typedef struct {
  std::string name;
  std::string topic;
  std::string publicName;
  uint64_t count;
  uint64_t prev;
} sensor_t;

void usage(int argc, char* argv[])
{
  std::cout << "Usage: " << argv[0] << " [-h <host>] [-t <col>] [-c <col[,col,col]>] <CSV File> <SensorPrefix>" << std::endl << std::endl;
  std::cout << "    -h <host>           - Database hostname" << std::endl;
  std::cout << "    -t <col>            - Column in the CSV that contains the timestamp [default: 0]" << std::endl;
  std::cout << "    -n <col>            - Sensor name column" << std::endl;
  std::cout << "    -c <col[,col,col]>  - Column in the CSV to use [default: all]" << std::endl;
  std::cout << "    -d                  - Drop constant values" << std::endl;
  std::cout << "    -s <offset>         - MQTT suffix start value [default: 0]" << std::endl;
  std::cout << "    -p                  - Publish sensors" << std::endl;

  std::cout << "    CSV File            - CSV file with sensor readings. First row has to contain sensor names" << std::endl;
  std::cout << "    MQTTPrefix          - MQTT prefix to use for sensors" << std::endl;
}

sensor_t* createSensor(DCDB::SensorConfig& sensorConfig, const std::string& name, const std::string& prefix, int& suffix) {
  sensor_t* sensor = new sensor_t;
  sensor->name = name;
  sensor->count = 0;

  DCDB::PublicSensor psensor;
  if (sensorConfig.getPublicSensorByName(psensor, name.c_str()) == DCDB::SC_OK) {
    if (verbose) {
      std::cout << "Found " << name << " in database: " << psensor.pattern << std::endl;
    }
    sensor->topic = psensor.pattern;
    sensor->publicName = psensor.name;
  } else {

    sensor->topic = prefix + "/" + name;
    sensor->publicName = name;
    std::replace(sensor->publicName.begin(), sensor->publicName.end(), ' ', '_');
    
    if (verbose) {
      std::cout << "Created new sensor " << sensor->publicName << " in database: " << sensor->topic << std::endl;
    }
  }
  return sensor;
}

int main(int argc, char** argv)
{
  std::cout << "dcdbcsvimport " << VERSION << " (libdcdb " << DCDB::Version::getVersion() << ")" << std::endl << std::endl;
  /* Check command line parameters */
  if (argc < 3) {
      usage(argc, argv);
      exit(EXIT_FAILURE);
  }

  int ret;
  const char *host = getenv("DCDB_HOSTNAME");
  if (!host) {
      host = "localhost";
  }

  int suffixStart = 0;
  int tsColumn = 0;
  std::set<int> columns;
  bool dropConstantValues = false;
  bool publish = false;
  int sensorNameColumn = -1;
  while ((ret=getopt(argc, argv, "+h:t:n:c:ds:pv:"))!=-1) {
    switch(ret) {
      case 'h':
	host = optarg;
	break;
      case 't':
	tsColumn = atoi(optarg);
	break;
      case 'n':
	sensorNameColumn = atoi(optarg);
	break;
      case 'c': {
	std::string s(optarg);
	boost::tokenizer<boost::escaped_list_separator<char> > tk(s, boost::escaped_list_separator<char>('\\', ',', '\"'));
	for (boost::tokenizer<boost::escaped_list_separator<char> >::iterator i=tk.begin(); i!=tk.end();++i)
	  columns.insert(std::stoi(*i));
      } break;
      case 'd':
	dropConstantValues = true;
	break;
      case 's':
	suffixStart = atoi(optarg);
	std::cout << ret << ": " << optarg << std::endl;
	break;
      case 'p':
	publish = true;
	break;
      case 'v':
	if (optarg) {
	  verbose = atoi(optarg);
        } else {
	  verbose = 1;
        }
	break;
      default:
	usage(argc, argv);
	std::cout << "Unknown parameter: " << (char) ret << std::endl;
	exit(EXIT_FAILURE);
    }
  }
  
  std::string csvFilename = argv[optind];
  std::string prefix = argv[optind+1];

  columns.erase(tsColumn);
  if (sensorNameColumn > -1) {
    columns.erase(sensorNameColumn);
  }

  /* Connect to the data store */
  std::cout << std::endl;
  std::cout << "Connecting to the data store..." << std::endl;

  DCDB::Connection* connection;
  connection = new DCDB::Connection();
  connection->setHostname(host);
  connection->setNumThreadsIo(4);
  connection->setQueueSizeIo(256*1024);
  
  if (!connection->connect()) {
      std::cout << "Cannot connect to database." << std::endl;
      return 1;
  }

  /* Initialize the SensorConfig and SensorDataStore interfaces */
  DCDB::SensorConfig sensorConfig(connection);
  sensorConfig.loadCache();
  DCDB::SensorDataStore sensorDataStore(connection);
  sensorDataStore.setDebugLog(true);

  std::ifstream fs;
  std::string s;
  uint64_t lineno = 0;

  /* Read header line from CSV to obtain sensor names and topics */
  std::cout << std::endl;
  std::cout << "Parsing CSV file: " << csvFilename << std::endl;
  std::cout << "Using MQTT prefix: " << prefix << std::endl;
  std::cout << "Columns:";
  if (columns.size()) {
	  std::set<int>::iterator it;
	  for (it=columns.begin(); it!=columns.end(); ++it) {
	      std::cout << " " << *it;
	  }
	  std::cout << std::endl;
  } else {
	  std::cout << "all" << std::endl;
  }
  std::cout << "Timestamp Column: " << tsColumn << std::endl;
  if (sensorNameColumn > -1) {
    std::cout << "Sensorname Column: " << sensorNameColumn << std::endl;
  }

  fs.open (csvFilename, std::fstream::in);

  int col = 0;
  int topics = suffixStart;
  std::map<int,sensor_t*> sensorsByCol;
  std::map<std::string,sensor_t*> sensorsByName;
  sensor_t* sensor = nullptr;
  if (sensorNameColumn == -1) {
    std::getline(fs, s);
    lineno++;
    boost::tokenizer<boost::escaped_list_separator<char> > tk(s, boost::escaped_list_separator<char>('\\', ',', '\"'));
    for (boost::tokenizer<boost::escaped_list_separator<char> >::iterator i=tk.begin(); i!=tk.end();++i)
    {
      if (col != tsColumn) {
	sensor = createSensor(sensorConfig, *i, prefix, topics);
	sensorsByCol.insert(std::pair<int,sensor_t*>(col, sensor));
      }
      col++;
    }
  }

  /* Read actual sensor readings */
  uint64_t count = 0;
  uint64_t total = 0;
  time_t t0 = time(NULL);
  while (std::getline(fs, s)) {
    lineno++;

    col = 0;
    DCDB::TimeStamp ts(UINT64_C(0));
    boost::tokenizer<boost::escaped_list_separator<char> > tk(s, boost::escaped_list_separator<char>('\\', ',', '\"'));
    for (boost::tokenizer<boost::escaped_list_separator<char> >::iterator i=tk.begin(); i!=tk.end();++i) {
      if (col == tsColumn) {
	try {
	  ts = DCDB::TimeStamp(*i);
	}
	catch (std::exception &e) {
	  if (verbose > 1) {
	    std::cerr << "Error parsing timestamp " << *i << std::endl;
	  }
	}
      } else if (col == sensorNameColumn) {
	std::map<std::string,sensor_t*>::iterator it = sensorsByName.find(*i);
	if (it != sensorsByName.end()) {
	  sensor = it->second ;
	} else {
	  sensor = createSensor(sensorConfig, *i, prefix, topics);
	  sensorsByName.insert(std::pair<std::string,sensor_t*>(sensor->name, sensor));
	}
      }
      col++;
    }
    
    if (ts.getRaw() == 0) {
      continue;
    }

    col = 0;
    for (boost::tokenizer<boost::escaped_list_separator<char> >::iterator i=tk.begin(); i!=tk.end();++i)
    {
      if (((columns.size() == 0) || (columns.find(col) != columns.end()))) {
	if (sensorNameColumn == -1) {
	  sensor = sensorsByCol[col];
	}
	if (verbose >= 2) {
	  std::cout << ts.getRaw() << " " << col << " " << sensor->name << " " << sensor->topic << " " << *i << std::endl;
	}
	try {
	  DCDB::SensorId sid(sensor->topic);
	  uint64_t val = std::stoll(*i);
	  if (!dropConstantValues || val != sensor->prev) {
	    sensorDataStore.insert(sid, ts.getRaw(), val);
	    sensor->count++;
	    sensor->prev = val;
	    count++;
	  }
	}
	catch (std::exception &e) {
	  if (verbose > 1) {
	    std::cerr << "Error parsing CSV line " << lineno << " column " << col+1 << ": \"" << s << "\"" << std::endl;
	  }
	}
	total++;
	if (total % 1000 == 0) {
	  time_t t1 = time(NULL);
	  if (t1 != t0) {
	    std::cout << total << " " << count/(t1-t0) << " inserts/s" << std::endl;
	    t0 = t1;
	    count = 0;
	  }
	}
      }
      col++;
    }
  }
  
  std::cout << "Inserted " << total << " readings" << std::endl;
  
  fs.close();

  /* Create public sensor names */
  if (publish) {
    std::cout << std::endl;
    std::cout << "Publishing sensors..." << std::endl;
    
    if (sensorNameColumn > -1) {
      std::map<std::string,sensor_t*>::iterator it;
      for (it = sensorsByName.begin(); it != sensorsByName.end(); it++) {
	sensor_t* sensor = it->second;
	std::cout << sensor->name  << " " << sensor->topic << " " << sensor->publicName << " (" << sensor->count << " inserts)" << std::endl;
	sensorConfig.publishSensor(sensor->publicName.c_str(), sensor->topic.c_str());
      }
    } else {
      std::map<int,sensor_t*>::iterator it;
      for (it = sensorsByCol.begin(); it != sensorsByCol.end(); it++) {
	sensor_t* sensor = it->second;
	std::cout << sensor->name  << " " << sensor->topic << " " << sensor->publicName << " (" << sensor->count << " inserts)" << std::endl;
	sensorConfig.publishSensor(sensor->publicName.c_str(), sensor->topic.c_str());
      }
    }
  }

  /* Disconnect */
  delete connection;

  return 0;
}

