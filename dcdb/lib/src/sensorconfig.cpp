//================================================================================
// Name        : sensorconfig.cpp
// Author      : Axel Auweter, Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API implementation for configuring libdcdb public sensors.
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
#include <iostream>
#include <algorithm>
#include <string>
#include <list>
#include <unordered_set>
#include <utility>

#include "cassandra.h"
#include <boost/regex.hpp>

#include "sensorconfig_internal.h"
#include "dcdbglobals.h"
#include "dcdbendian.h"
#include "dcdb/virtualsensor.h"
#include "dcdb/libconfig.h"

using namespace DCDB;

/*
 * PublicSensor functions.
 */
PublicSensor::PublicSensor()
{
    name = "";
    is_virtual = false;
    pattern = "";
    scaling_factor = 1.0;
    unit = "";
    sensor_mask = 0;
    expression = "";
    v_sensorid = "";
    t_zero = 0;
    interval = 0;
    ttl = 0;
}

PublicSensor::PublicSensor (const PublicSensor &copy)
{
    name = copy.name;
    is_virtual = copy.is_virtual;
    pattern = copy.pattern;
    scaling_factor = copy.scaling_factor;
    unit = copy.unit;
    sensor_mask = copy.sensor_mask;
    expression = copy.expression;
    v_sensorid = copy.v_sensorid;
    t_zero = copy.t_zero;
    interval = copy.interval;
    operations = copy.operations;
    ttl = copy.ttl;
}

PublicSensor PublicSensor::metadataToPublicSensor(const SensorMetadata& sm) {
    PublicSensor ps;
    if(sm.getPublicName())
        ps.name = *sm.getPublicName();
    if(sm.getIsVirtual())
        ps.is_virtual = *sm.getIsVirtual();
    if(sm.getPattern())
        ps.pattern = *sm.getPattern();
    if(sm.getUnit())
        ps.unit = *sm.getUnit();
    if(sm.getScale())
        ps.scaling_factor = *sm.getScale();
    if(sm.getTTL())
        ps.ttl = *sm.getTTL();
    if(sm.getInterval())
        ps.interval = *sm.getInterval();
    if(sm.getOperations())
        ps.operations = *sm.getOperations();
    uint64_t sensorMask = 0;
    if(sm.getIntegrable() && *sm.getIntegrable())
        sensorMask = sensorMask | INTEGRABLE;
    if(sm.getMonotonic() && *sm.getMonotonic())
        sensorMask = sensorMask | MONOTONIC;
    if(sm.getDelta() && *sm.getDelta())
	sensorMask = sensorMask | DELTA;
    ps.sensor_mask = sensorMask;
    return ps;
}

SensorMetadata PublicSensor::publicSensorToMetadata(const PublicSensor& ps) {
    SensorMetadata sm;
    sm.setPublicName(ps.name);
    sm.setIsVirtual(ps.is_virtual);

    // Stripping whitespace from the sensor pattern in the SID
    string stripPattern = ps.pattern;
    boost::algorithm::trim(stripPattern);
    sm.setPattern(stripPattern);

    sm.setUnit(ps.unit);
    sm.setScale(ps.scaling_factor);
    sm.setTTL(ps.ttl);
    sm.setInterval(ps.interval);
    sm.setOperations(ps.operations);
    sm.setIntegrable(ps.sensor_mask & INTEGRABLE);
    sm.setMonotonic(ps.sensor_mask & MONOTONIC);
    sm.setDelta(ps.sensor_mask & DELTA);
    return sm;
}


/*
 * SensorConfig functions
 */

SCError SensorConfig::loadCache()
{
    return impl->loadCache();
}

SCError SensorConfig::publishSensor(const char* publicName, const char* sensorPattern)
{
    return impl->publishSensor(publicName, sensorPattern);
}

SCError SensorConfig::publishSensor(const PublicSensor& sensor)
{
    return impl->publishSensor(sensor);
}

SCError SensorConfig::publishSensor(const SensorMetadata& sensor)
{
    return impl->publishSensor(sensor);
}

SCError SensorConfig::publishVirtualSensor(const char* publicName, const char* vSensorExpression, const char* vSensorId, TimeStamp tZero, uint64_t interval)
{
    return impl->publishVirtualSensor(publicName, vSensorExpression, vSensorId, tZero, interval);
}

SCError SensorConfig::unPublishSensor(const char* publicName)
{
    return impl->unPublishSensor(publicName);
}

SCError SensorConfig::unPublishSensorsByWildcard(const char* wildcard)
{
    return impl->unPublishSensorsByWildcard(wildcard);
}

SCError SensorConfig::getPublicSensorNames(std::list<std::string>& publicSensors)
{
    return impl->getPublicSensorNames(publicSensors);
}

SCError SensorConfig::getPublicSensorsVerbose(std::list<PublicSensor>& publicSensors)
{
    return impl->getPublicSensorsVerbose(publicSensors);
}

SCError SensorConfig::getPublicSensorByName(PublicSensor& sensor, const char* publicName)
{
    return impl->getPublicSensorByName(sensor, publicName);
}

SCError SensorConfig::getPublicSensorsByWildcard(std::list<PublicSensor>& sensors, const char* wildcard)
{
    return impl->getPublicSensorsByWildcard(sensors, wildcard);
}

SCError SensorConfig::isVirtual(bool& isVirtual, std::string publicName)
{
    return impl->isVirtual(isVirtual, publicName);
}

SCError SensorConfig::setSensorScalingFactor(std::string publicName, double scalingFactor) {
    return impl->setSensorScalingFactor(publicName, scalingFactor);
}

SCError SensorConfig::setSensorUnit(std::string publicName, std::string unit)
{
    return impl->setSensorUnit(publicName, unit);
}

SCError SensorConfig::setSensorMask(std::string publicName, uint64_t mask)
{
    return impl->setSensorMask(publicName, mask);
}

SCError SensorConfig::setOperations(std::string publicName, std::set<std::string> operations)
{
    return impl->setOperations(publicName,operations);
}

SCError SensorConfig::clearOperations(std::string publicName)
{
    return impl->clearOperations(publicName);
}

SCError SensorConfig::clearOperationsByWildcard(std::string wildcard)
{
    return impl->clearOperationsByWildcard(wildcard);
}

SCError SensorConfig::setTimeToLive(std::string publicName, uint64_t ttl)
{
    return impl->setTimeToLive(publicName, ttl);
}

