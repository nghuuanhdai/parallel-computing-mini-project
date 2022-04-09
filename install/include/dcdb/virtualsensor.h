//================================================================================
// Name        : virtualsensor.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for evaluating virtual sensors in libdcdb.
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
 * @file virtualsensor.h
 * @brief This file contains classes for handling virtual sensors.
 *
 * @ingroup libdcdb
 */

#include <stdexcept>
#include <string>
#include <unordered_set>

#include "connection.h"
#include "sensorconfig.h"
#include "sensordatastore.h"

#ifndef DCDB_VIRTUAL_SENSOR_H
#define DCDB_VIRTUAL_SENSOR_H

namespace DCDB {

/**
 * @brief Exception class for parsing Virtual %Sensor Expressions
 *
 * Exceptions of this type are thrown whenever the parser for virtual
 * sensor expressions encounters a syntax error. What() tries to
 * provide the user with a hint on the location where the error was
 * encountered while processing the expression string.
 */
class VSExpressionParserException : public std::runtime_error
{
public:
  VSExpressionParserException(const std::string& where) : runtime_error("Error parsing expression at: "), where_(where) {}
  virtual const char* what() const throw();
private:
  static std::string msg_;
  std::string where_;
};

/* Forward declare VirtualSensor::VSensorExpressionImpl and VirtualSensor::VSensorImpl */
namespace VirtualSensor{
class VSensorExpressionImpl;
class VSensorImpl;
}

/**
 * @brief Public class for evaluating Virtual Sensors expressions
 *
 * This class forwards to the library internal VSensorExpressionImpl class.
 */
class VSensorExpression
{
protected:
  VirtualSensor::VSensorExpressionImpl *impl;

public:
  void getInputs(std::unordered_set<std::string>& inputSet);
  void getInputsRecursive(std::unordered_set<std::string>& inputSet, bool virtualOnly = true);

  VSensorExpression(Connection* conn, std::string expr);
  virtual ~VSensorExpression();
};

typedef enum {
  VS_OK,
  VS_UNKNOWNERROR
} VSError;

/**
 * @brief Class for querying virtual sensors
 *
 * TODO: Implement this...
 */
class VSensor
{
protected:
  VirtualSensor::VSensorImpl *impl;

public:
  VSError query(std::list<SensorDataStoreReading>& result, TimeStamp& start, TimeStamp& end);
  VSError queryCB(SensorDataStore::QueryCbFunc cbFunc, void* userData, TimeStamp& start, TimeStamp& end);

  VSensor(Connection *conn, std::string name);
  VSensor(Connection *conn, PublicSensor sensor);
  virtual ~VSensor();
};

} /* End of namespace DCDB */

#endif /* DCDB_VIRTUAL_SENSOR_H */
