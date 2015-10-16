//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "WorkGroupManager.hpp"

#include <stdexcept>

#include "conf/ConfigParams.hpp"
#include "conf/ConfigManager.hpp"
#include "logging/Log.hpp"
#include "workers/WorkGroup.hpp"
#include "message/MessageBus.hpp"
#include "entities/Agent.hpp"
#include "path/PathSetManager.hpp"

#include <time.h>
#include <sstream>
#include "entities/profile/ProfileBuilder.hpp"

using std::vector;

using namespace sim_mob;

namespace
{
	bool firstTick = true;
}


WorkGroupManager::~WorkGroupManager() {

    try {
        // Distibute get all final messages before unregister the main thread.
        messaging::MessageBus::DistributeMessages();
    } catch (std::exception& ex) {
        WarnOut("MessageBus - This call should be done using the registered main thread context.");
    }
    //First, join and delete all WorkGroups
    for (vector<WorkGroup*>::iterator it = registeredWorkGroups.begin(); it != registeredWorkGroups.end(); it++) {
        delete *it;
    }

    //Finally, delete all barriers.
    safe_delete_item(frameTickBarr);
    safe_delete_item(buffFlipBarr);
    safe_delete_item(msgBusBarr);

    try {
        // UnRegisters the main thread for message bus.
        messaging::MessageBus::UnRegisterMainThread();
    } catch (std::exception& ex) {
        WarnOut("MessageBus - This call should be done using the registered main thread context.");
    }
}


std::list<std::string> sim_mob::WorkGroupManager::retrieveOutFileNames() const
{
	bool pass = currState.test(BARRIERS) || currState.test(STARTED);
	if (!pass) {
		throw std::runtime_error("retrieveOutFileNames() failed: the current state does not allow it.");
	}

	std::list<std::string> res;
	for (vector<WorkGroup*>::const_iterator it=registeredWorkGroups.begin(); it!=registeredWorkGroups.end(); it++) {
		(*it)->addOutputFileNames(res);
	}
	return res;
}


WorkGroup* sim_mob::WorkGroupManager::newWorkGroup(unsigned int numWorkers, unsigned int numSimTicks, unsigned int tickStep,
		AuraManager* auraMgr, PartitionManager* partitionMgr, sim_mob::PeriodicPersonLoader* periodicLoader)
{
	//Sanity check
	bool pass = (currState.test(INIT)||currState.test(CREATE)) && currState.set(CREATE);
	if (!pass) {
		throw std::runtime_error("newWorkGroup failed: the current state does not allow it.");
	}

	//Most of this involves passing paramters on to the WorkGroup itself, and then bookkeeping via static data.
	WorkGroup* res = new WorkGroup(registeredWorkGroups.size(), numWorkers, numSimTicks, tickStep, auraMgr, partitionMgr, periodicLoader);
	currBarrierCount += numWorkers;

	registeredWorkGroups.push_back(res);
	return res;
}


void sim_mob::WorkGroupManager::setSingleThreadMode(bool enable)
{
	//TODO: Might be overly restrictive; perhaps only "start" needs to be preempted.
	if (!currState.test(INIT)) { throw std::runtime_error("Can't change to/from single-threaded mode once WorkGroups have been registered."); }

	singleThreaded = enable;
}


void sim_mob::WorkGroupManager::initAllGroups()
{
        // Registers the main thread for message bus.
        messaging::MessageBus::RegisterMainThread();
	//Sanity check
	bool pass = currState.test(CREATE) && currState.set(BARRIERS);
	if (!pass) { throw std::runtime_error("Can't init work groups; barriers have already been established."); }

	//No barriers are created in single-threaded mode.
	if (!singleThreaded) {
		//Create a barrier for each of the three shared phases (aura manager optional)
		frameTickBarr = new FlexiBarrier(currBarrierCount);
		buffFlipBarr = new FlexiBarrier(currBarrierCount);
		msgBusBarr = new FlexiBarrier(currBarrierCount);

		//Initialize each WorkGroup with these new barriers.
		for (vector<WorkGroup*>::iterator it=registeredWorkGroups.begin(); it!=registeredWorkGroups.end(); it++) {
			(*it)->initializeBarriers(frameTickBarr, buffFlipBarr, msgBusBarr);
		}
	}
}


void sim_mob::WorkGroupManager::startAllWorkGroups()
{
	//Sanity check
	bool pass = currState.test(BARRIERS) && currState.set(STARTED);
	if (!pass) { throw std::runtime_error("Can't start all WorkGroups; no barrier."); }

	for (vector<WorkGroup*>::iterator it=registeredWorkGroups.begin(); it!=registeredWorkGroups.end(); it++) {
		(*it)->startAll(singleThreaded);
	}
}