SCError SensorConfig::setVirtualSensorExpression(std::string publicName, std::string expression)
{
    return impl->setVirtualSensorExpression(publicName, expression);
}

SCError SensorConfig::setVirtualSensorTZero(std::string publicName, TimeStamp tZero)
{
    return impl->setVirtualSensorTZero(publicName, tZero);
}

SCError SensorConfig::setSensorInterval(std::string publicName, uint64_t interval)
{
    return impl->setSensorInterval(publicName, interval);
}

SCError SensorConfig::getPublishedSensorsWritetime(uint64_t &ts) {
    return impl->getPublishedSensorsWritetime(ts);
}

SCError SensorConfig::setPublishedSensorsWritetime(const uint64_t &ts) {
    return impl->setPublishedSensorsWritetime(ts);
}

SensorConfig::SensorConfig(Connection* conn)
{
    /* Allocate impl object */
    impl = new SensorConfigImpl(conn);
}

SensorConfig::~SensorConfig()
{
    if (impl) {
        delete impl;
    }
}

/*
 * SensorConfigImpl protected members and functions
 */

/*
 * Validate the pattern for a Sensor to be published
 * Patterns may only consist of hex numbers and forward slashes
 * and at most one wildcard character (*). Also, the total number of
 * bits may not exceed 128.
 */
bool SensorConfigImpl::validateSensorPattern(const char* sensorPattern)
{
    unsigned int wildcards, seps;

    /* Iterate through the string and validate/count the characters */
    wildcards = 0, seps = 0;
    for (unsigned int c = 0; c < strlen(sensorPattern); c++) {
        switch (sensorPattern[c]) {
            case '*':
                wildcards++;
                break;
            case '/':
                seps++;
                break;
                /* Everything else is allowed */
            default:
                break;
        }
    }

    /* More than one wildcard is not allowed */
    if (wildcards > 1)
        return false;

    if (strlen(sensorPattern) - seps > MAX_PATTERN_LENGTH)
        return false;

    /* Looks good */
    return true;
}

/*
 * Validate the public name of a sensor.
 */
bool SensorConfigImpl::validateSensorPublicName(std::string publicName)
{
    return true;
}

/*
 * SensorConfigImpl public functions
 */

SCError SensorConfigImpl::loadCache()
{
    if (sensorList.size() != 0) {
	return SC_OK;
    } else {
#ifdef DEBUG
	TimeStamp ts;
#endif
	SCError ret = getPublicSensorNames(sensorList);
#ifdef DEBUG
	std::cerr << "loadCache: " << sensorList.size() << " sensors loaded in " << TimeStamp().getRaw() - ts.getRaw() << " ns" << std::endl;
#endif
	return ret;
    }
}

SCError SensorConfigImpl::getClusterName(std::string &name)
{
    name = "";
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Fill the list with all public sensors */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const char* query = "SELECT cluster_name FROM system.local;";

    statement = cass_statement_new(query, 0);

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc == CASS_OK) {
        const CassResult* result = cass_future_get_result(future);
        CassIterator* iterator = cass_iterator_from_result(result);

        if (cass_iterator_next(iterator)) {
            const char* nameStr;
            size_t      nameStr_len;

            const CassRow* row = cass_iterator_get_row(iterator);

            if (cass_value_get_string(cass_row_get_column_by_name(row, "cluster_name"), &nameStr, &nameStr_len) == CASS_OK) {
                name = std::string(nameStr, nameStr_len);
            }
        }
        cass_result_free(result);
        cass_iterator_free(iterator);
    } else {
        connection->printError(future);
        cass_future_free(future);
        cass_statement_free(statement);
        return SC_UNKNOWNERROR;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    return SC_OK;
}

SCError SensorConfigImpl::getPublishedSensorsWritetime(uint64_t &ts)
{
    ts = 0;
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }
    
    /* Fill the list with all public sensors */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const char* query = "SELECT value FROM " CONFIG_KEYSPACE_NAME "." CF_MONITORINGMETADATA " where name=\'" CF_PROPERTY_PSWRITETIME "\' ;";

    statement = cass_statement_new(query, 0);
    
    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc == CASS_OK) {
        const CassResult* result = cass_future_get_result(future);
        CassIterator* iterator = cass_iterator_from_result(result);

        if (cass_iterator_next(iterator)) {
            const char* tsStr;
            size_t      tsStr_len;

            const CassRow* row = cass_iterator_get_row(iterator);

            if (cass_value_get_string(cass_row_get_column_by_name(row, "value"), &tsStr, &tsStr_len) == CASS_OK) {
                try {
                    ts = TimeStamp(std::string(tsStr, tsStr_len)).getRaw();
                } catch(const std::exception &e) {
                    ts = 0;
                }
            }
        }
        cass_result_free(result);
        cass_iterator_free(iterator);
    } else {
        connection->printError(future);
        cass_future_free(future);
        cass_statement_free(statement);
        return SC_UNKNOWNERROR;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    return SC_OK;
}

SCError SensorConfigImpl::setPublishedSensorsWritetime(const uint64_t &ts)
{
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Insert the entry */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "INSERT INTO " CONFIG_KEYSPACE_NAME "." CF_MONITORINGMETADATA " (name, value) VALUES (?,?);";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    } else {
        prepared = cass_future_get_prepared(future);
    }

    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_string_by_name(statement, "name", CF_PROPERTY_PSWRITETIME);
    cass_statement_bind_string_by_name(statement, "value", std::to_string(ts).c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);

        cass_prepared_free(prepared);
        cass_future_free(future);
        cass_statement_free(statement);

        return SC_UNKNOWNERROR;
    }

    cass_prepared_free(prepared);
    cass_future_free(future);
    cass_statement_free(statement);

    return SC_OK;
}

SCError SensorConfigImpl::publishSensor(std::string publicName, std::string sensorPattern)
{
    /* Check if the pattern matches the requirements */
    SensorId sid;
    if (!validateSensorPattern(sensorPattern.c_str()) || !sid.mqttTopicConvert(sensorPattern)) {
        return SC_INVALIDPATTERN;
    }

    /* Check if the publicName is valid */
    if (!validateSensorPublicName(publicName.c_str())) {
        return SC_INVALIDPUBLICNAME;
    }

    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }
    
    /* Insert the entry */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "INSERT INTO " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " (name, pattern, virtual) VALUES (?,?, FALSE);";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    } else {
        prepared = cass_future_get_prepared(future);
    }

    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_string_by_name(statement, "name", publicName.c_str());
    cass_statement_bind_string_by_name(statement, "pattern", sid.getId().c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);

        cass_prepared_free(prepared);
        cass_future_free(future);
        cass_statement_free(statement);

        return SC_UNKNOWNERROR;
    }

    cass_prepared_free(prepared);
    cass_future_free(future);
    cass_statement_free(statement);

    return SC_OK;
}

