//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Broker.hpp"

#include <sstream>
#include <boost/assign/list_of.hpp>
#include <json/json.h>

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "workers/Worker.hpp"

#include "entities/commsim/connection/ConnectionHandler.hpp"
#include "entities/commsim/client/ClientHandler.hpp"

#include "entities/commsim/wait/WaitForAndroidConnection.hpp"
#include "entities/commsim/wait/WaitForNS3Connection.hpp"
#include "entities/commsim/wait/WaitForAgentRegistration.hpp"

#include "event/SystemEvents.hpp"
#include "event/args/EventArgs.hpp"
#include "event/EventPublisher.hpp"
#include "message/MessageBus.hpp"
#include "entities/profile/ProfileBuilder.hpp"

#include "geospatial/RoadRunnerRegion.hpp"


using namespace sim_mob;

namespace {
///Some control parameters.
const unsigned int MIN_CLIENTS = 1; //minimum number of registered clients(not waiting list)
const unsigned int MIN_AGENTS = 1;  //minimum number of registered agents
} //End un-named namespace.


BrokerBase* sim_mob::Broker::single_broker(nullptr);

const std::string sim_mob::Broker::ClientTypeAndroid = "android";
const std::string sim_mob::Broker::ClientTypeNs3 = "ns-3";

const unsigned int sim_mob::Broker::EventNewAndroidClient = 95000;


sim_mob::Broker::Broker(const MutexStrategy& mtxStrat, int id, std::string commElement, std::string commMode) :
		Agent(mtxStrat, id), commElement(commElement), commMode(commMode), numAgents(0), connection(*this)
{
	//Various Initializations
	configure();
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
	std::string client_type = ConfigManager::GetInstance().FullConfig().getCommSimMode(commElement);
	bool useNs3 = false;
	if (client_type == "android-ns3") {
		useNs3 = true;
	} else if (client_type == "android-only") {
		useNs3 = false;
	} else { throw std::runtime_error("Unknown clientType in Broker."); }


	//Register handlers with "useNs3" flag. OpaqueReceive will throw an exception if it attempts to process a message and useNs3 is not set.
	handleLookup.addHandlerOverride("opaque_send", new sim_mob::OpaqueSendHandler(useNs3));
	handleLookup.addHandlerOverride("opaque_receive", new sim_mob::OpaqueReceiveHandler(useNs3));

	//We always wait for MIN_CLIENTS Android emulators and MIN_CLIENTS Agents (and optionally, 1 ns-3 client).
	waitAndroidBlocker.reset(MIN_CLIENTS);
	waitAgentBlocker.reset(MIN_AGENTS);
	waitNs3Blocker.reset(useNs3?1:0);
}



/**
 * the callback function called by connection handlers to ask
 * the Broker what to do when a message arrives(this method is to be called
 * after whoareyou, registration and connectionHandler creation....)
 * anyway, what is does is :extract messages from the input string
 * (remember that a message object carries a reference to its handler also(except for "CLIENT_MESSAGES_DONE" messages)
 *
 * NOTE: Be careful; this function can be called multiple times by different "threads". Make sure you are locking where required.
 */
