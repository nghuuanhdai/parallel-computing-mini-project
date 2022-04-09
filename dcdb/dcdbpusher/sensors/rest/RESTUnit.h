//================================================================================
// Name        : RESTUnit.h
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for RESTUnit class.
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

#ifndef RESTUNIT_H_
#define RESTUNIT_H_

#include "../../includes/EntityInterface.h"

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tc

typedef std::vector<std::pair<std::string, std::string>>                      attributesVector_t;
typedef std::vector<std::tuple<std::string, std::string, attributesVector_t>> xmlPathVector_t;

/**
 * @brief Handles all connections to the same REST unit.
 *
 * @ingroup rest
 */
class RESTUnit : public EntityInterface {

      public:
	RESTUnit(const std::string &name = "");
	RESTUnit(const RESTUnit &other);
	virtual ~RESTUnit();
	RESTUnit &operator=(const RESTUnit &other);

	void setBaseURL(const std::string &baseURL) {
		_baseURL = baseURL;
		// URL parsing courtesy of https://stackoverflow.com/a/61214599
		boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
		boost::cmatch what;
		if(regex_match(baseURL.c_str(), what, ex))
		{
			std::string protocol = std::string(what[1].first, what[1].second);
			_hostname = std::string(what[2].first, what[2].second);
			_port     = std::string(what[3].first, what[3].second);
			_path     = std::string(what[4].first, what[4].second);
			std::string query = std::string(what[5].first, what[5].second);
			if (boost::iequals(protocol, "https")) {
				_ssl = true;
			}
			if (_port.size() == 0) {
				if (_ssl) {
					_port = "443";
				} else {
					_port = "80";
				}
			}
		}
	}
	void setAuthEndpoint(const std::string &authEndpoint) { _authEndpoint = authEndpoint; }
	void setAuthData(const std::string &authData) { _authData = authData; }
	
	const std::string &getBaseURL() { return _baseURL; }
	const std::string &getAuthEndpoint() { return _authEndpoint; }
	const std::string &getAuthData() { return _authData; }
	bool authenticate();
	/**
	 * Send the request to the host and write the response into response.
	 *
	 * @param request	String with the request to send
	 * @param response	String where the response will be stored
	 *
	 * @return	True on success, false otherwise
	 */
	bool sendRequest(const std::string &endpoint, const std::string &request, std::string &response);

	void execOnInit() final override;

      private:
	net::io_context _ioc;
	ssl::context _ctx;
	std::string _baseURL;
	std::string _hostname;
	std::string _port;
	std::string _path;
	std::string _authEndpoint;
	std::string _authData;
	std::string _cookie;
	bool _ssl;
};

#endif /* RESTUNIT_H_ */