SCError SensorConfigImpl::publishSensor(const PublicSensor& sensor)
{
    /* Check if the pattern matches the requirements */
    SensorId sid;
    if (!validateSensorPattern(sensor.pattern.c_str()) || !sid.mqttTopicConvert(sensor.pattern)) {
        return SC_INVALIDPATTERN;
    }

    /* Check if the publicName is valid */
    if (!validateSensorPublicName(sensor.name.c_str())) {
        return SC_INVALIDPUBLICNAME;
    }

    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Insert the entry */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "INSERT INTO " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " (name, pattern, virtual, scaling_factor, unit, sensor_mask, interval, ttl) VALUES (?,?, FALSE, ?, ?, ?, ?, ?);";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    } else {
        prepared = cass_future_get_prepared(future);
    }

    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_string_by_name(statement, "name", sensor.name.c_str());
    cass_statement_bind_string_by_name(statement, "pattern", sid.getId().c_str());
    cass_statement_bind_double_by_name(statement, "scaling_factor", sensor.scaling_factor);
    cass_statement_bind_string_by_name(statement, "unit", sensor.unit.c_str());
    cass_statement_bind_int64_by_name(statement, "sensor_mask", sensor.sensor_mask);
    cass_statement_bind_int64_by_name(statement, "interval", sensor.interval);
    cass_statement_bind_int64_by_name(statement, "ttl", sensor.ttl);
    
    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);

        cass_prepared_free(prepared);
        cass_future_free(future);
        cass_statement_free(statement);

        return SC_UNKNOWNERROR;
    }

    cass_prepared_free(prepared);
    cass_future_free(future);
    cass_statement_free(statement);

    // Operations are inserted as an update statement, if required
    if(sensor.operations.size() > 0)
        return setOperations(sensor.name, sensor.operations);
    else
        return SC_OK;
}

SCError SensorConfigImpl::publishSensor(const SensorMetadata& sensor)
{
    /* Check if the pattern matches the requirements */
    SensorId sid;
    if (!sensor.getPattern() || !validateSensorPattern(sensor.getPattern()->c_str()) || !sid.mqttTopicConvert(*sensor.getPattern())) {
        return SC_INVALIDPATTERN;
    }

    /* Check if the publicName is valid */
    if (!sensor.getPublicName() || !validateSensorPublicName(sensor.getPublicName()->c_str())) {
        return SC_INVALIDPUBLICNAME;
    }

    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Insert the entry */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    
    std::string queryBuf = "INSERT INTO " + std::string(CONFIG_KEYSPACE_NAME) + "." + std::string(CF_PUBLISHEDSENSORS) + " (name, pattern, virtual";
    std::string valuesBuf = ") VALUES (?, ?, FALSE";
    std::string closingBuf = ");";
            
    if(sensor.getScale()) {
        queryBuf += ", scaling_factor";
        valuesBuf += ", ?";
    }
    if(sensor.getUnit()) {
        queryBuf += ", unit";
        valuesBuf += ", ?";
    }
    if(sensor.getIntegrable() || sensor.getMonotonic() || sensor.getDelta()) {
        queryBuf += ", sensor_mask";
        valuesBuf += ", ?";
    }
    if(sensor.getInterval()) {
        queryBuf += ", interval";
        valuesBuf += ", ?";
    }
    if(sensor.getTTL()) {
        queryBuf += ", ttl";
        valuesBuf += ", ?";
    }
    
    std::string query = queryBuf + valuesBuf + closingBuf;

    future = cass_session_prepare(session, query.c_str());
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    } else {
        prepared = cass_future_get_prepared(future);
    }

    cass_future_free(future);

    statement = cass_prepared_bind(prepared);
    cass_statement_bind_string_by_name(statement, "name", sensor.getPublicName()->c_str());
    cass_statement_bind_string_by_name(statement, "pattern", sid.getId().c_str());
    
    if(sensor.getScale())
        cass_statement_bind_double_by_name(statement, "scaling_factor", *sensor.getScale());
    if(sensor.getUnit())
        cass_statement_bind_string_by_name(statement, "unit", sensor.getUnit()->c_str());
    if(sensor.getInterval())
        cass_statement_bind_int64_by_name(statement, "interval", *sensor.getInterval());
    if(sensor.getTTL())
        cass_statement_bind_int64_by_name(statement, "ttl", *sensor.getTTL());
    if(sensor.getIntegrable() || sensor.getMonotonic() || sensor.getDelta()) {
        uint64_t sensorMask = 0;
        if(sensor.getIntegrable())
            sensorMask = sensorMask | INTEGRABLE;
        if(sensor.getMonotonic())
            sensorMask = sensorMask | MONOTONIC;
	if(sensor.getDelta())
	    sensorMask = sensorMask | DELTA;
        cass_statement_bind_int64_by_name(statement, "sensor_mask", sensorMask);
    }
    
    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);

        cass_prepared_free(prepared);
        cass_future_free(future);
        cass_statement_free(statement);

        return SC_UNKNOWNERROR;
    }

    cass_prepared_free(prepared);
    cass_future_free(future);
    cass_statement_free(statement);
    
    // Operations are inserted as an update statement, if required
    if(sensor.getOperations() && sensor.getOperations()->size()>0)
        return setOperations(*sensor.getPublicName(), *sensor.getOperations());
    else
        return SC_OK;
}

