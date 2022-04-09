//================================================================================
// Name        : SKXPMUMetrics.cpp
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


#include "SKXPMUMetrics.h"


bool calculateFlopsPerSec(reading_t &scalarDB, reading_t & scalarSP,
        	reading_t & packedDP128, reading_t & packedSP128,
		    reading_t & packedDP256, reading_t & packedSP256,
		    reading_t & packedDP512, reading_t & packedSP512,
			reading_t & result, double scaling_factor, double measuring_interval_s) {
	if(!measuring_interval_s) return false;

	result.value = (packedDP128.value * 2 + packedSP128.value * 4
		+ packedDP256.value * 4 + packedSP256.value * 8
		+ packedDP512.value * 8 + packedSP512.value * 16 + scalarDB.value
		+ scalarSP.value)/(scaling_factor * measuring_interval_s);
	return getTimestampFromReadings(result.timestamp, scalarDB, scalarSP, packedDP128, packedSP128, packedDP256, packedSP256, packedDP512, packedSP512);
}

bool calculatePackedFlopsPerSec(reading_t & packedDP128, reading_t & packedSP128,
		    reading_t & packedDP256, reading_t & packedSP256,
		    reading_t & packedDP512, reading_t & packedSP512,
		    reading_t & result, double scaling_factor, double measuring_interval_s) {
	result.value = (packedDP128.value * 2 + packedSP128.value * 4
		+ packedDP256.value * 4 + packedSP256.value * 8
		+ packedDP512.value * 8 + packedSP512.value * 16)/(scaling_factor * measuring_interval_s);
	return getTimestampFromReadings(result.timestamp, packedDP128, packedSP128, packedDP256, packedSP256, packedDP512, packedSP512);
}

bool calculateVectorizationRatio(reading_t & scalarDB, reading_t & scalarSP,
		reading_t & packedDP128, reading_t & packedSP128,
		reading_t & packedDP256, reading_t & packedSP256,
		reading_t & packedDP512, reading_t & packedSP512, reading_t & result, double scaling_factor) {
	double denominator = static_cast<double>(scalarDB.value + scalarSP.value +
			packedDP128.value + packedSP128.value +
			packedDP256.value + packedSP256.value +
			packedDP512.value + packedSP512.value);
	if(denominator > 0 ){
		result.value = (packedDP128.value + packedSP128.value + packedDP256.value
				+ packedSP256.value + packedDP256.value + packedSP512.value + packedDP512.value)/(scaling_factor * denominator);
		return getTimestampFromReadings(result.timestamp, scalarDB, scalarSP,
				packedDP128, packedSP128, packedDP256, packedSP256, packedDP512,
				packedSP512);
	}
	return false;
}

bool calculateAVX512FlopsToVectorizedRatio(reading_t & packedDP128, reading_t & packedSP128,
	    reading_t & packedDP256, reading_t & packedSP256,
	    reading_t & packedDP512, reading_t & packedSP512,
	    reading_t & result, double scaling_factor){
	double denominator = static_cast<double>(packedDP128.value + packedSP128.value +
			packedDP256.value + packedSP256.value +
			packedDP512.value + packedSP512.value);
	if(denominator > 0){
		result.value = (packedDP512.value * 8 + packedSP512.value * 16)/( scaling_factor * denominator);
		return getTimestampFromReadings(result.timestamp, packedDP128, packedSP128, packedDP256, packedSP256, packedDP512, packedSP512);
	}
	return false;
}


bool calculateSP_TO_TOTAL_RATIO(reading_t &scalarDB, reading_t & scalarSP,
		reading_t & packedDP128, reading_t & packedSP128,
		reading_t & packedDP256, reading_t & packedSP256,
		reading_t & packedDP512, reading_t & packedSP512,
		reading_t & result, double scaling_factor){
	auto single_precision = packedSP128.value * 4 + packedSP256.value * 8
			+ packedSP512.value * 16 + scalarSP.value;
	double total = single_precision + packedDP128.value * 2 + packedDP256.value * 4 + packedDP512.value * 8 + scalarDB.value;
	if(total > 0){
		result.value = single_precision/(scaling_factor * total);
		return getTimestampFromReadings(result.timestamp, scalarDB, scalarSP,
				packedDP128, packedSP128, packedDP256, packedSP256, packedDP512,
				packedSP512);
	}
	return false;
}

bool calculateL3HitToL3MissRatio(reading_t & l3_misses, reading_t& l3_load_hits,
		reading_t & l3_load_misses, reading_t & result, double scaling_factor){
	double denominator = static_cast<double>(l3_load_hits.value + l3_load_misses.value);
	if(denominator > 0){
		result.value = l3_misses.value/(scaling_factor * denominator);
		return getTimestampFromReadings(result.timestamp, l3_misses, l3_load_hits, l3_load_misses);
	}
	return false;
}

