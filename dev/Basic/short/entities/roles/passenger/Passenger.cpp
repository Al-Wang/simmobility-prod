//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Passenger.hpp"
#include "entities/PT_Statistics.hpp"
#include "entities/roles/driver/Driver.hpp"
#include "message/ST_Message.hpp"
#include "message/ST_Message.hpp"
#include "PassengerFacets.hpp"

using std::vector;
using namespace sim_mob;

Passenger::Passenger(Person_ST *parent, PassengerBehavior *behavior, PassengerMovement *movement, std::string roleName, Role<Person_ST>::Type roleType) :
Role<Person_ST>(parent, behavior, movement, roleName, roleType), driver(nullptr), alightBus(false)
{
}

Role<Person_ST>* Passenger::clone(Person_ST *parent) const
{
	PassengerBehavior* behavior = new PassengerBehavior();
	PassengerMovement* movement = new PassengerMovement();
	Role<Person_ST>::Type personRoleType = Role<Person_ST>::RL_UNKNOWN;
	
	if (parent->currSubTrip->getMode() == "BusTravel")
	{
		personRoleType = Role<Person_ST>::RL_PASSENGER;
	}
	else
	{
		throw std::runtime_error("Unknown mode for passenger role");
	}
	
	Passenger *passenger = new Passenger(parent, behavior, movement, "Passenger_", personRoleType);
	behavior->setParentPassenger(passenger);
	movement->setParentPassenger(passenger);
	
	return passenger;
}

std::vector<BufferedBase*> Passenger::getSubscriptionParams()
{
	return vector<BufferedBase*>();
}

void Passenger::makeAlightingDecision(const BusStop *nextStop)
{
	if (parent->destNode.type == WayPoint::BUS_STOP && parent->destNode.busStop == nextStop)
	{
		setAlightBus(true);
		setDriver(nullptr);
	}
}

void Passenger::collectTravelTime()
{
	PersonTravelTime personTravelTime;
	personTravelTime.personId = parent->getDatabaseId();
	personTravelTime.tripStartPoint = (*(parent->currTripChainItem))->startLocationId;
	personTravelTime.tripEndPoint = (*(parent->currTripChainItem))->endLocationId;
	personTravelTime.subStartPoint = parent->currSubTrip->startLocationId;
	personTravelTime.subEndPoint = parent->currSubTrip->endLocationId;
	personTravelTime.subStartType = parent->currSubTrip->startLocationType;
	personTravelTime.subEndType = parent->currSubTrip->endLocationType;
	personTravelTime.mode = parent->currSubTrip->getMode();
	personTravelTime.service = parent->currSubTrip->ptLineId;
	personTravelTime.travelTime = ((double)parent->getRole()->getTravelTime()) / 1000.0; //convert to seconds
	personTravelTime.arrivalTime = DailyTime(parent->getRole()->getArrivalTime()).getStrRepr();
	
	if (roleType == Role<Person_ST>::RL_PASSENGER)
	{
		personTravelTime.mode = "BUS_TRAVEL";
	}

	messaging::MessageBus::PostMessage(PT_Statistics::getInstance(), STORE_PERSON_TRAVEL_TIME,
			messaging::MessageBus::MessagePtr(new PersonTravelTimeMessage(personTravelTime)), true);
}

