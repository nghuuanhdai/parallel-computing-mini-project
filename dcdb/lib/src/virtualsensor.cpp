//================================================================================
// Name        : virtualsensor.cpp
// Author      : Axel Auweter
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for evaluating virtual sensors in libdcdb.
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

#include "dcdb/virtualsensor.h"
#include "virtualsensor_internal.h"
#include "dcdb/sensorconfig.h"
#include "dcdbglobals.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cstdlib>

namespace DCDB {

/*
 * Implementations for VSExpressionParserException class.
 */
std::string VSExpressionParserException::msg_;
const char* VSExpressionParserException::what() const throw() {
   msg_ = runtime_error::what();
   msg_ += where_;
   msg_ += "\n";
   return msg_.c_str();
}

/*
 * Implementations for VSensorExpression class.
 */
void VSensorExpression::getInputs(std::unordered_set<std::string>& inputSet)
{
  impl->getInputs(inputSet);
}

void VSensorExpression::getInputsRecursive(std::unordered_set<std::string>& inputSet, bool virtualOnly)
{
  return impl->getInputsRecursive(inputSet, virtualOnly);
}

VSensorExpression::VSensorExpression(Connection* conn, std::string expr) {
  impl = new VirtualSensor::VSensorExpressionImpl(conn, expr);
}

VSensorExpression::~VSensorExpression() {
  if (impl) {
    delete impl;
  }
}

/*
 * Implementations for VSensor class.
 */
VSError VSensor::query(std::list<SensorDataStoreReading>& result, TimeStamp& start, TimeStamp& end)
{
  return impl->query(result, start, end);
}

VSError VSensor::queryCB(SensorDataStore::QueryCbFunc cbFunc, void* userData, TimeStamp& start, TimeStamp& end)
{
  return impl->queryCB(cbFunc, userData, start, end);
}

VSensor::VSensor(Connection *conn, std::string name)
{
  impl = new VirtualSensor::VSensorImpl(conn, name);
}

VSensor::VSensor(Connection *conn, PublicSensor sensor)
{
  impl = new VirtualSensor::VSensorImpl(conn, sensor);
}

VSensor::~VSensor()
{
  if (impl) {
      delete impl;
  }
}

/*
 * Implementations for PhysicalSensorCache class
 */
#define PSC_READ_AHEAD "1000"
#define PSC_READ_BEHIND "1"
void PhysicalSensorCache::populate(Connection* connection, SensorConfig& sc, uint64_t t)
{
  /* FIXME: This function has a couple of problems:
   *   - Error handling should be improved (exceptions instead of simply returning to caller)
   *   - If reading ahead or reading behind does not return the number of readings specified
   *     by PSC_READ_AHEAD/PSC_READ_BEHIND, this could have two reasons of which only one is
   *     currently being addressed:
   *       1) there is simply no more data (time series ends)
   *       2) there could be more data but under another sensorId (either because the request
   *          window crosses a week stamp or because the pattern expansion results in multiple
   *          internal sensor IDs).
   *   -
   */

  CassSession* session = connection->getSessionHandle();

  /* Expand the sensor's public name into its internal SensorId */
  DCDB::SensorId sid(s.name);
  sid.setRsvd(DCDB::TimeStamp(t).getWeekstamp());

  /* Find the readings before and after time t */
  CassError rc = CASS_OK;
  CassStatement* statement = NULL;
  CassFuture *future = NULL;
  const CassPrepared* prepared = nullptr;
  const char* queryBefore = "SELECT * FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? AND ws = ? AND ts <= ? ORDER BY ws DESC, ts DESC LIMIT " PSC_READ_BEHIND;
  const char* queryAfter = "SELECT * FROM " KEYSPACE_NAME "." CF_SENSORDATA " WHERE sid = ? AND ws = ? AND ts > ? LIMIT " PSC_READ_AHEAD;
  
  /* Query before... */
  future = cass_session_prepare(session, queryBefore);
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    cass_future_free(future);
    return;
  }

  prepared = cass_future_get_prepared(future);
  cass_future_free(future);

  statement = cass_prepared_bind(prepared);
  cass_statement_bind_string(statement, 0, sid.getId().c_str());
  cass_statement_bind_int16(statement, 1, sid.getRsvd());
  cass_statement_bind_int64(statement, 2, t);

  future = cass_session_execute(session, statement);
  cass_future_wait(future);

