//================================================================================
// Name        : RESTHttpsServer.cpp
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : General HTTPS server implementation intended for RESTful APIs.
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

#include "RESTHttpsServer.h"

#include <string>

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/uuid/detail/sha1.hpp>

// This is the C++11 equivalent of a generic lambda.
// The function object is used to send an HTTP message.
template<class Stream>
struct send_lambda {
    Stream& stream_;
    bool& close_;
    boost::beast::error_code& ec_;

    explicit send_lambda(Stream& stream, bool& close, boost::beast::error_code& ec) :
            stream_(stream),
            close_(close),
            ec_(ec) {
    }

    template<bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields>&& msg) const {
        // Determine if we should close the connection after
        close_ = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::write(stream_, sr, ec_);
    }
};

RESTHttpsServer::RESTHttpsServer(serverSettings_t settings, boost::asio::io_context& io) :
    _retCode(0),    
    _io(io),
    _isRunning(false) {
    
    _ctx = std::shared_ptr<ssl::context>(new ssl::context(ssl::context::tls_server));

    _ctx->set_options(ssl::context::default_workarounds |
                      ssl::context::no_tlsv1 |
                      ssl::context::single_dh_use);

    // Password callback needs to be set before setting cert and key.
    /*
    _ctx->set_password_callback([](std::size_t max_length,
                                  ssl::context::password_purpose purpose)
    {
        return "password";
    });
    */

    try {
        _ctx->use_certificate_chain_file(settings.certificate);
        _ctx->use_private_key_file(settings.privateKey, ssl::context::pem);
    } catch (const std::exception& e) {
        ServerLOG(fatal) << "Could not load certificate OR private key settings file! "
                            "Please ensure the paths in the config file are valid!";
        std::runtime_error re("RESTAPI config error");
        throw re;
    }
		
    // 2048bit Diffie-Hellman parameters from RFC3526
    static unsigned char const s_dh2048_pem[] = { 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x42, 0x45, 0x47, 0x49, 0x4E, 0x20, 0x44, 0x48, 0x20, 0x50, 0x41, 0x52, 0x41, 0x4D, 0x45, 0x54, 0x45, 0x52, 0x53, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x0A, 0x4D, 0x49, 0x49, 0x42, 0x43, 0x41, 0x4B, 0x43, 0x41, 0x51, 0x45, 0x41, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x4A, 0x44, 0x39, 0x71, 0x69, 0x49, 0x57, 0x6A, 0x43, 0x4E, 0x4D, 0x54, 0x47, 0x59, 0x6F, 0x75, 0x41, 0x33, 0x42, 0x7A, 0x52, 0x4B, 0x51, 0x4A, 0x4F, 0x43, 0x49, 0x70, 0x6E, 0x7A, 0x48, 0x51, 0x43, 0x43, 0x37, 0x36, 0x6D, 0x4F, 0x78, 0x4F, 0x62, 0x0A, 0x49, 0x6C, 0x46, 0x4B, 0x43, 0x48, 0x6D, 0x4F, 0x4E, 0x41, 0x54, 0x64, 0x37, 0x35, 0x55, 0x5A, 0x73, 0x38, 0x30, 0x36, 0x51, 0x78, 0x73, 0x77, 0x4B, 0x77, 0x70, 0x74, 0x38, 0x6C, 0x38, 0x55, 0x4E, 0x30, 0x2F, 0x68, 0x4E, 0x57, 0x31, 0x74, 0x55, 0x63, 0x4A, 0x46, 0x35, 0x49, 0x57, 0x31, 0x64, 0x6D, 0x4A, 0x65, 0x66, 0x73, 0x62, 0x30, 0x54, 0x45, 0x4C, 0x70, 0x70, 0x6A, 0x66, 0x74, 0x0A, 0x61, 0x77, 0x76, 0x2F, 0x58, 0x4C, 0x62, 0x30, 0x42, 0x72, 0x66, 0x74, 0x37, 0x6A, 0x68, 0x72, 0x2B, 0x31, 0x71, 0x4A, 0x6E, 0x36, 0x57, 0x75, 0x6E, 0x79, 0x51, 0x52, 0x66, 0x45, 0x73, 0x66, 0x35, 0x6B, 0x6B, 0x6F, 0x5A, 0x6C, 0x48, 0x73, 0x35, 0x46, 0x73, 0x39, 0x77, 0x67, 0x42, 0x38, 0x75, 0x4B, 0x46, 0x6A, 0x76, 0x77, 0x57, 0x59, 0x32, 0x6B, 0x67, 0x32, 0x48, 0x46, 0x58, 0x54, 0x0A, 0x6D, 0x6D, 0x6B, 0x57, 0x50, 0x36, 0x6A, 0x39, 0x4A, 0x4D, 0x39, 0x66, 0x67, 0x32, 0x56, 0x64, 0x49, 0x39, 0x79, 0x6A, 0x72, 0x5A, 0x59, 0x63, 0x59, 0x76, 0x4E, 0x57, 0x49, 0x49, 0x56, 0x53, 0x75, 0x35, 0x37, 0x56, 0x4B, 0x51, 0x64, 0x77, 0x6C, 0x70, 0x5A, 0x74, 0x5A, 0x77, 0x77, 0x31, 0x54, 0x6B, 0x71, 0x38, 0x6D, 0x41, 0x54, 0x78, 0x64, 0x47, 0x77, 0x49, 0x79, 0x68, 0x67, 0x68, 0x0A, 0x66, 0x44, 0x4B, 0x51, 0x58, 0x6B, 0x59, 0x75, 0x4E, 0x73, 0x34, 0x37, 0x34, 0x35, 0x35, 0x33, 0x4C, 0x42, 0x67, 0x4F, 0x68, 0x67, 0x4F, 0x62, 0x4A, 0x34, 0x4F, 0x69, 0x37, 0x41, 0x65, 0x69, 0x6A, 0x37, 0x58, 0x46, 0x58, 0x66, 0x42, 0x76, 0x54, 0x46, 0x4C, 0x4A, 0x33, 0x69, 0x76, 0x4C, 0x39, 0x70, 0x56, 0x59, 0x46, 0x78, 0x67, 0x35, 0x6C, 0x55, 0x6C, 0x38, 0x36, 0x70, 0x56, 0x71, 0x0A, 0x35, 0x52, 0x58, 0x53, 0x4A, 0x68, 0x69, 0x59, 0x2B, 0x67, 0x55, 0x51, 0x46, 0x58, 0x4B, 0x4F, 0x57, 0x6F, 0x71, 0x73, 0x71, 0x6D, 0x6A, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x2F, 0x77, 0x49, 0x42, 0x41, 0x67, 0x3D, 0x3D, 0x0A, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x45, 0x4E, 0x44, 0x20, 0x44, 0x48, 0x20, 0x50, 0x41, 0x52, 0x41, 0x4D, 0x45, 0x54, 0x45, 0x52, 0x53, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D };
    _ctx->use_tmp_dh(boost::asio::buffer(s_dh2048_pem));
    
    try {
        // Resolving the hostname-port pair given in the configuration
        boost::asio::ip::tcp::resolver resolver(io);
        boost::system::error_code ec;
        boost::asio::ip::tcp::resolver::query query(settings.host, settings.port);
        boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query, ec);
        
        if(ec) {
            ServerLOG(fatal) << "RestAPI cannot resolve hostname " << settings.host << " with port " << settings.port << "!" << std::endl;
            throw std::runtime_error("RestAPI host resolution error");
        }
        
        // Instantiating one accept socket per resolved IP
        for (; iter != boost::asio::ip::tcp::resolver::iterator(); ++iter) {
            auto acceptor = std::shared_ptr<tcp::acceptor>(new tcp::acceptor(io, iter->endpoint()));
            acceptor->set_option(tcp::acceptor::reuse_address(true));
            _acceptors.push_back(acceptor);
        }
    } catch (const std::exception& e) {
        ServerLOG(fatal) << "RestAPI address invalid! Please make sure IP address and port are valid!";
        std::runtime_error re("RestAPI config error");
        throw re;
    }
}


