//================================================================================
// Name        : ProcfsSensorGroup.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Procfs sensor group class.
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

#ifndef PROCFSSENSORGROUP_H_
#define PROCFSSENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"
#include "ProcfsParser.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @details The ProcfsSensorGroup manages all sensors related to metrics in the
 *          same file. This is done in order to reduce parsing overhead, as all
 *          metrics are sampled in one reading pass.
 *
 * @ingroup procfs
 */
class ProcfsSensorGroup : public SensorGroupTemplate<ProcfsSensorBase> {

      public:
	// Constructor and destructor
	ProcfsSensorGroup(const std::string &name)
	    : SensorGroupTemplate(name) {
		_parser = NULL;
		_htVal = 0;
		_sarMax = 1000000;
	}
	ProcfsSensorGroup &operator=(const ProcfsSensorGroup &other);
	virtual ~ProcfsSensorGroup();

	// Methods inherited from SensorGroupTemplate
	bool execOnStart() final override;

	// Setters and getters
	void setParser(ProcfsParser *parser);
	void setHtVal(int htVal) { this->_htVal = htVal; }
	void setType(std::string t) { this->_type = t; }
	void setPath(std::string p) { this->_path = p; }
	void setCpuSet(std::set<int> s) { this->_cpuSet = s; }
	void setSarMax(uint64_t sf) {
		if (sf > 0)
			this->_sarMax = sf;
	}

	int            getHtVal() { return this->_htVal; }
	ProcfsParser * getParser() { return this->_parser; }
	std::string    getType() { return this->_type; }
	std::string    getPath() { return this->_path; }
	std::set<int> *getCpuSet() { return &this->_cpuSet; }
	uint64_t       getSarMax() { return this->_sarMax; }
	
	void printGroupConfig(LOG_LEVEL ll, unsigned leadingSpaces = 16) final override;

      private:
	// Methods inherited from SensorGroupTemplate
	void read() final override;

	// ProcfsParser object to be used to extract metrics' values and names
	ProcfsParser *_parser;
	// Type of the ProcfsSensorGroup (i.e. vmstat, meminfo, procstat), currently unused
	std::string _type;
	// Path to the parsed file
	std::string _path;
	// Aggregation value for hyperthreading cores
	int _htVal;
	// Set of cpu ids read during configuration
	std::set<int> _cpuSet;
	// Scaling factor to be used for ratio metrics
	uint64_t _sarMax;

	std::vector<ureading_t> *_readingVector;
	ureading_t               _readingBuffer;
};

using ProcfsSGPtr = std::shared_ptr<ProcfsSensorGroup>;

#endif /* PROCFSSENSORGROUP_H_ */
