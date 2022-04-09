//================================================================================
// Name        : PerSystSqlOperator.h
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

#ifndef ANALYTICS_OPERATORS_PERSYSTSQL_PERSYSTSQLOPERATOR_H_
#define ANALYTICS_OPERATORS_PERSYSTSQL_PERSYSTSQLOPERATOR_H_

#include <string>
#include <vector>
#include <mutex>

#include "../../../common/include/cacheentry.h"
#include "../../../common/include/logging.h"
#include "../../includes/JobOperatorTemplate.h"
#include "../aggregator/AggregatorSensorBase.h"
#include "MariaDB.h"


class PerSystSqlOperator: public JobOperatorTemplate<AggregatorSensorBase>{
public:
	enum Formula {
		NOFORMULA = 0,
		FORMULA1 = 1,
		FORMULA2 = 2,
		FORMULA3 = 3,
		MEMORY_FORMULA = 4,
	};

	enum Backend_t {
		CASSANDRA = 0,
		MARIADB = 1,
		DEFAULT = MARIADB
	};

public:
	PerSystSqlOperator(const std::string& name);
	virtual ~PerSystSqlOperator();
	PerSystSqlOperator(const PerSystSqlOperator& other);
	PerSystSqlOperator& operator=(const PerSystSqlOperator& other);
	void copy(const PerSystSqlOperator& other);

	void compute(U_Ptr unit, qeJobData& jobData) override;
	void printConfig(LOG_LEVEL ll) override;

	unsigned int getNumberOfEvenQuantiles() const {
		return _number_of_even_quantiles;
	}

	void setNumberOfEvenQuantiles(unsigned int numberOfEvenQuantiles) {
		_number_of_even_quantiles = numberOfEvenQuantiles;
	}

	void setBatchDomain(int batch_domain){
		_batch_domain = batch_domain;
	}

	void pushbackQuantileSensor(AggregatorSBPtr qSensor){
		_quantileSensors.push_back(qSensor);
	}

	void setSeverityExponent(double severityExponent) {
		_severity_exponent = severityExponent;
	}

	void setSeverityFormula(Formula severityFormula) {
		_severity_formula = severityFormula;
	}

	void setSeverityMaxMemory(double severityMaxMemory) {
		_severity_max_memory = severityMaxMemory;
	}

	void setSeverityThreshold(double severityThreshold) {
		_severity_threshold = severityThreshold;
	}

	void setGoBackInMs(uint64_t go_back_ms){
		_go_back_ns = go_back_ms * 1000000;
	}

	void setBackend(Backend_t backend) {
		_backend = backend;
	}

	void setMariaDBEveryXDays(unsigned int every_x_days){
		_conn.every_x_days = every_x_days;
	}

	void setMariaDBHost(const std::string & host){
		_conn.host = host;
	}

	void setMariaDBUser(const std::string & user){
		_conn.user = user;
	}

	void setMariaDBPassword(const std::string & password){
		_conn.password = password;
	}

	void setMariaDBDatabaseName(const std::string & database_name){
		_conn.database_name = database_name;
	}

	void setMariaDBPort(int port){
		_conn.port = port;
	}

	void setMariaDBRotation(MariaDB::Rotation_t rotation){
		_conn.rotation = rotation;
	}

	void setPropertyId(int property_id){
		_property_id = property_id;
	}

private:
	std::vector<reading_t> _buffer;
	std::vector<AggregatorSBPtr> _quantileSensors;
	unsigned int _number_of_even_quantiles;
	int _batch_domain;
	Formula _severity_formula;
	double _severity_threshold;
	double _severity_exponent;
	double _severity_max_memory;
	std::vector<double> _severities;
	uint64_t _go_back_ns;
	Backend_t _backend;
	double _scaling_factor;
	bool _searchedOnceForMetaData;

	struct MariaDB_conn_t {
		std::string host;
		std::string user;
		std::string password;
		std::string database_name;
		int port;
		MariaDB::Rotation_t rotation;
		unsigned int every_x_days;
	};

	MariaDB_conn_t _conn;
	int _property_id;
	MariaDB *_persystdb;
	const int SCALING_FACTOR_SEVERITY=1000000;

protected:
	virtual void compute(U_Ptr unit) override;
	void compute_internal(U_Ptr& unit, vector<reading_t>& buffer, Aggregate_info_t &agg_info, uint64_t measurement_ts);
	double computeSeverityAverage(vector<double> & buffer);
	void convertToDoubles(std::vector<reading_t> &buffer, std::vector<double> &douBuffer);
	bool execOnStart() override;
	void execOnStop() override;
};

double severity_formula1(double metric, double threshold, double exponent);
double severity_formula2(double metric, double threshold, double exponent);
double severity_formula3(double metric, double threshold, double exponent);
double severity_memory(double metric, double threshold, double max_memory);
constexpr double severity_noformula(){return 0.0;} //No severity
void computeEvenQuantiles(std::vector<double> &data, const unsigned int NUMBER_QUANTILES, std::vector<double> &quantiles);


#endif /* ANALYTICS_OPERATORS_PERSYSTSQL_PERSYSTSQLOPERATOR_H_ */
