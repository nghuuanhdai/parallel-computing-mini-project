//================================================================================
// Name        : SMUCNGPerfOperator.cpp
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

#include "SMUCNGPerfOperator.h"

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/parameter/keyword.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

#include "../../../common/include/cacheentry.h"
#include "../../../common/include/logging.h"
#include "../../../common/include/sensorbase.h"
#include "../../../common/include/timestamp.h"
#include "../../includes/QueryEngine.h"
#include "../../includes/UnitTemplate.h"
#include "SKXPMUMetrics.h"
#include "../../includes/CommonStatistics.h"

SMUCNGPerfOperator::SMUCNGPerfOperator(const std::string& name): OperatorTemplate(name), _go_back_ns(0), _measuring_interval_s(1) {
    _buffers.resize(64);
    _metricPerSecToId[SMUCSensorBase::INSTRUCTIONS_PER_SECOND] = SMUCSensorBase::INSTRUCTIONS;
    _metricPerSecToId[SMUCSensorBase::EXPENSIVE_INSTRUCTIONS_PER_SECOND] = SMUCSensorBase::ARITH_FPU_DIVIDER_ACTIVE;
//    _metricToPerSecId[SMUCSensorBase::L2_HITS] = SMUCSensorBase::L2_HITS_PER_SECOND;
    _metricPerSecToId[SMUCSensorBase::L2_MISSES_PER_SECOND] = SMUCSensorBase::L2_RQSTS_MISS;
    _metricPerSecToId[SMUCSensorBase::L3_HITS_PER_SECOND] = SMUCSensorBase::MEM_LOAD_RETIRED_L3_HIT;
    _metricPerSecToId[SMUCSensorBase::L3_MISSES_PER_SECOND] = SMUCSensorBase::MEM_LOAD_RETIRED_L3_MISS;
    _metricPerSecToId[SMUCSensorBase::MISSBRANCHES_PER_SECOND] = SMUCSensorBase::PERF_COUNT_HW_BRANCH_MISSES;
    _metricPerSecToId[SMUCSensorBase::NETWORK_BYTES_XMIT_PER_SECOND] = SMUCSensorBase::NETWORK_XMIT_BYTES;
    _metricPerSecToId[SMUCSensorBase::NETWORK_BYTES_RCVD_PER_SECOND] = SMUCSensorBase::NETWORK_RCVD_BYTES;
    _metricPerSecToId[SMUCSensorBase::IOOPENS_PER_SECOND] = SMUCSensorBase::IOOPENS;
    _metricPerSecToId[SMUCSensorBase::IOCLOSES_PER_SECOND] = SMUCSensorBase::IOCLOSES;
    _metricPerSecToId[SMUCSensorBase::IOBYTESREAD_PER_SECOND] = SMUCSensorBase::IOBYTESREAD;
    _metricPerSecToId[SMUCSensorBase::IOBYTESWRITE_PER_SECOND] = SMUCSensorBase::IOBYTESWRITE;
    _metricPerSecToId[SMUCSensorBase::IOREADS_PER_SECOND] = SMUCSensorBase::IOREADS;
    _metricPerSecToId[SMUCSensorBase::IOWRITES_PER_SECOND] = SMUCSensorBase::IOWRITES;

    _metricRatioToPair.emplace(std::piecewise_construct, 
        std::forward_as_tuple(SMUCSensorBase::L3_TO_INSTRUCTIONS_RATIO),
	std::forward_as_tuple(SMUCSensorBase::MEM_LOAD_UOPS_RETIRED_L3_MISS, SMUCSensorBase::INSTRUCTIONS));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::CPI),
        std::forward_as_tuple(SMUCSensorBase::CLOCKS, SMUCSensorBase::INSTRUCTIONS));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::LOADS_TO_STORES),
        std::forward_as_tuple(SMUCSensorBase::MEM_INST_RETIRED_ALL_LOADS, SMUCSensorBase::MEM_INST_RETIRED_ALL_STORES));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::BRANCH_PER_INSTRUCTIONS),
         std::forward_as_tuple(SMUCSensorBase::PERF_COUNT_HW_BRANCH_MISSES, SMUCSensorBase::INSTRUCTIONS));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::MISSBRANCHES_TO_TOTAL_BRANCH_RATIO),
         std::forward_as_tuple(SMUCSensorBase::PERF_COUNT_HW_BRANCH_MISSES,SMUCSensorBase::PERF_COUNT_HW_BRANCH_INSTRUCTIONS));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::LOADS_TOL3MISS_RATIO),
        std::forward_as_tuple(SMUCSensorBase::MEM_INST_RETIRED_ALL_LOADS, SMUCSensorBase::MEM_LOAD_UOPS_RETIRED_L3_MISS));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::IO_BYTES_READ_PER_OP),
        std::forward_as_tuple(SMUCSensorBase::IOBYTESREAD, SMUCSensorBase::IOREADS));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::IO_BYTES_WRITE_PER_OP),
        std::forward_as_tuple(SMUCSensorBase::IOBYTESWRITE, SMUCSensorBase::IOWRITES));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::NETWORK_XMIT_BYTES_PER_PKT),
        std::forward_as_tuple(SMUCSensorBase::NETWORK_XMIT_BYTES, SMUCSensorBase::NETWORK_XMIT_PKTS));
    _metricRatioToPair.emplace(std::piecewise_construct, std::forward_as_tuple(SMUCSensorBase::NETWORK_RCV_BYTES_PER_PKT),
        std::forward_as_tuple(SMUCSensorBase::NETWORK_RCVD_BYTES, SMUCSensorBase::NETWORK_RCVD_PKTS));

    _profileMetricToMetricIds[SMUCSensorBase::IOBYTESREAD_PER_SECOND_PROF]=	{SMUCSensorBase::IOBYTESREAD};
    _profileMetricToMetricIds[SMUCSensorBase::IOBYTESWRITE_PER_SECOND_PROF] = {SMUCSensorBase::IOBYTESWRITE};
    _profileMetricToMetricIds[SMUCSensorBase::IOREADS_PER_SECOND_PROF] = {SMUCSensorBase::IOREADS};
    _profileMetricToMetricIds[SMUCSensorBase::IOWRITES_PER_SECOND_PROF] = {SMUCSensorBase::IOWRITES};
    _profileMetricToMetricIds[SMUCSensorBase::IO_BYTES_READ_PER_OP_PROF] = {SMUCSensorBase::IOBYTESREAD, SMUCSensorBase::IOREADS};
    _profileMetricToMetricIds[SMUCSensorBase::IO_BYTES_WRITE_PER_OP_PROF] = {SMUCSensorBase::IOBYTESWRITE, SMUCSensorBase::IOWRITES};

    _flop_metric = {SMUCSensorBase::FLOPS, SMUCSensorBase::PACKED_FLOPS, SMUCSensorBase::AVX512_TOVECTORIZED_RATIO, SMUCSensorBase::VECTORIZATION_RATIO,
        SMUCSensorBase::SINGLE_PRECISION_TO_TOTAL_RATIO, SMUCSensorBase::PACKED128_FLOPS, SMUCSensorBase::PACKED256_FLOPS, SMUCSensorBase::PACKED512_FLOPS,
        SMUCSensorBase::SINGLE_PRECISION_FLOPS,SMUCSensorBase::DOUBLE_PRECISION_FLOPS};

    _as_is_metric[SMUCSensorBase::USERPCT] = SMUCSensorBase::USERPCT0; 
    _as_is_metric[SMUCSensorBase::SYSTEMPCT] = SMUCSensorBase::SYSTEMPCT0; 
    _as_is_metric[SMUCSensorBase::IOWAITPCT] = SMUCSensorBase::IOWAITPCT0; 
}

