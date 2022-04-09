//================================================================================
// Name        : sensorid_num.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for handling DCDB SensorIDs
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2016 Leibniz Supercomputing Centre
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

#include <string>
#include <algorithm>
#include <cstdint>
#include <cinttypes>
#include <cstdio>

#include <boost/algorithm/string.hpp>

#include "dcdbendian.h"

#include "dcdb/sensorid_num.h"

using namespace DCDB;

/*
 * Public Functions
 */
DeviceLocation SensorIdNumerical::getDeviceLocation()
{
  return data.dl;
}

void SensorIdNumerical::setDeviceLocation(DeviceLocation dl)
{
  data.dl = dl;
}

DeviceSensorId SensorIdNumerical::getDeviceSensorId()
{
  return data.dsid;
}

void SensorIdNumerical::setDeviceSensorId(DeviceSensorId dsid)
{
  /* Do not overwrite the RSVD field */
  data.dsid.sensor_number = dsid.sensor_number;
  data.dsid.device_id = dsid.device_id;
}

uint16_t SensorIdNumerical::getSensorNumber()
{
  return data.dsid.sensor_number;
}

void SensorIdNumerical::setSensorNumber(uint16_t sn)
{
  data.dsid.sensor_number = sn;
}

uint16_t SensorIdNumerical::getRsvd()
{
  return data.dsid.rsvd;
}

void SensorIdNumerical::setRsvd(uint16_t rsvd)
{
  data.dsid.rsvd = rsvd;
}

uint32_t SensorIdNumerical::getDeviceId()
{
  return data.dsid.device_id;
}

void SensorIdNumerical::setDeviceId(uint32_t did)
{
  data.dsid.device_id = did;
}

uint64_t* SensorIdNumerical::getRaw()
{
  return data.raw;
}

void SensorIdNumerical::setRaw(uint64_t* raw)
{
  data.raw[0] = raw[0];
  data.raw[1] = raw[1];
}

/**
  * @details
  * The conversion of a MQTT message topic to a SensorId
  * object is performed by byte-wise scanning of the string,
  * and skipping of all characters except for those in the
  * range [0-9,a-f,A-F]. Each character is then turned from
  * hex string into its binary representation an OR'ed into
  * the 128-bit raw fields of the SensorId object.
  */
bool SensorIdNumerical::mqttTopicConvert(std::string mqttTopic)
{
  uint64_t pos = 0;
  const char* buf = mqttTopic.c_str();
  data.raw[0] = 0;
  data.raw[1] = 0;
  while (*buf && pos < 112) {
      if (*buf >= '0' && *buf <= '9') {
          data.raw[pos / 64] |= (((uint64_t)(*buf - '0')) << (60-(pos%64)));
          pos += 4;
      }
      else if (*buf >= 'A' && *buf <= 'F') {
          data.raw[pos / 64] |= (((uint64_t)(*buf - 'A' + 0xa)) << (60-(pos%64)));
          pos += 4;
      }
      else if (*buf >= 'a' && *buf <= 'f') {
          data.raw[pos / 64] |= (((uint64_t)(*buf - 'a' + 0xa)) << (60-(pos%64)));
          pos += 4;
      }
      buf++;
  }
  return pos == 112;
}

/**
 * @details
 * This function serializes a SensorId object into a
 * big-endian 128-bit character array represented as
 * std::string.
 */
std::string SensorIdNumerical::serialize() const
{
    uint64_t ll[2];
    ll[0] = Endian::hostToBE(data.raw[0]);
    ll[1] = Endian::hostToBE(data.raw[1]);
    return std::string((char*)ll, 16);
}

/**
 * @details
 * This function returns a hex representation of a SensorId as a
 * 28-character std::string.
 */
std::string SensorIdNumerical::toString() const
{
	char buf[33];
	snprintf(buf, sizeof(buf), "%016" PRIx64 "%016" PRIx64, data.raw[0], data.raw[1]);
	return std::string(buf).substr(0, 28);
}

/**
 * @details
 * This function strips all slashes from the pattern string and
 * then compares the sensor and the pattern character by
 * character, skipping over the possible wildcard in the pattern.
 */
bool SensorIdNumerical::patternMatch(std::string pattern)
{
  /* Strip all slashes from pattern */
  pattern.erase(std::remove(pattern.begin(), pattern.end(), '/'), pattern.end());

  /* Convert to lower case */
  boost::algorithm::to_lower(pattern);

  /* Calculate the wildcard length */
  int wl = (pattern.find("*") != std::string::npos) ? 29 - pattern.length() : 0;

  /* Do a character by character comparison */
  int posP = 0;
  int posS = 0;
  while (posS < 28) {
      char p, cs[2];
      uint64_t s;
      p = pattern.c_str()[posP];
      s = data.raw[posS / 16];
      s >>= (60-((4*posS)%64));
      s &= 0xf;
      snprintf(cs, 2, "%" PRIx64, s);

      if (p == '*') {
          /* Jump over the wildcard */
          posS += wl;
          posP++;
          continue;
      }
      else if (p==cs[0]) {
          posS++;
          posP++;
          continue;
      }
      else {
          return false;
      }
  }
  return true;
}

SensorIdNumerical::SensorIdNumerical()
{
  /* Initialize to zeros */
  data.raw[0] = 0;
  data.raw[1] = 0;
}

SensorIdNumerical::SensorIdNumerical(std::string mqttTopic)
{
  /* Try to convert from MQTT topic */
  if (!mqttTopicConvert(mqttTopic)) {
      /* Fall back to zeros */
      data.raw[0] = 0;
      data.raw[1] = 0;
  }
}

SensorIdNumerical::~SensorIdNumerical()
{
  /* Nothing to do here */
}
