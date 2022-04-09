//================================================================================
// Name        : libconfig.cpp
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for configuring libdcdb.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2019 Leibniz Supercomputing Centre
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//================================================================================

/**
 * @file libconfig.cpp
 * @brief This file contains actual implementations of the LibConfig
 * interface. The interface itself forwards all functions to the internal
 * LibConfigImpl. The real logic is implemented in the LibConfigImpl.
 */

#include <dcdb/libconfig.h>

DCDB::LibConfig libConfig;

using namespace DCDB;

LibConfig::LibConfig() {
}

LibConfig::~LibConfig() {
}

void LibConfig::init() {
    _initialized = true;
}

bool LibConfig::isInitialzed() {
    return _initialized;
}

void LibConfig::setTempDir(const std::string& tempDir) {
    if (tempDir.back() == '/') {
	_tempDir = tempDir.substr(0, tempDir.size()-1);
    } else {
	_tempDir = tempDir;
    }
}

std::string& LibConfig::getTempDir() {
    return _tempDir;
}
