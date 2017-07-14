/*
 * IncrementalSharing.h
 *
 *  Created on: 14 Jul 2017
 *      Author: araldo
 */

#ifndef ENTITIES_CONTROLLERS_INCREMENTALSHARING_H_
#define ENTITIES_CONTROLLERS_INCREMENTALSHARING_H_


namespace sim_mob
{

class IncrementalSharing : public OnCallController {
public:
	IncrementalSharing
		(const MutexStrategy& mtxStrat, unsigned int computationPeriod, unsigned id) :
		OnCallController(mtxStrat, computationPeriod, MobilityServiceControllerType::SERVICE_CONTROLLER_INCREMENTAL, id)
	{
	}

	virtual void checkSequence (const std::string& sequence) const;

	// Inherits from the parent
	virtual void sendCruiseCommand(const Person* driver, const Node* nodeToCruiseTo, const timeslice currTick ) const;



#ifndef NDEBUG
	// Overrides the parent method
	virtual void consistencyChecks(const std::string& label) const;
#endif

protected:
	virtual bool isCruising(Person* p);
	virtual const Node* getCurrentNode(Person* p);

	/**
	 * Performs the controller algorithm to assign vehicles to requests
	 */
	virtual void computeSchedules();
};
}

#endif /* ENTITIES_CONTROLLERS_INCREMENTALSHARING_H_ */
