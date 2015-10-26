//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Broker.hpp"

#include <sstream>
#include <boost/assign/list_of.hpp>
#include <json/json.h>

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "config/ST_Config.hpp"
#include "workers/Worker.hpp"

#include "entities/commsim/connection/ConnectionHandler.hpp"
#include "entities/commsim/connection/CloudHandler.hpp"
#include "entities/commsim/connection/WhoAreYouProtocol.hpp"
#include "entities/commsim/client/ClientHandler.hpp"
#include "entities/commsim/serialization/Base64Escape.hpp"

#include "entities/commsim/wait/WaitForAndroidConnection.hpp"
#include "entities/commsim/wait/WaitForNS3Connection.hpp"

#include "event/SystemEvents.hpp"
#include "event/args/EventArgs.hpp"
#include "event/EventPublisher.hpp"
#include "message/MessageBus.hpp"
#include "entities/profile/ProfileBuilder.hpp"

#include "geospatial/RoadRunnerRegion.hpp"

namespace {
//TEMPORARY: I just need an easy way to disable output for now. This is *not* the ideal solution.
const bool EnableDebugOutput = false;
} //End unnamed namespace


using namespace sim_mob;

BrokerBase* sim_mob::Broker::single_broker(nullptr);

const std::string sim_mob::Broker::ClientTypeAndroid = "android";
const std::string sim_mob::Broker::ClientTypeNs3 = "ns-3";

const std::string sim_mob::Broker::OpaqueFormatBase64Esc = "base64escape";

const std::string sim_mob::Broker::OpaqueTechDsrc = "dsrc";
const std::string sim_mob::Broker::OpaqueTechLte = "lte";

const unsigned int sim_mob::Broker::EventNewAndroidClient = 95000;


sim_mob::Broker::Broker(const MutexStrategy& mtxStrat, int id) :
		Agent(mtxStrat, id), numAgents(0), connection(*this)
{
	//Various Initializations
	configure();

	//Note: This should only be done once, and never in parallel. This is *probably* safe to do here, but change it if you
	//      switch to multiple Brokers.
	Base64Escape::Init();
}

sim_mob::Broker::~Broker()
{
}

bool sim_mob::Broker::insertSendBuffer(boost::shared_ptr<sim_mob::ClientHandler> client,  const std::string& message)
{
	if(!(client && client->connHandle && client->isValid() && client->connHandle->isValid())) {
		return false;
	}

	///TODO: It is not clear that this is needed; basically, the Broker tells the Publishers to publish all events,
	///      and the publishers call this method. (It may be called from other places that do require a lock, but it's
	///      possible these are all thread-local). We need to re-examine the Publisher model for this class; it seems like
	///      a lot of unnecessary extra work.
	boost::unique_lock<boost::mutex> lock(mutex_send_buffer);

	//Is this the first message received for this ClientHandler/destID pair?
	if (sendBuffer.find(client)==sendBuffer.end()) {
		OngoingSerialization& ongoing = sendBuffer[client];
		CommsimSerializer::serialize_begin(ongoing, client->clientId);
	}

	//Now just add it.
	CommsimSerializer::addGeneric(sendBuffer[client], message);
	return true;
}

void sim_mob::Broker::configure()
{
	//NOTE: I am fairly sure we don't need a context here, since the handler never checks it.
	sim_mob::Worker::GetUpdatePublisher().subscribe(sim_mob::event::EVT_CORE_AGENT_UPDATED, 
		this, &Broker::onAgentUpdate
	);

	//Hook up Android emulators to OnClientRegister
	registrationPublisher.registerEvent(EventNewAndroidClient);
	registrationPublisher.subscribe(EventNewAndroidClient, this, &Broker::onAndroidClientRegister);

	//Hook up the NS-3 simulator to OnClientRegister.
	//registrationPublisher.registerEvent(comm::NS3_SIMULATOR);

	//Dispatch differently depending on whether we are using "android-ns3" or "android-only"
    bool useNs3 = ST_Config::getInstance().commsim.useNs3;

	//Register handlers with "useNs3" flag. OpaqueReceive will throw an exception if it attempts to process a message and useNs3 is not set.
	handleLookup.addHandlerOverride("opaque_send", new sim_mob::OpaqueSendHandler(useNs3));
	handleLookup.addHandlerOverride("opaque_receive", new sim_mob::OpaqueReceiveHandler(useNs3));

	//We always wait for MIN_CLIENTS Android emulators and MIN_CLIENTS Agents (and optionally, 1 ns-3 client).
    waitAndroidBlocker.reset(ST_Config::getInstance().commsim.minClients);
	waitNs3Blocker.reset(useNs3?1:0);
}



void sim_mob::Broker::onNewConnection(boost::shared_ptr<ConnectionHandler> cnnHandler)
{
	//Save this connection.
	saveConnByToken(cnnHandler->getToken(), cnnHandler);

	//Ask the client to identify itself.
	WhoAreYouProtocol::QueryAgentAsync(cnnHandler);
}


void sim_mob::Broker::onNewCloudConnection(boost::shared_ptr<CloudHandler> cnnHandler)
{
	//Save it, signal.
	{
	boost::unique_lock<boost::mutex> lock(mutex_verified_cloud_connections);
	verifiedCloudConnections.insert(cnnHandler);
	}
	COND_VAR_VERIFIED_CLOUD_CONNECTIONS.notify_all();
}



