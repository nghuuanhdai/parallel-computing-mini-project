//================================================================================
// Name        : timestamp.cpp
// Author      : Axel Auweter, Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for handling time stamps in libdcdb.
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

#include <cstring>
#include <cinttypes>

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/local_time/local_time.hpp"
#include "boost/date_time/c_local_time_adjustor.hpp"
#include "boost/regex.hpp"

#include "dcdb/timestamp.h"

using namespace DCDB;

/* Use this as our local to utc adjustor */
typedef boost::date_time::c_local_adjustor<boost::posix_time::ptime> local_adj;

/**
 * This function parses a string and tries to do a best guess at the contained
 * time information. Currently, it detects strings in the format "yyyy-mm-dd hh:mm:ss.000"
 * and posix time.
 */
void TimeStamp::guessFromString(std::string timestr, bool localTime)
{
  boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
  uint64_t tmp;

  /* First try to match it against a time string */
  try {
      boost::posix_time::ptime ts(boost::posix_time::time_from_string(timestr));
      if (ts != boost::posix_time::not_a_date_time) {
          boost::posix_time::time_duration diff = ts - epoch;
          raw = diff.total_nanoseconds();
          if (localTime) {
              convertFromLocal();
          }
          return;
      }
  }
  catch (std::exception& e) {
      /* Ignore on error */
  }

  /*
   * Try to match it against a string containing "now".
   * Note that we ignore the localTime flag in this case since
   * we already get "now" in UTC.
   */
  if (timestr.find("now") != std::string::npos) {

      /* If the string is just "now"... */
      if ((timestr.compare("now") == 0) && (timestr.length() == 3)) {
          setNow();
          return;
      }

      /* If the string is of the form "now-X" */
      boost::regex d("now-[0-9]*d", boost::regex::extended);
      boost::regex h("now-[0-9]*h", boost::regex::extended);
      boost::regex m("now-[0-9]*m", boost::regex::extended);
      boost::regex s("now-[0-9]*s", boost::regex::extended);

      if (boost::regex_match(timestr, d)) {
          sscanf(timestr.c_str(), "now-%" PRIu64, &tmp);
          setNow();
          raw -= tmp * 24 * 60 * 60 * 1000 * 1000 * 1000;
          return;
      }
      if (boost::regex_match(timestr, h)) {
          sscanf(timestr.c_str(), "now-%" PRIu64, &tmp);
          setNow();
          raw -= tmp * 60 * 60 * 1000 * 1000 * 1000;
          return;
      }
      if (boost::regex_match(timestr, m)) {
          sscanf(timestr.c_str(), "now-%" PRIu64, &tmp);
          setNow();
          raw -= tmp * 60 * 1000 * 1000 * 1000;
          return;
      }
      if (boost::regex_match(timestr, s)) {
          sscanf(timestr.c_str(), "now-%" PRIu64, &tmp);
          setNow();
          raw -= tmp * 1000 * 1000 * 1000;
          return;
      }

      /* "now" keyword is in the timestamp but does not match one of the predefined formats */
      throw TimeStampConversionException();
  }

  /* Try to match it against a POSIX time */
  if (sscanf(timestr.c_str(), "%" PRIu64, &tmp) == 1) {
	  /* Checker whether it is a date before 2070-12-31 */
	  if (tmp < 31872923400ull) {                   // s
		raw = tmp * 1000 * 1000 * 1000;
	  } else if (tmp < 31872923400000ull) {         // ms
		raw = tmp * 1000 * 1000;
	  } else if (tmp < 31872923400000000ull) {      // ns
		raw = tmp * 1000;
	  } else {                                      // us
		raw = tmp;
	  }
	  return;
  }

  /* Not successful - throw exception */
  throw TimeStampConversionException();
}

/**
 * This constructor is implemented by calling setNow().
 */
TimeStamp::TimeStamp()
{
  setNow();
}

/**
 * This constructor sets the time using the magic implemented in guessFromString.
 */
TimeStamp::TimeStamp(std::string ts, bool localTime)
{
  guessFromString(ts, localTime);
}

/**
 * This constructor sets the from a time_t struct.
 */
TimeStamp::TimeStamp(time_t ts)
{
  boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
  boost::posix_time::ptime t =  boost::posix_time::from_time_t(ts);
  boost::posix_time::time_duration diff = t - epoch;

  raw = diff.total_nanoseconds();
}

/**
 * Currently, the destructor doesn't need to do anything.
 */
TimeStamp::~TimeStamp()
{
}

/**
 * This function sets the value of raw to the nanoseconds since epoch
 * using some magic help of boost::posix_time.
 */
void TimeStamp::setNow(void)
{
  boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
  boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
  boost::posix_time::time_duration diff = now - epoch;

  raw = diff.total_nanoseconds();
}

/**
 * Treat the current value as local time and replace with the equivalent in UTC.
 */
void TimeStamp::convertFromLocal()
{
  /*
   * Unfortunately BOOST c_local_adjustor doesn't support local-to-utc conversion.
   * Therefore, we first check the offset of "raw" during utc-to-local conversion
   * and then apply this offset in the other direction.
   */

  /* Construct ptime object t from raw */
  boost::posix_time::ptime t(boost::gregorian::date(1970,1,1));
  t += boost::posix_time::nanoseconds(raw);

  /* Convert t to UTC */
  boost::posix_time::ptime t2 = local_adj::utc_to_local(t);

  /* Check the difference of the two times */
  boost::posix_time::time_duration diff = t - t2;

  /* Write back adjusted value to raw */
  raw += diff.total_nanoseconds();
}

/**
 * Treat the current value as UTC and replace with the equivalent in local time.
 */
void TimeStamp::convertToLocal()
{
  /* Construct ptime object t from raw */
  boost::posix_time::ptime t(boost::gregorian::date(1970,1,1));
  t += boost::posix_time::nanoseconds(raw);

  /* Convert t to Local */
  t = local_adj::utc_to_local(t);

  /* Write back to raw */
  boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
  boost::posix_time::time_duration diff = t - epoch;
  raw = diff.total_nanoseconds();
}

/**
 * Return the raw time value (nanoseconds since Unix epoch).
 */
uint64_t TimeStamp::getRaw(void) const
{
  return raw;
}

/**
 * Return the current value as string.
 */
std::string TimeStamp::getString(void) const
{
#ifndef BOOST_DATE_TIME_HAS_NANOSECONDS
#error Needs nanoseconds support in boost.
#endif
  boost::posix_time::ptime t(boost::gregorian::date(1970,1,1));
  t += boost::posix_time::nanoseconds(raw);
  return boost::posix_time::to_iso_extended_string(t);
}


/**
 * Return the "weekstamp" of the current value.
 */
uint16_t TimeStamp::getWeekstamp(void) const
{
  uint16_t week = raw / 604800000000000;
  return week;
}

