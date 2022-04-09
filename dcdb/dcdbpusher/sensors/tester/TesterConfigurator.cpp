//================================================================================
// Name        : TesterConfigurator.cpp
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Tester plugin configurator class.
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

#include "TesterConfigurator.h"

TesterConfigurator::TesterConfigurator() {
	_groupName = "group";
	_baseName = "INVALID";
}

TesterConfigurator::~TesterConfigurator() {}

void TesterConfigurator::sensorBase(TesterSensorBase &s, CFG_VAL config) {}

void TesterConfigurator::sensorGroup(TesterSensorGroup &s, CFG_VAL config) {

	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
		if (boost::iequals(val.first, "numSensors")) {
			s.setNumSensors(stoull(val.second.data()));
		} else if (boost::iequals(val.first, "startValue")) {
			s.setValue(stoi(val.second.data()));
		}
	}

	std::string sTopic = "";
	for (unsigned int i = 0; i < s.getNumSensors(); i++) {
		sTopic = s.getGroupName() + std::to_string(i);
		std::shared_ptr<TesterSensorBase> sensor = std::make_shared<TesterSensorBase>(sTopic);
		sensor->setMqtt(sTopic);
		sensor->setCacheInterval(_cacheInterval);
		s.pushBackSensor(sensor);
	}
}
