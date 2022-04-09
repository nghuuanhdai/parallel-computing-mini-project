//================================================================================
// Name        : RESTSensorBase.h
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Sensor base class for REST plugin.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2021 Leibniz Supercomputing Centre
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
 * @defgroup rest REST plugin
 * @ingroup  pusherplugins
 *
 * @brief Collect data from REST APIs
 */

#ifndef RESTSENSORBASE_H_
#define RESTSENSORBASE_H_

#include "RESTUnit.h"
#include "sensorbase.h"
#include <sstream>

/**
 * @brief SensorBase specialization for this plugin.
 *
 * @ingroup rest
 */
class RESTSensorBase : public SensorBase {
      public:
	RESTSensorBase(const std::string &name)
	    : SensorBase(name) {}

	RESTSensorBase(const RESTSensorBase &other)
	    : SensorBase(other),
	      _xmlPath(other._xmlPath) {}

	virtual ~RESTSensorBase() {}

	RESTSensorBase &operator=(const RESTSensorBase &other) {
		SensorBase::operator=(other);
		_xmlPath = other._xmlPath;

		return *this;
	}

	const xmlPathVector_t &getXMLPath() const { return _xmlPath; }
	std::string            getXMLPathString() const {
                std::stringstream ss;
                for (const auto &i : _xmlPath) {
                        ss << "." << std::get<0>(i) << "." << std::get<1>(i);
                        for (const auto &j : std::get<2>(i)) {
                                ss << "(" << j.first << "=" << j.second << ")";
                        }
                }
                return ss.str();
	}
	void setXMLPath(const std::string &path) {
		std::vector<std::string> subStrings;

		std::stringstream pathStream(path);
		std::string       item;
		//split into parts if a attribute (indicated by '(' ')' ) was defined
		while (std::getline(pathStream, item, ')')) {
			subStrings.push_back(item);
		}

		for (auto subStr : subStrings) {
			//extract the attributes from the path-parts
			if (subStr.find('(') != std::string::npos) { //attribute specified
				std::stringstream pathWithAttributesSStream(subStr);

				//split into path and attributes string
				std::string subPath, attributeString;
				std::getline(pathWithAttributesSStream, subPath, '(');
				std::getline(pathWithAttributesSStream, attributeString);

				if (subPath.front() == '.') {
					subPath.erase(0, 1);
				}

				//now further split the attributes string as multiple attributes could be defined
				std::vector<std::string> attributes;
				std::stringstream        attributeStream(attributeString);
				while (std::getline(attributeStream, item, ',')) {
					attributes.push_back(item);
				}
				attributesVector_t attrs;
				for (auto att : attributes) {
					//part attributes into name and value
					if (att.find('=') != std::string::npos) {
						std::stringstream attStream(att);

						std::string attName, attVal;
						std::getline(attStream, attName, '=');
						std::getline(attStream, attVal);

						attrs.push_back(std::make_pair(attName, attVal));
					} else { //should not happen. If it does the path was malformed
						//LOG(error) << "  Could not parse XML-path!";
						return;
					}
				}
				//split of the last child in the path. Required to iterate over multiple nodes which only differ in the attributes with BOOST_FOREACH
				auto index = subPath.find_last_of('.');
				if (index != std::string::npos) {
					std::string subPathChild(subPath.substr(++index));
					subPath.erase(--index);
					_xmlPath.push_back(std::make_tuple(subPath, subPathChild, attrs));
				} else { //the path contained only one node
					_xmlPath.push_back(std::make_tuple("", subPath, attrs));
				}
			} else { //no attributes specified. Last (sub)path
				if (subStr.front() == '.') {
					subStr.erase(0, 1);
				}
				_xmlPath.push_back(std::make_tuple(subStr, "", attributesVector_t()));
				break;
			}
		}
	}

	void printConfig(LOG_LEVEL ll, LOGGER &lg, unsigned leadingSpaces = 16) {
		std::string leading(leadingSpaces, ' ');
		LOG_VAR(ll) << leading << "    XML Path: " << getXMLPathString();
	}

      protected:
	xmlPathVector_t _xmlPath;
};

#endif /* RESTSENSORBASE_H_ */