  if (cass_future_error_code(future) == CASS_OK) {
      const CassResult* cresult = cass_future_get_result(future);
      CassIterator* rows = cass_iterator_from_result(cresult);

      while (cass_iterator_next(rows)) {
          const CassRow* row = cass_iterator_get_row(rows);

          cass_int64_t ts, value;
          cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &ts);
          cass_value_get_int64(cass_row_get_column_by_name(row, "value"), &value);

          SensorDataStoreReading r;
          r.sensorId = sid;
          r.timeStamp = (uint64_t)ts;
          r.value = (int64_t)value;

          cache.insert(std::make_pair((uint64_t)ts, r));
      }
      cass_iterator_free(rows);
      cass_result_free(cresult);
  }

  cass_statement_free(statement);
  cass_future_free(future);
  cass_prepared_free(prepared);

  /* Query after... */
  future = cass_session_prepare(session, queryAfter);
  cass_future_wait(future);

  rc = cass_future_error_code(future);
  if (rc != CASS_OK) {
    connection->printError(future);
    cass_future_free(future);
    return;
  }

  prepared = cass_future_get_prepared(future);
  cass_future_free(future);

  statement = cass_prepared_bind(prepared);
  cass_statement_bind_string(statement, 0, sid.getId().c_str());
  cass_statement_bind_int16(statement, 1, sid.getRsvd());
  cass_statement_bind_int64(statement, 2, t);
  
  future = cass_session_execute(session, statement);
  cass_future_wait(future);

  if (cass_future_error_code(future) == CASS_OK) {
      const CassResult* cresult = cass_future_get_result(future);
      CassIterator* rows = cass_iterator_from_result(cresult);

      while (cass_iterator_next(rows)) {
          const CassRow* row = cass_iterator_get_row(rows);

          cass_int64_t ts, value;
          cass_value_get_int64(cass_row_get_column_by_name(row, "ts"), &ts);
          cass_value_get_int64(cass_row_get_column_by_name(row, "value"), &value);

          SensorDataStoreReading r;
          r.sensorId = sid;
          r.timeStamp = (uint64_t)ts;
          r.value = (int64_t)value;

          cache.insert(std::make_pair((uint64_t)ts, r));
      }

      cass_iterator_free(rows);
      cass_result_free(cresult);
  }

  cass_statement_free(statement);
  cass_future_free(future);
  cass_prepared_free(prepared);

#if 0
  std::cerr << "Cache for sensor " << s.name << " after populate:" << std::endl;
  for (std::map<uint64_t, SensorDataStoreReading>::iterator i=cache.begin(); i != cache.end(); i++) {
      std::cerr << i->first << " : " << i->second.timeStamp.getRaw() << " : " << i->second.value << std::endl;
  }
#endif
}

void PhysicalSensorCache::getBefore(Connection* connection, SensorConfig& sc, SensorDataStoreReading& r, uint64_t t)
{
  /* Check the cache */
  std::map<uint64_t, SensorDataStoreReading>::iterator i = cache.lower_bound(t);
  if (i == cache.end()) {
    //  std::cerr << "Cache miss in getBefore(" << s.name << ", " << t << ")" << std::endl;
    populate(connection, sc, t);
    i = cache.lower_bound(t);
  }
  if(i != cache.begin() && i != cache.end()) {
      --i;
  }
  else {
      std::stringstream msg;
      TimeStamp ts(t);
      msg << "Cannot find reading for sensor " << s.name << " prior to time " << ts.getString() << "(" << ts.getRaw() << ")" << std::endl;
      throw PhysicalSensorEvaluatorException(msg.str());
  }

  /* If we are here, we have a value */
  r = i->second;
}

void PhysicalSensorCache::getAfter(Connection* connection, SensorConfig& sc, SensorDataStoreReading& r, uint64_t t)
{
  /* Check the cache */
  std::map<uint64_t, SensorDataStoreReading>::iterator i = cache.upper_bound(t);
  if (i == cache.end()) {
    //  std::cerr << "Cache miss in getAfter(" << s.name << ", " << t << ")" << std::endl;
    populate(connection, sc, t);
    i = cache.lower_bound(t);
    if (i == cache.end()) {
        std::stringstream msg;
        TimeStamp ts(t);
        msg << "Cannot find reading for sensor " << s.name << " following time " << ts.getString() << "(" << ts.getRaw() << ")" << std::endl;
        throw PhysicalSensorEvaluatorException(msg.str());
    }
  }

  /* If we are here, we have a value */
  r = i->second;
}

