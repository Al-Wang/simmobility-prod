//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * ConnectionServer.hpp
 *
 *  Created on: May 29, 2013
 *      Author: vahid
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <queue>


namespace sim_mob {
class BrokerBase;
class ConnectionHandler;
class CloudHandler;

/**
 * This class serves two purposes: first, it spins waiting for new client connections on a given port
 *   (efficiently, using async_accept). Second, it contains our only io_service object and a thread pool
 *   used to run() io_service tasks.
 * Once a connection is accepted, a ConnectionHandler class is spun off to deal with the details of
 *   reading and writing client information (also using async I/O), and the ConnectionServer proceeds to
 *   wait for the next connection.
 */
class ConnectionServer {
public:
    ConnectionServer(BrokerBase& broker, unsigned short port = DEFAULT_SERVER_PORT);
    ~ConnectionServer();

    ///Starts the server's accept() loop.
    ///\param numThreads The number of threads to assign to the accept() loop.
    void start(unsigned int numThreads);

    ///Specifically request a connection to the cloud. This will be handled asynchronously.
    ///Returns the CloudHandler; NOTE that you cannot use this until handle_cloud_connect() informs you it is ready.
    ///\param host The host address of this cloud server (e.g., 127.0.0.1).
    ///\param port The port to connect on (e.g., 50000)
    ///\returns A CloudHandler representing this connection.
    boost::shared_ptr<CloudHandler> connectToCloud(const std::string& host, int port);

private:
    void handle_accept(boost::shared_ptr<ConnectionHandler> conn, const boost::system::error_code& e);

    void handle_cloud_connect(boost::shared_ptr<CloudHandler> conn, const boost::system::error_code& e);

private:
    ///This is used to loop accepting connections.
    void creatSocketAndAccept();

    //Our listen port (if you change this, you will also have to change the apps and the relay).
    const static unsigned int DEFAULT_SERVER_PORT = 6745;

    //List of Connections that this ConnectionServer knows about.
    std::vector< boost::shared_ptr<ConnectionHandler> > knownConnections;
    boost::mutex knownConnectionsMUTEX;

    //List of Cloud connections that this ConnectionServer knows about.
    std::vector< boost::shared_ptr<CloudHandler> > knownCloudConnections;
    boost::mutex knownCloudConnectionsMUTEX;

    //The io_service is used by Boost to multiplex all I/O operations. There should generally only be one of these.
    boost::asio::io_service io_service;
    boost::thread_group threads; //These provide "workers" for the io_service.

    //The acceptor is used by this class to wait on incoming connections on a given port.
    boost::asio::ip::tcp::acceptor acceptor;

    //A reference back to the broker, for various callback-related tasks.
    sim_mob::BrokerBase& broker;
};

}