void RESTHttpsServer::handle_session(tcp::socket& socket, ssl::context& ctx, size_t accIdx) {
    if(_isRunning) {
        ServerLOG(debug) << _remoteEndpoint.address().to_string() << ":" << _remoteEndpoint.port() << " connecting";
        // Launching a new async accept call
        startAccept(accIdx);
    } else {
        delete &socket;
        return;
    }
    
    bool close = false;
    boost::beast::error_code ec;
    
    // Construct the stream around the socket
    boost::beast::ssl_stream<tcp::socket&> stream{socket, ctx};
    
    // Perform the SSL handshake
    stream.handshake(ssl::stream_base::server, ec);
    if(ec) {
        ServerLOG(debug) << "handshake error: " << ec.message();
        goto serverError;
    }
    
    {//scope, so any goto before does not cross variable initialization
        // This buffer is required to persist across reads
        boost::beast::flat_buffer buffer;

        // This lambda is used to send messages
        send_lambda<boost::beast::ssl_stream<tcp::socket&>> lambda{stream, close, ec};
        
		while(true) {
			// Declare a parser for a request with a string body
			http::request_parser<http::string_body> parser;
			
			// Read the header
			read_header(stream, buffer, parser, ec);
			if(ec == http::error::end_of_stream) {
				break;
			} else if (ec) {
				ServerLOG(debug) << "read error (header): " << ec.message();
				goto serverError;
			}
			
			if(!validateUser(parser.get(), lambda)) {
				break;
			}
			
			// Check for the Expect field value
			if(parser.get()[http::field::expect] == "100-continue")
			{
				// send 100 response
				http::response<http::empty_body> res;
				res.version(11);
				res.result(http::status::continue_);
				write(stream, res, ec);
				if (ec) {
					ServerLOG(debug) << "read write (continue): " << ec.message();
					goto serverError;
				}
			}
			
			http::read(stream, buffer, parser, ec);
			if(ec == http::error::end_of_stream) {
				break;
			} else if(ec) {
				ServerLOG(debug) << "read error (body): " << ec.message();
				goto serverError;
			}

			// Send the response
			handle_request(parser.get(), lambda);
			
			if(ec) {
				ServerLOG(debug) << "write error: " << ec.message();
				goto serverError;
			}
			
			if(close) {
				// This means we should close the connection, usually because
				// the response indicated the "Connection: close" semantic.
				break;
				
			}
		}
	}

    // Perform the SSL shutdown
    stream.shutdown(ec);
    if(ec) { ServerLOG(debug) << "stream shutdown error: " << ec.message(); }

serverError:
    //For graceful closure of a connected socket we shut it down first although
    //this is not strictly necessary.
    //Fails if client already disconnected from socket.
    socket.shutdown(tcp::socket::shutdown_both, ec);
    if(ec) { ServerLOG(debug) << "socket shutdown: " << ec.message(); }

    socket.close(ec);
    if(ec) { ServerLOG(debug) << "socket close error: " << ec.message(); }
    
    // Getting rid of the allocated socket
    delete &socket;
}

