//================================================================================
// Name        : libconfig.h
// Author      : Michael Ott
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : C++ API for configuring libdcdb.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2011-2021 Leibniz Supercomputing Centre
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
 * @file libconfig.h
 * @brief This file contains parts of the public API for the libdcdb library.
 * It contains the class definition of the LibConfig class to configure global
 * parameters of libdcdb.
 *
 * @ingroup libdcdb
 */


#ifndef DCDB_LIBCONFIG_H
#define DCDB_LIBCONFIG_H

#include <string>

namespace DCDB {
    class LibConfig {
    private:
	std::string _tempDir;
	bool _initialized = false;

    public:
	LibConfig();
	~LibConfig();
	void init();
	bool isInitialzed();
	void setTempDir(const std::string& tempDir);
	std::string& getTempDir();
    };
}

extern DCDB::LibConfig libConfig;

#endif
