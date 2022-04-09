//================================================================================
// Name        : SysfsConfigurator.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Sysfs plugin configurator class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2017-2019 Leibniz Supercomputing Centre
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

#include "SysfsConfigurator.h"

SysfsConfigurator::SysfsConfigurator() {
	_groupName = "group";
	_baseName = "sensor";
}

SysfsConfigurator::~SysfsConfigurator() {}

void SysfsConfigurator::sensorBase(SysfsSensorBase &s, CFG_VAL config) {
	ADD {
		ATTRIBUTE("filter", setFilter);
	}
}

void SysfsConfigurator::sensorGroup(SysfsSensorGroup &s, CFG_VAL config) {
	BOOST_FOREACH (boost::property_tree::iptree::value_type &val, config) {
		if (boost::iequals(val.first, "path"))
			s.setPath(val.second.data());
		else if (boost::iequals(val.first, "retain"))
			s.setRetain(to_bool(val.second.data()));
	}
}
