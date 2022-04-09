//================================================================================
// Name        : SysfsSensorBase.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for Sysfs plugin.
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

/**
 * @defgroup sysfs SysFS plugin
 * @ingroup  pusherplugins
 *
 * @brief Collect data from the /sys file system.
 */

#ifndef SYSFS_SYSFSSENSORBASE_H_
#define SYSFS_SYSFSSENSORBASE_H_

#include "sensorbase.h"

#include <boost/regex.hpp>

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup sysfs
 */
class SysfsSensorBase : public SensorBase {
      public:
	SysfsSensorBase(const std::string &name)
	    : SensorBase(name),
	      _filter(false),
	      _substitution("") {
		//_regx = "";
	}

	SysfsSensorBase(const SysfsSensorBase &other)
	    : SensorBase(other),
	      _filter(other._filter),
	      _regx(other._regx),
	      _substitution(other._substitution) {
	}

	virtual ~SysfsSensorBase() {}

	SysfsSensorBase &operator=(const SysfsSensorBase &other) {
		SensorBase::operator=(other);
		_filter = other._filter;
		_regx = other._regx;
		_substitution = other._substitution;

		return *this;
	}

	bool               hasFilter() const { return _filter; }
	boost::regex       getRegex() const { return _regx; }
	const std::string &getSubstitution() const { return _substitution; }

	void setFilter(bool filter) { _filter = filter; }
	void setFilter(const std::string &filter) {
		setFilter(true);

		//check if input has sed format of "s/.../.../" for substitution
		boost::regex  checkSubstitute("s([^\\\\]{1})([\\S|\\s]*)\\1([\\S|\\s]*)\\1");
		boost::smatch matchResults;

		if (regex_match(filter, matchResults, checkSubstitute)) {
			//input has substitute format
			setRegex(boost::regex(matchResults[2].str(), boost::regex_constants::extended));
			setSubstitution(matchResults[3].str());
		} else {
			//input is only a regex
			setRegex(boost::regex(filter, boost::regex_constants::extended));
			setSubstitution("&");
		}
	}
	void setRegex(boost::regex regx) { _regx = regx; }
	void setSubstitution(const std::string &substitution) { _substitution = substitution; }

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		if (_filter) {
			LOG_VAR(ll) << leading << "    Using regular expression as filter";
			LOG_VAR(ll) << leading << "    Substitution: " << _substitution;
		} else {
			LOG_VAR(ll) << leading << "    Not using any filter";
		}
	}

      protected:
	bool         _filter;
	boost::regex _regx;
	std::string  _substitution;
};

#endif /* SYSFS_SYSFSSENSORBASE_H_ */
