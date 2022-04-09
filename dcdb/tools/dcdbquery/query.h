//================================================================================
// Name        : query.h
// Author      : Axel Auweter, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header of query class of dcdbquery
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


#include <list>
#include <string>
#include <map>

#include "dcdb/connection.h"
#include "dcdb/timestamp.h"
#include "dcdb/sensorid.h"
#include "dcdb/sensordatastore.h"
#include "dcdb/sensorconfig.h"
#include "dcdb/unitconv.h"
#include "dcdb/virtualsensor.h"
#include "dcdb/sensor.h"

#ifndef QUERY_H
#define QUERY_H

class DCDBQuery
{

typedef enum {
    DCDB_OP_NONE,
    DCDB_OP_DELTA,
    DCDB_OP_DELTAT,
    DCDB_OP_DERIVATIVE,
    DCDB_OP_INTEGRAL,
    DCDB_OP_RATE,
    DCDB_OP_WINTERMUTE,
    DCDB_OP_UNKNOWN,
} DCDB_OP_TYPE;
    
typedef struct queryConfig {
    double scalingFactor;
    DCDB::Unit unit;
    DCDB_OP_TYPE operation;
    DCDB::QueryAggregate aggregate;
    std::string wintermuteOp;
} queryConfig_t;
typedef std::multimap<DCDB::PublicSensor, queryConfig_t> queryMap_t;

typedef enum {
    CONVERT_OK,
    CONVERT_ERR,
} CONVERT_RESULT;
    
private:
    void genOutput(std::list<DCDB::SensorDataStoreReading> &results, queryMap_t::iterator start, queryMap_t::iterator stop);
    void setInterval(DCDB::TimeStamp start, DCDB::TimeStamp end);
    void parseSensorSpecification(const std::string sensor, std::string& sensorName, queryConfig_t& queryCfg);
    void prepareQuery(std::list<std::string> sensors);
    void prepareQuery(std::list<std::string> sensors, std::list<std::string> prefixes);
    void execute();


  DCDB::Connection* connection;
  bool useLocalTime;
  bool useRawOutput;

    queryMap_t queries;
    double baseScalingFactor;
    DCDB::Unit baseUnit;
    DCDB::TimeStamp start_ts;
    DCDB::TimeStamp end_ts;

public:
  void setLocalTimeEnabled(bool enable);
  bool getLocalTimeEnabled();
  void setRawOutputEnabled(bool enable);
  bool getRawOutputEnabled();

  int connect(const char* hostname);
  void disconnect();
  void doQuery(std::list<std::string> sensors, DCDB::TimeStamp start, DCDB::TimeStamp end);
  void dojobQuery(std::list<std::string> sensors, std::string jobId);


  DCDBQuery();
  virtual ~DCDBQuery() {};

};


#endif /* QUERY_H */
