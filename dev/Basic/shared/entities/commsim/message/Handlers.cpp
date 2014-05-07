//Copyright (c) 2014 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Handlers.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>

#include "entities/Agent.hpp"
#include "entities/commsim/broker/Broker.hpp"
#include "entities/commsim/connection/WhoAreYouProtocol.hpp"
#include "entities/commsim/message/Messages.hpp"
#include "entities/commsim/client/ClientHandler.hpp"
#include "entities/commsim/serialization/CommsimSerializer.hpp"
#include "event/args/ReRouteEventArgs.hpp"
#include "message/MessageBus.hpp"
#include "logging/Log.hpp"



sim_mob::HandlerLookup::HandlerLookup() 
{
	//Register all known handlers.
	defaultHandlerMap["id_ack"] = new sim_mob::NullHandler();
	defaultHandlerMap["reroute_request"] = new sim_mob::RerouteRequestHandler();
	defaultHandlerMap["remote_log"] = new sim_mob::RemoteLogHandler();
	defaultHandlerMap["tcp_connect"] = new sim_mob::TcpConnectHandler();
	defaultHandlerMap["tcp_disconnect"] = new sim_mob::TcpDisconnectHandler();

	//These should always be handled by the Broker
	defaultHandlerMap["new_client"] = new sim_mob::BrokerErrorHandler();
	defaultHandlerMap["ticked_client"] = new sim_mob::BrokerErrorHandler();
	defaultHandlerMap["id_response"] = new sim_mob::BrokerErrorHandler();

	//Help avoid common errors with old message types.
	defaultHandlerMap["MULTICAST"] = new sim_mob::ObsoleteHandler();
	defaultHandlerMap["UNICAST"] = new sim_mob::ObsoleteHandler();
}

sim_mob::HandlerLookup::~HandlerLookup() 
{
	//Reclaim handlers
	for (std::map<std::string, const sim_mob::Handler*>::const_iterator it=defaultHandlerMap.begin(); it!=defaultHandlerMap.end(); it++) {
		delete it->second;
	}
	for (std::map<std::string, const sim_mob::Handler*>::const_iterator it=customHandlerMap.begin(); it!=customHandlerMap.end(); it++) {
		delete it->second;
	}
	defaultHandlerMap.clear();
	customHandlerMap.clear();
}


const sim_mob::Handler* sim_mob::HandlerLookup::getHandler(const std::string& msgType) const
{
	std::map<std::string, const sim_mob::Handler*>::const_iterator it = customHandlerMap.find(msgType);
	if (it!=customHandlerMap.end()) {
		return it->second;
	}
	it = defaultHandlerMap.find(msgType);
	if (it!=defaultHandlerMap.end()) {
		return it->second;
	}

	throw std::runtime_error("Unknown handler for message type.");
}

void sim_mob::HandlerLookup::addHandlerOverride(const std::string& mType, const Handler* handler)
{
	if (mType=="MULTICAST" || mType=="UNICAST") {
		throw std::runtime_error("Cannot register a custom handler for MULTICAST/UNICAST messages; they are obsolete (use OPAQUE_SEND/RECIEVE instead).");
	}
	if (customHandlerMap.find(mType) != customHandlerMap.end()) {
		throw std::runtime_error("Custom handler type has already been registered.");
	}
	customHandlerMap[mType] = handler;
}


void sim_mob::RemoteLogHandler::handle(boost::shared_ptr<ConnectionHandler> handler, const MessageConglomerate& messages, int msgNumber, BrokerBase* broker) const
{
	//Ask the serializer for a RemoteLog message.
	RemoteLogMessage rmMsg = CommsimSerializer::parseRemoteLog(messages, msgNumber);

	//Now we can just log it.
	//At the moment, we are so many levels removed from Broker that we'll just put it on stdout.
	//Ideally, it would (eventually) go into out.txt.
	Print() <<"Client [" <<messages.getSenderId() <<"] relayed remote log message: \"" <<rmMsg.logMessage <<"\"\n";
}

