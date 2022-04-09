/*
 * CommonStatistics.h
 *
 *  Created on: Aug 7, 2019
 *      Author: carla
 */

#ifndef ANALYTICS_INCLUDES_COMMONSTATISTICS_H_
#define ANALYTICS_INCLUDES_COMMONSTATISTICS_H_

#include <vector>
#include <cacheentry.h>

int64_t computeObs(std::vector<reading_t> &buffer) {
    return buffer.size();
}

int64_t computeSum(std::vector<reading_t> &buffer) {
    int64_t acc=0;
    for(const auto& v : buffer)
            acc += v.value;
    return acc;
}

int64_t computeAvg(std::vector<reading_t> &buffer) {
    int64_t acc=0, ctr=buffer.size();
    for(const auto& v : buffer)
        acc += v.value;
    acc = ctr > 0 ? acc/ctr : acc;
    return acc;
}

int64_t computeMax(std::vector<reading_t> &buffer) {
    int64_t acc=0;
    bool maxInit=false;
    for(const auto& v : buffer)
        if(v.value>acc || !maxInit) {
            acc = v.value;
            maxInit = true;
        }
    return acc;
}

int64_t computeMin(std::vector<reading_t> &buffer) {
    int64_t acc=0;
    bool minInit=false;
    for(const auto& v : buffer)
        if(v.value<acc || !minInit) {
            acc = v.value;
            minInit = true;
        }
    return acc;
}

int64_t computeStd(std::vector<reading_t> &buffer) {
    int64_t avg = computeAvg(buffer);
    int64_t acc=0, val=0, ctr=buffer.size();
    for(const auto& v : buffer) {
        val = v.value - avg;
        acc += val*val;
    }
    acc = ctr > 0 ? sqrt(acc/ctr) : sqrt(acc);
    return acc;
}

void computeEvenQuantiles(std::vector<reading_t> &data, const unsigned int NUMBER_QUANTILES, std::vector<int64_t> &quantiles) {
	if (data.empty() || NUMBER_QUANTILES == 0) {
		return;
	}
	std::sort(data.begin(), data.end(), [ ](const reading_t& lhs, const reading_t& rhs) { return lhs.value < rhs.value; });
	int elementNumber = data.size();
	quantiles.resize(NUMBER_QUANTILES + 1); //+min
	float factor = elementNumber/static_cast<float>(NUMBER_QUANTILES);
	quantiles[0] = data[0].value; //minimum
	quantiles[NUMBER_QUANTILES] = data[data.size() - 1].value; //maximum
	for (unsigned int i = 1; i < NUMBER_QUANTILES; i++) {
		if (elementNumber > 1) {
			int idx = static_cast<int>(std::floor(i * factor));
			if(idx == 0){
				quantiles[i] = data[0].value;
			} else {
				float rest = (i * factor) - idx;
				quantiles[i] = data[idx - 1].value + rest * (data[idx].value - data[idx - 1].value); //ToDo scaling factor??
			}
		} else { //optimization, we don't need to calculate all the quantiles
			quantiles[i] = data[0].value;
		}
	}
}

void computePercentiles(std::vector<reading_t> &data, const std::vector<size_t> &percentilePositions, std::vector<int64_t> &percentiles) {
    if (data.empty() || percentilePositions.empty())
        return;
    
    size_t idx, mod;
    percentiles.clear();
    // Sorting the sensor reading buffer to extract quantiles
    std::sort(data.begin(), data.end(), [ ](const reading_t& lhs, const reading_t& rhs) { return lhs.value < rhs.value; });
    for(const auto& q : percentilePositions) {
        idx = ((data.size()-1) * q) / 100;
        mod = ((data.size()-1) * q) % 100;
        percentiles.push_back((mod==0 || idx==data.size()-1) ? data[idx].value : (data[idx].value + data[idx+1].value)/2);
    }
}

#endif /* ANALYTICS_INCLUDES_COMMONSTATISTICS_H_ */

