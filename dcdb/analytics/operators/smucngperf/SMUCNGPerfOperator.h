//================================================================================
// Name        : SMUCNGPerfOperator.h
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

#ifndef ANALYTICS_SMUCNGPERF_SMUCNGPERFOPERATOR_H_
#define ANALYTICS_SMUCNGPERF_SMUCNGPERFOPERATOR_H_

#include "../../includes/OperatorTemplate.h"
#include "SMUCSensorBase.h"
#include "SKXPMUMetrics.h"
#include <map>
#include <set>

class SMUCNGPerfOperator: virtual public OperatorTemplate<SMUCSensorBase>{
public:
    SMUCNGPerfOperator(const std::string& name);
    virtual ~SMUCNGPerfOperator();
    SMUCNGPerfOperator(const SMUCNGPerfOperator& other);
    SMUCNGPerfOperator& operator=(const SMUCNGPerfOperator& other);
    void printConfig(LOG_LEVEL ll) override;

    void setMetricToPosition(const std::map<SMUCSensorBase::Metric_t,unsigned int>&metricToPosition) {
		_metricToPosition = metricToPosition;
    }

    void setMeasuringInterval(double measurement_interval_s){
    	_measuring_interval_s = measurement_interval_s;
    }
    void setGoBackMs(int go_back_ms){
    	_go_back_ns = MS_TO_NS(go_back_ms);
    }

protected:
    std::map<SMUCSensorBase::Metric_t, unsigned int> _metricToPosition;
    std::map<SMUCSensorBase::Metric_t, SMUCSensorBase::Metric_t> _metricPerSecToId;
    std::map<SMUCSensorBase::Metric_t, std::pair<SMUCSensorBase::Metric_t, SMUCSensorBase::Metric_t>> _metricRatioToPair;
    std::map<SMUCSensorBase::Metric_t, std::vector<SMUCSensorBase::Metric_t>> _profileMetricToMetricIds;
    std::set<SMUCSensorBase::Metric_t> _flop_metric;
    std::map<SMUCSensorBase::Metric_t, SMUCSensorBase::Metric_t> _as_is_metric;
    vector<vector<reading_t>> _buffers;
    const unsigned int MAX_FREQ_MHZ = 2700;
    const unsigned int MIN_FREQ_MHZ = 1200;
    uint64_t _go_back_ns;
    double _measuring_interval_s; //<! preferred as double since we need decimal places 
                                  //   and use it in divisionis (metric/s)

    virtual void compute(U_Ptr unit) override;


    void resetBuffers();
    void query(const std::string & sensor_name, const uint64_t timestamp, vector<reading_t> &buffer);

    void copy(const SMUCNGPerfOperator& other);

    void computeMetricRatio(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp);
    void computeMetricPerSecond(std::vector<SMUCNGPtr> &inputs, SMUCNGPtr& outSensor, const uint64_t timestamp);

    void computeFREQUENCY(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp);
	void computeFLOPS(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp);
	void computeLOADIMBALANCES(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp);
	void computeL3_BANDWIDTH(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp);
	void computeL3HIT_TO_L3MISS_RATIO(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp);
	void computeMEMORY_BANDWIDTH(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp);
	void computeProfileMetric(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr &outSensor, const uint64_t timestamp);

	/**
         * Both DRAM and PACKAGE are calculated the same way: first divide by the interval and then 
         * aggregate socket-wise
	 **/
	void computePOWER(std::vector<SMUCNGPtr>& inputs, SMUCNGPtr & outSensor, const uint64_t timestamp);

	bool isAMetricPerSecond(SMUCSensorBase::Metric_t comp);
	bool isAMetricRatio(SMUCSensorBase::Metric_t comp);
	bool isAProfileMetric(SMUCSensorBase::Metric_t comp);
};

#endif /* ANALYTICS_SMUCNGPERF_SMUCNGPERFOPERATOR_H_ */
