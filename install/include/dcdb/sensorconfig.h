//================================================================================
// Name        : sensorconfig.h
// Author      : Axel Auweter, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for configuring libdcdb public sensors.
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


/**
 * @file sensorconfig.h
 * @brief This file contains parts of the public API for the
 * libdcdb library.
 * It contains the class definition of the SensorConfig class,
 * that handles sensor configuration and initialization.
 *
 * @ingroup libdcdb
 */

#include <string>
#include <list>
#include <set>
#include <fstream>
#include <fcntl.h>

#include "connection.h"
#include "timestamp.h"
#include "sensorid.h"
#include "metadatastore.h"

#include "cassandra.h"

#ifndef DCDB_SENSORCONFIG_H
#define DCDB_SENSORCONFIG_H

#define MAX_PATTERN_LENGTH 64

#define SENSOR_CACHE_FILENAME "dcdb_sensor_cache_"

namespace DCDB {

/* Forward-declaration of the implementation-internal classes */
class SensorConfigImpl;

/**
 * This class is a container for the information DCDB keeps about public sensors.
 */
class PublicSensor
{
public:
  std::string name;                       /**< The public sensor's (public) name. */
  bool        is_virtual;                 /**< Denotes whether the sensor is a virtual sensor. */
  std::string pattern;                    /**< For non-virtual sensors, this holds a pattern describing the (internal) sensor IDs to which this public sensor matches. */
  double      scaling_factor;             /**< Scaling factor for every sensor reading */
  std::string unit;                       /**< Describes the unit of the sensor. See unitconv.h for known units. */
  uint64_t    sensor_mask;                /**< Determines the properties of the sensor. Currently defined are: integrable, monotonic, delta. */
  std::string expression;                 /**< For virtual sensors, this field holds the expression through which the virtual sensor's value is calculated. */
  std::string v_sensorid;                 /**< For virtual sensors, this field holds a SensorID used for storing cached values in the database. (FIXME: Cache to be implemented) */
  uint64_t    t_zero;                     /**< For virtual sensors, this field holds the first point in time at which the sensor carries a value. */
  uint64_t    interval;                   /**< This field holds the interval at which the sensor evaluates (in nanoseconds). */
  std::set<std::string> operations;       /**< Defines the operations on the sensor, e.g. avg, std deviation, etc. */
  uint64_t    ttl;                        /**< Defines the time to live (in nanoseconds) for the readings of this sensor. */
    
  PublicSensor();
  PublicSensor(const PublicSensor &copy);
    
  inline bool operator <  (const PublicSensor& rhs) const {return name <  rhs.name;}

  /**
  * @brief               Converts a SensorMetadata object to a DCDB::PublicSensor one.
  * 
  * @param s             SensorMetadata object to be converted
  * @return              Output PublicSensor object
  */
  static PublicSensor metadataToPublicSensor(const SensorMetadata& s);

  /**
  * @brief               Converts a DCDB::PublicSensor object to its SensorMetadata representation.
  * 
  * @param ps            The PublicSensor object to be converted
  * @return              Output SensorMetadata object
  */
  static SensorMetadata publicSensorToMetadata(const PublicSensor& ps);
};

/**
 * Enum type for representing the outcome of a DCDB SensorConfig API operation.
 */
typedef enum {
  SC_OK,                /**< Everything went fine. */
  SC_INVALIDSESSION,    /**< The session / database connection is invalid */
  SC_INVALIDPATTERN,    /**< The supplied SensorID pattern is invalid */
  SC_INVALIDPUBLICNAME, /**< The specified public name is invalid */
  SC_INVALIDEXPRESSION, /**< The specified virtual sensor expression is invalid */
  SC_EXPRESSIONSELFREF, /**< The specified virtual sensor references itself in the expression */
  SC_INVALIDVSENSORID,  /**< The virtual SensorID is invalid */
  SC_WRONGTYPE,         /**< You requested an operation for virtual sensors on a physical sensor or vice versa */
  SC_UNKNOWNSENSOR,     /**< The specified sensor is not known */
  SC_OBSOLETECACHE,     /**< Sensor cache is no longer valid */
  SC_CACHEERROR,        /**< Some error while reading the sensor cache occurred */
  SC_PATHERROR,         /**< Path-related error, likely due to permissions */
  SC_UNKNOWNERROR       /**< An unknown error occurred */
} SCError;

#define INTEGRABLE 1
#define MONOTONIC  2
#define DELTA      4

/**
 * This class holds all functions to create/delete/modify the configuration of (virtual and non-virtual) public sensors in DCDB.
 */
class SensorConfig
{
protected:
  SensorConfigImpl* impl;

public:
  /**
   * @brief Load all published sensors into cache.
   *
   * @return               See SCError.
   */
  SCError loadCache();

