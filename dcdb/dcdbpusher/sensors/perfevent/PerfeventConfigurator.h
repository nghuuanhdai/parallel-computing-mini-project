//================================================================================
// Name        : PerfeventConfigurator.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for Perfevent plugin configurator class.
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

#ifndef PERFEVENTCONFIGURATOR_H_
#define PERFEVENTCONFIGURATOR_H_

#include <set>

#include "../../includes/ConfiguratorTemplate.h"
#include "PerfSensorGroup.h"

/**
 * @brief ConfiguratorTemplate specialization for this plugin.
 *
 * @ingroup perf
 */
class PerfeventConfigurator : public ConfiguratorTemplate<PerfSensorBase, PerfSensorGroup> {

	typedef std::map<std::string, std::set<int>> templateCpuMap_t;
	typedef std::map<std::string, unsigned>      templateHTMap_t;
	typedef std::map<std::string, unsigned int>  enumMap_t;

      public:
	PerfeventConfigurator();
	virtual ~PerfeventConfigurator();

      protected:
	/* OVerwritten from ConfiguratorTemplate */
	void sensorBase(PerfSensorBase &s, CFG_VAL config) override;
	void sensorGroup(PerfSensorGroup &s, CFG_VAL config) override;
	bool readConfig(std::string cfgPath) override;

      private:
	/**
	 *  Takes a PerfSensorGroup and duplicates it for every CPU specified.
	 *  Assigns one CPU value to every newly constructed group and stores them
	 *  afterwards. Also sets everything for hyper-threading aggregation if
	 *  enabled.
	 *
	 *  @param group    PerfSensorGroup which is to be customized for every CPU
	 *  @param cfg      Config tree for the group. Required to read in possibly
	 *                  differing CPU lists and default groups.
	 */
	void customizeAndStore(PerfSensorGroup &group, CFG_VAL cfg);

	templateCpuMap_t _templateCpus;
	templateHTMap_t  _templateHTs;

	enumMap_t _enumType;
	enumMap_t _enumConfig;
};

extern "C" ConfiguratorInterface *create() {
	return new PerfeventConfigurator;
}

extern "C" void destroy(ConfiguratorInterface *c) {
	delete c;
}

#endif /* PERFEVENTCONFIGURATOR_H_ */