SCError SensorConfigImpl::publishVirtualSensor(std::string publicName, std::string vSensorExpression, std::string vSensorId, TimeStamp tZero, uint64_t interval)
{
    /* Check if the publicName is valid */
    if (!validateSensorPublicName(publicName.c_str())) {
        return SC_INVALIDPUBLICNAME;
    }

    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Validate vSensorExpression */
    try {
        VSensorExpression vsExp(connection, vSensorExpression);

        /* Check that it is not recursive-pointing to itself */
        std::unordered_set<std::string> inputSet;
        vsExp.getInputsRecursive(inputSet);
        std::unordered_set<std::string>::const_iterator found = inputSet.find(publicName);
        if (found != inputSet.end()) {
            return SC_EXPRESSIONSELFREF;
        }
    }
    catch (std::exception& e) {
        std::cout << e.what();
        return SC_INVALIDEXPRESSION;
    }

    /* Check if the vSensorId is valid */
    SensorId vSensor;
    if (!vSensor.mqttTopicConvert(vSensorId)) {
        return SC_INVALIDVSENSORID;
    }

    /* Insert the entry */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "INSERT INTO " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " (name, expression, vsensorid, tzero, interval, virtual) VALUES (?,?,?,?,?,TRUE);";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    } else {
        prepared = cass_future_get_prepared(future);
    }

    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_string_by_name(statement, "name", publicName.c_str());
    cass_statement_bind_string_by_name(statement, "expression", vSensorExpression.c_str());
    cass_statement_bind_string_by_name(statement, "vsensorid", vSensorId.c_str());
    cass_statement_bind_int64_by_name(statement, "tzero", tZero.getRaw());
    cass_statement_bind_int64_by_name(statement, "interval", interval);

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);

        cass_prepared_free(prepared);
        cass_future_free(future);
        cass_statement_free(statement);

        return SC_UNKNOWNERROR;
    }

    cass_prepared_free(prepared);
    cass_future_free(future);
    cass_statement_free(statement);

    return SC_OK;
}


SCError SensorConfigImpl::unPublishSensor(std::string publicName)
{
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Remove the entry */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "DELETE FROM " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    } else {
        prepared = cass_future_get_prepared(future);
    }

    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_string_by_name(statement, "name", publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);

        cass_prepared_free(prepared);
        cass_future_free(future);
        cass_statement_free(statement);

        return SC_UNKNOWNERROR;
    }

    cass_prepared_free(prepared);
    cass_future_free(future);
    cass_statement_free(statement);

    return SC_OK;
}

SCError SensorConfigImpl::unPublishSensorsByWildcard(std::string wildcard)
{
    std::list<PublicSensor> sensors;
    if(getPublicSensorsByWildcard(sensors, wildcard.c_str())!=SC_OK)
        return SC_UNKNOWNERROR;
    
    for(const auto& s : sensors) {
        if (unPublishSensor(s.name.c_str()) != SC_OK) {
            return SC_UNKNOWNERROR;
        }
    }
        
    return SC_OK;
}

SCError SensorConfigImpl::getPublicSensorNames(std::list<std::string>& publicSensors)
{
#ifdef USE_SENSOR_CACHE
    // If we have a valid cache file, we bypass querying the database
    if(findSensorCachePath() == SC_OK && loadNamesFromFile(publicSensors) == SC_OK) {
        return SC_OK;
    }
#endif
    
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Clear the list */
    publicSensors.clear();

    /* Fill the list with all public sensors */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const char* query = "SELECT name FROM " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " ;";

    statement = cass_statement_new(query, 0);
    cass_statement_set_paging_size(statement, PAGING_SIZE);

    bool morePages = false;
    do {
        future = cass_session_execute(session, statement);
        cass_future_wait(future);
    
        rc = cass_future_error_code(future);
        if (rc == CASS_OK) {
            const CassResult* result = cass_future_get_result(future);
            CassIterator* iterator = cass_iterator_from_result(result);
    
            while (cass_iterator_next(iterator)) {
                const char* name;
                size_t      name_len;
    
                const CassRow* row = cass_iterator_get_row(iterator);
    
                if (cass_value_get_string(cass_row_get_column_by_name(row, "name"), &name, &name_len) != CASS_OK) {
                    name = ""; name_len = 0;
                }
    
                publicSensors.push_back(std::string(name, name_len));
            }

            if((morePages = cass_result_has_more_pages(result)))
                cass_statement_set_paging_state(statement, result);
            
            cass_result_free(result);
            cass_iterator_free(iterator);
        } else {
            connection->printError(future);
            cass_future_free(future);
            cass_statement_free(statement);
            return SC_UNKNOWNERROR;
        }

        cass_future_free(future);
        
    }
    while(morePages);

    cass_statement_free(statement);

#ifdef USE_SENSOR_CACHE
    if(findSensorCachePath() == SC_OK) {
	saveNamesToFile(publicSensors);
    }
#endif
    
    return SC_OK;
}

