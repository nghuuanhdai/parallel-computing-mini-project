//================================================================================
// Name        : PerSystSqlConfigurator.cpp
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

#include "PerSystSqlConfigurator.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/detail/ptree_implementation.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <string>
#include <vector>

#include "../../includes/OperatorConfiguratorTemplate.h"
#include "../../includes/UnitInterface.h"

PerSystSqlConfigurator::PerSystSqlConfigurator(): JobOperatorConfiguratorTemplate() {
	 _operatorName = "persystsql";
	 _baseName     = "sensor";
	 _rotation_map["EVERY_YEAR"] = MariaDB::EVERY_YEAR;
	 _rotation_map["EVERY_MONTH"] = MariaDB::EVERY_MONTH;
	 _rotation_map["EVERY_XDAYS"] = MariaDB::EVERY_XDAYS;
}

PerSystSqlConfigurator::~PerSystSqlConfigurator() {
}

void PerSystSqlConfigurator::sensorBase(AggregatorSensorBase& s, CFG_VAL config) {
	BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
	{
		if (boost::iequals(val.first, "operation")) {
			std::string opName = val.second.data();
			if (opName == "average") {
				s.setOperation(AggregatorSensorBase::AVG);
			} else if (opName == "deciles" || opName == "percentiles" || opName == "quantile") {
				s.setOperation(AggregatorSensorBase::QTL);
			} else if (opName == "observations" || opName == "numobs") {
				s.setOperation(AggregatorSensorBase::OBS);
			} else if (opName == "average_severity") {
				s.setOperation(AggregatorSensorBase::AVG_SEV);
			} else {
				LOG(error) << "PerSystSqlConfigurator operation " << opName << " not supported!";
			}
		}
	}
}

void PerSystSqlConfigurator::operatorAttributes(PerSystSqlOperator& op, CFG_VAL config) {
	BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config) {
		if (boost::iequals(val.first, "number_quantiles")) {
    		try {
    			unsigned int num_quantiles = std::stoul(val.second.data());
    			op.setNumberOfEvenQuantiles(num_quantiles);
    		} catch (const std::exception &e) {
				LOG(error) << "  Error parsing number_quantiles \"" << val.second.data() << "\": " << e.what();
			}
		} else if (boost::iequals(val.first, "batch_domain")){
			try {
				int batch_domain = std::stoi(val.second.data());
				op.setBatchDomain(batch_domain);
			} catch (const std::exception &e) {
				LOG(error) << "  Error parsing batch_domain \"" << val.second.data() << "\": " << e.what();
			}
		} else if (boost::iequals(val.first, "go_back_ms")) {
			try {
				uint64_t go_back_ms = std::stoull(val.second.data());
				op.setGoBackInMs(go_back_ms);
			} catch (const std::exception &e) {
				LOG(error) << "  Error parsing go_back_ms \"" << val.second.data() << "\": " << e.what();
			}
		} else if (boost::iequals(val.first, "severity_threshold")){
			try {
				double threshold = std::stod(val.second.data());
				op.setSeverityThreshold(threshold);
			} catch (const std::exception &e) {
				LOG(error) << "  Error parsing severity_threshold \"" << val.second.data() << "\": " << e.what();
			}
		} else if (boost::iequals(val.first, "severity_exponent")){
			try {
				double exponent =  std::stod(val.second.data());
				op.setSeverityExponent(exponent);
			} catch (const std::exception &e) {
				LOG(error) << "  Error parsing severity_exponent \"" << val.second.data() << "\": " << e.what();
			}
		} else if (boost::iequals(val.first, "severity_formula")){
			std::string formula = val.second.data();
			if(formula == "formula1" || formula == "FORMULA1"){
				op.setSeverityFormula(PerSystSqlOperator::FORMULA1);
			} else if (formula == "formula2" || formula == "FORMULA2") {
				op.setSeverityFormula(PerSystSqlOperator::FORMULA2);
			} else if (formula == "formula3" || formula == "FORMULA3") {
				op.setSeverityFormula(PerSystSqlOperator::FORMULA3);
			} else if (formula == "memory_formula" || formula == "MEMORY_FORMULA"){
				op.setSeverityFormula(PerSystSqlOperator::MEMORY_FORMULA);
			} else {
				LOG(error) << "Unrecognized/unsupported severity formula: " << formula;
			}
		} else if (boost::iequals(val.first, "severity_max_memory")){
			try {
				auto max_memory =  std::stof(val.second.data());
				op.setSeverityMaxMemory(max_memory);
			} catch (const std::exception &e) {
				LOG(error) << "  Error parsing severity_max_memory \"" << val.second.data() << "\": " << e.what();
			}
		} else if (boost::iequals(val.first, "backend")) {
			if(val.second.data() == "cassandra") {
				op.setBackend(PerSystSqlOperator::CASSANDRA);
			} else if(val.second.data() == "mariadb") {
				op.setBackend(PerSystSqlOperator::MARIADB);
			} else {
				LOG(error) << "  Unrecognized/unsupported backend \"" << val.second.data();
			}
	    } else if(boost::iequals(val.first, "property_id")){
	    	try {
	    		auto property_id = std::stoi(val.second.data());
	    		op.setPropertyId(property_id);
	    	} catch (const std::exception &e) {
	    		LOG(error) << "  Error parsing property_id \"" << val.second.data() << "\": " << e.what();
	    	}
		} else if (boost::iequals(val.first, "mariadb_host")) {
			op.setMariaDBHost(val.second.data());
		} else if (boost::iequals(val.first, "mariadb_user")){
			op.setMariaDBUser(val.second.data());
		} else if (boost::iequals(val.first, "mariadb_password")){
			op.setMariaDBPassword(val.second.data());
		} else if (boost::iequals(val.first, "mariadb_database_name")){
			op.setMariaDBDatabaseName(val.second.data());
		} else if (boost::iequals(val.first, "mariadb_port")){
			try {
				int port = std::stoi(val.second.data());
				op.setMariaDBPort(port);
			} catch (const std::exception &e) {
	    		LOG(error) << "  Error parsing mariadb_port \"" << val.second.data() << "\": " << e.what();
	    	}
		} else if (boost::iequals(val.first, "mariadb_rotation")){
			auto found = _rotation_map.find(val.second.data());
			if (found != _rotation_map.end()){
				op.setMariaDBRotation(found->second);
			} else {
				LOG(error) << " Rotation strategy (" << val.second.data() << ") not found.";
			}
		} else if (boost::iequals(val.first, "mariadb_every_x_days")){
			try {
				unsigned int every_x_days = std::stoi(val.second.data());
				op.setMariaDBEveryXDays(every_x_days);
			} catch (const std::exception &e) {
	    		LOG(error) << "  Error parsing mariadb_every_x_days \"" << val.second.data() << "\": " << e.what();
	    	}
		}
	}
}