template<class Body, class Send>
void RESTHttpsServer::handle_request(http::request<Body>& req, Send&& send) {

    http::response<http::string_body> res {http::status::internal_server_error, req.version()};
    res.set(http::field::server, SERVER_STRING);
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());
    res.body() = "Unknown error occurred\n";

    //split target and find matching endpoint handler
    queries_t queries;
    const std::string endpointName = splitUri(req.target().to_string(), queries);

    //Look up the endpoint
    try {
        apiEndpoint_t endpoint = _endpoints.at(endpointName);

        if (endpoint.first == req.method()) {
            //Everything matches --> call the endpoint function
            ServerLOG(debug) << req.method_string() << " " << endpointName << " requested";
            endpoint.second(req, res, queries);
        } else {
            const std::string msg = "Request method " + req.method_string().to_string() +
                                    " does not match endpoint " + endpointName + "\n";
            ServerLOG(debug) << msg;
            res.result(http::status::bad_request);
            res.body() = msg;
        }
    } catch (const std::out_of_range& e) {
        ServerLOG(debug) << "Requested endpoint " << endpointName << " not found";
        res.result(http::status::not_implemented);
        res.body() = "Invalid endpoint\n";
    }

    //ServerLOG(info) << "Responding:\n" << res.body();

    res.prepare_payload();
    send(std::move(res));
    return;
}

