//================================================================================
// Name        : query.cpp
// Author      : Axel Auweter, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Implementation of query class of dcdbquery
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

#include <iostream>
#include <list>
#include <string>
#include <algorithm>

#include <cstdlib>
#include <cinttypes>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "dcdbendian.h"
#include "query.h"
#include "dcdb/sensoroperations.h"
#include <dcdb/jobdatastore.h>


void DCDBQuery::setLocalTimeEnabled(bool enable) {
  useLocalTime = enable;
}

bool DCDBQuery::getLocalTimeEnabled() {
  return useLocalTime;
}

void DCDBQuery::setRawOutputEnabled(bool enable) {
  useRawOutput = enable;
}

bool DCDBQuery::getRawOutputEnabled() {
  return useRawOutput;
}

int DCDBQuery::connect(const char* hostname) {
    if (connection != nullptr) {
	return 0;
    }
    connection = new DCDB::Connection();
    connection->setHostname(hostname);
    if (!connection->connect()) {
	std::cout << "Cannot connect to database." << std::endl;
	return 1;
    }
    return 0;
}

void DCDBQuery::disconnect() {
    connection->disconnect();
    delete connection;
    connection = nullptr;
}

bool scaleAndConvert(int64_t &value, double baseScalingFactor, double scalingFactor, DCDB::Unit baseUnit, DCDB::Unit unit) {
    if(scalingFactor != 1.0 || baseScalingFactor != 1.0) {
	if( DCDB::scale(&value, scalingFactor, baseScalingFactor) == DCDB::DCDB_OP_OVERFLOW)
	    return false;
    }
    
    /* Convert the unit if requested */
    if ((unit != DCDB::Unit_None) && (unit != baseUnit)) {
	if (!DCDB::UnitConv::convert(value, baseUnit, unit)) {
	    std::cerr << "Warning, cannot convert units ("
	    << DCDB::UnitConv::toString(baseUnit) << " -> "
	    << DCDB::UnitConv::toString(unit) << ")"
	    << std::endl;
	    return false;
	}
    }
	    
    return true;
}