void sim_mob::WorkGroupManager::waitAllGroups()
{
	//Collect entities.
	std::set<Agent*> removedEntities;

	timeval start, end, endFT, endFB, endDM, endMT;
	std::stringstream timestream;
	gettimeofday(&start, nullptr);
	//Call each function in turn.
	//NOTE: Each sub-function tests the current state.
    if(ConfigManager::GetInstance().FullConfig().RunningMidTerm() && firstTick)
	{
		//first tick has two frameTickBarr
		if (frameTickBarr) {
			frameTickBarr->wait();
		}
		firstTick = false;
	}
	waitAllGroups_FrameTick();
	gettimeofday(&endFT, nullptr);
	timestream << "FT: "<< (ProfileBuilder::diff_ms(endFT, start))/1000.0 << "s|";

	waitAllGroups_FlipBuffers(&removedEntities);
	gettimeofday(&endFB, nullptr);
	timestream << "FB: "<< (ProfileBuilder::diff_ms(endFB, endFT))/1000.0 << "s|";

	waitAllGroups_DistributeMessages(removedEntities);
	gettimeofday(&endDM, nullptr);
	timestream << "DM: "<< (ProfileBuilder::diff_ms(endDM, endFB))/1000.0 << "s|";

	waitAllGroups_MacroTimeTick();
	gettimeofday(&endMT, nullptr);
	timestream << "MT: "<< (ProfileBuilder::diff_ms(endMT, endDM))/1000.0 << "s|";

	//Delete all collected entities:
	while (!removedEntities.empty()) {
		Agent* ag = *removedEntities.begin();
		removedEntities.erase(removedEntities.begin());
		delete ag;
	}

	gettimeofday(&end, nullptr);
	timestream << "total: "<< (ProfileBuilder::diff_ms(end, start))/1000.0 << "s" << std::endl;
	std::cout << timestream.str();
}

void sim_mob::WorkGroupManager::waitAllGroups_FrameTick()
{
	//Sanity check
	if (!currState.test(STARTED)) { throw std::runtime_error("Can't tick WorkGroups; no barrier."); }

	for (vector<WorkGroup*>::iterator it=registeredWorkGroups.begin(); it!=registeredWorkGroups.end(); it++) {
		(*it)->waitFrameTick(singleThreaded);
	}

	//Here is where we actually block, ensuring a tick-wide synchronization.
	if (frameTickBarr) {
		frameTickBarr->wait();
	}
}

void sim_mob::WorkGroupManager::waitAllGroups_FlipBuffers(std::set<Agent*>* removedEntities)
{
	//Sanity check
	if (!currState.test(STARTED)) { throw std::runtime_error("Can't tick WorkGroups; no barrier."); }

	for (vector<WorkGroup*>::iterator it=registeredWorkGroups.begin(); it!=registeredWorkGroups.end(); it++) {
		(*it)->waitFlipBuffers(singleThreaded, removedEntities);
	}

	//Here is where we actually block, ensuring a tick-wide synchronization.
	if (buffFlipBarr) {
		buffFlipBarr->wait();
	}	
}

void sim_mob::WorkGroupManager::waitAllGroups_MacroTimeTick()
{
	//Sanity check
	if (!currState.test(STARTED)) { throw std::runtime_error("Can't tick WorkGroups; no barrier."); }

	for (vector<WorkGroup*>::iterator it=registeredWorkGroups.begin(); it!=registeredWorkGroups.end(); it++) {
		(*it)->waitMacroTimeTick();
	}

	//NOTE: There is no need for a "wait()" here, since macro barriers are used internally.
}

void sim_mob::WorkGroupManager::waitAllGroups_DistributeMessages(std::set<Agent*>& removedEntities)
{
	//Sanity check
	if (!currState.test(STARTED)) { throw std::runtime_error("Can't tick WorkGroups; no barrier."); }

	//We don't need this if there's no Aura Manager.
	if (!msgBusBarr) {
		return;
	}

	if(!ConfigManager::GetInstance().FullConfig().RunningLongTerm())
	{
		for (vector<WorkGroup*>::iterator it=registeredWorkGroups.begin(); it!=registeredWorkGroups.end(); it++) {
			if (ConfigManager::GetInstance().FullConfig().RunningMidTerm())
			{
				(*it)->processMultiUpdateEntities(removedEntities); // VQ
				(*it)->processMultiUpdateEntities(removedEntities); // output & reset for next tick
			}
			else // RunningShortTerm()
			{
				(*it)->waitAuraManager(removedEntities);
			}
		}
		TravelTimeManager::updateCurrTimeInterval();
	}

	//Now is a good time to distribute messages since all agents have finished processing for this tick
	sim_mob::messaging::MessageBus::DistributeMessages();

	//Here is where we actually block, ensuring a tick-wide synchronization.
	if (msgBusBarr) {
		msgBusBarr->wait();
	}
}


const std::vector<sim_mob::WorkGroup*> sim_mob::WorkGroupManager::getRegisteredWorkGroups()
{
	//Sanity check
	bool pass = currState.test(BARRIERS) || currState.test(STARTED);
	if (!pass) { throw std::runtime_error("WorkGroup creation still in progress; can't retrieve registered groups."); }

	return registeredWorkGroups;
}