SMUCNGPerfOperator::~SMUCNGPerfOperator() {
}

SMUCNGPerfOperator::SMUCNGPerfOperator(const SMUCNGPerfOperator& other) : OperatorTemplate(other){
    copy(other);
}

SMUCNGPerfOperator& SMUCNGPerfOperator::operator=(const SMUCNGPerfOperator& other){
    OperatorTemplate::operator =(other);
    copy(other);
    return *this;
}

void SMUCNGPerfOperator::copy(const SMUCNGPerfOperator& other){
    this->_buffers = other._buffers;
    this->_metricToPosition = other._metricToPosition;
    this->_measuring_interval_s = other._measuring_interval_s;
    this->_go_back_ns = other._go_back_ns;
    this->_metricPerSecToId = other._metricPerSecToId;
    this->_metricRatioToPair = other._metricRatioToPair;
    this->_profileMetricToMetricIds = other._profileMetricToMetricIds;
    this->_flop_metric = other._flop_metric;
    this->_as_is_metric = other._as_is_metric;
}

void SMUCNGPerfOperator::printConfig(LOG_LEVEL ll) {
    OperatorTemplate<SMUCSensorBase>::printConfig(ll);
    LOG_VAR(ll) << "Operator " << _name << ":";
    LOG_VAR(ll) << "Metric to position map size(" << _metricToPosition.size() << "):";
    for(auto &kv : _metricToPosition){
    	LOG_VAR(ll) << "\tMetric = " << kv.first << " Position = " << kv.second;
    }
    LOG_VAR(ll) << "_measuring_interval_s = " << _measuring_interval_s;
    LOG_VAR(ll) << "_go_back_ns = " << _go_back_ns;
}

