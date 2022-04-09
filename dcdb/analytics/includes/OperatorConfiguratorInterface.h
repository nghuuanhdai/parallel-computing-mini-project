//================================================================================
// Name        : OperatorConfiguratorInterface.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Interface to configurators for operator plugins.
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

#ifndef PROJECT_OPERATORCONFIGURATORINTERFACE_H
#define PROJECT_OPERATORCONFIGURATORINTERFACE_H

#include <string>
#include <vector>
#include "version.h"

#include "QueryEngine.h"
#include "OperatorInterface.h"
#include "globalconfiguration.h"

/**
 * @brief Interface to configurators for operator plugins
 *
 * @details This interface is the one exposed outside of the .dll library
 *          containing a plugin. Classes implementing this interface must
 *          perform configuration and instantiation of Operator objects, which
 *          are made accessible externally.
 *
 * @ingroup operator
 */
class OperatorConfiguratorInterface {

public:

    /**
    * @brief            Class constructor
    */
    OperatorConfiguratorInterface() {}

    /**
    * @brief            Class destructor
    */
    virtual ~OperatorConfiguratorInterface() {}

    /**
    * @brief            Reads a config file and instantiates operators
    *
    *                   This method will read the config file to which the input path points, and instantiate operator
    *                   objects accordingly. This method must be implemented in derived classes.
    *
    * @param cfgPath    Path of the input config file
    * @return           True if successful, false otherwise
    */
    virtual bool	readConfig(std::string cfgPath)	= 0;

    /**
    * @brief            Repeats the configuration of the plugin
    *
    *                   This method will stop and clear all operators that were created, and repeats the configuration,
    *                   by reading the file once again. This method must be implemented in derived classes.
    *
    * @return           True if successful, false otherwise
    */
    virtual bool	reReadConfig() 					= 0;

    /**
    * @brief            Clears the plugin configuration
    *
    *                   This method will stop and clear all operators that were created, returning the plugin to its
    *                   uninitialized state.
    *
    */
    virtual void	clearConfig() 					= 0;

    /**
    * @brief                    Sets a structure containing global settings to be used during operator creation This
    *                           method must be implemented in derived classes.
    *
    *
    * @param pluginSettings     A structure of plugin settings (see pluginsettings.h)
    */
    virtual void	setGlobalSettings(const pluginSettings_t& pluginSettings) = 0;

    /**
    * @brief            Returns a vector of operators
    *
    *                   This method will return the internal vector of OperatorInterface objects, granting access
    *                   to their units and output sensors. This method must be implemented in derived classes.
    *
    * @return           a vector of pointers to OperatorInterface objects
    */
    virtual std::vector<OperatorPtr>& getOperators()	= 0;

    /**
    * @brief            Prints the current plugin configuration
    *
    * @param ll         Logging level at which the configuration is printed
    */
    virtual void printConfig(LOG_LEVEL ll)       = 0;

    /**
    * @brief Get current DCDB version.
    *
    * @return Version number as string.
    */
    std::string getVersion() {
        return std::string(VERSION);
    }

protected:

    // Logger object
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
};

// Typedefs for readable usage of create() and destroy() methods, required for dynamic libraries
typedef OperatorConfiguratorInterface* op_create_t();
typedef void op_destroy_t(OperatorConfiguratorInterface*);

#endif //PROJECT_OPERATORCONFIGURATORINTERFACE_H