//NOTE: Be careful; this function can be called multiple times by different "threads". Make sure you are locking where required.
void sim_mob::Broker::onMessageReceived(boost::shared_ptr<ConnectionHandler> cnnHandler, const char* msg, unsigned int len)
{
	//If the stream has closed, fail. (Otherwise, the simulation will freeze.)
	if(len==0){
		throw std::runtime_error("Empty packet; canceling simulation.");
	}

	//Let the CommsimSerializer handle the heavy lifting.
	BundleHeader header = BundleParser::read_bundle_header(std::string(msg, 8));
	MessageConglomerate conglom;
	if (!CommsimSerializer::deserialize(header, std::string(msg+8, len-8), conglom)) { //TODO: We could pass the pointer in here.
		throw std::runtime_error("Broker couldn't parse packet.");
	}


	//We have to introspect a little bit, in order to find our CLIENT_MESSAGES_DONE and WHOAMI messages.
	bool res = false;
	for (int i=0; i<conglom.getCount(); i++) {
		MessageBase mb = conglom.getBaseMessage(i);
		if (mb.msg_type == "ticked_client") {
			boost::unique_lock<boost::mutex> lock(mutex_clientDone);
			{
			boost::unique_lock<boost::mutex> lock(mutex_client_done_chk);
			std::map<boost::shared_ptr<sim_mob::ConnectionHandler>, ConnClientStatus>::iterator chkIt = clientDoneChecklist.find(cnnHandler);
			if (chkIt==clientDoneChecklist.end()) {
				throw std::runtime_error("Unexpected client/connection mapping.");
			}
			chkIt->second.done++;
			} //mutex_client_done_chk unlocks

			if (EnableDebugOutput) {
				Print() << "connection [" <<&(*cnnHandler) << "] DONE\n";
			}

			COND_VAR_CLIENT_DONE.notify_one();
		} else if (mb.msg_type == "new_client") {
			//Send out an immediate message query; don't pend the outgoing message.
			WhoAreYouProtocol::QueryAgentAsync(cnnHandler);
		} else if (mb.msg_type == "id_response") {
			IdResponseMessage msg = CommsimSerializer::parseIdResponse(conglom, i);

			//What type is this?
			if (!(msg.type==Broker::ClientTypeAndroid || msg.type==Broker::ClientTypeNs3)) {
				throw std::runtime_error("Client type is unknown; cannot re-assign.");
			}

			//Since we now have an "id_response" AND a valid ConnectionHandler, we can pend a registration request.
			sim_mob::ClientRegistrationRequest candidate;
			candidate.clientID = msg.id;
			candidate.client_type = msg.type;
			for (size_t i=0; i<msg.services.size(); i++) {
				candidate.requiredServices.insert(Services::GetServiceType(msg.services[i]));
			}

			//Retrieve the connection associated with this token.
			boost::shared_ptr<ConnectionHandler> connHandle;
			{
				boost::unique_lock<boost::mutex> lock(mutex_token_lookup);
				std::map<std::string, boost::shared_ptr<sim_mob::ConnectionHandler> >::const_iterator connHan = tokenConnectionLookup.find(msg.token);
				if (connHan == tokenConnectionLookup.end()) {
					throw std::runtime_error("Unknown token; can't receive id_response.");
				}
				connHandle = connHan->second;
			}
			if (!connHandle) {
				throw std::runtime_error("id_response received, but no clients are in the waiting list.");
			}

			//At this point we need to check the Connection's clientType and set it if it is UNKNOWN.
			//If it is known, make sure it's the expected type.
			if (connHandle->getSupportedType().empty()) {
				connHandle->setSupportedType(candidate.client_type);
			} else {
				if (connHandle->getSupportedType() != candidate.client_type) {
					throw std::runtime_error("ConnectionHandler received a message for a clientType it did not expect.");
				}
			}

			//Now, wait on it.
			insertClientWaitingList(candidate.client_type, candidate, connHandle);
		}
	}

	//New messages to process
	receiveQueue.push(MessageElement(cnnHandler, conglom));
}



void sim_mob::Broker::onEvent(event::EventId eventId, sim_mob::event::Context ctxId, event::EventPublisher* sender, const event::EventArgs& args)
{
	switch (eventId) {
		case sim_mob::event::EVT_CORE_AGENT_DIED: {
			const event::AgentLifeCycleEventArgs& args_ = MSG_CAST(event::AgentLifeCycleEventArgs, args);
			unRegisterEntity(args_.GetAgent());
			break;
		}
	}
}

size_t sim_mob::Broker::getRegisteredAgentsSize() const
{
	return registeredAgents.size();
}


std::map<const Agent*, AgentInfo>& sim_mob::Broker::getRegisteredAgents()
{
	return registeredAgents;
}


const ClientList::Type& sim_mob::Broker::getAndroidClientList() const
{
	return registeredAndroidClients;
}


boost::shared_ptr<sim_mob::ClientHandler> sim_mob::Broker::getAndroidClientHandler(std::string clientId) const
{
	ClientList::Type::const_iterator it=registeredAndroidClients.find(clientId);
	if (it!=registeredAndroidClients.end()) {
		return it->second;
	}

	Warn() <<"Client " << clientId << " not found\n";
	return boost::shared_ptr<sim_mob::ClientHandler>();
}

