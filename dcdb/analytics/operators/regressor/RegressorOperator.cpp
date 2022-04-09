//================================================================================
// Name        : RegressorOperator.cpp
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

#include "RegressorOperator.h"

RegressorOperator::RegressorOperator(const std::string& name) : OperatorTemplate(name) {
    _modelIn = "";
    _modelOut = "";
    _aggregationWindow = 0;
    _targetDistance = 1;
    _smoothResponses = false;
    _numFeatures = REG_NUMFEATURES;
    _numInputs = 0;
    _trainingSamples = 256;
    _trainingPending = true;
    _importances = false;
    _includeTarget = true;
    _trainingSet = nullptr;
    _responseSet = nullptr;
    _currentfVector = nullptr;
}

RegressorOperator::RegressorOperator(const RegressorOperator& other) : OperatorTemplate(other) {
    _modelIn = other._modelIn;
    _modelOut = "";
    _aggregationWindow = other._aggregationWindow;
    _targetDistance = other._targetDistance;
    _smoothResponses = other._smoothResponses;
    _numFeatures = other._numFeatures;
    _numInputs = 0;
    _trainingSamples = other._trainingSamples;
    _importances = other._importances;
    _includeTarget = true;
    _trainingPending = true;
    _trainingSet = nullptr;
    _responseSet = nullptr;
    _currentfVector = nullptr;
}

RegressorOperator::~RegressorOperator() {
    if(_trainingSet)
        delete _trainingSet;
    if(_responseSet)
        delete _responseSet;
    if(_currentfVector)
        delete _currentfVector;
    _rForest.release();
    _currentfVector = nullptr;
    _trainingSet = nullptr;
    _responseSet = nullptr;
}

restResponse_t RegressorOperator::REST(const string& action, const unordered_map<string, string>& queries) {
    restResponse_t resp;
    if(action=="train") {
        resp.response = "Re-training triggered for model " + this->_name + "!\n";
        this->_trainingPending = true;
    } else if(action=="importances") {
        resp.response = getImportances();
    } else
        throw invalid_argument("Unknown plugin action " + action + " requested!");
    return resp;
}

void RegressorOperator::execOnInit() {
    // Is a target set at all?
    bool targetSet = false;
    for(const auto& s: _units[0]->getInputs()) {
        targetSet = targetSet || s->getTrainingTarget();
    }
    _numInputs = (!targetSet || _includeTarget) ? _units[0]->getInputs().size() : (_units[0]->getInputs().size() - 1);
    bool useDefault=true;
    if(_modelIn!="") {
        try {
            _rForest = cv::ml::RTrees::load(_modelIn);
            if(!_rForest->isTrained() || _units.empty() || _numInputs*_numFeatures!=(uint64_t)_rForest->getVarCount()) 
                LOG(error) << "Operator " + _name + ": incompatible model, falling back to default!";
            else {
                _trainingPending = false;
                useDefault = false;
            }
        } catch(const std::exception& e) {
            LOG(error) << "Operator " + _name + ": cannot load model from file, falling back to default!"; }
    }
    if(useDefault) {
        _rForest = cv::ml::RTrees::create();
        _rForest->setCalculateVarImportance(_importances);
    }
}

void RegressorOperator::printConfig(LOG_LEVEL ll) {
    LOG_VAR(ll) << "            Window:          " << _aggregationWindow;
    LOG_VAR(ll) << "            Target Distance: " << _targetDistance;
    LOG_VAR(ll) << "            Smooth Response: " << (_smoothResponses ? "enabled" : "disabled");
    LOG_VAR(ll) << "            Training Sample: " << _trainingSamples;
    LOG_VAR(ll) << "            Input Path:      " << (_modelIn!="" ? _modelIn : std::string("none"));
    LOG_VAR(ll) << "            Output Path:     " << (_modelOut!="" ? _modelOut : std::string("none"));
    LOG_VAR(ll) << "            Importances:     " << (_importances ? "enabled" : "disabled");
    LOG_VAR(ll) << "            Raw Mode:        " << (getRawMode() ? "enabled" : "disabled");
    OperatorTemplate<RegressorSensorBase>::printConfig(ll);
}

void RegressorOperator::compute(U_Ptr unit) {
    // Not much to do without a valid feature vector
    if(!computeFeatureVector(unit))
        return;
    if (_trainingPending && _streaming) {
        if (!_trainingSet)
            _trainingSet = new cv::Mat();
        if (!_responseSet)
            _responseSet = new cv::Mat();
        if(_targetFound) {
            _trainingSet->push_back(*_currentfVector);
            _responseSet->push_back(_currentTarget);
        }
        if ((uint64_t)_trainingSet->size().height >= _trainingSamples + _targetDistance)
            trainRandomForest(false);
    }
    if(_rForest.empty() || !(_rForest->isTrained() || (_trainingPending && _streaming))) 
        throw std::runtime_error("Operator " + _name + ": cannot perform prediction, the model is untrained!");
    if(_rForest->isTrained()) {
        reading_t predict;
        predict.value = (int64_t) _rForest->predict(*_currentfVector);
        predict.timestamp = getTimestamp();
        unit->getOutputs()[0]->storeReading(predict);
    }
}