bool calculateMemoryBandwidth(std::vector<reading_t> membw_counts, reading_t & result, double measuring_interval_s, double scaling_factor ) {
	result.value = 0;
	const unsigned int SIZEPERACCESS = 64;
	bool ret_val = false;
	for(auto &rt : membw_counts){
		result.value += rt.value * SIZEPERACCESS;
		if(rt.timestamp != 0){
			result.timestamp = rt.timestamp;
			ret_val = true;
		}
	}
        result.value = result.value/ ( scaling_factor * measuring_interval_s);
	return ret_val;
}


/**
 * For CPI, LoadsToStore, Branch rate, miss branch ratio, etc..
 */
bool calculateMetricRatio(reading_t & dividend, reading_t & divisor, reading_t & result, double scaling_factor) {
	if(divisor.value > 0){
		result.value =  dividend.value / (scaling_factor * static_cast<double>(divisor.value));
		return getTimestampFromReadings(result.timestamp, dividend, divisor);
	}
	return false; //Division by zero
}

/** Any generic metric per second. For instance: instructions per second, l2 misses per second **/
bool calculateMetricPerSec(reading_t & metric, double measuring_interval_s, reading_t & result, double scaling_factor) {
	if(measuring_interval_s > 0) {
		result.value = metric.value / (scaling_factor * measuring_interval_s);
		return getTimestampFromReadings(result.timestamp, metric);
	}
	return false; //Division by zero
}

bool calculateFrequency(reading_t & unhaltedRef, reading_t & unhaltedClocks,
		unsigned int min_freq, unsigned int max_freq, reading_t & result, double scaling_factor) {
	if(unhaltedRef.value > 0){
	        double resValue = (unhaltedClocks.value / static_cast<double>(unhaltedRef.value)) * max_freq;
		if(resValue > (max_freq * 1.1) || resValue < (min_freq*0.9)) { //There is something wrong here...
			return false;
		}
		result.value = resValue/scaling_factor;
		return getTimestampFromReadings(result.timestamp, unhaltedRef, unhaltedClocks);
	}
	return false; //Division by zero
}

bool calculatePacked128PerSec(reading_t & packedDP128, reading_t & packedSP128,
		reading_t & result, double scaling_factor, double measuring_interval_s){
	if(!measuring_interval_s) return false;

	result.value = (packedDP128.value * 2 + packedSP128.value * 4)/(scaling_factor * measuring_interval_s);
	return getTimestampFromReadings(result.timestamp, packedDP128, packedSP128);
}

bool calculatePacked256PerSec(reading_t & packedDP256, reading_t & packedSP256,
		reading_t & result, double scaling_factor, double measuring_interval_s){
	if(!measuring_interval_s) return false;

	result.value = (packedDP256.value * 4 + packedSP256.value * 8)/(scaling_factor * measuring_interval_s);
	return getTimestampFromReadings(result.timestamp, packedDP256, packedSP256);
}

bool calculatePacked512PerSec(reading_t & packedDP512, reading_t & packedSP512,
		reading_t & result, double scaling_factor, double measuring_interval_s){
	if(!measuring_interval_s) return false;

	result.value = (packedDP512.value * 8 + packedSP512.value * 16)/(scaling_factor * measuring_interval_s);
	return getTimestampFromReadings(result.timestamp, packedDP512, packedSP512);
}

bool calculateSinglePrecisionPerSec(reading_t & scalarSP, reading_t & packedSP128,
		reading_t & packedSP256, reading_t & packedSP512,
		reading_t & result, double scaling_factor, double measuring_interval_s) {
	if(!measuring_interval_s) return false;

	result.value = (packedSP128.value * 4 + packedSP256.value * 8 + packedSP512.value * 16 + scalarSP.value)/(scaling_factor * measuring_interval_s);
	return getTimestampFromReadings(result.timestamp, scalarSP, packedSP128, packedSP256, packedSP512);
}

bool calculateDoublePerSec(reading_t &scalarDB,  reading_t & packedDP128,
		reading_t & packedDP256, reading_t & packedDP512,
		reading_t & result, double scaling_factor, double measuring_interval_s) {
	if(!measuring_interval_s) return false;

	result.value = (packedDP128.value * 2 + packedDP256.value * 4
		+ packedDP512.value * 8 + scalarDB.value)/(scaling_factor * measuring_interval_s);
	return getTimestampFromReadings(result.timestamp, scalarDB, packedDP128, packedDP256, packedDP512);
}