boost::shared_ptr<sim_mob::ClientHandler> sim_mob::Broker::getNs3ClientHandler() const
{
	if (registeredNs3Clients.size() == 1) {
		return registeredNs3Clients.begin()->second;
	}
	Warn() <<"Ns-3 client not found; count: " <<registeredNs3Clients.size() <<"\n";
	return boost::shared_ptr<sim_mob::ClientHandler>();
}

void sim_mob::Broker::insertClientList(const std::string& clientID, const std::string& cType, boost::shared_ptr<sim_mob::ClientHandler>& clientHandler)
{
	//Sanity check.
	if (clientID=="0") {
		throw std::runtime_error("Cannot add a client with ID 0.");
	}

	//Put it in the right place
	if (cType==ClientTypeAndroid) {
		ClientList::Type::const_iterator it = registeredAndroidClients.find(clientID);
		if (it!=registeredAndroidClients.end()) {
			throw std::runtime_error("Duplicate client ID requested.");
		}
		registeredAndroidClients[clientID] = clientHandler;
	} else if (cType==ClientTypeNs3) {
		if (!registeredNs3Clients.empty()) {
			throw std::runtime_error("Unable to insert into ns-3 client list; multiple ns-3 clients not supported.");
		}
		registeredNs3Clients[clientID] = clientHandler;
	} else {
		throw std::runtime_error("Unable to insert into client list; unknown type.");
	}

	if (EnableDebugOutput) {
		Print() << "connection [" <<&(*clientHandler->connHandle) << "] +1 client\n";
	}

	//+1 client for this connection.
	boost::unique_lock<boost::mutex> lock(mutex_client_done_chk);
	clientDoneChecklist[clientHandler->connHandle].total++;
	numAgents++;

	//publish an event to inform- interested parties- of the registration of a new android client
	if (cType==Broker::ClientTypeAndroid) {
		registrationPublisher.publish(Broker::EventNewAndroidClient, ClientRegistrationEventArgs(clientHandler));
	}
}

void sim_mob::Broker::saveConnByToken(const std::string& token, boost::shared_ptr<sim_mob::ConnectionHandler> newConn)
{
	boost::unique_lock<boost::mutex> lock(mutex_token_lookup);
	if (tokenConnectionLookup.find(token) != tokenConnectionLookup.end()) {
		Warn() <<"Overwriting known connection; possible token duplication: " <<token <<"\n";
	}
	tokenConnectionLookup[token] = newConn;
}


void  sim_mob::Broker::insertClientWaitingList(std::string clientType, ClientRegistrationRequest request, boost::shared_ptr<sim_mob::ConnectionHandler> existingConn)
{
	//TODO: The clientType will have to come from somewhere else (e.g., ONLY in the WHOAMI message).
	{
	boost::unique_lock<boost::mutex> lock(mutex_client_wait_list);
	if (clientType=="android") {
		clientWaitListAndroid.push(ClientWaiting(request,existingConn));
	} else if (clientType=="ns-3") {
		clientWaitListNs3.push(ClientWaiting(request,existingConn));
	} else {
		throw std::runtime_error("Can't insert client into waiting list: unknown type.");
	}
	}
	COND_VAR_CLIENT_REQUEST.notify_one();
}


void sim_mob::Broker::processAgentRegistrationRequests()
{
	boost::unique_lock<boost::mutex> lock(mutex_pre_register_agents);
	for (std::map<const Agent*, AgentInfo>::const_iterator it=preRegisterAgents.begin(); it!=preRegisterAgents.end(); it++) {
		registeredAgents[it->first] = it->second;
	}
	preRegisterAgents.clear();
}


void sim_mob::Broker::scanAndProcessWaitList(std::queue<ClientWaiting>& waitList, bool isNs3)
{
	for (;;) {
		//Retrieve the next ClientWaiting request; minimal locking. Does NOT remove the element (it might not handle() properly).
		ClientWaiting item;
		{
		boost::unique_lock<boost::mutex> lock(mutex_client_wait_list);
		if (waitList.empty()) {
			break;
		}
		item = waitList.front();
		}

		//Try to handle it. If this succeeds, remove it (it's guaranteed to be at the front of the queue).
		if(registrationHandler.handle(*this, item.request, item.existingConn, isNs3)) { //true=ns3
			boost::unique_lock<boost::mutex> lock(mutex_client_wait_list);
			waitList.pop();
		} else {
			//Assume that if the first Android client can't match then none will be able to.
			break;
		}
	}
}


void sim_mob::Broker::processClientRegistrationRequests()
{
	//NOTE: Earlier comments suggest that Android clients *must* be processed first, so I have preserved this order.
	scanAndProcessWaitList(clientWaitListAndroid, false);

	//Now repeat for ns-3.
	scanAndProcessWaitList(clientWaitListNs3, true);
}

void sim_mob::Broker::registerEntity(sim_mob::Agent* agent)
{
	boost::unique_lock<boost::mutex> lock(mutex_pre_register_agents);
	preRegisterAgents[agent] = AgentInfo();

	//Register to receive a callback when this entity is removed.
	sim_mob::messaging::MessageBus::SubscribeEvent(sim_mob::event::EVT_CORE_AGENT_DIED, agent, this);

	//Wake up any threads waiting on this.
	COND_VAR_CLIENT_REQUEST.notify_all();
}


