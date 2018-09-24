/*
 * SharedController.hpp
 *
 *  Created on: Apr 18, 2017
 *      Author: Akshay Padmanabha
 */

#ifndef SharedController_HPP_
#define SharedController_HPP_

#include <vector>

#include "entities/Agent.hpp"
#include "OnCallController.hpp"

namespace sim_mob
{

class SharedController : public OnCallController
{
public:
    SharedController(const MutexStrategy &mtxStrat, unsigned int computationPeriod, unsigned id, std::string tripSupportMode_,
                     TT_EstimateType ttEstimateType, unsigned maxAggregatedRequests_,bool studyAreaEnabledController_,unsigned int toleratedExtraTime_,unsigned int maxWaitingTime_,bool parkingEnabled)
            : OnCallController(mtxStrat, computationPeriod,MobilityServiceControllerType::SERVICE_CONTROLLER_SHARED, id, tripSupportMode_,
                                                                        ttEstimateType, maxAggregatedRequests_,studyAreaEnabledController,toleratedExtraTime_,maxWaitingTime_,parkingEnabled)
    {
    }

    virtual ~SharedController()
    {
    }

    virtual void checkSequence(const std::string &sequence) const;

    // Inhertis from the parent
    virtual void sendCruiseCommand(const Person *driver, const Node *nodeToCruiseTo, const timeslice currTick) const;

#ifndef NDEBUG
    // Overrides the parent method
    virtual void consistencyChecks(const std::string& label) const;
#endif

protected:

    /**
     * Performs the controller algorithm to assign vehicles to requests
     */
    virtual void computeSchedules();
};
}
#endif /* SharedController_HPP_ */