void RegressorOperator::trainRandomForest(bool categorical) {
    if(!_trainingSet || _rForest.empty())
        throw std::runtime_error("Operator " + _name + ": cannot perform training, missing model!");
    if((uint64_t)_responseSet->size().height <= _targetDistance)
        throw std::runtime_error("Operator " + _name + ": cannot perform training, insufficient data!");
    // Shifting the training and response sets so as to obtain the desired prediction distance
    if(!categorical && _smoothResponses && _targetDistance>0) {
        smoothResponsesArray();
        *_responseSet = _responseSet->rowRange(0, _responseSet->size().height-_targetDistance);
    } else {
        *_responseSet = _responseSet->rowRange(_targetDistance, _responseSet->size().height);
    }
    *_trainingSet = _trainingSet->rowRange(0, _trainingSet->size().height-_targetDistance);
    shuffleTrainingSet();
    
    cv::Mat varType = cv::Mat(_trainingSet->size().width + 1, 1, CV_8U);
    varType.setTo(cv::Scalar(cv::ml::VAR_NUMERICAL));
    varType.at<unsigned char>(_trainingSet->size().width, 0) = categorical ? cv::ml::VAR_CATEGORICAL : cv::ml::VAR_NUMERICAL;
    cv::Ptr<cv::ml::TrainData> td = cv::ml::TrainData::create(*_trainingSet, cv::ml::ROW_SAMPLE, *_responseSet, cv::noArray(), cv::noArray(), cv::noArray(), varType);
    
    if(!_rForest->train(td))
        throw std::runtime_error("Operator " + _name + ": model training failed!");
    
    td.release();
    LOG(info) << "Operator " << _name << ": model training performed using " <<  _trainingSet->size().height << " samples and " << _trainingSet->size().width << " features.";
    LOG(info) << getImportances();
    delete _trainingSet;
    _trainingSet = nullptr;
    delete _responseSet;
    _responseSet = nullptr;
    _trainingPending = false;
    if(_modelOut!="") {
        try {
            _rForest->save(_modelOut);
        } catch(const std::exception& e) {
            LOG(error) << "Operator " + _name + ": cannot save the model to a file!"; }
    }
}

void RegressorOperator::smoothResponsesArray() {
    for(size_t midx=0; midx < (_responseSet->size().height - _targetDistance); midx++) {
        float meanBuf = 0;
        for(size_t nidx=0; nidx <= _targetDistance; nidx++) {
            meanBuf += _responseSet->at<float>(midx + nidx);
        }
        _responseSet->at<float>(midx) = meanBuf / (_targetDistance + 1);
    }
}

void RegressorOperator::shuffleTrainingSet() {
    if(!_trainingSet || !_responseSet)
        return;
    size_t idx1, idx2, swaps = _trainingSet->size().height / 2;
    std::random_device dev;
    std::mt19937 rng(dev());
    rng.seed((unsigned)getTimestamp());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, _trainingSet->size().height-1);
    cv::Mat swapBuf;
    for(uint64_t nS=0; nS<swaps; nS++) {
        idx1 = dist(rng);
        idx2 = dist(rng);
        
        _trainingSet->row(idx1).copyTo(swapBuf);
        _trainingSet->row(idx2).copyTo(_trainingSet->row(idx1));
        swapBuf.copyTo(_trainingSet->row(idx2));

        _responseSet->row(idx1).copyTo(swapBuf);
        _responseSet->row(idx2).copyTo(_responseSet->row(idx1));
        swapBuf.copyTo(_responseSet->row(idx2));
        //LOG(debug) << "Swapping " << idx1 << " and " << idx2;
    }
}