void sim_mob::RerouteRequestHandler::handle(boost::shared_ptr<ConnectionHandler> handler, const MessageConglomerate& messages, int msgNumber, BrokerBase* broker) const
{
	//Ask the serializer for a RerouteRequest message.
	RerouteRequestMessage rmMsg = CommsimSerializer::parseRerouteRequest(messages, msgNumber);

	//Does this client exist?
	boost::shared_ptr<sim_mob::ClientHandler> agentHandle = broker->getAndroidClientHandler(messages.getSenderId());
	if(!agentHandle) {
		WarnOut("RerouteRequest can't find Agent (self)." << std::endl);
		return;
	}

	//Double-check Agent validity.
	if(!agentHandle->agent) {
		WarnOut("RerouteRequest found invalid Agent; may have completed route already." << std::endl);
		return;
	}

	//Now dispatch through the MessageBus.
	sim_mob::messaging::MessageBus::PublishEvent(sim_mob::event::EVT_CORE_COMMSIM_REROUTING_REQUEST,
		agentHandle->agent, sim_mob::messaging::MessageBus::EventArgsPtr(new sim_mob::event::ReRouteEventArgs(rmMsg.blacklistRegion))
	);
}


void sim_mob::TcpConnectHandler::handle(boost::shared_ptr<ConnectionHandler> handler, const MessageConglomerate& messages, int msgNumber, BrokerBase* broker) const
{
	//Ask the serializer for a TcpConnectMessage message.
	TcpConnectMessage rmMsg = CommsimSerializer::parseTcpConnect(messages, msgNumber);

	broker->cloudConnect(handler, rmMsg.host, rmMsg.port);
}

void sim_mob::TcpDisconnectHandler::handle(boost::shared_ptr<ConnectionHandler> handler, const MessageConglomerate& messages, int msgNumber, BrokerBase* broker) const
{
	//Ask the serializer for a TcpConnectMessage message.
	TcpConnectMessage rmMsg = CommsimSerializer::parseTcpConnect(messages, msgNumber);

	broker->cloudDisconnect(handler, rmMsg.host, rmMsg.port);
}





