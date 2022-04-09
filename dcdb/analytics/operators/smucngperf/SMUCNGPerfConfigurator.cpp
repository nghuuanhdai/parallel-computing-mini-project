//================================================================================
// Name        : SMUCNGPerfConfigurator.cpp
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

#include "SMUCNGPerfConfigurator.h"


SMUCNGPerfConfigurator::SMUCNGPerfConfigurator() : OperatorConfiguratorTemplate(), _vector_position(0) {
	_operatorName = "supermucngperf";
	_baseName     = "sensor";
	_metricMap["INSTRUCTIONS"]=SMUCSensorBase::INSTRUCTIONS;
	_metricMap["CLOCKS"]=SMUCSensorBase::CLOCKS;
	_metricMap["CLOCKS_REF"]=SMUCSensorBase::CLOCKS_REF;
	_metricMap["USERPCT"]=SMUCSensorBase::USERPCT;
	_metricMap["USERPCT0"]=SMUCSensorBase::USERPCT0;
	_metricMap["SYSTEMPCT"]=SMUCSensorBase::SYSTEMPCT;
	_metricMap["SYSTEMPCT0"]=SMUCSensorBase::SYSTEMPCT0;
	_metricMap["IOWAITPCT"]=SMUCSensorBase::IOWAITPCT;
	_metricMap["IOWAITPCT0"]=SMUCSensorBase::IOWAITPCT0;
	_metricMap["MEMUSED"]=SMUCSensorBase::MEMUSED;
	_metricMap["IOBYTESREAD"]=SMUCSensorBase::IOBYTESREAD;
	_metricMap["IOBYTESWRITE"]=SMUCSensorBase::IOBYTESWRITE;
	_metricMap["IOOPENS"]=SMUCSensorBase::IOOPENS;
	_metricMap["IOCLOSES"]=SMUCSensorBase::IOCLOSES;
	_metricMap["IOREADS"]=SMUCSensorBase::IOREADS;
	_metricMap["IOWRITES"]=SMUCSensorBase::IOWRITES;
	_metricMap["NETWORK_XMIT_BYTES"]=SMUCSensorBase::NETWORK_XMIT_BYTES;
	_metricMap["NETWORK_RCVD_BYTES"]=SMUCSensorBase::NETWORK_RCVD_BYTES;
	_metricMap["NETWORK_XMIT_PKTS"]=SMUCSensorBase::NETWORK_XMIT_PKTS;
	_metricMap["NETWORK_RCVD_PKTS"]=SMUCSensorBase::NETWORK_RCVD_PKTS;
	_metricMap["L2_RQSTS_MISS"]=SMUCSensorBase::L2_RQSTS_MISS;
	_metricMap["ARITH_FPU_DIVIDER_ACTIVE"]=SMUCSensorBase::ARITH_FPU_DIVIDER_ACTIVE;
	_metricMap["FP_ARITH_SCALAR_DOUBLE"]=SMUCSensorBase::FP_ARITH_SCALAR_DOUBLE;
	_metricMap["FP_ARITH_SCALAR_SINGLE"]=SMUCSensorBase::FP_ARITH_SCALAR_SINGLE;
	_metricMap["FP_ARITH_128B_PACKED_DOUBLE"]=SMUCSensorBase::FP_ARITH_128B_PACKED_DOUBLE;
	_metricMap["FP_ARITH_128B_PACKED_SINGLE"]=SMUCSensorBase::FP_ARITH_128B_PACKED_SINGLE;
	_metricMap["FP_ARITH_256B_PACKED_DOUBLE"]=SMUCSensorBase::FP_ARITH_256B_PACKED_DOUBLE;
	_metricMap["FP_ARITH_256B_PACKED_SINGLE"]=SMUCSensorBase::FP_ARITH_256B_PACKED_SINGLE;
	_metricMap["FP_ARITH_512B_PACKED_DOUBLE"]=SMUCSensorBase::FP_ARITH_512B_PACKED_DOUBLE;
	_metricMap["FP_ARITH_512B_PACKED_SINGLE"]=SMUCSensorBase::FP_ARITH_512B_PACKED_SINGLE;
	_metricMap["MEM_INST_RETIRED_ALL_LOADS"]=SMUCSensorBase::MEM_INST_RETIRED_ALL_LOADS;
	_metricMap["MEM_INST_RETIRED_ALL_STORES"]=SMUCSensorBase::MEM_INST_RETIRED_ALL_STORES;
	_metricMap["MEM_LOAD_UOPS_RETIRED_L3_MISS"]=SMUCSensorBase::MEM_LOAD_UOPS_RETIRED_L3_MISS;
	_metricMap["MEM_LOAD_RETIRED_L3_HIT"]=SMUCSensorBase::MEM_LOAD_RETIRED_L3_HIT;
	_metricMap["MEM_LOAD_RETIRED_L3_MISS"]=SMUCSensorBase::MEM_LOAD_RETIRED_L3_MISS;
	_metricMap["PERF_COUNT_HW_BRANCH_INSTRUCTIONS"]=SMUCSensorBase::PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
	_metricMap["PERF_COUNT_HW_BRANCH_MISSES"]=SMUCSensorBase::PERF_COUNT_HW_BRANCH_MISSES;
	_metricMap["CORE_TEMPERATURE"]=SMUCSensorBase::CORE_TEMPERATURE;
	_metricMap["CAS_COUNT_READ0"]=SMUCSensorBase::CAS_COUNT_READ0;
	_metricMap["CAS_COUNT_READ1"]=SMUCSensorBase::CAS_COUNT_READ1;
	_metricMap["CAS_COUNT_READ2"]=SMUCSensorBase::CAS_COUNT_READ2;
	_metricMap["CAS_COUNT_READ3"]=SMUCSensorBase::CAS_COUNT_READ3;
	_metricMap["CAS_COUNT_READ4"]=SMUCSensorBase::CAS_COUNT_READ4;
	_metricMap["CAS_COUNT_READ5"]=SMUCSensorBase::CAS_COUNT_READ5;
	_metricMap["CAS_COUNT_WRITE0"]=SMUCSensorBase::CAS_COUNT_WRITE0;
	_metricMap["CAS_COUNT_WRITE1"]=SMUCSensorBase::CAS_COUNT_WRITE1;
	_metricMap["CAS_COUNT_WRITE2"]=SMUCSensorBase::CAS_COUNT_WRITE2;
	_metricMap["CAS_COUNT_WRITE3"]=SMUCSensorBase::CAS_COUNT_WRITE3;
	_metricMap["CAS_COUNT_WRITE4"]=SMUCSensorBase::CAS_COUNT_WRITE4;
	_metricMap["CAS_COUNT_WRITE5"]=SMUCSensorBase::CAS_COUNT_WRITE5;
	_metricMap["PKG0_ENERGY"]=SMUCSensorBase::PKG0_ENERGY;
	_metricMap["PKG1_ENERGY"]=SMUCSensorBase::PKG1_ENERGY;
	_metricMap["DRAM0_ENERGY"]=SMUCSensorBase::DRAM0_ENERGY;
	_metricMap["DRAM1_ENERGY"]=SMUCSensorBase::DRAM1_ENERGY;
	_metricMap["CPI"]=SMUCSensorBase::CPI;
	_metricMap["FREQUENCY"]=SMUCSensorBase::FREQUENCY;
	_metricMap["INSTRUCTIONS_PER_SECOND"]=SMUCSensorBase::INSTRUCTIONS_PER_SECOND;
	_metricMap["FLOPS"]=SMUCSensorBase::FLOPS;
	_metricMap["PACKED_FLOPS"]=SMUCSensorBase::PACKED_FLOPS;
	_metricMap["AVX512_TOVECTORIZED_RATIO"]=SMUCSensorBase::AVX512_TOVECTORIZED_RATIO;
	_metricMap["VECTORIZATION_RATIO"]=SMUCSensorBase::VECTORIZATION_RATIO;
	_metricMap["SINGLE_PRECISION_TO_TOTAL_RATIO"]=SMUCSensorBase::SINGLE_PRECISION_TO_TOTAL_RATIO;
	_metricMap["EXPENSIVE_INSTRUCTIONS_PER_SECOND"]=SMUCSensorBase::EXPENSIVE_INSTRUCTIONS_PER_SECOND;
	_metricMap["INTRA_NODE_LOADIMBALANCE"]=SMUCSensorBase::INTRA_NODE_LOADIMBALANCE;
	_metricMap["INTER_NODE_LOADIMBALANCE"]=SMUCSensorBase::INTER_NODE_LOADIMBALANCE;
	_metricMap["L2_HITS_PER_SECOND"]=SMUCSensorBase::L2_HITS_PER_SECOND;
	_metricMap["L2_MISSES_PER_SECOND"]=SMUCSensorBase::L2_MISSES_PER_SECOND;
	_metricMap["L3_HITS_PER_SECOND"]=SMUCSensorBase::L3_HITS_PER_SECOND;
	_metricMap["L3_MISSES_PER_SECOND"]=SMUCSensorBase::L3_MISSES_PER_SECOND;
	_metricMap["L3_TO_INSTRUCTIONS_RATIO"]=SMUCSensorBase::L3_TO_INSTRUCTIONS_RATIO;
	_metricMap["L3_BANDWIDTH"]=SMUCSensorBase::L3_BANDWIDTH;
	_metricMap["L3HIT_TO_L3MISS_RATIO"]=SMUCSensorBase::L3HIT_TO_L3MISS_RATIO;
	_metricMap["LOADS_TO_STORES"]=SMUCSensorBase::LOADS_TO_STORES;
	_metricMap["LOADS_TOL3MISS_RATIO"]=SMUCSensorBase::LOADS_TOL3MISS_RATIO;
	_metricMap["MISSBRANCHES_PER_SECOND"]=SMUCSensorBase::MISSBRANCHES_PER_SECOND;
	_metricMap["BRANCH_PER_INSTRUCTIONS"]=SMUCSensorBase::BRANCH_PER_INSTRUCTIONS;
	_metricMap["MISSBRANCHES_TO_TOTAL_BRANCH_RATIO"]=SMUCSensorBase::MISSBRANCHES_TO_TOTAL_BRANCH_RATIO;
	_metricMap["MEMORY_BANDWIDTH"]=SMUCSensorBase::MEMORY_BANDWIDTH;
	_metricMap["RAPL_PKG"]=SMUCSensorBase::RAPL_PKG;
	_metricMap["RAPL_MEM"]=SMUCSensorBase::RAPL_MEM;
	_metricMap["IPMI_CPU"]=SMUCSensorBase::IPMI_CPU;
	_metricMap["IPMI_MEM"]=SMUCSensorBase::IPMI_MEM;
	_metricMap["IPMI_DC"]=SMUCSensorBase::IPMI_DC;
	_metricMap["IPMI_AC"]=SMUCSensorBase::IPMI_AC;
	_metricMap["NETWORK_XMIT_BYTES_PER_PKT"]=SMUCSensorBase::NETWORK_XMIT_BYTES_PER_PKT;
	_metricMap["NETWORK_BYTES_XMIT_PER_SECOND"]=SMUCSensorBase::NETWORK_BYTES_XMIT_PER_SECOND;
	_metricMap["NETWORK_RCV_BYTES_PER_PKT"]=SMUCSensorBase::NETWORK_RCV_BYTES_PER_PKT;
	_metricMap["NETWORK_BYTES_RCVD_PER_SECOND"]=SMUCSensorBase::NETWORK_BYTES_RCVD_PER_SECOND;
	_metricMap["IOOPENS_PER_SECOND"]=SMUCSensorBase::IOOPENS_PER_SECOND;
	_metricMap["IOCLOSES_PER_SECOND"]=SMUCSensorBase::IOCLOSES_PER_SECOND;
	_metricMap["IOBYTESREAD_PER_SECOND"]=SMUCSensorBase::IOBYTESREAD_PER_SECOND;
	_metricMap["IOBYTESWRITE_PER_SECOND"]=SMUCSensorBase::IOBYTESWRITE_PER_SECOND;
	_metricMap["IOREADS_PER_SECOND"]=SMUCSensorBase::IOREADS_PER_SECOND;
	_metricMap["IOWRITES_PER_SECOND"]=SMUCSensorBase::IOWRITES_PER_SECOND;
	_metricMap["IO_BYTES_READ_PER_OP"]=SMUCSensorBase::IO_BYTES_READ_PER_OP;
	_metricMap["IO_BYTES_WRITE_PER_OP"]=SMUCSensorBase::IO_BYTES_WRITE_PER_OP;
	_metricMap["IOBYTESREAD_PER_SECOND_PROF"]=SMUCSensorBase::IOBYTESREAD_PER_SECOND_PROF;
	_metricMap["IOBYTESWRITE_PER_SECOND_PROF"]=SMUCSensorBase::IOBYTESWRITE_PER_SECOND_PROF;
	_metricMap["IOREADS_PER_SECOND_PROF"]=SMUCSensorBase::IOREADS_PER_SECOND_PROF;
	_metricMap["IOWRITES_PER_SECOND_PROF"]=SMUCSensorBase::IOWRITES_PER_SECOND_PROF;
	_metricMap["IO_BYTES_READ_PER_OP_PROF"]=SMUCSensorBase::IO_BYTES_READ_PER_OP_PROF;
	_metricMap["IO_BYTES_WRITE_PER_OP_PROF"]=SMUCSensorBase::IO_BYTES_WRITE_PER_OP_PROF;
	_metricMap["PACKED128_FLOPS"] = SMUCSensorBase::PACKED128_FLOPS;
	_metricMap["PACKED256_FLOPS"] = SMUCSensorBase::PACKED256_FLOPS;
	_metricMap["PACKED512_FLOPS"] = SMUCSensorBase::PACKED512_FLOPS;
	_metricMap["SINGLE_PRECISION_FLOPS"] = SMUCSensorBase::SINGLE_PRECISION_FLOPS;
	_metricMap["DOUBLE_PRECISION_FLOPS"] = SMUCSensorBase::DOUBLE_PRECISION_FLOPS;
	_metricMap["PKG_POWER"] = SMUCSensorBase::PKG_POWER;
	_metricMap["DRAM_POWER"] = SMUCSensorBase::DRAM_POWER;
}

