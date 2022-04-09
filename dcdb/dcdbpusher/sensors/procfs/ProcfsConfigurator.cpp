//================================================================================
// Name        : ProcfsConfigurator.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Procfs plugin configurator class.
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

#include "ProcfsConfigurator.h"

/**
 *  Class constructor.
 *
 */
ProcfsConfigurator::ProcfsConfigurator() {
	_groupName = "file";
	_baseName = "metric";
}

/**
 *  Class destructor.
 *
 */
ProcfsConfigurator::~ProcfsConfigurator() {}

/**
 * Configures a ProcfsSensorBase object instantiated by the readSensorBase method.
 *
 * @param s: the ProcfsSensorBase object to be configured
 * @param config: the BOOST property (sub-)tree containing configuration options for the sensor
 *
 */
void ProcfsConfigurator::sensorBase(ProcfsSensorBase &s, CFG_VAL config) {
	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
		if (boost::iequals(val.first, "type"))
			s.setMetric(val.second.data());
		else if (boost::iequals(val.first, "perCPU"))
			s.setPerCPU(to_bool(val.second.data()));
	}
}

/**
 * Configures a ProcfsSensorGroup object instantiated by the readSensorGroup method.
 *
 * For simplicity, the assignment of MQTT topics to sensors is done here and not in readConfig(),
 * unlike in ConfiguratorTemplate.
 *
 * @param sGroup: the ProcfsSensorGroup object to be configured
 * @param config: the BOOST property (sub-)tree containing configuration options for the sensor group
 *
 */
void ProcfsConfigurator::sensorGroup(ProcfsSensorGroup &sGroup, CFG_VAL config) {
	std::vector<ProcfsSBPtr> derivedSensors;
	ProcfsParser *           parser;

	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
		if (boost::iequals(val.first, "type")) {
			sGroup.setType(val.second.data());
		} else if (boost::iequals(val.first, "path")) {
			sGroup.setPath(val.second.data());
		} else if (boost::iequals(val.first, "cpus")) {
			sGroup.setCpuSet(parseCpuString(val.second.data()));
		} else if (boost::iequals(val.first, "htVal")) {
			sGroup.setHtVal(stoi(val.second.data()));
		} else if (boost::iequals(val.first, "sarMax")) {
			sGroup.setSarMax(stoull(val.second.data()));
		}
	}

	// The "type" parameter must refer to either vmstat, procstat or meminfo. If not, the sensor group is not initialized
	// The only other case is when an empty string is found, which can happen for template groups
	std::string fileType = sGroup.getType();
	std::string filePath = sGroup.getPath();
	if (fileType == "")
		return;
	else if (fileType == "vmstat")
		parser = new VmstatParser(filePath);
	else if (fileType == "procstat")
		parser = new ProcstatParser(filePath);
	else if (fileType == "sar")
		parser = new SARParser(filePath);
	else if (fileType == "meminfo")
		parser = new MeminfoParser(filePath);
	else {
		LOG(warning) << _groupName << " " << sGroup.getGroupName() << "::"
			     << "Unspecified or invalid type! Available types are vmstat, meminfo, procstat, sar";
		return;
	}

	// After getting the vector of available metrics (subset of the sensor map, if any) from the parsed file, we delete
	// all sensors not matching any parsed metrics, and arrange their order to respect the one in the file
	derivedSensors = sGroup.getDerivedSensors();
	parser->setHtVal(sGroup.getHtVal());
	parser->setSarMax(sGroup.getSarMax());

	// If no metrics were found in the file (or the file is unreadable) the configuration aborts
	int numMetrics = 0;
	if (!parser->init(&derivedSensors, sGroup.getCpuSet()) || (numMetrics = parser->getNumMetrics()) == 0) {
		LOG(warning) << _groupName << " " << sGroup.getGroupName() << "::"
			     << "Unable to parse file " << filePath << ", please check your configuration!";
		sGroup.acquireSensors().clear();
		sGroup.getDerivedSensors().clear();
		return;
	}
	LOG(debug) << "  Number of metrics found: " << numMetrics;

	// We assign the final sensor objects as parsed in the target file, and assign the parser object
	sGroup.setParser(parser);
}