void sim_mob::Broker::unRegisterEntity(sim_mob::Agent* agent)
{
	if (EnableDebugOutput) {
		Print() << "inside Broker::unRegisterEntity for agent: \"" << agent << "\"\n";
	}

	//Erase from the registered agents list.
	if (registeredAgents.erase(agent) == 0) {
		//If not found, it might be in the pre-registered list.
		boost::unique_lock<boost::mutex> lock(mutex_pre_register_agents);
		preRegisterAgents.erase(agent);
	}

	{
	boost::unique_lock<boost::mutex> lock(mutex_clientList);

	//Sanity check: We can't remove the ns-3 simulator right now.
	if (!registeredNs3Clients.empty() && agent==registeredNs3Clients.begin()->second->agent) {
		throw std::runtime_error("Ns-3 simulator cannot be un-registered.");
	}

	//TODO: Perhaps we need a lookup by agent pointer too?
	for (ClientList::Type::iterator it=registeredAndroidClients.begin(); it!=registeredAndroidClients.end(); it++) {
		if(agent == it->second->agent) {
			//unsubscribe from all publishers he is subscribed to
			sim_mob::ClientHandler* clientHandler = it->second.get();

			//Note: We only set validation to false, rather than actually removing the ClientHandler.
			//      I *think* the reason we do this is because the Agent may send one last message, then tell its Worker
			//      that it is "done", thus triggering this asynchronous message. However, we update the connection count
			//      below. I think we need to double-check our ClientHandler lifecycle, maybe using a "preRemove" list, just
			//      like we have a "preRegister". ~Seth
			it->second->agent = nullptr;
			it->second->setValidation(false);

			//Update the connection count too.
			boost::unique_lock<boost::mutex> lock(mutex_client_done_chk);
			std::map<boost::shared_ptr<sim_mob::ConnectionHandler>, ConnClientStatus>::iterator chkIt = clientDoneChecklist.find(it->second->connHandle);
			if (chkIt==clientDoneChecklist.end()) {
				throw std::runtime_error("Client somehow registered without a valid connection handler.");
			}
			chkIt->second.total--;
			numAgents--;
			if (chkIt->second.total==0) {
				it->second->connHandle->invalidate(); //this is even more important
			}
		}
	}
	}
}


void sim_mob::Broker::cloudConnect(boost::shared_ptr<ConnectionHandler> handler, const std::string& host, int port)
{
	//Give this connection a unique Id, "host:port"
	std::string cloudId ;//= host + ":" + boost::lexical_cast<std::string>(port);

	//We only need 1 connection to the cloud, but for efficiency's sake we open 1 connection per Android connectionHandler.
	boost::shared_ptr<CloudHandler> cloud = getCloudHandler(cloudId, handler);
	if (!cloud) {
		//We don't have a connection; open a new one.
		boost::unique_lock<boost::mutex> lock(mutex_cloud_connections);
		cloudConnections[cloudId][handler] = connection.connectToCloud(host, port);
	}
}

void sim_mob::Broker::cloudDisconnect(boost::shared_ptr<ConnectionHandler> handler, const std::string& host, int port)
{
	//Note: At the moment it is not necessary (or even beneficial) to close cloud connections. Leave this function
	//      here as a no-op, since we may want to track open connections at some point.
}


void sim_mob::Broker::sendToCloud(boost::shared_ptr<ConnectionHandler> conn, const std::string& cloudId, const OpaqueSendMessage& msg, bool useNs3)
{
	//Prepare a message for response.
	MessageBase temp;
	temp.msg_type = "opaque_receive";
	OpaqueReceiveMessage newMsg(temp); //TODO: This is a hackish workaround.
	newMsg.format = msg.format;
	newMsg.tech = msg.tech;

	//Reverse the sender/receiver.
	newMsg.fromId = cloudId;
	newMsg.toId = msg.fromId;

	//Before waiting for the cloud, decode the actual message, and split it at every newline.
	if (msg.format != OpaqueFormatBase64Esc) {
		throw std::runtime_error("Unknown message format sending to cloud; can't decode.");
	}
	std::vector<std::string> msgLines;
	Base64Escape::Decode(msgLines, msg.data, '\n');

	//Get the cloud handler.
	boost::shared_ptr<CloudHandler> cloud = getCloudHandler(cloudId, conn);
	if (!cloud) {
		throw std::runtime_error("Could not find cloud handler for given cloudId.");
	}

	//Now, wait for the client to connect.
	{
	boost::unique_lock<boost::mutex> lock(mutex_verified_cloud_connections);
	while (verifiedCloudConnections.count(cloud)==0) {
		COND_VAR_VERIFIED_CLOUD_CONNECTIONS.wait(lock);
	}
	}

	//We now have a valid cloud connection and a set of messages to send. This must be done in a single transaction:
	// [write,write,..,write] => [read,read,..,read]
	//This will incur some delay due to false synchronization; however, it is by far the simplest solution. Recall that most
	// RoadRunner agents will connect, send, and receive in the *same* update tick, so this approach is unfortunately necessary for now.
	std::vector<std::string> resLines = cloud->writeLinesReadLines(msgLines);

	//Re-encode the response using our base64-encoding.
	newMsg.data = Base64Escape::Encode(resLines, '\n');

	//Now, either route this through ns-3 or send it directly to the client.
	if (useNs3) {
		throw std::runtime_error("Can't send cloud messages through ns-3; at the moment ns-3 is only configured for Wi-Fi.");
	} else {
		dynamic_cast<const OpaqueReceiveHandler*>(handleLookup.getHandler(newMsg.msg_type))->handleDirect(newMsg, this);
	}
}