void sim_mob::Broker::onMessageReceived(boost::shared_ptr<ConnectionHandler> cnnHandler, const BundleHeader& header, std::string input)
{
	//If the stream has closed, fail. (Otherwise, the simulation will freeze.)
	if(input.empty()){
		throw std::runtime_error("Empty packet; canceling simulation.");
	}

	//Let the CommsimSerializer handle the heavy lifting.
	MessageConglomerate conglom;
	if (!CommsimSerializer::deserialize(header, input, conglom)) {
		throw std::runtime_error("Broker couldn't parse packet.");
	}


	//We have to introspect a little bit, in order to find our CLIENT_MESSAGES_DONE and WHOAMI messages.
	bool res = false;
	for (int i=0; i<conglom.getCount(); i++) {
		if (NEW_BUNDLES) {
			throw std::runtime_error("onMessageReceived() for NEW_BUNDLES not yet supported.");
		} else {
			const Json::Value& jsMsg = conglom.getMessage(i);
			if (jsMsg.isMember("msg_type") && jsMsg["msg_type"] == "ticked_client") {
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
			} else if (jsMsg.isMember("msg_type") && jsMsg["msg_type"] == "id_response") {
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
				if (connHandle->getClientType().empty()) {
					connHandle->setClientType(candidate.client_type);
				} else {
					if (connHandle->getClientType() != candidate.client_type) {
						throw std::runtime_error("ConnectionHandler received a message for a clientType it did not expect.");
					}
				}

				//Now, wait on it.
				insertClientWaitingList(candidate.client_type, candidate, connHandle);
			}
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
	if (cType==ClientTypeAndroid) {
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

void sim_mob::Broker::insertIntoWaitingOnWHOAMI(const std::string& token, boost::shared_ptr<sim_mob::ConnectionHandler> newConn)
{
	boost::unique_lock<boost::mutex> lock(mutex_token_lookup);
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

void sim_mob::Broker::processIncomingData(timeslice now) {
	//just pop off the message queue and click handle ;)
	MessageElement msgTuple;
	while (receiveQueue.pop(msgTuple)) {
		//Conglomerates contain whole swaths of messages themselves.
		for (int i=0; i<msgTuple.conglom.getCount(); i++) {
			if (NEW_BUNDLES) {
				throw std::runtime_error("processIncoming() for NEW_BUNDLES not yet supported.");
			} else {
				const Json::Value& jsMsg = msgTuple.conglom.getMessage(i);
				if (!jsMsg.isMember("msg_type")) {
					std::cout <<"Invalid message, no message_type\n";
					return;
				}

				//Certain message types have already been handled.
				std::string msgType = jsMsg["msg_type"].asString();
				if (msgType=="ticked_client" || msgType=="id_response") {
					continue;
				}

				//Get the handler, let it parse its own expected message type.
				const sim_mob::Handler* handler = handleLookup.getHandler(msgType);
				if (handler) {
					handler->handle(msgTuple.cnnHandler, msgTuple.conglom, i, this);
				} else {
					std::cout <<"no handler for type \"" <<msgType << "\"\n";
				}
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

	//if we are operating on android-ns3 set up, each android client registration should be brought to ns3's attention
	//TODO: We should keep the "useNs3" flag instead of checking the config file every time.
	if (ConfigManager::GetInstance().FullConfig().getCommSimMode(commElement) != "android-ns3") {
		return;
	}

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

	//Create the AgentsInfo message.
	std::vector<unsigned int> agentIds;
	agentIds.push_back(clientHandler->agent->getId());
	std::string message = CommsimSerializer::makeNewAgents(agentIds, std::vector<unsigned int>());


	//Add it.
	insertSendBuffer(nsHand, message);

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
	std::map<unsigned int, DPoint> allLocs;
	for (std::map<const Agent*, AgentInfo>::const_iterator it=registeredAgents.begin(); it!=registeredAgents.end(); it++) {
		allLocs[it->first->getId()] = DPoint(it->first->xPos.get(), it->first->yPos.get());
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
		/*if (cHandler->regisTime) {
			insertSendBuffer(cHandler, timeMsg);
		}*/
		if (cHandler->regisLocation) {
			//Attempt to reverse-project the Agent's (x,y) location into Lat/Lng, if such a projection is possible.
			LatLngLocation loc;
			CoordinateTransform* trans = ConfigManager::GetInstance().FullConfig().getNetwork().getCoordTransform(false);
			if (trans) {
				loc = trans->transform(DPoint(cHandler->agent->xPos.get(), cHandler->agent->yPos.get()));
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
		if (cHandler->regisAllLocations) {
			insertSendBuffer(cHandler, allLocMsg);
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
		if (!CommsimSerializer::serialize_end(it->second, header, message)) {
			throw std::runtime_error("Broker: Could not finalize serialization.");
		}

		//Forward to the given client.
		//TODO: We can add per-client routing here.
		conn->forwardMessage(header, message);
	}

	//Clear the buffer for the next time tick.
	sendBuffer.clear();
}



//returns true if you need to wait
bool sim_mob::Broker::checkAllBrokerBlockers()
{
	bool pass = waitAgentBlocker.pass(*this) && waitAndroidBlocker.pass(*this) && waitNs3Blocker.pass(*this);
	if (!pass && EnableDebugOutput) {
		Print() <<"BrokerBlocker is causing simulator to wait.\n";
	}
	return pass;
}

void sim_mob::Broker::waitAndAcceptConnections() {
	//Wait for more clients if:
	//  1- number of subscribers is too low
	//  2-there is no client(emulator) waiting in the queue
	//  3-this update function never started to process any data so far
	for (;;) {
		//boost::unique_lock<boost::mutex> lock(mutex_client_wait_list);

		//Add pending Agents.
		processAgentRegistrationRequests();

		//Add pending clients.
		processClientRegistrationRequests();

		//Sleep if we're not ready.
		if (!checkAllBrokerBlockers()) {
			if (EnableDebugOutput) {
				Print() << " brokerCanTickForward->WAITING" << std::endl;
			}

			//NOTE: I am fairly sure this is an acceptable place to establish the lock.
			boost::unique_lock<boost::mutex> lock(mutex_client_wait_list);
			COND_VAR_CLIENT_REQUEST.wait(lock);
		}
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
		connection.start();  //Not profiled; this only happens once.
	}

	if (EnableDebugOutput) {
		Print() << "=====================ConnectionStarted =======================================\n";
	}

	PROFILE_LOG_COMMSIM_UPDATE_BEGIN(currWorkerProvider, this, now);

	//Step-2: Ensure that we have enough clients to process
	//(in terms of client type (like ns3, android emulator, etc) and quantity(like enough number of android clients) ).
	//Block the simulation here(if you have to)
	waitAndAcceptConnections();

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
	//clientDoneChecker.clear();
	boost::unique_lock<boost::mutex> lock(mutex_client_done_chk);
	std::map<boost::shared_ptr<sim_mob::ConnectionHandler>, ConnClientStatus>::iterator chkIt = clientDoneChecklist.begin();
	for (;chkIt!=clientDoneChecklist.end(); chkIt++) {
		chkIt->second.done = 0;
	}

	return;

	//note:this part is supposed to delete clientList entries for the dead agents
	//But there is a complication on deleting connection handler
	//there is a chance the sockets are deleted before a send ACK arrives

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



