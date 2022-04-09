//================================================================================
// Name        : timestamp.h
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Provides timestamp functionality.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2017-2019 Leibniz Supercomputing Centre
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
 * @file common/include/timestamp.h
 *
 * @brief Timestamp utility functions.
 *
 * @ingroup common
 */

#ifndef INCLUDE_TIMESTAMP_H_
#define INCLUDE_TIMESTAMP_H_
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <sstream>

#define S_TO_NS(x)  x*1000000000ll
#define MS_TO_NS(x) x*1000000ll
#define US_TO_NS(x) x*1000ll
#define NS_TO_US(x) x/1000ll
#define NS_TO_MS(x) x/1000000ll
#define NS_TO_S(x)  x/1000000000ll


static uint64_t getTimestamp() {
  boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
  boost::posix_time::time_duration diff = now - epoch;

  return diff.total_nanoseconds();
}

static uint64_t ptime2Timestamp(const boost::posix_time::ptime t) {
  boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
  boost::posix_time::time_duration diff = t - epoch;

  return diff.total_nanoseconds();
}

static boost::posix_time::ptime timestamp2ptime(const uint64_t ts) {
  return boost::posix_time::ptime(boost::gregorian::date(1970,1,1), boost::posix_time::nanoseconds(ts));
}

static std::string prettyPrintTimestamp(const uint64_t ts) {
  std::stringstream ss;
  ss << ts / 1000000000 << "." << std::setw(9) << std::setfill('0') << ts % 1000000000;
  return ss.str();
}



#endif /* INCLUDE_TIMESTAMP_H_ */
