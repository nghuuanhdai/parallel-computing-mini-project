//================================================================================
// Name        : timestamp.h
// Author      : Axel Auweter
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


/**
 * @file dcdb/timestamp.h
 * @brief This file is a companion to the sensordatastore API.
 * It contains the TimeStamp class definition that helps in
 * creating and modifying timestamps in the SensorDataStore.
 *
 * @ingroup libdcdb
 */

#include <cstdint>
#include <ctime>
#include <string>
#include <stdexcept>

#ifndef DCDB_TIMESTAMP_H
#define DCDB_TIMESTAMP_H

#define NS_PER_S 1000000000

namespace DCDB {

class TimeStampConversionException : public std::runtime_error
{
public:
  TimeStampConversionException():runtime_error("Time stamp conversion error.") {}
};

/**
 * @brief The %TimeStamp class contains a single %TimeStamp.
 */
class TimeStamp
{
protected:
  uint64_t raw;         /**< The raw timestamp data (nanoseconds since Unix Epoch) */

  /**
   * @brief Parses a string and tries to derive the time from it by
   * guessing the format. Throws TimeStampException on failure.
   * @param timestr   A string containing a representation of time
   * @param localTime Denotes if the timestr contains local time instead of UTC
   */
  void guessFromString(std::string timestr, bool localTime = false);

public:

  /**
   * @brief Standard constructor. Initializes the object with the current time.
   */
  TimeStamp();

  /**
   * @brief Raw constructor. Initializes the object with an existing raw time.
   */
  TimeStamp(uint64_t ts) {raw = ts;}

  /**
   * @brief String constructor. Initializes the object by best guess from a time string.
   */
  TimeStamp(std::string ts, bool localTime = false);

  /**
   * @brief Time_t constructor. Initializes the object from a time_t struct.
   */
  TimeStamp(time_t ts);

  /**
   * @brief Standard destructor.
   */
  virtual ~TimeStamp();

  /**
   * @brief Sets the object's value to the current time.
   */
  void setNow(void);

  /**
   * @brief Convert from local time to UTC
   */
  void convertFromLocal(void);

  /**
   * @brief Convert from UTC to local time
   */
  void convertToLocal(void);

  /**
   * @brief Returns the raw time stamp value.
   * @return The object's value as uint64_t.
   */
  uint64_t getRaw(void) const;

  /**
   * @brief Returns the time stamp's value as human readable string
   * @return The object's value as std::string.
   */
  std::string getString(void) const;

  /**
   * @brief Returns the "weekstamp" corresponding to the object's value
   * @return The week number of the timestamp.
   */
  uint16_t getWeekstamp(void) const;

  /* Overloaded operators (compare raw values) */
  inline bool operator == (const TimeStamp& rhs) const {return raw == rhs.raw;}
  inline bool operator != (const TimeStamp& rhs) const {return raw != rhs.raw;}
  inline bool operator <  (const TimeStamp& rhs) const {return raw <  rhs.raw;}
  inline bool operator >  (const TimeStamp& rhs) const {return raw >  rhs.raw;}
  inline bool operator <= (const TimeStamp& rhs) const {return raw <= rhs.raw;}
  inline bool operator >= (const TimeStamp& rhs) const {return raw >= rhs.raw;}
};

} /* End of namespace DCDB */

#endif /* DCDB_TIMESTAMP_H */
