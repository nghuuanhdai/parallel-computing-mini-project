//================================================================================
// Name        : ProcfsParser.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for ProcfsParser.
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

#include "ProcfsParser.h"

// Definitions for the ProcfsParser abstract class

/**
 * Class constructor.
 *
 * @param path: string containing the path to the file to be parsed
 *
 */
ProcfsParser::ProcfsParser(std::string path) {
	_path = path;
	_htVal = 0;
	_htAggr = false;
	_numMetrics = 0;
	_numInternalMetrics = 0;
	_numCPUs = 0;
	_sensors = NULL;
	_readings = NULL;
	_initialized = false;
	_metricsFile = NULL;
	_stringBuffer = NULL;
	_chars_read = 0;
	_cacheIndex = 0;
	_sarMax = 1000000;
}

/**
 * Class destructor.
 *
 */
ProcfsParser::~ProcfsParser() {
	ProcfsParser::close();
}

/**
 * Initializes the parser.
 *
 * If a file can be opened at the specified path, and metrics can be successfully parsed and read, the parser is
 * flagged as initialized. If any of this fails, the parser remains in its un-initialized state.
 *
 * @param sensorVec:    A vector containing all user-specified sensors that must be sampled. If none (or an empty
 *                      vector) is supplied, all available metrics in the file will be extracted.
 * @param cpuSet:       A set containing the CPU ids whose metrics must be extracted. If none (or an empty set) is 
 *                      supplied, metrics for all available CPUs in the system will be extracted.
 * @return              true if initialization was successful, false otherwise
 */
bool ProcfsParser::init(std::vector<ProcfsSBPtr> *sensorVec, std::set<int> *cpuSet) {
	if (_initialized)
		return true;
	// Building the auxiliary sensor map
	std::map<std::string, ProcfsSBPtr> *sensorMap = sensorVec != NULL ? new std::map<std::string, ProcfsSBPtr>() : NULL;
	if (sensorVec != NULL)
		for (auto &s : *sensorVec)
			sensorMap->insert(std::make_pair(s->getMetric(), s));

	// For the parser to be initialized, both the metrics' names and initial readings must be successfully read
	_initialized = _readNames(sensorMap, cpuSet) && _readMetrics();
	if (sensorMap != NULL) {
		sensorMap->clear();
		delete sensorMap;
	}
	return _initialized;
}

/**
 * Closes the parser.
 *
 * The open file handle is closed, all memory is released, and the parser returns to a un-initialized state. Be aware
 * that while all internal vectors are cleared, the sensor objects are not, and must be deleted manually if necessary.
 * That is because the parser is designed to be used in tandem with a sensor group, sharing the same sensor objects,
 * and therefore the latter is expected to perform all necessary cleanup upon termination.
 *
 */
void ProcfsParser::close() {
	if (!_initialized)
		return;
	if (_metricsFile != NULL)
		fclose(_metricsFile);
	_metricsFile = NULL;
	_lines.clear();
	_skipColumn.clear();
	if (_readings != NULL) {
		_readings->clear();
		delete _readings;
	}
	if (_sensors != NULL) {
		_sensors->clear();
		delete _sensors;
	}
	if (_stringBuffer != NULL)
		free(_stringBuffer);
	_stringBuffer = NULL;
	_sensors = NULL;
	_readings = NULL;
	_numMetrics = 0;
	_numCPUs = 0;
	_initialized = false;
}

/**
 * Returns the vector of parsed sensors without updating the readings.
 *
 * The parser must have been initialized earlier through the init() method.
 *
 * @return    Pointer to a vector of ProcfsSensorBase objects if the parser is initialized and no errors are encountered, NULL otherwise
 */
std::vector<ProcfsSBPtr> *ProcfsParser::getSensors() {
	if (!_initialized)
		return NULL;
	return _sensors;
}

/**
 * Returns the vector of parsed sensors and updates the readings in the process.
 *
 * The parser must have been initialized earlier through the init() method.
 *
 * @return    Pointer to a vector of ureading_t structs if the parser is initialized and no errors are encountered, NULL otherwise
 */
std::vector<ureading_t> *ProcfsParser::readSensors() {
	if (!_initialized || !_readMetrics())
		return NULL;
	return _readings;
}

// **************************************************************
// Definitions for the MeminfoParser class

