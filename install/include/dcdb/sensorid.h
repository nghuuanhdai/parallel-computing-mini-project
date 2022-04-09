//================================================================================
// Name        : sensorid.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for handling DCDB SensorIDs.
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
 * @file sensorid.h
 *
 * @ingroup libdcdb
 */

#include <cstdint>
#include <string>

#ifndef DCDB_SENSORID_H
#define DCDB_SENSORID_H

namespace DCDB {

/* Ensure that the unions and structs are created without padding */
#pragma pack(push,1)

/**
 * @brief The SensorId class uniquely identifies sensors that can be queried
 *
 * In DCDB, a sensor is described by an ID in the form of a string, which 
 * usually corresponds to its MQTT topic. An additional field of 16 bits 
 * allows to set secondary identification info (such as the week-stamp).
 *
 */
class SensorId {
    protected:
        // The string object used to store the sensor's ID
        std::string data;
        // 16-bits reserved field
        uint16_t    rsvd;

    public:
        
        /**
         * @brief           Sets the internal data field
         * 
         *                  The input string is assumed to be an MQTT topic, which is sanitized and then stored
         *                  in the data field.
         * 
         * @param d         The string to be used as sensor ID
         */
        void setId(std::string &d) { if (!mqttTopicConvert(d)) data = ""; }
        
        /**
         * @brief           Sets the value of the internal reserved bit field
         * 
         *                  This field usually contains the week-stamp associated to the row of a certain sensor.
         * 
         * @param rs        The short integer value encoding the bit field
         */
        void setRsvd(uint16_t rs)  { rsvd = rs; }

        /**
         * @param           Get the current sensor ID 
         * @return          The string encoding the current sensor ID
         */
        std::string getId() const  { return data; }
        
        /**
         * @param           Get the reserved bit field
         * @return          A short integer encoding the bit field
         */
        uint16_t getRsvd()  const  { return rsvd; }
        
        /**
         * @brief This function populates the data field from a MQTT topic string.
         * @param topic    The string from which the SensorId object will be populated.
         * @return         Returns true if the topic string was valid and the data field of the object was populated.
         */
        bool mqttTopicConvert(std::string mqttTopic);
        
        /**
         * @brief This function matches the sensor against a
         *        sensor pattern string.
         * @return     Returns true if the sensor name matches the pattern, false otherwise.
         */
        bool patternMatch(std::string pattern);

        // Overloaded operators for comparisons
        inline bool operator == (const SensorId& rhs) const {
            return (data == rhs.data) && (rsvd == rhs.rsvd); 
        }

        inline bool operator < (const SensorId& rhs) const {
            if (data == rhs.data)
                return rsvd < rhs.rsvd;
            else
                return data < rhs.data;
        }

        inline bool operator <= (const SensorId& rhs) const {
            if (data == rhs.data)
                return rsvd <= rhs.rsvd;
            else
                return data < rhs.data;
        }
        
        SensorId();
        SensorId(std::string mqttTopic);
        virtual ~SensorId();
    };

#pragma pack(pop)

} /* End of namespace DCDB */

#endif /* DCDB_SENSORID_H */