SMUCNGPerfConfigurator::~SMUCNGPerfConfigurator() {
}

bool endsWith (std::string const &fullString, std::string const &ending) {
	if(ending.size() > fullString.size()){
		return false;
	}
	std::size_t last_forward = fullString.find_last_of('/');
	std::size_t last_greaterthan = fullString.find_last_of('>');
	std::size_t max_pos;
	if(last_greaterthan == std::string::npos){ //no >
		if(last_forward == std::string::npos) { // no /
			return fullString.compare(ending)==0;
		} else { // we have /
			max_pos = last_forward;
		}
	} else { // > is present
		if(last_forward == std::string::npos) { // no /
			max_pos = last_greaterthan;
		} else {
			max_pos = last_forward > last_greaterthan ? last_forward : last_greaterthan;
		}
	}
	max_pos++; //increment so that we compare the next char.
	if(max_pos < fullString.length()){
		return 0 == fullString.compare(max_pos, fullString.size() - max_pos, ending);
	}
	return false;
}

void SMUCNGPerfConfigurator::sensorBase(SMUCSensorBase& s, CFG_VAL config) {
	std::string name = s.getName();
	bool found = false;
	for (auto & kv : _metricMap) {
		if(endsWith(name,kv.first)) {
			found = true;
			_metricToPosition[kv.second] = _vector_position;
			s.setMetric(kv.second);
			break;
		}
	}
	if(!found){
		LOG(error) << "Unable to configure sensor "<< name << " no match in _metricMap found."; 
	}
	_vector_position++;
}