/**
 * Parses the pointed file and instantiates sensors accordingly.
 *
 * This private method does not perform any check on the status of the parser, and is meant to only contain the parsing logic.
 * As vmstat and meminfo do not contain CPU-related information.
 *
 * @param sensorMap:    A map containing the list of metric names to be extracted from the parsed file.
 *                      If an empty map is supplied, all available metrics in the file will be extracted.
 * @param cpuSet:       A set containing the CPU ids whose metrics must be extracted. If none (or an empty set) is supplied,
 *                      metrics for all available CPUs in the system will be extracted.
 * @return              true if successful, false otherwise
 */
bool MeminfoParser::_readNames(std::map<std::string, ProcfsSBPtr> *sensorMap, std::set<int> *cpuSet) {
    _foundCPUs.clear();
	if (_sensors == NULL) {
		bool memUsedEnabled = sensorMap != NULL && !sensorMap->empty() && sensorMap->find(_memUsedToken) != sensorMap->end();
		_memFreeLine = -1;
		_memTotalLine = -1;
		_sensors = new std::vector<ProcfsSBPtr>();
		_lines.clear();
		char *      lineToken, *savePtr;
		ProcfsSBPtr tempSPtr;
		ProcLine    line;
		line.cpuID = -1;
		line.columns = 1;
		line.multi = false;

		// Closing and re-opening a ProcFS file at each sampling time incurs overhead, but solves issues with certain
		// files not showing updated values
		//fseek(_metricsFile, 0, SEEK_SET);
		if (_metricsFile) {
			fclose(_metricsFile);
			_metricsFile = NULL;
		}
		if ((_metricsFile = fopen(_path.c_str(), "r")) == NULL)
			return false;

		// Reading the target file line by line
		while (getline(&_stringBuffer, &_chars_read, _metricsFile) > 0) {
			savePtr = NULL;
			// strtok splits the line in delimiter-separated tokens
			lineToken = strtok_r(_stringBuffer, LINE_SEP, &savePtr);
			// Found a malformed line, aborting
			if (lineToken == NULL) {
				_sensors->clear();
				_lines.clear();
				break;
			}
			// We check if the token matches one entry in sensorMap, meaning that it must be extracted
			else if (sensorMap == NULL || sensorMap->empty() || sensorMap->find(lineToken) != sensorMap->end()) {
				if (sensorMap == NULL || sensorMap->empty()) {
					tempSPtr = std::make_shared<ProcfsSensorBase>(std::string(lineToken), std::string(lineToken));
					// If in "discovery" mode, the MQTT suffix is derived from the metric name
					tempSPtr->setMqtt(std::string(lineToken));
				} else
					tempSPtr = std::make_shared<ProcfsSensorBase>(*sensorMap->find(lineToken)->second);
				_sensors->push_back(tempSPtr);
				line.dest = _sensors->size() - 1;
				line.skip = false;
			}
			// If the token does not match any entry in sensorMap, its line is flagged as ignored
			else {
				line.dest = -1;
				line.skip = true;
			}
			_lines.push_back(line);

			// Computing the position of MemTotal and MemFree values, if necessary
			if (memUsedEnabled && strcmp(lineToken, "MemTotal") == 0)
				_memTotalLine = _lines.size() - 1;
			else if (memUsedEnabled && strcmp(lineToken, "MemFree") == 0)
				_memFreeLine = _lines.size() - 1;
		}

		// If MemUsed was requested and both MemTotal and MemFree were found, we create a new sensor
		if (_memTotalLine != -1 && _memFreeLine != -1)
			_sensors->push_back(std::make_shared<ProcfsSensorBase>(*sensorMap->find(_memUsedToken)->second));

		_numMetrics = _sensors->size();
	}
	return true;
}

/**
 * Parses the pointed file and updates sensor readings accordingly.
 *
 * This private method does not perform any check on the status of the parser, and is meant to only contain the
 * parsing logic. Once all of the metrics specified at initialization are read, the parsing stops so as to reduce
 * overhead; this means we are not able to detect if the parsed file increases in size, mismatching the original
 * mapping of metrics. This event, however, should not ever happen at runtime.
 *
 * @return    true if successful, false otherwise
 */
