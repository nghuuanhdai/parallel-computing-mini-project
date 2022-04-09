//================================================================================
// Name        : PDUSensorGroup.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for PDU sensor group class.
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

#include "PDUSensorGroup.h"

#include <sstream>

#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>

PDUSensorGroup::PDUSensorGroup(const std::string &name)
    : SensorGroupTemplateEntity(name) {
}

PDUSensorGroup::PDUSensorGroup(const PDUSensorGroup &other)
    : SensorGroupTemplateEntity(other),
      _request(other._request) {}

PDUSensorGroup::~PDUSensorGroup() {}

PDUSensorGroup &PDUSensorGroup::operator=(const PDUSensorGroup &other) {
	SensorGroupTemplate::operator=(other);
	_request = other._request;

	return *this;
}

void PDUSensorGroup::read() {
	//send request
	std::string response;
	if (!_entity->sendRequest(_request, response)) {
		LOG(error) << _groupName << " could not send request!";
		return;
	}

	//parse response
	boost::property_tree::ptree ptree;
	std::string                 xml = response.substr(response.find("<"));
	std::istringstream          treeStream(xml);
	try {
		boost::property_tree::read_xml(treeStream, ptree);
	} catch (const std::exception &e) {
		LOG(error) << _groupName << " got malformed XML response";
		return;
	}

	//read values for every sensor from response
	reading_t reading;
	reading.timestamp = getTimestamp();

	for (const auto &s : _sensors) {
		try {
			std::string                 readStr;
			const xmlPathVector_t &     xmlPath = s->getXMLPath();
			boost::property_tree::ptree node = ptree;
			for (size_t i = 0; i < xmlPath.size(); i++) {
				const std::string &       path = std::get<0>(xmlPath[i]);
				const std::string &       child = std::get<1>(xmlPath[i]);
				const attributesVector_t &attVec = std::get<2>(xmlPath[i]);

				unsigned matchCount;
				if (child != "") {
					BOOST_FOREACH (boost::property_tree::ptree::value_type &v, node.get_child(path)) {
						if (v.first == child) {
							matchCount = 0;
							for (size_t j = 0; j < attVec.size(); j++) {
								std::string attributeVal = v.second.get_child("<xmlattr>." + attVec[j].first).data();

								if (attributeVal != attVec[j].second) { //attribute values don't match
									break;
								} else {
									matchCount++;
								}
							}
							if (matchCount == attVec.size()) { //all attributes matched
								readStr = v.second.data();
								node = v.second;
								break;
							}
						}
					}
				} else { //child == ""
					readStr = node.get(path, "");
					break; //last (part of the) path
				}
			}

			if (readStr == "") {
				throw std::runtime_error("Value not found!");
			}
			reading.value = stoll(readStr);

#ifdef DEBUG
			LOG(debug) << _groupName << "::" << s->getName() << " raw reading: \"" << reading.value << "\"";
#endif
			s->storeReading(reading);
		} catch (const std::exception &e) {
			LOG(error) << _groupName << "::" << s->getName() << " could not read value: " << e.what();
			continue;
		}
	}
}

void PDUSensorGroup::printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) {
	std::string leading(leadingSpaces, ' ');
	LOG_VAR(ll) << leading << "Request: " << _request;
}
