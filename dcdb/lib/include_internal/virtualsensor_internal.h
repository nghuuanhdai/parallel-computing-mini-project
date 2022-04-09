//================================================================================
// Name        : virtualsensor_internal.h
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Internal interface for evaulating virtual sensors in libdcdb.
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
 * @file
 * @brief This file contains internal classes for handling virtual sensors.
 */

#include <boost/spirit/include/qi.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <cstdint>
#include <string>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <map>

#include "dcdb/timestamp.h"
#include "dcdb/sensorid.h"
#include "dcdb/sensordatastore.h"

#include "dcdb/virtualsensor.h"

#ifndef DCDB_VIRTUAL_SENSOR_INTERNAL_H
#define DCDB_VIRTUAL_SENSOR_INTERNAL_H
#define BOOST_SPIRIT_DEBUG

namespace DCDB {

/**
 * @brief Exception class for errors during Physical Sensor Evaluation
 *
 * Exceptions of this type are thrown whenever the the evaluation of
 * a physical sensor is impossible due to data being out of range.
 * What() returns a human readable error string.
 */
class PhysicalSensorEvaluatorException : public std::runtime_error
{
public:
  PhysicalSensorEvaluatorException(const std::string& msg) : runtime_error(msg) {}
};

class PhysicalSensorCache {
protected:
  std::map<uint64_t, SensorDataStoreReading> cache;
  PublicSensor s;
  void populate(Connection* connection, SensorConfig& sc, uint64_t t);

public:
  void getBefore(Connection* connection, SensorConfig& sc, SensorDataStoreReading& r, uint64_t t);
  void getAfter(Connection* connection, SensorConfig& sc, SensorDataStoreReading& r, uint64_t t);

  PhysicalSensorCache(PublicSensor sensor);
  virtual ~PhysicalSensorCache();
};

typedef std::unordered_map<std::string, PhysicalSensorCache*> PhysicalSensorCacheContainer;

namespace VirtualSensor {

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

/**
 * @brief Definitions for objects in the Virtual Sensor expression's AST.
 */
namespace AST {

/**
 * @brief The NIL object.
 *
 * The NIL object represents a no-op or empty operand in the AST.
 */
struct Nil {};

/**
 * @brief The SIGNED object.
 *
 * The SIGNED object represents a positive or negative sign preceding an operand in the AST.
 */
struct Signd;

/**
 * @brief The FUNCTION object.
 *
 * The FUNCTION object represents a generic function that could be applied to an operand (expression) in the AST.
 * Currently supported functions: delta_(sensor).
 */
//struct Function;

/**
 * @brief The OPSEQ object.
 *
 * The OPSEQ object represents a sequence of operations in the AST.
 */
struct Opseq;

/**
 * @brief The OP object.
 *
 * The OP object represents a simple operation in the AST.
 */
struct Op;

typedef boost::variant<
    Nil,
    unsigned int,
    std::string,
    boost::recursive_wrapper<Signd>,
    boost::recursive_wrapper<Opseq>
  > Operand;

struct Signd {
  char sgn;
  Operand oprnd;
};

struct Opseq {
  Operand frst;
  std::list<Op> rst;
};

struct Op {
  char oprtr;
  Operand oprnd;
};

} /* End of namespace AST */
} /* End of namespace VirtualSensor */
} /* End of namespace DCDB */

/*
 * Create Random Access Sequence for AST Objects.
 * These must be created at the top of namespaces but obviously after defining the structs.
 */
BOOST_FUSION_ADAPT_STRUCT (
    DCDB::VirtualSensor::AST::Signd,
    (char, sgn)
    (DCDB::VirtualSensor::AST::Operand, oprnd)
)

BOOST_FUSION_ADAPT_STRUCT (
    DCDB::VirtualSensor::AST::Opseq,
    (DCDB::VirtualSensor::AST::Operand, frst)
    (std::list<DCDB::VirtualSensor::AST::Op>, rst)
)

BOOST_FUSION_ADAPT_STRUCT (
    DCDB::VirtualSensor::AST::Op,
    (char, oprtr)
    (DCDB::VirtualSensor::AST::Operand, oprnd)
)