boost::shared_ptr<CloudHandler> sim_mob::Broker::getCloudHandler(const std::string& id, boost::shared_ptr<ConnectionHandler> conn)
{
	boost::unique_lock<boost::mutex> lock(mutex_cloud_connections);
	std::map<std::string, CloudConnections>::const_iterator it=cloudConnections.find(id);
	if (it!=cloudConnections.end()) {
		CloudConnections::const_iterator it2=it->second.find(conn);
		if (it2!=it->second.end()) {
			return it2->second;
		}
	}
	return boost::shared_ptr<CloudHandler>();
}


void sim_mob::Broker::processIncomingData(timeslice now)
{
	//just pop off the message queue and click handle ;)
	MessageElement msgTuple;
	while (receiveQueue.pop(msgTuple)) {
		//Conglomerates contain whole swaths of messages themselves.
		for (int i=0; i<msgTuple.conglom.getCount(); i++) {
			MessageBase mb = msgTuple.conglom.getBaseMessage(i);

			//Certain message types have already been handled.
			if (mb.msg_type=="ticked_client" || mb.msg_type=="id_response" || mb.msg_type=="new_client") {
				continue;
			}

			//Get the handler, let it parse its own expected message type.
			const sim_mob::Handler* handler = handleLookup.getHandler(mb.msg_type);
			if (handler) {
				handler->handle(msgTuple.cnnHandler, msgTuple.conglom, i, this);
			} else {
				std::cout <<"no handler for type \"" <<mb.msg_type << "\"\n";
			}
		}
	}
}

bool sim_mob::Broker::frame_init(timeslice now)
{
	return true;
}

Entity::UpdateStatus sim_mob::Broker::frame_tick(timeslice now)
{
	return Entity::UpdateStatus::Continue;
}



void sim_mob::Broker::agentUpdated(const Agent* target )
{
	//The Agent is either located in the preRegister list or the reigstered list. In the case of the former,
	//all we have to do is change its "done" flag; we don't have to signal anything (it will be copied over before that anyway).
	{
	boost::unique_lock<boost::mutex> lock(mutex_pre_register_agents);
	std::map<const Agent*, AgentInfo>::iterator it = preRegisterAgents.find(target);
	if (it!=preRegisterAgents.end()) {
		it->second.done = true;
		return;
	}
	}

	//If it's in the registered list, we need to also signal the conditional variable; the Broker might be waiting for this.
	std::map<const Agent*, AgentInfo>::iterator it = registeredAgents.find(target);
	if (it==registeredAgents.end()) {
		Warn() <<"Warning! Agent: " <<target->getId() <<" couldn't be found in the registered OR pre-registered agents list.\n";
		return;
	}

	//Update, signal the condition variable.
	{
	boost::unique_lock<boost::mutex> lock(mutex_agentDone);
	it->second.done = true;
	}
	COND_VAR_AGENT_DONE.notify_all();
}

void sim_mob::Broker::onAgentUpdate(sim_mob::event::EventId id, sim_mob::event::Context context, sim_mob::event::EventPublisher* sender, const UpdateEventArgs& argums)
{
	const Agent* target = dynamic_cast<const Agent*>(argums.GetEntity());
	agentUpdated(target);
}


void sim_mob::Broker::onAndroidClientRegister(sim_mob::event::EventId id, sim_mob::event::Context context, sim_mob::event::EventPublisher* sender, const ClientRegistrationEventArgs& argums)
{
	boost::shared_ptr<ClientHandler>clientHandler = argums.client;

	//If we are operating on android-ns3 set up, each android client registration should be brought to ns3's attention
    if (ST_Config::getInstance().commsim.useNs3) {
		//note: based on the current implementation of
		// the ns3 client registration handler, informing the
		//statistics of android clients IS a part of ns3
		//registration and configuration process. So the following implementation
		//should be executed for those android clients who will join AFTER
		//ns3's registration. So we check if the ns3 is already registered or not:
		boost::shared_ptr<sim_mob::ClientHandler> nsHand = getNs3ClientHandler();
		if (!nsHand) {
			std::cout <<"ERROR: Android client registered without a known ns-3 handler (and one was expected).\n";
			return;
		}

		//Pend this information for later.
		boost::unique_lock<boost::mutex> lock(mutex_new_agents_message);
		new_agents_message.push_back(clientHandler->agent->getId());
	}

	//Enable Region support if this client requested it.
	if(clientHandler->getRequiredServices().find(sim_mob::Services::SIMMOB_SRV_REGIONS_AND_PATH) != clientHandler->getRequiredServices().end()){
		boost::unique_lock<boost::mutex> lock(mutex_clientList);
		newClientsWaitingOnRegionEnabling.insert(clientHandler);
	}
}



