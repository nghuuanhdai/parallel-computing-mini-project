//================================================================================
// Name        : CSOperator.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description :
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
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

#ifndef PROJECT_CSOPERATOR_H
#define PROJECT_CSOPERATOR_H


#include "../../includes/OperatorTemplate.h"
#include "CSSensorBase.h"
#include <boost/foreach.hpp>
#include <boost/math/interpolators/cubic_b_spline.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "opencv4/opencv2/core.hpp"
#include "opencv4/opencv2/core/mat.hpp"
#include "opencv4/opencv2/core/cvstd.hpp"
#include <fstream>
#include <set>
#include <math.h>

/**
 * @brief CS Smoothing operator plugin.
 *
 * @ingroup cs-smoothing
 */
class CSOperator : virtual public OperatorTemplate<CSSensorBase> {

public:

    CSOperator(const std::string& name);
    CSOperator(const CSOperator& other);

    virtual ~CSOperator();

    virtual restResponse_t REST(const string& action, const unordered_map<string, string>& queries) override;

    virtual void execOnInit() override;

    void setInputPath(std::string in)                    { _modelIn = in; }
    void setOutputPath(std::string out)                  { _modelOut = out; }
    void setAggregationWindow(unsigned long long a)      { _aggregationWindow = a; }
    void setTrainingSamples(unsigned long long s)        { if(s>100) _trainingSamples = s; }
    void setNumBlocks(unsigned long long b)              { if(b>0) _numBlocks = b; }
    void setScalingFactor(unsigned long long sf)         { if(sf>0) _scalingFactor = sf; }
    void setReuseModel(bool r)                           { _reuseModel = r; }

    void triggerTraining()                               { _trainingPending = true; }

    std::string getInputPath()                           { return _modelIn;}
    std::string getOutputPath()                          { return _modelOut; }
    unsigned long long getAggregationWindow()            { return _aggregationWindow; }
    unsigned long long getTrainingSamples()              { return _trainingSamples; }
    unsigned long long getNumBlocks()                    { return _numBlocks; }
    unsigned long long getScalingFactor()                { return _scalingFactor; }
    bool getReuseModel()                                 { return _reuseModel; }

    void printConfig(LOG_LEVEL ll) override;

protected:

    virtual void compute(U_Ptr unit) override;
    void computeSignature(U_Ptr unit, uint64_t nowTs);

    bool dumpToFile(std::string &path);
    bool readFromFile(std::string &path);

    void accumulateData(std::vector<std::vector<reading_t>>& v, CSSBPtr s, size_t idx, uint64_t nowTs);
    void computeMinMax(std::vector<std::vector<reading_t>>& v);
    void computePermutation(std::vector<std::vector<reading_t>>& v);
    bool checkTrainingSet(std::vector<std::vector<reading_t>>& v);
    
    void normalize(std::vector<reading_t> &v, size_t idx);
    int64_t getAvg(std::vector<reading_t> &v);
    int64_t getDer(std::vector<reading_t> &v);
    void convertToCorrelation(cv::Mat &m);
    
    std::string _modelOut;
    std::string _modelIn;
    unsigned long long _aggregationWindow;
    unsigned long long _trainingSamples;
    unsigned long long _numBlocks;
    unsigned long long _scalingFactor;
    long long _trainingReady;
    bool _trainingPending;
    bool _reuseModel;
    
    
    // CS data
    unsigned long long _actualBlocks;
    float _blockLen;
    std::vector<size_t>  _permVector;
    std::vector<int64_t> _min;
    std::vector<int64_t> _max;
    std::vector<std::vector<reading_t>> _trainingData;
    std::string _trainingUnit;
    
    // Misc buffers
    std::vector<int64_t> _avgBuffer;
    std::vector<int64_t> _derBuffer;
    std::vector<reading_t> _buffer;
    size_t _bBegin;
    size_t _bEnd;
};


#endif //PROJECT_CSOPERATOR_H
