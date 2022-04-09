//================================================================================
// Name        : ClassifierOperator.cpp
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

#include "ClassifierOperator.h"

ClassifierOperator::ClassifierOperator(const std::string& name) : OperatorTemplate(name), RegressorOperator(name) {
    _targetDistance = 0;
    _includeTarget = false;
}

ClassifierOperator::ClassifierOperator(const ClassifierOperator& other) : OperatorTemplate(other), RegressorOperator(other) {
    _includeTarget = false;
}

ClassifierOperator::~ClassifierOperator() {}

void ClassifierOperator::printConfig(LOG_LEVEL ll) {
    LOG_VAR(ll) << "            Window:          " << _aggregationWindow;
    LOG_VAR(ll) << "            Target Distance: " << _targetDistance;
    LOG_VAR(ll) << "            Training Sample: " << _trainingSamples;
    LOG_VAR(ll) << "            Input Path:      " << (_modelIn!="" ? _modelIn : std::string("none"));
    LOG_VAR(ll) << "            Output Path:     " << (_modelOut!="" ? _modelOut : std::string("none"));
    LOG_VAR(ll) << "            Importances:     " << (_importances ? "enabled" : "disabled");
    LOG_VAR(ll) << "            Raw Mode:        " << (getRawMode() ? "enabled" : "disabled");
    OperatorTemplate<RegressorSensorBase>::printConfig(ll);
}

void ClassifierOperator::compute(U_Ptr unit) {
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
            // Using an int instead of a float for the responses makes OpenCV interpret the variable as categorical
            _currentClass = (int) _currentTarget;
            _responseSet->push_back(_currentClass);
        }
        if ((uint64_t)_trainingSet->size().height >= _trainingSamples + _targetDistance)
            trainRandomForest(true);
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
