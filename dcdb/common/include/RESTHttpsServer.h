//================================================================================
// Name        : RESTHttpsServer.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : General HTTPS server intended for RESTful APIs.
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



/**
 * @defgroup server Https Server
 * @ingroup common
 *
 * @brief Definitions for a common HttpsServer base class.
 */

#ifndef COMMON_INCLUDE_RESTHTTPSSERVER_H_
#define COMMON_INCLUDE_RESTHTTPSSERVER_H_

#include <bitset>
#include <functional>
#include <thread>
#include <unordered_map>
#include <utility>

#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

#include "globalconfiguration.h"
#include "logging.h"

#define SERVER_STRING "RestAPIServer"

#define endpointArgs http::request<http::string_body>& req, http::response<http::string_body>& res, queries_t& queries

/******************************************************************************/

/**
 * @brief Enum to distinguish different permissions
 *
 * @ingroup server
 */
enum permission {
    GET     = 0, /**< Permission to make GET requests */
    PUT     = 1, /**< Permission to make PUT requests */
    POST    = 2, /**< Permission to make POST requests */
    DELETE  = 3, /**< Permission to make DELETE requests */
    NUM_PERMISSIONS = 4 /**< Number of permissions there are. */
};

namespace http = boost::beast::http;
namespace ssl  = boost::asio::ssl;
using     tcp  = boost::asio::ip::tcp;

/**
 * TODO how to document using directives to get them recognized by doxygen?
 */
using userAttributes_t = std::pair<std::string, std::bitset<NUM_PERMISSIONS>>;
using userBase_t       = std::unordered_map<std::string, userAttributes_t>;

using queries_t = std::unordered_map<std::string, std::string>;

using apiEndpointHandler_t = std::function<void(http::request<http::string_body>&, http::response<http::string_body>&, queries_t&)>;
using apiEndpoint_t        = std::pair<http::verb, apiEndpointHandler_t>;
using apiEndpoints_t       = std::unordered_map<std::string, apiEndpoint_t>;

/**
 * @ingroup server
 *
 * @brief Utility method for RestAPI implementations: retrieve the value
 *        for a key from queries.
 *
 * @param key       The key whose value should be retrieved.
 * @param queries   The queries to search.
 *
 * @return  The corresponding value or and empty string if the key was not
 *          found.
 */
inline std::string getQuery(const std::string& key, queries_t& queries) {
    try {
        return queries.at(key);
    } catch (const std::out_of_range&) {
        //fall through
    }
    return "";
}

/**
 * @ingroup server
 *
 * @brief Utility method for RestAPI implementations: test if a plugin query
 *        was given. Prepares the response accordingly to continue execution on
 *        success or immediately return on failure.
 *
 * @param plugin The plugin string retrieved from getQuery("plugin", ...).
 * @param res    The response to set appropriately.
 *
 * @return  True if a plugin query was given, false otherwise.
 */
inline bool hasPlugin(const std::string& plugin, http::response<http::string_body>& res) {
    if (plugin == "") {
        res.body() = "Request malformed: plugin query missing\n";
        res.result(http::status::bad_request);
        return false;
    } else {
        res.body() = "Plugin not found!\n";
        res.result(http::status::not_found);
        return true;
    }
}

/******************************************************************************/

/**
 * @ingroup server
 *
 * @brief General RESTful API https server leveraging Boost::Beast to provide
 *        common functionality for different REST API servers. Intended as
 *        parent class for concrete REST API implementations.
 *
 * @details Code partially copied from the boost beast synchronous https
 *          server example:
 * https://www.boost.org/doc/libs/develop/libs/beast/example/http/server/sync-ssl/http_server_sync_ssl.cpp
 * Example/Howto set up an endpoint in a derived class:
 * 1. Implement a handler function that obeys the required handler signature:
 *    @code
 *    void handle_get_example(http::response<http::string_body>& res, \
 *                            queries_t& queries) {
 *        //Implement here the logic for the endpoint taking into account the
 *        //queries as required.
 *        //TODO
 *        std::string result;
 *        if(queries.at("number") == "true") {
 *            result = "a number string\n";
 *        } else {
 *            result = "a string string\n";
 *        }
 *        //Set the response accordingly at the end. Result must have string
 *        //format!
 *        res.body() = res;
 *        res.result(http::status::ok);
 *    }
 *    @endcode
 * 2. Add a new endpoint by specifying its path, supported REST method and
 *    corresponding handler function:
 *    @code
 *    addEndpoint("/endpoints/example", {http::verb::get, \
 *                                       std::bind(handle_get_example, \
 *                                                 std::placeholders::_1, \
 *                                                 std::placeholders::_2)});
 *    @endcode
 * 3. Document your endpoints (expected queries etc.) somewhere.
 * 4. Enjoy.
 */
class RESTHttpsServer {

public:

    /**
     * @brief Start the server and listen to incoming requests.
     */
    void start() {
        if(_isRunning) {
            ServerLOG(warning) << "Request to start, but is already running";
            return;
        }
        ServerLOG(info) << "Starting...";
        for(size_t idx=0; idx<_acceptors.size(); idx++) {
            startAccept(idx);
        }

        _isRunning = true;
        ServerLOG(info) << "Started!";
    }

