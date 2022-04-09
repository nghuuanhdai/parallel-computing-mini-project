//================================================================================
// Name        : CSOperator.cpp
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

#include "CSOperator.h"

CSOperator::CSOperator(const std::string& name) : OperatorTemplate(name) {
    _modelIn = "";
    _modelOut = "";
    _aggregationWindow = 0;
    _trainingSamples = 3600;
    _numBlocks = 20;
    _scalingFactor = 1000000;
    _reuseModel = true;
    _trainingPending = true;
    _trainingReady = -1;
}

CSOperator::CSOperator(const CSOperator& other) : OperatorTemplate(other) {
    _modelIn = other._modelIn;
    _modelOut = "";
    _aggregationWindow = other._aggregationWindow;
    _trainingSamples = other._trainingSamples;
    _numBlocks = other._numBlocks;
    _scalingFactor = other._scalingFactor;
    _reuseModel = other._reuseModel;
    _trainingPending = true;
    _trainingReady = -1;
}

CSOperator::~CSOperator() {}

restResponse_t CSOperator::REST(const string& action, const unordered_map<string, string>& queries) {
    restResponse_t resp;
    if(action=="train") {
        resp.response = "Re-training triggered for CS Signatures operator " + this->_name + "!\n";
        this->_trainingPending = true;
        this->_trainingReady = -1;
    } else
        throw invalid_argument("Unknown plugin action " + action + " requested!");
    return resp;
}

void CSOperator::printConfig(LOG_LEVEL ll) {
    LOG_VAR(ll) << "            Window:          " << _aggregationWindow;
    LOG_VAR(ll) << "            Input Path:      " << (_modelIn!="" ? _modelIn : std::string("none"));
    LOG_VAR(ll) << "            Output Path:     " << (_modelOut!="" ? _modelOut : std::string("none"));
    LOG_VAR(ll) << "            Blocks:          " << _numBlocks;
    LOG_VAR(ll) << "            Scaling factor:  " << _scalingFactor;
    LOG_VAR(ll) << "            Training Sample: " << _trainingSamples;
    LOG_VAR(ll) << "            Reuse Model:     " << (_reuseModel ? "enabled" : "disabled");
    OperatorTemplate<CSSensorBase>::printConfig(ll);
}

void CSOperator::execOnInit() {
    bool useDefault=true;
    // Establishing the training unit and the appropriate number of signature blocks
    if(_streaming && !_units.empty()) {
        _trainingUnit = _units[0]->getName();
        _actualBlocks = _units[0]->getInputs().size() < _numBlocks ? _units[0]->getInputs().size() : _numBlocks;
        if(_actualBlocks!=_numBlocks)
            LOG(warning) << "Operator " << _name << ": cannot enforce " << _numBlocks << " blocks, using " << _actualBlocks << " instead.";
    } else {
        _actualBlocks = _numBlocks;
    }
    
    if(_modelIn!="") {
        try {
            if(!readFromFile(_modelIn))
                LOG(error) << "Operator " + _name + ": incompatible CS data, falling back to default!";
            else {
                _trainingPending = false;
                _trainingReady = -1;
                useDefault = false;
            }
        } catch(const std::exception& e) {
            LOG(error) << "Operator " + _name + ": cannot load CS data from file, falling back to default!"; }
    }
    if(useDefault) {
        _trainingPending = true;
        _trainingReady = -1;
        _max.clear();
        _min.clear();
        _permVector.clear();
    }
    _trainingData.clear();
}

void CSOperator::compute(U_Ptr unit) {
    uint64_t nowTs = getTimestamp();
    
    // Training-related tasks
    if(_trainingPending && _streaming && _trainingUnit==unit->getName()) {
        // Fetching sensor data
        if(_trainingData.empty())
            _trainingData.resize(unit->getInputs().size());
        for(size_t idx=0; idx<unit->getInputs().size(); idx++)
            accumulateData(_trainingData, unit->getInputs()[idx], idx, nowTs);
        // Performing training once enough samples are obtained
        if(!_trainingData.empty() && _trainingReady!=-1) {
            if(!checkTrainingSet(_trainingData)) {
                LOG(error) << "Operator " + _name + ": collected training set does not appear to be valid!";
                _trainingData.clear();
                _trainingPending = true;
                _trainingReady = -1;
            } else {
                computeMinMax(_trainingData);
                computePermutation(_trainingData);
                _trainingData.clear();
                _trainingPending = false;
                _trainingReady = -1;
                LOG(info) << "Operator " + _name + ": CS training performed.";
                if (_modelOut != "" && !dumpToFile(_modelOut))
                    LOG(error) << "Operator " + _name + ": cannot save CS data to a file!";
            }
        }
    }
    
    // If the operator is in an invalid state
    if(_permVector.empty() && !(_trainingPending && _streaming)) {
        throw std::runtime_error("Operator " + _name + ": cannot compute signatures, no CS data available!");
    // If an unit has an unexpected number of input sensors
    } else if(!_permVector.empty() && _permVector.size()!=unit->getInputs().size()) {
        throw std::runtime_error("Operator " + _name + ": unit " + unit->getName() + " has an anomalous number of inputs!");
    }
        
    if(!_permVector.empty()) {
        computeSignature(unit, nowTs);
    }
}

