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

#include "entities/commsim/broker/Common.hpp"
#include "entities/commsim/comm_support/AgentCommUtility.hpp"
#include "entities/commsim/connection/ConnectionServer.hpp"
#include "entities/commsim/connection/ConnectionHandler.hpp"
#include "entities/commsim/event/subscribers/base/ClientHandler.hpp"

#include "entities/commsim/wait/WaitForAndroidConnection.hpp"
#include "entities/commsim/wait/WaitForNS3Connection.hpp"
#include "entities/commsim/wait/WaitForAgentRegistration.hpp"
#include "event/SystemEvents.hpp"
#include "message/MessageBus.hpp"
#include "event/EventPublisher.hpp"

//todo :temprorary
#include "entities/commsim/message/derived/roadrunner-android/RR_Factory.hpp"
#include "entities/commsim/message/derived/roadrunner-android-ns3/roadrunner_ns3_factory.hpp"

using namespace sim_mob;

namespace{
//subclassed Eventpublisher coz its destructor is pure virtual
	class BrokerPublisher: public sim_mob::event::EventPublisher  {
	public:
		BrokerPublisher(){}
		virtual ~BrokerPublisher(){}
	};
}  //End un-named namespace


std::map<std::string, sim_mob::Broker*> sim_mob::Broker::externalCommunicators;

sim_mob::Broker::Broker(const MutexStrategy& mtxStrat, int id ) : Agent(mtxStrat, id), enabled(true)
{
	//Various Initializations
	connection.reset(new ConnectionServer(*this));
	brokerCanTickForward = false;
	m_messageReceiveCallback = boost::function<void(boost::shared_ptr<ConnectionHandler>, std::string)>
		(boost::bind(&Broker::messageReceiveCallback,this, _1, _2));

	Print() << "Creating Broker [" << this << "]" << std::endl;
	configure();
}

sim_mob::Broker::~Broker()
{}

void sim_mob::Broker::enable()
{
	enabled = true;
}

void sim_mob::Broker::disable()
{
	enabled = false;
}

bool sim_mob::Broker::isEnabled() const
{
	return enabled;
}

bool sim_mob::Broker::insertSendBuffer(boost::shared_ptr<sim_mob::ConnectionHandler> cnnHandler, sim_mob::comm::MsgData &value )
{
	if(!cnnHandler) {
		return false;
	}
	if (cnnHandler->isValid() == false) {
		return false;
	}
	sendBuffer[cnnHandler].add(value);
	return true;
}

void sim_mob::Broker::configure()
{
	//Only configure once.
	if (!configured_.check()) {
		return;
	}

	//Dispatch differently depending on whether we are using "android-ns3" or "android-only"
	//TODO: Find a more dynamic way of adding new clients.
	const std::string client_type = ConfigManager::GetInstance().FullConfig().getAndroidClientType();

	sim_mob::Worker::GetUpdatePublisher().Subscribe(sim_mob::event::EVT_CORE_AGENT_UPDATED, (void*)sim_mob::event::CXT_CORE_AGENT_UPDATE, this, CONTEXT_CALLBACK_HANDLER(UpdateEventArgs, Broker::onAgentUpdate));

	BrokerPublisher* onlyLocationsPublisher = new BrokerPublisher();
	onlyLocationsPublisher->RegisterEvent(COMMEID_LOCATION);

	publishers.insert(std::make_pair(
		sim_mob::Services::SIMMOB_SRV_LOCATION,
		PublisherList::dataType(onlyLocationsPublisher))
	);

	//NS-3 has its own publishers
	if(client_type == "android-ns3") {
		BrokerPublisher* allLocationsPublisher = new BrokerPublisher();
		allLocationsPublisher->RegisterEvent(COMMEID_LOCATION);
		publishers.insert(std::make_pair(
			sim_mob::Services::SIMMOB_SRV_ALL_LOCATIONS,
			PublisherList::dataType(allLocationsPublisher))
		);
	}

	BrokerPublisher* timePublisher = new BrokerPublisher();
	timePublisher->RegisterEvent(COMMEID_TIME);
	publishers.insert(std::make_pair(
		sim_mob::Services::SIMMOB_SRV_TIME,
		PublisherList::dataType(timePublisher))
	);

	ClientRegistrationHandler::getPublisher().Subscribe(ConfigParams::ANDROID_EMULATOR, this, CALLBACK_HANDLER(ClientRegistrationEventArgs, Broker::onClientRegister));

	if(client_type == "android-ns3") {
		//listen to publishers who announce registration of new clients...
		ClientRegistrationHandler::getPublisher().Subscribe(ConfigParams::NS3_SIMULATOR, this, CALLBACK_HANDLER(ClientRegistrationEventArgs, Broker::onClientRegister));
	}

	//current message factory
	//todo: choose a factory based on configurations not hardcoding
	if(client_type == "android-ns3") {
		boost::shared_ptr<sim_mob::MessageFactory<std::vector<sim_mob::comm::MsgPtr>&, std::string&> >
			android_factory(new sim_mob::rr_android_ns3::RR_Android_Factory());
		boost::shared_ptr<sim_mob::MessageFactory<std::vector<sim_mob::comm::MsgPtr>&, std::string&> >
			ns3_factory(new sim_mob::rr_android_ns3::RR_NS3_Factory());

		//note that both client types refer to the same message factory belonging to roadrunner application. we will modify this to a more generic approach later-vahid
		messageFactories.insert(std::make_pair(ConfigParams::ANDROID_EMULATOR, android_factory));
		messageFactories.insert(std::make_pair(ConfigParams::NS3_SIMULATOR, ns3_factory));
	} else if (client_type == "android-only") {
		boost::shared_ptr<sim_mob::MessageFactory<std::vector<sim_mob::comm::MsgPtr>&, std::string&> >
			android_factory(new sim_mob::roadrunner::RR_Factory());

		//note that both client types refer to the same message factory belonging to roadrunner application. we will modify this to a more generic approach later-vahid
		messageFactories.insert(std::make_pair(ConfigParams::ANDROID_EMULATOR, android_factory));
	}

	// wait for connection criteria for this broker
	clientBlockers.insert(std::make_pair(ConfigParams::ANDROID_EMULATOR,
			boost::shared_ptr<WaitForAndroidConnection>(new WaitForAndroidConnection(*this,MIN_CLIENTS))));


	if(client_type == "android-ns3") {
		clientBlockers.insert(std::make_pair(ConfigParams::NS3_SIMULATOR,
				boost::shared_ptr<WaitForNS3Connection>(new WaitForNS3Connection(*this))));

	}

	// wait for connection criteria for this broker
	agentBlockers.insert(std::make_pair(0,
			boost::shared_ptr<WaitForAgentRegistration>(new WaitForAgentRegistration(*this,MIN_AGENTS))));
}


