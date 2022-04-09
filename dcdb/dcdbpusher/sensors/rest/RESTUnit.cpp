//================================================================================
// Name        : RESTUnit.cpp
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for RESTUnit class.
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

#include "RESTUnit.h"
#include "globalconfiguration.h"

#include <iostream>

#include <openssl/bio.h>
#include <openssl/err.h>

RESTUnit::RESTUnit(const std::string &name)
    : EntityInterface(name),
      _ctx(ssl::context::tlsv12_client),
      _ssl(false) {
}

RESTUnit::RESTUnit(const RESTUnit &other)
    : EntityInterface(other),
      _ctx(ssl::context::tlsv12_client),
      _baseURL(other._baseURL),
      _hostname(other._hostname),
      _port(other._port),
      _path(other._path),
      _authEndpoint(other._authEndpoint),
      _authData(other._authData),
      _ssl(false) {
}

RESTUnit::~RESTUnit() {
}

RESTUnit &RESTUnit::operator=(const RESTUnit &other) {
	EntityInterface::operator=(other);
	_baseURL = other._baseURL;
	_hostname = other._hostname;
	_port = other._port;
	_path = other._path;
	_authEndpoint = other._authEndpoint;
	_authData = other._authData;
	_ssl = other._ssl;

	return *this;
}

void RESTUnit::execOnInit() {
}

bool RESTUnit::authenticate() {
    tcp::resolver resolver(_ioc);
    auto const hosts = resolver.resolve(_hostname, _port);
    
    beast::ssl_stream<beast::tcp_stream> stream(_ioc, _ctx);
    beast::get_lowest_layer(stream).connect(hosts);

    if (_ssl) {
	if(! SSL_set_tlsext_host_name(stream.native_handle(), _hostname.c_str()))
	{
	    beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
	    throw beast::system_error{ec};
	}
	stream.handshake(ssl::stream_base::client);
    }
    
    
    http::request<http::string_body> auth{http::verb::post, _path + _authEndpoint, 11};
    auth.set(http::field::host, _hostname);
    auth.body() = _authData;
    auth.prepare_payload();
    
    if (_ssl) {
	http::write(stream, auth);
    } else {
	http::write(beast::get_lowest_layer(stream), auth);
    }
    
    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    if (_ssl) {
	http::read(stream, buffer, res);
    } else {
	http::read(beast::get_lowest_layer(stream), buffer, res);
    }
    
    _cookie = std::string(res[http::field::set_cookie]);
    if (_cookie.size() > 0) {
	LOG(debug) << "Authenticated to " << _hostname << ", received cookie: " << _cookie;
	return true;
    } else {
	LOG(info) << "Could not authenticate to " << _hostname << ", no cookie received.";
	return false;
    }
}

bool RESTUnit::sendRequest(const std::string &endpoint, const std::string &request, std::string &response) {
    if ((_authData.size() > 0) && (_cookie.size() == 0) && (!authenticate())) {
	return false;
    }

    tcp::resolver resolver(_ioc);
    auto const hosts = resolver.resolve(_hostname, _port);
    
    beast::ssl_stream<beast::tcp_stream> stream(_ioc, _ctx);
    beast::get_lowest_layer(stream).connect(hosts);

    if (_ssl) {
	if(! SSL_set_tlsext_host_name(stream.native_handle(), _hostname.c_str()))
	{
	    beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
	    throw beast::system_error{ec};
	}
	
	stream.handshake(ssl::stream_base::client);
    }

    http::request<http::string_body> req{http::verb::get, _path + endpoint, 11};
    req.set(http::field::host, _hostname);
    if (_cookie.size() > 0) {
	req.set(http::field::cookie, _cookie);
    }
    req.body() = request;
    req.prepare_payload();
    
    // Send the HTTP request to the remote host
    if (_ssl) {
	http::write(stream, req);
    } else {
	http::write(beast::get_lowest_layer(stream), req);
    }

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    if (_ssl) {
	http::read(stream, buffer, res);
    } else {
	http::read(beast::get_lowest_layer(stream), buffer, res);		
    }
    
    if (res.body().size() > 0) {
	response = res.body();
	return true;
    } else {
	return false;
    }
}