bool MeminfoParser::_readMetrics() {
	if (_readings == NULL)
		_readings = new std::vector<ureading_t>(_numMetrics);

	//fseek(_metricsFile, 0, SEEK_SET);
	if (_metricsFile) {
		fclose(_metricsFile);
		_metricsFile = NULL;
	}
	if ((_metricsFile = fopen(_path.c_str(), "r")) == NULL)
		return false;

	int          lineCtr = 0;
	unsigned int ctr = 0;
	char *       lineToken, *savePtr;

	_memFreeValue.value = 0;
	_memFreeValue.timestamp = 0;
	_memTotalValue.value = 0;
	_memTotalValue.timestamp = 0;

	// Reading the target file line by line
	// Any changes in the file compared to when initialization was performed cause the method to return NULL
	while (getline(&_stringBuffer, &_chars_read, _metricsFile) > 0 && ctr < _numMetrics) {
		_l = &_lines[lineCtr];
		if (!_l->skip || lineCtr == _memFreeLine || lineCtr == _memTotalLine) {
			savePtr = NULL;
			lineToken = strtok_r(_stringBuffer, LINE_SEP, &savePtr);
			// First token is discarded, represents the metric's name
			if (lineToken == NULL)
				return false;
			// Second token is the actual metric
			lineToken = strtok_r(NULL, LINE_SEP, &savePtr);
			if (lineToken == NULL)
				return false;
			try {
				_valueBuffer.value = std::stoull(lineToken);
			} catch (const std::invalid_argument &ia) { return false; } catch (const std::out_of_range &oor) {
				return false;
			}

			if (!_l->skip) {
				_readings->at(_l->dest).value = _valueBuffer.value;
				ctr++;
			}
			// Storing MemTotal and MemFree values to be used to compute MemUsed
			if (lineCtr == _memFreeLine) {
				_memFreeValue.value = _valueBuffer.value;
				_memFreeValue.timestamp = 1;
			} else if (lineCtr == _memTotalLine) {
				_memTotalValue.value = _valueBuffer.value;
				_memTotalValue.timestamp = 1;
			}
			// If the last metric left to compute is MemUsed, and both MemTotal and MemFree have been acquired,
			// we can break out of the loop early
			if (_memTotalValue.timestamp != 0 && _memFreeValue.timestamp != 0 && ctr + 1 == _numMetrics)
				break;
		}
		lineCtr++;
	}

	// Computing MemUsed as a last sensor
	if (_memTotalValue.timestamp != 0 && _memFreeValue.timestamp != 0)
		_readings->at(ctr++).value = _memTotalValue.value - _memFreeValue.value;

	// Error: the number of read metrics does not match the one at initialization, file must have changed
	if (ctr != _numMetrics)
		return false;
	return true;
}

// **************************************************************
// Definitions for the ProcstatParser class

/**
 * Parses the pointed file and instantiates sensors accordingly.
 *
 * This private method does not perform any check on the status of the parser, and is meant to only contain the parsing logic.
 * Additionally, this method computes for each metric the associated CPU core (if any), which can be inferred from the metric name iself.

 * @param sensorMap:    A map containing the list of metric names to be extracted from the parsed file.
 *                      If an empty map is supplied, all available metrics in the file will be extracted.
 * @param cpuSet:       A set containing the CPU ids whose metrics must be extracted. If none (or an empty set) is 
 *                      supplied, metrics for all available CPUs in the system will be extracted.
 * @return              true if successful, false otherwise
 */