void DCDBQuery::genOutput(std::list<DCDB::SensorDataStoreReading> &results, queryMap_t::iterator start, queryMap_t::iterator stop) {
    /* Print Header */
    std::cout << "Sensor,Time";
    for (queryMap_t::iterator it=start; it!=stop; it++) {
	switch(it->second.operation) {
	    case DCDB_OP_NONE:
	    case DCDB_OP_WINTERMUTE: {
		switch(it->second.aggregate) {
		    case DCDB::AGGREGATE_MIN:
			std::cout << ",min";
			break;
		    case DCDB::AGGREGATE_MAX:
			std::cout << ",max";
			break;
		    case DCDB::AGGREGATE_AVG:
			std::cout << ",avg";
			break;
		    case DCDB::AGGREGATE_SUM:
			std::cout << ",sum";
			break;
		    case DCDB::AGGREGATE_COUNT:
			std::cout << ",count";
			break;
		    default:
			std::cout << ",Value";
			break;
		}
		break;
	    }
	    case DCDB_OP_DELTA:
		std::cout << ",Delta";
		break;
	    case DCDB_OP_DELTAT:
		std::cout << ",Delta_t";
		break;
	    case DCDB_OP_DERIVATIVE:
		std::cout << ",Derivative";
		break;
	    case DCDB_OP_INTEGRAL:
		std::cout << ",Integral";
		break;
	    case DCDB_OP_RATE:
		std::cout << ",Rate";
		break;
	    default:
		break;
	}
	
	std::string unitStr;
	if(it->second.unit != DCDB::Unit_None) {
	    unitStr = DCDB::UnitConv::toString(it->second.unit);
	} else if (baseUnit != DCDB::Unit_None) {
	    unitStr = DCDB::UnitConv::toString(baseUnit);
	}
	if (it->second.operation == DCDB_OP_DERIVATIVE) {
	    if ((unitStr.back() == 's') || (unitStr.back() == 'h')) {
		unitStr.pop_back();
	    } else if (unitStr.back() == 'J') {
		unitStr.pop_back();
		unitStr.append("W");
	    }
	} else if (it->second.operation == DCDB_OP_INTEGRAL) {
	    if (unitStr.back() == 'W') {
		unitStr.append("s");
	    }
	}
	if (unitStr.size() > 0) {
	    std::cout << " (" << unitStr << ")";
	}
    }
    std::cout << std::endl;
    
    int64_t prevReading=0;
    DCDB::TimeStamp prevT((uint64_t) 0);
    for (std::list<DCDB::SensorDataStoreReading>::iterator reading = results.begin(); reading != results.end(); reading++) {
	DCDB::TimeStamp ts = (*reading).timeStamp;
	
	/* Print the sensor's public name */
	std::cout << start->first.name << ",";
	
	/* Print the time stamp */
	if (useRawOutput) {
	    std::cout << ts.getRaw();
	} else {
	    if (useLocalTime) {
		ts.convertToLocal();
	    }
	    std::cout << ts.getString();
	}
	
	int64_t value, result;
	/* Print the sensor value */
	for (queryMap_t::iterator it=start; it!=stop; it++) {
	    DCDB::Unit unit;
	    if (it->second.unit != DCDB::Unit_None) {
		unit = it->second.unit;
	    } else {
		unit = baseUnit;
	    }
	    value = (*reading).value;
	    result = 0;
	    bool resultOk = false;
	    switch(it->second.operation) {
		case DCDB_OP_NONE:
		case DCDB_OP_WINTERMUTE:
		    if (scaleAndConvert(value, baseScalingFactor, it->second.scalingFactor, baseUnit, it->second.unit)) {
			result = value;
			resultOk = true;
		    }
		    break;
		case DCDB_OP_DELTA:
		    if (scaleAndConvert(value, baseScalingFactor, it->second.scalingFactor, baseUnit, it->second.unit)) {
			if ((prevT > (uint64_t) 0) && (DCDB::delta(value, prevReading, &result) == DCDB::DCDB_OP_SUCCESS)) {
			    resultOk = true;
			}
		    }
		    break;
		case DCDB_OP_DELTAT:
		    if (scaleAndConvert(value, baseScalingFactor, it->second.scalingFactor, baseUnit, it->second.unit)) {
			if ((prevT > (uint64_t) 0) && (DCDB::delta(ts.getRaw(), prevT.getRaw(), &result) == DCDB::DCDB_OP_SUCCESS)) {
			    resultOk = true;
			}
		    }
		    break;
		case DCDB_OP_DERIVATIVE: {
		    int64_t prevValue = 0;
		    if ((it->first.sensor_mask & DELTA) != DELTA) {
			prevValue = prevReading;
		    }
		    if (scaleAndConvert(value, baseScalingFactor, it->second.scalingFactor, baseUnit, it->second.unit) && scaleAndConvert(prevValue, baseScalingFactor, it->second.scalingFactor, baseUnit, it->second.unit)) {
			if( (prevT > (uint64_t) 0) && DCDB::derivative(value, prevValue, ts.getRaw(), prevT.getRaw(), &result, unit) == DCDB::DCDB_OP_SUCCESS) {
			    resultOk = true;
			}
		    }
		    break;}
		case DCDB_OP_INTEGRAL: {
		    int64_t prevValue = prevReading;
		    if (scaleAndConvert(prevValue, baseScalingFactor, it->second.scalingFactor, baseUnit, it->second.unit)) {
			if( (prevT > (uint64_t) 0) && DCDB::integral(prevValue, ts.getRaw(), prevT.getRaw(), &result, unit) == DCDB::DCDB_OP_SUCCESS) {
			    resultOk = true;
			}
		    }
		    break;}
		case DCDB_OP_RATE: {
		    int64_t prevValue = prevReading;
		    if (scaleAndConvert(value, baseScalingFactor, it->second.scalingFactor, baseUnit, it->second.unit)) {
			if( (prevT > (uint64_t) 0) && DCDB::rate(value, ts.getRaw(), prevT.getRaw(), &result) == DCDB::DCDB_OP_SUCCESS) {
			    resultOk = true;
			}
		    }
		    break;}
		default:
		    break;
	    }
	    if (resultOk) {
		std::cout << "," << result;
	    } else {
		std::cout << ",";
	    }
	}
	prevReading = value;
	prevT = ts;
	std::cout << std::endl;
    }
}

void DCDBQuery::setInterval(DCDB::TimeStamp start, DCDB::TimeStamp end) {
    start_ts = start;
    end_ts = end;
}

