//================================================================================
// Name        : sensorid_num.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description :
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2015-2019 Leibniz Supercomputing Centre
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
 * @file sensorid_num.h
 *
 * @ingroup libdcdb
 */

#include <cstdint>
#include <string>

#ifndef DCDB_SENSORIDNUM_H
#define DCDB_SENSORIDNUM_H

namespace DCDB {

/* Ensure that the unions and structs are created without padding */
#pragma pack(push,1)

/**
 * @brief The DeviceLocation type describes the location of a device. A
 * device is the smallest piece of hardware containing sensors.
 *
 * The location of a device is highly specific to the system architecture
 * and thus, it is only treated as unsigned 64 bit integer internally.
 * Since these bits, however, make up for the location of the data within
 * the distributed database, it is recommended to assign a globally used
 * schema in advance leaving the higher-order bits to higher level
 * entities.
 *
 * Example:
 *  ----------------------------------------------
 * | 8 Bits | 8  Bits | 16 Bits | 8  Bits |  ...
 *  ----------------------------------------------
 * |  Data  |         |         |         |
 * | Center | Cluster |  Rack   | Chassis |  ...
 * |   ID   |   ID    |   ID    |   ID    |
 *  ----------------------------------------------
 */
typedef uint64_t DeviceLocation;

/**
 * @brief The %DeviceSensorId type describes the tuple of the sensor
 *        number and a unique (location-independent) device id.
 * In combination with a location specified by the DeviceLocation type,
 * the sensor_number defines a sensor by location.
 *
 * The device_id member is - in return - used to uniquely identify
 * certain components, even when their location changes. A suitable
 * approach is, for example, to use MAC addresses as the device_id.
 */
typedef struct {
  uint64_t rsvd             : 16; /**< Reserved */
  uint64_t sensor_number    : 16; /**< The sensor_number of the sensor */
  uint64_t device_id        : 32; /**< The location-independent device_id */
} DeviceSensorId;

/**
 * @brief The SensorId class packs the DeviceLocation and DeviceSensorId
 *        types into a single object.
 *
 * In DCDB, a sensor is described by it's location, a sensor number at
 * this location, and a unique part number identifier. This information
 * is packed into a 128 bit wide bitfield.
 *
 * Access to this bitfield can be done through the raw member of through
 * the two variables dl and dsid of the helper types DeviceLocation and
 * DeviceSensorId.
 */
class SensorIdNumerical {
protected:
  union {
    uint64_t raw[2];   /**< The raw bit-field representation of a sensor */
    struct {
      DeviceLocation dl;
      DeviceSensorId dsid;
    };
  } data;

public:
  DeviceLocation getDeviceLocation();
  void setDeviceLocation(DeviceLocation dl);
  DeviceSensorId getDeviceSensorId();
  void setDeviceSensorId(DeviceSensorId dsid);
  uint16_t getSensorNumber();
  void setSensorNumber(uint16_t sn);
  uint16_t getRsvd();
  void setRsvd(uint16_t rsvd);
  uint32_t getDeviceId();
  void setDeviceId(uint32_t did);
  uint64_t* getRaw();
  void setRaw(uint64_t* raw);

  /**
   * @brief This function populates the data field from a MQTT topic string.
   * @param topic    The string from which the SensorId object will be populated.
   * @return         Returns true if the topic string was valid and the data field of the object was populated.
   */
  bool mqttTopicConvert(std::string mqttTopic);

  /**
   * @brief This function returns a "key" string which
   *        corresponds to the supplied SensorId object.
   * @return      Returns the serialized string that can be used by Cassandra as row key.
   */
  std::string serialize() const;

  /**
   * @brief This function returns a hex string which corresponds to the
   * 		supplied SensorId object.
   * @return      Returns a human-readable hex string of the SensorId.
   */
  std::string toString() const;

  /**
   * @brief This function matches the sensor against a
   *        sensor pattern string.
   * @return     Returns true if the sensor name matches the pattern, false otherwise.
   */
  bool patternMatch(std::string pattern);

  inline bool operator == (const SensorIdNumerical& rhs) const {
    return (data.raw[0] == rhs.data.raw[0]) &&
        (data.raw[1] == rhs.data.raw[1]);}

  inline bool operator < (const SensorIdNumerical& rhs) const {
	  if (data.raw[0] == rhs.data.raw[0])
		  return data.raw[1] < rhs.data.raw[1];
	  else
		  return data.raw[0] < rhs.data.raw[0];
  }

  inline bool operator <= (const SensorIdNumerical& rhs) const {
            if (data.raw[0] == rhs.data.raw[0])
                    return data.raw[1] <= rhs.data.raw[1];
            else
                    return data.raw[0] < rhs.data.raw[0];
    }

  inline SensorIdNumerical operator & (const SensorIdNumerical& rhs) const {
      SensorIdNumerical sid;
      sid.data.raw[0] = (data.raw[0] & rhs.data.raw[0]);
      sid.data.raw[1] = (data.raw[1] & rhs.data.raw[1]);
      return sid;
    }
    
  SensorIdNumerical();
  SensorIdNumerical(std::string mqttTopic);
  virtual ~SensorIdNumerical();
};

#pragma pack(pop)

} /* End of namespace DCDB */

#endif /* DCDB_SENSORIDNUM_H */
