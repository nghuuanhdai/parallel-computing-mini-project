//================================================================================
// Name        : OperatorManager.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Management class for the data analytics framework.
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

/**
 * @defgroup analytics Analytics Framework
 *
 * @brief Framework for data analytics at various points within DCDB.
 */

#ifndef PROJECT_OPERATORMANAGER_H
#define PROJECT_OPERATORMANAGER_H

#include <set>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <dlfcn.h>
#include <exception>
#include "logging.h"
#include "RESTHttpsServer.h"
#include "mqttchecker.h"
#include "includes/UnitInterface.h"
#include "includes/OperatorConfiguratorInterface.h"

using namespace std;

/**
 * Struct of values required to load operator dynamic libraries.
 *
 * @ingroup analytics
 */
typedef struct {
    std::string id;
    void*	DL;
    OperatorConfiguratorInterface* configurator;
    op_create_t*  create;
    op_destroy_t* destroy;
} op_dl_t;

typedef std::vector<op_dl_t> op_pluginVector_t;

/**
 * @brief Management class for the entire data analytics framework
 *
 * @details Allows to load, configure, start and generally manage plugins
 *          developed with the data analytics framework.
 *
 * @ingroup analytics
 */
class OperatorManager {

public:

    enum managerState_t { CLEAR = 1, LOADED = 2};

    /**
    * @brief            Class constructor
    * @param io         Boost ASIO service to be used
    */
    OperatorManager(boost::asio::io_context& io) : _io(io) { _status = CLEAR; }

    /**
    * @brief            Class destructor
    */
    ~OperatorManager() { clear(); }

    /**
    * @brief            Resets the state of the data analytics framework
    *
    *                   All currently running operators will be stopped, and the related objects destroyed.
    */
    void clear();

    /**
    * @brief                Probes a configuration file to determine if initialization is required
    *
    *                       This method will read through the specified configuration file, and search for an
    *                       operatorPlugin block, with its associated data analytics plugin. If the method returns
    *                       true, then one or more plugins were requested for initialization.
    *
    * @param path           Path to the global and plugin configuration files
    * @param globalFile     Name of the global file (usually global.conf or collectagent.conf)
    * @return               true if a configuration is necessary, false otherwise
    */
    bool probe(const string& path, const string& globalFile);

    /**
    * @brief                Loads plugins as specified in the input config file
    *
    *                       This method will load .dll libraries containing data analytics plugins. It will then
    *                       configure them, according to the respective configuration files, and store the generated
    *                       OperatorInterface objects, ready to be started.
    *
    * @param path           Path to the global and plugin configuration files
    * @param globalFile     Name of the global file (usually global.conf or collectagent.conf)
    * @param pluginSettings Structure containing global plugin settings
    * @return               true if successful, false otherwise
    */
    bool load(const string& path, const string& globalFile, const pluginSettings_t& pluginSettings);

    /**
    * @brief            Initialize one or more stored plugins
    *
    *                   This method must be called after "load", and before "start". It will prepare operators for
    *                   operation, and initialize the related sensors and caches.
    *
    * @param plugin     Name of the plugin on which the action must be performed. If none, all plugins will be affected
    * @return           true if successful, false otherwise
    */
    bool init(const string& plugin="");

    /**
    * @brief            Reload one or more plugins
    *
    *                   This method will cause all running operators of a plugin to be stopped and destroyed. The
    *                   configuration file is then read once again, and new operators are created and initialized.
    *
    * @param plugin     Name of the plugin on which the action must be performed. If none, all plugins will be affected
    * @return           true if successful, false otherwise
    */
    bool reload(const string& plugin="");

    /**
    * @brief            Start one or more stored plugins
    *
    *                   This method must be called after "init". It will start up all operators stored in a plugin,
    *                   which will then perform computation autonomously according to their sampling rates.
    *
    * @param plugin     Name of the plugin on which the action must be performed. If none, all plugins will be affected
    * @param operatorN  Name of the operator in the specified plugin affected by the action
    * @return           true if successful, false otherwise
    */
    bool start(const string& plugin="", const string& operatorN="");

    /**
    * @brief            Stops one or more stored plugins
    *
    *                   This method must be called after "start". It will stop all operators of a plugin that are
    *                   currently running, and halt their computation. The operators can be re-started again later
    *                   with the "start" method.
    *
    * @param plugin     Name of the plugin on which the action must be performed. If none, all plugins will be affected
    * @param operatorN  Name of the operator in the specified plugin affected by the action
    * @return           true if successful, false otherwise
    */
    bool stop(const string& plugin="", const string& operatorN="");

