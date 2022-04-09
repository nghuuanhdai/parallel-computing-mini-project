//================================================================================
// Name        : ProcfsSensorGroup.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Procfs sensor group class.
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

#include "ProcfsSensorGroup.h"

// Overriding assignment operator so that sensors are not copy-constructed
ProcfsSensorGroup &ProcfsSensorGroup::operator=(const ProcfsSensorGroup &other) {
	SensorGroupInterface::operator=(other);
	_sensors.clear();
	_baseSensors.clear();

	_cpuSet = other._cpuSet;
	_type = other._type;
	_path = other._path;
	_htVal = other._htVal;
	_sarMax = other._sarMax;

	return *this;
}

/**
 * Class destructor.
 *
 * Disposes of the internal parser object, supposing it was allocated with "new".
 *
 */
ProcfsSensorGroup::~ProcfsSensorGroup() {
	if (_parser != NULL)
		delete _parser;
}

/**
 * Assigns a new parser and replaces the internal sensor objects
 *
 * Old sensors are deleted and cleared, therefore make sure the parser does not contain any object previously used
 * by the sensor group.
 *
 * @param parser: Pointer to a ProcfsParser object
 *
 */
void ProcfsSensorGroup::setParser(ProcfsParser *parser) {
	if(parser)
		_parser = parser;
	else
		return;
	
	std::set<std::string> sensorSet = std::set<std::string>();
	for (const auto &s_new : *parser->getSensors()) {
		sensorSet.insert(s_new->getMetric());
	}
	std::set<int> &effCpuSet = parser->getFoundCPUs();

	for (const auto &s : _sensors)
		if (sensorSet.find(s->getMetric()) == sensorSet.end())
			LOG(warning) << _groupName << "::Sensor " << s->getName() << " could not be matched to any metric!";
	for (const auto &c : _cpuSet)
		if (c != -1 && effCpuSet.find(c) == effCpuSet.end())
			LOG(warning) << _groupName << "::CPU ID " << c << " could not be found!";

	sensorSet.clear();
	effCpuSet.clear();
	_sensors.clear();
	_baseSensors.clear();
	for (auto s : *parser->getSensors())
		pushBackSensor(std::make_shared<ProcfsSensorBase>(*s));
}

/**
 * Starts up the Procfs sensor group.
 *
 * Identical to its parent method, except for the parser-related initialization logic.
 *
 */
bool ProcfsSensorGroup::execOnStart() {
	// If a parser object has not been set, the sensor group cannot be initialized
	if (this->_parser != NULL) {
		//this->_parser->init();

		// Crude debugging stuff
		//this->_parser->readSensors();
		//for( auto sensor : _sensors)
		//    LOG(debug) << sensor->getName() << " : " << sensor->getMqtt() << " -> " <<  sensor->getLatestValue().value;

		return true;
	} else
		LOG(error) << "Sensorgroup " << _groupName << " could not be started, missing parser.";

	return false;
}

/**
 * Performs sampling of metrics using the internal parser object.
 *
 * If values cannot be read from the target file, the corresponding sensors are not updated.
 *
 */
void ProcfsSensorGroup::read() {
	// Read values from the target file using the parser's _readMetrics() method
	// If an error is encountered (NULL return value) we abort here
	// Sensors are automatically updated from within the parser
	_readingVector = _parser->readSensors();
	_readingBuffer.timestamp = getTimestamp();
	if (_readingVector == NULL) {
		LOG(error) << _groupName << "::"
			   << "Could not read values from " << _type << "!";
		return;
	} else {
		for (unsigned int i = 0; i < _sensors.size(); i++) {
			_readingBuffer.value = _readingVector->at(i).value;
			_sensors.at(i)->storeReading(_readingBuffer);
		}
	}
#ifdef DEBUG
	for (unsigned int i = 0; i < _parser->getNumMetrics(); i++)
		LOG(debug) << _groupName << "::" << _sensors[i]->getName() << ": \"" << _sensors[i]->getLatestValue().value << "\"";
#endif
}

void ProcfsSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "Type:         " << _type;
	LOG_VAR(ll) << leading << "Path:         " << _path;
	LOG_VAR(ll) << leading << "HTVal:        " << _htVal;
	LOG_VAR(ll) << leading << "SarMax:       " << _sarMax;
	LOG_VAR(ll) << leading << "Parser:       " << (_parser ? "true" : "false");
}