void SMUCNGPerfOperator::compute(U_Ptr unit) {
	auto inputs = unit->getInputs();
	auto timestamp = _scheduledTime - _go_back_ns;
	for(auto& outSensor : unit->getOutputs()){
		if(outSensor->getMetadata() == nullptr || !outSensor->getMetadata()->getScale()) {
			LOG(error) << "No metadata defined, sensor " << outSensor->getName() <<  " can't compute anything.";
			continue;
		}
		if (outSensor->getMetric() == SMUCSensorBase::PKG_POWER || outSensor->getMetric() == SMUCSensorBase::DRAM_POWER){
			computePOWER(inputs, outSensor, timestamp);
		} else if (outSensor->getMetric() == SMUCSensorBase::FREQUENCY) {
			computeFREQUENCY(inputs, outSensor, timestamp);
		} else if (_flop_metric.find(outSensor->getMetric()) != _flop_metric.end()) {
			computeFLOPS(inputs, outSensor, timestamp);
		} else if (outSensor->getMetric() == SMUCSensorBase::L3HIT_TO_L3MISS_RATIO ){
			computeL3HIT_TO_L3MISS_RATIO(inputs, outSensor, timestamp);
		} else if (outSensor->getMetric() == SMUCSensorBase::MEMORY_BANDWIDTH) {
			computeMEMORY_BANDWIDTH(inputs, outSensor, timestamp);
		} else if (isAMetricPerSecond(outSensor->getMetric())){
			computeMetricPerSecond(inputs, outSensor, timestamp);
		} else if (isAMetricRatio(outSensor->getMetric())){
			computeMetricRatio(inputs, outSensor, timestamp);
		} else if (isAProfileMetric(outSensor->getMetric())){
			computeProfileMetric(inputs, outSensor, timestamp);
		} else if (outSensor->getMetric() == SMUCSensorBase::INTER_NODE_LOADIMBALANCE ||
				outSensor->getMetric() == SMUCSensorBase::INTRA_NODE_LOADIMBALANCE) {
			computeLOADIMBALANCES(inputs, outSensor, timestamp);
		} else {
			auto found = _as_is_metric.find(outSensor->getMetric());
			if(found == _as_is_metric.end()){
				LOG(error) << "Metric as is " << outSensor->getMetric() << " not implemented.";
			} 
			SMUCSensorBase::Metric_t metric = found->second;
			query(inputs[_metricToPosition[metric]]->getName(), timestamp, _buffers[0]);
                        if(_buffers[0].size() > 0 )     {
                                outSensor->storeReading(_buffers[0][0]);
                        }
		}
		resetBuffers();
	}
}

