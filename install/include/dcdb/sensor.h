//================================================================================
// Name        : sensor.h
// Author      : Michael Ott, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Sensor class.
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

/**
 * @file sensor.h
 *
 * @ingroup libdcdb
 */

#ifndef LIB_INCLUDE_DCDB_SENSOR_H_
#define LIB_INCLUDE_DCDB_SENSOR_H_

#include <string>
#include <list>
#include "dcdb/sensordatastore.h"
#include "dcdb/sensorconfig.h"
#include "dcdb/timestamp.h"

namespace DCDB {
  
  class Sensor {
    public:
      Sensor(DCDB::Connection* connection, const std::string& publicName);
      Sensor(Connection* connection, const PublicSensor& sensor);
      virtual ~Sensor();
      void query(std::list<SensorDataStoreReading>& reading, TimeStamp& start, TimeStamp& end, QueryAggregate aggregate = AGGREGATE_NONE, uint64_t tol_ns=3600000000000);

    private:
      Connection* connection;
      PublicSensor publicSensor;
      SensorConfig* sensorConfig;
      
  };

} /* namespace DCDB */

#endif /* LIB_INCLUDE_DCDB_SENSOR_H_ */