SCError SensorConfigImpl::getPublicSensorsVerbose(std::list<PublicSensor>& publicSensors)
{
#ifdef USE_SENSOR_CACHE
    // If we have a valid cache file, we bypass querying the database
    if(findSensorCachePath() == SC_OK && loadMetadataFromFile(publicSensors) == SC_OK) {
        return SC_OK;
    }
#endif
    
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Clear the list */
    publicSensors.clear();

    /* Fill the list with all public sensors */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const char* query = "SELECT * FROM " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " ;";

    statement = cass_statement_new(query, 0);
    cass_statement_set_paging_size(statement, PAGING_SIZE);

    bool morePages = false;
    do {
        future = cass_session_execute(session, statement);
        cass_future_wait(future);
    
        rc = cass_future_error_code(future);
        if (rc == CASS_OK) {
            const CassResult* result = cass_future_get_result(future);
            CassIterator* iterator = cass_iterator_from_result(result);
    
            while (cass_iterator_next(iterator)) {
                const char* name;
                size_t      name_len;
                cass_bool_t is_virtual;
                const char* pattern;
                size_t      pattern_len;
                double      scaling_factor;
                const char* unit;
                size_t      unit_len;
                int64_t     sensor_mask;
                const char* expression;
                size_t      expression_len;
                const char* vsensorid;
                size_t      vsensorid_len;
                int64_t     tzero;
                int64_t     interval;
                int64_t     ttl;
                set<string> operations;
                PublicSensor sensor;
    
                const CassRow* row = cass_iterator_get_row(iterator);
    
                if (cass_value_get_string(cass_row_get_column_by_name(row, "name"), &name, &name_len) != CASS_OK) {
                    name = ""; name_len = 0;
                }
                if (cass_value_get_bool(cass_row_get_column_by_name(row, "virtual"), &is_virtual) != CASS_OK) {
                    is_virtual = cass_false;
                }
                if (cass_value_get_string(cass_row_get_column_by_name(row, "pattern"), &pattern, &pattern_len) != CASS_OK) {
                    pattern = ""; pattern_len = 0;
                }
                if (cass_value_get_double(cass_row_get_column_by_name(row, "scaling_factor"), &scaling_factor) != CASS_OK) {
                    scaling_factor = 1.0;
                }
                if (cass_value_get_string(cass_row_get_column_by_name(row, "unit"), &unit, &unit_len) != CASS_OK) {
                    unit = ""; unit_len = 0;
                }
                if (cass_value_get_int64(cass_row_get_column_by_name(row, "sensor_mask"), &sensor_mask) != CASS_OK) {
                    sensor_mask = 0;
                }
                if (cass_value_get_string(cass_row_get_column_by_name(row, "expression"), &expression, &expression_len) != CASS_OK) {
                    expression = ""; expression_len = 0;
                }
                if (cass_value_get_string(cass_row_get_column_by_name(row, "vsensorid"), &vsensorid, &vsensorid_len) != CASS_OK) {
                    vsensorid = ""; vsensorid_len = 0;
                }
                if (cass_value_get_int64(cass_row_get_column_by_name(row, "tzero"), &tzero) != CASS_OK) {
                    tzero = 0;
                }
                if (cass_value_get_int64(cass_row_get_column_by_name(row, "interval"), &interval) != CASS_OK) {
                    interval = 0;
                }
                if (cass_value_get_int64(cass_row_get_column_by_name(row, "ttl"), &ttl) != CASS_OK) {
                    ttl = 0;
                }
    
                const CassValue* opSet = nullptr;
                CassIterator *opSetIt = nullptr;
                if((opSet=cass_row_get_column_by_name(row, "operations")) && (opSetIt=cass_iterator_from_collection(opSet))) {
                    const char *opString;
                    size_t opLen;
    
                    while (cass_iterator_next(opSetIt)) {
                        if (cass_value_get_string(cass_iterator_get_value(opSetIt), &opString, &opLen) != CASS_OK) {
                            operations.clear();
                            break;
                        } else
                            operations.insert(std::string(opString, opLen));
                    }
    
                    cass_iterator_free(opSetIt);
                }
                
                sensor.name = std::string(name, name_len);
                sensor.is_virtual = is_virtual == cass_true ? true : false;
                sensor.pattern = std::string(pattern, pattern_len);
                sensor.scaling_factor = scaling_factor;
                sensor.unit = std::string(unit, unit_len);
                sensor.sensor_mask = sensor_mask;
                sensor.expression = std::string(expression, expression_len);
                sensor.v_sensorid = std::string(vsensorid, vsensorid_len);
                sensor.t_zero = tzero;
                sensor.interval = interval;
                sensor.ttl = ttl;
                sensor.operations = operations;
    
                publicSensors.push_back(sensor);
            }

            if((morePages = cass_result_has_more_pages(result)))
                cass_statement_set_paging_state(statement, result);
            
            cass_result_free(result);
            cass_iterator_free(iterator);
        } else {
            connection->printError(future);
            cass_future_free(future);
            cass_statement_free(statement);
            return SC_UNKNOWNERROR;
        }

        cass_future_free(future);
        
    }
    while(morePages);
    
    cass_statement_free(statement);

#ifdef USE_SENSOR_CACHE
    if(findSensorCachePath() == SC_OK) {
	saveMetadataToFile(publicSensors);
    }
#endif
    
    return SC_OK;
}

SCError SensorConfigImpl::getPublicSensorByName(PublicSensor& sensor, const char* publicName)
{
    /* Check if the sensor definition is already in the cache */
    SensorMap_t::const_iterator got = sensorMapByName.find(publicName);
    if (got != sensorMapByName.end()) {
        sensor = got->second;
        return SC_OK;
    }

    /* Not in cache - query the data base */
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Fill the list with all public sensors */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "SELECT * FROM " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " WHERE name = ?;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    } else {
        prepared = cass_future_get_prepared(future);
    }
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);
    cass_statement_bind_string_by_name(statement, "name", publicName);

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        cass_statement_free(statement);
        cass_prepared_free(prepared);
        return SC_UNKNOWNERROR;
    } else {
        const CassResult* result = cass_future_get_result(future);
        CassIterator* iterator = cass_iterator_from_result(result);

        if (cass_iterator_next(iterator)) {
            const char* name;
            size_t      name_len;
            cass_bool_t is_virtual;
            const char* pattern;
            size_t      pattern_len;
            double      scaling_factor;
            const char* unit;
            size_t      unit_len;
            int64_t     sensor_mask;
            const char* expression;
            size_t      expression_len;
            const char* vsensorid;
            size_t      vsensorid_len;
            int64_t     tzero;
            int64_t     interval;
            int64_t     ttl;
            set<string> operations;

            const CassRow* row = cass_iterator_get_row(iterator);

            if (cass_value_get_string(cass_row_get_column_by_name(row, "name"), &name, &name_len) != CASS_OK) {
                name = ""; name_len = 0;
            }
            if (cass_value_get_bool(cass_row_get_column_by_name(row, "virtual"), &is_virtual) != CASS_OK) {
                is_virtual = cass_false;
            }
            if (cass_value_get_string(cass_row_get_column_by_name(row, "pattern"), &pattern, &pattern_len) != CASS_OK) {
                pattern = ""; pattern_len = 0;
            }
            if (cass_value_get_double(cass_row_get_column_by_name(row, "scaling_factor"), &scaling_factor) != CASS_OK) {
                scaling_factor = 1.0;
            }
            if (cass_value_get_string(cass_row_get_column_by_name(row, "unit"), &unit, &unit_len) != CASS_OK) {
                unit = ""; unit_len = 0;
            }
            if (cass_value_get_int64(cass_row_get_column_by_name(row, "sensor_mask"), &sensor_mask) != CASS_OK) {
                sensor_mask = 0;
            }
            if (cass_value_get_string(cass_row_get_column_by_name(row, "expression"), &expression, &expression_len) != CASS_OK) {
                expression = ""; expression_len = 0;
            }
            if (cass_value_get_string(cass_row_get_column_by_name(row, "vsensorid"), &vsensorid, &vsensorid_len) != CASS_OK) {
                vsensorid = ""; vsensorid_len = 0;
            }
            if (cass_value_get_int64(cass_row_get_column_by_name(row, "tzero"), &tzero) != CASS_OK) {
                tzero = 0;
            }
            if (cass_value_get_int64(cass_row_get_column_by_name(row, "interval"), &interval) != CASS_OK) {
                interval = 0;
            }
            if (cass_value_get_int64(cass_row_get_column_by_name(row, "ttl"), &ttl) != CASS_OK) {
                ttl = 0;
            }

            const CassValue* opSet = nullptr;
            CassIterator *opSetIt = nullptr;
            if((opSet=cass_row_get_column_by_name(row, "operations")) && (opSetIt=cass_iterator_from_collection(opSet))) {
                const char *opString;
                size_t opLen;

                while (cass_iterator_next(opSetIt)) {
                    if (cass_value_get_string(cass_iterator_get_value(opSetIt), &opString, &opLen) != CASS_OK) {
                        operations.clear();
                        break;
                    } else
                        operations.insert(std::string(opString, opLen));
                }

                cass_iterator_free(opSetIt);
            }

            sensor.name = std::string(name, name_len);
            sensor.is_virtual = is_virtual == cass_true ? true : false;
            sensor.pattern = std::string(pattern, pattern_len);
            sensor.scaling_factor = scaling_factor;
            sensor.unit = std::string(unit, unit_len);
            sensor.sensor_mask = sensor_mask;
            sensor.expression = std::string(expression, expression_len);
            sensor.v_sensorid = std::string(vsensorid, vsensorid_len);
            sensor.t_zero = tzero;
            sensor.interval = interval;
            sensor.ttl = ttl;
            sensor.operations = operations;

            /* Add to sensorMap for later use */
            sensorMapByName.insert(std::make_pair(publicName, sensor));
        }
        else {
            cass_result_free(result);
            cass_iterator_free(iterator);
            cass_future_free(future);
            cass_statement_free(statement);
            cass_prepared_free(prepared);
            return SC_UNKNOWNSENSOR;
        }
        cass_result_free(result);
        cass_iterator_free(iterator);
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);
    return SC_OK;
}