/*
 * the callback function called by connection handlers to ask
 * the Broker what to do when a message arrives(this method is to be called
 * after whoareyou, registration and connectionHandler creation....)
 * anyway, what is does is :extract messages from the input string
 * (remember that a message object carries a reference to its handler also(except for "CLIENT_MESSAGES_DONE" messages)
 */
void sim_mob::Broker::messageReceiveCallback(boost::shared_ptr<ConnectionHandler> cnnHandler, std::string input)
{
	boost::shared_ptr<MessageFactory<std::vector<sim_mob::comm::MsgPtr>&, std::string&> > m_f = messageFactories[cnnHandler->clientType];
	std::vector<sim_mob::comm::MsgPtr> messages;
	m_f->createMessage(input, messages);
//	post the messages into the message queue one by one(add their cnnHandler also)
	for(std::vector<sim_mob::comm::MsgPtr>::iterator it = messages.begin(); it != messages.end(); it++)
	{

		sim_mob::comm::MsgData& data = it->get()->getData();
		std::string type = data["MESSAGE_TYPE"].asString();
//		Print() << "Received " << (cnnHandler->clientType == ConfigParams::NS3_SIMULATOR ? "NS3" : ( cnnHandler->clientType == ConfigParams::ANDROID_EMULATOR ? "ANDROID" : "UNKNOWN_TYPE") ) << " : " << type << std::endl;
		if(type == "CLIENT_MESSAGES_DONE")
		{
			boost::unique_lock<boost::mutex> lock(mutex_clientDone);
			//update() will wait until all clients send this message for each tick
//			Print()
//								<< "received CLIENT_MESSAGES_DONE for client["
//								<< cnnHandler->clientType << ":" << cnnHandler->clientID << "]" << std::endl;
			clientDoneChecker.insert(cnnHandler);
			COND_VAR_CLIENT_DONE.notify_one();
		}
		else
		{
			receiveQueue.post(boost::make_tuple(cnnHandler,*it));
		}
	}
}

boost::function<void(boost::shared_ptr<ConnectionHandler>, std::string)> sim_mob::Broker::getMessageReceiveCallBack()
{
	return m_messageReceiveCallback;
}


void sim_mob::Broker::OnEvent(event::EventId eventId, sim_mob::event::Context ctxId, event::EventPublisher* sender, const event::EventArgs& args)
{
	switch(eventId){
		case sim_mob::event::EVT_CORE_AGENT_DIED:{
			const event::AgentLifeCycleEventArgs& args_ = MSG_CAST(event::AgentLifeCycleEventArgs, args);
			Print() << "Broker::UNregisterINGEntity[" << args_.GetAgent() << "][" << args_.GetAgentId() << "]" << std::endl;
			unRegisterEntity(args_.GetAgent());
			break;
		}
	default:break;
	}

}

