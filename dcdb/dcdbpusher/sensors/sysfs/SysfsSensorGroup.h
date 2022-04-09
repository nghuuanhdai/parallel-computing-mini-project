//================================================================================
// Name        : SysfsSensorGroup.h
// Author      : Michael Ott, Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Sysfs sensor group class.
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

#ifndef SYSFSSENSORGROUP_H_
#define SYSFSSENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"
#include "SysfsSensorBase.h"
#include "glob.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup sysfs
 */
class SysfsSensorGroup : public SensorGroupTemplate<SysfsSensorBase> {

      public:
	SysfsSensorGroup(const std::string &name);
	SysfsSensorGroup(const SysfsSensorGroup &other);
	virtual ~SysfsSensorGroup();
	SysfsSensorGroup &operator=(const SysfsSensorGroup &other);

	void execOnInit() final override;
	bool execOnStart() final override;
	void execOnStop() final override;

	void setRetain(const bool r)		  { _retain = r; }
	void setPath(const std::string &path) { _path = path; }
	//	void setFile(FILE* file)				{ _file = file; }
	//	const std::string&	getPath()	const { return _path; }
	//	FILE* 				getFile()	const { return _file; }
	
	bool getRetain()					  { return _retain; }

	void printGroupConfig(LOG_LEVEL ll, unsigned int leadingSpaces) final override;

      private:
	void read() final override;

	std::string _path;
	FILE *      _file;
	bool		_retain;
};

#endif /* SYSFSSENSORGROUP_H_ */