void DCDBQuery::parseSensorSpecification(const std::string sensor, std::string& sensorName, queryConfig_t& queryCfg) {
    
    std::string s = sensor;
    /* Check for function first */
    boost::regex functRegex("^([^\\(\\)]+)\\(([^\\(\\)]+)\\)$", boost::regex::extended);
    boost::smatch match;
    std::string functName;
    if(boost::regex_search(s, match, functRegex)) {
	functName = match[1].str();
	s = match[2].str();
    }
    
    /* Split into sensor name and potential modifier, i.e. unit conversion or scaling factor */
    boost::regex sensorRegex("([^\\@]+)\\@?([^\\@]*)", boost::regex::extended);
    std::string modifierStr;
    if(boost::regex_search(s, match, sensorRegex)) {
	sensorName = match[1].str();
	modifierStr = match[2].str();
    }
    
    queryCfg = { 1.0, DCDB::Unit_None, DCDB_OP_NONE, DCDB::AGGREGATE_NONE};
    if (functName.length() == 0) {
	queryCfg.operation = DCDB_OP_NONE;
    } else if (boost::iequals(functName, "delta")) {
	queryCfg.operation = DCDB_OP_DELTA;
    } else if (boost::iequals(functName, "delta_t")) {
	queryCfg.operation = DCDB_OP_DELTAT;
    } else if (boost::iequals(functName, "derivative")) {
	queryCfg.operation = DCDB_OP_DERIVATIVE;
    } else if (boost::iequals(functName, "integral")) {
	queryCfg.operation = DCDB_OP_INTEGRAL;
    } else if (boost::iequals(functName, "rate")) {
	queryCfg.operation = DCDB_OP_RATE;
    } else if (boost::iequals(functName, "min")) {
	queryCfg.aggregate = DCDB::AGGREGATE_MIN;
    } else if (boost::iequals(functName, "max")) {
	queryCfg.aggregate = DCDB::AGGREGATE_MAX;
    } else if (boost::iequals(functName, "avg")) {
	queryCfg.aggregate = DCDB::AGGREGATE_AVG;
    } else if (boost::iequals(functName, "sum")) {
	queryCfg.aggregate = DCDB::AGGREGATE_SUM;
    } else if (boost::iequals(functName, "count")) {
	queryCfg.aggregate = DCDB::AGGREGATE_COUNT;
    } else {
	queryCfg.operation = DCDB_OP_WINTERMUTE;
	queryCfg.wintermuteOp = functName;
    }
    
    if (queryCfg.operation != DCDB_OP_UNKNOWN) {
	if (modifierStr.length() > 0) {
	    boost::regex e("[0-9]*\\.?[0-9]*", boost::regex::extended);
	    if (boost::regex_match(modifierStr, e)) {
		queryCfg.scalingFactor = atof(modifierStr.c_str());
	    } else {
		queryCfg.unit = DCDB::UnitConv::fromString(modifierStr);
	    }
	}
    }
}

void DCDBQuery::prepareQuery(std::list<std::string> sensors) {
    /* Initialize the SensorConfig interface */
    DCDB::SensorConfig sensorConfig(connection);
    
    /* Iterate over list of sensors requested by the user */
    for (std::list<std::string>::iterator it = sensors.begin(); it != sensors.end(); it++) {
	std::string sensorName;
	queryConfig_t queryCfg;
	parseSensorSpecification(*it, sensorName, queryCfg);
	if (queryCfg.operation != DCDB_OP_UNKNOWN) {
	    std::list <DCDB::PublicSensor> publicSensors;
	    sensorConfig.getPublicSensorsByWildcard(publicSensors, sensorName.c_str());
	    if (publicSensors.size() > 0) {
		for (auto sen: publicSensors) {
			if(queryCfg.operation == DCDB_OP_WINTERMUTE) {
				if(sen.operations.find(queryCfg.wintermuteOp) != sen.operations.end()) {
					sen.name = sen.name + queryCfg.wintermuteOp;
					sen.pattern = sen.pattern + queryCfg.wintermuteOp;
				} else {
					std::cerr << "Unknown sensor operation: " << queryCfg.wintermuteOp << std::endl;
					continue;
				}
			}
		    queries.insert(std::pair<DCDB::PublicSensor, queryConfig_t>(sen, queryCfg));
		}
	    } else {
		DCDB::PublicSensor pS;
		pS.name = sensorName;
		pS.pattern = sensorName;
		if(queryCfg.operation != DCDB_OP_WINTERMUTE)
			queries.insert(std::pair<DCDB::PublicSensor, queryConfig_t>(pS, queryCfg));
		else
			std::cerr << "Unknown sensor operation: " << queryCfg.wintermuteOp << std::endl;
	    }
	}
    }
}