//todo: again this function is also intrusive to Broker. find a way to move the following switch cases outside the broker-vahid
//todo suggestion: for publishment, don't iterate through the list of clients, rather, iterate the publishers list, access their subscriber list and say publish and publish for their subscribers(keep the clientlist for MHing only)
void sim_mob::Broker::processPublishers(timeslice now)
{
	//Create a single Time message.
	//std::string timeMsg = CommsimSerializer::makeTimeData(now.frame(), ConfigManager::GetInstance().FullConfig().baseGranMS());

	//Create a single AllLocations message.
	std::map<unsigned int, Point> allLocs;
	for (std::map<const Agent*, AgentInfo>::const_iterator it=registeredAgents.begin(); it!=registeredAgents.end(); it++) {
		allLocs[it->first->getId()] = Point(it->first->xPos.get(), it->first->yPos.get());
	}
	std::string allLocMsg = CommsimSerializer::makeAllLocations(allLocs);

	//Process all clients for messages.
	for (ClientList::Type::const_iterator it=registeredAndroidClients.begin(); it!=registeredAndroidClients.end(); it++) {
		//Skip dead Agents.
		const boost::shared_ptr<sim_mob::ClientHandler>& cHandler = it->second;
		if(!(cHandler && cHandler->agent && cHandler->isValid())) {
			continue;
		}

		//Publish whatever this agent requests.
		if (cHandler->regisLocation) {
			//Attempt to reverse-project the Agent's (x,y) location into Lat/Lng, if such a projection is possible.
			LatLngLocation loc;
			CoordinateTransform* trans = 0;//ConfigManager::GetInstance().FullConfig().getNetwork().getCoordTransform(false);
			if (trans) {
				loc = trans->transform(Point(cHandler->agent->xPos.get(), cHandler->agent->yPos.get()));
			}

			insertSendBuffer(cHandler, CommsimSerializer::makeLocation(cHandler->agent->xPos.get(), cHandler->agent->yPos.get(), loc));
		}
		if (cHandler->regisRegionPath) {
			if (cHandler->agent->getRegionSupportStruct().isEnabled()) {
				//NOTE: Const-cast is unfortunately necessary. We could also make the Region tracking data mutable.
				//      We can't push a message back to the Broker, since it has to arrive in the same time tick
				//      (the Broker uses a half-time-tick mechanism).
				std::vector<sim_mob::RoadRunnerRegion> all_regions = const_cast<Agent*>(cHandler->agent)->getAndClearNewAllRegionsSet();
				std::vector<sim_mob::RoadRunnerRegion> reg_path = const_cast<Agent*>(cHandler->agent)->getAndClearNewRegionPath();
				if (!(all_regions.empty() && reg_path.empty())) {
					insertSendBuffer(cHandler, CommsimSerializer::makeRegionsAndPath(all_regions, reg_path));
				}
			}
		}
	}

	//"All locations" only applies to the ns-3 client.
	if (registeredNs3Clients.size() == 1) {
		boost::shared_ptr<sim_mob::ClientHandler> ns3Handler = registeredNs3Clients.begin()->second;
		if (ns3Handler->regisAllLocations) {
			insertSendBuffer(ns3Handler, allLocMsg);
		}

		//Create a single "new agents" message, if appropriate.
		std::string newAgentsMessage;
		{
		boost::unique_lock<boost::mutex> lock(mutex_new_agents_message);
		if (!new_agents_message.empty()) {
			newAgentsMessage = CommsimSerializer::makeNewAgents(new_agents_message, std::vector<unsigned int>());
			new_agents_message.clear();
		}
		}
		if (!newAgentsMessage.empty()) {
			//Add it.
			insertSendBuffer(ns3Handler, newAgentsMessage);
		}
	}
}

void sim_mob::Broker::sendReadyToReceive(timeslice now)
{
	//Iterate over all clients actually waiting in the client list.
	//NOTE: This is slightly different than how the previous code did it, but it *should* work.
	//It will at least fail predictably: if the simulator freezes in the first time tick for a new agent, this is where to look.
	//TODO: We need a better way of tracking <client,destAgentID> pairs anyway; that fix will likely simplify this function.
	std::map<SendBuffer::Key, std::string> pendingMessages;
	for (std::map<SendBuffer::Key, OngoingSerialization>::const_iterator it=sendBuffer.begin(); it!=sendBuffer.end(); it++) {
		pendingMessages[it->first] = CommsimSerializer::makeTickedSimMob(now.frame(), ConfigManager::GetInstance().FullConfig().baseGranMS());
	}

	for (std::map<SendBuffer::Key, std::string>::const_iterator it=pendingMessages.begin(); it!=pendingMessages.end(); it++) {
		insertSendBuffer(it->first, it->second);
	}
}

void sim_mob::Broker::processOutgoingData(timeslice now)
{
	for (std::map<SendBuffer::Key, OngoingSerialization>::iterator it=sendBuffer.begin(); it!=sendBuffer.end(); it++) {
		boost::shared_ptr<sim_mob::ConnectionHandler> conn = it->first->connHandle;

		//Getting the string is easy:
		BundleHeader header;
		std::string message;
		CommsimSerializer::serialize_end(it->second, header, message);

		//Forward to the given client.
		//TODO: We can add per-client routing here.
		conn->postMessage(header, message);
	}

	//Clear the buffer for the next time tick.
	sendBuffer.clear();
}



//returns true if you need to wait
bool sim_mob::Broker::checkAllBrokerBlockers()
{
	bool pass = waitAndroidBlocker.pass(*this) && waitNs3Blocker.pass(*this);
	if (!pass && EnableDebugOutput) {
		Print() <<"BrokerBlocker is causing simulator to wait.\n";
	}
	return pass;
}