SCError SensorConfigImpl::getPublicSensorsByWildcard(std::list<PublicSensor>& sensors, const char* wildcard)
{
    SCError err = SC_OK;

    if (strpbrk(wildcard, "*?") == NULL) {
        PublicSensor sen;
        if ((err = getPublicSensorByName(sen, wildcard)) == SC_OK) {
            sensors.push_back(sen);
        }
        return err;
    } else {
	if (loadCache() == SC_OK) {
            std::string w("^");
            while (*wildcard != 0) {
                switch (*wildcard) {
                    case '.':
                    case '[':
                    case '\\':
                    case '^':
                    case '$':
                        w.append("\\");
                        w.append(wildcard, 1);
                        break;
                    case '?':
                        w.append(".");
                        break;
                    case '*':
                        w.append(".*");
                        break;
                    default:
                        w.append(wildcard, 1);
                        break;
                }
                wildcard++;
            }

            boost::regex r(w, boost::regex::basic);
	    for (auto s: sensorList) {
                if (boost::regex_match(s, r)) {
		    PublicSensor ps;
		    getPublicSensorByName(ps, s.c_str());
		    sensors.push_back(ps);
                }
            }

            if (sensors.size() > 0) {
                return SC_OK;
            } else {
                return SC_UNKNOWNSENSOR;
            }
        } else {
            return err;
        }
    }
}

SCError SensorConfigImpl::isVirtual(bool& isVirtual, std::string publicName)
{
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Read the virtual field from the database */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "SELECT virtual FROM " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    } else {
        prepared = cass_future_get_prepared(future);
    }
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);
    cass_statement_bind_string_by_name(statement, "name", publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
    } else {
        const CassResult* result = cass_future_get_result(future);
        CassIterator* iterator = cass_iterator_from_result(result);

        if (cass_iterator_next(iterator)) {
            cass_bool_t isVirtual_;
            const CassRow* row = cass_iterator_get_row(iterator);
            cass_value_get_bool(cass_row_get_column_by_name(row, "virtual"), &isVirtual_);
            isVirtual = isVirtual_ ? true : false;
        }
        else {
            cass_result_free(result);
            cass_iterator_free(iterator);
            cass_future_free(future);
            cass_statement_free(statement);
            cass_prepared_free(prepared);
            return SC_UNKNOWNSENSOR;
        }

        cass_result_free(result);
        cass_iterator_free(iterator);
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);
    return SC_OK;
}

SCError SensorConfigImpl::setSensorScalingFactor(std::string publicName, double scalingFactor)
{
    SCError error = SC_UNKNOWNERROR;

    /* Check the sensorconfig whether the given public sensor has the integrable flag set to true */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET scaling_factor = ? WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_double(statement, 0, scalingFactor);
    cass_statement_bind_string(statement, 1, publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);

    return error;
}

SCError SensorConfigImpl::setSensorUnit(std::string publicName, std::string unit)
{
    SCError error = SC_UNKNOWNERROR;

    /* Check the sensorconfig whether the given public sensor has the integrable flag set to true */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET unit = ? WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_string(statement, 0, unit.c_str());
    cass_statement_bind_string(statement, 1, publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);

    return error;
}

SCError SensorConfigImpl::setSensorMask(std::string publicName, uint64_t mask)
{
    SCError error = SC_UNKNOWNERROR;

    /* Check the sensorconfig whether the given public sensor has the integrable flag set to true */
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET sensor_mask = ? WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_int64(statement, 0, mask);
    cass_statement_bind_string(statement, 1, publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);

    return error;
}

SCError SensorConfigImpl::setOperations(std::string publicName, std::set<std::string> operations)
{
    SCError error = SC_UNKNOWNERROR;
    
    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET operations = operations + ? WHERE name = ? ;";

    CassCollection* cassSet = cass_collection_new(CASS_COLLECTION_TYPE_SET, operations.size());
    for(const auto& op : operations)
        cass_collection_append_string(cassSet, op.c_str());
    
    future = cass_session_prepare(session, query);
    cass_future_wait(future);
    
    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        cass_collection_free(cassSet);
        return SC_UNKNOWNERROR;
    }
    
    prepared = cass_future_get_prepared(future);
    cass_future_free(future);
    
    statement = cass_prepared_bind(prepared);
    
    cass_statement_bind_collection(statement, 0, cassSet);
    cass_statement_bind_string(statement, 1, publicName.c_str());
    
    future = cass_session_execute(session, statement);
    cass_future_wait(future);
    
    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);
    cass_collection_free(cassSet);
    
    return error;
}

SCError SensorConfigImpl::clearOperations(std::string publicName)
{
    SCError error = SC_UNKNOWNERROR;

    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET operations = {} WHERE name = ? ;";
    
    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_string(statement, 0, publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);

    return error;
}