void SMUCNGPerfOperator::query(const std::string & sensor_name, const uint64_t timestamp, vector<reading_t> &buffer){
	if(!_queryEngine.querySensor(sensor_name, timestamp, timestamp, buffer, false)) {
		//LOG(debug) << "SMUCNGPerf Operator " << _name << " cannot read from sensor " << sensor_name  << "!";
	}
}

void SMUCNGPerfOperator::resetBuffers(){
	for(auto &buffer: _buffers){
		buffer.clear();
	}

}

void SMUCNGPerfOperator::computeMetricPerSecond(std::vector<SMUCNGPtr> &inputs, SMUCNGPtr& outSensor, const uint64_t timestamp) {
	auto found = _metricPerSecToId.find(outSensor->getMetric());
	if(found == _metricPerSecToId.end()) { //not found
		LOG(error) << "Metric per second " << outSensor->getMetric() << " not implemented.";
		return;
	}
	SMUCSensorBase::Metric_t metric = found->second;
	query(inputs[_metricToPosition[metric]]->getName(), timestamp, _buffers[0]);
	reading_t metricpersec;
	if(_buffers[0].size() > 0 && calculateMetricPerSec(_buffers[0][0], _measuring_interval_s, metricpersec, *outSensor->getMetadata()->getScale())){
		outSensor->storeReading(metricpersec);
	}
}

void SMUCNGPerfOperator::computeMetricRatio(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp){
	std::vector<reading_t> & dividend = _buffers[0];
	std::vector<reading_t> & divisor = _buffers[1];
	auto found = _metricRatioToPair.find(outSensor->getMetric());
	if(found == _metricRatioToPair.end()) { //not found
		LOG(error) << "Metric ratio " << outSensor->getMetric() << " not implemented.";
		return;
	}
	SMUCSensorBase::Metric_t metric_dividend = found->second.first;
	SMUCSensorBase::Metric_t metric_divisor = found->second.second;
	query(inputs[_metricToPosition[metric_dividend]]->getName(), timestamp, dividend);
	query(inputs[_metricToPosition[metric_divisor]]->getName(), timestamp, divisor);
	reading_t ratio;
	if (dividend.size() > 0 && divisor.size() > 0 && calculateMetricRatio(dividend[0], divisor[0], ratio, *outSensor->getMetadata()->getScale())) {
		outSensor->storeReading(ratio);
	}
}

void SMUCNGPerfOperator::computeProfileMetric(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp){
	auto queryMetrics = _profileMetricToMetricIds[outSensor->getMetric()]; //should be here since this was queried in the compute() member function
	for(std::size_t i = 0; i < queryMetrics.size(); ++i){
		if(!_queryEngine.querySensor(inputs[_metricToPosition[queryMetrics[i]]]->getName(), timestamp - MS_TO_NS(_interval), timestamp, _buffers[i], false)){
			LOG(debug) << "Could not find data for metric id " << queryMetrics[i];
			return;
		}
	}
	auto value = computeSum(_buffers[0]);
	reading_t result;
	if(queryMetrics.size()==2){ //_buffer[0] and _buffer[1] should have been filled
		auto second_value = computeSum(_buffers[1]);
		if(second_value != 0){
			result.value =  value / (*outSensor->getMetadata()->getScale() * static_cast<double>(second_value));
			result.timestamp =  _buffers[0][0].timestamp;
			outSensor->storeReading(result);
		}
	} else { //only one buffer was filled
		result.value = value / (*outSensor->getMetadata()->getScale() * (_interval/1000.0));
		result.timestamp = _buffers[0][0].timestamp;
		outSensor->storeReading(result);
	}
}