void sim_mob::Broker::waitAndAcceptConnections(uint32_t tick) {
	//Wait for more clients if:
	//  1- number of subscribers is too low
	//  2-there is no client(emulator) waiting in the queue
	//  3-this update function never started to process any data so far
	bool notified = false;
	for (;;) {
		//boost::unique_lock<boost::mutex> lock(mutex_client_wait_list);

		//Add pending Agents.
		processAgentRegistrationRequests();

		//Add pending clients.
		processClientRegistrationRequests();

		//If this is not the "hold" time tick, then continue.
        if (tick < ST_Config::getInstance().commsim.holdTick) {
			break;
		}

		//If everything's reigstered, break out of the loop.
		if (checkAllBrokerBlockers()) {
			break;
		} else if (!notified) {
			notified = true;
			std::cout <<"Holding on all broker blockers...\n";
		}

		//Else, sleep
		if (EnableDebugOutput) {
			Print() << " brokerCanTickForward->WAITING" << std::endl;
		}

		//NOTE: I am fairly sure this is an acceptable place to establish the lock.
		boost::unique_lock<boost::mutex> lock(mutex_client_wait_list);
		COND_VAR_CLIENT_REQUEST.wait(lock);
	}
}

void sim_mob::Broker::waitForAgentsUpdates()
{
	boost::unique_lock<boost::mutex> lock(mutex_agentDone);
	bool notDone = true;
	while (notDone) {
		//Add pending Agents (to make sure we're checking that ALL agents are done).
		processAgentRegistrationRequests();

		//Check if any Agents in the registeredAgents list is not done.
		notDone = false;
		for (std::map<const Agent*, AgentInfo>::const_iterator it=registeredAgents.begin(); it!=registeredAgents.end(); it++) {
			if (!it->second.done) {
				notDone = true;
				break;
			}
		}

		//If so, sleep until more agents list themselves as done.
		if (notDone) {
			if (EnableDebugOutput) {
				Print() << "waitForAgentsUpdates _WAIT" << std::endl;
			}
			COND_VAR_AGENT_DONE.wait(lock);
			if (EnableDebugOutput) {
				Print() << "waitForAgentsUpdates _WAIT_released" << std::endl;
			}
		}
	}
}


bool sim_mob::Broker::allClientsAreDone()
{
	boost::shared_ptr<sim_mob::ClientHandler> clnHandler;
	boost::unique_lock<boost::mutex> lock(mutex_clientList);

	for (ClientList::Type::const_iterator it=registeredAndroidClients.begin(); it!=registeredAndroidClients.end(); it++) {
		clnHandler = it->second;
		//If we have a valid client handler.
		if (clnHandler && clnHandler->isValid()) {
			//...and a valid connection handler.
			if (clnHandler->connHandle && clnHandler->connHandle->isValid()) {
				//Check if this connection's "done" count equals its "total" known agent count.
				//TODO: This actually checks the same connection handler multiple times if connections are multiplexed
				//      (one for each ClientHandler). This is harmless, but inefficient.
				boost::unique_lock<boost::mutex> lock(mutex_client_done_chk);
				std::map<boost::shared_ptr<sim_mob::ConnectionHandler>, ConnClientStatus>::iterator chkIt = clientDoneChecklist.find(clnHandler->connHandle);
				if (chkIt==clientDoneChecklist.end()) {
					throw std::runtime_error("Client somehow registered without a valid connection handler.");
				}
				if (chkIt->second.done < chkIt->second.total) {
					if (EnableDebugOutput) {
						Print() << "connection [" <<&(*clnHandler->connHandle) << "] not done yet: " <<chkIt->second.done <<" of " <<chkIt->second.total <<"\n";
					}
					return false;
				}
			}
		}
	}
	return true;
}

Entity::UpdateStatus sim_mob::Broker::update(timeslice now)
{
	if (EnableDebugOutput) {
		Print() << "Broker tick:" << now.frame() << std::endl;
	}

	//step-1 : Create/start the thread if this is the first frame.
	//TODO: transfer this to frame_init
	if (now.frame() == 0) {
		//Not profiled; this only happens once.
        connection.start(ST_Config::getInstance().commsim.numIoThreads);
	}

	if (EnableDebugOutput) {
		Print() << "=====================ConnectionStarted =======================================\n";
	}

	PROFILE_LOG_COMMSIM_UPDATE_BEGIN(currWorkerProvider, this, now);

	//Step-2: Ensure that we have enough clients to process
	//(in terms of client type (like ns3, android emulator, etc) and quantity(like enough number of android clients) ).
	//Block the simulation here(if you have to)
	waitAndAcceptConnections(now.frame());

	if (EnableDebugOutput) {
		Print() << "===================== wait Done =======================================\n";
	}

	//step-3: Process what has been received in your receive container(message queue perhaps)
	processIncomingData(now);
	if (EnableDebugOutput) {
		Print() << "===================== processIncomingData Done =======================================\n";
	}

	//step-4: if need be, wait for all agents(or others)
	//to complete their tick so that you are the last one ticking)
	PROFILE_LOG_COMMSIM_LOCAL_COMPUTE_BEGIN(currWorkerProvider, this, now, numAgents);
	waitForAgentsUpdates();
	PROFILE_LOG_COMMSIM_LOCAL_COMPUTE_END(currWorkerProvider, this, now, numAgents);
	if (EnableDebugOutput) {
		Print()<< "===================== waitForAgentsUpdates Done =======================================\n";
	}

	//step-5: signal the publishers to publish their data
	processPublishers(now);

	if (EnableDebugOutput) {
		Print() << "===================== processPublishers Done =======================================\n";
	}

	//step-5.5:for each client, append a message at the end of all messages saying Broker is ready to receive your messages
	sendReadyToReceive(now);

	//step-6: Now send all what has been prepared, by different sources, to their corresponding destications(clients)
	PROFILE_LOG_COMMSIM_MIXED_COMPUTE_BEGIN(currWorkerProvider, this, now, numAgents);
	processOutgoingData(now);
	PROFILE_LOG_COMMSIM_MIXED_COMPUTE_END(currWorkerProvider, this, now, numAgents);
	if (EnableDebugOutput) {
		Print() << "===================== processOutgoingData Done =======================================\n";
	}

	//step-7:
	//the clients will now send whatever they want to send(into the incoming messagequeue)
	//followed by a Done! message.That is when Broker can go forwardClientList::pair clientByType;
	PROFILE_LOG_COMMSIM_ANDROID_COMPUTE_BEGIN(currWorkerProvider, this, now);
	waitForClientsDone();
	PROFILE_LOG_COMMSIM_ANDROID_COMPUTE_END(currWorkerProvider, this, now);
	if (EnableDebugOutput) {
		Print() << "===================== waitForClientsDone Done =======================================\n";
	}

	//step-8:
	//Now that all clients are done, set any properties on new clients.
	//TODO: This is RR only.
	if (!newClientsWaitingOnRegionEnabling.empty()) {
		setNewClientProps();
		if (EnableDebugOutput) {
			Print() << "===================== setNewClientProps Done =======================================\n";
		}
	}

	//step-9: final steps that should be taken before leaving the tick
	//prepare for next tick.
	cleanup();
	PROFILE_LOG_COMMSIM_UPDATE_END(currWorkerProvider, this, now);
	return UpdateStatus(UpdateStatus::RS_CONTINUE);
}

