//================================================================================
// Name        : PerSystSqlOperator.cpp
// Author      : Carla Guillen
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Template implementing features to use Units in Operators.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
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

#include "PerSystSqlOperator.h"

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/parameter/keyword.hpp>
#include <stddef.h>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <numeric>
#include <sstream>

#include "../../../common/include/logging.h"
#include "../../../common/include/sensorbase.h"
#include "../../../common/include/timestamp.h"
#include "../../includes/CommonStatistics.h"
#include "../../includes/QueryEngine.h"
#include "../../includes/UnitTemplate.h"


PerSystSqlOperator::PerSystSqlOperator(const std::string& name) :
		OperatorTemplate(name), JobOperatorTemplate(name), _number_of_even_quantiles(
				0), _batch_domain(-1), _severity_formula(NOFORMULA), _severity_threshold(0), _severity_exponent(
				0), _severity_max_memory(0), _go_back_ns(0), _backend(DEFAULT), _scaling_factor(
				1), _searchedOnceForMetaData(false), _property_id(0) {
	_persystdb = MariaDB::getInstance();
}

PerSystSqlOperator::~PerSystSqlOperator() {
}


PerSystSqlOperator::PerSystSqlOperator(const PerSystSqlOperator& other) : OperatorTemplate(other._name), JobOperatorTemplate<AggregatorSensorBase>(other._name){
	copy(other);
}

PerSystSqlOperator& PerSystSqlOperator::operator=(const PerSystSqlOperator& other){
	JobOperatorTemplate::operator=(other);
	copy(other);
	return *this;
}

void PerSystSqlOperator::copy(const PerSystSqlOperator& other){
	this->_buffer = other._buffer;
	this->_quantileSensors = other._quantileSensors;
	this->_number_of_even_quantiles = other._number_of_even_quantiles;
	this->_batch_domain = other._batch_domain;
	this->_severity_formula = other._severity_formula;
	this->_severity_threshold = other._severity_threshold;
	this->_severity_exponent = other._severity_exponent;
	this->_severity_max_memory = other._severity_max_memory;
	this->_severities = other._severities;
	this->_go_back_ns = other._go_back_ns;
	this->_backend = other._backend;
	this->_scaling_factor = other._scaling_factor;
	this->_conn.database_name = other._conn.database_name;
	this->_conn.every_x_days = other._conn.every_x_days;
	this->_conn.host = other._conn.host;
	this->_conn.password = other._conn.password;
	this->_conn.port = other._conn.port;
	this->_conn.rotation = other._conn.rotation;
	this->_conn.user = other._conn.user;
	this->_property_id = other._property_id;
	this->_persystdb = other._persystdb;
	this->_searchedOnceForMetaData = other._searchedOnceForMetaData;
}

void PerSystSqlOperator::printConfig(LOG_LEVEL ll) {
	OperatorTemplate<AggregatorSensorBase>::printConfig(ll);
	LOG_VAR(ll) << "====================================";
	LOG_VAR(ll) << "PerSystSQL Operator " << _name;
	LOG_VAR(ll) << "====================================";
	LOG_VAR(ll) << "backend=" << _backend;
	LOG_VAR(ll) << "go_back_ms=" << _go_back_ns/1e6;
	LOG_VAR(ll) << "scaling_factor=" << _scaling_factor;
	LOG_VAR(ll) << "batch_domain=" << _batch_domain;
	if(_backend == MARIADB){
		LOG_VAR(ll) << "PerSystSQL Operator Connection information:";
		LOG_VAR(ll) << "\tHost=" << _conn.host;
		LOG_VAR(ll) << "\tUser=" << _conn.user;
		LOG_VAR(ll) << "\tDatabase=" << _conn.database_name;
		LOG_VAR(ll) << "\tPort=" << _conn.port;
		LOG_VAR(ll) << "\tRotation=" << _conn.rotation;
		if(_conn.rotation == MariaDB::EVERY_XDAYS){
			LOG_VAR(ll) << "\tEvery_X_days=" << _conn.every_x_days;
		}
	}
	LOG_VAR(ll) << "Property Configuration:";
	LOG_VAR(ll) << "\tnumber_of_even_quantiles=" << _number_of_even_quantiles;
	LOG_VAR(ll) << "\tproperty_id=" << _property_id;
	LOG_VAR(ll) << "Severity Configuration:";
	LOG_VAR(ll) << "\tseverity_formula=" << _severity_formula;
	LOG_VAR(ll) << "\tseverity_exponent=" << _severity_exponent;
	LOG_VAR(ll) << "\tseverity_threshold=" << _severity_threshold;
	LOG_VAR(ll) << "\tseverity_max_memory=" << _severity_max_memory;
}


bool PerSystSqlOperator::execOnStart(){
	if( _backend == MARIADB ) {
		if(!_persystdb->initializeConnection(_conn.host, _conn.user, _conn.password, _conn.database_name, _conn.rotation, _conn.port, _conn.every_x_days)){
			LOG(error) << "Database not initialized";
			return false;
		}
	}
	return true;
}