  /**
   * @brief Makes a physical sensor public.
   *
   * @param publicName     Name under which the sensor becomes publicly available
   * @param sensorPattern  Pattern which describes the SensorIDs to which this sensor matches
   * @return               See SCError.
   */
  SCError publishSensor(const char* publicName, const char* sensorPattern);

   /**
   * @brief Makes a physical sensor public, and publish its metadata as well.
   *
   * @param sensor         Sensor to be published.
   * @return               See SCError.
   */
    SCError publishSensor(const PublicSensor& sensor);


    /**
    * @brief Makes a physical sensor public, and publish its metadata as well.
    *
    *                       This variant of the method takes as input a SensorMetadata object. All fields which are not
    *                       set (null) will not be published.
    * 
    * @param sensor         SensorMetadata object containing metadata to be published.
    * @return               See SCError.
    */
    SCError publishSensor(const SensorMetadata& sensor);

  /**
   * @brief Creates a new virtual sensor.
   *
   * @param publicName          Name under which the sensor becomes publicly available.
   * @param vSensorExpression   Arithmetic expression describing how to evaluate the virtual sensor.
   * @param vSensorId           SensorID under which previously evaluated values may be cached (FIXME: cache to be implemented).
   * @param tZero               Point in time at which the sensor evaluates for the first time.
   * @param interval           Interval at which the sensor will evaluate (starting from tZero) in nanoseconds.
   * @return                    See SCError.
   */
  SCError publishVirtualSensor(const char* publicName, const char* vSensorExpression, const char * vSensorId, TimeStamp tZero, uint64_t interval);

  /**
   * @brief Removes a (virtual or non-virtual) sensor from the list of public sensors.
   *
   * @param publicName          Name under which the sensor becomes publicly available
   * @return                    See SCError.
   */
  SCError unPublishSensor(const char* publicName);

  /**
   * @brief Removes one or more sensors from the list of public sensors using a wildcard.
   *
   * @param wildcard            Wildcard used to identify the sensors to be removed
   * @return                    See SCError.
   */
  SCError unPublishSensorsByWildcard(const char* wildcard);

  /**
   * @brief Get the entire list of (virtual or non-virtual) public sensors.
   *
   * @param publicSensors       Reference to a list of strings that will be populated with the sensor names.
   * @return                    See SCError.
   */
  SCError getPublicSensorNames(std::list<std::string>& publicSensors);

  /**
   * @brief Get the entire list of (virtual and non-virtual) public sensors including their definition.
   *
   * @param publicSensors       Reference to a list of PublicSensor that will be populated with the sensors' definition.
   * @return                    See SCError.
   */
  SCError getPublicSensorsVerbose(std::list<PublicSensor>& publicSensors);

  /**
   * @brief Retrieve a public sensor by name.
   *
   * @param sensor              Reference to a PublicSensor object that will be populated with the sensor's definition.
   * @param publicName          Name of the sensor whose information should be retrieved.
   * @return                    See SCError.
   */
  SCError getPublicSensorByName(PublicSensor& sensor, const char* publicName);
    
  /**
   * @brief Retrieve a list of public sensors that match a wildcard.
   *
   * @param sensors             Reference to a list of PublicSensor that will be populated with the sensors' definition.
   * @param wildcard            Wildcard to search for in the list of public sensors.
   * @return                    See SCError.
   */
  SCError getPublicSensorsByWildcard(std::list<PublicSensor>& sensors, const char* wildcard);

