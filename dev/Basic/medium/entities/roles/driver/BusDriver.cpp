//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * BusDriver.cpp
 *
 *  Created on: May 6, 2013
 *      Author: zhang huai peng
 *      		melani
 */

#include "BusDriver.hpp"
#include "entities/Person.hpp"
#include "entities/BusStopAgent.hpp"
#include "message/MT_Message.hpp"
#include "entities/roles/passenger/Passenger.hpp"
#include "util/DwellTimeCalc.hpp"

using namespace sim_mob;
using std::max;
using std::vector;
using std::set;
using std::map;
using std::string;
using std::endl;

sim_mob::medium::BusDriver::BusDriver(Person* parent, MutexStrategy mtxStrat,
		sim_mob::medium::BusDriverBehavior* behavior,
		sim_mob::medium::BusDriverMovement* movement,
		std::string roleName)
: sim_mob::medium::Driver(parent, mtxStrat, behavior, movement, roleName),
  requestMode(mtxStrat, 0), visitedBusStop(mtxStrat, nullptr),
  visitedBusStopSequenceNo(mtxStrat, -1), arrivalTime(mtxStrat, 0.0),
  dwellTime(mtxStrat, 0.0), visitedBusTripSequenceNo(mtxStrat, 0),
  visitedBusLine(mtxStrat, "0"), holdingTime(mtxStrat, 0.0),
  waitingTimeAtbusStop(0.0)
{}

sim_mob::medium::BusDriver::~BusDriver() {}

Role* sim_mob::medium::BusDriver::clone(Person* parent) const {
	BusDriverBehavior* behavior = new BusDriverBehavior(parent);
	BusDriverMovement* movement = new BusDriverMovement(parent);
	BusDriver* busdriver = new BusDriver(parent, parent->getMutexStrategy(), behavior, movement, "BusDriver_");
	behavior->setParentBusDriver(busdriver);
	movement->setParentBusDriver(busdriver);
	movement->setParentDriver(busdriver);
	return busdriver;
}

const std::vector<const sim_mob::BusStop*>* sim_mob::medium::BusDriver::getBusStopsVector() const {
	const std::vector<const sim_mob::BusStop*>* stopsVec=nullptr;
	sim_mob::Person* person = dynamic_cast<Person*>(parent);
	if(!person){
		return stopsVec;
	}

	const BusTrip* busTrip =dynamic_cast<const BusTrip*>(*(person->currTripChainItem));
	if(!busTrip){
		return stopsVec;
	}
	const BusRouteInfo& routeInfo = busTrip->getBusRouteInfo();
	stopsVec = &(routeInfo.getBusStops());
	return stopsVec;
}

bool sim_mob::medium::BusDriver::addPassenger(sim_mob::medium::Passenger* passenger) {
	passengerList.push_back(passenger);
	return true;
}

sim_mob::DriverRequestParams sim_mob::medium::BusDriver::getDriverRequestParams()
{
	sim_mob::DriverRequestParams res;
	return res;
}

int sim_mob::medium::BusDriver::alightPassenger(sim_mob::medium::BusStopAgent* busStopAgent){
	int numAlighting = 0;
	std::list<sim_mob::medium::Passenger*>::iterator itPassenger = passengerList.begin();
	while (itPassenger != passengerList.end()) {

		messaging::MessageBus::SendInstantaneousMessage((*itPassenger)->getParent(),
				ALIGHT_BUS,
				messaging::MessageBus::MessagePtr(new BusStopMessage(busStopAgent->getBusStop())));

		if ((*itPassenger)->canAlightBus()) {
			busStopAgent->addAlightingPerson(*itPassenger);
			itPassenger = passengerList.erase(itPassenger);
			numAlighting++;
		} else {
			itPassenger++;
		}
	}
	return numAlighting;
}

void sim_mob::medium::BusDriver::predictArrivalAtBusStop(double preArrivalTime,
		sim_mob::medium::BusStopAgent* busStopAgent) {
	sim_mob::Person* person = dynamic_cast<Person*>(parent);
	if (!person) {
		return;
	}

	const BusTrip* busTrip =
			dynamic_cast<const BusTrip*>(*(person->currTripChainItem));
	if (busTrip) {
		const Busline* busLine = busTrip->getBusline();
		visitedBusLine.set(busLine->getBusLineID());
		visitedBusTripSequenceNo.set(busTrip->getBusTripRun_SequenceNum());
		requestMode.set(Role::REQUEST_DECISION_TIME);
		visitedBusStop.set(busStopAgent->getBusStop());
		visitedBusStopSequenceNo.set(visitedBusStopSequenceNo.get() + 1);
		arrivalTime.set(preArrivalTime);
	}
}

void sim_mob::medium::BusDriver::openBusDoors(
		sim_mob::medium::BusStopAgent* busStopAgent) {
	messaging::MessageBus::SendInstantaneousMessage(busStopAgent, BOARD_BUS,
			messaging::MessageBus::MessagePtr(new BusDriverMessage(this)));

	int numAlighting = alightPassenger(busStopAgent);
	int numBoarding = busStopAgent->getBoardingNum(this);

	int totalNumber = numAlighting + numBoarding;
	waitingTimeAtbusStop = sim_mob::dwellTimeCalculation(totalNumber);

	if (requestMode.get() == Role::REQUEST_DECISION_TIME) {
		requestMode.set(Role::REQUEST_NONE);
		//final waiting time is maximum value between dwelling time and holding time
		waitingTimeAtbusStop = std::max(waitingTimeAtbusStop, holdingTime.get());
		holdingTime.set(0.0);
	}
	currResource->setMoving(false);
}

void sim_mob::medium::BusDriver::closeBusDoors(
		sim_mob::medium::BusStopAgent* busStopAgent) {
	messaging::MessageBus::SendInstantaneousMessage(busStopAgent, BUS_DEPARTURE,
			messaging::MessageBus::MessagePtr(new BusDriverMessage(this)));
	currResource->setMoving(true);
}

std::vector<BufferedBase*> sim_mob::medium::BusDriver::getSubscriptionParams() {
	return vector<BufferedBase*>();
}

void sim_mob::medium::BusDriver::make_frame_tick_params(timeslice now) {
	getParams().reset(now);
}


