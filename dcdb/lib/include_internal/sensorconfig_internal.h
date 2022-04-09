//================================================================================
// Name        : sensorconfig_internal.h
// Author      : Axel Auweter, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Internal Interface for configuring libdcdb public sensors.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2019 Leibniz Supercomputing Centre
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

#include <list>
#include <string>
#include <unordered_map>
#include <functional>
#include <unistd.h>
#include <sys/stat.h>

#include "cassandra.h"

#include "dcdb/connection.h"
#include "dcdb/timestamp.h"
#include "dcdb/sensorid.h"

#include "dcdb/sensorconfig.h"
#include "dcdb/libconfig.h"

#ifndef DCDB_SENSORCONFIG_INTERNAL_H
#define DCDB_SENSORCONFIG_INTERNAL_H

namespace DCDB {

class SensorConfigImpl
{
protected:
  Connection* connection;
  CassSession* session;

  typedef std::unordered_map<std::string, PublicSensor> SensorMap_t;
  SensorMap_t sensorMapByName;
  std::list<std::string> sensorList;
  std::string _clusterName;
  bool _useCache;
  std::string _sensorCacheFile;

  bool validateSensorPattern(const char* sensorPattern);
  bool validateSensorPublicName(std::string publicName);
  int acquireCacheLock(const bool write);
  int releaseCacheLock(const int fd);

public:
  SCError loadCache();
  SCError publishSensor(std::string publicName, std::string sensorPattern);
  SCError publishSensor(const PublicSensor& sensor);
  SCError publishSensor(const SensorMetadata& sensor);
  SCError publishVirtualSensor(std::string publicName, std::string vSensorExpression, std::string vSensorId, TimeStamp tZero, uint64_t interval);
  SCError unPublishSensor(std::string publicName);
  SCError unPublishSensorsByWildcard(std::string wildcard);
  SCError getPublicSensorNames(std::list<std::string>& publicSensors);
  SCError getPublicSensorsVerbose(std::list<PublicSensor>& publicSensors);
  
  SCError getPublicSensorByName(PublicSensor& sensor, const char* publicName);
  SCError getPublicSensorsByWildcard(std::list<PublicSensor>& sensors, const char* wildcard);
    
  SCError isVirtual(bool& isVirtual, std::string publicName);

  SCError setSensorScalingFactor(std::string publicName, double scalingFactor);
  SCError setSensorUnit(std::string publicName, std::string unit);
  SCError setSensorMask(std::string publicName, uint64_t mask);
  SCError setOperations(std::string publicName, std::set<std::string> operations);
  SCError clearOperations(std::string publicName);
  SCError clearOperationsByWildcard(std::string wildcard);
  SCError setTimeToLive(std::string publicName, uint64_t ttl);
  SCError setSensorInterval(std::string publicName, uint64_t interval);

  SCError setVirtualSensorExpression(std::string publicName, std::string expression);
  SCError setVirtualSensorTZero(std::string publicName, TimeStamp tZero);

  SCError getPublishedSensorsWritetime(uint64_t &ts);
  SCError setPublishedSensorsWritetime(const uint64_t &ts);

  SCError getClusterName(std::string &name);
  SCError isSensorCacheValid(const bool names, bool& isValid, uint64_t& entries);
  SCError findSensorCachePath();
  SCError saveNamesToFile(const std::list<std::string>& publicSensors);
  SCError saveMetadataToFile(const std::list<PublicSensor>& publicSensors);
  SCError loadNamesFromFile(std::list<std::string>& publicSensors);
  SCError loadMetadataFromFile(std::list<PublicSensor>& publicSensors);
  
  SensorConfigImpl(Connection* conn);
  virtual ~SensorConfigImpl();
};

} /* End of namespace DCDB */

#endif /* DCDB_SENSORCONFIG_INTERNAL_H */