void sim_mob::Broker::setNewClientProps()
{
	//NOTE: I am locking this anyway, just to be safe. (In case a new Broker request arrives in the meantime). ~Seth
	boost::unique_lock<boost::mutex> lock(mutex_clientList);

	//Now, loop through each client and send it a message to inform that the Broker has registered it.
	for (std::set< boost::weak_ptr<sim_mob::ClientHandler> >::iterator it=newClientsWaitingOnRegionEnabling.begin(); it!=newClientsWaitingOnRegionEnabling.end(); it++) {
		//Attempt to resolve the weak pointer.
		boost::shared_ptr<sim_mob::ClientHandler> cHand = it->lock();
		if (cHand) {
			//NOTE: Be CAREFUL here using the Agent pointer as a Context (void*). If you have a class with multiple inheritance,
			//      the void* for different realizations of the same object may have DIFFERENT pointer values. ~Seth
			messaging::MessageBus::PublishEvent(sim_mob::event::EVT_CORE_COMMSIM_ENABLED_FOR_AGENT,
				cHand->agent,
				messaging::MessageBus::EventArgsPtr(new event::EventArgs())
			);
		} else {
			Warn() <<"Broker::setNewClientProps() -- Client was destroyed before its weak_ptr() could be resolved.\n";
		}
	}

	//These clients have been processed.
	newClientsWaitingOnRegionEnabling.clear();
}

void sim_mob::Broker::waitForClientsDone()
{
	boost::unique_lock<boost::mutex> lock(mutex_clientDone);
	while (!allClientsAreDone()) {
		COND_VAR_CLIENT_DONE.wait(lock);
	}
}

void sim_mob::Broker::cleanup()
{
	{
	boost::unique_lock<boost::mutex> lock(mutex_client_done_chk);
	std::map<boost::shared_ptr<sim_mob::ConnectionHandler>, ConnClientStatus>::iterator chkIt = clientDoneChecklist.begin();
	for (;chkIt!=clientDoneChecklist.end(); chkIt++) {
		chkIt->second.done = 0;
	}
	}

	//Reset any "done" flags on the preRegisteredAgents array --they were done for the "last" time tick.
	{
	boost::unique_lock<boost::mutex> lock(mutex_pre_register_agents);
	for (std::map<const Agent*, AgentInfo>::iterator it=preRegisterAgents.begin(); it!=preRegisterAgents.end(); it++) {
		it->second.done = false;  //tosent is never set on preRegistered agents.
	}
	}

	//Reset any "done flags on the registeredAgents array.
	{
	boost::unique_lock<boost::mutex> lock(mutex_agentDone);
	for (std::map<const Agent*, AgentInfo>::iterator it=registeredAgents.begin(); it!=registeredAgents.end(); it++) {
		it->second.done = false;
	}
	}

	//NOTE: Earlier comments indicate a bug with closing any previously open connection handlers. In reality, there is no real
	//      problem leaving them open and !valid, although perhaps with the ConnectionHandler cleanup it is now possible to just
	//      float the ConnectionHandler shared pointer and let it clear itself.
}

void sim_mob::Broker::SetSingleBroker(BrokerBase* broker)
{
	if (single_broker) {
		throw std::runtime_error("Cannot SetSingleBroker(); it has already been set.");
	}
	Broker::single_broker = broker;
}


BrokerBase* sim_mob::Broker::GetSingleBroker()
{
	if (!single_broker) {
		throw std::runtime_error("Cannot GetSingleBroker(); it has not been set yet.");
	}
	return Broker::single_broker;
}

//abstracts & virtuals
void sim_mob::Broker::load(const std::map<std::string, std::string>& configProps)
{
}

void sim_mob::Broker::frame_output(timeslice now)
{
}

bool sim_mob::Broker::isNonspatial()
{
	return true;
}



