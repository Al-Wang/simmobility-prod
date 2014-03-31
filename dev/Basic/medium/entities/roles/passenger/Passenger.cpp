//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)



#include "Passenger.hpp"
#include "PassengerFacets.hpp"
#include "entities/Person.hpp"
#include "../driver/Driver.hpp"

using std::vector;
using namespace sim_mob;

namespace sim_mob {

namespace medium {

sim_mob::medium::Passenger::Passenger(Agent* parent, MutexStrategy mtxStrat,
		sim_mob::medium::PassengerBehavior* behavior,
		sim_mob::medium::PassengerMovement* movement) :
		sim_mob::Role(behavior, movement, parent, "Passenger_"),associateDriver(nullptr) {

}

Role* sim_mob::medium::Passenger::clone(Person* parent) const {

	PassengerBehavior* behavior = new PassengerBehavior(parent);
	PassengerMovement* movement = new PassengerMovement(parent);
	Passenger* passenger = new Passenger(parent, parent->getMutexStrategy(),
			behavior, movement);
	behavior->setParentPassenger(passenger);
	movement->setParentPassenger(passenger);
	return passenger;
}

void sim_mob::medium::Passenger::setAssociateDriver(Driver* driver){
	associateDriver = driver;
}

sim_mob::medium::Driver* sim_mob::medium::Passenger::getAssociateDriver(){
	return associateDriver;
}

}

}

