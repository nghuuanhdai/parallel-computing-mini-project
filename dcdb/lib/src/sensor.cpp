//================================================================================
// Name        : sensor.cpp
// Author      : Michael Ott, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Sensor class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2017-2019 Leibniz Supercomputing Centre
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//================================================================================

#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include <dcdb/sensor.h>
#include <dcdb/virtualsensor.h>

namespace DCDB {

  Sensor::Sensor(Connection* connection, const std::string& publicName) {
    /* Initialize the SensorConfig interface */
    sensorConfig = new SensorConfig(connection);

    this->connection = connection;
      
    /* Retrieve publicSensor info */
    switch (sensorConfig->getPublicSensorByName(publicSensor, publicName.c_str())) {
    case DCDB::SC_OK:
      break;
    case DCDB::SC_INVALIDSESSION:
      std::cout << "Invalid session." << std::endl;
      return;
    case DCDB::SC_UNKNOWNSENSOR:
      std::cout << "Unknown sensor: " << publicName << std::endl;
      return;
    default:
      std::cout << "Unknown error." << std::endl;
      return;
    }
  }
  
  Sensor::Sensor(Connection* connection, const PublicSensor& sensor) {
    /* Initialize the SensorConfig interface */
    sensorConfig = new SensorConfig(connection);
    
    this->connection = connection;
    publicSensor = sensor;
  }

  Sensor::~Sensor() {
    delete sensorConfig;
  }

  void Sensor::query(std::list<SensorDataStoreReading>& result, TimeStamp& start, TimeStamp& end, QueryAggregate aggregate, uint64_t tol_ns) {
    SensorDataStore sensorDataStore(connection);

    if (publicSensor.is_virtual) {
        VSensor vSen(connection, publicSensor);
        vSen.query(result, start, end);
	if ((result.size() > 0) && (aggregate != AGGREGATE_NONE)) {
	    switch(aggregate) {
		case AGGREGATE_MIN:
		    result.begin()->value = std::min_element(result.begin(), result.end())->value;;
		    break;
		case AGGREGATE_MAX:
		    result.begin()->value = std::max_element(result.begin(), result.end())->value;;
		    break;
		case AGGREGATE_AVG: {
		    int64_t sum = 0;
		    for (auto r: result) {
			sum+= r.value;
		    }
		    result.begin()->value = sum / result.size();
		    break; }
		case AGGREGATE_SUM: {
		    int64_t sum = 0;
		    for (auto r: result) {
			sum+= r.value;
		    }
		    result.begin()->value = sum;
		    break;}
		case AGGREGATE_COUNT:
		    result.begin()->value = result.size();
		    break;
		default:
		    std::cout << "Unknown aggregate " << aggregate << " in Sensor::query()" << std::endl;
		    break;
	    }
	    result.erase(std::next(result.begin()), result.end());
	}
    }
    else {
        /* Iterate over the sensorIds within the query interval and output the results in CSV format */
        if(start.getRaw() != end.getRaw()) {
	    uint16_t wsStart = start.getWeekstamp();
	    uint16_t wsEnd   = end.getWeekstamp();
	    
	    SensorId sid(publicSensor.name);
	    for(uint16_t ws=wsStart; ws<=wsEnd; ws++) {
		sid.setRsvd(ws);
                sensorDataStore.query(result, sid, start, end, aggregate);
            }
        } else {
	    SensorId sid(publicSensor.name);
	    sid.setRsvd(start.getWeekstamp());
            sensorDataStore.fuzzyQuery(result, sid, start, tol_ns);
        }
    }
  }
  
} /* namespace DCDB */
