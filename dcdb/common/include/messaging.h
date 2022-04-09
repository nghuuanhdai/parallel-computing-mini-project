//================================================================================
// Name        : messaging.h
// Author      : Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Generic functions and types for DCDB MQTT messages
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
 * @file messaging.h
 *
 * @brief Generic functions and types for DCDB MQTT messages.
 *
 * @ingroup common
 */

#include <boost/date_time/posix_time/posix_time.hpp>

#ifndef MESSAGING_H_
#define MESSAGING_H_

typedef struct {
  uint64_t value;
  uint64_t timestamp;
} mqttPayload;

class Messaging
{
public:

  static uint64_t calculateTimestamp() {
    boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    boost::posix_time::time_duration diff = now - epoch;

    return diff.total_nanoseconds();
  }
};

#endif /* MESSAGING_H_ */