AgentsList::type& sim_mob::Broker::getRegisteredAgents()
{
	//use it with caution
	return REGISTERED_AGENTS.getAgents();
}

AgentsList::type& sim_mob::Broker::getRegisteredAgents(AgentsList::Mutex* mutex)
{
	//always supply the mutex along
	mutex = REGISTERED_AGENTS.getMutex();
	return REGISTERED_AGENTS.getAgents();
}

ClientWaitList& sim_mob::Broker::getClientWaitingList()
{
	return clientRegistrationWaitingList;
}

ClientList::type& sim_mob::Broker::getClientList()
{
	return clientList;
}


bool sim_mob::Broker::getClientHandler(std::string clientId,std::string clientType, boost::shared_ptr<sim_mob::ClientHandler> &output)
{
	//use try catch to use map's .at() and search only once
	try {
		ConfigParams::ClientType clientType_ = sim_mob::Services::ClientTypeMap.at(clientType);
		boost::unordered_map<std::string , boost::shared_ptr<sim_mob::ClientHandler> > & inner = clientList[clientType_];
		try
		{
			output = inner.at(clientId); //this is what we are looking for
			return true;
		}
		catch(std::out_of_range &e)
		{
			WarnOut("Client " <<  clientId << " of type " <<  clientType << " not found" << std::endl);
			return false;
		}

	} catch(std::out_of_range& e) {
		Print() << "Client type" <<  clientType << " not found" << std::endl;
		return false;
	}
	//program never reaches here :)
	return false;
}

void sim_mob::Broker::insertClientList(std::string clientID, unsigned int clientType, boost::shared_ptr<sim_mob::ClientHandler> &clientHandler)
{
	boost::unique_lock<boost::mutex> lock(mutex_clientList);
	clientList[clientType][clientID] = clientHandler;
}

void  sim_mob::Broker::insertClientWaitingList(std::pair<std::string,ClientRegistrationRequest > p)//pair<client type, request>
{
	boost::unique_lock<boost::mutex> lock(mutex_client_request);
//	Print() << "Inserting into clientRegistrationWaitingList" << std::endl;
	clientRegistrationWaitingList.insert(p);
	COND_VAR_CLIENT_REQUEST.notify_one();
}

PublisherList::type& sim_mob::Broker::getPublishers()
{
	return publishers;
}

void sim_mob::Broker::processClientRegistrationRequests()
{
	boost::shared_ptr<ClientRegistrationHandler > handler;
	ClientWaitList::iterator it_erase;//helps avoid multimap iterator invalidation
	for(ClientWaitList::iterator it = clientRegistrationWaitingList.begin(), it_end(clientRegistrationWaitingList.end()); it != it_end;/*no ++ here */  )
	{
		handler = clientRegistrationFactory.getHandler((sim_mob::Services::ClientTypeMap[it->first]));
		if(handler->handle(*this,it->second))
		{
			//success: handle() just added to the client to the main client list and started its connectionHandler
			//	next, see if the waiting state of waiting-for-client-connection changes after this process
			bool wait = clientBlockers[sim_mob::Services::ClientTypeMap[it->first]]->calculateWaitStatus();
//			Print() << "calculateWaitStatus for  " << it->first << "  " << (wait? "WAIT " : "DONT_WAIT") << std::endl;
			if(!wait)
			{
				//	then, get this request out of registration list.
				it_erase =  it++;//keep the erase candidate. dont loose it :)
				clientRegistrationWaitingList.erase(it_erase) ;
				//note: if needed,remember to do the necessary work in the
				//corresponding agent w.r.t the result of handle()
				//do this through a callback to agent's reuest
			}
			else
			{
				it++; //putting it here coz multimap is not like a vector. erase doesn't return an iterator.
			}
		}
		else
		{
			Print() << "clientRegistration handler for " << it->first << " failed" << std::endl;
			it++; //putting it here coz multimap is not like a vector. erase doesn't return an iterator.
		}
	}
}

bool sim_mob::Broker::registerEntity(sim_mob::AgentCommUtilityBase* value)
{
	Print() << std::dec;
//	registeredAgents.insert(std::make_pair(value->getEntity(), value));
	REGISTERED_AGENTS.insert(value->getEntity(), value);
	Print()<< REGISTERED_AGENTS.size() << ":  Broker[" << this <<  "] :  Broker::registerEntity [" << value->getEntity()->getId() << "]" << std::endl;
	//feedback
	value->registrationCallBack(true);
	//tell me if you are dying
	sim_mob::messaging::MessageBus::SubscribeEvent(sim_mob::event::EVT_CORE_AGENT_DIED,
			static_cast<event::Context>(value->getEntity()), this);
	return true;
}