    /**
     * @brief             Loads a plugin dynamically
     * 
     * @param name        Name of the plugin
     * @param pluginPath  Path to the plugin library
     * @param config      Path to the configuration file
     * @return            True if successful, false otherwise
     */
    bool loadPlugin(const string& name, const string& pluginPath, const string& config);

    /**
     * @brief             Unloads a currently loaded plugin
     * 
     * @param id          Name of the plugin to be unloaded
     */
    void unloadPlugin(const string& id);

    /**
    * @brief            Get the vector of currently loaded plugins
    *
    *                   The vector can then be used to access single operators within plugins, the related units, and
    *                   finally all output sensors.
    *
    * @return           Vector of plugins represented as op_dl_t structures
    */
    std::vector<op_dl_t>& getPlugins() { return _plugins; }

    /**
     * @brief   Get the current status of the OperatorManager.
     *
     * @return  Status of the OperatorManager.
     */
    managerState_t getStatus() { return _status; }

    /**
     * @brief   Adds analytics RestAPI endpoints to an RestAPI server.
     *
     * @param restServer    The RestAPI server which should offer analytics
     *                      endpoints.
     */
    void addRestEndpoints(RESTHttpsServer* restServer);

    // String used as a response for the REST GET /help command
    const string restCheatSheet = "DCDB Analytics RESTful API cheatsheet:\n"
                                    "(All commands must be prepended by \"/analytics\" !)\n"
                                    " -GET: /plugins?[json]   D List off currently loaded plugins.\n"
                                    "       /sensors?plugin;[operator];[json]\n"
                                    "                         D List of currently running sensors which belong to\n"
                                    "                           the specified plugin (and operator).\n"
                                    "       /operators?plugin;[json]\n"
                                    "                         D List of running operators in the specified data\n"
                                    "                           analytics plugin.\n"
                                    "       /units?plugin;[operator];[json]\n"
                                    "                         D List of units to which sensors are associated in\n"
                                    "                           the specified data analytics plugin (and operator).\n"
                                    " -PUT  /reload?[plugin]    Reload all or only a specific analytics plugin.\n"
                                    "       /load?plugin;[path];[config]\n"
                                    "                           Load a new plugin. Optionally specify path to the\n"
                                    "                           shared library and/or the config file for the \n"
                                    "                           plugin.\n"
                                    "       /unload?plugin      Unload a plugin.\n"
                                    "       /compute?plugin;operator;[unit];[json]\n"
                                    "                           Query the specified operator for a unit. Default\n"
                                    "                           unit is the root.\n"
                                    "       /operator?plugin;action;[operator]\n"
                                    "                           Do a custom operator action for all or only an\n"
                                    "                           selected operator within a plugin (refer to plugin\n"
                                    "                           documentation).\n"
                                    "       /navigator          Reloads the sensor navigator.\n"
                                    " -POST:/start?[plugin];[operator]\n"
                                    "                           Start all or only a specific analytics plugin or\n"
                                    "                           start only a specific operator within a plugin.\n"
                                    "       /stop?[plugin];[operator]\n"
                                    "                           Stop all or only a specific analytics plugin or\n"
                                    "                           stop only a specific operator within a plugin.\n"
                                    "\n"
                                    "D = Discovery method\n"
                                    "All resources have to be prepended by host:port.\n"
                                    "A query can be appended as ?query=[value] at the end. Multiple queries\n"
                                    "need to be separated by semicolons(';'). \"query=value\" syntax was shortened\n"
                                    "to \"query\" for readability. Optional queries are marked with [ ]\n";

protected:

    // Utility method to drop all topics associated to a certain plugin
    void removeTopics(op_dl_t p);
    // Utility method to check all MQTT topics within a certain plugin
    bool checkTopics(op_dl_t p);

    // Vector of plugins represented as op_dl_t structures
    std::vector<op_dl_t> _plugins;
    // Path used to load config files
    string _configPath;
    // Structure containing global plugin settings
    pluginSettings_t _pluginSettings;
    // Keeps track of the manager's state
    managerState_t _status;

    //Logger object
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;

private:
    // all stuff related to REST API