//handler implementation
//this handler handles the multicast requests sent by android
//the hadler does this by finding the sender's nearby agents
//please take note that in this implementation:
//1-multicast is treated same as broadcast
//2-although it is like a broadcast, simmobility will add the specific receiver
//  information while redirecting to NS3(as opposed to letting NS3 find the recipients)
void sim_mob::OpaqueSendHandler::handle(boost::shared_ptr<ConnectionHandler> handler, const MessageConglomerate& messages, int msgNumber, BrokerBase* broker) const
{
	OpaqueSendMessage sendMsg = CommsimSerializer::parseOpaqueSend(messages, msgNumber);

	//Retrieve the sending client/agent, making sure that it is still valid.
	boost::shared_ptr<sim_mob::ClientHandler> clnHandler = broker->getAndroidClientHandler(messages.getSenderId());
	if (!(clnHandler && clnHandler->isValid() && clnHandler->agent)) {
		Print() << "Invalid agent record" << std::endl;
		return;
	}

	const Agent* sendAgent = sendAgent = clnHandler->agent;
	//If "broadcast" is set, we have to build up a list of receiving agents.
	if (sendMsg.broadcast) {
		//Get agents around you.
		std::vector<const Agent*> nearAgents = AuraManager::instance().agentsInRect(
			Point2D((sendAgent->xPos - 3500), (sendAgent->yPos - 3500)),
			Point2D((sendAgent->xPos + 3500), (sendAgent->yPos + 3500)),
			sendAgent
		);

		//Add all IDs in this list to "toIds" in the message.
		for (std::vector<const Agent*>::const_iterator it=nearAgents.begin(); it!=nearAgents.end(); it++) {
			if ((*it) != sendAgent) {//Skip yourself.
				sendMsg.toIds.push_back(boost::lexical_cast<std::string>((*it)->getId()));
			}
		}
	}


	//We need to further synthesize the toIds field, removing any messages to cloud recipients.
	//It's unlikely that we'll have messages to both Android agents and the cloud in the same OpaqueSend,
	// but there's no reason not to support it.
	broker->opaqueSendCloud(handler, sendMsg, useNs3);


	//If there's no agents left, return.
	if(sendMsg.toIds.empty()) {
		return;
	}


	//Behavior differs based on ns-3 (either dispatch messages now, or route them all through ns-3).
	if (useNs3) {
		//step-5: insert messages into send buffer
		//NOTE: This part only exists for ns-3+android.
		boost::shared_ptr<sim_mob::ClientHandler> ns3Handle = broker->getNs3ClientHandler();
		if (!ns3Handle) {
			throw std::runtime_error("Expected ns-3 client handler; returned null.");
		}

		//Serialize the message, send it.
		std::string msg = CommsimSerializer::makeOpaqueSend(sendMsg.fromId, sendMsg.toIds, sendMsg.broadcast, sendMsg.data);
		broker->insertSendBuffer(ns3Handle, msg);
	} else {
		//Iterate through all registered clients
		const ClientList::Type& allClients = broker->getAndroidClientList();
		for (ClientList::Type::const_iterator clientIt=allClients.begin(); clientIt!=allClients.end(); clientIt++) {
			//Get the agent associated to this client
			boost::shared_ptr<sim_mob::ClientHandler> destClientHandlr  = clientIt->second;
			const sim_mob::Agent* agent = destClientHandlr->agent;
			std::string agentId = boost::lexical_cast<std::string>(agent->getId());

			//See if the agent associated with this handle is in our list of recipients.
			if(std::find(sendMsg.toIds.begin(), sendMsg.toIds.end(), agentId) == sendMsg.toIds.end()) {
				continue;
			}

			//step-4: fabricate a message for each(core  is taken from the original message)
			//actually, you don't need to modify any field in the original jsoncpp's Json::Value message.
			//just add the recipients directly request to send
			std::string msg = CommsimSerializer::makeOpaqueReceive(sendMsg.fromId, agentId, sendMsg.format, sendMsg.data);
			broker->insertSendBuffer(boost::shared_ptr<ClientHandler>(destClientHandlr), msg);
		}
	}

}


//you are going to handle something like this:
void sim_mob::OpaqueReceiveHandler::handle(boost::shared_ptr<ConnectionHandler> handler, const MessageConglomerate& messages, int msgNumber, BrokerBase* broker) const
{
	if (!useNs3) {
		throw std::runtime_error("NS-3 handler called when ns-3 was disabled.");
	}

	OpaqueReceiveMessage recMsg = CommsimSerializer::parseOpaqueReceive(messages, msgNumber);
	handleDirect(recMsg, broker);
}


void sim_mob::OpaqueReceiveHandler::handleDirect(const OpaqueReceiveMessage& recMsg, BrokerBase* broker) const
{
	//Get the client handler for this recipient.
	boost::shared_ptr<sim_mob::ClientHandler> receiveAgentHandle;

	//find the client destination client_handler
	const ClientList::Type& all_clients = broker->getAndroidClientList();
	for (ClientList::Type::const_iterator clientIt=all_clients.begin(); clientIt!=all_clients.end(); clientIt++) {
		boost::shared_ptr<sim_mob::ClientHandler> clnHandler = clientIt->second;
		//valid agent, matching ID
		if (clnHandler->agent && clnHandler->clientId == recMsg.toId) {
			receiveAgentHandle = clnHandler;
			break;
		}
	}

	//insert into sending buffer
	if (receiveAgentHandle && receiveAgentHandle->connHandle) {
		broker->insertSendBuffer(receiveAgentHandle, CommsimSerializer::makeOpaqueReceive(recMsg.fromId, recMsg.toId, recMsg.format, recMsg.data));
	} else {
		Warn() <<"Could not find a receive (cloud) handler for agent with ID: " <<recMsg.toId <<"\n";
	}
}