void PerSystSqlOperator::execOnStop(){
	if( _backend == MARIADB ) {
		_persystdb->finalizeConnection();
	}
}

void PerSystSqlOperator::compute(U_Ptr unit, qeJobData& jobData) {
	// Clearing the buffer, if already allocated
	_buffer.clear();
	size_t elCtr = 0;
	uint64_t tolerance_ms = (uint64_t)_interval;
	uint64_t my_timestamp = getTimestamp() - _go_back_ns;
	
	// Too early to fetch data for the job
	if(my_timestamp < jobData.startTime)
	    return;
	// We snap the timestamp to the job's end time if outside of its boundaries
	else if(jobData.endTime!=0 && my_timestamp > jobData.endTime)
	    my_timestamp = jobData.endTime;
	
	// Job units are hierarchical, and thus we iterate over all sub-units associated to each single node
	std::vector<std::string> vectorOfSensorNames;
	for (const auto& subUnit : unit->getSubUnits()) {
		// Since we do not clear the internal buffer, all sensor readings will be accumulated in the same vector
		for (const auto& in : subUnit->getInputs()) {
			if(!_searchedOnceForMetaData){
				SensorMetadata buffer;
				if(_queryEngine.queryMetadata(in->getName(), buffer) && buffer.getScale()){
					_scaling_factor = *buffer.getScale();
					LOG(debug) << "PerSystSql Operator " << _name << " using scaling factor of " << _scaling_factor;
					_searchedOnceForMetaData = true;
				}
			}
			vectorOfSensorNames.push_back(in->getName());
		}
	}
	if(vectorOfSensorNames.size() == 0 ){
		LOG(debug) << "PerSystSql Operator: No names found for vectorOfSensorNames ";
		return;
	}
	if (!_queryEngine.querySensor(vectorOfSensorNames, my_timestamp, my_timestamp, _buffer, false, tolerance_ms * 1000000)) {
		LOG(debug)<< "PerSystSql Operator " << _name << " cannot read vector sensor " << (*vectorOfSensorNames.begin()) << "!";
	}
	uint64_t measurement_ts = 0;
	if(_buffer.size() == 0){
		LOG(debug) << "PerSystSql Operator " << _name << ": no data in queryEngine found!";
		return;
	} else {
		measurement_ts = _buffer[0].timestamp;
		if(measurement_ts < jobData.startTime){
			LOG(debug) << "PerSystSQL Operator: timestamps not part of job.";
			return; 
		}
	}
	Aggregate_info_t agg_info;
	agg_info.timestamp = (measurement_ts/1e9);
	compute_internal(unit, _buffer, agg_info, measurement_ts);

	if( _backend == MARIADB ) {
		if (!_persystdb->isInitialized()
				&& !_persystdb->initializeConnection(_conn.host, _conn.user, _conn.password, _conn.database_name, _conn.rotation,
						_conn.port, _conn.every_x_days)) {
			LOG(error) << "Database not initialized";
			return;
		}

		std::string table_suffix;
		if(!_persystdb->getTableSuffix(table_suffix)){
			LOG(error) << "Failed to create Aggregate table!";
			return;
		}
		if(!_persystdb->getDBJobID(jobData.jobId, agg_info.job_id_db, jobData.userId, jobData.nodes.size(), _batch_domain) &&
	       	!_persystdb->insertIntoJob(jobData.jobId, jobData.userId, agg_info.job_id_db, table_suffix, jobData.nodes.size(), _batch_domain) ){
           		LOG(error) << "Job insertion not possible, no job id db available for slurm job id" << jobData.jobId;
        		return;
		}

		_persystdb->updateJobsLastSuffix(jobData.jobId, jobData.userId, jobData.nodes.size(), agg_info.job_id_db, table_suffix);

		_persystdb->insertInAggregateTable(table_suffix, agg_info, jobData.jobId);

	}
}

void PerSystSqlOperator::convertToDoubles(std::vector<reading_t> &buffer, std::vector<double> &douBuffer){
	for(auto &reading: buffer){
		double value = reading.value * _scaling_factor;
		douBuffer.push_back(value);
	}
}


