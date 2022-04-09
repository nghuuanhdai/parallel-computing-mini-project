//================================================================================
// Name        : TesterSensorGroup.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Tester sensor group class.
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

#ifndef TESTERSENSORGROUP_H_
#define TESTERSENSORGROUP_H_

#include "../../includes/SensorGroupTemplate.h"
#include "TesterSensorBase.h"

/**
 * @brief SensorGroupTemplate specialization for this plugin.
 *
 * @ingroup tester
 */
class TesterSensorGroup : public SensorGroupTemplate<TesterSensorBase> {

      public:
	TesterSensorGroup(const std::string &name);
	TesterSensorGroup &operator=(const TesterSensorGroup &other);
	virtual ~TesterSensorGroup();

	void setValue(long long n) { _value = n; }
	void setNumSensors(unsigned int n) { _numSensors = n; }

	long long    getValue() { return _value; }
	unsigned int getNumSensors() { return _numSensors; }

	void printGroupConfig(LOG_LEVEL ll, unsigned leadingSpaces) final override;

      private:
	void read() final override;

	long long    _value;
	unsigned int _numSensors;
};

#endif /* TESTERSENSORGROUP_H_ */
