//================================================================================
// Name        : PDUUnit.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for PDUUnit class.
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

#ifndef PDUUNIT_H_
#define PDUUNIT_H_

#include "../../includes/EntityInterface.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/property_tree/ptree.hpp>
#include <openssl/ssl.h>

typedef std::vector<std::pair<std::string, std::string>>                      attributesVector_t;
typedef std::vector<std::tuple<std::string, std::string, attributesVector_t>> xmlPathVector_t;

/**
 * @brief Handles all connections to the same PDU unit.
 *
 * @ingroup pdu
 */
class PDUUnit : public EntityInterface {

      public:
	PDUUnit(const std::string &name = "");
	PDUUnit(const PDUUnit &other);
	virtual ~PDUUnit();
	PDUUnit &operator=(const PDUUnit &other);

	void setHost(const std::string &host) {
		size_t pos = host.find(':');
		if (pos != std::string::npos) {
			_name = host;
		} else {
			_name = host + ":443";
		}
	}

	const std::string &getHost() { return _name; }

	/**
	 * Send the request to the host and write the response into response.
	 *
	 * @param request	String with the request to send
	 * @param response	String where the response will be stored
	 *
	 * @return	True on success, false otherwise
	 */
	bool sendRequest(const std::string &request, std::string &response);

	void execOnInit() final override;

      private:
	SSL_CTX *_ctx;
};

#endif /* PDUUNIT_H_ */
