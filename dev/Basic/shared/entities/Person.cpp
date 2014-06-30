//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Person.hpp"

#include <algorithm>
#include <sstream>

#include <boost/lexical_cast.hpp>

//For debugging
#include "roles/Role.hpp"
#include "entities/misc/TripChain.hpp"
#include "entities/roles/activityRole/ActivityPerformer.hpp"
#include "util/GeomHelpers.hpp"
#include "util/DebugFlags.hpp"

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "logging/Log.hpp"
#include "geospatial/Node.hpp"
#include "entities/misc/TripChain.hpp"
#include "workers/Worker.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "entities/amodController/AMODController.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#include "entities/misc/TripChain.hpp"
#endif
//
using std::map;
using std::string;
using std::vector;
using namespace sim_mob;
typedef Entity::UpdateStatus UpdateStatus;


namespace {
Trip* MakePseudoTrip(const Person& ag, const std::string& mode)
{
	//Make sure we have something to work with
	if (!(ag.originNode .node_&& ag.destNode.node_)) {
		std::stringstream msg;
		msg <<"Can't make a pseudo-trip for an Agent with no origin and destination nodes: " <<ag.originNode.node_ <<" , " <<ag.destNode.node_;
		throw std::runtime_error(msg.str().c_str());
	}

	//Make the trip itself
	Trip* res = new Trip();
	res->setPersonID(ag.getId());
	res->itemType = TripChainItem::getItemType("Trip");
	res->sequenceNumber = 1;
	res->startTime = DailyTime(ag.getStartTime());  //TODO: This may not be 100% correct
	res->endTime = res->startTime; //No estimated end time.
	res->tripID = "";
	res->fromLocation = WayPoint(ag.originNode);
	res->fromLocationType = TripChainItem::getLocationType("node");
	res->toLocation = WayPoint(ag.destNode);
	res->toLocationType = res->fromLocationType;

	//SubTrip generatedSubTrip(-1, "Trip", 1, DailyTime(candidate.start), DailyTime(),
	//candidate.origin, "node", candidate.dest, "node", "Car", true, "");

	//Make and assign a single sub-trip
	sim_mob::SubTrip subTrip;
	subTrip.setPersonID(-1);
	subTrip.itemType = TripChainItem::getItemType("Trip");
	subTrip.sequenceNumber = 1;
	subTrip.startTime = res->startTime;
	subTrip.endTime = res->startTime;
	subTrip.fromLocation = res->fromLocation;
	subTrip.fromLocationType = res->fromLocationType;
	subTrip.toLocation = res->toLocation;
	subTrip.toLocationType = res->toLocationType;
	subTrip.tripID = "";
	subTrip.mode = mode;
	subTrip.isPrimaryMode = true;
	subTrip.ptLineId = "";

	//Add it to the Trip; return this value.
	res->addSubTrip(subTrip);
	return res;
}

}  //End unnamed namespace

sim_mob::Person::Person(const std::string& src, const MutexStrategy& mtxStrat, int id, std::string databaseID) : Agent(mtxStrat, id),
	prevRole(nullptr), currRole(nullptr), nextRole(nullptr), agentSrc(src), currTripChainSequenceNumber(0), remainingTimeThisTick(0.0),
	requestedNextSegment(nullptr), canMoveToNextSegment(NONE), databaseID(databaseID), debugMsgs(std::stringstream::out), tripchainInitialized(false), laneID(-1),
	age(0), boardingTimeSecs(0), alightingTimeSecs(0), client_id(-1), resetParamsRequired(false)
{
}

sim_mob::Person::Person(const std::string& src, const MutexStrategy& mtxStrat, std::vector<sim_mob::TripChainItem*>  tcs)
	: Agent(mtxStrat), remainingTimeThisTick(0.0), requestedNextSegment(nullptr), canMoveToNextSegment(NONE),
	  databaseID(tcs.front()->getPersonID()), debugMsgs(std::stringstream::out), prevRole(nullptr), currRole(nullptr),
	  nextRole(nullptr), laneID(-1), agentSrc(src), tripChain(tcs), tripchainInitialized(false), age(0), boardingTimeSecs(0), alightingTimeSecs(0),
	  client_id(-1),amodPath( std::vector<WayPoint>() )
{
	/*if (!ConfigManager::GetInstance().CMakeConfig().UsingConfluxes()) {
		simplyModifyTripChain(tcs);
	}*/
	initTripChain();
}

