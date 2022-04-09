//================================================================================
// Name        : sensorid.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for handling DCDB SensorIDs.
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

#include <string>
#include <algorithm>
#include <cstdint>
#include <cinttypes>
#include <cstdio>

#include <boost/algorithm/string.hpp>

#include "dcdbendian.h"

#include "dcdb/sensorid.h"

using namespace DCDB;

/**
  * @details
  * This method includes all necessary logic to convert a MQTT topic to its internal sensor ID structure.
  * No particular checks are performed at the moment; changing this implementation will affect the storage
  * of sensor IDs at all levels in DCDB.
  */
bool SensorId::mqttTopicConvert(std::string mqttTopic)
{
    if (!mqttTopic.empty()) {
	data = mqttTopic;
	/* Fill string with trailing whitespace to 128bits so Cassandra's ByteOrder
	 Partitioner creates proper numerically sorted tokens */
	if (data.size() < 16) {
	    data.append(16-data.size(), ' ');
	}
	//data.erase(std::remove(data.begin(), data.end(), '/'), data.end());
    }
    return !data.empty();
}

/**
 * @details
 * This function strips all slashes from the pattern string and
 * then compares the sensor and the pattern character by
 * character, skipping over the possible wildcard in the pattern.
 */
bool SensorId::patternMatch(std::string pattern)
{
    /* Strip all slashes from pattern */
    pattern.erase(std::remove(pattern.begin(), pattern.end(), '/'), pattern.end());
    
    if(pattern.length() > data.length())
        return false;

    /* Calculate the wildcard length */
    int wl = (pattern.find("*") != std::string::npos) ? data.length() + 1 - pattern.length() : 0;

    /* Do a character by character comparison */
    int posP = 0;
    int posS = 0;
    char p, s;
    while (posS < (int)data.length()) {
        p = pattern[posP];
        s = data[posS];
        
        if(p == '*') {
            posS+=wl;
            posP++;
            continue;
        } else if (p==s) {
            posS++;
            posP++;
            continue;
        } else {
            return false;
        }
    }
    return true;
}

SensorId::SensorId()
{
    /* Initialize to zeros */
    data = "";
    rsvd = 0;
}

SensorId::SensorId(std::string mqttTopic)
{
    /* Try to convert from MQTT topic */
    if (!mqttTopicConvert(mqttTopic)) {
        /* Fall back to zeros */
        data = "";
    }
}

SensorId::~SensorId()
{
    /* Nothing to do here */
}