SCError SensorConfigImpl::clearOperationsByWildcard(std::string wildcard)
{
    std::list<PublicSensor> sensors;
    if(getPublicSensorsByWildcard(sensors, wildcard.c_str())!=SC_OK)
        return SC_UNKNOWNERROR;

    for(const auto& s : sensors) {
        if(clearOperations(s.name) != SC_OK) {
            return SC_UNKNOWNERROR;
        }
    }

    return SC_OK;
}

SCError SensorConfigImpl::setTimeToLive(std::string publicName, uint64_t ttl)
{
    SCError error = SC_UNKNOWNERROR;

    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET ttl = ? WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_int64(statement, 0, ttl);
    cass_statement_bind_string(statement, 1, publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);

    return error;
}
                                                  
SCError SensorConfigImpl::setVirtualSensorExpression(std::string publicName, std::string expression)
{
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Ensure that the public sensor is virtual */
    bool virt;
    SCError err = isVirtual(virt, publicName);
    if (err != SC_OK) {
        return err;
    }
    if (!virt) {
        return SC_WRONGTYPE;
    }

    /* TODO: Validate expression! */

    /* Update the database with the new expression */
    SCError error = SC_UNKNOWNERROR;

    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET expression = ? WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_string(statement, 0, expression.c_str());
    cass_statement_bind_string(statement, 1, publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);

    return error;
}

SCError SensorConfigImpl::setVirtualSensorTZero(std::string publicName, TimeStamp tZero)
{
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Ensure that the public sensor is virtual */
    bool virt;
    SCError err = isVirtual(virt, publicName);
    if (err != SC_OK) {
        return err;
    }
    if (!virt) {
        return SC_WRONGTYPE;
    }

    /* Update the database with the new expression */
    SCError error = SC_UNKNOWNERROR;

    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET tzero = ? WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_int64(statement, 0, tZero.getRaw());
    cass_statement_bind_string(statement, 1, publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);

    return error;
}

SCError SensorConfigImpl::setSensorInterval(std::string publicName, uint64_t interval)
{
    /* Check if the session is valid */
    if (!session) {
        return SC_INVALIDSESSION;
    }

    /* Ensure that the public sensor is virtual */
    //bool virt;
    //SCError err = isVirtual(virt, publicName);
    //if (err != SC_OK) {
    //    return err;
    //}
    //if (!virt) {
    //    return SC_WRONGTYPE;
    //}

    /* Update the database with the new expression */
    SCError error = SC_UNKNOWNERROR;

    CassError rc = CASS_OK;
    CassStatement* statement = nullptr;
    CassFuture* future = nullptr;
    const CassPrepared* prepared = nullptr;
    const char* query = "UPDATE " CONFIG_KEYSPACE_NAME "." CF_PUBLISHEDSENSORS " SET interval = ? WHERE name = ? ;";

    future = cass_session_prepare(session, query);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        cass_future_free(future);
        return SC_UNKNOWNERROR;
    }

    prepared = cass_future_get_prepared(future);
    cass_future_free(future);

    statement = cass_prepared_bind(prepared);

    cass_statement_bind_int64(statement, 0, interval);
    cass_statement_bind_string(statement, 1, publicName.c_str());

    future = cass_session_execute(session, statement);
    cass_future_wait(future);

    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        connection->printError(future);
        error = SC_UNKNOWNERROR;
    }
    else {
        error = SC_OK;
    }

    cass_future_free(future);
    cass_statement_free(statement);
    cass_prepared_free(prepared);

    return error;
}

SCError SensorConfigImpl::isSensorCacheValid(const bool names, bool& isValid, uint64_t& entries) {
    isValid = false;
    entries = 0;
    uint64_t ts = 0;
    std::string stringBuffer;
    uint64_t cacheTs;
    bool cacheNames;
    uint64_t cacheEntries;
    
    if (!session) {
        return SC_INVALIDSESSION;
    }
    if(getPublishedSensorsWritetime(ts) != SC_OK) {
        return SC_UNKNOWNERROR;
    }
    
    std::ifstream cacheFile(_sensorCacheFile);
    if(!cacheFile.is_open()) {
        return SC_CACHEERROR;
    }
    if(!std::getline(cacheFile, stringBuffer)) {
        return SC_CACHEERROR;
    }
    
    try {
        size_t oldPos=0, newPos=0;
        // Parsing cache timestamp
        if((newPos = stringBuffer.find(",", oldPos)) == std::string::npos) {
            return SC_CACHEERROR;
        }
        cacheTs = std::stoull(stringBuffer.substr(oldPos, newPos - oldPos));
        oldPos = newPos + 1;
        
        // Parsing cache type
        if((newPos = stringBuffer.find(",", oldPos)) == std::string::npos) {
            return SC_CACHEERROR;
        }
        cacheNames = stringBuffer.substr(oldPos, newPos - oldPos) == "true";
        cacheEntries = std::stoull(stringBuffer.substr(newPos + 1));
    } catch(const std::exception &e) {
        return SC_CACHEERROR;
    }
    
    cacheFile.close();
    isValid = cacheTs >= ts && cacheEntries > 0 && (cacheNames || cacheNames == names);
    entries = cacheEntries;
    return SC_OK;
}

SCError SensorConfigImpl::findSensorCachePath() {
    if (_sensorCacheFile.size() > 0) {
	return SC_OK;
    }
    
    // Retrieving name of the cluster (just once)
    if(_clusterName == "" && getClusterName(_clusterName) != SC_OK) {
	_clusterName = "";
	return SC_UNKNOWNERROR;
    }

    // Create list of candidates to store the cache file at
    std::list<std::string> candidates;
    if (libConfig.getTempDir().size() > 0) {
	candidates.push_back(libConfig.getTempDir());
    }
    char* homeDir = getenv("HOME");
    if (homeDir != nullptr) {
	candidates.push_back(homeDir + std::string("/.cache"));
    }
    char* tempDir = getenv("TMPDIR");
    if (tempDir != nullptr) {
	candidates.push_back(std::string(tempDir));
    }
    candidates.push_back(std::string("/tmp"));
    
    for (auto c: candidates) {
	// Directory does not exist - we create it
        if (access(c.c_str(), F_OK) != 0 && mkdir(c.c_str(), 0700) != 0) {
	    continue;
        }

        // Directory exists but it is not writeable
        if (access(c.c_str(), W_OK) != 0) {
	    continue;
        }
	
	_sensorCacheFile = c + "/" + std::string(SENSOR_CACHE_FILENAME) + _clusterName;
	return SC_OK;
    }
	
    return SC_PATHERROR;
}

