//================================================================================
// Name        : ClusteringOperator.h
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

#ifndef PROJECT_CLUSTERINGOPERATOR_H
#define PROJECT_CLUSTERINGOPERATOR_H


#include "../../includes/OperatorTemplate.h"
#include "ClusteringSensorBase.h"
#include "opencv4/opencv2/core/mat.hpp"
#include "opencv4/opencv2/core/cvstd.hpp"
#include "opencv4/opencv2/ml.hpp"
#include <math.h>
#include <random>

#define OUTLIER_ID 1000

/**
 * @brief Clustering operator plugin.
 *
 * @ingroup clustering
 */
class ClusteringOperator : virtual public OperatorTemplate<ClusteringSensorBase> {

public:

    ClusteringOperator(const std::string& name);
    ClusteringOperator(const ClusteringOperator& other);
    
    virtual ~ClusteringOperator();

    virtual restResponse_t REST(const string& action, const unordered_map<string, string>& queries) override;

    virtual void execOnInit() override;

    void setInputPath(std::string in)                    { _modelIn = in; }
    void setOutputPath(std::string out)                  { _modelOut = out; }
    void setAggregationWindow(unsigned long long a)      { _aggregationWindow = a; }
    void setLookbackWindow(unsigned long long w)         { _lookbackWindow = w; }
    void setNumComponents(unsigned long long n)          { _numComponents = n; }
    void setOutlierCut(float s)                          { _outlierCut = s; }
    void setReuseModel(bool r)                           { _reuseModel = r; }
    
    void triggerTraining()                               { _trainingPending = true; }

    std::string getInputPath()                           { return _modelIn;}
    std::string getOutputPath()                          { return _modelOut; }
    unsigned long long getAggregationWindow()            { return _aggregationWindow; }
    unsigned long long getLookbackWindow()               { return _lookbackWindow; }
    unsigned long long getNumComponents()                { return _numComponents; }
    float getOutlierCut()                                { return _outlierCut; }
    bool getReuseModel()                                 { return _reuseModel; }
    
    void printConfig(LOG_LEVEL ll) override;

protected:

    virtual void compute(U_Ptr unit)	 override;
    bool computeFeatureVector(U_Ptr unit, uint64_t offset=0);
    bool isOutlier(cv::Mat vec1, cv::Mat vec2, cv::Mat cov);
    std::string printMeans();
    std::string printCovs();
    
    std::string _modelOut;
    std::string _modelIn;
    unsigned long long _aggregationWindow;
    unsigned long long _lookbackWindow;
    unsigned long long _numWindows;
    unsigned long long _numComponents;
    float _outlierCut;
    bool _reuseModel;
    bool _trainingPending;

    vector<reading_t> _buffer;
    cv::Ptr<cv::ml::EM> _gmm;
    cv::Mat _trainingSet;
    cv::Mat _tempSet;
    cv::Mat _currentfVector;

    // Misc buffers
    int64_t _mean;
};


#endif //PROJECT_CLUSTERINGOPERATOR_H
