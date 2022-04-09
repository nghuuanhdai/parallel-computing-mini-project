//================================================================================
// Name        : simplemqttservermessage.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Implementation of a class for receiving a simple MQTT message
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

#include "simplemqttserver.h"
#include <arpa/inet.h> 

using namespace std;
using namespace boost::system;

#ifdef SimpleMQTTVerbose
static boost::mutex coutMtx;
#endif

SimpleMQTTMessage::SimpleMQTTMessage()
{
  /*
   * Initialize all class variables to an
   * empty message state.
   */
  state = Empty;
  bytesProcessed = 0;
  remainingRaw = NULL;
  remainingLength = 0;
  bufferLength = 0;
  fixedHeaderLength = 0;
  msgId = 0;
  payloadLength = 0;
  payloadPtr = NULL;
}

SimpleMQTTMessage::~SimpleMQTTMessage()
{
  /*
   * Free the memory allocated for the remainingRaw buffer.
   */
  if(remainingRaw)
    free(remainingRaw);
}

void SimpleMQTTMessage::clear() {
    //We reset all variables except for the internal buffer, which is recycled
    state = Empty;
    bytesProcessed = 0;
    remainingLength = 0;
    fixedHeaderLength = 0;
    msgId = 0;
    payloadLength = 0;
    payloadPtr = NULL;
}

ssize_t SimpleMQTTMessage::decodeFixedHeader(void* buf, size_t len)
{
  /*
   * Decode the MQTTFixedHeader.
   */
  char* readPtr = (char*)buf;
  ssize_t lbytes = len;

  /*
   * Check for the first byte in the MQTT Fixed Header.
   */
  if (state == Empty) {
      if (lbytes && bytesProcessed == 0) {
          fixedHeader.raw[0] = *readPtr;

          lbytes--;
          bytesProcessed++;
          readPtr++;

          state = DecodingFixedHeader;
      }
      else {
          state = Error;
      }
  }

  /*
   * Decode the length of the message.
   */
  if (state == DecodingFixedHeader) {
      if (lbytes) {
          int multiplier;
          char digit;
          do {
              digit = *readPtr;
              fixedHeader.raw[bytesProcessed] = digit;
              multiplier = 1 << ((bytesProcessed-1)*7);
              remainingLength += (digit & 127) * multiplier;

              lbytes--;
              bytesProcessed++;
              readPtr++;
          }
          while (lbytes && (bytesProcessed < 5) && ((digit&128) != 0));

          if ((digit&128) == 0) {
              fixedHeaderLength = bytesProcessed;

              /*
               * If this message has no variable length part,
               * we're already done. Otherwise, we need to
               * receive a little more.
               */
              if (remainingLength == 0)
                state = Complete;
              else
                state = FixedHeaderOk;

              bytesProcessed = 0;
          }
          else if (bytesProcessed >= 5) {
              state = Error;
          }
      }
  }

  return len-lbytes;
}

ssize_t SimpleMQTTMessage::receiveMessage(void* buf, size_t len)
{
  /*
   * Receive the remainder of an MQTT message.
   */
  ssize_t lbytes = len;

  /*
   * If we are in this function for the first time,
   * we need to allocate the buffer.
   */
  if (!remainingRaw || remainingLength > bufferLength) {
      if(remainingRaw) free(remainingRaw);
      remainingRaw = malloc(remainingLength);
      bufferLength = remainingLength;
      if (!remainingRaw) {
          throw new boost::system::system_error(errno, boost::system::system_category(), "Error in SimpleMQTTMessage::receiveMessage().");
      }
  }

  /*
   * Fill the buffer with all we have (until full).
   */
  char* writePtr = (char*)remainingRaw;
  writePtr += bytesProcessed;

  if (bytesProcessed+len >= remainingLength) {
    memcpy(writePtr, buf, remainingLength-bytesProcessed);
    lbytes -= remainingLength-bytesProcessed;
    bytesProcessed += remainingLength-bytesProcessed;

    /*
     * In this case, we have received the entire message.
     */
    state = Complete;
    switch(fixedHeader.type) {
      case MQTT_PUBLISH: {
        char* data = (char*) remainingRaw;
        /*
         * The topic is contained at the beginning of the remainingRaw field
         * if the message is a publish message.
         * Bytes 0 and 1 of the remainingRaw field encode the string length.
         */
        ssize_t topicLen = ntohs(((uint16_t*) data)[0]);
        data+= 2;

        topic = string(data, topicLen);
        data+= topicLen;
        
        /* if qos is 1 or 2, the msg id follow in the next 2 bytes */
        if (fixedHeader.qos > 0) {
          msgId = ntohs(((uint16_t*) data)[0]);
          data+= 2;
        }
        
        /* The rest of the message is its payload */
        payloadPtr = (void*) data;
        payloadLength = remainingLength - ((uint8_t*)payloadPtr - (uint8_t*)remainingRaw);
        break;
      }
      case MQTT_CONNECT: {
          char* data = (char*) remainingRaw;
          // First 10 bytes compose the CONNECT message's variable header
          data+= 10;
          
          // Message is malformed, break out
          if(remainingLength < 12) {
              state = Error;
          } else {
              ssize_t idLen = ntohs(((uint16_t *) data)[0]);
              data += 2;

              // Leveraging the topic field to store also the client ID on CONNECT messages
              if (idLen > 0) {
                  topic = string(data, idLen);
                  data += idLen;
              } else {
                  topic = "";
              }

              // We store the rest of the CONNECT payload in its raw form
              payloadPtr = (void *) data;
              payloadLength = remainingLength - ((uint8_t *) payloadPtr - (uint8_t *) remainingRaw);
          }
          break;
      }
      case MQTT_PUBREL: {
        msgId = ntohs(((uint16_t*) remainingRaw)[0]);
        break;
      }
    }

  }
  else {
    memcpy(writePtr, buf, len);
    lbytes -= len;
    bytesProcessed += len;
  }

  return len-lbytes;
}

