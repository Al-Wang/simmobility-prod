/*
 * PassengerFacets.cpp
 *
 *  Created on: Mar 13, 2014
 *      Author: zhang huai peng
 */

#include "PassengerFacets.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "geospatial/network/Node.hpp"
#include "Passenger.hpp"

namespace sim_mob
{
namespace medium
{

PassengerBehavior::PassengerBehavior(sim_mob::Person* parentAgent) : BehaviorFacet(parentAgent), parentPassenger(nullptr)
{}

PassengerBehavior::~PassengerBehavior() {}

PassengerMovement::PassengerMovement(sim_mob::Person* parentAgent) : MovementFacet(parentAgent), parentPassenger(nullptr), totalTimeToCompleteMS(0)
{}

PassengerMovement::~PassengerMovement() {}

void PassengerMovement::setParentPassenger(sim_mob::medium::Passenger* parentPassenger)
{
	this->parentPassenger = parentPassenger;
}

void PassengerBehavior::setParentPassenger(sim_mob::medium::Passenger* parentPassenger)
{
	this->parentPassenger = parentPassenger;
}

void PassengerMovement::frame_init()
{
	totalTimeToCompleteMS = 0;
}

void PassengerMovement::frame_tick()
{
	unsigned int tickMS = ConfigManager::GetInstance().FullConfig().baseGranMS();
	totalTimeToCompleteMS += tickMS;
	parentPassenger->setTravelTime(totalTimeToCompleteMS);
}

void PassengerMovement::frame_tick_output() {}


TravelMetric & PassengerMovement::startTravelTimeMetric()
{
	travelMetric.startTime = DailyTime(parentPassenger->getArrivalTime());
	travelMetric.origin = WayPoint(parentPassenger->getStartNode());
	travelMetric.started = true;
	return travelMetric;
}

TravelMetric & PassengerMovement::finalizeTravelTimeMetric()
{
	travelMetric.destination = WayPoint(parentPassenger->getEndNode());
	travelMetric.endTime = DailyTime(parentPassenger->getArrivalTime() + totalTimeToCompleteMS);
	travelMetric.travelTime = TravelMetric::getTimeDiffHours(travelMetric.endTime , travelMetric.startTime); // = totalTimeToCompleteMS in hours
	travelMetric.finalized = true;
	return travelMetric;
}
sim_mob::Conflux* PassengerMovement::getStartingConflux() const
{
	if (parentPassenger->roleType == Role::RL_CARPASSENGER)
	{
		const Node* location = dynamic_cast<const Node*>(parentPassenger->parent->currSubTrip->toLocation.node_);
		if (location)
		{
			return ConfigManager::GetInstanceRW().FullConfig().getConfluxForNode(location);
		}
	}
	return nullptr;
}
}//medium
} /* namespace sim_mob */
