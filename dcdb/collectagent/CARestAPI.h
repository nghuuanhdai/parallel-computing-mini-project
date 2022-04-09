//================================================================================
// Name        : CARestAPI.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : RESTful API for collectagent.
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

#ifndef COLLECTAGENT_CARESTAPI_H_
#define COLLECTAGENT_CARESTAPI_H_

#include "RESTHttpsServer.h"

#include "analyticscontroller.h"
#include "mqttchecker.h"
#include "configuration.h"
#include "simplemqttserver.h"
#include <dcdb/sensorconfig.h>
#include <signal.h>

/**
 * @brief Class providing a RESTful API to collect agent via network (HTTPs only).
 *
 * @ingroup ca
 */
class CARestAPI : public RESTHttpsServer {
public:
    CARestAPI(serverSettings_t settings,
	      influx_t* influxSettings,
              SensorCache* sensorCache, 
              SensorDataStore* sensorDataStore,
              SensorConfig *sensorConfig,
              AnalyticsController* analyticsController,
              SimpleMQTTServer* mqttServer,
              boost::asio::io_context& io);

    virtual ~CARestAPI() {}

    // String used as a response for the REST GET /help command
    const std::string caRestCheatSheet = "collectAgent RESTful API cheatsheet:\n"
                                         " -GET:  /help     This help message.\n"
                                         "        /analytics/help\n"
                                         "                  An help message for data analytics commands.\n"
                                         "        /hosts\n"
                                         "                  Prints the list of connected hosts.\n"
                                         "        /average?sensor;[interval]\n"
                                         "                  Average of last sensor readings from the last\n"
                                         "                  [interval] seconds or of all cached readings\n"
                                         "                  if no interval is given\n"
                                         " -PUT:  /quit?[code]\n"
                                         "                  The collectagent quits with the specified\n"
                                         "                  return code.\n"
                                         "\n";

    uint64_t getInfluxCounter() { uint64_t ctr=_influxCounter; _influxCounter=0; return ctr; };

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
     * @brief Return version number.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional |  -      |        -             |      -
     */
    void GET_version(endpointArgs);
    
    /**
     * GET "/hosts"
     *
     * @brief Returns a CSV list of connected hosts and their "last seen" timestamps.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional |  -      |        -             |      -
     */
    void GET_hosts(endpointArgs);

    /**
     * GET "/average"
     *
     * @brief Get the average of the last readings of a sensor.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | sensor  | all sensor names of  | specify the sensor within the
     *          |         | the plugin within the| plugin
     *          |         | sensor cache         |
     * Optional | interval| number of seconds    | use only readings more recent
     *          |         |                      | than (now - interval) for
     *          |         |                      | average calculation
     */
    void GET_average(endpointArgs);

    /**
     * PUT "/quit"
     *
     * @brief Quits the collectagent with a certain return code.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | code    | integer value        |      return code to be used
     */
    void PUT_quit(endpointArgs);

    void GET_ping(endpointArgs);
    void POST_query(endpointArgs);
    void POST_write(endpointArgs);

    /**
     * PUT "/analytics/reload"
     *
     * @brief Reload configuration and initialization of all or only a specific
     *        analytics plugin.
     *
     * @detail Overwrites the method from the OperatorManager to ensure that
     *         analyticsctonroller is stopped before reloading plugins.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional | plugin  | all analyzer plugin  | reload only the specified
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
     * Optional | plugin  | all analyzer plugin  | unload only the specified
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

    influx_t*             _influxSettings;
    SensorCache*          _sensorCache;
    SensorDataStore*      _sensorDataStore;
    SensorConfig*         _sensorConfig;
    AnalyticsController*  _analyticsController;
    SimpleMQTTServer*     _mqttServer;
    std::set<std::string> _influxSensors;
    uint64_t              _influxCounter;
};

#endif /* COLLECTAGENT_CARESTAPI_H_ */