    /**
     * @brief Stop the server, i.e. stop accepting new request. Blocks until
     *        already accepted requests are finished.
     */
    void stop() {
        if(!_isRunning) {
            ServerLOG(warning) << "Request to stop, but is not running";
            return;
        }

        ServerLOG(info) << "Stopping...";
        _isRunning = false;
        for(auto &acc : _acceptors) {
            acc->cancel();
            acc->close();
        }
        
        ServerLOG(info) << "Stopped!";
    }

    /**
     * @brief Add a user to the list of known users. If an user with the same
     *        name already exists it will get overwritten.
     *
     * @param userName    Identifying name of the new user.
     * @param att         Attributes related to the new user.
     *
     * @return  True if an already existing user was overwritten, false
     *          otherwise.
     */
    bool addUser(const std::string& userName, const userAttributes_t& att) {
        bool ret = (_users.find(userName) != _users.end());
        _users[userName] = att;
        return ret;
    }

    /**
     * @brief Add an endpoint to the map of known endpoints. If an endpoint
     *        with the same path already exists it will get overwritten.
     *
     * @param path      Path of the new endpoint.
     * @param endpoint  Associated endpoint, i.e. supported method and endpoint
     *                  handler.
     *
     * @return  True if an already existing endpoint was overwritten, false
     *          otherwise.
     */
    bool addEndpoint(const std::string& path, const apiEndpoint_t& endpoint) {
        bool ret = (_endpoints.find(path) != _endpoints.end());
        _endpoints[path] = endpoint;
        return ret;
    }
    
    /**
     * @brief Returns the code to be used at exit by the entity in which the server is running.
     * 
     * @return A numerical return code. 
     */
    int getReturnCode() { return _retCode; }

protected:

    //This class should not be constructible on its own
    RESTHttpsServer(serverSettings_t settings, boost::asio::io_context& io);

    virtual ~RESTHttpsServer() {
        if(_isRunning)
            stop();
    }

    logger_t lg;
    int _retCode;

private:

    /**
     * @brief Starts asynchronous wait for a connection. Returns immediately.
     *        On a connection attempt handle_request() is called.
     */
    void startAccept(size_t idx) {
        // Start asynchronous wait until we get a connection.
        // On connection start launch the session, transferring ownership of the socket
        // This will receive the new connection
        auto newSocket = new tcp::socket(_io);
        _acceptors[idx]->async_accept(*newSocket, _remoteEndpoint,
                                      std::bind(&RESTHttpsServer::handle_session,
                                                this,
                                                std::ref(*newSocket),
                                                std::ref(*_ctx),
                                                idx));
    }
    
    /**
     * @brief Handles an HTTP server connection session.
     *
     * @details Reads incoming requests, validates their credentials and
     *          forwards them to handle_request().
     *
     * @param socket    Socket for the session
     * @param ctx       SSL context object
     * @param accIdx    Index of the acceptor associated to this session
     */
    void handle_session(tcp::socket& socket, ssl::context& ctx, size_t accIdx);

    /**
     * @brief Matches the request to the associated endpoint handler and sends a
     *        response for the given request.
     *
     * @details The type of the response object depends on the
     *          contents of the request, so the interface requires the
     *          caller to pass a generic lambda for receiving the response.
     *
     * @param req       Received request.
     * @param send      Lambda which is used to send the response.
     */
    template<class Body, class Send>
    void handle_request(http::request<Body>& req, Send&& send);

    /**
     * @brief Validates if correct user and password credentials were provided.
     *
     * @details Extracts and decodes the credentials from req and checks whether
     *          the user name is known and the password matches. If not, an
     *          appropriate "Unauthorized" response is sent and no further
     *          response from the caller is required. If the password is valid,
     *          it is further verified that the user has the required permission
     *          for the desired action. If the user is not permitted, a
     *          "Forbidden" response is sent and no further response from the
     *          caller is required.
     *
     * @param req  The request message for which to validate the user.
     * @param send Lambda which is used to send the response.
     *
     * @return    True if the user was successfully authenticated and has the
     *            required permission. False otherwise (i.e. a response has
     *            already been sent).
     */
    template<class Body, class Send>
    bool validateUser(const http::request<Body>& req, Send&& send);

    /**
     * @brief Split a relative URI into its components.
     *
     * @details Splits a URI of format /goto/location?arg1=test1;arg2=test2 into
     *          path:    "/goto/location"
     *          queries: [("arg1", "test1"), ("arg2", "test")]
     *
     * @param uri         The relative URI to split.
     * @param queries     Vector of string pairs, where pairs of query-identifier
     *                    and query-value are stored in.
     *
     * @return  The path part of the URI.
     */
    //TODO extend to support #fragments
    //FIXME remove once boost::beast includes a URI parser (https://github.com/boostorg/beast/issues/787)
    std::string splitUri(const std::string& uri, queries_t& queries);

    boost::asio::io_context& _io; /**< Central io_context for all I/O */
    std::shared_ptr<ssl::context> _ctx; /**< SSL context hold the certificates and is required for https support */
    std::vector<std::shared_ptr<tcp::acceptor>> _acceptors; /**< Acceptors receive incoming connections */

    tcp::endpoint _remoteEndpoint; /**< Used to store information about the connecting client endpoint */
    
    bool _isRunning; /**< Indicate whether the server is already running */

    apiEndpoints_t _endpoints;/**< Store all supported endpoints for later lookup*/
    userBase_t _users; /**< Storage for users and related attributes like permissions and password */
};

#endif /* COMMON_INCLUDE_RESTHTTPSSERVER_H_ */
