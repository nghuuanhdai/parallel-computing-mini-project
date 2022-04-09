//================================================================================
// Name        : SysfsSensorGroup.cpp
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for Sysfs sensor group class.
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

#include "SysfsSensorGroup.h"
#include <cstring>
#include <functional>

using namespace std;

SysfsSensorGroup::SysfsSensorGroup(const std::string &name)
    : SensorGroupTemplate(name),
      _path("") {
	_file = NULL;
	_retain = true;
}

SysfsSensorGroup::SysfsSensorGroup(const SysfsSensorGroup &other)
    : SensorGroupTemplate(other),
      _path(other._path) {
	_file = NULL;
	_retain = other._retain;
}

SysfsSensorGroup::~SysfsSensorGroup() {}

SysfsSensorGroup &SysfsSensorGroup::operator=(const SysfsSensorGroup &other) {
	SensorGroupTemplate::operator=(other);
	_path = other._path;
	_file = NULL;
	_retain = other._retain;

	return *this;
}

void SysfsSensorGroup::execOnInit() {
	if(std::strpbrk(_path.c_str(), "?*[]")) {
		LOG(debug) << "Detected pattern " << _path << ".";
		glob_t paths;
		if(glob(_path.c_str(), 0, NULL, &paths)==0) {
			if(paths.gl_pathc > 1) {
				LOG(warning) << "Multiple matches found for pattern " << _path << ". Only the first one will be picked.";
			}
			_path = paths.gl_pathv[0];
            LOG(debug) << "Using path " << _path << " from pattern.";
			globfree(&paths);
		} else {
			LOG(warning) << "No matches found for pattern " << _path << "!";
		}
	}
}

bool SysfsSensorGroup::execOnStart() {
	if (_retain && _file == NULL) {
		_file = fopen(_path.c_str(), "r");
		if (_file == NULL) {
			LOG(error) << "Error starting group\"" << _groupName << "\": " << strerror(errno);
			return false;
		}
	}
	return true;
}

void SysfsSensorGroup::execOnStop() {
	if (_retain && _file != NULL) {
		fclose(_file);
		_file = NULL;
	}
}

void SysfsSensorGroup::read() {
	reading_t reading;
	char      buf[1024];

	// In case the file needs to be opened and closed at every read
	if(!_retain && _file == NULL) {
		_file = fopen(_path.c_str(), "r");
		if (_file == NULL) {
			LOG(error) << "Error opening file " << _path << ":" << strerror(errno);
			return;
		}
	} else {
		fseek(_file, 0, SEEK_SET);
	}
	
	size_t nelem = fread(buf, 1, 1024, _file);
	reading.timestamp = getTimestamp();

	if (!_retain && _file != NULL) {
		fclose(_file);
		_file = NULL;
	}

	if (nelem) {
		buf[nelem - 1] = 0;
		for (const auto &s : _sensors) {
			reading.value = 0;
			try {
				//filter the payload if necessary
				if (s->hasFilter()) {
					//substitution must have sed format
					//if no substitution is defined the whole regex-match is copied as is.
					//parts which do not match are not copied --> filter

					//reading.value = stoll(regex_replace(buf, s->getRegex(), s->getSubstitution(), boost::regex_constants::format_sed | boost::regex_constants::format_no_copy));
					//there is no 1:1 match for the std::regex_replace function used previously, but the code below should have the same outcome
					std::string input(buf);
					std::string result;
					boost::regex_replace(std::back_inserter(result), input.begin(), input.end(), s->getRegex(), s->getSubstitution().c_str(), boost::regex_constants::format_sed | boost::regex_constants::format_no_copy);
					reading.value = stoll(result);
				} else {
					reading.value = stoll(buf);
				}
#ifdef DEBUG
				LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
				s->storeReading(reading);
			} catch (const std::exception &e) {
				LOG(error) << _groupName << "::" << s->getName() << " could not read value: " << e.what();
				continue;
			}
		}
	} else {
		LOG(error) << _groupName << " could not read file!";
	}
}

void SysfsSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "File path: " << _path;
	LOG_VAR(ll) << leading << "Retain:    " << (_retain ? "true" : "false");
}