bool ProcstatParser::_readNames(std::map<std::string, ProcfsSBPtr> *sensorMap, std::set<int> *cpuSet) {
    _foundCPUs.clear();
	if (_sensors == NULL) {
		_numMetrics = 0;
		_numInternalMetrics = 0;
		_sensors = new std::vector<ProcfsSBPtr>();
		_lines.clear();
		_skipColumn.clear();
		char *                    lineToken, *savePtr;
		short                     currCPU = -1, effCPU = -1, colCtr = 0, parsedCols = 0;
		ProcfsSBPtr               s = NULL;
		std::map<short, ProcLine> htMap;
		ProcLine                  line;

		// We detect which columns in the procstat file, corresponding to CPU-specific metrics must be skipped or
		// considered depending on the sensors in the sensormap
		_numCPUs = 0;
		for (int i = 0; i < DEFAULTMETRICS; i++)
			if (sensorMap != NULL && !sensorMap->empty() && sensorMap->find(DEFAULT_NAMES[i]) == sensorMap->end())
				_skipColumn.push_back(0);
			else if (sensorMap != NULL && !sensorMap->empty())
				_skipColumn.push_back(sensorMap->find(DEFAULT_NAMES[i])->second->isPerCPU() ? 1 : 2);
			else
				_skipColumn.push_back(1);

		//fseek(_metricsFile, 0, SEEK_SET);
		if (_metricsFile) {
			fclose(_metricsFile);
			_metricsFile = NULL;
		}
		if ((_metricsFile = fopen(_path.c_str(), "r")) == NULL)
			return false;

		// Reading the target file line by line
		while (getline(&_stringBuffer, &_chars_read, _metricsFile) > 0) {
			currCPU = -1;
			savePtr = NULL;
			// First token in the line is the metric's name
			lineToken = strtok_r(_stringBuffer, LINE_SEP, &savePtr);
			if (lineToken == NULL) {
				// Found a malformed line, aborting
				_sensors->clear();
				_skipColumn.clear();
				_lines.clear();
				_numCPUs = 0;
				break;
			}
			// We check whether the line is cpu core-related or not through a regular expression
			// If the name contains the "cpu" keyword, then it is cpu-related
			if (strncmp(lineToken, _cpu_prefix, _cpu_prefix_len) == 0) {
				// If a match to an integer number is found within the name, the metric is related to a specific core. Otherwise,
				// it is considered as a system-wide metric
				try {
					currCPU = !boost::regex_search(lineToken, _match, reg_exp_num) ? -1 : (short)std::stol(_match.str(0));
				} catch (const std::invalid_argument &ia) { return false; } catch (const std::out_of_range &oor) {
					return false;
				}
				// Computing the destination CPU ID in case hyperthreading aggregation is enabled
				effCPU = (_htAggr && currCPU >= 0) ? currCPU % _htVal : currCPU;
				colCtr = 0;
				parsedCols = 0;
				if (currCPU == -1 || cpuSet == NULL || cpuSet->empty() || cpuSet->find(currCPU) != cpuSet->end()) {
					// The number of CPU cores in the system is inferred from the maximum CPU ID found while parsing
					if (currCPU >= (int)_numCPUs)
						_numCPUs = currCPU + 1;
					// Counting the number of CPU-related lines
					std::string cpuName(lineToken);
					while ((lineToken = strtok_r(_stringBuffer, LINE_SEP, &savePtr)) != NULL && colCtr < DEFAULTMETRICS) {
						if (_skipColumn[colCtr] == 1 || (_skipColumn[colCtr] == 2 && currCPU == -1)) {
							if (currCPU == -1 || htMap.find(effCPU) == htMap.end()) {
								if (sensorMap != NULL && !sensorMap->empty())
									s = std::make_shared<ProcfsSensorBase>(*sensorMap->find(DEFAULT_NAMES[colCtr])->second);
								else {
									s = std::make_shared<ProcfsSensorBase>(DEFAULT_NAMES[colCtr]);
									s->setMqtt(DEFAULT_NAMES[colCtr]);
								}
								s->setCPUId(currCPU);
								s->setMetric(DEFAULT_NAMES[colCtr]);
								s->setMqtt(MQTTChecker::formatTopic(s->getMqtt(), currCPU));
								_sensors->push_back(s);
							}
							// This variable counts the number of metrics for each CPU that are effectively parsed from
							// the stat file Current maximum allowed is 10 metrics, any additional metrics are discarded
							parsedCols++;
							_numInternalMetrics++;
						}
						colCtr++;
					}
					if(currCPU!=-1)
					    _foundCPUs.insert(currCPU);
				}
				// The line is flagged as skippable if no columns should be read from it. This can happen in two cases:
				//  - The cpu in said line is not part of the input CPU set
				//  - No sensors for cpu-related metrics (see DEFAULT_NAMES) were specified
				line.skip = parsedCols == 0;
				line.cpuID = currCPU;
				line.columns = parsedCols;
				line.multi = true;
				line.dest = _sensors->size() - parsedCols;
			}
			// If the metric is not cpu core-related, it is simply pushed to the _metricsNames vector
			else {
				if (sensorMap != NULL && !sensorMap->empty() && sensorMap->find(lineToken) == sensorMap->end())
					line.skip = true;
				else {
					if (sensorMap != NULL && !sensorMap->empty())
						s = std::make_shared<ProcfsSensorBase>(*sensorMap->find(lineToken)->second);
					else {
						s = std::make_shared<ProcfsSensorBase>(std::string(lineToken));
						// If in "discovery" mode, the MQTT suffix is derived from the metric name
						s->setMqtt(std::string(lineToken));
					}
					s->setCPUId(-1);
					s->setMetric(std::string(lineToken));
					_sensors->push_back(s);
					line.skip = false;
					_numInternalMetrics++;
				}
				line.columns = 1;
				line.multi = false;
				line.cpuID = -1;
				line.dest = _sensors->size() - 1;
			}
			_lines.push_back(line);
			// Mapping the current CPU id to its first sensor entry in the vector
			if (line.cpuID >= 0 && !line.skip && htMap.find(effCPU) == htMap.end())
				htMap.insert(std::make_pair(effCPU, line));
		}
		_numMetrics = _sensors->size();
		// Adjusting the destinations in case of hyperthreading aggregation
		if (_htAggr)
			for (auto &l : _lines)
				if (!l.skip && l.cpuID >= 0 && htMap.find(l.cpuID) == htMap.end() && htMap.find(l.cpuID % _htVal) != htMap.end())
					l.dest = htMap[l.cpuID % _htVal].dest;
		htMap.clear();
	}
	return true;
}

