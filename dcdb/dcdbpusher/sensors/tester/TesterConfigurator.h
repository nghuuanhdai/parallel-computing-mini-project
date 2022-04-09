//================================================================================
// Name        : TesterConfigurator.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Tester plugin configurator class.
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

#ifndef TESTERCONFIGURATOR_H_
#define TESTERCONFIGURATOR_H_

#include "../../includes/ConfiguratorTemplate.h"
#include "TesterSensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup tester
 */
class TesterConfigurator : public ConfiguratorTemplate<TesterSensorBase, TesterSensorGroup> {

      public:
	TesterConfigurator();
	virtual ~TesterConfigurator();

      protected:
	/* Overwritten from ConfiguratorTemplate */
	void sensorBase(TesterSensorBase &s, CFG_VAL config) override;
	void sensorGroup(TesterSensorGroup &s, CFG_VAL config) override;
};

extern "C" ConfiguratorInterface *create() {
	return new TesterConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* TESTERCONFIGURATOR_H_ */
