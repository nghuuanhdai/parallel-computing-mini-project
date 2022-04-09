//================================================================================
// Name        : RegressorOperator.h
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

#ifndef PROJECT_REGRESSOROPERATOR_H
#define PROJECT_REGRESSOROPERATOR_H


#include "../../includes/OperatorTemplate.h"
#include "RegressorSensorBase.h"
#include "opencv4/opencv2/core/mat.hpp"
#include "opencv4/opencv2/core/cvstd.hpp"
#include "opencv4/opencv2/ml.hpp"
#include <math.h>
#include <random>

#define REG_NUMFEATURES 6

/**
 * @brief Regressor operator plugin.
 *
 * @ingroup regressor
 */
class RegressorOperator : virtual public OperatorTemplate<RegressorSensorBase> {

public:
    
    RegressorOperator(const std::string& name);
    RegressorOperator(const RegressorOperator& other);
    
    virtual ~RegressorOperator();

    virtual restResponse_t REST(const string& action, const unordered_map<string, string>& queries) override;

    virtual void execOnInit() override;

    void setInputPath(std::string in)                    { _modelIn = in; }
    void setOutputPath(std::string out)                  { _modelOut = out; }
    void setAggregationWindow(unsigned long long a)      { _aggregationWindow = a; }
    void setTrainingSamples(unsigned long long t)        { _trainingSamples = t; }
    void setTargetDistance(unsigned long long d)         { _targetDistance = d;  }
    void setComputeImportances(bool i)                   { _importances = i; }
    void setRawMode(bool r)                              { _numFeatures = r ? 1 : REG_NUMFEATURES; }
    void triggerTraining()                               { _trainingPending = true;}
    void setSmoothResponses(bool s)                      { _smoothResponses = s; }

    std::string getInputPath()                           { return _modelIn;}
    std::string getOutputPath()                          { return _modelOut; }
    unsigned long long getAggregationWindow()            { return _aggregationWindow; }
    unsigned long long getTrainingSamples()              { return _trainingSamples; }
    bool getComputeImportances()                         { return _importances; }
    bool getRawMode()                                    { return _numFeatures != REG_NUMFEATURES; }
    bool getSmoothResponses()                            { return _smoothResponses; }

    virtual void printConfig(LOG_LEVEL ll) override;

protected:

    virtual void compute(U_Ptr unit)	 override;
    bool computeFeatureVector(U_Ptr unit);
    void trainRandomForest(bool categorical=false);
    void shuffleTrainingSet();
    void smoothResponsesArray();
    std::string getImportances();
    
    std::string _modelOut;
    std::string _modelIn;
    unsigned long long _aggregationWindow;
    unsigned long long _trainingSamples;
    unsigned long long _targetDistance;
    unsigned long long _numFeatures;
    unsigned long long _numInputs;
    bool _smoothResponses;
    bool _trainingPending;
    bool _importances;
    bool _includeTarget;

    vector<reading_t> _buffer;
    cv::Ptr<cv::ml::RTrees> _rForest;
    cv::Mat *_trainingSet;
    cv::Mat *_responseSet;
    cv::Mat *_currentfVector;
    float _currentTarget;
    bool _targetFound;

    // Misc buffers
    int64_t _mean;
    int64_t _std;
    int64_t _diffsum;
    int64_t _qtl25;
    int64_t _qtl75;
    int64_t _latest;

    // Helper struct to store importance pairs
    struct ImportancePair {
        std::string name;
        float value;
    };

};


#endif //PROJECT_REGRESSOROPERATOR_H