// -------------------------------------- INPUT / OUTPUT --------------------------------------

bool CSOperator::dumpToFile(std::string &path) {
    boost::property_tree::ptree root, blocks;
    std::ostringstream data;
    
    if(_trainingPending || _permVector.empty())
        return false;
    
    // Saving CS data in terms of permutation index, minimum and maximum for each input sensor
    for(size_t idx=0; idx<_permVector.size(); idx++) {
        boost::property_tree::ptree group;
        group.push_back(boost::property_tree::ptree::value_type("idx", boost::property_tree::ptree(std::to_string(_permVector[idx]))));
        group.push_back(boost::property_tree::ptree::value_type("min", boost::property_tree::ptree(std::to_string(_min[_permVector[idx]]))));
        group.push_back(boost::property_tree::ptree::value_type("max", boost::property_tree::ptree(std::to_string(_max[_permVector[idx]]))));
        blocks.add_child(std::to_string(idx), group);
    }
    root.add_child(std::to_string(_permVector.size()), blocks);
    
    try {
        std::ofstream outFile(path);
        boost::property_tree::write_json(outFile, root, true);
        outFile.close();
    } catch(const std::exception &e) { return false; }
    return true;
}

bool CSOperator::readFromFile(std::string &path) {
    boost::property_tree::iptree config;
    try {
        boost::property_tree::read_json(path, config);
    } catch(const std::exception &e) { return false; }
    
    // The root JSON node encodes the number of sensors with which the model was trained
    if(config.begin() == config.end() || stoull(config.begin()->first) < _actualBlocks)
        return false;
    
    uint64_t numSensors = stoull(config.begin()->first);
    std::vector<size_t>  newPermVector(numSensors);
    std::vector<int64_t> newMin(numSensors);
    std::vector<int64_t> newMax(numSensors);
    
    BOOST_FOREACH(boost::property_tree::iptree::value_type &val, config.begin()->second) {
        size_t blockID = std::stoull(val.first);
        boost::property_tree::iptree &blk = val.second;
        if(blk.find("idx")==blk.not_found() || blk.find("min")==blk.not_found() || blk.find("max")==blk.not_found())
            return false;
        if(blockID>=numSensors)
            return false;

        size_t tempIdx = 0;
        int64_t tempMin = 0, tempMax = 0;
        BOOST_FOREACH(boost::property_tree::iptree::value_type &val2, blk) {
            if (boost::iequals(val2.first, "idx")) {
                tempIdx = std::stoull(val2.second.data());
            } else if (boost::iequals(val2.first, "min")) {
                tempMin = std::stoll(val2.second.data());
            } else if (boost::iequals(val2.first, "max")) {
                tempMax = std::stoll(val2.second.data());
            }
        }
        
        newPermVector[blockID] = tempIdx;
        newMin[tempIdx] = tempMin;
        newMax[tempIdx] = tempMax;
        
    }
    
    // Replacing the operator's CS data
    _permVector = newPermVector;
    _min = newMin;
    _max = newMax;
    return true;
}

// -------------------------------------- MODEL TRAINING --------------------------------------

// Accumulates sensor data in-memory for later training
void CSOperator::accumulateData(std::vector<std::vector<reading_t>>& v, CSSBPtr s, size_t idx, uint64_t nowTs) {
    // We query all new data for the sensor since the last one - we want a clean time series 
    uint64_t endTs = nowTs;
    uint64_t startTs = v[idx].empty() ? endTs - _aggregationWindow : v[idx].back().timestamp+100000;
    _buffer.clear();
    // This query might possibly fail very often, depending on the batching of sensors
    if(!_queryEngine.querySensor(s->getName(), startTs, endTs, _buffer, false))
        return;
    // We add the queried values only if they are actually "new"
    if(!_buffer.empty() && (v[idx].empty() || _buffer[0].timestamp>v[idx].back().timestamp)) {
        v[idx].insert(v[idx].end(), _buffer.begin(), _buffer.end());
        // Triggering training if right amount of sensor readings is reached
        if(v[idx].size() >= _trainingSamples)
            _trainingReady = idx;
    }
}

