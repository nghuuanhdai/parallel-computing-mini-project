//================================================================================
// Name        : ClusteringOperator.cpp
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

#include "ClusteringOperator.h"

ClusteringOperator::ClusteringOperator(const std::string& name) : OperatorTemplate(name) {
    _modelIn = "";
    _modelOut = "";
    _aggregationWindow = 0;
    _lookbackWindow = 0;
    _numWindows = 0;
    _numComponents = 3;
    _outlierCut = 2.0f;
    _reuseModel = false;
    _trainingPending = true;
    _trainingSet = cv::Mat();
    _tempSet = cv::Mat();
    _currentfVector = cv::Mat();
}

ClusteringOperator::ClusteringOperator(const ClusteringOperator& other) : OperatorTemplate(other) {
    _modelIn = other._modelIn;
    _modelOut = "";
    _aggregationWindow = other._aggregationWindow;
    _lookbackWindow = other._lookbackWindow;
    _numWindows = other._numWindows;
    _numComponents = other._numComponents;
    _outlierCut = other._outlierCut;
    _reuseModel = other._reuseModel;
    _trainingPending = true;
    _trainingSet = cv::Mat();
    _tempSet = cv::Mat();
    _currentfVector = cv::Mat();
}

ClusteringOperator::~ClusteringOperator() {
    _gmm.release();
}

restResponse_t ClusteringOperator::REST(const string& action, const unordered_map<string, string>& queries) {
    restResponse_t resp;
    if(action=="train") {
        resp.response = "Re-training triggered for gaussian mixture model " + this->_name + "!\n";
        this->_trainingPending = true;
    } else if(action=="means") {
        resp.response = printMeans();
    } else if(action=="covs") {
        resp.response = printCovs();
    } else
        throw invalid_argument("Unknown plugin action " + action + " requested!");
    return resp;
}

void ClusteringOperator::execOnInit() {
    if(_interval==0 || _lookbackWindow==0 || _lookbackWindow <= _aggregationWindow)
        _numWindows = 0;
    else
        _numWindows = (_lookbackWindow - _aggregationWindow) / ((uint64_t)_interval * 1000000);
    
    bool useDefault=true;
    if(_modelIn!="") {
        try {
            _gmm = cv::ml::EM::load(_modelIn);
            if(!_gmm->isTrained() || _units.empty() || _units[0]->getSubUnits().empty() || 
            _units[0]->getSubUnits()[0]->getInputs().size()!=(uint64_t)_gmm->getMeans().size().width) 
                LOG(error) << "Operator " + _name + ": incompatible model, falling back to default!";
            else {
                _trainingPending = false;
                useDefault = false;
            }
        } catch(const std::exception& e) {
            LOG(error) << "Operator " + _name + ": cannot load model from file, falling back to default!"; }
    }
    if(useDefault) {
        _gmm = cv::ml::EM::create();
        _gmm->setClustersNumber(_numComponents);
    }
}

void ClusteringOperator::printConfig(LOG_LEVEL ll) {
    LOG_VAR(ll) << "            Window:          " << _aggregationWindow;
    LOG_VAR(ll) << "            Lookback window: " << _lookbackWindow;
    LOG_VAR(ll) << "            Input Path:      " << (_modelIn!="" ? _modelIn : std::string("none"));
    LOG_VAR(ll) << "            Output Path:     " << (_modelOut!="" ? _modelOut : std::string("none"));
    LOG_VAR(ll) << "            Clusters:        " << _numComponents;
    LOG_VAR(ll) << "            Outlier Cut:     " << _outlierCut;
    LOG_VAR(ll) << "            Reuse Model:     " << (_reuseModel ? "enabled" : "disabled");
    OperatorTemplate<ClusteringSensorBase>::printConfig(ll);
}