void sim_mob::Person::initTripChain(){
	currTripChainItem = tripChain.begin();
	setStartTime((*currTripChainItem)->startTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime()));
	if((*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_TRIP || (*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_FMODSIM)
	{
		currSubTrip = ((dynamic_cast<sim_mob::Trip*>(*currTripChainItem))->getSubTripsRW()).begin();
		//consider putting this in IT_TRIP clause
		if(!updateOD(*currTripChainItem)){ //Offer some protection
				throw std::runtime_error("Trip/Activity mismatch, or unknown TripChainItem subclass.");
		}
	}

	if((*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_BUSTRIP) {
		std::cout << "Person " << this << "  is going to ride a bus\n";
	}
	setNextPathPlanned(false);
	first_update_tick = true;
	tripchainInitialized = true;
}

sim_mob::Person::~Person() {
	safe_delete_item(prevRole);
	safe_delete_item(currRole);
	safe_delete_item(nextRole);
}



void sim_mob::Person::load(const map<string, string>& configProps)
{
	if(!tripChain.empty()) {
		return;// already have a tripchain usually from generateFromTripChain, no need to load
	}
	//Make sure they have a mode specified for this trip
	map<string, string>::const_iterator it = configProps.find("#mode");
	if (it==configProps.end()) {
		throw std::runtime_error("Cannot load person: no mode");
	}
	std::string mode = it->second;

	//Consistency check: specify both origin and dest
	if (configProps.count("originPos") != configProps.count("destPos")) {
		throw std::runtime_error("Agent must specify both originPos and destPos, or neither.");
	}


	std::map<std::string, std::string>::const_iterator lanepointer = configProps.find("lane");
	if(lanepointer != configProps.end())
	{
		try {
		    int x = boost::lexical_cast<int>( lanepointer->second );
		    laneID = x;
		} catch( boost::bad_lexical_cast const& ) {
		    Warn() << "Error: input string was not valid" << std::endl;
		}
	}


	//Consistency check: are they requesting a pseudo-trip chain when they actually have one?
	map<string, string>::const_iterator origIt = configProps.find("originPos");
	map<string, string>::const_iterator destIt = configProps.find("destPos");
	if (origIt!=configProps.end() && destIt!=configProps.end()) {
		//Double-check some potential error states.
		if (!tripChain.empty()) {
			throw std::runtime_error("Manual position specified for Agent with existing Trip Chain.");
		}
		if (this->originNode.node_ || this->destNode.node_ ) {
			throw std::runtime_error("Manual position specified for Agent with existing start and end of Trip Chain.");
		}

		//Otherwise, make a trip chain for this Person.
		this->originNode = WayPoint( ConfigManager::GetInstance().FullConfig().getNetwork().locateNode(parse_point(origIt->second), true) );
		this->destNode = WayPoint( ConfigManager::GetInstance().FullConfig().getNetwork().locateNode(parse_point(destIt->second), true) );

		//Make sure they have a mode specified for this trip
		it = configProps.find("#mode");
		if (it==configProps.end()) {
			throw std::runtime_error("Cannot load person: no mode");
		}
		std::string mode = it->second;

		Trip* singleTrip = MakePseudoTrip(*this, mode);

		std::vector<TripChainItem*> trip_chain;
		trip_chain.push_back(singleTrip);

		//////
		//////TODO: Some of this should be performed in a centralized place; e.g., "Agent::setTripChain"
		//////
		////////TODO: This needs to go in a centralized place.
		this->originNode = singleTrip->fromLocation;
		this->destNode = singleTrip->toLocation;
		this->setNextPathPlanned(false);
		this->setTripChain(trip_chain);
		this->initTripChain();
	}

	//One more check: If they have a special string, save it now
	/*it = configProps.find("special");
	if (it != configProps.end()) {
		this->specialStr = it->second;
	}*/
}


void Person::rerouteWithBlacklist(const std::vector<const sim_mob::RoadSegment*>& blacklisted)
{
	//This requires the Role's intervention.
	if (currRole) {
		currRole->rerouteWithBlacklist(blacklisted);
	}
}
void sim_mob::Person::handleAMODEvent(sim_mob::event::EventId id,
            sim_mob::event::Context ctxId,
            sim_mob::event::EventPublisher* sender,
            const AMOD::AMODEventArgs& args)
{
	if(id == event::EVT_AMOD_REROUTING_REQUEST_WITH_PATH)
	{
		AMOD::AMODEventPublisher* pub = (AMOD::AMODEventPublisher*) sender;
		const AMOD::AMODRerouteEventArgs& rrArgs = MSG_CAST(AMOD::AMODRerouteEventArgs, args);
		std::cout<<"person <"<<amodId<<"> get reroute event <"<< rrArgs.reRoutePath.size() <<"> from <"<<pub->id<<">"<<std::endl;

//		Driver *driver = (Driver*)currRole;
//		driver->rerouteWithPath(rrArgs.reRoutePath);
	//role gets chance to handle event
		if(currRole){
			currRole->onParentEvent(id, ctxId, sender, args);
		}
	}
}
void sim_mob::Person::onEvent(event::EventId eventId, sim_mob::event::Context ctxId, event::EventPublisher* sender, const event::EventArgs& args)
{
	sim_mob::Agent::onEvent(eventId, ctxId, sender, args);

	//role gets chance to handle event
	if(currRole){
		currRole->onParentEvent(eventId, ctxId, sender, args);
	}
}


bool sim_mob::Person::frame_init(timeslice now)
{
	currTick = now;
	//Agents may be created with a null Role and a valid trip chain
	if (!currRole) {
		//TODO: This UpdateStatus has a "prevParams" and "currParams" that should
		//      (one would expect) be dealt with. Where does this happen?
		UpdateStatus res =	checkTripChain(now.ms());

		//Reset the start time (to the current time tick) so our dispatcher doesn't complain.
		setStartTime(now.ms());

		//Nothing left to do?
		if (res.status == UpdateStatus::RS_DONE) {
			return false;
		}
	}

	//Failsafe: no Role at all?
	if (!currRole) {
		std::ostringstream txt ;
		txt << "Person " << this->getId() <<  " has no Role.";
		throw std::runtime_error(txt.str());
	}

	//Get an UpdateParams instance.
	//TODO: This is quite unsafe, but it's a relic of how Person::update() used to work.
	//      We should replace this eventually (but this will require a larger code cleanup).
	currRole->make_frame_tick_params(now);

	//Now that the Role has been fully constructed, initialize it.
	if((*currTripChainItem)) {
//		currRole->Movement()->setPath();
		currRole->Movement()->frame_init();
	}

	return true;
}
void sim_mob::Person::setPath(std::vector<WayPoint>& path)
{
	if (path.size() == 0) {
		std::cout << "Warning! Path size is zero!" << std::endl;
	}

	amodPath = path;
}

void sim_mob::Person::invalidateAMODVehicle(void) {
	std::cout << "Invalidating: " << amodId << std::endl;
	std::cout << "An error has occured with this vehicle." << std::endl;
	sim_mob::AMOD::AMODController *a = sim_mob::AMOD::AMODController::instance();
	a->handleVHError(this);
};

Entity::UpdateStatus sim_mob::Person::frame_tick(timeslice now)
{
	currTick = now;
	//TODO: Here is where it gets risky.
	if (resetParamsRequired) {
		currRole->make_frame_tick_params(now);
		resetParamsRequired = false;
	}

	Entity::UpdateStatus retVal(UpdateStatus::RS_CONTINUE);

	if (!isToBeRemoved()) {
		currRole->Movement()->frame_tick();
	}

//	DriverMovement *d = (DriverMovement *)currRole->Movement();
	if(isToBeRemoved())
	{
		sim_mob::AMOD::AMODController *a = sim_mob::AMOD::AMODController::instance();
		a->handleVHArrive(this);
	}

	//If we're "done", try checking to see if we have any more items in our Trip Chain.
	// This is not strictly the right way to do things (we shouldn't use "isToBeRemoved()"
	// in this manner), but it's the easiest solution that uses the current API.
	//TODO: This section should technically go after frame_output(), but doing that
	//      (by overriding Person::update() and calling Agent::update() then inserting this code)
	//      will bring us outside the bounds of our try {} catch {} statement. We might move this
	//      statement into the worker class, but I don't want to change too many things
	//      about Agent/Person at once. ~Seth
	if (isToBeRemoved()) {
		retVal = checkTripChain(now.ms());

		//Reset the start time (to the NEXT time tick) so our dispatcher doesn't complain.
		setStartTime(now.ms()+ConfigManager::GetInstance().FullConfig().baseGranMS());

		//IT_ACTIVITY as of now is just a matter of waiting for a period of time(between its start and end time)
		//since start time of the activity is usually later than what is configured initially,
		//we have to make adjustments so that it waits for exact amount of time
		if(currTripChainItem != tripChain.end()) {
			if((*currTripChainItem)) {// if currTripChain not end and has value, call frame_init and switching roles
				if(isCallFrameInit()) {
					currRole->Movement()->frame_init();
					setCallFrameInit(false);// set to be false so later no need to frame_init later
				}
			}
			if((*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_ACTIVITY) {
				sim_mob::ActivityPerformer *ap = dynamic_cast<sim_mob::ActivityPerformer*>(currRole);
				ap->setActivityStartTime(sim_mob::DailyTime(now.ms() + ConfigManager::GetInstance().FullConfig().baseGranMS()));
				ap->setActivityEndTime(sim_mob::DailyTime(now.ms() + ConfigManager::GetInstance().FullConfig().baseGranMS() + ((*currTripChainItem)->endTime.getValue() - (*currTripChainItem)->startTime.getValue())));
				ap->initializeRemainingTime();
			}
		}
	}

	// check if arrive dest
//	Driver *d = (Driver *)currRole;
//	if(d->vehicle->isDone())
//	if(currRole->getResource() && currRole->getResource()->isDone())
//	{
//		sim_mob::AMOD::AMODController *a = sim_mob::AMOD::AMODController::instance();
//		a->handleVHArrive(this);
//	}
//	eventPub.publish(sim_mob::event::EVT_AMOD_ARRIVING_TO_DEST, a, arg);
	return retVal;
}



void sim_mob::Person::frame_output(timeslice now)
{
	//Save the output
	if (!isToBeRemoved()) {
		currRole->Movement()->frame_tick_output();
	}

	//avoiding logical errors while improving the code
	resetParamsRequired = true;
}



bool sim_mob::Person::updateOD(sim_mob::TripChainItem * tc, const sim_mob::SubTrip *subtrip)
{
	return tc->setPersonOD(this, subtrip);
}

bool sim_mob::Person::changeRoleRequired(sim_mob::Role & currRole,sim_mob::SubTrip &currSubTrip) const
{
	string roleName = RoleFactory::GetSubTripMode(currSubTrip);
	const RoleFactory& rf = ConfigManager::GetInstance().FullConfig().getRoleFactory();
	const sim_mob::Role* targetRole = rf.getPrototype(roleName);
	if(targetRole->getRoleName() ==  currRole.getRoleName()) {
		return false;
	}
	//the current role type and target(next) role type are not same. so we need to change the role!
	return true;
}

bool sim_mob::Person::changeRoleRequired(sim_mob::TripChainItem &tripChinItem) const
{
	if(tripChinItem.itemType == sim_mob::TripChainItem::IT_TRIP)
		return changeRoleRequired_Trip();
		return changeRoleRequired_Activity();
}
bool sim_mob::Person::changeRoleRequired_Trip(/*sim_mob::Trip &trip*/) const
{
	//std::cout << "Checking if the change is required from currRole[" << currRole << "]: "<< currRole->getRoleName() << std::endl;
	string roleName = RoleFactory::GetSubTripMode(*currSubTrip);
	//std::cout << "Person::changeRoleRequired_Trip->roleName = " << roleName << std::endl;
	const RoleFactory& rf = ConfigManager::GetInstance().FullConfig().getRoleFactory();
	const sim_mob::Role* targetRole = rf.getPrototype(roleName);
	//std::cout << " and targetRole->getRoleName() will be " << targetRole->getRoleName() << " vs curr:" << currRole->getRoleName()<< std::endl;
	if(targetRole->getRoleName() ==  currRole->getRoleName())
		return false;
	//the current role type and target(next) role type are not same. so we need to change the role!
	return true;
}

bool sim_mob::Person::changeRoleRequired_Activity(/*sim_mob::Activity &activity*/) const
{
	return true;
}

bool sim_mob::Person::findPersonNextRole()
{
	if(!updateNextTripChainItem())
	{
		safe_delete_item(nextRole);
		return false;
	}
	//Prepare to delete the previous Role. We _could_ delete it now somewhat safely, but
	// it's better to avoid possible errors (e.g., if the equality operator is defined)
	// by saving it until the next time tick.
	//safe_delete_item(prevRole);
	safe_delete_item(nextRole);
	const RoleFactory& rf = ConfigManager::GetInstance().FullConfig().getRoleFactory();

	const sim_mob::TripChainItem* tci = *(this->nextTripChainItem);
	const sim_mob::SubTrip* str = (tci->itemType == sim_mob::TripChainItem::IT_TRIP ? &(*nextSubTrip) : 0);

	nextRole = rf.createRole(tci, str, this);
	return true;
}

bool sim_mob::Person::updatePersonRole(sim_mob::Role* newRole)
{
	if(!((!currRole) ||(changeRoleRequired(*(*(this->currTripChainItem)))))) {
		return false;
	}
	//Prepare to delete the previous Role. We _could_ delete it now somewhat safely, but
	// it's better to avoid possible errors (e.g., if the equality operator is defined)
	// by saving it until the next time tick.
	safe_delete_item(prevRole);
	const RoleFactory& rf = ConfigManager::GetInstance().FullConfig().getRoleFactory();
//		prevRole = currRole;

	const sim_mob::TripChainItem* tci = *(this->currTripChainItem);

	const sim_mob::SubTrip* str = 0;
	if( tci->itemType==sim_mob::TripChainItem::IT_TRIP || tci->itemType==sim_mob::TripChainItem::IT_FMODSIM )
		str =  &(*currSubTrip);

	if(newRole == 0)
		newRole = rf.createRole(tci, str, this);

	changeRole(newRole);
	return true;
}

UpdateStatus sim_mob::Person::checkTripChain(uint32_t currTimeMS) {
	//some normal checks
	if(tripChain.size() < 1) {
		return UpdateStatus::Done;
	}

	//advance the trip, subtrip or activity....
	if(!first_update_tick) {
		if(!(advanceCurrentTripChainItem())) {
			return UpdateStatus::Done;
		}
	}

	first_update_tick = false;

	//must be set to false whenever tripchainitem changes. And it has to happen before a probable creation of (or changing to) a new role
	setNextPathPlanned(false);

	//Create a new Role based on the trip chain type
	updatePersonRole(nextRole);

	//Update our origin/dest pair.
	if((*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_TRIP) { //put if to avoid & evade bustrips, can be removed when everything is ok
		updateOD(*currTripChainItem, &(*currSubTrip));
	}

	//currentTipchainItem or current subtrip are changed
	//so OD will be changed too,
	//therefore we need to call frame_init regardless of change in the role
	resetFrameInit();

	//Create a return type based on the differences in these Roles
	vector<BufferedBase*> prevParams;
	vector<BufferedBase*> currParams;
	if (prevRole) {
		prevParams = prevRole->getSubscriptionParams();
	}
	if (currRole) {
		currParams = currRole->getSubscriptionParams();
	}

	//Null out our trip chain, remove the "removed" flag, and return
	clearToBeRemoved();
	return UpdateStatus(UpdateStatus::RS_CONTINUE, prevParams, currParams);

}

//sets the current subtrip to the first subtrip of the provided trip(provided trip is usually the current tripChianItem)
std::vector<sim_mob::SubTrip>::iterator sim_mob::Person::resetCurrSubTrip()
{
	sim_mob::Trip *trip = dynamic_cast<sim_mob::Trip *>(*currTripChainItem);
	if(!trip) {
		throw std::runtime_error("non sim_mob::Trip cannot have subtrips");
	}
	return trip->getSubTripsRW().begin();
}

void sim_mob::Person::simplyModifyTripChain(std::vector<TripChainItem*>& tripChain)
{
	std::vector<TripChainItem*>::iterator tripChainItem;
	for(tripChainItem = tripChain.begin(); tripChainItem != tripChain.end(); tripChainItem++ )
	{
		if((*tripChainItem)->itemType == sim_mob::TripChainItem::IT_TRIP )
		{
			std::vector<SubTrip>::iterator subChainItem1, subChainItem2;
			std::vector<sim_mob::SubTrip>& subtrip = (dynamic_cast<sim_mob::Trip*>(*tripChainItem))->getSubTripsRW();
			for(subChainItem1 = subtrip.begin();subChainItem1!=subtrip.end(); subChainItem1++)
			{
				//std::cout << "first item  " << subChainItem1->fromLocation.getID() << " " <<subChainItem1->toLocation.getID() <<" mode " <<subChainItem1->mode << std::endl;
			}
			subChainItem2 = subChainItem1 = subtrip.begin();
			subChainItem2++;
			vector<SubTrip> newsubchain;
			newsubchain.push_back(*subChainItem1);
			while(subChainItem1!=subtrip.end() && subChainItem2!=subtrip.end() )
			{
				//std::cout << "first item  " << subChainItem1->fromLocation.getID() << " " <<subChainItem1->toLocation.getID() << std::endl;
				//std::cout << "second item  " << subChainItem2->fromLocation.getID() << " " <<subChainItem2->toLocation.getID() << std::endl;

				WayPoint source, destination;
				if( (subChainItem1->mode=="Walk") && (subChainItem2->mode=="BusTravel") )
				{
					BusStopFinder finder(subChainItem2->fromLocation.node_, subChainItem2->toLocation.node_);
					if(finder.getSourceBusStop())
					{
						source = subChainItem1->toLocation;
						destination = WayPoint(finder.getSourceBusStop());
					}
				}
				else if((subChainItem2->mode=="Walk") && (subChainItem1->mode=="BusTravel"))
				{
					BusStopFinder finder(subChainItem1->fromLocation.node_, subChainItem1->toLocation.node_);
					if(finder.getSourceBusStop() && finder.getDestinationBusStop())
					{
						source = WayPoint(finder.getDestinationBusStop());
						destination = subChainItem2->fromLocation;
					}
				}
				if(source.type_!=WayPoint::INVALID && destination.type_!=WayPoint::INVALID )
				{
					sim_mob::SubTrip subTrip;
					subTrip.setPersonID(-1);
					subTrip.itemType = TripChainItem::getItemType("Trip");
					subTrip.sequenceNumber = 1;
					subTrip.startTime = subChainItem1->endTime;
					subTrip.endTime = subChainItem1->endTime;
					subTrip.fromLocation = source;
					subTrip.fromLocationType = subChainItem1->fromLocationType;
					subTrip.toLocation = destination;
					subTrip.toLocationType = subChainItem2->toLocationType;
					subTrip.tripID = "";
					subTrip.mode = "Walk";
					subTrip.isPrimaryMode = true;
					subTrip.ptLineId = "";

					//subtrip.insert(subChainItem2, subTrip);

					//if(destination.type_==WayPoint::BUS_STOP )
					//	subChainItem2++;
					//else if(source.type_==WayPoint::BUS_STOP)
					//	subChainItem2->fromLocation = source;

					if(destination.type_==WayPoint::BUS_STOP )
						subChainItem1->toLocation = destination;
					else if(source.type_==WayPoint::BUS_STOP)
						subChainItem2->fromLocation = source;

					newsubchain.push_back(subTrip);
				}

				newsubchain.push_back(*subChainItem2);
				subChainItem1 = subChainItem2;
				subChainItem2++;

				if(subChainItem1==subtrip.end() || subChainItem2==subtrip.end())
					break;
			}

			if(newsubchain.size()>2)
			{
				std::vector<SubTrip>::iterator subChainItem;
				/*subtrip.clear();
				for(subChainItem = newsubchain.begin();subChainItem!=newsubchain.end(); subChainItem++)
				{
					subtrip.push_back(*subChainItem);
				}*/

				for(subChainItem = subtrip.begin();subChainItem!=subtrip.end(); subChainItem++)
				{
					//std::cout << "first item  " << subChainItem->fromLocation.getID() << " " <<subChainItem->toLocation.getID() <<" mode " <<subChainItem->mode << std::endl;
				}
			}
		}
	}
}

bool sim_mob::Person::insertTripBeforeCurrentTrip(Trip* newone)
{
	bool ret = false;
	if(dynamic_cast<sim_mob::Trip*>(*currTripChainItem))
	{
		currTripChainItem = tripChain.insert(currTripChainItem, newone);
		ret = true;
	}
	return ret;
}
bool sim_mob::Person::insertSubTripBeforeCurrentSubTrip(SubTrip* newone)
{
	bool ret = false;
	if(dynamic_cast<sim_mob::Trip*>(*currTripChainItem))
	{
		std::vector<sim_mob::SubTrip>& subtrip = (dynamic_cast<sim_mob::Trip*>(*currTripChainItem))->getSubTripsRW();
		currSubTrip = subtrip.insert(currSubTrip, *newone);
		ret = true;
	}
	return ret;
}

//only affect items after current trip chain item
bool sim_mob::Person::insertATripChainItem(TripChainItem* before, TripChainItem* newone)
{
	bool ret = false;
	if((dynamic_cast<SubTrip*>(newone)))
	{
		sim_mob::SubTrip* before2 = dynamic_cast<sim_mob::SubTrip*> (before);
		if(before2)
		{
			std::vector<sim_mob::SubTrip>& subtrip = (dynamic_cast<sim_mob::Trip*>(*currTripChainItem))->getSubTripsRW();
			std::vector<SubTrip>::iterator itfinder2 = currSubTrip++;
			itfinder2 = std::find(currSubTrip, subtrip.end(), *before2);
			if( itfinder2 != subtrip.end())
			{
				sim_mob::SubTrip* newone2 = dynamic_cast<sim_mob::SubTrip*> (newone);
				subtrip.insert(itfinder2, *newone2);
			}

		}
	}
	else if((dynamic_cast<Trip*>(newone)))
	{
		std::vector<TripChainItem*>::iterator itfinder;
		itfinder = std::find(currTripChainItem, tripChain.end(), before);
		if(itfinder!=currTripChainItem && itfinder!=tripChain.end())
		{
			tripChain.insert(itfinder, newone );
			ret = true;
		}
	}

	return ret;
}

bool sim_mob::Person::deleteATripChainItem(TripChainItem* del)
{
	bool ret = false;
	std::vector<TripChainItem*>::iterator itfinder;
	itfinder = std::find(currTripChainItem, tripChain.end(), del);
	if(itfinder!=currTripChainItem && itfinder!=tripChain.end())
	{
		tripChain.erase(itfinder);
		ret = true;
	}
	else if((dynamic_cast<SubTrip*>(del)))
	{
		std::vector<TripChainItem*>::iterator itfinder = currTripChainItem;
		for(itfinder++; itfinder!=tripChain.end(); itfinder++)
		{
			std::vector<sim_mob::SubTrip>& subtrip = (dynamic_cast<sim_mob::Trip*>(*currTripChainItem))->getSubTripsRW();
			SubTrip temp = *(dynamic_cast<SubTrip*>(del));
			std::vector<SubTrip>::iterator itfinder2 = std::find(subtrip.begin(), subtrip.end(), temp);
			if(itfinder2!=subtrip.end())
			{
				subtrip.erase(itfinder2);
				ret = true;
				break;
			}
		}
	}
	return ret;
}

bool sim_mob::Person::replaceATripChainItem(TripChainItem* rep, TripChainItem* newone)
{
	bool ret = false;
	std::vector<TripChainItem*>::iterator itfinder;
	itfinder = std::find(currTripChainItem, tripChain.end(), rep);
	if(itfinder!=currTripChainItem && itfinder!=tripChain.end())
	{
		(*itfinder) = newone;
		ret = true;
	}
	return ret;

}

bool sim_mob::Person::updateNextSubTrip()
{
	sim_mob::Trip *trip = dynamic_cast<sim_mob::Trip *>(*currTripChainItem);
	if(!trip) return false;
	if (currSubTrip == trip->getSubTrips().end())//just a routine check
		return false;

	nextSubTrip = currSubTrip + 1;

	if (nextSubTrip == trip->getSubTrips().end())
		return false;
	return true;
}

bool sim_mob::Person::updateNextTripChainItem()
{
	bool res = false;
	if(currTripChainItem == tripChain.end()) return false; //just a harmless basic check
	if((*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_TRIP)
	{
		//dont advance to next tripchainItem immediately, check the subtrip first
		res = updateNextSubTrip();
	}

	if(res)
	{
		nextTripChainItem = currTripChainItem;
		return res;
	}

	//no, it is not the subtrip we need to advance, it is the tripchain item
	nextTripChainItem = currTripChainItem + 1;
	if(nextTripChainItem != tripChain.end())
		if((*nextTripChainItem)->itemType == sim_mob::TripChainItem::IT_ACTIVITY) {
			//	std::cout << "processing Activity";
		}
	//but tripchainitems are also over, get out !
	if(nextTripChainItem == tripChain.end()) return false;

	//so far, advancing the tripchainitem has been successful

	//Also set the currSubTrip to the beginning of trip , just in case
	if((*nextTripChainItem)->itemType == sim_mob::TripChainItem::IT_TRIP)
	{
		sim_mob::Trip *trip = dynamic_cast<sim_mob::Trip *>(*nextTripChainItem);//easy reading
			if(!trip) throw std::runtime_error("non sim_mob::Trip cannot have subtrips");
			nextSubTrip =  trip->getSubTrips().begin();
	}

	return true;
}

//advance to the next subtrip inside the current TripChainItem
bool sim_mob::Person::advanceCurrentSubTrip()
{
	sim_mob::Trip *trip = dynamic_cast<sim_mob::Trip *>(*currTripChainItem);
	if(!trip) {
		return false;
	}

	if (currSubTrip == trip->getSubTripsRW().end()) /*just a routine check*/ {
		return false;
	}

	currSubTrip++;

	if (currSubTrip == trip->getSubTripsRW().end()) {
		return false;
	}

	return true;
}

////advance to the next subtrip inside the current TripChainItem assuming that the current TripChainItem is an activity
//bool sim_mob::Person::advanceCurrentTripChainItem_Activity()
//{
//
//}

bool sim_mob::Person::advanceCurrentTripChainItem()
{
	bool res = false;
	if(currTripChainItem == tripChain.end()) /*just a harmless basic check*/ {
		return false;
	}
	//first check if you just need to advance the subtrip
	if((*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_TRIP || (*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_FMODSIM )
	{
		//don't advance to next tripchainItem immediately, check the subtrip first
		res = advanceCurrentSubTrip();
	}

	if(res) {
		return res;
	}

	//no, it is not the subtrip we need to advance, it is the tripchain item
	currTripChainItem++;
	if (currTripChainItem == tripChain.end())  {
		//but tripchain items are also over, get out !
		return false;
	}

	//so far, advancing the tripchainitem has been successful
	if((*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_ACTIVITY) {
	}

	//Also set the currSubTrip to the beginning of trip , just in case
	if((*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_TRIP  || (*currTripChainItem)->itemType == sim_mob::TripChainItem::IT_FMODSIM) {
		currSubTrip = resetCurrSubTrip();
	}
	return true;
}

void sim_mob::Person::buildSubscriptionList(vector<BufferedBase*>& subsList) {
	//First, add the x and y co-ordinates
	Agent::buildSubscriptionList(subsList);

	//Now, add our own properties.
	if (this->getRole()) {
		vector<BufferedBase*> roleParams = this->getRole()->getSubscriptionParams();
		for (vector<BufferedBase*>::iterator it = roleParams.begin(); it != roleParams.end(); it++) {
			subsList.push_back(*it);
		}
	}
}

//Role changing should always be done through changeRole, because it also manages subscriptions.
//TODO: Currently, this is also done via the return value to worker. Probably best not to have
//      the same code in two places.
void sim_mob::Person::changeRole(sim_mob::Role* newRole) {
	if (currRole) {
		currRole->setParent(nullptr);
		if (this->currWorkerProvider) {
			this->currWorkerProvider->stopManaging(currRole->getSubscriptionParams());
			this->currWorkerProvider->stopManaging(currRole->getDriverRequestParams().asVector());
		}
	}
	prevRole = currRole;
	currRole = newRole;

	if (currRole) {
		currRole->setParent(this);
		if (this->currWorkerProvider) {
			this->currWorkerProvider->beginManaging(currRole->getSubscriptionParams());
			this->currWorkerProvider->beginManaging(currRole->getDriverRequestParams().asVector());
		}
	}
}

sim_mob::Role* sim_mob::Person::getRole() const {
	return currRole;
}

void sim_mob::Person::setNextRole(sim_mob::Role* newRole)
{
	nextRole = newRole;
}

sim_mob::Role* sim_mob::Person::getNextRole() const {
	return nextRole;
}

void sim_mob::Person::setPersonCharacteristics()
{
	boost::mt19937 gen(static_cast<unsigned int>(getId()*getId()));
	boost::uniform_int<> ageRange(20, 60);
	boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varAge(gen, ageRange);
	age = (unsigned int)varAge();
	if(age >= 20 && age < 30)
	{
		boost::uniform_int<> BoardingTime(3, 7);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varBoardingTime(gen, BoardingTime);
		boardingTimeSecs = varBoardingTime();

		boost::uniform_int<> AlightingTime(3, 7);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varAlightingTime(gen, AlightingTime);
		alightingTimeSecs = varAlightingTime();
	}
	if(age >=30 && age < 40)
	{
		boost::uniform_int<> BoardingTime(4, 8);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varBoardingTime(gen, BoardingTime);
		boardingTimeSecs = varBoardingTime();

		boost::uniform_int<> AlightingTime(4, 8);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varAlightingTime(gen, AlightingTime);
		alightingTimeSecs = varAlightingTime();
	}
	if(age >= 40 && age < 50)
	{
		boost::uniform_int<> BoardingTime(5, 9);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varBoardingTime(gen, BoardingTime);
		boardingTimeSecs = varBoardingTime();

		boost::uniform_int<> AlightingTime(5, 9);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varAlightingTime(gen, AlightingTime);
		alightingTimeSecs = varAlightingTime();
	}
	if(age >= 50 && age <= 60)
	{
		boost::uniform_int<> BoardingTime(6, 10);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varBoardingTime(gen, BoardingTime);
		boardingTimeSecs = varBoardingTime();

		boost::uniform_int<> AlightingTime(6, 10);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varAlightingTime(gen, AlightingTime);
		alightingTimeSecs = varAlightingTime();
	}
}