bool PerSystSqlConfigurator::unit(UnitTemplate<AggregatorSensorBase>& u) {
	if(!u.isTopUnit()) {
		LOG(error) << "    " << _operatorName << ": This operator type only supports hierarchical units!";
		return false;
	}
	if(u.getOutputs().empty()) {
		LOG(error) << "    " << _operatorName << ": At least one output sensor per unit must be defined!";
		return false;
	}
	return true;
}

bool PerSystSqlConfigurator::readUnits(PerSystSqlOperator& op,
		std::vector<shared_ptr<AggregatorSensorBase>>& protoInputs,
		std::vector<shared_ptr<AggregatorSensorBase>>& protoOutputs,
	    std::vector<shared_ptr<AggregatorSensorBase>>& protoGlobalOutputs,
	    inputMode_t inputMode) {


	int num_quantiles = op.getNumberOfEvenQuantiles();
	if(num_quantiles == 0){
		return false;
	}
	bool quantile_found = false;
	AggregatorSensorBase quantsensor("");
	for(auto &sensor: protoGlobalOutputs) {
		if(sensor->getOperation() == AggregatorSensorBase::QTL){
			quantile_found = true;
			quantsensor = *(sensor.get());
			sensor->setPercentile(0);
			auto topic = sensor->getMqtt();
			sensor->setMqtt(topic + "0");
			break;
		}
	}

	for(int i = 1; i <= num_quantiles && quantile_found; ++i) {
		auto outputSensor = std::make_shared<AggregatorSensorBase>(quantsensor);
		outputSensor->setMqtt(outputSensor->getMqtt() + std::to_string(i));
		outputSensor->setOperation(AggregatorSensorBase::QTL);
		outputSensor->setPercentile(i);
		protoGlobalOutputs.push_back(outputSensor);
	}

	return JobOperatorConfiguratorTemplate::readUnits(op, protoInputs, protoOutputs, protoGlobalOutputs, inputMode);
}