void PerSystSqlOperator::compute_internal(U_Ptr& unit, vector<reading_t>& buffer, Aggregate_info_t & agg_info, uint64_t measurement_ts) {
	_quantileSensors.clear();

	reading_t reading;
	AggregatorSensorBase::aggregationOps_t op;
	reading.timestamp = measurement_ts;

	std::vector<double> douBuffer;
	convertToDoubles(buffer, douBuffer);
	// Performing the actual aggregation operation
	for (const auto& out : unit->getOutputs()) {
		op = out->getOperation();
		if (op != AggregatorSensorBase::QTL) {
			switch (op) {
			case AggregatorSensorBase::AVG:
				if (_backend == CASSANDRA) {
					reading.value = std::accumulate(douBuffer.begin(), douBuffer.end(), 0.0)/(douBuffer.size() * _scaling_factor);
				} else {
					agg_info.average = std::accumulate(douBuffer.begin(), douBuffer.end(), 0.0)/douBuffer.size();
				}
				break;
			case AggregatorSensorBase::OBS:
				reading.value = computeObs(buffer);
				agg_info.num_of_observations = computeObs(buffer);
				break;
			case AggregatorSensorBase::AVG_SEV:
				if (_backend == CASSANDRA) {
					reading.value = computeSeverityAverage(douBuffer) * SCALING_FACTOR_SEVERITY;
				} else {
					agg_info.severity_average = computeSeverityAverage(douBuffer);
				}
				break;
			default:
				LOG(warning)<< _name << ": Operation " << op << " not supported!";
				reading.value = 0;
				break;
			}
			if(_backend == CASSANDRA) {
				out->storeReading(reading);
			}
		} else {
			_quantileSensors.push_back(out);
		}
	}

	if (!_quantileSensors.empty()) {
		vector<double> quantiles;
		computeEvenQuantiles(douBuffer, _number_of_even_quantiles, quantiles);
		if (_backend == CASSANDRA) {
			for (unsigned idx = 0; idx < quantiles.size(); idx++) {
				reading.value = quantiles[idx] / _scaling_factor;
				_quantileSensors[idx]->storeReading(reading);
			}
		} else {
			for(auto q: quantiles){
				agg_info.quantiles.push_back(static_cast<float>(q));
			}
		}
	}
	agg_info.property_type_id = _property_id;

}

void PerSystSqlOperator::compute(U_Ptr unit) {
//nothing here!
}

double severity_formula1(double metric, double threshold, double exponent) {
	double val = metric - threshold;
	if (val > 0) {
		double ret = (pow(val, exponent));
		if (ret > 1) {
			return 1;
		}
		return ret;
	}
	return 0;
}

double severity_formula2(double metric, double threshold, double exponent) {
	if (!threshold) {
		return -1;
	}
	double val = metric / threshold - 1;
	if (val > 0) {
		double ret = (pow(val, exponent));
		if (ret > 1) {
			return 1;
		}
		return ret;
	}
	return 0;
}

double severity_formula3(double metric, double threshold, double exponent) {
	if (!threshold) {
		return -1;
	}
	double val = metric / threshold;
	if (val > 0) {
		double ret = (1 - pow(val, exponent));
		if (ret > 1) {
			return 1;
		}
		if (ret < 0) {
			return 0;
		}
		return ret;
	}
	return 0;
}

double severity_memory(double metric, double threshold, double max_memory) {
	double denominator = max_memory - threshold;
	double severity = -1;
	if (denominator) {
		severity = metric - threshold / (max_memory - threshold);
		if (severity > 1) {
			severity = 1;
		} else if (severity < 0) {
			severity = 0;
		}
	}
	return severity;
}

double PerSystSqlOperator::computeSeverityAverage(
		std::vector<double> & buffer) {
	std::vector<double> severities;
	switch (_severity_formula) {
	case (FORMULA1):
		for (auto val : buffer) {
			auto severity = severity_formula1(val, _severity_threshold, _severity_exponent);
			severities.push_back(severity);
		}
		break;
	case (FORMULA2):
		for (auto val : buffer) {
			auto severity = severity_formula2(val, _severity_threshold,	_severity_exponent);
			severities.push_back(severity);
		}
		break;
	case (FORMULA3):
		for (auto val : buffer) {
			auto severity = severity_formula3(val, _severity_threshold,	_severity_exponent);
			severities.push_back(severity);
		}
		break;
	case (MEMORY_FORMULA):
		for (auto val : buffer) {
			auto severity = severity_memory(val, _severity_threshold, _severity_max_memory);
			severities.push_back(severity);
		}
		break;
	case (NOFORMULA):
		for (auto val : buffer) {
			severities.push_back(severity_noformula());
		}
		break;
	default:
		return 0.0;
		break;
	}
	if (severities.size()) {
		return (std::accumulate(severities.begin(), severities.end(), 0.0)
				/ severities.size());
	}
	return 0.0;
}

void computeEvenQuantiles(std::vector<double> &data, const unsigned int NUMBER_QUANTILES, std::vector<double> &quantiles) {
	if (data.empty() || NUMBER_QUANTILES == 0) {
		return;
	}
	std::sort(data.begin(), data.end());
	int elementNumber = data.size();
	quantiles.resize(NUMBER_QUANTILES + 1); //+min
	double factor = elementNumber / static_cast<double>(NUMBER_QUANTILES);
	quantiles[0] = data[0]; //minimum
	quantiles[NUMBER_QUANTILES] = data[data.size() - 1]; //maximum
	for (unsigned int i = 1; i < NUMBER_QUANTILES; i++) {
		if (elementNumber > 1) {
			int idx = static_cast<int>(std::floor(i * factor));
			if (idx == 0) {
				quantiles[i] = data[0];
			} else {
				double rest = (i * factor) - idx;
				quantiles[i] = data[idx - 1] + rest * (data[idx] - data[idx - 1]);
			}
		} else { //optimization, we don't need to calculate all the quantiles
			quantiles[i] = data[0];
		}
	}
}

