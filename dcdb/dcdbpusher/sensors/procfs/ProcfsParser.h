//================================================================================
// Name        : ProcfsParser.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for ProcfsParser.
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

#ifndef PROCFSPARSER_H_
#define PROCFSPARSER_H_

#include "ProcfsSensorBase.h"
#include "mqttchecker.h"
#include "timestamp.h"
#include <boost/regex.hpp>
#include <exception>
#include <map>
#include <set>
#include <string.h>
#include <string>
#include <vector>

/**
 * @brief Provides an interface to parse metrics (and their values) from files
 *        in the /proc filesystem.
 *
 * @ingroup procfs
 */
class ProcfsParser {

      public:
	struct ProcLine {
		bool skip;
		bool multi;
		char columns;
		int  dest;
		int  cpuID;
	};

	// Constructor and destructor
	ProcfsParser(std::string path);
	virtual ~ProcfsParser();

	// Init/close methods
	bool         init(std::vector<ProcfsSBPtr> *sensorVec = NULL, std::set<int> *cpuSet = NULL);
	virtual void close();

	// Setters and getters
	std::set<int>& getFoundCPUs() {return _foundCPUs; }
	std::string  getPath() { return _path; }
	int          getHtVal() { return _htVal; }
	unsigned int getNumMetrics() { return _numMetrics; }
	unsigned int getNumCPUs() { return _numCPUs; }
	unsigned int getCacheIndex() { return _cacheIndex; }
	uint64_t     getSarMax() { return _sarMax; }
	void         setCacheIndex(unsigned int c) { _cacheIndex = c; }
	void         setHtVal(int v) {
                _htVal = v;
                _htAggr = v > 0;
	}
	void setPath(std::string new_path) {
		_path = new_path;
		close();
	}
	void setSarMax(uint64_t sf) { _sarMax = sf; }

	std::vector<ProcfsSBPtr> *getSensors();
	std::vector<ureading_t> * readSensors();

      protected:
	// Private parsing methods that implement all necessary logic to parse metrics' names (and associated CPU cores, if any)
	// and values. MUST be overridden by children classes
	virtual bool _readNames(std::map<std::string, ProcfsSBPtr> *sensorMap, std::set<int> *cpuSet) { return false; };
	virtual bool _readMetrics() { return false; };

	// Vector containing the names of parsed metrics, associated CPU cores (if any), and values.
	// After initialization, _sensors is always non-NULL.
	std::vector<ProcfsSBPtr> *_sensors;
	// Vector of performed readings
	std::vector<ureading_t> *_readings;
	// Keeps track of which lines and columns in the parsed file must be skipped
	std::vector<ProcLine> _lines;
	// Pointer to the current line being parsed
	ProcLine *_l;
	// Every element in skipColumn can be either 0 (skip column), 1 (parse for all CPUs) or 2 (parse only at node level)
	std::vector<char> _skipColumn;

	// Internal variables
	std::set<int> _foundCPUs;
	bool          _initialized;
	uint64_t      _sarMax;
	unsigned int  _cacheIndex;
	unsigned int  _numMetrics;
	unsigned int  _numInternalMetrics;
	unsigned int  _numCPUs;
	int           _htVal;
	bool          _htAggr;
	std::string   _path;
	FILE *        _metricsFile;
	char *        _stringBuffer;
	size_t        _chars_read;
	// C string containing the delimiters used to split single lines during parsing
	const char *LINE_SEP;
};

// **************************************************************
/**
 * The MeminfoParser class allows for parsing and reading values from a /proc/meminfo file.
 *
 */
class MeminfoParser : public ProcfsParser {

      public:
	MeminfoParser(const std::string path = "")
	    : ProcfsParser(path) {
		LINE_SEP = " :\t";
		_path = (path == "" ? "/proc/meminfo" : path);
	}

      protected:
	bool _readNames(std::map<std::string, ProcfsSBPtr> *sensorMap, std::set<int> *cpuSet) override;
	bool _readMetrics() override;

	int               _memTotalLine;
	int               _memFreeLine;
	ureading_t        _memTotalValue;
	ureading_t        _memFreeValue;
	ureading_t        _valueBuffer;
	const std::string _memUsedToken = "MemUsed";
};

// **************************************************************
/**
 * The VmstatParser class allows for parsing and reading values from a /proc/vmstat file.
 *
 * The vmstat parser is identical to the meminfo one. The only difference lies in some tokens to be discarded
 * (the ':' after the metrics' names) which can be handled by strtok without any alteration to the code. Moreover,
 * the logic required to compute the MemUsed derived sensor in MeminfoParser is never used here.
 */
class VmstatParser : public MeminfoParser {

      public:
	VmstatParser(const std::string path = "")
	    : MeminfoParser(path) {
		LINE_SEP = " \t";
		_path = (path == "" ? "/proc/vmstat" : path);
	}
};

// **************************************************************
/**
 * The ProcstatParser class allows for parsing and reading values from a /proc/stat file.
 *
 * The proc/stat parser is slightly different compared to the vmstat and meminfo ones. It provides additional logic to distinguish
 * the CPUs to which each metric belongs, and the _metricsCores vector is in this class set to a non-NULL value after initialization
 */
class ProcstatParser : public ProcfsParser {

      public:
	ProcstatParser(const std::string path = "")
	    : ProcfsParser(path) {
		LINE_SEP = " \t";
		_path = (path == "" ? "/proc/stat" : path);
	}

      protected:
	bool _readNames(std::map<std::string, ProcfsSBPtr> *sensorMap, std::set<int> *cpuSet) override;
	bool _readMetrics() override;

	// Internal variables
	unsigned int  _numColumns;
	boost::cmatch _match;
	// The number of known metrics in each "cpu" line together with their names, as of Oct. 2018
	enum { DEFAULTMETRICS = 10 };
	const std::string DEFAULT_NAMES[DEFAULTMETRICS] = {"col_user", "col_nice", "col_system", "col_idle", "col_iowait", "col_irq", "col_softirq", "col_steal", "col_guest", "col_guest_nice"};
	// Lookup vector that keeps track of where all the sensors for a specific CPU are mapped in the reading vector
	// C strings that encode the prefixes of cpu-related lines
	const char *         _cpu_prefix = "cpu";
	const unsigned short _cpu_prefix_len = strlen(_cpu_prefix);
	// Regex to match strings ending with integer numbers (in this case, CPU core IDs)
	const boost::regex reg_exp_num = boost::regex("[0-9]+$");
};

// **************************************************************
/**
 * The SARParser class allows for parsing and reading values from a /proc/stat file, with percentage readings.
 *
 * This parser extends the normal ProcstatParser by computing percentage readings for the cpu columns, instead of
 * using the raw values. Each column is divided by the sum of all columns, and then multiplied by 1000 to be stored
 * as an integer. Moreover, we use the differences between successive readings for each column.
 *
 */
class SARParser : public ProcstatParser {

      public:
	SARParser(const std::string path = "")
	    : ProcstatParser(path) {}
	void close() override;
	virtual ~SARParser();

      protected:
	bool _readMetrics() override;

	// Accumulates the readings for all columns of a given CPU line; used to later compute percentages
	unsigned long long *_accumulators;
	// Matrix storing the readings of columns from all CPU lines; used to compute differences between readings
	unsigned long long *_columnRawReadings;
	unsigned long long  _accumulator;
	unsigned long long  _latestValue;
	unsigned long long  _latestBuffer;
};

#endif /* PROCFSPARSER_H_ */