SCError SensorConfigImpl::saveNamesToFile(const std::list<std::string>& publicSensors) {
    std::string stringBuffer = "";
    std::ofstream cacheFile;

    int lfd = acquireCacheLock(true);
    cacheFile.open(_sensorCacheFile);
    
    if(!cacheFile.is_open()) {
        releaseCacheLock(lfd);
        return SC_CACHEERROR;
    }

    try {
        stringBuffer = std::to_string(TimeStamp().getRaw()) + "," + "false" + "," + std::to_string(publicSensors.size());
        cacheFile << stringBuffer << std::endl;
        for(const auto& p : publicSensors) {
            SensorMetadata sm;
            sm.setPublicName(p);
            sm.setPattern(p);
            stringBuffer = sm.getCSV();
            cacheFile << stringBuffer << std::endl;
        }
    } catch(const std::exception& e) {
        cacheFile.close();
        releaseCacheLock(lfd);
        return SC_CACHEERROR;
    }

    cacheFile.close();
    releaseCacheLock(lfd);
    // std::cout << "Saved " << publicSensors.size() << " sensors to name cache..." << std::endl;
    return SC_OK;
}

SCError SensorConfigImpl::saveMetadataToFile(const std::list<PublicSensor>& publicSensors) {
    std::string stringBuffer = "";
    std::ofstream cacheFile;

    int lfd = acquireCacheLock(true);
    cacheFile.open(_sensorCacheFile);

    if(!cacheFile.is_open()) {
        releaseCacheLock(lfd);
        return SC_CACHEERROR;
    }

    try {
        stringBuffer = std::to_string(TimeStamp().getRaw()) + "," + "true" + "," + std::to_string(publicSensors.size());
        cacheFile << stringBuffer << std::endl;
        for(const auto& p : publicSensors) {
            stringBuffer = PublicSensor::publicSensorToMetadata(p).getCSV();
            cacheFile << stringBuffer << std::endl;
        }
    } catch(const std::exception& e) {
        cacheFile.close();
        releaseCacheLock(lfd);
        return SC_CACHEERROR;
    }
    
    cacheFile.close();
    releaseCacheLock(lfd);
    // std::cout << "Saved " << publicSensors.size() << " sensors to metadata cache..." << std::endl;
    return SC_OK;
}

SCError SensorConfigImpl::loadNamesFromFile(std::list<std::string>& publicSensors) {
    std::string stringBuffer = "";
    std::ifstream cacheFile;
    bool valid = true;
    uint64_t entries = 0;
    
    int lfd = acquireCacheLock(false);
    cacheFile.open(_sensorCacheFile);
    
    if(isSensorCacheValid(false, valid, entries) != SC_OK || !valid) {
        releaseCacheLock(lfd);
        return SC_OBSOLETECACHE;
    }
    if(!cacheFile.is_open() || !std::getline(cacheFile, stringBuffer)) {
        cacheFile.close();
        releaseCacheLock(lfd);
        return SC_CACHEERROR;
    }
    
    uint64_t sCtr = 0;
    publicSensors.clear();
    while(std::getline(cacheFile, stringBuffer)) {
        SensorMetadata sm;
        try {
            sm.parseCSV(stringBuffer);
            if (sm.isValid()) {
                publicSensors.push_back(*sm.getPublicName());
                sCtr++;
            }
        } catch(const std::exception& e) {}
    }

    cacheFile.close();
    releaseCacheLock(lfd);
    if(sCtr != entries) {
        publicSensors.clear();
        return SC_CACHEERROR;
    } else {
        // std::cout << "Loaded " << publicSensors.size() << " sensors from names cache..." << std::endl;
        return SC_OK;    
    }
}

SCError SensorConfigImpl::loadMetadataFromFile(std::list<PublicSensor>& publicSensors) {
    std::string stringBuffer = "";
    std::ifstream cacheFile;
    bool valid = true;
    uint64_t entries = 0;

    int lfd = acquireCacheLock(false);
    cacheFile.open(_sensorCacheFile);
    
    if(isSensorCacheValid(true, valid, entries) != SC_OK || !valid) {
        releaseCacheLock(lfd);
        return SC_OBSOLETECACHE;
    }
    if(!cacheFile.is_open() || !std::getline(cacheFile, stringBuffer)) {
        cacheFile.close();
        releaseCacheLock(lfd);
        return SC_CACHEERROR;
    }
    
    uint64_t sCtr = 0;
    publicSensors.clear();
    while(std::getline(cacheFile, stringBuffer)) {
        SensorMetadata sm;
        try {
            sm.parseCSV(stringBuffer);
            if (sm.isValid()) {
                publicSensors.push_back(PublicSensor::metadataToPublicSensor(sm));
                sCtr++;
            }
        } catch(const std::exception& e) {}
    }

    cacheFile.close();
    releaseCacheLock(lfd);
    if(sCtr != entries) {
        publicSensors.clear();
        return SC_CACHEERROR;
    } else {
        // std::cout << "Loaded " << publicSensors.size() << " sensors from metadata cache..." << std::endl;
        return SC_OK;
    }
}

int SensorConfigImpl::acquireCacheLock(const bool write) {
    struct flock fl;
    fl.l_type = write ? F_WRLCK : F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    
    if(_sensorCacheFile == "") {
        return -1;
    }
    
    std::string lockPath = _sensorCacheFile + "_lock_file";
    int l_fd = open(lockPath.c_str(), O_RDWR | O_CREAT, S_IRWXU);
    
    if(l_fd < 0 || fcntl(l_fd, F_SETLKW, &fl) < 0) {
        return -1;
    }

    // std::cout << "Successfully acquired cache " << (write ? "write" : "read") << " lock..." << std::endl;
    return l_fd;
}

int SensorConfigImpl::releaseCacheLock(const int fd) {
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if(fd < 0) {
        return -1;
    }
    
    if(fcntl(fd, F_SETLKW, &fl) < 0 || close(fd) < 0) {
        return -1;
    }
    
    // std::cout << "Successfully released cache lock..." << std::endl;
    return 0;
}

SensorConfigImpl::SensorConfigImpl(Connection* conn)
{
    connection = conn;
    session = connection->getSessionHandle();
    _clusterName = "";
}

SensorConfigImpl::~SensorConfigImpl()
{
    connection = nullptr;
    session = nullptr;
}