void  sim_mob::Broker::unRegisterEntity(sim_mob::AgentCommUtilityBase *value)
{
	unRegisterEntity(value->getEntity());
}

void  sim_mob::Broker::unRegisterEntity(sim_mob::Agent * agent)
{
	Print() << "inside Broker::unRegisterEntity for agent[" << agent << "]" << std::endl;
	//search agent's list looking for this agent
//	registeredAgents.erase(agent); //hopefully the agent is there
	REGISTERED_AGENTS.erase(agent);

	//search the internal container also
	duplicateEntityDoneChecker.erase(agent);

	{
		boost::unique_lock<boost::mutex> lock(mutex_clientList);
	//search registered clients list looking for this agent. whoever has it, dump him
	for(ClientList::iterator it_clientType = clientList.begin(); it_clientType != clientList.end(); it_clientType++)
	{
		boost::unordered_map<std::string, boost::shared_ptr<sim_mob::ClientHandler> >::iterator
			it_clientID(it_clientType->second.begin()),
			it_clientID_end(it_clientType->second.end()),
			it_erase;

		for(; it_clientID != it_clientID_end; )
		{
			if(it_clientID->second->agent == agent)
			{
				it_erase = it_clientID++;
				//unsubscribe from all publishers he is subscribed to
				sim_mob::ClientHandler * clientHandler = it_erase->second.get();
				sim_mob::Services::SIM_MOB_SERVICE srv;
				BOOST_FOREACH(srv, clientHandler->requiredServices)
				{
					switch(srv)
					{
					case sim_mob::Services::SIMMOB_SRV_TIME:
						publishers[sim_mob::Services::SIMMOB_SRV_TIME]->UnSubscribe(COMMEID_TIME,clientHandler);
						break;
					case sim_mob::Services::SIMMOB_SRV_LOCATION:
						publishers[sim_mob::Services::SIMMOB_SRV_LOCATION]->UnSubscribe(COMMEID_LOCATION,(void*)clientHandler->agent,clientHandler);
						break;
					case sim_mob::Services::SIMMOB_SRV_ALL_LOCATIONS:
						publishers[sim_mob::Services::SIMMOB_SRV_ALL_LOCATIONS]->UnSubscribe(COMMEID_LOCATION,(void*)COMMCID_ALL_LOCATIONS,clientHandler);
						break;
					}
				}
				//erase him from the list
				//clientList.erase(it_erase);
				//don't erase it here. it may already have something to send
				//invalidate it and clean it up when necessary
				//invalidation 1:
				it_erase->second->agent = 0;
				it_erase->second->cnnHandler->agentPtr = 0; //this is even more important
				//or a better version //todo: use one version only
				it_erase->second->setValidation(false);
				it_erase->second->cnnHandler->setValidation(false); //this is even more important

			}
			else
			{
				it_clientID++;
			}
		}//inner loop

	}//outer loop
	}
}