    // Utility method to check the status of the operator manager. If not
    // loaded: prepares the response accordingly so no further actions are
    // required.
    // Return true if loaded, false otherwise.
    inline bool managerLoaded(http::response<http::string_body>& res) {
        if (_status != LOADED) {
            const std::string err = "OperatorManager is not loaded!\n";
            RESTAPILOG(error) << err;
            res.body() = err;
            res.result(http::status::internal_server_error);
            return false;
        }
        return true;
    }

    // methods for REST API endpoints
    /**
     * GET "/analytics/help"
     *
     * @brief Return a cheatsheet of available REST API endpoints specific for
     *        the operator manager.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional |  -      |        -             |      -
     */
    void GET_analytics_help(endpointArgs);

    /**
     * GET "/analytics/plugins"
     *
     * @brief (Discovery) List all currently loaded data analytic plugins.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | json    | true                 | format response as json
     */
    void GET_analytics_plugins(endpointArgs);

    /**
     * GET "/analytics/sensors"
     *
     * @brief (Discovery) List all sensors of a plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all operator plugin  | specify the plugin
     *          |         | names                |
     * Optional | operator| all operators of a   | restrict sensors list to an
     *          |         | plugin               | operator
     *          | json    | true                 | format response as json
     */
    void GET_analytics_sensors(endpointArgs);

    /**
     * GET "/analytics/units"
     *
     * @brief (Discovery) List all units of a plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all operator plugin  | specify the plugin
     *          |         | names                |
     * Optional | operator| all operators of a   | restrict unit list to an
     *          |         | plugin               | operator
     *          | json    | true                 | format response as json
     */
    void GET_analytics_units(endpointArgs);

    /**
     * GET "/analytics/operators"
     *
     * @brief (Discovery) List all active operators of a plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all operator plugin  | specify the plugin
     *          |         | names                |
     * Optional | json    | true                 | format response as json
     */
    void GET_analytics_operators(endpointArgs);

    /**
     * POST "/analytics/start"
     *
     * @brief Start all or only a specific plugin. Or only start a specific
     *        streaming operator within a specific plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | plugin  | all operator plugin  | only start the specified
     *          |         | names                | plugin
     *          | operator| all operators of a   | only start the specified
     *          |         | plugin               | operator. Requires a plugin
     *          |         |                      | to be specified. Limited to
     *          |         |                      | streaming operators.
     */
    void POST_analytics_start(endpointArgs);

    /**
     * POST "/analytics/stop"
     *
     * @brief Stop all or only a specific plugin. Or only stop a specific
     *        streaming operator within a plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | plugin  | all operator plugin  | only stop the specified
     *          |         | names                | plugin
     *          | operator| all operators of a   | only stop the specified
     *          |         | plugin               | operator. Requires a plugin
     *          |         |                      | to be specified. Limited to
     *          |         |                      | streaming operators.
     */
    void POST_analytics_stop(endpointArgs);

    /**
     * This endpoint must either be overwritten (by adding a custom
     * "analytics/reload" endpoint) or must not be used. A reload requires
     * an external io_service object and can therefore not be conducted by the
     * AnalyticsManager itself.
     *
     * PUT "/analytics/reload"
     *
     * @brief Reload configuration and initialization of all or only a specific
     *        analytics plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | plugin  | all operator plugin  | reload only the specified
     *          |         | names                | plugin
     */
    void PUT_analytics_reload(endpointArgs);

    /**
     * PUT "/analytics/compute"
     *
     * @brief Query the given operator for a certain input unit. Intended for
     *        "on-demand" operators, but works with "streaming" operators as
     *        well.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all operator plugin  | select plugin
     *          |         | names                |
     *          | operator| all operators of a   | select operator
     *          |         | plugin               |
     * Optional | unit    | all units of a plugin| select target unit
     *          | json    | true                 | format response as json
     */
    void PUT_analytics_compute(endpointArgs);

    /**
     * PUT "/analytics/operator"
     *
     * @brief Perform a custom REST PUT action defined at operator level.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all operator plugin  | select plugin
     *          |         | names                |
     *          | action  | see operator         | select custom action
     *          |         | documentation        |
     *          | custom action may require more queries!
     * Optional | operator| all operators of a   | select operator
     *          |         | plugin               |
     *          | custom action may allow for more queries!
     */
    void PUT_analytics_operator(endpointArgs);

    boost::asio::io_context& _io;
};

#endif //PROJECT_ANALYTICSMANAGER_H
