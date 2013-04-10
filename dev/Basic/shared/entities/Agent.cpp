/* Copyright Singapore-MIT Alliance for Research and Technology */

#include "Agent.hpp"

#include "conf/settings/ProfileOptions.h"
#include "conf/settings/DisableMPI.h"
#include "conf/settings/StrictAgentErrors.h"

#include "util/DebugFlags.hpp"
#include "util/OutputUtil.hpp"
#include "partitions/PartitionManager.hpp"
#include <cstdlib>
#include <cmath>

#include "conf/simpleconf.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "geospatial/Node.hpp"
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#endif

using namespace sim_mob;
typedef Entity::UpdateStatus UpdateStatus;

using std::vector;
using std::priority_queue;



StartTimePriorityQueue sim_mob::Agent::pending_agents;
vector<Entity*> sim_mob::Agent::all_agents;
EventTimePriorityQueue sim_mob::Agent::agents_with_pending_event;
vector<Entity*> sim_mob::Agent::agents_on_event;

//Implementation of our comparison function for Agents by start time.
bool sim_mob::cmp_agent_start::operator()(const Agent* x, const Agent* y) const {
	//TODO: Not sure what to do in this case...
	if ((!x) || (!y)) {
		return 0;
	}

	//We want a lower start time to translate into a higher priority.
	return x->getStartTime() > y->getStartTime();
}

//Implementation of our comparison function for events by start time.
bool sim_mob::cmp_event_start::operator()(const PendingEvent& x, const PendingEvent& y) const {
	//We want a lower start time to translate into a higher priority.
	return x.start > y.start;
}

unsigned int sim_mob::Agent::next_agent_id = 0;
unsigned int sim_mob::Agent::GetAndIncrementID(int preferredID) {
	//If the ID is valid, modify next_agent_id;
	if (preferredID > static_cast<int> (next_agent_id)) {
		next_agent_id = static_cast<unsigned int> (preferredID);
	}

#ifndef SIMMOB_DISABLE_MPI
	if (ConfigParams::GetInstance().is_run_on_many_computers) {
		PartitionManager& partitionImpl = PartitionManager::instance();
		int mpi_id = partitionImpl.partition_config->partition_id;
		int cycle = partitionImpl.partition_config->maximum_agent_id;
		return (next_agent_id++) + cycle * mpi_id;
	}
#endif

	//Assign either the value asked for (assume it will not conflict) or
	//  the value of next_agent_id (if it's <0)
	unsigned int res = (preferredID>=0) ? static_cast<unsigned int>(preferredID) : next_agent_id++;

	//std::cout <<"  assigned: " <<res <<std::endl;

	return res;
}


void sim_mob::Agent::SetIncrementIDStartValue(int startID, bool failIfAlreadyUsed)
{
	//Check fail condition
	if (failIfAlreadyUsed && Agent::next_agent_id!=0) {
		throw std::runtime_error("Can't call SetIncrementIDStartValue(); Agent ID has already been used.");
	}

	//Fail if we've already passed this ID.
	if(Agent::next_agent_id>startID) {
		throw std::runtime_error("Can't call SetIncrementIDStartValue(); Agent ID has already been assigned.");
	}

	//Set
	Agent::next_agent_id = startID;
}



sim_mob::Agent::Agent(const MutexStrategy& mtxStrat, int id) : Entity(GetAndIncrementID(id)),
	mutexStrat(mtxStrat), call_frame_init(true),
	originNode(), destNode(), xPos(mtxStrat, 0), yPos(mtxStrat, 0),
	fwdVel(mtxStrat, 0), latVel(mtxStrat, 0), xAcc(mtxStrat, 0), yAcc(mtxStrat, 0), currLink(nullptr), currLane(nullptr),
	isQueuing(false), distanceToEndOfSegment(0.0), currTravelStats(nullptr, 0.0), profile(nullptr)
{
	toRemoved = false;
	nextPathPlanned = false;
	dynamic_seed = id;

	if (ConfigParams::GetInstance().ProfileAgentUpdates()) {
		profile = new ProfileBuilder();
		profile->logAgentCreated(*this);
	}
}

sim_mob::Agent::~Agent()
{
	if (ConfigParams::GetInstance().ProfileAgentUpdates()) {
		profile->logAgentDeleted(*this);
	}
	safe_delete_item(profile);
}


void sim_mob::Agent::resetFrameInit()
{
	call_frame_init = true;
}


namespace {
//Ensure all time ticks are valid.
void check_frame_times(unsigned int agentId, uint32_t now, unsigned int startTime, bool wasFirstFrame, bool wasRemoved) {
	//Has update() been called early?
	if (now<startTime) {
		std::stringstream msg;
		msg << "Agent(" <<agentId << ") specifies a start time of: " <<startTime
				<< " but it is currently: " << now
				<< "; this indicates an error, and should be handled automatically.";
		throw std::runtime_error(msg.str().c_str());
	}

	//Has update() been called too late?
	if (wasRemoved) {
		std::stringstream msg;
		msg << "Agent(" <<agentId << ") should have already been removed, but was instead updated at: " <<now
				<< "; this indicates an error, and should be handled automatically.";
		throw std::runtime_error(msg.str().c_str());
	}

	//Was frame_init() called at the wrong point in time?
	if (wasFirstFrame) {
		if (abs(now-startTime)>=ConfigParams::GetInstance().baseGranMS) {
			std::stringstream msg;
			msg <<"Agent was not started within one timespan of its requested start time.";
			msg <<"\nStart was: " <<startTime <<",  Curr time is: " <<now <<"\n";
			msg <<"Agent ID: " <<agentId <<"\n";
			throw std::runtime_error(msg.str().c_str());
		}
	}
}
} //End un-named namespace

