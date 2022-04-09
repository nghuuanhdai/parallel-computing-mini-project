//================================================================================
// Name        : SKXPMUMetrics.h
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

#ifndef ANALYTICS_ANALYZERS_SMUCNGPERFANALYZER_SKXDERIVEDMETRICS_SKXPMUMETRICS_H_
#define ANALYTICS_ANALYZERS_SMUCNGPERFANALYZER_SKXDERIVEDMETRICS_SKXPMUMETRICS_H_
#include <vector>
#include "cacheentry.h"

template<typename ...Args>
bool getTimestampFromReadings(uint64_t & timestamp, Args&...args){
	reading_t readings[] = {args...};
	for(auto & reading: readings){
		if(reading.timestamp != 0){
			timestamp = reading.timestamp;
			return true;
		}
	}
	return false;
}

bool calculateFlopsPerSec(reading_t &scalarDB, reading_t & scalarSP,
		reading_t & packedDP128, reading_t & packedSP128,
		reading_t & packedDP256, reading_t & packedSP256,
		reading_t & packedDP512, reading_t & packedSP512,
		reading_t & result, double scaling_factor, double measuring_interval_s);

bool calculatePackedFlopsPerSec(reading_t & packedDP128, reading_t & packedSP128,
		    reading_t & packedDP256, reading_t & packedSP256,
		    reading_t & packedDP512, reading_t & packedSP512,
		    reading_t & result, double scaling_factor, double measuring_interval_s);

bool calculateVectorizationRatio(reading_t & scalarDB, reading_t & scalarSP,
		reading_t & packedDP128, reading_t & packedSP128,
		reading_t & packedDP256, reading_t & packedSP256,
		reading_t & packedDP512, reading_t & packedSP512, reading_t & result, double scaling_factor);


bool calculateAVX512FlopsToVectorizedRatio(reading_t & packedDP128, reading_t & packedSP128,
	    reading_t & packedDP256, reading_t & packedSP256,
	    reading_t & packedDP512, reading_t & packedSP512,
	    reading_t & result, double scaling_factor);

bool calculateSP_TO_TOTAL_RATIO(reading_t &scalarDB, reading_t & scalarSP,
		reading_t & packedDP128, reading_t & packedSP128,
		reading_t & packedDP256, reading_t & packedSP256,
		reading_t & packedDP512, reading_t & packedSP512, 
		reading_t & result, double scaling_factor);

bool calculatePacked128PerSec(reading_t & packedDP128, reading_t & packedSP128,
		reading_t & result, double scaling_factor, double measuring_interval_s);

bool calculatePacked256PerSec(reading_t & packedDP256, reading_t & packedSP256,
		reading_t & result, double scaling_factor, double measuring_interval_s);

bool calculatePacked512PerSec(reading_t & packedDP512, reading_t & packedSP512,
		reading_t & result, double scaling_factor, double measuring_interval_s);

bool calculateSinglePrecisionPerSec(reading_t & scalarSP, reading_t & packedSP128,
		reading_t & packedSP256, reading_t & packedSP512,
		reading_t & result, double scaling_factor, double measuring_interval_s);

bool calculateDoublePerSec(reading_t &scalarDB,  reading_t & packedDP128,
		reading_t & packedDP256, reading_t & packedDP512,
		reading_t & result, double scaling_factor, double measuring_interval_s);

bool calculateL3HitToL3MissRatio(reading_t & l3_misses, reading_t& l3_load_hits,
		reading_t & l3_load_misses, reading_t & result, double scaling_factor);

bool calculateMemoryBandwidth(std::vector<reading_t> membw_counts, reading_t & result, double measuring_interval_s, double scaling_factor);

bool calculateMetricRatio(reading_t & dividend, reading_t & divisor, reading_t & result, double scaling_factor);

/** Any generic metric per second. For instance: instructions per second, l2 misses per second **/
bool calculateMetricPerSec(reading_t & metric, double measuring_interval_s, reading_t & result, double scaling_factor) ;

bool calculateFrequency(reading_t & unhaltedRef, reading_t & unhaltedClocks,
		unsigned int min_freq, unsigned int max_freq, reading_t & result, double scaling_factor);


#endif /* ANALYTICS_ANALYZERS_SMUCNGPERFANALYZER_SKXDERIVEDMETRICS_SKXPMUMETRICS_H_ */
