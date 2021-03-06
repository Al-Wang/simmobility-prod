//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)


#include "ClientHandler.hpp"
#include "entities/commsim/broker/Broker.hpp"

using namespace sim_mob;

sim_mob::ClientHandler::ClientHandler(BrokerBase& broker, boost::shared_ptr<sim_mob::ConnectionHandler> conn,  const sim_mob::Agent* agent, std::string clientId) :
    broker(broker), valid(true), connHandle(conn), agent(agent), clientId(clientId),
    regisLocation(false), regisRegionPath(false), regisAllLocations(false)
{
    if (!conn) {
        throw std::runtime_error("Cannot create a client handler with a null connection handler.");
    }
}

sim_mob::ClientHandler::~ClientHandler()
{
}

void sim_mob::ClientHandler::setRequiredServices(const std::set<sim_mob::Services::SIM_MOB_SERVICE>& requiredServices)
{
    this->requiredServices = requiredServices;
}

const std::set<sim_mob::Services::SIM_MOB_SERVICE>& sim_mob::ClientHandler::getRequiredServices()
{
    return requiredServices;
}

bool sim_mob::ClientHandler::isValid() const
{
    return valid;
}

void sim_mob::ClientHandler::setValidation(bool value)
{
    valid = value;
}