void sim_mob::Broker::processIncomingData(timeslice now)
{
	//just pop off the message queue and click handl ;)
	MessageElement::type msgTuple;
	while(receiveQueue.pop(msgTuple))
	{
		sim_mob::comm::MsgPtr &msg = msgTuple.get<1>();
		sim_mob::comm::MsgData data = msg->getData();
		Json::FastWriter w;
		msg->supplyHandler()->handle(msg,this);
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

//todo consider scrabbing DriverComm
bool sim_mob::Broker::allAgentUpdatesDone()
{
	AgentsList::done_range its = REGISTERED_AGENTS.getNotDone();
	bool res = its.first == its.second;
	if(!res) {
		Print() << "allAgentUpdatesDone not done : " <<  std::endl;

		for (; its.first != its.second; its.first++) {
			Print() << "Not Done: [" << its.first->agent->getId() << "]" << std::endl;
		}
	}
	return res;
}

void sim_mob::Broker::onAgentUpdate(sim_mob::event::EventId id, sim_mob::event::Context context, sim_mob::event::EventPublisher* sender, const UpdateEventArgs& argums)
{
	Print() << "Inside onAgentUpdate" << std::endl;
	Agent * target = const_cast<Agent*>(dynamic_cast<const Agent*>(argums.GetEntity()));
	Print() << "onAgentUpdate:: setting[" << target->getId() << "] to.done" << std::endl;
	boost::unique_lock<boost::mutex> lock(mutex_agentDone);
	if(REGISTERED_AGENTS.setDone(target,true))
	{
		Print() << "Agent[" << target->getId() << "] done" << std::endl;
		COND_VAR_AGENT_DONE.notify_all();
	}
	else
	{
		Print() << "Thread[" << boost::this_thread::get_id() << "] Discarded Agent[" << target->getId() << "]" << std::endl;
	}
}

void sim_mob::Broker::AgentUpdated(Agent * target)
{
	Print() << "Inside AgentUpdated" << std::endl;
	Print() << "AgentUpdated:: setting[" << target->getId() << "] to.done" << std::endl;
	boost::unique_lock<boost::mutex> lock(mutex_agentDone);
	if(REGISTERED_AGENTS.setDone(target,true))
	{
		Print() << "Agent[" << target->getId() << "] done" << std::endl;
//		duplicateEntityDoneChecker.insert(target);
		COND_VAR_AGENT_DONE.notify_all();
	}
	else
	{
		Print() << "Thread[" << boost::this_thread::get_id() << "] Discarded Agent[" << target->getId() << "]" << std::endl;
	}
}

void sim_mob::Broker::onClientRegister(sim_mob::event::EventId id, sim_mob::event::EventPublisher* sender, const ClientRegistrationEventArgs& argums)
{
	ConfigParams::ClientType type = argums.getClientType();
	boost::shared_ptr<ClientHandler>clientHandler = argums.getClient();

	switch (type) {
	case ConfigParams::ANDROID_EMULATOR: {
		//if we are operating on android-ns3 set up,
		//each android client registration should be brought to
		//ns3's attention
		if (ConfigManager::GetInstance().FullConfig().getAndroidClientType()!= "android-ns3") {
			break;
		}
		//note: based on the current implementation of
		// the ns3 client registration handler, informing the
		//statistics of android clients IS a part of ns3
		//registration and configuration process. So the following implementation
		//should be executed for those android clients who will join AFTER
		//ns3's registration. So we check if the ns3 is already registered or not:
		if (clientList.find(ConfigParams::NS3_SIMULATOR) == clientList.end()) {
			break;
		}
		msg_header mHeader_("0", "SIMMOBILITY", "AGENTS_INFO", "SYS");
		sim_mob::comm::MsgData jMsg = JsonParser::createMessageHeader(mHeader_);
		const Agent *agent = clientHandler->agent;
		Json::Value jAgent;
		jAgent["AGENT_ID"] = agent->getId();
		//sorry it should be in an array to be compatible with the other
		//place where many agents are informed to be added
		Json::Value jArray_agent;
		jArray_agent.append(jAgent);
		//sorry again, compatibility issue. I will change this later.
		Json::Value jArray_add;
		jArray_add["ADD"].append(jArray_agent);
		insertSendBuffer(clientHandler->cnnHandler,jArray_add);
		break;
	}

	}
}

//todo: again this function is also intrusive to Broker. find a way to move the following switch cases outside the broker-vahid
//todo suggestion: for publishment, don't iterate through the list of clients, rather, iterate the publishers list, access their subscriber list and say publish and publish for their subscribers(keep the clientlist for MHing only)
void sim_mob::Broker::processPublishers(timeslice now)
{
	PublisherList::pair publisher_pair;
	BOOST_FOREACH(publisher_pair, publishers)
	{
		//easy reading
		sim_mob::Services::SIM_MOB_SERVICE service = publisher_pair.first;
		sim_mob::event::EventPublisher & publisher = *publisher_pair.second;

		switch (service) {
		case sim_mob::Services::SIMMOB_SRV_TIME: {
			publisher.Publish(COMMEID_TIME, TimeEventArgs(now));
			break;
		}
		case sim_mob::Services::SIMMOB_SRV_LOCATION: {

			//get to each client handler, look at his requred service and then publish for him
			ClientList::pair clientsByType;
			ClientList::IdPair clientsByID;
			BOOST_FOREACH(clientsByType, clientList)
			{
				BOOST_FOREACH(clientsByID, clientsByType.second)
				{
					boost::shared_ptr<sim_mob::ClientHandler> & cHandler = clientsByID.second;//easy read
					publisher.Publish(COMMEID_LOCATION,(void*) cHandler->agent,LocationEventArgs(cHandler->agent));
				}
			}
			break;
		}
		case sim_mob::Services::SIMMOB_SRV_ALL_LOCATIONS: {
			publisher.Publish(COMMEID_LOCATION,(void*) COMMCID_ALL_LOCATIONS,AllLocationsEventArgs(REGISTERED_AGENTS));
			break;
		}
		default:
			break;
		}
	}
}

void sim_mob::Broker::sendReadyToReceive()
{
	ClientList::pair clientByType;
	ClientList::IdPair clientByID;

	boost::shared_ptr<sim_mob::ClientHandler> clnHandler;
	msg_header msg_header_;
	{
		boost::unique_lock<boost::mutex> lock(mutex_clientList);
	BOOST_FOREACH(clientByType, clientList)
	{
		BOOST_FOREACH(clientByID, clientByType.second)
		{
			clnHandler = clientByID.second;
			msg_header_.msg_cat = "SYS";
			msg_header_.msg_type = "READY_TO_RECEIVE";
			msg_header_.sender_id = "0";
			msg_header_.sender_type = "SIMMOBILITY";
			sim_mob::comm::MsgData msg = JsonParser::createMessageHeader(msg_header_);
			insertSendBuffer(clnHandler->cnnHandler, msg);
		}
	}
	}
}

void sim_mob::Broker::processOutgoingData(timeslice now)
{
//	now send what you have to send:
	int debug_sendBuffer_cnt = sendBuffer.size();
	int debug_cnt = 0;
	int debug_buffer_size;
	Json::FastWriter debug_writer;
	std::ostringstream debug_out;
	for(SEND_BUFFER<sim_mob::comm::MsgData>::iterator it = sendBuffer.begin(); it!= sendBuffer.end(); it++, debug_cnt++)
	{
		sim_mob::BufferContainer<sim_mob::comm::MsgData> & buffer = it->second;
		boost::shared_ptr<sim_mob::ConnectionHandler> cnn = it->first;

		//build a jsoncpp structure comprising of a header and data array(containing messages)
		Json::Value jpacket;
		Json::Value jheader;
		Json::Value jpacketData;
		Json::Value jmsg;
		debug_buffer_size = buffer.size();
		jpacketData.clear();
		while(buffer.pop(jmsg))
		{
			jpacketData.append(jmsg);
		}
		int nof_messages;
		if(!(nof_messages = jpacketData.size()))
		{
			continue;
		}
		jheader = JsonParser::createPacketHeader(pckt_header(nof_messages));
		jpacket.clear();
		jpacket["PACKET_HEADER"] = jheader;
		jpacket["DATA"] = jpacketData;

		//convert the jsoncpp packet to a json string
		std::string str = Json::FastWriter().write(jpacket);
		cnn->async_send(str);
	}
	sendBuffer.clear();
}


//checks to see if the subscribed entity(agent) is alive
bool sim_mob::Broker::deadEntityCheck(sim_mob::AgentCommUtilityBase * info)
{
	if(!info)
	{
		throw std::runtime_error("Invalid AgentCommUtility\n");
	}

	Agent * target = info->getEntity();
	try {

		if (!(target->currWorkerProvider)) {
//			Print() << "1-deadEntityCheck for[" << target << "]" << std::endl;
			return true;
		}

		//one more check to see if the entity is deleted
		const std::set<sim_mob::Entity*> & managedEntities_ =
				target->currWorkerProvider->getEntities();
		std::set<sim_mob::Entity*>::const_iterator it =
				managedEntities_.begin();
		if(!managedEntities_.size())
		{
//			Print() << "2-deadEntityCheck for[" << target << "]" << std::endl;
			return true;
		}
		for (std::set<sim_mob::Entity*>::const_iterator it =
				managedEntities_.begin(); it != managedEntities_.end(); it++) {
			//agent is still being managed, so it is not dead
			if (*it == target)
				return false;
		}
	} catch (std::exception& e) {
//		Print() << "3-deadEntityCheck for[" << target << "]" << std::endl;
		return true;
	}

//	Print() << "4-deadEntityCheck for[" << target << "]" << std::endl;

	return true;
}

//iterate the entire agent registration list looking for
//those who are not done with their update and check if they are dead.
//you better hope they are dead otherwise you have to hold the simulation
//tick waiting for them to finish
void sim_mob::Broker::refineSubscriptionList()
{
	Print() << "inside Broker::refineSubscriptionList" << std::endl;

	//do all the operation using the objects's mutex
	boost::function<void(sim_mob::Agent*)> Fn = boost::bind(&Broker::refineSubscriptionList, this,_1);
	REGISTERED_AGENTS.for_each_agent(Fn);
}

//tick waiting for them to finish
void sim_mob::Broker::refineSubscriptionList(sim_mob::Agent * target)
{
		//you or your worker are probably dead already. you just don't know it
		if (!target->currWorkerProvider) {
			Print() << "1-refine subscription for agent ["  << target << "]" << std::endl;
			unRegisterEntity(target);
			return;
		}
		const std::set<sim_mob::Entity*> & managedEntities_ = (target->currWorkerProvider)->getEntities();
		std::set<sim_mob::Entity*>::const_iterator  it_entity = std::find(managedEntities_.begin(), managedEntities_.end(), target);
		if(it_entity == managedEntities_.end())
		{
			Print() << "2-refine subscription for agent ["  << target << "]" << std::endl;
			unRegisterEntity(target);
			return;
		}
}
//sim_mob::Broker sim_mob::Broker::instance(MtxStrat_Locked, 0);

//todo:  put a better condition here. this is just a placeholder
bool sim_mob::Broker::isWaitingForAgentRegistration() const
{
	return agentBlockers.at(0)->calculateWaitStatus();
}

//todo:  put a better condition here. this is just a placeholder
bool sim_mob::Broker::clientsQualify() const
{
	return clientList.size() >= MIN_CLIENTS;
}

//returns true if you need to wait
bool sim_mob::Broker::isWaitingForAnyClientConnection() {
//	Print() << "inside isWaitingForAnyClientConnection " << clientBlockers.size() << std::endl;
	BrokerBlockers::IdPair pp;
	int i = -1;
	BOOST_FOREACH(pp, clientBlockers) {
		i++;
		if (pp.second->isWaiting()) {
			Print() << " isWaitingForAnyClientConnection[" << i << "] : wait" << std::endl;
			return true;
		}
	}
//	Print() << "isWaitingForAnyClientConnection : Dont wait" << std::endl;
	return false;
}

bool sim_mob::Broker::wait()
{
	//	Initial evaluation
	{
		boost::unique_lock<boost::mutex> lock(mutex_client_request);
		processClientRegistrationRequests();
		bool res1 = isWaitingForAgentRegistration();
		bool res2 = isWaitingForAnyClientConnection();
		bool res3 = !res1 && !res2;
		bool res = /*brokerCanTickForward || */res3;
		brokerCanTickForward = res;
	}

	/**if:
	 * 1- number of subscribers is too low
	 * 2-there is no client(emulator) waiting in the queue
	 * 3-this update function never started to process any data so far
	 * then:
	 *  wait for enough number of clients and agents to join
	 */

	int i = -1;
	{
		boost::unique_lock<boost::mutex> lock(mutex_client_request);
		while (!brokerCanTickForward) {
			Print() << ++i << " brokerCanTickForward->WAITING" << std::endl;
			COND_VAR_CLIENT_REQUEST.wait(lock);
			Print() << "COND_VAR_CLIENT_REQUEST released" << std::endl;
			processClientRegistrationRequests();

			bool res1 = isWaitingForAgentRegistration();
			bool res2 = isWaitingForAnyClientConnection();
			bool res3 = !res1 && !res2;
			bool res = /*brokerCanTickForward ||*/res3;
			brokerCanTickForward = res;
			Print() << "brokerCanTickForward["<< res1 << "-" << res2 << "]"  << std::endl;
			//	brokerCanTickForward = brokerCanTickForward || ((isWaitingForAgentRegistration() && !isWaitingForAnyClientConnection()));
//		Print() << "Broker::wait()::Secondary Evaluation => " << (brokerCanTickForward ? "True" : "false") << std::endl;
		}
	}


	//broker started before but suddenly is no more qualified to run
//	if(brokerCanTickForward && (!isWaitingForAgentRegistration())) {
//		//don't block, just cooperate & don't do anything until this simulation ends
//		//TODO: This might be why our client eventually gives up.
//		return false;
//	}



	//Success! Continue.
	return true;
}

void sim_mob::Broker::waitForAgentsUpdates()
{
	int i = 0;
//	boost::system_time const timeout=boost::get_system_time()+ boost::posix_time::seconds(1);

	boost::unique_lock<boost::mutex> lock(mutex_agentDone);
//	refineSubscriptionList();
	while(!allAgentUpdatesDone()) {
		Print() << "waitForAgentsUpdates _WAIT" << std::endl;
//		COND_VAR_AGENT_DONE.timed_wait(lock, timeout);
		COND_VAR_AGENT_DONE.wait(lock);
		Print() << "waitForAgentsUpdates _WAIT_released" << std::endl;
//		refineSubscriptionList();
	}
}

bool sim_mob::Broker::isClientDone(boost::shared_ptr<sim_mob::ClientHandler> &clnHandler)
{
	if(clientDoneChecker.end() == clientDoneChecker.find(clnHandler->cnnHandler))
	{
		return false;
	}
	return true;
}

bool sim_mob::Broker::allClientsAreDone()
{
		ClientList::pair clientByType;
		ClientList::IdPair clientByID;

		boost::shared_ptr<sim_mob::ClientHandler> clnHandler;
		msg_header msg_header_;
		boost::unique_lock<boost::mutex> lock(mutex_clientList);
		BOOST_FOREACH(clientByType, clientList)
		{
			BOOST_FOREACH(clientByID, clientByType.second)
			{
				clnHandler = clientByID.second;
				if(!clnHandler)
				{
					continue;
				}
				if(!(clnHandler->cnnHandler))
				{
					continue;
				}
				if(!(clnHandler->cnnHandler->is_open()))
				{
					continue;
				}
				if(!(clnHandler->isValid()))
				{
					continue;
				}
				if(!(clnHandler->cnnHandler->isValid()))
				{
					continue;
				}
				//but
				if(!isClientDone(clnHandler))
				{
					return false;
				}
			}
		}
		return true;
}

Entity::UpdateStatus sim_mob::Broker::update(timeslice now)
{
	Print() << "Broker tick:"<< now.frame() << std::endl;

	//step-1 : Create/start the thread if this is the first frame.
	//TODO: transfer this to frame_init
	if(now.frame() == 0) {
		connection->start();
	}
	Print() << "=====================ConnectionStarted =======================================" << std::endl;

	//Step-2: Ensure that we have enough clients to process
	//(in terms of client type (like ns3, android emulator, etc) and quantity(like enough number of android clients) ).
	//Block the simulation here(if you have to)
	wait();
	Print() << "===================== wait Done =======================================" << std::endl;

	//step-3: Process what has been received in your receive container(message queue perhaps)
	processIncomingData(now);
	Print() << "===================== processIncomingData Done =======================================" << std::endl;

	//step-4: if need be, wait for all agents(or others)
	//to complete their tick so that you are the last one ticking)
	waitForAgentsUpdates();
	Print() << "===================== waitForAgentsUpdates Done =======================================" << std::endl;

	//step-5: signal the publishers to publish their data
	processPublishers(now);
	Print() << "===================== processPublishers Done =======================================" << std::endl;
//	step-5.5:for each client, append a message at the end of all messages saying Broker is ready to receive your messages
	sendReadyToReceive();

	//step-6: Now send all what has been prepared, by different sources, to their corresponding destications(clients)
	processOutgoingData(now);
	Print() << "===================== processOutgoingData Done =======================================" << std::endl;

	//step-7:
	//the clients will now send whatever they want to send(into the incoming messagequeue)
	//followed by a Done! message.That is when Broker can go forwardClientList::pair clientByType;
	waitForClientsDone();

//	//Step 7.5: output
//	if (ConfigManager::GetInstance().FullConfig().ProfileWorkerUpdates()) {
//		//TODO: This is a bit of a hack; won't work with both ns3/Android enabled. ~Seth.
//		size_t sz = 0;
//		for (ClientList::type::iterator it=clientList.begin(); it!=clientList.end(); it++) {
//			sz += it->second.size();
//		}
//		std::stringstream msg;
//		msg <<sz;
//		profile->logAgentCustomMessage(*this, now, "custom-num-connected-clients", msg.str());
//		//TODO: End hack.
//
//		//TODO: And another hack!
//		const uint32_t TickAmt = 100; //"Every X ticks"
//		const uint32_t TickStep = 1; //HACK: For what the Worker sees (for all Agents).
//		if ((now.frame()/TickStep)%TickAmt==0) {
//			profile->flushLogFile();
//		}
//	}
	Print() << "===================== waitForClientsDone Done =======================================" << std::endl;

	//step-8: final steps that should be taken before leaving the tick
	//prepare for next tick.
	cleanup();
	return UpdateStatus(UpdateStatus::RS_CONTINUE);
}


void sim_mob::Broker::removeClient(ClientList::iterator it_erase)
{
//	Print() << "Broker::removeClient locking mutex_clientList" << std::endl;
//	if(!it_erase->second)
//	{
//		return;
//	}
//	//delete the connection handler inside the client handler
//	if(it_erase->second->cnnHandler)
//	{
////		it_erase->second->cnnHandler.reset();
//	}
//	//delete the clientHandler
////	it_erase->second.reset();
//	//remove it from the list
//	clientList.erase(it_erase);
//	Print() << "Broker::removeClient UNlocking mutex_clientList" << std::endl;
}

void sim_mob::Broker::waitForClientsDone()
{
	boost::unique_lock<boost::mutex> lock(mutex_clientDone);
	while(!allClientsAreDone()) {
		COND_VAR_CLIENT_DONE.wait(lock);
	}
}

void sim_mob::Broker::cleanup()
{
	//for internal use
	duplicateEntityDoneChecker.clear();
	clientDoneChecker.clear();
	return;

	//note:this part is supposed to delete clientList entries for the dead agents
	//But there is a complication on deleting connection handler
	//there is a chance the sockets are deleted before a send ACK arrives

}

std::map<std::string, sim_mob::Broker*>& sim_mob::Broker::getExternalCommunicators()
{
	return externalCommunicators;
}

sim_mob::Broker* sim_mob::Broker::getExternalCommunicator(const std::string & value)
{
	if(externalCommunicators.find(value) != externalCommunicators.end())
	{
		return externalCommunicators[value];
	}
	return 0;
}

void sim_mob::Broker::addExternalCommunicator(const std::string & name, sim_mob::Broker* broker)
{
	externalCommunicators.insert(std::make_pair(name,broker));
}

//abstracts & virtuals
void sim_mob::Broker::load(const std::map<std::string, std::string>& configProps)
{}

void sim_mob::Broker::frame_output(timeslice now)
{}

bool sim_mob::Broker::isNonspatial()
{
	return true;
}

