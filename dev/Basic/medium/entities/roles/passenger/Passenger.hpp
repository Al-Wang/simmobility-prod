//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include "entities/roles/Role.hpp"
#include "PassengerFacets.hpp"
#include "entities/roles/waitBusActivity/waitBusActivity.hpp"

namespace sim_mob {

class Agent;
class Person;
class BusStop;

namespace medium {

class PassengerBehavior;
class PassengerMovement;
class Driver;

/**
 * A medium-term Passenger.
 * \author Seth N. Hetu
 * \author zhang huai peng
 */
class Passenger: public sim_mob::Role {
public:

	explicit Passenger(Person* parent, MutexStrategy mtxStrat,
			sim_mob::medium::PassengerBehavior* behavior = nullptr,
			sim_mob::medium::PassengerMovement* movement = nullptr);

	virtual ~Passenger() {
	}

	//Virtual overrides
	virtual sim_mob::Role* clone(sim_mob::Person* parent) const;
	virtual void make_frame_tick_params(timeslice now) {
		throw std::runtime_error(
				"make_frame_tick_params not implemented in Passenger.");
	}
	virtual std::vector<sim_mob::BufferedBase*> getSubscriptionParams() {
		throw std::runtime_error(
				"getSubscriptionParams not implemented in Passenger.");
	}

	void setDriver(const Driver* driver);
	const Driver* getDriver() const;

	/**
	 * make a decision for alighting.
	 * @param nextStop is the next stop which bus will arrive at
	 */
	void makeAlightingDecision(const sim_mob::BusStop* nextStop);

	/**
	 * message handler which provide a chance to handle message transfered from parent agent.
	 * @param type of the message.
	 * @param message data received.
	 */
	virtual void HandleParentMessage(messaging::Message::MessageType type,
			const messaging::Message& message);

	bool canAlightBus() const {
		return alightBus;
	}

	void setAlightBus(bool alightBus) {
		this->alightBus = alightBus;
	}

private:
	friend class PassengerBehavior;
	friend class PassengerMovement;

	/** Driver who is driving the vehicle of this passenger*/
	const Driver* driver;

	/**flag to indicate whether the passenger has decided to alight the bus*/
	bool alightBus;
};

}
}