namespace DCDB {
namespace VirtualSensor {

/**
 * @brief The ExpressionGrammar struct holds the grammar definition for
 *        the arithmetic expressions that describe virtual sensors.
 *
 * Since we're using the Boost Spirit framework's Qi parser generator,
 * we define the grammar using the qi syntax and built-in qi parsers
 * (e.g. qi::uint_ or qi:hex).
 */
template <typename Iterator>
struct ExpressionGrammar : qi::grammar<Iterator, AST::Opseq(), ascii::space_type>
{
  ExpressionGrammar() : ExpressionGrammar::base_type(expression)
  {
    expression =
        term
        >> *(   (qi::char_('+') >> term)
            |   (qi::char_('-') >> term)
            )
        ;

    term =
        factor
        >> *(   (qi::char_('*') >> factor)
            |   (qi::char_('/') >> factor)
            )
        ;

    factor =
        ("0x" >> qi::hex)
        |   qi::uint_
        |   '(' >> expression >> ')'
        |   (qi::char_('-') >> factor)
        |   (qi::char_('+') >> factor)
        |   delta
        |   sensor
        ;

  }

  qi::rule<Iterator, AST::Opseq(), ascii::space_type> expression;
  qi::rule<Iterator, AST::Opseq(), ascii::space_type> term;
  qi::rule<Iterator, AST::Operand(), ascii::space_type> factor;
  qi::symbols<char, std::string> sensor;
  qi::symbols<char, std::string> delta;
  std::string deltaStr = "delta_";
    

  /**
   * @brief Populates the parser grammar with a symbol table of available sensor names.
   *
   * Since it eases the implementation, references to sensors in the virtual sensor
   * expressions are parsed as symbols by the Qi parser. Therefore, we have to populate
   * the list of sensors from the public sensors table during runtime. This should be
   * done always after instantiating objects of the ExpressionGrammar.
   */
  void addSensorNames(std::list<std::string> sensorNames) {
    for (std::list<std::string>::iterator it = sensorNames.begin(); it != sensorNames.end(); it++) {
        sensor.add (it->c_str(), *it);
        delta.add(deltaStr + it->c_str(), deltaStr + *it);
    }
  }
};

/**
 * @brief Private implementation class for evaluating Virtual Sensors expressions
 *
 * This class implements the expression parser and provides functions that create an
 * unordered set of inputs required to evaluate the expression.
 */
class VSensorExpressionImpl
{
protected:
  Connection* connection;
  VirtualSensor::AST::Opseq opseq;

  void generateAST(std::string expr);
  void dumpAST();

  static int64_t physicalSensorInterpolator(Connection* connection, SensorConfig& sc,  PhysicalSensorCacheContainer& pscc, PublicSensor& sensor, TimeStamp t);
  static int64_t physicalSensorDelta(Connection* connection, SensorConfig& sc,  PhysicalSensorCacheContainer& pscc, PublicSensor& sensor, TimeStamp t, TimeStamp tzero, uint64_t frequency);

public:
  void getInputs(std::unordered_set<std::string>& inputSet);
  void getInputsRecursive(std::unordered_set<std::string>& inputSet, bool virtualOnly);

  int64_t evaluateAt(TimeStamp time, PhysicalSensorCacheContainer& pscc, TimeStamp tzero, uint64_t frequency);

  VSensorExpressionImpl(Connection* conn, std::string expr);
  virtual ~VSensorExpressionImpl();
};

/**
 * @brief Private implementation class for querying virtual sensors
 *
 * TODO: Implement this...
 */
class VSensorImpl
{
protected:
  Connection* connection;
  std::string name;
  VSensorExpressionImpl* expression;
  SensorId* vsensorid;
  TimeStamp tzero;
  uint64_t frequency;
  static PhysicalSensorCacheContainer physicalSensorCaches;

public:

  TimeStamp getTZero();
  uint64_t getFrequency();
  void setTZero(TimeStamp tzero);
  void setFrequency(uint64_t frequency);
  VSError query(std::list<SensorDataStoreReading>& result, TimeStamp& start, TimeStamp& end);
  VSError queryCB(SensorDataStore::QueryCbFunc cbFunc, void* userData, TimeStamp& start, TimeStamp& end);

  VSensorImpl(Connection *conn, std::string name);
  VSensorImpl(Connection *conn, PublicSensor sensor);
  virtual ~VSensorImpl();
};

} /* End of namesapce VirtualSensor */
} /* End of namespace DCDB */

#endif /* DCDB_VIRTUAL_SENSOR_INTERNAL_H */
