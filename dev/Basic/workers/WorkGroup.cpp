/* Copyright Singapore-MIT Alliance for Research and Technology */

#include "WorkGroup.hpp"

//For debugging
#include <stdexcept>
#include <boost/thread.hpp>
#include "entities/Agent.hpp"
#include "entities/Person.hpp"
#include "util/OutputUtil.hpp"

using std::vector;
using boost::barrier;
using boost::function;

using namespace sim_mob;




/**
 * Template function must be defined in the same translational unit as it is declared.
 */

void sim_mob::WorkGroup::initWorkers(Worker::ActionFunction* action)
{
	for (size_t i=0; i<total_size; i++) {
		workers.push_back(new Worker(this, action, &shared_barr, &external_barr, endTick, tickStep, auraManagerActive));
	}
}



//////////////////////////////////////////////
// These also need the temoplate parameter,
// but don't actually do anything with it.
//////////////////////////////////////////////



sim_mob::WorkGroup::WorkGroup(size_t size, unsigned int endTick, unsigned int tickStep, bool auraManagerActive) :
		shared_barr(size+1), external_barr(size+1), nextWorkerID(0), endTick(endTick), tickStep(tickStep), total_size(size), auraManagerActive(auraManagerActive),
		nextTimeTickToStage(0)
{
}



sim_mob::WorkGroup::~WorkGroup()
{
	for (size_t i=0; i<workers.size(); i++) {
		workers[i]->join();  //NOTE: If we don't join all Workers, we get threading exceptions.
		delete workers[i];
	}
}



void sim_mob::WorkGroup::startAll()
{
	//Stage any Agents that will become active within the first time tick (in time for the next tick)
	nextTimeTickToStage = 0;
	//stageAgents();

	//Start all workers
	tickOffset = tickStep;
	for (size_t i=0; i<workers.size(); i++) {
		workers[i]->start();
	}
}



void sim_mob::WorkGroup::stageAgents()
{
	unsigned int nextTickMS = nextTimeTickToStage*ConfigParams::GetInstance().baseGranMS;
	while (!Agent::pending_agents.empty() && Agent::pending_agents.top()->startTime <= nextTickMS) {
		if (sim_mob::Debug::WorkGroupSemantics) {
			std::cout <<"Staging agent ID: " <<Agent::pending_agents.top()->getId() <<" in time for tick: " <<nextTimeTickToStage <<"\n";
		}

		//Add it to our global list. Requires locking.
		{
			//TODO: This shouldn't actually require locking. Leaving it in here for now to be safe.
			boost::mutex::scoped_lock local_lock(sim_mob::Agent::all_agents_lock);
			Agent::all_agents.push_back(Agent::pending_agents.top());
		}

		//Find a worker to assign this to and send it the Entity to manage.
		assignAWorker(Agent::pending_agents.top());
		Agent::pending_agents.pop();
	}
}


void sim_mob::WorkGroup::assignAWorker(Entity* ag)
{
	workers.at(nextWorkerID++)->scheduleForAddition(ag);
	nextWorkerID %= workers.size();
}



size_t sim_mob::WorkGroup::size()
{
	return workers.size();
}


/*
void sim_mob::WorkGroup::migrate(Entity& ag, Worker* from, Worker* to)
{
	if (from) {
		//Remove
		from->remEntity(&ag);
	}

	if (to) {
		//Add
		to->addEntity(&ag);
	}
}*/



sim_mob::Worker* sim_mob::WorkGroup::getWorker(int id)
{
	if (id<0) {
		return nullptr;
	}
	return workers.at(id);
}



void sim_mob::WorkGroup::wait()
{
	if (--tickOffset>0) {
		return;
	}
	tickOffset = tickStep;

	//Stay in sync with the workers.
	nextTimeTickToStage += tickStep;

	shared_barr.wait();

	//Stage Agent updates based on nextTimeTickToStage
	stageAgents();

	//Remove any Agents staged for removal.
	for (std::vector<Agent*>::iterator it=agToBeRemoved.begin(); it!=agToBeRemoved.end(); it++) {
		boost::mutex::scoped_lock local_lock(sim_mob::Agent::all_agents_lock);
		std::vector<Agent*>::iterator it2 = std::find(Agent::all_agents.begin(), Agent::all_agents.end(), *it);
		if (it2!=Agent::all_agents.end()) {
			Agent::all_agents.erase(it2);
		}
	}
	agToBeRemoved.clear();

	external_barr.wait();
}



void sim_mob::WorkGroup::waitExternAgain()
{
	if (!auraManagerActive) {
		throw std::runtime_error("Aura manager must be active for waitExternAgain()");
	}
	external_barr.wait();
}



void sim_mob::WorkGroup::interrupt()
{
	for (size_t i=0; i<workers.size(); i++)
		workers[i]->interrupt();
}



