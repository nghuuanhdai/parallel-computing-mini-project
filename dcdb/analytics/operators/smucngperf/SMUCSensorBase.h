//================================================================================
// Name        : SMUCSensorBase.h
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

#ifndef ANALYTICS_SMUCNGPERF_SMUCSENSORBASE_H_
#define ANALYTICS_SMUCNGPERF_SMUCSENSORBASE_H_

#include "sensorbase.h"

class SMUCSensorBase : public SensorBase {
public:
	enum Metric_t {
		INSTRUCTIONS=0,
		CLOCKS=1,
		CLOCKS_REF=2,
		USERPCT=3,
		SYSTEMPCT=4,
		IOWAITPCT=5,
		MEMUSED=6,
		IOBYTESREAD=7,
		IOBYTESWRITE=8,
		IOOPENS=9,
		IOCLOSES=10,
		IOREADS=11,
		IOWRITES=12,
		NETWORK_XMIT_BYTES=13,
		NETWORK_RCVD_BYTES=14,
		NETWORK_XMIT_PKTS=15,
		NETWORK_RCVD_PKTS=16,
		L2_RQSTS_MISS=17,
		ARITH_FPU_DIVIDER_ACTIVE=18,
		FP_ARITH_SCALAR_DOUBLE=19,
		FP_ARITH_SCALAR_SINGLE=20,
		FP_ARITH_128B_PACKED_DOUBLE=21,
		FP_ARITH_128B_PACKED_SINGLE=22,
		FP_ARITH_256B_PACKED_DOUBLE=23,
		FP_ARITH_256B_PACKED_SINGLE=24,
		FP_ARITH_512B_PACKED_DOUBLE=25,
		FP_ARITH_512B_PACKED_SINGLE=26,
		MEM_INST_RETIRED_ALL_LOADS=27,
		MEM_INST_RETIRED_ALL_STORES=28,
		MEM_LOAD_UOPS_RETIRED_L3_MISS=29,
		MEM_LOAD_RETIRED_L3_HIT=30,
		MEM_LOAD_RETIRED_L3_MISS=31,
		PERF_COUNT_HW_BRANCH_INSTRUCTIONS=32,
		PERF_COUNT_HW_BRANCH_MISSES=33,
		CORE_TEMPERATURE=34,
		CAS_COUNT_READ0=35,
		CAS_COUNT_READ1=36,
		CAS_COUNT_READ2=37,
		CAS_COUNT_READ3=38,
		CAS_COUNT_READ4=39,
		CAS_COUNT_READ5=40,
		CAS_COUNT_WRITE0=41,
		CAS_COUNT_WRITE1=42,
		CAS_COUNT_WRITE2=43,
		CAS_COUNT_WRITE3=44,
		CAS_COUNT_WRITE4=45,
		CAS_COUNT_WRITE5=46,
		PKG0_ENERGY=1000,
		PKG1_ENERGY=1001,
		DRAM0_ENERGY=1002,
		DRAM1_ENERGY=1003,
		//
		CPI=50,
		FREQUENCY=51,
		INSTRUCTIONS_PER_SECOND=52,
		FLOPS=53,
		PACKED_FLOPS=54,
		AVX512_TOVECTORIZED_RATIO=55, //AVX512/(TOTAL VECTORIZED)
		VECTORIZATION_RATIO=56, //(TOTAL VECTORIZED)/(ALL FLOPS)
		SINGLE_PRECISION_TO_TOTAL_RATIO=57, //Flops
		EXPENSIVE_INSTRUCTIONS_PER_SECOND=58,
		INTRA_NODE_LOADIMBALANCE=59,
		INTER_NODE_LOADIMBALANCE=60,
		L2_HITS_PER_SECOND=61,
		L2_MISSES_PER_SECOND=62,
		L3_HITS_PER_SECOND=63,
		L3_MISSES_PER_SECOND=64,
		L3_TO_INSTRUCTIONS_RATIO=65,
		L3_BANDWIDTH=66,
		L3HIT_TO_L3MISS_RATIO=67,
		LOADS_TO_STORES=68,
		LOADS_TOL3MISS_RATIO=69,
		MISSBRANCHES_PER_SECOND=70,
		BRANCH_PER_INSTRUCTIONS=71,
		MISSBRANCHES_TO_TOTAL_BRANCH_RATIO=72,
		MEMORY_BANDWIDTH=73,
		RAPL_PKG=74,
		RAPL_MEM=75,
		IPMI_CPU=76,
		IPMI_MEM=77,
		IPMI_DC=78,
		IPMI_AC=79,
		NETWORK_XMIT_BYTES_PER_PKT=80,
		NETWORK_BYTES_XMIT_PER_SECOND=81,
		NETWORK_RCV_BYTES_PER_PKT=82,
		NETWORK_BYTES_RCVD_PER_SECOND=83,
		IOOPENS_PER_SECOND=84,
		IOCLOSES_PER_SECOND=85,
		IOBYTESREAD_PER_SECOND=86,
		IOBYTESWRITE_PER_SECOND=87,
		IOREADS_PER_SECOND=88,
		IOWRITES_PER_SECOND=89,
		IO_BYTES_READ_PER_OP=90,
		IO_BYTES_WRITE_PER_OP=91,
		IOBYTESREAD_PER_SECOND_PROF=92,
		IOBYTESWRITE_PER_SECOND_PROF=93,
		IOREADS_PER_SECOND_PROF=94,
		IOWRITES_PER_SECOND_PROF=95,
		IO_BYTES_READ_PER_OP_PROF=96,
		IO_BYTES_WRITE_PER_OP_PROF=97,
		PACKED128_FLOPS=98,
		PACKED256_FLOPS=99,
		PACKED512_FLOPS=100,
		SINGLE_PRECISION_FLOPS=101,
		DOUBLE_PRECISION_FLOPS=102,
		PKG_POWER=200,
		DRAM_POWER=201,
		USERPCT0=300,
		SYSTEMPCT0=301,
		IOWAITPCT0=302,
		NONE
	};
public:
	SMUCSensorBase(const std::string& name): SensorBase(name), _position(0), _metric(NONE) {}
	SMUCSensorBase(const SMUCSensorBase& other): SensorBase(other){
		copy(other);
	}
	SMUCSensorBase& operator=(const SMUCSensorBase& other){
 		SensorBase::operator=(other);
		copy(other);
		return *this;
	}
	virtual ~SMUCSensorBase(){}

	
	void setMetric(Metric_t metric){
		_metric = metric;
	}

	Metric_t getMetric(){
		return _metric;
	}

	unsigned int getPosition(){
		return _position;
	}

	void setPosition(unsigned int position){
		_position = position;
	}

private:
	unsigned int _position;
	Metric_t _metric;

	void copy(const SMUCSensorBase & other){
                _position = other._position;
                _metric = other._metric;
	}
};

using SMUCNGPtr = std::shared_ptr<SMUCSensorBase>;

#endif /* ANALYTICS_SMUCNGPERF_SMUCSENSORBASE_H_ */
