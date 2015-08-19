//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Pedestrian.hpp"
#include "PedestrianFacets.hpp"
#include "entities/Person.hpp"
#include "config/MT_Config.hpp"
#include "message/MT_Message.hpp"
#include "entities/PT_Statistics.hpp"
#include "entities/roles/activityRole/ActivityFacets.hpp"

using std::vector;
using namespace sim_mob;

namespace sim_mob {

namespace medium {

sim_mob::medium::Pedestrian::Pedestrian(Person* parent, MutexStrategy mtxStrat,
		sim_mob::medium::PedestrianBehavior* behavior,
		sim_mob::medium::PedestrianMovement* movement,
		std::string roleName, Role::type roleType) :
		sim_mob::Role(behavior, movement, parent, roleName, roleType )
{}

sim_mob::medium::Pedestrian::~Pedestrian() {}

Role* sim_mob::medium::Pedestrian::clone(Person* parent) const {
	double walkSpeed = MT_Config::getInstance().getPedestrianWalkSpeed();
	PedestrianBehavior* behavior = new PedestrianBehavior(parent);
	PedestrianMovement* movement = new PedestrianMovement(parent, walkSpeed);
	Pedestrian* pedestrian = new Pedestrian(parent, parent->getMutexStrategy(), behavior, movement);
	behavior->setParentPedestrian(pedestrian);
	movement->setParentPedestrian(pedestrian);
	return pedestrian;
}

std::vector<BufferedBase*> sim_mob::medium::Pedestrian::getSubscriptionParams() {
	return vector<BufferedBase*>();
}

void sim_mob::medium::Pedestrian::make_frame_tick_params(timeslice now) {}

}

void sim_mob::medium::Pedestrian::collectTravelTime()
{
	PersonTravelTime personTravelTime;
	personTravelTime.personId = boost::lexical_cast<std::string>(parent->getId());
	if(parent->getPrevRole() && parent->getPrevRole()->roleType==Role::RL_ACTIVITY)
	{
		ActivityPerformer* activity = dynamic_cast<ActivityPerformer*>(parent->getPrevRole());
		personTravelTime.tripStartPoint = boost::lexical_cast<std::string>(activity->getLocation()->getID());
		personTravelTime.tripEndPoint = boost::lexical_cast<std::string>(activity->getLocation()->getID());
		personTravelTime.subStartPoint = boost::lexical_cast<std::string>(activity->getLocation()->getID());
		personTravelTime.subEndPoint = boost::lexical_cast<std::string>(activity->getLocation()->getID());
		personTravelTime.subStartType = "NODE";
		personTravelTime.subEndType = "NODE";
		personTravelTime.mode = "ACTIVITY";
		personTravelTime.service = parent->currSubTrip->ptLineId;
		personTravelTime.travelTime = DailyTime(activity->getTravelTime()).getStrRepr();
		personTravelTime.arrivalTime = DailyTime(activity->getArrivalTime()).getStrRepr();
		messaging::MessageBus::PostMessage(PT_Statistics::getInstance(),
				STORE_PERSON_TRAVEL_TIME, messaging::MessageBus::MessagePtr(new PersonTravelTimeMessage(personTravelTime)), true);
	}
	personTravelTime.tripStartPoint = (*(parent->currTripChainItem))->startLocationId;
	personTravelTime.tripEndPoint = (*(parent->currTripChainItem))->endLocationId;
	personTravelTime.subStartPoint = parent->currSubTrip->startLocationId;
	personTravelTime.subEndPoint = parent->currSubTrip->endLocationId;
	personTravelTime.subStartType = parent->currSubTrip->startLocationType;
	personTravelTime.subEndType = parent->currSubTrip->endLocationType;
	personTravelTime.mode = "WALK";
	personTravelTime.service = parent->currSubTrip->ptLineId;
	personTravelTime.travelTime = DailyTime(parent->getRole()->getTravelTime()).getStrRepr();
	personTravelTime.arrivalTime = DailyTime(parent->getRole()->getArrivalTime()).getStrRepr();
	messaging::MessageBus::PostMessage(PT_Statistics::getInstance(),
			STORE_PERSON_TRAVEL_TIME, messaging::MessageBus::MessagePtr(new PersonTravelTimeMessage(personTravelTime)), true);
}
}

