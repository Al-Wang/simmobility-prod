//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "WaitBusActivity.hpp"

#include <boost/algorithm/string.hpp>
#include "entities/Person_MT.hpp"
#include "entities/PT_Statistics.hpp"
#include "entities/roles/driver/BusDriver.hpp"
#include "geospatial/network/PT_Stop.hpp"
#include "message/MT_Message.hpp"
#include "WaitBusActivityFacets.hpp"

using std::vector;
using namespace sim_mob;

namespace sim_mob
{

namespace medium
{

sim_mob::medium::WaitBusActivity::WaitBusActivity(Person_MT* parent, sim_mob::medium::WaitBusActivityBehavior* behavior,
		sim_mob::medium::WaitBusActivityMovement* movement, std::string roleName, Role<Person_MT>::Type roleType) :
		sim_mob::Role<Person_MT>::Role(parent, behavior, movement, roleName, roleType), waitingTime(0), stop(nullptr), boardBus(false), failedToBoardCount(0)
{
}

sim_mob::medium::WaitBusActivity::~WaitBusActivity()
{
}

Role<Person_MT>* sim_mob::medium::WaitBusActivity::clone(Person_MT* parent) const
{
	WaitBusActivityBehavior* behavior = new WaitBusActivityBehavior();
	WaitBusActivityMovement* movement = new WaitBusActivityMovement();
	WaitBusActivity* waitBusActivity = new WaitBusActivity(parent, behavior, movement);
	behavior->setParentWaitBusActivity(waitBusActivity);
	movement->setParentWaitBusActivity(waitBusActivity);
	return waitBusActivity;
}

void sim_mob::medium::WaitBusActivity::increaseWaitingTime(unsigned int incrementMs)
{
	waitingTime += incrementMs;
}

void sim_mob::medium::WaitBusActivity::make_frame_tick_params(timeslice now)
{
	getParams().reset(now);
}

void sim_mob::medium::WaitBusActivity::collectTravelTime()
{
	PersonTravelTime personTravelTime;
	std::string personId, tripStartPoint, tripEndPoint, subStartPoint, subEndPoint, subStartType, subEndType, mode, service, arrivaltime, travelTime;
	personTravelTime.personId = parent->getDatabaseId();
	personTravelTime.tripStartPoint = (*(parent->currTripChainItem))->startLocationId;
	personTravelTime.tripEndPoint = (*(parent->currTripChainItem))->endLocationId;
	personTravelTime.subStartPoint = parent->currSubTrip->startLocationId;
	personTravelTime.subEndPoint = parent->currSubTrip->endLocationId;
	personTravelTime.subStartType = parent->currSubTrip->startLocationType;
	personTravelTime.subEndType = parent->currSubTrip->endLocationType;
	personTravelTime.mode = "WAITING_BUS";
	personTravelTime.service = parent->currSubTrip->ptLineId;
	personTravelTime.travelTime = ((double) parent->getRole()->getTravelTime())/1000.0; //convert to seconds
	personTravelTime.arrivalTime = DailyTime(parent->getRole()->getArrivalTime()).getStrRepr();
	messaging::MessageBus::PostMessage(PT_Statistics::getInstance(),
					STORE_PERSON_TRAVEL_TIME, messaging::MessageBus::MessagePtr(new PersonTravelTimeMessage(personTravelTime)), true);
}

void sim_mob::medium::WaitBusActivity::incrementDeniedBoardingCount()
{
	failedToBoardCount++;
}

const std::string sim_mob::medium::WaitBusActivity::getBusLines() const
{
	return parent->currSubTrip->getBusLineID();
}

void sim_mob::medium::WaitBusActivity::makeBoardingDecision(BusDriver* driver)
{
	const std::vector<const sim_mob::BusStop*>* stopsVec = driver->getBusStopsVector();
	if (!stopsVec)
	{
		boardBus = false;
		return;
	}

	const std::string busLineID = driver->getBusLineID();
	sim_mob::SubTrip& subTrip = *(parent->currSubTrip);
	const std::string tripLineID = subTrip.getBusLineID();
	std::vector<std::string> lines;
	boost::split(lines, tripLineID, boost::is_any_of("/"));
	if (std::find(lines.begin(), lines.end(), busLineID) != lines.end())
	{
		boardBus = true;
		return;
	}
	else
	{
		boardBus = false;
		return;
	}

	const sim_mob::BusStop* destStop = nullptr;
	if (parent->destNode.type == WayPoint::BUS_STOP && parent->destNode.busStop)
	{
		destStop = parent->destNode.busStop;
	}

	if (!destStop)
	{
		boardBus = false;
		return;
	}

	std::vector<const sim_mob::BusStop*>::const_iterator itStop = std::find(stopsVec->begin(), stopsVec->end(), destStop);
	if (itStop != stopsVec->end())
	{
		boardBus = true;
	}
}

void sim_mob::medium::WaitBusActivity::HandleParentMessage(messaging::Message::MessageType type, const messaging::Message& message)
{}

std::vector<BufferedBase*> sim_mob::medium::WaitBusActivity::getSubscriptionParams()
{
	return vector<BufferedBase*>();
}
}
}