void DCDBQuery::prepareQuery(std::list<std::string> sensors, std::list<std::string> prefixes) {
    /* Initialize the SensorConfig interface */
    DCDB::SensorConfig sensorConfig(connection);
    
    /* Iterate over list of sensors requested by the user */
    for (std::list<std::string>::iterator it = sensors.begin(); it != sensors.end(); it++) {
	std::string sensorName;
	queryConfig_t queryCfg;
	parseSensorSpecification(*it, sensorName, queryCfg);
	for (auto p: prefixes) {
	    std::string s = p;
	    if (s.back() != '/') {
		s.push_back('/');
	    }
	    s.append(sensorName);
	    if (queryCfg.operation != DCDB_OP_UNKNOWN) {
		std::list <DCDB::PublicSensor> publicSensors;
		sensorConfig.getPublicSensorsByWildcard(publicSensors, s.c_str());
		if (publicSensors.size() > 0) {
		    for (auto sen: publicSensors) {
				if(queryCfg.operation == DCDB_OP_WINTERMUTE) {
					if(sen.operations.find(queryCfg.wintermuteOp) != sen.operations.end()) {
						sen.name = sen.name + queryCfg.wintermuteOp;
						sen.pattern = sen.pattern + queryCfg.wintermuteOp;
					} else {
						std::cerr << "Unknown sensor operation: " << queryCfg.wintermuteOp << std::endl;
						continue;
					}
				}
				queries.insert(std::pair<DCDB::PublicSensor, queryConfig_t>(sen, queryCfg));
		    }
		} else {
		    DCDB::PublicSensor pS;
		    pS.name = s;
		    pS.pattern = s;
			if(queryCfg.operation != DCDB_OP_WINTERMUTE)
				queries.insert(std::pair<DCDB::PublicSensor, queryConfig_t>(pS, queryCfg));
			else
				std::cerr << "Unknown sensor operation: " << queryCfg.wintermuteOp << std::endl;
		}
		}
	    }
	}
}

void DCDBQuery::execute() {
    std::string prevSensorName;
    auto q = queries.begin();
    while (q != queries.end()) {
	if (q->first.name != prevSensorName) {
	    prevSensorName = q->first.name;

	    // Find all queries for the same sensor
	    std::pair<queryMap_t::iterator, queryMap_t::iterator> range = queries.equal_range(q->first);
	    
	    /* Base scaling factor and unit of the public sensor */
	    baseUnit = DCDB::UnitConv::fromString(q->first.unit);
	    baseScalingFactor = q->first.scaling_factor;
	    
	    std::list<DCDB::SensorDataStoreReading> results;
	    DCDB::Sensor sensor(connection, q->first);
	    q = range.second;
	    
	    // Query aggregates first
	    auto it=range.first;
	    while(it!=range.second) {
		if (it->second.aggregate != DCDB::AGGREGATE_NONE) {
		    sensor.query(results, start_ts, end_ts, it->second.aggregate);
		    if (results.size() > 0) {
			genOutput(results, it, std::next(it));
			results.clear();
		    }
		    // Remove the query from the list so it doesn't show up in the raw values below anymore
		    if (it == range.first) {
			range.first = std::next(it);
		    }
		    it = queries.erase(it);
		    continue;
		} else {
		    it++;
		}
	    }

	    // Query raw values next
	    for (auto it=range.first; it!=range.second; it++) {
		if (it->second.aggregate == DCDB::AGGREGATE_NONE) {
		    sensor.query(results, start_ts, end_ts, DCDB::AGGREGATE_NONE);
		    break;
		}
	    }
	    if (results.size() > 0) {
		genOutput(results, range.first, range.second);
		results.clear();
	    }
	}
    }
}

void DCDBQuery::doQuery(std::list<std::string> sensors, DCDB::TimeStamp start, DCDB::TimeStamp end) {
    setInterval(start, end);
    prepareQuery(sensors);
    execute();
}

void DCDBQuery::dojobQuery(std::list<std::string> sensors, std::string jobId) {
    DCDB::JobDataStore jobDataStore(connection);
    DCDB::JobData jobData;
    DCDB::JDError err = jobDataStore.getJobById(jobData, jobId);
    
    if (err == DCDB::JD_OK) {
	setInterval(jobData.startTime, jobData.endTime);
	prepareQuery(sensors, jobData.nodes);
	execute();
    } else {
	std::cerr << "Job not found: " << jobId << std::endl;
    }

}

DCDBQuery::DCDBQuery()
{
  connection = nullptr;
  useLocalTime = true;
  useRawOutput = false;
}