void SMUCNGPerfConfigurator::operatorAttributes(SMUCNGPerfOperator& op, CFG_VAL config){
	op.setMetricToPosition(_metricToPosition);
    _vector_position = 0;
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config)
    {
    	if (boost::iequals(val.first, "measuring_interval")){
    		try {
    			auto measurementInterval_s = std::stod(val.second.data())/1000.0; //miliseconds to seconds
    			op.setMeasuringInterval(measurementInterval_s);
    		} catch (const std::exception &e) {
    			LOG(error) << "  Error parsing measuring_interval \"" << val.second.data() << "\": " << e.what();
    		}
    	} else if(boost::iequals(val.first, "go_back_ms")) {
    		try {
    			auto go_back_ms = std::stoull(val.second.data());
    			op.setGoBackMs(go_back_ms);
    		} catch (const std::exception &e) {
    			LOG(error) << "  Error parsing go_back_ms \"" << val.second.data() << "\": " << e.what();
    		}
    	}
    }
	_metricToPosition.clear();
}

bool SMUCNGPerfConfigurator::unit(UnitTemplate<SMUCSensorBase>& u) {
	if(u.isTopUnit()) {
		LOG(error) << "    " << _operatorName << ": This operator type only supports flat units!";
		return false;
	}
	if(u.getOutputs().empty()) {
		LOG(error) << "    " << _operatorName << ": At least one output sensor per unit must be defined!";
		return false;
	}
	return true;
}