PhysicalSensorCache::PhysicalSensorCache(PublicSensor sensor)
{
  s = sensor;
}

PhysicalSensorCache::~PhysicalSensorCache()
{
}

namespace VirtualSensor {
/*
 * Implementations for VSensorExpressionImpl class.
 */
void VSensorExpressionImpl::generateAST(std::string expr)
{
  /* Try to generate AST */
  typedef std::string::const_iterator StringIterator;
  typedef ExpressionGrammar<StringIterator> Grammar;

  ascii::space_type space;
    Grammar grammar;

  /* Add the list of known sensors to the grammar */
  std::list<std::string> sensorNames;
  SensorConfig sc(connection);
  sc.getPublicSensorNames(sensorNames);
  grammar.addSensorNames(sensorNames);

  StringIterator it = expr.begin();
  StringIterator end = expr.end();
  bool success = phrase_parse(it, end, grammar, space, opseq);

  if ((!success) || (it != end)) {
      std::string rest(it, end);
      throw VSExpressionParserException(rest);
  }

  /* Success - opseq now represents the top level of our AST */
}

void VSensorExpressionImpl::dumpAST()
{
  /* Declare a struct describing the action for each object in the AST when it comes to printing */
  struct ASTPrinter {
    typedef void result_type;
    void operator()(AST::Nil) const {}
    void operator()(unsigned int n) const { std::cout << n; }
    void operator()(std::string s) const { std::cout << "sensor(" << s << ")"; }
    void operator()(AST::Op const& x) const {
      boost::apply_visitor(*this, x.oprnd);
      switch (x.oprtr) {
      case '+': std::cout << " add"; break;
      case '-': std::cout << " sub"; break;
      case '*': std::cout << " mul"; break;
      case '/': std::cout << " div"; break;
      }
    }
    void operator()(AST::Signd const& x) const {
      boost::apply_visitor(*this, x.oprnd);
      switch (x.sgn) {
      case '-': std::cout << " neg"; break;
      case '+': std::cout << " pos"; break;
      }
    }

//    void operator()(AST::Function const& x) const {std::cout << " " << x.fnctn;}
    void operator()(AST::Opseq const& x) const {
      boost::apply_visitor(*this, x.frst);
      BOOST_FOREACH(AST::Op const& o, x.rst) {
        std::cout << ' ';
        (*this)(o);
      }
    }
  };

  ASTPrinter printer;
  printer(opseq);
  std::cout << std::endl;
}

int64_t VSensorExpressionImpl::physicalSensorDelta(Connection* connection, SensorConfig& sc,  PhysicalSensorCacheContainer& pscc, PublicSensor& sensor, TimeStamp t, TimeStamp tzero, uint64_t frequency)
{

	int64_t currentSensorReading = physicalSensorInterpolator(connection, sc, pscc, sensor, t);
	if( t.getRaw() == tzero.getRaw())
		return currentSensorReading;
	else {
		TimeStamp previousT(t.getRaw() - frequency);
		int64_t previousSensorReading = physicalSensorInterpolator(connection, sc, pscc, sensor, previousT);
		return currentSensorReading - previousSensorReading;
	}
}

int64_t VSensorExpressionImpl::physicalSensorInterpolator(Connection* connection, SensorConfig& sc,  PhysicalSensorCacheContainer& pscc, PublicSensor& sensor, TimeStamp t)
{
  SensorDataStoreReading readingBefore, readingAfter;

  /* Get readingBefore and readingAfter from the sensor's cache */
  pscc[sensor.name]->getBefore(connection, sc, readingBefore, t.getRaw());
  pscc[sensor.name]->getAfter(connection, sc, readingAfter, t.getRaw());

//  std::cerr << "For time " << t.getRaw() << ", sensor " << sensor.name << ": before=(" << readingBefore.value << "," << readingBefore.timeStamp.getRaw() << ") after=(" << readingAfter.value << "," << readingAfter.timeStamp.getRaw() << ")" << std::endl;

  /*
   * Linearly interpolate between the readings using the following equation:
   *
   *     y2 - y1       x2y1 - x1y2
   * y = ------- * x + -----------
   *     x2 - x1         x2 - x1
   */
  typedef double eval_type;
  eval_type x1 = readingBefore.timeStamp.getRaw();
  eval_type x2 = readingAfter.timeStamp.getRaw();
  eval_type y1 = readingBefore.value;
  eval_type y2 = readingAfter.value;
  eval_type x = t.getRaw();

  return (((y2 - y1) / (x2 - x1)) * x) + (((x2 * y1) - (x1 * y2)) / (x2 - x1));
}

void VSensorExpressionImpl::getInputs(std::unordered_set<std::string>& inputSet)
{
  /* Declare a struct describing the action for each object in the AST when it comes to collecting the sensor inputs */
  struct ASTInputCollector {
    typedef void result_type;
    void operator()(AST::Nil) const {}
    void operator()(unsigned int n) const { }
    void operator()(std::string s) const {
        if(s.find("delta_")!=std::string::npos)
    		is.insert(s.substr(6, (std::size_t) s.length() - 6));
    	else
    		is.insert(s);
    }
    void operator()(AST::Op const& x) const {
      boost::apply_visitor(*this, x.oprnd);
    }
    void operator()(AST::Signd const& x) const {
      boost::apply_visitor(*this, x.oprnd);
    }

    void operator()(AST::Opseq const& x) const {
      boost::apply_visitor(*this, x.frst);
      BOOST_FOREACH(AST::Op const& o, x.rst) {
        (*this)(o);
      }
    }

    ASTInputCollector(std::unordered_set<std::string>& inputSet) : is(inputSet) {}

    std::unordered_set<std::string>& is;
  };

  ASTInputCollector inputCollector(inputSet);
  inputCollector(opseq);
}

void VSensorExpressionImpl::getInputsRecursive(std::unordered_set<std::string>& inputSet, bool virtualOnly)
{
  /* Get our own inputs */
  std::unordered_set<std::string> myInputs;
  getInputs(myInputs);

  /* Iterate over inputs and append to set */
  for (std::unordered_set<std::string>::iterator it = myInputs.begin(); it != myInputs.end(); it++ ) {

      /* Get information for this sensor */
      SensorConfig sc(connection);
      PublicSensor psen;
      sc.getPublicSensorByName(psen, it->c_str());

      /* Check if the sensor is physical and whether we only append virtual sensors */
      if (!psen.is_virtual && virtualOnly) {
          continue;
      }

      /* Append the sensor to the set */
      inputSet.insert(*it);

      /* If the sensor is physical, we're done */
      if (!psen.is_virtual) {
          continue;
      }

      /* Recurse into this sensor's list of inputs */
      VSensorExpressionImpl vsen(connection, psen.expression);
      vsen.getInputsRecursive(inputSet, virtualOnly);
  }
}

int64_t VSensorExpressionImpl::evaluateAt(TimeStamp time, PhysicalSensorCacheContainer& pscc, TimeStamp tzero, uint64_t frequency)
{
  /* Declare a struct describing the action for each object in the AST when it comes to evaluation */
  struct ASTEvaluator {
    typedef int64_t result_type;
    int64_t operator()(AST::Nil) const { return 0; }
    int64_t operator()(unsigned int n) const { return n; }
    int64_t operator()(std::string s) const {

    	if(s.find("delta_")!=std::string::npos) {
    		/* Evaluate sensor s at time t */
    		SensorConfig sc(c);
    		PublicSensor sen;
    		std::string sensorName = s.substr(6, (std::size_t) s.length() - 6);
    		if (sc.getPublicSensorByName(sen, sensorName.c_str()) != SC_OK) {
    			std::cout << "Internal error on getPublicSensorByName while trying to evaluate " << sensorName << " at " << t.getString() << std::endl;
    			return 0;
    		}

    		/* Things are easy if the sensor is virtual */
    		if (sen.is_virtual) {

    			VSensorExpressionImpl vSen(c, sen.expression);
    			int64_t currentSensorReading = vSen.evaluateAt(t, ps, tz, f);

    			if(t.getRaw() == tz.getRaw())
    				return currentSensorReading;
    			else {
    				TimeStamp previousT(t.getRaw() - f);
    				int64_t previousSensorReading = vSen.evaluateAt(previousT, ps, tz, f);
    				return currentSensorReading - previousSensorReading;
    			}
    		}
    		else {
    			/* Physical sensors need a little bit more thinkin' */
				return physicalSensorDelta(c, sc, ps, sen, t, tz, f);
    		}

    	}
    	else {

    		/* Evaluate sensor s at time t */
    		SensorConfig sc(c);
    		PublicSensor sen;
    		if (sc.getPublicSensorByName(sen, s.c_str()) != SC_OK) {
    			std::cout << "Internal error on getPublicSensorByName while trying to evaluate " << s << " at " << t.getString() << std::endl;
    			return 0;
    		}

    		/* Things are easy if the sensor is virtual */
    		if (sen.is_virtual) {
    			VSensorExpressionImpl vSen(c, sen.expression);
    			return vSen.evaluateAt(t, ps, tz, f);
    		}
    		else {
    			/* Physical sensors need a little bit more thinkin' */
    			return physicalSensorInterpolator(c, sc, ps, sen, t);
    		}

    		return 0;
      }
    }
    int64_t operator()(int64_t lhs, AST::Op const& x) const {
      int64_t rhs = boost::apply_visitor(*this, x.oprnd);

      switch (x.oprtr) {
      case '+': return lhs + rhs; break;
      case '-': return lhs - rhs; break;
      case '*': return lhs * rhs; break;
      case '/': return lhs / rhs; break;
      }
      return 0;
    }
    int64_t operator()(AST::Signd const& x) const {
      int64_t rhs = boost::apply_visitor(*this, x.oprnd);
      switch (x.sgn) {
      case '-': return -rhs; break;
      case '+': return +rhs; break;
      }
      return 0;
    }
//    int64_t operator()(AST::Function const& x) const {

//    	  switch (x.fnctn) {
//
//    	  /* Switch-case is here for future function definitions...*/
//    	  case "delta_":
//
//
//    	  /* case 'functionX_':
//    	   * case 'functionY_':
//    	   * ...
//    	   */
//    		 break;
//    	  }
//    	  return 0;
//    }
    int64_t operator()(AST::Opseq const& x) const {
      return std::accumulate(
          x.rst.begin(), x.rst.end(),
          boost::apply_visitor(*this, x.frst),
          *this
          );
    }

    ASTEvaluator(Connection* conn, TimeStamp time, PhysicalSensorCacheContainer& pscc, TimeStamp tzero, uint64_t frequency) : c(conn), t(time), ps(pscc), tz(tzero), f(frequency) {}

    Connection* c;
    TimeStamp t;
    PhysicalSensorCacheContainer& ps;
    TimeStamp tz;
    uint64_t f;
  };

  ASTEvaluator eval(connection, time, pscc, tzero, frequency);
  return eval(opseq);
}

VSensorExpressionImpl::VSensorExpressionImpl(Connection* conn, std::string expr)
{
  /* Assign connection variable */
  connection = conn;

  /* Generate the AST for the expression */
  generateAST(expr);
    
    dumpAST();
}

VSensorExpressionImpl::~VSensorExpressionImpl()
{
}

/*
 * Implementations for VSensorImpl class.
 */
PhysicalSensorCacheContainer VSensorImpl::physicalSensorCaches;

VSError VSensorImpl::query(std::list<SensorDataStoreReading>& result, TimeStamp& start, TimeStamp& end)
{
  /* Clear physical sensor caches */
  for (PhysicalSensorCacheContainer::iterator it = physicalSensorCaches.begin(); it != physicalSensorCaches.end(); it++) {
    delete it->second;
  }
  physicalSensorCaches.clear();

  /* Initialize sensor caches */
  std::unordered_set<std::string> inputs;
  expression->getInputsRecursive(inputs, false);

  SensorConfig sc(connection);
  PublicSensor psen;
  for (std::unordered_set<std::string>::iterator it = inputs.begin(); it != inputs.end(); it++) {
      sc.getPublicSensorByName(psen, it->c_str());
      if (!psen.is_virtual) {
      //    std::cerr << "Adding to list of phys sensor caches: " << *it << std::endl;
          physicalSensorCaches.insert(std::make_pair(*it, new PhysicalSensorCache(psen)));
      }
  }

  /*
   * Calculate first and last time stamp at which this virtual sensor fires:
   * Each virtual sensor fires at t0 + n*frequency (n=0,1,2,....)
   */
  uint64_t n_start, n_end;
  n_start = (start.getRaw() - tzero.getRaw()) / frequency;
  n_end = (end.getRaw() - tzero.getRaw()) / frequency;

  /* Clear the result set */
  result.clear();

  /* Iterate over all time steps at which this sensor fires. */
  for (
      uint64_t i = (tzero.getRaw() + (n_start * frequency));
      i <= (tzero.getRaw() + (n_end * frequency));
      i += frequency)
  {
      try {
          int64_t eval = expression->evaluateAt(i, physicalSensorCaches, this->getTZero(), this->getFrequency());
          TimeStamp t(i);
          SensorDataStoreReading r;
          r.timeStamp = t;
          r.value = eval;
          result.push_back(r);
      }
      catch (PhysicalSensorEvaluatorException& e) {
          std::cerr << e.what();
      }
  }

  return VS_OK;
}

VSError VSensorImpl::queryCB(SensorDataStore::QueryCbFunc cbFunc, void* userData, TimeStamp& start, TimeStamp& end)
{
  /* Clear physical sensor caches */
  for (PhysicalSensorCacheContainer::iterator it = physicalSensorCaches.begin(); it != physicalSensorCaches.end(); it++) {
    delete it->second;
  }
  physicalSensorCaches.clear();

  /* Initialize sensor caches */
  std::unordered_set<std::string> inputs;
  expression->getInputsRecursive(inputs, false);

  SensorConfig sc(connection);
  PublicSensor psen;
  for (std::unordered_set<std::string>::iterator it = inputs.begin(); it != inputs.end(); it++) {
      sc.getPublicSensorByName(psen, it->c_str());
      if (!psen.is_virtual) {
      //    std::cerr << "Adding to list of phys sensor caches: " << *it << std::endl;
          physicalSensorCaches.insert(std::make_pair(*it, new PhysicalSensorCache(psen)));
      }
  }

  /*
   * Calculate first and last time stamp at which this virtual sensor fires:
   * Each virtual sensor fires at t0 + n*frequency (n=0,1,2,....)
   */
  uint64_t n_start, n_end;
  n_start = (start.getRaw() - tzero.getRaw()) / frequency;
  n_end = (end.getRaw() - tzero.getRaw()) / frequency;

  /* Iterate over all time steps at which this sensor fires. */
  for (
      uint64_t i = (tzero.getRaw() + (n_start * frequency));
      i <= (tzero.getRaw() + (n_end * frequency));
      i += frequency)
  {
      try {
          int64_t eval = expression->evaluateAt(i, physicalSensorCaches, this->tzero, this->frequency);
          TimeStamp t(i);
          SensorDataStoreReading r;
          r.timeStamp = t;
          r.value = eval;
          cbFunc(r, userData);
      }
      catch (PhysicalSensorEvaluatorException& e) {
          std::cerr << e.what();
      }
  }

  return VS_OK;
}

TimeStamp VSensorImpl::getTZero() {
	return tzero;
}

uint64_t VSensorImpl::getFrequency() {
	return frequency;
}

void VSensorImpl::setTZero(TimeStamp t) {
	tzero = t;
}

void VSensorImpl::setFrequency(uint64_t f) {
	frequency = f;
}

VSensorImpl::VSensorImpl(Connection *conn, std::string name)
{
  SensorConfig sc(conn);
  PublicSensor sen;

  /* TODO: How to behave if sensor not found? */
  if (sc.getPublicSensorByName(sen, name.c_str()) != SC_OK) {
      std::cout << "Internal error: getPublicSensorByName(" << name << ") failed." << std::endl;
      exit(EXIT_FAILURE);
  }

  /* The rest is done by the constructor working on the PublicSensor object */
  VSensorImpl(conn, sen);
}

VSensorImpl::VSensorImpl(Connection *conn, PublicSensor sensor)
{
  /* TODO: How to behave if sensor is a physical public sensor? Exception? */
  if (!sensor.is_virtual) {
      std::cout << "Internal error: Trying to populate VSensorImpl with physical sensor data." << std::endl;
      exit(EXIT_FAILURE);
  }

  /* Assign the connection variable */
  connection = conn;

  /* Populate the local fields from the PublicSensor struct */
  name = sensor.name;
  expression = new VSensorExpressionImpl(connection, sensor.expression);
  vsensorid = new SensorId(sensor.v_sensorid);
  tzero = sensor.t_zero;
  frequency = sensor.interval;
}

VSensorImpl::~VSensorImpl()
{
  /* Clear physical sensor caches */
  for (PhysicalSensorCacheContainer::iterator it = physicalSensorCaches.begin(); it != physicalSensorCaches.end(); it++) {
    delete it->second;
  }
  physicalSensorCaches.clear();

  if (expression) {
      delete expression;
  }
  if (vsensorid) {
      delete vsensorid;
  }
}

} /* End of namespace VirtualSensor */
} /* End of namespace DCDB */
