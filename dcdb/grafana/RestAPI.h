//================================================================================
// Name        : RestAPI.h
// Author      : Daniele Tafani
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : RESTful API for the Grafana server.
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

#ifndef GRAFANA_RESTAPI_H_
#define GRAFANA_RESTAPI_H_

#include "RESTHttpsServer.h"
#include "Configuration.h"
#include "sensornavigator.h"
#include "cassandra.h"
#include "mqttchecker.h"
#include "metadatastore.h"

#include "dcdb/connection.h"
#include "dcdb/timestamp.h"
#include "dcdb/sensorid.h"
#include "dcdb/sensordatastore.h"
#include "dcdb/sensorconfig.h"
#include "dcdb/sensor.h"

#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <atomic>

#define MAX_DATAPOINTS 100000

/**
 * @brief Class providing a RESTful API to Grafana via network (HTTPs only).
 *
 * @ingroup grafana
 */
class RestAPI : public RESTHttpsServer {
    
public:
    RestAPI(serverSettings_t settings,
            hierarchySettings_t hierarchySettings,
            DCDB::Connection* cassandraConnection,
            boost::asio::io_context &io);
    
    virtual ~RestAPI() {
        if(_sensorConfig) {
            delete _sensorConfig;
            _sensorConfig = NULL;
        }
        if(_sensorDataStore) {
            delete _sensorDataStore;
            _sensorDataStore = NULL;
        }
    }

    //Builds the internal sensor navigator
    bool buildTree();
    void checkPublishedSensorsAsync();

private:
    
/******************************************************************************/
    /**
     * GET "/"
     *
     * @brief Dummy handler to perform server side checks for the creation of the Grafana DCDB data source.
     *        Currently, all necessary checks are performed by the REST API server directly
     *        (e.g., user credentials, connectivity to the DB,...).
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |   -     | -                    | -
     * Optional |   -     | -                    | -
     */
    void GET_datasource(endpointArgs);

/******************************************************************************/

    /**
     * POST "/levels"
     *
     * @brief Returns the maximum number of hierarchy levels in the sensor navigator.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required | -       | -                    | -
     * Optional | -       | -                    | -
     */
    void POST_levels(endpointArgs);

    /**
     * POST "/search"
     *
     * @brief Returns the list of metrics that can be queried at a specific level.
     *
     * Queries  | key     | possible values       | explanation
     * -------------------------------------------------------------------------------
     * Required | -       | -                     | -
     * Optional | node    | all nodes in the      | specify the parent node of the
                |           sensor tree           | sensor tree.
     *          | sensors | true                  | allows to return all sensors at
                |         |                       | the selected node level.
     */
    void POST_search(endpointArgs);

    /**
     * POST "/query"
     *
     * @brief Performs a query given a list of sensors and a time range. The Grafana plugin
     *        automatically encapsulates all the required infor for performing the query in
     *        the body of the request in a JSON format. To issue a similar request from
     *        command line, see the documentation (there is no real good reason to do it...).
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     * Optional |  -      |        -             |      -
     */
    void POST_query(endpointArgs);

    /**
     * PUT "/navigator"
     *
     * @brief Reloads the sensor navigator.
     *
     * Queries  | key     | possible values      | explanation
     * -------------------------------------------------------------------------
     * Required |  -      |        -             |      -
     */
    void PUT_navigator(endpointArgs);

/******************************************************************************/

    //Clears all internal storage
    void clear();
    
    //Quick and dirty utility methods
    inline int64_t sign(int64_t val) { return val < 0 ? -1 : 1; }
    inline int64_t abs(int64_t val) { return val < 0 ? -val : val; }

    std::shared_ptr<SensorNavigator> _navigator;
    std::shared_ptr<MetadataStore> _metadataStore;
    DCDB::Connection*   _connection;
    DCDB::SensorConfig* _sensorConfig;
    DCDB::SensorDataStore* _sensorDataStore;
    atomic<bool> _updating;
    boost::asio::deadline_timer _timer;
    uint64_t _publishedSensorsWritetime;
    
    std::string _separator;
    hierarchySettings_t _hierarchySettings;
    boost::regex _smootherRegex;
    boost::regex _numRegex;
    boost::cmatch _match;
    
};

#endif /* GRAFANA_RESTAPI_H_ */
