//================================================================================
// Name        : RestAPI.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : RESTful API for dcdb-pusher.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2019-2019 Leibniz Supercomputing Centre
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

#ifndef DCDBPUSHER_RESTAPI_H_
#define DCDBPUSHER_RESTAPI_H_

#include "RESTHttpsServer.h"

#include <boost/asio.hpp>

#include "../analytics/OperatorManager.h"
#include "MQTTPusher.h"
#include "PluginManager.h"
#include "mqttchecker.h"

/**
 * @brief Class providing a RESTful API to pusher via network (HTTPs only).
 *
 * @ingroup pusher
 */
class RestAPI : public RESTHttpsServer {
      public:
	RestAPI(serverSettings_t         settings,
		PluginManager *          pluginManager,
		MQTTPusher *             mqttPusher,
		OperatorManager *        manager,
		boost::asio::io_context  &io);

	virtual ~RestAPI() {}

	// String used as a response for the REST GET /help command
	const string restCheatSheet = "dcdbpusher RESTful API cheatsheet:\n"
				      " -GET:  /help               This help message.\n"
				      "        /analytics/help     An help message for data analytics commands.\n"
				      "        /plugins?[json]   D List of currently loaded plugins.\n"
				      "        /sensors?plugin;[json]\n"
				      "                          D List of currently running sensors which belong to\n"
				      "                            the specified plugin.\n"
				      "        /average?plugin;sensor;[interval]\n"
				      "                            Average of last sensor readings from the last\n"
				      "                            [interval] seconds or of all cached readings if no\n"
				      "                            interval is given.\n"
				      " -PUT:  /load?plugin;[path];[config]\n"
				      "                            Load a new plugin. Optionally specify path to the\n"
				      "                            shared library and/or the config file for the "
				      "                            plugin.\n"
				      "        /unload?plugin      Unload a plugin.\n"
				      "        /reload?plugin      Reload the plugin configuration.\n"
				      "        /quit?[code]        The pusher quits with the specified\n"
				      "                            return code.\n"
    				      " -POST: /start?plugin       Start the sensors of the plugin.\n"
				      "        /stop?plugin        Stop the sensors of the plugin.\n"
				      "\n";

      private:
	/**
     * GET "/help"
     *
     * @brief Return a cheatsheet of possible REST API endpoints.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional |  -      |        -             |      -
     */
	void GET_help(endpointArgs);

	/**
	 * GET "/version"
	 *
	 * @brief Return version number
	 *
	 * Queries  | key     | possible values      | explanation
	 * -------------------------------------------------------------------------
	 * Required |  -      |        -             |      -
	 * Optional |  -      |        -             |      -
	 */
	void GET_version(endpointArgs);
	
	/**
     * GET "/plugins"
     *
     * @brief List all loaded dcdbpusher plugins.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | json    | true                 | format response as json
     */
	void GET_plugins(endpointArgs);

	/**
     * GET "/sensors"
     *
     * @brief List all sensors of a specific plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all plugin names     | specify the plugin
     * Optional | json    | true                 | format response as json
     */
	void GET_sensors(endpointArgs);

	/**
     * GET "/average"
     *
     * @brief Get the average of the last readings of a sensor. Also allows
     *        access to analytics sensors.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all plugin names     | specify the plugin
     *          | sensor  | all sensor names of  | specify the sensor within the
     *          |         | the plugin or the    | plugin
     *          |         | analytics manager    |
     * Optional | interval| number of seconds    | use only readings more recent
     *          |         |                      | than (now - interval) for
     *          |         |                      | average calculation
     */
	void GET_average(endpointArgs);

	/******************************************************************************/

	/**
	 * PUT "/quit"
	 *
	 * @brief Quits the pusher with a certain return code.
	 *
	 * Queries  | key     | possible values      | explanation
	 * -------------------------------------------------------------------------
	 * Required |  -      |        -             |      -
	 * Optional | code    | integer value        |      return code to be used
	 */
	void PUT_quit(endpointArgs);

	/**
     * PUT "/load"
     *
     * @brief Load and initialize a plugin but do not start it yet.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | name of the new      | specify the plugin
     *          |         | plugin               |
     * Optional | path    | file path            | specify a file path where to
     *          |         |                      | search for the shared lib.
     *          |         |                      | Defaults to (usr/lib etc.)
     *          | config  | file path + name     | specify the config file for
     *          |         |                      | the plugin. Defaults to
     *          |         |                      | ./PLUGINNAME.conf
     */
	void PUT_load(endpointArgs);

	/**
     * PUT "/unload"
     *
     * @brief Unload a plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all plugin names     | specify the plugin
     * Optional |  -      |        -             |      -
     */
	void PUT_unload(endpointArgs);

	/**
     * POST "/start"
     *
     * @brief Start a plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all plugin names     | specify the plugin
     * Optional |  -      |        -             |      -
     */
	void POST_start(endpointArgs);

	/**
     * POST "/stop"
     *
     * @brief Stop a plugin.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all plugin names     | specify the plugin
     * Optional |  -      |        -             |      -
     */
	void POST_stop(endpointArgs);

	/**
     * PUT "/reload"
     *
     * @brief Reload a plugin's configuration (includes plugin restart).
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | all plugin names     | specify the plugin
     * Optional |  -      |        -             |      -
     */
	void PUT_reload(endpointArgs);

	/**
     * PUT "/analytics/reload"
     *
     * @brief Reload configuration and initialization of all or only a specific
     *        operator plugin.
     *
     * @detail Overwrites the method from the AnalyticsManager to ensure that
     *         MQTTPusher is stopped before reloading plugins.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | plugin  | all operator plugin  | reload only the specified
     *          |         | names                | plugin
     */
	void PUT_analytics_reload(endpointArgs);

	/**
     * PUT "/analytics/load"
     *
     * @brief Load and initialize an operator plugin but do not start it yet.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | plugin  | name of the new      | specify the plugin
     *          |         | plugin               |
     * Optional | path    | file path            | specify a file path where to
     *          |         |                      | search for the shared lib.
     *          |         |                      | Defaults to (usr/lib etc.)
     *          | config  | file path + name     | specify the config file for
     *          |         |                      | the plugin. Defaults to
     *          |         |                      | ./PLUGINNAME.conf
     *          
     */
	void PUT_analytics_load(endpointArgs);

	/**
     * PUT "/analytics/unload"
     *
     * @brief Unload a specific operator plugin.
     *
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | plugin  | all operator plugin  | unload only the specified
     *          |         | names                | plugin
     */
	void PUT_analytics_unload(endpointArgs);

	/**
     * PUT "/analytics/navigator"
     *
     * @brief Reloads the sensor navigator.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     */
	void PUT_analytics_navigator(endpointArgs);

	/******************************************************************************/

	// Utility method to reload the sensor navigator whenever needed
	bool reloadQueryEngine(const bool force = false);
	void unloadQueryEngine();

	PluginManager *          _pluginManager;
	MQTTPusher *             _mqttPusher;
	OperatorManager *        _manager;
};

#endif /* DCDBPUSHER_RESTAPI_H_ */