void ClusteringOperator::compute(U_Ptr unit) {
    if(_numWindows==0)
        _trainingSet = cv::Mat();
    _tempSet = cv::Mat();
    for(const auto& su : unit->getSubUnits()) {
        if(computeFeatureVector(su))
            _tempSet.push_back(_currentfVector);
    }
    if(_tempSet.empty()) {
        LOG(debug) << "Operator " + _name + ": could not build any feature vector!";
        return;
    }
    if(_trainingSet.empty())
        _trainingSet = _tempSet;
    else
        cv::vconcat(_tempSet, _trainingSet, _trainingSet);
    
    // Performing training if the conditions are met
    if ((_trainingPending || !_reuseModel) && _trainingSet.size().height/unit->getSubUnits().size() > _numWindows) {
        if(_gmm.empty())
            throw std::runtime_error("Operator " + _name + ": cannot perform training, missing model!");
        if(!_gmm->trainEM(_trainingSet))
            throw std::runtime_error("Operator " + _name + ": model training failed!");
        _trainingPending = false;
        LOG(debug) << "Operator " + _name + ": model training performed using " << _trainingSet.size().height << " points.";
        if(_modelOut!="") {
            try {
                _gmm->save(_modelOut);
            } catch(const std::exception& e) {
                LOG(error) << "Operator " + _name + ": cannot save the model to a file!"; }
        }
    }
    
    // Checking that the operator is not in any invalid state
    if(_gmm.empty() || !(_gmm->isTrained() || (_trainingPending && _streaming && _numWindows>0)))
        throw std::runtime_error("Operator " + _name + ": cannot perform prediction, the model is untrained!");

    // Performing prediction
    if(_gmm->isTrained()) {
        std::vector <std::shared_ptr<UnitTemplate<ClusteringSensorBase>>> subUnits = unit->getSubUnits();
        cv::Vec2d res;
        int64_t label;
        bool outlier;
        std::vector <cv::Mat> covs;
        _gmm->getCovs(covs);

        reading_t predict;
        predict.timestamp = getTimestamp();

        for (unsigned int idx = 0; idx < subUnits.size(); idx++) {
            res = _gmm->predict2(_trainingSet.row(idx), cv::noArray());
            label = (int64_t) res[1];
            outlier = isOutlier(_trainingSet.row(idx), _gmm->getMeans().row(label), covs[label]);
            predict.value = outlier ? OUTLIER_ID : label;
            subUnits[idx]->getOutputs()[0]->storeReading(predict);
        }
    }
    
    if(_numWindows==0)
        _trainingSet = cv::Mat();
    // Removing the oldest time window if lookback is enabled
    else if(_trainingSet.size().height/unit->getSubUnits().size() > _numWindows)
        _trainingSet = _trainingSet.rowRange(0, _numWindows * unit->getSubUnits().size());
    _tempSet = cv::Mat();
}

bool ClusteringOperator::computeFeatureVector(U_Ptr unit, uint64_t offset) {
    _currentfVector = cv::Mat(1, unit->getInputs().size(), CV_32F);
    std::vector<ClusteringSBPtr>& inputs = unit->getInputs();
    uint64_t endTs = getTimestamp() - offset;
    uint64_t startTs = endTs - _aggregationWindow;
    for(size_t idx=0; idx<inputs.size(); idx++) {
        _mean=0;
        _buffer.clear();
        if(!_queryEngine.querySensor(inputs[idx]->getName(), startTs, endTs, _buffer, false) || _buffer.empty()) {
            LOG(debug) << "Operator " + _name + ": cannot read from sensor " + inputs[idx]->getName() + "!";
            return false;
        }
        
        // Computing MEAN
        for(const auto& v : _buffer)
            _mean += v.value;
        _mean /= _buffer.size();
        
        // Casting and storing the statistical features
        _currentfVector.at<float>(idx) = (float)_mean;
    }
    return true;
}

bool ClusteringOperator::isOutlier(cv::Mat vec1, cv::Mat vec2, cv::Mat cov) {
    double dist = 0.0;
    try {
        cv::Mat iCov;
        cv::invert(cov, iCov, cv::DECOMP_SVD);
        dist = cv::Mahalanobis(vec1, vec2, iCov);
    } catch(const std::exception& e) {
        return false;
    }
    return dist > _outlierCut;
}

std::string ClusteringOperator::printMeans() {
    std::ostringstream out;
    if(_gmm.empty() || !_gmm->isTrained())
        out << "Model is uninitialized or not trained.\n";
    else {
        for(size_t idx=0; idx<(size_t)_gmm->getMeans().size().height; idx++)
            out << "Component " << idx << ":\n" << _gmm->getMeans().row(idx) << "\n";
    }
    return out.str();
}

std::string ClusteringOperator::printCovs() {
    std::ostringstream out;
    if(_gmm.empty() || !_gmm->isTrained())
        out << "Model is uninitialized or not trained.\n";
    else {
        std::vector<cv::Mat> covs;
        _gmm->getCovs(covs);
        for(size_t idx=0; idx<(size_t)covs.size(); idx++)
            out << "Component " << idx << ":\n" << covs[idx] << "\n";
    }
    return out.str();
}