void SMUCNGPerfOperator::computeLOADIMBALANCES(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp){
	//query for every cpu
	std::vector<reading_t> & cpus_vec = _buffers[0];
	for(auto & input: inputs){
		query(input->getName(), timestamp, cpus_vec);
	}
	if(cpus_vec.size() == 0) {
		return;
	}
	reading_t result;
	result.timestamp = cpus_vec.begin()->timestamp;
	if(outSensor->getMetric() == SMUCSensorBase::INTRA_NODE_LOADIMBALANCE){
		//calculate max - min
		auto smallest = std::min_element(cpus_vec.begin(), cpus_vec.end(),
				[](const reading_t& l, const reading_t& r) -> bool {return l.value < r.value;});

		auto largest = std::max_element(cpus_vec.begin(), cpus_vec.end(),
				[](const reading_t& l, const reading_t& r) -> bool {return l.value < r.value;});

		result.value = (largest->value - smallest->value)/_measuring_interval_s;

	} else { //outSensor->getMetric() == SMUCSensorBase::INTER_NODE_LOADIMBALANCE
		//calculate avg
		result.value = computeAvg(cpus_vec)/_measuring_interval_s;
	}
	outSensor->storeReading(result);
}

void SMUCNGPerfOperator::computePOWER(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr & outSensor, const uint64_t timestamp){
	//query for every socket
	std::vector<reading_t> & sockets_vec = _buffers[0];
	for(auto & input: inputs) {
		query(input->getName(), timestamp, sockets_vec);
	}
	if(sockets_vec.size() == 0){
		return;
	}
	reading_t result;
	result.timestamp = sockets_vec.begin()->timestamp;
 	double agg_value = 0.0;
	for(auto & socket_reading :sockets_vec){
		agg_value += socket_reading.value/1e6; //convertion to joules
	}
	
	result.value = agg_value/(_measuring_interval_s *  *outSensor->getMetadata()->getScale() );
	outSensor->storeReading(result);	
}


void SMUCNGPerfOperator::computeFREQUENCY(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr& outSensor, const uint64_t timestamp) {
	std::vector<reading_t> & clocks = _buffers[0];
	std::vector<reading_t> & clocks_ref = _buffers[1];
	query(inputs[_metricToPosition[SMUCSensorBase::CLOCKS]]->getName(), timestamp, clocks);
	query(inputs[_metricToPosition[SMUCSensorBase::CLOCKS_REF]]->getName(), timestamp, clocks_ref);
	reading_t frequency;
	if( clocks.size() > 0 && clocks_ref.size() > 0 && calculateFrequency(clocks_ref[0],clocks[0], MIN_FREQ_MHZ, MAX_FREQ_MHZ, frequency, *outSensor->getMetadata()->getScale())) {
		outSensor->storeReading(frequency);
	}
}