// Applies the sorting stage of the CS method and finds a permutation vector
void CSOperator::computePermutation(std::vector<std::vector<reading_t>>& v) {
    // Each column of the matrix will be an interpolated sensor
    cv::Mat sensorMatrix = cv::Mat(_trainingSamples, v.size(), CV_64F);
    // Evaluation parameters post-interpolation
    // Beware of the accuracy loss: casting timestamps to doubles should result in a loss only
    // at the level of microseconds, but if there are issues, then check this
    double startEval=(double)v[_trainingReady].front().timestamp;
    double stepEval=(double)(v[_trainingReady].back().timestamp - v[_trainingReady].front().timestamp) / (double)_trainingSamples;
    double startInterp, stepInterp;
    for(size_t idx=0; idx<v.size(); idx++) {
        std::vector<reading_t>& vals = v[idx];
        startInterp = startEval - (double)vals.front().timestamp;
        stepInterp = (double)(vals.back().timestamp - vals.front().timestamp) / (double)vals.size();
        // Copying element by element into a temporary vector - ugly and slow
        std::vector<double> sValues(vals.size());
        for(size_t idx2=0; idx2<vals.size(); idx2++)
            sValues[idx2] = (double)vals[idx2].value;
        // Spline interpolation
        boost::math::cubic_b_spline<double> spline(sValues.begin(), sValues.end(), startInterp, stepInterp);
        // Evaluating in the interpolated points and storing in the matrix
        for(size_t idx2=0; idx2<_trainingSamples; idx2++)
            sensorMatrix.at<double>(idx2, idx) = spline(stepEval*idx2);
        sValues.clear();
    }
    
    // Calculating covariance matrix
    cv::Mat covMatrix, meanMatrix;
    cv::calcCovarMatrix(sensorMatrix, covMatrix, meanMatrix, cv::COVAR_ROWS + cv::COVAR_SCALE + cv::COVAR_NORMAL, CV_64F);
    sensorMatrix.release();
    meanMatrix.release();
    // Transforming the matrix
    convertToCorrelation(covMatrix);
    
    // Initial set of available sensors
    std::set<size_t> availSet;
    for(size_t idx=0; idx<v.size(); idx++)
        availSet.insert(idx);

    // Correlation-based sorting
    _permVector.clear();
    double corrMax = -1000.0;
    double corrCoef = 0.0;
    size_t corrIdx = 0;

    for(size_t idx=0; idx<v.size(); idx++) {
        if (covMatrix.at<double>(idx, idx) > corrMax) {
            corrMax = covMatrix.at<double>(idx, idx);
            corrIdx = idx;
        }
    }
    
    _permVector.push_back(corrIdx);
    availSet.erase(corrIdx);
    
    while(!availSet.empty()) {
        corrMax = -1000;
        corrIdx = 0;
        for(const auto& avId : availSet) {
            corrCoef = covMatrix.at<double>(avId, avId) * covMatrix.at<double>(_permVector.back(), avId);
            if(corrCoef > corrMax) {
                corrMax = corrCoef;
                corrIdx = avId;
            }
        }
        _permVector.push_back(corrIdx);
        availSet.erase(corrIdx);
    }
    covMatrix.release();
}

// Computes minimum and maximum for each separate sensor
void CSOperator::computeMinMax(std::vector<std::vector<reading_t>>& v) {
    _min.resize(v.size());
    _max.resize(v.size());
    
    int64_t max, min;
    for(size_t idx=0; idx<v.size(); idx++) {
        max = LLONG_MIN;
        min = LLONG_MAX;
        if (v[idx].size() > 0) {
            for (const auto &s : v[idx]) {
                if (s.value > max)
                    max = s.value;
                if (s.value < min)
                    min = s.value;
            }
        } else {
            max = 0;
            min = 0;
        }
        _min[idx] = min;
        _max[idx] = max;
    }
}