template<class Body, class Send>
bool RESTHttpsServer::validateUser(const http::request<Body>& req, Send&& send) {

    http::response<http::string_body> res {http::status::unauthorized, req.version()};
    res.set(http::field::server, SERVER_STRING);
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());
    res.body() = "Unauthorized access!\n";
    res.prepare_payload();

    //GET /help and /version do not need any authorization
    if ((req.method() == http::verb::get) && ((req.target() == "/help") || (req.target() == "/version"))) {
        return true;
    }

    std::string auth;
    std::string credentials;

    try {
        auth = req.base().at(http::field::authorization).to_string();
    } catch (const std::out_of_range& e) {
        ServerLOG(info) << "No credentials were provided";
        send(std::move(res));
        return false;
    }

    //Remove the substring "Basic" and decode the credentials.
    auth.erase(0,6);
    using namespace boost::archive::iterators;
    using ItBinaryT =
            transform_width<binary_from_base64<remove_whitespace<std::string::const_iterator>>, 8, 6>;

    try {
        // If the input isn't a multiple of 4, pad with =
        size_t num_pad_chars((4 - auth.size() % 4) % 4);
        auth.append(num_pad_chars, '=');

        size_t pad_chars(std::count(auth.begin(), auth.end(), '='));
        std::replace(auth.begin(), auth.end(), '=', 'A');
        std::string output(ItBinaryT(auth.begin()), ItBinaryT(auth.end()));
        output.erase(output.end() - pad_chars, output.end());
        credentials = output;
    } catch (std::exception const&) {
        credentials = std::string("");
    }

    size_t pos = credentials.find(':');
    const std::string usr = credentials.substr(0, pos);
    const std::string pwd = credentials.substr(pos+1, credentials.length());

    //Check credentials
    userAttributes_t userData;
    try {
        userData = _users.at(usr);
    } catch (const std::out_of_range& e) {
        ServerLOG(warning) << "User does not exist: " << usr;
        send(std::move(res));
        return false;
    }

	boost::uuids::detail::sha1 sha1;
	sha1.process_bytes(pwd.data(), pwd.size());
	unsigned hash[5] = {0};
	sha1.get_digest(hash);
	std::stringstream ss;
	for (int i = 0; i < 5; i++)	{
		ss << std::hex << std::setfill('0') << std::setw(8) << hash[i];
	}
	
    if (ss.str() != userData.first) {
        ServerLOG(warning) << "Invalid password provided for user " << usr;
        send(std::move(res));
        return false;
    }

    permission perm;

    switch (req.method()) {
        case http::verb::get:
            perm = permission::GET;
            break;
        case http::verb::put:
            perm = permission::PUT;
            break;
        case http::verb::post:
            perm = permission::POST;
            break;
        case http::verb::delete_:
            perm = permission::DELETE;
            break;
        default:
            perm = permission::NUM_PERMISSIONS;
            break;
    }

    try {
        if (!userData.second.test(perm)) {
            ServerLOG(warning) << "User " << usr << " has insufficient permissions";
            res.result(http::status::forbidden);
            res.body() = "Insufficient permissions\n";
            res.prepare_payload();
            send(std::move(res));
            return false;
        }
    } catch (const std::out_of_range& e) {
        ServerLOG(debug) << "Permission out of range (method not supported)";
        res.result(http::status::not_implemented);
        res.body() = "Request method not supported!\n";
        res.prepare_payload();
        send(std::move(res));
        return false;
    }

    return true;
}

std::string RESTHttpsServer::splitUri(const std::string& uri, queries_t& queries) {
    //split into path and query
    std::string path;
    std::string query;

    //ServerLOG(debug) << "Splitting URI " << uri;

    size_t pos = uri.find('?');
    path = uri.substr(0, pos);
    query = uri.substr(pos+1, uri.length());

    //split query part into the individual queries (key-value pairs)
    std::vector<std::string> queryStrs;
    std::stringstream stream(query);
    std::string part;

    while(std::getline(stream, part, ';')) {
        queryStrs.push_back(part);
    }

    for(auto& key : queryStrs) {
        size_t pos = key.find("=");
        if (pos != std::string::npos) {
            const std::string value = key.substr(pos+1);
            key.erase(pos);
            queries[key] = value;
        }
    }

    return path;
}

