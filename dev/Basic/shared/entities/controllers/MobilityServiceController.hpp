/*
 * MobilityServiceController.hpp
 *
 *  Created on: Feb 20, 2017
 *      Author: Akshay Padmanabha
 */

#ifndef MobilityServiceController_HPP_
#define MobilityServiceController_HPP_
#include <vector>

#include "entities/Agent.hpp"
#include "message/Message.hpp"
#include "message/MobilityServiceControllerMessage.hpp"
#include "entities/controllers/Rebalancer.hpp"

namespace sim_mob
{



class MobilityServiceController : public Agent {
protected:
	explicit MobilityServiceController(const MutexStrategy& mtxStrat = sim_mob::MtxStrat_Buffered, unsigned int computationPeriod = 0)
		: Agent(mtxStrat, -1), scheduleComputationPeriod(computationPeriod)
	{
		rebalancer = new SimpleRebalancer();
#ifndef NDEBUG
		isComputingSchedules = false;
#endif
	}

public:

	virtual ~MobilityServiceController();

	/**
	 * Signals are non-spatial in nature.
	 */
	bool isNonspatial();

	/*
	 * It returns the pointer to the driver closest to the node
	 */
	const Person* findClosestDriver(const Node* node) const;



protected:
	/**
	 * Inherited from base class agent to initialize parameters
	 */
	Entity::UpdateStatus frame_init(timeslice now);

	/**
	 * Inherited from base class to update this agent
	 */
	Entity::UpdateStatus frame_tick(timeslice now);

	/**
	 * Inherited from base class to output result
	 */
	void frame_output(timeslice now);

	/**
	 * Inherited from base class to handle message
	 */
    void HandleMessage(messaging::Message::MessageType type, const messaging::Message& message);

	/**
	 * Makes a vehicle driver unavailable to the controller
	 * @param person Driver to be removed
	 */
	void driverUnavailable(Person* person);

	/** Store list of subscribed drivers */
	std::vector<Person*> subscribedDrivers;

	/** Store list of available drivers */
	std::vector<Person*> availableDrivers;

	/** Store queue of requests */
	//TODO: It should be vector<const TripRequest>, but it does not compile in that case:
	// check why
	std::list<TripRequestMessage> requestQueue;

	void sendScheduleProposition(const Person* driver, Schedule schedule) const;

	bool isCruising(Person* driver) const;
	const Node* getCurrentNode(Person* driver) const;
	/**
	 * Performs the controller algorithm to assign vehicles to requests
	 */
	virtual void computeSchedules() = 0;


#ifndef NDEBUG
	bool isComputingSchedules; //true during computing schedules. Used for debug purposes
#endif

private:
	/**
	 * Subscribes a vehicle driver to the controller
	 * @param person Driver to be added
	 */
	void subscribeDriver(Person* person);

	/**
	 * Unsubscribes a vehicle driver from the controller
	 * @param person Driver to be removed
	 */
	void unsubscribeDriver(Person* person);

	/**
	 * Makes a vehicle driver available to the controller
	 * @param person Driver to be added
	 */
	void driverAvailable(Person* person);


	/** Keeps track of current local tick */
	unsigned int localTick = 0;

	/** Keeps track of how often to process messages */
	unsigned int scheduleComputationPeriod;

	Rebalancer* rebalancer;
};
}
#endif /* MobilityServiceController_HPP_ */