ssize_t SimpleMQTTMessage::appendRawData(void* buf, size_t len)
{
  /*
   * This function appends len bytes to the message.
   * The function returns the number of processed bytes.
   */
  char* readPtr = (char*)buf;
  ssize_t bytes = 0, lbytes = len;

  /*
   * Process the data in buf.
   */
  while(lbytes > 0 && state != Error && state != Complete) {
      switch(state) {
      case Empty:
      case DecodingFixedHeader:
        bytes = decodeFixedHeader(readPtr, lbytes);
        break;
      case FixedHeaderOk:
        bytes = receiveMessage(readPtr, lbytes);
        break;
      default:
        cout << "Unhandled state in SimpleMQTTMessage::appendRawData!\n";
        break;
      }

      readPtr += bytes;
      lbytes -= bytes;
  }

#ifdef SimpleMQTTVerbose
  coutMtx.lock();
  cout << "Finished appendRawData() function. Results follow...\n";
  coutMtx.unlock();
  dump();
#endif

  return len-lbytes;
}

void SimpleMQTTMessage::dump()
{
#ifdef SimpleMQTTVerbose
  coutMtx.lock();
#endif
  cout << "Dump of SimpleMQTTMessage (" << this << "):\n";
  cout << "    State: ";
  switch (state) {
  case Empty: cout << "Empty"; break;
  case DecodingFixedHeader: cout << "DecodingFixedHeader"; break;
  case FixedHeaderOk: cout << "FixedHeaderOk"; break;
  case Complete: cout << "Complete"; break;
  case Error: cout << "Error"; break;
  default: cout << "Unknown state (bad!)"; break;
  }
  cout << "\n    Fixed Header: Type=";
  switch (fixedHeader.type) {
  case MQTT_RESERVED: cout << "RESERVED"; break;
  case MQTT_CONNECT: cout << "CONNECT"; break;
  case MQTT_CONNACK: cout << "CONNACK"; break;
  case MQTT_PUBLISH: cout << "PUBLISH"; break;
  case MQTT_PUBACK: cout << "PUBACK"; break;
  case MQTT_PUBREC: cout << "PUBREC"; break;
  case MQTT_PUBREL: cout << "PUBREL"; break;
  case MQTT_PUBCOMP: cout << "PUBCOMP"; break;
  case MQTT_SUBSCRIBE: cout << "SUBSCRIBE"; break;
  case MQTT_SUBACK: cout << "SUBACK"; break;
  case MQTT_UNSUBSCRIBE: cout << "UNSUBSCRIBE"; break;
  case MQTT_UNSUBACK: cout << "UNSUBACK"; break;
  case MQTT_PINGREQ: cout << "PINGREQ"; break;
  case MQTT_PINGRESP: cout << "PINGRESP"; break;
  case MQTT_DISCONNECT: cout << "DISCONNECT"; break;
  default: cout << "Unknown type (bad!)"; break;
  }
  cout << ", Dup=" << hex << (int)fixedHeader.dup
      << ", QoS=" << hex << (int)fixedHeader.qos
      << ", RETAIN=" << hex << (int)fixedHeader.retain << "\n" << dec;
  cout << "    Bytes Processed: " << bytesProcessed << "\n";
  cout << "    Remaining Length: " << remainingLength << "\n";

  cout << "    MessageID: " << msgId << "\n";
  if (isPublish()) {
      cout << "    Message Topic: " << getTopic() << "\n";
      cout << "    Message Length: " << getPayloadLength() << "\n";
      cout << "    Message Payload: " << string((char*)getPayload(), getPayloadLength()) << "\n";
  }

#ifdef SimpleMQTTVerbose
  coutMtx.unlock();
#endif
}

bool SimpleMQTTMessage::complete()
{
  return state == Complete;
}

bool SimpleMQTTMessage::isPublish()
{
  return complete() && fixedHeader.type == MQTT_PUBLISH;
}

uint8_t SimpleMQTTMessage::getType() {
  return fixedHeader.type;
}

const string& SimpleMQTTMessage::getTopic()
{
  return topic;
}

uint16_t SimpleMQTTMessage::getMsgId() {
  return msgId;
}

uint8_t SimpleMQTTMessage::getQoS() {
  return fixedHeader.qos;
}

size_t SimpleMQTTMessage::getPayloadLength()
{
  return payloadLength;
}

void* SimpleMQTTMessage::getPayload()
{
  return payloadPtr;
}