void SMUCNGPerfOperator::computeFLOPS(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr& outSensor, const uint64_t timestamp) {
	SMUCSensorBase::Metric_t flop_metric = outSensor->getMetric();
	std::vector<reading_t> & fp_arith_scalar_double = _buffers[0];
	std::vector<reading_t> & fp_arith_scalar_single = _buffers[1];
	std::vector<reading_t> & fp_arith_128b_packed_double = _buffers[2];
	std::vector<reading_t> & fp_arith_128b_packed_single = _buffers[3];
	std::vector<reading_t> & fp_arith_256b_packed_double = _buffers[4];
	std::vector<reading_t> & fp_arith_256b_packed_single = _buffers[5];
	std::vector<reading_t> & fp_arith_512b_packed_double = _buffers[6];
	std::vector<reading_t> & fp_arith_512b_packed_single = _buffers[7];


	query(inputs[_metricToPosition[SMUCSensorBase::FP_ARITH_SCALAR_DOUBLE]]->getName(), timestamp, fp_arith_scalar_double);
	query(inputs[_metricToPosition[SMUCSensorBase::FP_ARITH_SCALAR_SINGLE]]->getName(), timestamp, fp_arith_scalar_single);
	query(inputs[_metricToPosition[SMUCSensorBase::FP_ARITH_128B_PACKED_DOUBLE]]->getName(), timestamp, fp_arith_128b_packed_double);
	query(inputs[_metricToPosition[SMUCSensorBase::FP_ARITH_128B_PACKED_SINGLE]]->getName(), timestamp, fp_arith_128b_packed_single);
	query(inputs[_metricToPosition[SMUCSensorBase::FP_ARITH_256B_PACKED_DOUBLE]]->getName(), timestamp, fp_arith_256b_packed_double);
	query(inputs[_metricToPosition[SMUCSensorBase::FP_ARITH_256B_PACKED_SINGLE]]->getName(), timestamp, fp_arith_256b_packed_single);
	query(inputs[_metricToPosition[SMUCSensorBase::FP_ARITH_512B_PACKED_DOUBLE]]->getName(), timestamp, fp_arith_512b_packed_double);
	query(inputs[_metricToPosition[SMUCSensorBase::FP_ARITH_512B_PACKED_SINGLE]]->getName(), timestamp, fp_arith_512b_packed_single);
	reading_t empty;
	empty.value = 0;
	empty.timestamp = 0;
	reading_t & scalar_double = fp_arith_scalar_double.size() > 0 ? fp_arith_scalar_double[0]: empty;
	reading_t & scalar_single = fp_arith_scalar_single.size() > 0 ? fp_arith_scalar_single[0]: empty;
	reading_t & packed128_double = fp_arith_128b_packed_double.size() > 0 ? fp_arith_128b_packed_double[0] : empty;
	reading_t & packed128_single = fp_arith_128b_packed_single.size() > 0 ? fp_arith_128b_packed_single[0] : empty;
	reading_t & packed256_double =  fp_arith_256b_packed_double.size() > 0 ?  fp_arith_256b_packed_double[0] : empty;
	reading_t & packed256_single = fp_arith_256b_packed_single.size() > 0 ? fp_arith_256b_packed_single[0] : empty;
	reading_t & packed512_double =  fp_arith_512b_packed_double.size() > 0 ?  fp_arith_512b_packed_double[0] :empty;
	reading_t & packed512_single = fp_arith_512b_packed_single.size() > 0 ? fp_arith_512b_packed_single[0] : empty;

	reading_t result;
	switch (flop_metric) {
		case SMUCSensorBase::FLOPS:
			if (calculateFlopsPerSec(scalar_double, scalar_single, packed128_double,
							packed128_single, packed256_double, packed256_single,
							packed512_double, packed512_single, result, *outSensor->getMetadata()->getScale(), _measuring_interval_s) ) {
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::PACKED_FLOPS:
			if (calculatePackedFlopsPerSec(packed128_double, packed128_single,
					packed256_double, packed256_single, packed512_double,
					packed512_single, result, *outSensor->getMetadata()->getScale(), _measuring_interval_s)) {
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::VECTORIZATION_RATIO:
			if(calculateVectorizationRatio(scalar_double, scalar_single, packed128_double,
					packed128_single, packed256_double, packed256_single,
					packed512_double, packed512_single, result, *outSensor->getMetadata()->getScale())) {
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::AVX512_TOVECTORIZED_RATIO:
			if (calculateAVX512FlopsToVectorizedRatio(packed128_double,
					packed128_single, packed256_double, packed256_single,
					packed512_double, packed512_single, result, *outSensor->getMetadata()->getScale())) {
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::SINGLE_PRECISION_TO_TOTAL_RATIO:
			if(calculateSP_TO_TOTAL_RATIO(scalar_double, scalar_single, packed128_double,
					packed128_single, packed256_double, packed256_single,
					packed512_double, packed512_single, result, *outSensor->getMetadata()->getScale())){
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::PACKED128_FLOPS:
			if(calculatePacked128PerSec(packed128_double, packed128_single, result,
					*outSensor->getMetadata()->getScale(), _measuring_interval_s)){
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::PACKED256_FLOPS:
			if(calculatePacked256PerSec(packed256_double, packed256_single, result,
					*outSensor->getMetadata()->getScale(), _measuring_interval_s)){
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::PACKED512_FLOPS:
			if (calculatePacked512PerSec(packed512_double, packed512_single, result,
				*outSensor->getMetadata()->getScale(), _measuring_interval_s)) {
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::SINGLE_PRECISION_FLOPS:
			if (calculateSinglePrecisionPerSec(scalar_single, packed128_single,	packed256_single, packed512_single, result,
				*outSensor->getMetadata()->getScale(), _measuring_interval_s)) {
				outSensor->storeReading(result);
			}
			break;
		case SMUCSensorBase::DOUBLE_PRECISION_FLOPS:
			if (calculateDoublePerSec(scalar_double, packed128_double,
				packed256_double, packed512_double, result,
				*outSensor->getMetadata()->getScale(), _measuring_interval_s)) {
				outSensor->storeReading(result);
			}
			break;
		default:
			//no default...
			LOG(error) << "Flop metric " << flop_metric << " not implemented.";
			break;
	}
}



/*void SMUCNGPerfOperator::computeINSTR_INTRA_NODE_LOADIMBALANCE(std::vector<SMUCNGPtr>& inputs,
		SMUCNGPtr& outSensor, const uint64_t timestamp) {
}*/

void SMUCNGPerfOperator::computeL3_BANDWIDTH(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr& outSensor,
		const uint64_t timestamp) {
}

void SMUCNGPerfOperator::computeL3HIT_TO_L3MISS_RATIO(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr& outSensor,
		const uint64_t timestamp) {
	//MEM_LOAD_UOPS_RETIRED_L3_MISS/(MEM_LOAD_RETIRED_L3_HIT+MEM_LOAD_RETIRED_L3_MISS
	std::vector<reading_t> & l3_misses = _buffers[0];
	std::vector<reading_t> & l3_hits = _buffers[1];
	std::vector<reading_t> & l3_load_miss = _buffers[2];
	query(inputs[_metricToPosition[SMUCSensorBase::MEM_LOAD_UOPS_RETIRED_L3_MISS]]->getName(), timestamp, l3_misses);
	query(inputs[_metricToPosition[SMUCSensorBase::MEM_LOAD_RETIRED_L3_HIT]]->getName(), timestamp, l3_hits);
	query(inputs[_metricToPosition[SMUCSensorBase::MEM_LOAD_RETIRED_L3_MISS]]->getName(), timestamp, l3_load_miss);
	reading_t l3hitToMissRatio;
	if (l3_misses.size() > 0 && l3_hits.size() > 0 && l3_load_miss.size() > 0 && calculateL3HitToL3MissRatio(l3_misses[0], l3_hits[0], l3_load_miss[0], l3hitToMissRatio, *outSensor->getMetadata()->getScale())) {
		outSensor->storeReading(l3hitToMissRatio);
	}
}

void SMUCNGPerfOperator::computeMEMORY_BANDWIDTH(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr& outSensor, const uint64_t timestamp) {
	std::vector<reading_t> & mem_counters = _buffers[0];

	reading_t memory_bw;
	for(auto &input: inputs){
		query(input->getName(), timestamp, mem_counters);	
	}
	if(mem_counters.size() > 0 && calculateMemoryBandwidth(mem_counters, memory_bw, _measuring_interval_s, *outSensor->getMetadata()->getScale())){
		outSensor->storeReading(memory_bw);
	}
}

bool SMUCNGPerfOperator::isAMetricPerSecond(SMUCSensorBase::Metric_t comp){
	if(_metricPerSecToId.find(comp) != _metricPerSecToId.end() ){ //found
		return true;
	}
	return false;
}

bool SMUCNGPerfOperator::isAMetricRatio(SMUCSensorBase::Metric_t comp){
	if(_metricRatioToPair.find(comp) != _metricRatioToPair.end()){
		return true;
	}
	return false;
}

bool SMUCNGPerfOperator::isAProfileMetric(SMUCSensorBase::Metric_t comp){
	if(_profileMetricToMetricIds.find(comp) != _profileMetricToMetricIds.end()){
		return true;
	}
	return false;
}