// Converts a covariance matrix to a correlation one and additionally stores the "total" correlation 
// of each variable in the diagonal of the matrix
void CSOperator::convertToCorrelation(cv::Mat &m) {
    // Computing Pearson correlations
    for(size_t i=0; i<(size_t)m.size().height; i++) {
        for(size_t j=0; j<(size_t)m.size().width; j++) {
            if(i!=j)
                m.at<double>(i,j) = m.at<double>(i,j) / (sqrt(m.at<double>(i,i))*sqrt(m.at<double>(j,j)) + 0.00001);
        }
    }
    // Getting global correlations
    for(size_t i=0; i<(size_t)m.size().height; i++) {
        m.at<double>(i,i) = 0;
        for(size_t j=0; j<(size_t)m.size().width; j++) {
            if(i!=j) {
                m.at<double>(i,i) += m.at<double>(i,j);
            }
        }
        m.at<double>(i,i) /= m.size().width - 1;
    }
}

// Checks that the training set is actually valid
bool CSOperator::checkTrainingSet(std::vector<std::vector<reading_t>>& v) {
    if(v.empty())
        return false;
    bool foundValid=false;
    for(const auto& s : v) {
        if(s.size() < 100) {
            return false;
        } else if(s.size() >= _trainingSamples) {
            foundValid = true;
        }
    }
    return foundValid;
}

// -------------------------------------- SIGNATURE COMPUTATION --------------------------------------

// Actual signature computation
void CSOperator::computeSignature(U_Ptr unit, uint64_t nowTs) {
    uint64_t endTs = nowTs;
    uint64_t startTs = endTs - _aggregationWindow;
    // Buffers need to have the same number of elements as the input sensors, and uniform across units
    if(_avgBuffer.size()!=_permVector.size() || _derBuffer.size()!=_permVector.size()) {
        _avgBuffer.resize(_permVector.size());
        _derBuffer.resize(_permVector.size());
    }

    // Querying sensors, calculating averages and first-order derivatives
    for(size_t idx=0; idx<unit->getInputs().size(); idx++) {
        _buffer.clear();
        if(!_queryEngine.querySensor(unit->getInputs()[idx]->getName(), startTs, endTs, _buffer, false)) {
            LOG(debug) << "Operator " + _name + ": cannot read from sensor " << unit->getInputs()[idx]->getName() << "!";
            return;
        }
        normalize(_buffer, idx);
        _avgBuffer[idx] = getAvg(_buffer);
        _derBuffer[idx] = getDer(_buffer);
    }

    // Computing blocks and storing result into output sensors
    reading_t val;
    val.timestamp = nowTs;
    _blockLen = (float)unit->getInputs().size() / (float)_actualBlocks;
    for(auto &s : unit->getOutputs()) {
        if(s->getBlockID()<_actualBlocks) {
            _bBegin = (size_t)floor(_blockLen*s->getBlockID());
            _bEnd = (size_t)ceil(_blockLen*(s->getBlockID()+1));
            val.value = 0;

            if(!s->getImag()) {
                for (size_t idx = _bBegin; idx < _bEnd; idx++)
                    val.value += _avgBuffer[_permVector[idx]];
            } else {
                for (size_t idx = _bBegin; idx < _bEnd; idx++)
                    val.value += _derBuffer[_permVector[idx]];
            }
            val.value /= ((int64_t)_bEnd - (int64_t)_bBegin);
            s->storeReading(val);
        }
    }
}

// Normalizes sensor data
void CSOperator::normalize(std::vector<reading_t> &v, size_t idx) {
    double denom = _max[idx]!=_min[idx] ? (double)_scalingFactor / (double)(_max[idx] - _min[idx]) : (double)_scalingFactor;
    for(size_t idx2=0; idx2<v.size(); idx2++) {
        if(v[idx2].value > _max[idx])
            v[idx2].value = _max[idx];
        else if(v[idx2].value < _min[idx])
            v[idx2].value = _min[idx];
        v[idx2].value = (int64_t)((double)(v[idx2].value - _min[idx]) * denom);
    }
}

// Computes average sensor values
int64_t CSOperator::getAvg(std::vector<reading_t> &v) {
    int64_t avg = 0;
    for(size_t idx=0; idx<v.size(); idx++) {
        avg += v[idx].value;
    }
    avg = v.size()>0 ? avg/v.size() : 0;
    return avg;
}

// Computes average first-order derivatives
int64_t CSOperator::getDer(std::vector<reading_t> &v) {
    int64_t der = 0;
    for(size_t idx=1; idx<v.size(); idx++) {
        der += (int64_t)v[idx].value - (int64_t)v[idx-1].value;
    }
    der = v.size()>1 ? der/((int64_t)v.size()-1) : 0;
    return der;
}
