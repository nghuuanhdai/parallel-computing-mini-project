//================================================================================
// Name        : simplemqttservermessage.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Headers of a class for receiving a simple MQTT message
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

/**
 * @file simplemqttservermessage.h
 *
 * @brief Class for receiving a simple MQTT message.
 *
 * @ingroup mqttserver
 */
#ifndef SIMPLEMQTTMESSAGE_H_
#define SIMPLEMQTTMESSAGE_H_

#define MQTT_RESERVED           0x0
#define MQTT_CONNECT            0x1
#define MQTT_CONNACK            0x2
#define MQTT_PUBLISH            0x3
#define MQTT_PUBACK             0x4
#define MQTT_PUBREC             0x5
#define MQTT_PUBREL             0x6
#define MQTT_PUBCOMP            0x7
#define MQTT_SUBSCRIBE          0x8
#define MQTT_SUBACK             0x9
#define MQTT_UNSUBSCRIBE        0xa
#define MQTT_UNSUBACK           0xb
#define MQTT_PINGREQ            0xc
#define MQTT_PINGRESP           0xd
#define MQTT_DISCONNECT         0xe

#define DCDB_MAP "/DCDB_MAP/"
#define DCDB_MAP_LEN 10
#define DCDB_MET "/DCDB_MAP/METADATA/"
#define DCDB_MET_LEN 19
#define DCDB_CALIEVT "/DCDB_CE/"
#define DCDB_CALIEVT_LEN 9
#define DCDB_JOBDATA "/DCDB_JOBDATA/"
#define DCDB_JOBDATA_LEN 14

#pragma pack(push,1)

typedef union {
  uint8_t raw[5];
  struct {
    uint8_t retain      :1;
    uint8_t qos         :2;
    uint8_t dup         :1;
    uint8_t type        :4;
    uint8_t remaining_length[4];
  };
} MQTTFixedHeader;

#pragma pack(pop)

enum MQTTMessageState {
  Empty,
  DecodingFixedHeader,
  FixedHeaderOk,
  Complete,
  Error
};

class SimpleMQTTMessage
{
protected:
  MQTTMessageState state;
  MQTTFixedHeader fixedHeader;
  size_t fixedHeaderLength;
  size_t bytesProcessed;
  size_t remainingLength;
  uint16_t msgId;
  std::string topic;
  void *remainingRaw;
  size_t bufferLength;
  size_t payloadLength;
  void *payloadPtr;

  ssize_t decodeFixedHeader(void* buf, size_t len);
  ssize_t receiveMessage(void* buf, size_t len);

public:
  ssize_t appendRawData(void* buf, size_t len);
  bool complete();
  bool isPublish();
  uint8_t getType();
  const std::string& getTopic();
  uint16_t getMsgId();
  uint8_t getQoS();
  size_t getPayloadLength();
  void* getPayload();  

  void dump();

  SimpleMQTTMessage();
  virtual ~SimpleMQTTMessage();
  void clear();
};

#endif /* SIMPLEMQTTMESSAGE_H_ */