  /**
   * @brief Determine whether a given sensor is a virtual sensor.
   *
   * @param isVirtual           Reference to a bool which holds the result.
   * @param publicName          Name of the sensor whose information should be retrieved.
   * @return                    See SCError.
   */
  SCError isVirtual(bool& isVirtual, std::string publicName);

  /**
   * @brief Set the scaling factor for a public sensor. (FIXME: Scaling factors system is not in use!)
   *
   * @param publicName          Name of the sensor.
   * @param scalingFactor       New scaling factor for the sensor.
   * @return                    See SCError.
   */
  SCError setSensorScalingFactor(std::string publicName, double scalingFactor);

  /**
   * @brief Set the unit for a public sensor.
   *
   * @param publicName          Name of the sensor.
   * @param unit                New unis for the sensor. See unitconv.h for a list of supported units.
   * @return                    See SCError.
   */
  SCError setSensorUnit(std::string publicName, std::string unit);

  /**
   * @brief Set a sensor property with a mask. Currently implemented properties are "integrable" and "monotonic".
   *
   * @param publicName          Name of the sensor.
   * @param mask                New bit mask for the sensor properties.
   * @return                    See SCError.
   */
  SCError setSensorMask(std::string publicName, uint64_t mask);

   /**
    * @brief Set an operation for the sensor.
    *
    * @param publicName          Name of the sensor.
    * @param operation           Set of operations for the sensor.
    * @return                    See SCError.
    */
    SCError setOperations(std::string publicName, std::set<std::string> operations);

    /**
    * @brief Removes all operations of the sensor.
    *
    * @param publicName          Name of the sensor.
    * @return                    See SCError.
    */
    SCError clearOperations(std::string publicName);

    /**
    * @brief Removes all operations of all sensors matching a given wildcard.
    *
    * @param wildcard            Wildcard to identify sensors whose operations must be cleared.
    * @return                    See SCError.
    */
    SCError clearOperationsByWildcard(std::string wildcard);

    /**
    * @brief Set a new sensor expression for a virtual sensor.
    *
    * @param publicName          Name of the sensor.
    * @param ttl                 Time to live value in nanoseconds.
    * @return                    See SCError.
    */
    SCError setTimeToLive(std::string publicName, uint64_t ttl);

    /**
   * @brief Set the evaluation interval for a sensor.
   *
   * @param publicName          Name of the sensor.
   * @param interval            New evaluation interval for the sensor in nanoseconds.
   * @return                    See SCError.
   */
    SCError setSensorInterval(std::string publicName, uint64_t interval);
    
  /**
   * @brief Set a new sensor expression for a virtual sensor.
   *
   * @param publicName          Name of the sensor.
   * @param expression          New virtual sensor expression.
   * @return                    See SCError.
   */
  SCError setVirtualSensorExpression(std::string publicName, std::string expression);
  
  /**
   * @brief Set the t0 for a virtual sensor.
   *
   * @param publicName          Name of the sensor.
   * @param tZero               New tZero for the sensor.
   * @return                    See SCError.
   */
  SCError setVirtualSensorTZero(std::string publicName, TimeStamp tZero);

  /**
   * @brief Get the timestamp of the most recent update to the publishedsensors table.
   *
   * @param ts                  Unsigned 64-bit integer where to store the result.
   * @return                    See SCError.
   */
  SCError getPublishedSensorsWritetime(uint64_t &ts);

  /**
   * @brief Update the timestamp of the most recent update to the publishedsensors table.
   *
   * @param ts                  New timestamp to insert.
   * @return                    See SCError.
   */
  SCError setPublishedSensorsWritetime(const uint64_t &ts);
  
  /**
   * @brief Constructor for the SensorConfig class.
   *
   * @param conn                Connection object of the current connection into the DCDB data store.
   */
  SensorConfig(Connection* conn);
  virtual ~SensorConfig();
};

} /* End of namespace DCDB */

#endif /* DCDB_SENSORCONFIG_H */
