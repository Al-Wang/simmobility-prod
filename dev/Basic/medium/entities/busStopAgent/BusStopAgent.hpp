//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * BusStopAgent.hpp
 *
 *  Created on: 17 Apr, 2014
 *      Author: zhang huai peng
 */

#pragma once

#include "entities/Agent.hpp"
#include "entities/Person.hpp"
#include "entities/roles/driver/BusDriver.hpp"

namespace sim_mob {

namespace medium {

class BusStopAgent: public sim_mob::Agent {
public:
	BusStopAgent(const MutexStrategy& mtxStrat, int id, const sim_mob::BusStop* stop);
	virtual ~BusStopAgent();

protected:

	//Virtual overrides
	virtual bool frame_init(timeslice now) {
		throw std::runtime_error(
				"frame_* methods are not required and are not implemented for BusStopAgent.");
	}
	virtual Entity::UpdateStatus frame_tick(timeslice now) {
		throw std::runtime_error(
				"frame_* are not required and are not implemented for BusStopAgent.");
	}
	virtual void frame_output(timeslice now) {
		throw std::runtime_error(
				"frame_* methods are not required and are not implemented for BusStopAgent.");
	}

	/**
	 * Inherited from Agent.
	 */
	virtual void onEvent(event::EventId eventId, sim_mob::event::Context ctxId,
			event::EventPublisher* sender, const event::EventArgs& args);

	/**
	 * Inherited from MessageHandler.
	 */
	 virtual void HandleMessage(messaging::Message::MessageType type, const messaging::Message& message);

public:
	/**
	 * register a new waiting person.
	 * @param person person who wants to enter this bus stop
	 */
	void registerWaitingPerson(sim_mob::Person* person);

	/**
	 * remove a waiting people from this bus stop.
	 * @param person person to be removed from this bus stop
	 */
	void removeWaitingPerson(sim_mob::Person* person);

	/**
	 * add person who is alighting at this stop
	 * @param person person who is alighting at this bus stop
	 */
	void addAlightingPerson(sim_mob::Person* person);

	/**
	 * the getter of associate bus stop to this agent.
	 * @return bus stop is the associate to this agent
	 */
	const sim_mob::BusStop* getBusStop() const;

	/**
	 * process the persons boarding
	 * @param Bus Driver is the associate driver which waiting people will board
	 */
	void boardWaitingPersons(sim_mob::medium::BusDriver* busDriver);

	/**
	 * get the number of boarding people
	 * @param Bus Driver is the associate driver which waiting people will board
	 */
	int getBoardingNum(sim_mob::medium::BusDriver* busDriver);

private:
	std::list<sim_mob::Person*> waitingPersons;
	std::list<sim_mob::Person*> alightingPersons;
	std::list<sim_mob::medium::BusDriver*> parkingDrivers;
	const sim_mob::BusStop* busStop;

};
}
}