/**
 * Parses the pointed file and updates sensor readings accordingly.
 *
 * This private method does not perform any check on the status of the parser, and is meant to only contain the parsing logic.
 *
 * @return    true if successful, false otherwise
 */
bool ProcstatParser::_readMetrics() {
	if (_readings == NULL)
		_readings = new std::vector<ureading_t>(_numMetrics);

	//fseek(_metricsFile, 0, SEEK_SET);
	if (_metricsFile) {
		fclose(_metricsFile);
		_metricsFile = NULL;
	}
	if ((_metricsFile = fopen(_path.c_str(), "r")) == NULL)
		return false;

	for (auto &v : *_readings)
		v.value = 0;

	unsigned int ctr = 0, lineCtr = 0, colCtr = 0, parsedCols = 0;
	char *       lineToken, *savePtr;
	// Reading the file line by line
	while (getline(&_stringBuffer, &_chars_read, _metricsFile) > 0 && ctr < _numInternalMetrics) {
		_l = &_lines[lineCtr++];
		if (!_l->skip) {
			savePtr = NULL;
			lineToken = strtok_r(_stringBuffer, LINE_SEP, &savePtr);
			// First token is discarded, represents the metric's name
			if (lineToken == NULL)
				return false;
			// We check whether the line is CPU core-related or not (see _readNames)
			if (_l->multi) {
				colCtr = 0;
				parsedCols = 0;
				// We iterate over the line and capture all of the CPU core-related metrics
				while ((lineToken = strtok_r(NULL, LINE_SEP, &savePtr)) != NULL && parsedCols < (unsigned)_l->columns) {
					// Second part of the if: if the line contains node-level CPU metrics (starts with "cpu") and the
					// column is flagged with 2 (sample only at node level) then the metric can be sampled
					if (_skipColumn[colCtr] == 1 || (_skipColumn[colCtr] == 2 && _l->cpuID < 0)) {
						try {
							_readings->at(_l->dest + parsedCols).value += std::stoull(lineToken);
						} catch (const std::invalid_argument &ia) { return false; } catch (const std::out_of_range &oor) {
							return false;
						}
						ctr++;
						parsedCols++;
					}
					colCtr++;
				}
			} else {
				// If the line is not CPU core-related, the second token is the metric itself
				// If the line contains multiple metrics, only the first will be captured, and the others discarded
				// This means that for the intr line, only the total number of interrupts raised is captured
				lineToken = strtok_r(NULL, LINE_SEP, &savePtr);
				if (lineToken == NULL)
					return false;
				try {
					_readings->at(_l->dest).value = std::stoull(lineToken);
				} catch (const std::invalid_argument &ia) { return false; } catch (const std::out_of_range &oor) {
					return false;
				}
				ctr++;
			}
		}
	}
	// Error: the number of read metrics does not match the one at initialization, file must have changed
	if (ctr != _numInternalMetrics)
		return false;
	return true;
}

// **************************************************************
// Definitions for the SARParser class

/**
 * Class destructor.
 *
 */
SARParser::~SARParser() {
	SARParser::close();
}

/**
 * Closes the parser.
 *
 * For SARParser, this method also releases all internal buffers used to store temporary readings.
 *
 */
void SARParser::close() {
	ProcfsParser::close();
	if (_accumulators != NULL)
		delete[] _accumulators;
	if (_columnRawReadings != NULL)
		delete[] _columnRawReadings;
	_accumulators = NULL;
	_columnRawReadings = NULL;
}

/**
 * Parses the pointed file and updates sensor readings accordingly.
 *
 * This private method does not perform any check on the status of the parser, and is meant to only contain the parsing logic.
 *
 * @return    true if successful, false otherwise
 */
