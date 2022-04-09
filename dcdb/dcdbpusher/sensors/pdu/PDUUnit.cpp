//================================================================================
// Name        : PDUUnit.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Source file for PDUUnit class.
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

#include "PDUUnit.h"

#include <iostream>

#include <openssl/bio.h>
#include <openssl/err.h>

PDUUnit::PDUUnit(const std::string &name)
    : EntityInterface(name),
      _ctx(nullptr) {
}

PDUUnit::PDUUnit(const PDUUnit &other)
    : EntityInterface(other),
      _ctx(nullptr) {
}

PDUUnit::~PDUUnit() {
	if (_ctx) {
		SSL_CTX_free(_ctx);
	}
}

PDUUnit &PDUUnit::operator=(const PDUUnit &other) {
	EntityInterface::operator=(other);
	_ctx = nullptr;

	return *this;
}

void PDUUnit::execOnInit() {
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	_ctx = SSL_CTX_new(SSLv23_method());
}

bool PDUUnit::sendRequest(const std::string &request, std::string &response) {

	BIO *bio;

	if (!_ctx) {
		LOG(error) << "OpenSSL: Could not create context: " << ERR_reason_error_string(ERR_get_error());
		return false;
	}

	/*
	if (!SSL_CTX_load_verify_locations(ctx, "/home/micha/LRZ/dcdbOwnFork/deps/openssl-1.0.2l/certs/demo/ca-cert.pem", NULL)) {
		LOG(error) << "OpenSSL: Could not load certificate: " << ERR_reason_error_string(ERR_get_error());
		SSL_CTX_free(ctx);
		return;
	}
	*/

	bio = BIO_new_ssl_connect(_ctx);

	BIO_set_conn_hostname(bio, _name.c_str());

	if (BIO_do_connect(bio) <= 0) {
		LOG(error) << "OpenSSL: Could not connect: " << ERR_reason_error_string(ERR_get_error());
		BIO_free_all(bio);
		return false;
	}

	size_t      len = request.length();
	const char *reqBuf = request.c_str(); //request

	char resBuf[2048];
	memset(resBuf, 0, 2048);

	// do not bother too long if a write/read failed. Sensor intervals are usually small, just try again when next sensor wants to read.

	// send request
	if (BIO_write(bio, reqBuf, len) <= 0) {
		LOG(error) << "OpenSSL: Could not send request: " << ERR_reason_error_string(ERR_get_error());
		BIO_free_all(bio);
		return false;
	}

	// read reply
	response.clear();
	while ((len = BIO_read(bio, resBuf, sizeof(resBuf))) > 0) {
		resBuf[len] = 0;
		response.append(resBuf);
	}

	if (len < 0) {
		std::cerr << "OpenSSL: Could not read response: " << ERR_reason_error_string(ERR_get_error()) << std::endl;
		BIO_free_all(bio);
		return false;
	}

	BIO_free_all(bio);

	return true;
}