UpdateStatus sim_mob::Agent::perform_update(timeslice now)
{
	//We give the Agent the benefit of the doubt here and simply call frame_init().
	//This allows them to override the start_time if it seems appropriate (e.g., if they
	// are swapping trip chains). If frame_init() returns false, immediately exit.
	bool calledFrameInit = false;
	if (call_frame_init) {
		//Call frame_init() and exit early if requested to.
		if (!frame_init(now)) {
			return UpdateStatus::Done;
		}

		//Set call_frame_init to false here; you can only reset frame_init() in frame_tick()
		call_frame_init = false; //Only initialize once.
		calledFrameInit = true;
	}

	//Now that frame_init has been called, ensure that it was done so for the correct time tick.
	check_frame_times(getId(), now.ms(), getStartTime(), calledFrameInit, isToBeRemoved());

	//Perform the main update tick
	UpdateStatus retVal = frame_tick(now);

	//Save the output
	if (retVal.status != UpdateStatus::RS_DONE) {
		frame_output(now);
	}

	//Output if removal requested.
	if (Debug::WorkGroupSemantics && isToBeRemoved()) {
		LogOut("Person requested removal: " <<"(Role Hidden)" <<std::endl);
	}

	return retVal;
}



Entity::UpdateStatus sim_mob::Agent::update(timeslice now)
{
	PROFILE_LOG_AGENT_UPDATE_BEGIN(profile, *this, now);

	//Update within an optional try/catch block.
	UpdateStatus retVal(UpdateStatus::RS_CONTINUE);

#ifndef SIMMOB_STRICT_AGENT_ERRORS
	try {
#endif
		//Update functionality
		retVal = perform_update(now);

//Respond to errors only if STRICT is off; otherwise, throw it (so we can catch it in the debugger).
#ifndef SIMMOB_STRICT_AGENT_ERRORS
	} catch (std::exception& ex) {
		PROFILE_LOG_AGENT_EXCEPTION(profile, *this, frameNumber, ex);

		//Add a line to the output file.
		if (ConfigParams::GetInstance().OutputEnabled()) {
			std::stringstream msg;
			msg <<"Error updating Agent[" <<getId() <<"], will be removed from the simulation.";
			if(originNode.type_ == WayPoint::NODE)
			{
				msg <<"\n  From node: " <<(originNode.node_?originNode.node_->originalDB_ID.getLogItem():"<Unknown>");
			}
			if(destNode.type_ == WayPoint::NODE )
			{
				msg <<"\n  To node: " <<(destNode.node_?destNode.node_->originalDB_ID.getLogItem():"<Unknown>");
			}
			msg <<"\n  " <<ex.what();
			LogOut(msg.str() <<std::endl);
		}
		setToBeRemoved();
	}
#endif

	//Ensure that isToBeRemoved() and UpdateStatus::status are in sync
	if (isToBeRemoved() || retVal.status==UpdateStatus::RS_DONE) {
		retVal.status = UpdateStatus::RS_DONE;
		setToBeRemoved();
	}

	PROFILE_LOG_AGENT_UPDATE_END(profile, *this, now);
	return retVal;
}



void sim_mob::Agent::buildSubscriptionList(vector<BufferedBase*>& subsList)
{
	subsList.push_back(&xPos);
	subsList.push_back(&yPos);
	subsList.push_back(&fwdVel);
	subsList.push_back(&latVel);
	subsList.push_back(&xAcc);
	subsList.push_back(&yAcc);
	//subscriptionList_cached.push_back(&currentLink);
	//subscriptionList_cached.push_back(&currentCrossing);
}

bool sim_mob::Agent::isToBeRemoved() {
	return toRemoved;
}

void sim_mob::Agent::setToBeRemoved() {
	toRemoved = true;
}

void sim_mob::Agent::clearToBeRemoved() {
	toRemoved = false;
}

const sim_mob::Link* sim_mob::Agent::getCurrLink() const{
	return currSegment->getLink();
}
void sim_mob::Agent::setCurrLink(const sim_mob::Link* link){
	currLink = link;
}
const sim_mob::Lane* sim_mob::Agent::getCurrLane() const{
	return currLane;
}
void sim_mob::Agent::setCurrLane(const sim_mob::Lane* lane){
	currLane = lane;
}
const sim_mob::RoadSegment* sim_mob::Agent::getCurrSegment() const{
	return currSegment;
}
void sim_mob::Agent::setCurrSegment(const sim_mob::RoadSegment* rdSeg){
	currSegment = rdSeg;
}

void sim_mob::Agent::initTravelStats(const Link* link, double entryTime)
{
	currTravelStats.link_ = link;
	currTravelStats.linkEntryTime_= entryTime;
}

void sim_mob::Agent::addToTravelStatsMap(travelStats ts, double exitTime){
	travelStatsMap.insert(std::make_pair(exitTime, ts));
}

#ifndef SIMMOB_DISABLE_MPI
int sim_mob::Agent::getOwnRandomNumber() {
	int one_try = -1;
	int second_try = -2;
	int third_try = -3;
	//		int forth_try = -4;

	while (one_try != second_try || third_try != second_try) {
		srand(dynamic_seed);
		one_try = rand();

		srand(dynamic_seed);
		second_try = rand();

		srand(dynamic_seed);
		third_try = rand();
	}

	dynamic_seed = one_try;
	return one_try;
}
#endif