bool SARParser::_readMetrics() {
	if (_readings == NULL) {
		_readings = new std::vector<ureading_t>(_numMetrics);
		_columnRawReadings = new unsigned long long[DEFAULTMETRICS * _lines.size()]();
		int accumSize = _htAggr ? _htVal + 1 : _numCPUs + 1;
		_accumulators = new unsigned long long[accumSize]();
		for (int idx = 0; idx < accumSize; idx++)
			_accumulators[idx] = 1;
	}

	//fseek(_metricsFile, 0, SEEK_SET);
	if (_metricsFile) {
		fclose(_metricsFile);
		_metricsFile = NULL;
	}
	if ((_metricsFile = fopen(_path.c_str(), "r")) == NULL)
		return false;

	for (auto &v : *_readings)
		v.value = 0;

	unsigned int ctr = 0, lineCtr = 0, colCtr = 0, parsedCols = 0, accIdx = 0;
	char *       lineToken, *savePtr;
	// Reading the file line by line
	while (getline(&_stringBuffer, &_chars_read, _metricsFile) > 0 && ctr < _numInternalMetrics) {
		_l = &_lines[lineCtr++];
		if (!_l->skip) {
			savePtr = NULL;
			lineToken = strtok_r(_stringBuffer, LINE_SEP, &savePtr);
			// First token is discarded, represents the metric's name
			if (lineToken == NULL)
				return false;
			// We check whether the line is CPU core-related or not (see _readNames)
			if (_l->multi) {
				colCtr = 0;
				parsedCols = 0;
				accIdx = (_htAggr && _l->cpuID >= 0) ? _l->cpuID % _htVal + 1 : _l->cpuID + 1;
				// We iterate over the line and capture all of the CPU core-related metrics
				while ((lineToken = strtok_r(NULL, LINE_SEP, &savePtr)) != NULL && colCtr < DEFAULTMETRICS) {
					// Computing the difference between the latest reading and the corresponding previous one
					try {
						_latestBuffer = std::stoull(lineToken);
					} catch (const std::invalid_argument &ia) { return false; } catch (const std::out_of_range &oor) {
						return false;
					}
					// Overflow checking - this occurrence is extremely rare (if not impossible) on /proc/stat values
					if (_latestBuffer >= _columnRawReadings[(lineCtr - 1) * DEFAULTMETRICS + colCtr])
						_latestValue = _latestBuffer - _columnRawReadings[(lineCtr - 1) * DEFAULTMETRICS + colCtr];
					else
						_latestValue = _latestBuffer + (ULLONG_MAX - _columnRawReadings[(lineCtr - 1) * DEFAULTMETRICS + colCtr]);
					_columnRawReadings[(lineCtr - 1) * DEFAULTMETRICS + colCtr] = _latestBuffer;
					_accumulators[accIdx] += _latestValue;
					// Second part of the if: if the line contains node-level CPU metrics (starts with "cpu") and the
					// column is flagged with 2 (sample only at node level) then the metric can be sampled
					if (_skipColumn[colCtr] == 1 || (_skipColumn[colCtr] == 2 && _l->cpuID < 0)) {
						_readings->at(_l->dest + parsedCols++).value += _latestValue;
						ctr++;
					}
					colCtr++;
				}
			} else {
				// If the line is not CPU core-related, the second token is the metric itself
				// If the line contains multiple metrics, only the first will be captured, and the others discarded
				// This means that for the intr line, only the total number of interrupts raised is captured
				lineToken = strtok_r(NULL, LINE_SEP, &savePtr);
				if (lineToken == NULL)
					return false;
				try {
					_readings->at(_l->dest).value = std::stoull(lineToken);
				} catch (const std::invalid_argument &ia) { return false; } catch (const std::out_of_range &oor) {
					return false;
				}
				ctr++;
			}
		}
	}

	// Computing percentages in SAR style
	short effCpu=0;
	for (auto &l : _lines) {
		effCpu = (_htAggr && l.cpuID >= 0) ? l.cpuID % _htVal : l.cpuID;
		// Percentages are computed only once per hyperthreading "bin"
		if (!l.skip && l.multi && _accumulators[effCpu + 1] != 1) {
			for (colCtr = 0; colCtr < (unsigned) l.columns; colCtr++)
				_readings->at(l.dest + colCtr).value = _readings->at(l.dest + colCtr).value * _sarMax / _accumulators[effCpu + 1];
			_accumulators[effCpu + 1] = 1;
		}
	}

	// Error: the number of read metrics does not match the one at initialization, file must have changed
	if (ctr != _numInternalMetrics)
		return false;
	return true;
}