bool RegressorOperator::computeFeatureVector(U_Ptr unit) {
    if(!_currentfVector)
        _currentfVector = new cv::Mat(1, _numInputs*_numFeatures, CV_32F);
    _targetFound = false;
    int64_t val;
    size_t qId, qMod, idx=0, fIdx=0;
    uint64_t endTs = getTimestamp();
    uint64_t startTs = endTs - _aggregationWindow;
    std::vector<RegressorSBPtr>& inputs = unit->getInputs();
    for(idx=0; idx<inputs.size(); idx++) {
        _mean=0; _std=0; _diffsum=0; _qtl25=0; _qtl75=0; _latest=0;
        _buffer.clear();
        if(!_queryEngine.querySensor(inputs[idx]->getName(), startTs, endTs, _buffer, false) || _buffer.empty()) {
            LOG(debug) << "Operator " + _name + ": cannot read from sensor " + inputs[idx]->getName() + "!";
            return false;
        }
        _latest = _buffer.back().value;
        if (inputs[idx]->getTrainingTarget()) {
            _currentTarget = (float) _latest;
            _targetFound = true;
        }

        if(!inputs[idx]->getTrainingTarget() || _includeTarget) {
            // Computing MEAN and SUM OF DIFFERENCES
            val = _buffer.front().value;
            for (const auto &v : _buffer) {
                _mean += v.value;
                _diffsum += v.value - val;
                val = v.value;
            }
            // Casting and storing the statistical features
            _mean /= _buffer.size();
            _currentfVector->at<float>(fIdx++) = (float) _mean;

            // Computing additional features only if we are not in "raw" mode
            if(_numFeatures == REG_NUMFEATURES) {
                // Computing STD
                for (const auto &v : _buffer) {
                    val = v.value - _mean;
                    _std += val * val;
                }
                _std = sqrt(_std / _buffer.size());

                // I know, sorting is costly; here, we assume that the aggregation window of sensor data is going to be relatively
                // small, in which case the O(log(N)) complexity of the std::sort implementation converges to O(N)
                std::sort(_buffer.begin(), _buffer.end(), [](const reading_t &lhs, const reading_t &rhs) { return lhs.value < rhs.value; });
                // Computing 25th PERCENTILE
                qId = ((_buffer.size() - 1) * 25) / 100;
                qMod = ((_buffer.size() - 1) * 25) % 100;
                _qtl25 = (qMod == 0 || qId == _buffer.size() - 1) ? _buffer[qId].value : (_buffer[qId].value + _buffer[qId + 1].value) / 2;
                // Computing 75th PERCENTILE
                qId = ((_buffer.size() - 1) * 75) / 100;
                qMod = ((_buffer.size() - 1) * 75) % 100;
                _qtl75 = (qMod == 0 || qId == _buffer.size() - 1) ? _buffer[qId].value : (_buffer[qId].value + _buffer[qId + 1].value) / 2;

                _currentfVector->at<float>(fIdx++) = (float) _std;
                _currentfVector->at<float>(fIdx++) = (float) _diffsum;
                _currentfVector->at<float>(fIdx++) = (float) _qtl25;
                _currentfVector->at<float>(fIdx++) = (float) _qtl75;
                _currentfVector->at<float>(fIdx++) = (float) _latest;
            }
        }
    }
    //LOG(error) << "Target: " << _currentTarget;
    //LOG(error) << "Vector: ";
    //for(idx=0; idx<_currentfVector->size().width;idx++)
    //    LOG(error) << _currentfVector->at<float>(idx);
    return true;
}

std::string RegressorOperator::getImportances() {
    if(!_importances || _rForest.empty() || !_rForest->getCalculateVarImportance())
        return "Operator " + _name + ": feature importances are not available.";
    std::vector<ImportancePair> impLabels;
    cv::Mat_<float> impValues; 
    _rForest->getVarImportance().convertTo(impValues, CV_32F);
    if(impValues.empty() || _units.empty() || impValues.total()!=_numFeatures*_numInputs)
        return "Operator " + _name + ": error when computing feature importances.";

    size_t sidx=0;
    for(size_t idx=0; idx<impValues.total(); idx++) {
        // Iterating over the vector of sensors in _numFeatures blocks
        // For classifier models, the target sensor is not included in the feature vectors: in these cases,
        // we simply skip that sensor in the original array
        if(idx > 0 && idx%_numFeatures == 0) {
            sidx += (_units[0]->getInputs().at(sidx+1)->getTrainingTarget() && !_includeTarget) ? 2 : 1;
        }
        
        if(sidx >= _units[0]->getInputs().size()) {
            return "Operator " + _name + ": Mismatch between the model and input sizes.";
        }

        ImportancePair pair;
        pair.name = _units[0]->getInputs().at(sidx)->getName();
        pair.value = impValues.at<float>(idx);
        switch(idx%_numFeatures) {
            case 0:
                pair.name += " - mean";
                break;
            case 1:
                pair.name += " - std";
                break;
            case 2:
                pair.name += " - diffsum";
                break;
            case 3:
                pair.name += " - qtl25";
                break;
            case 4:
                pair.name += " - qtl75";
                break;
            case 5:
                pair.name += " - latest";
                break;
            default:
                break;
        }
        impLabels.push_back(pair);
    }
    
    std::sort(impLabels.begin(), impLabels.end(), [ ](const ImportancePair& lhs, const ImportancePair& rhs) { return lhs.value > rhs.value; });
    std::string outString="Operator " + _name + ": listing feature importances for unit " + _units[0]->getName() + ":\n";
    for(const auto& imp : impLabels)
        outString +=  "    " + imp.name + " - " + std::to_string(imp.value) + "\n";
    return outString;
}