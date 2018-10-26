/*
 * WaitBusActivityFacets.cpp
 *
 *  Created on: Mar 13, 2014
 *      Author: zhang huai peng
 */

#include "WaitBusActivityFacets.hpp"

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "entities/BusStopAgent.hpp"
#include "geospatial/network/PT_Stop.hpp"
#include "WaitBusActivity.hpp"

using namespace sim_mob;
using namespace sim_mob::medium;

WaitBusActivityBehavior::WaitBusActivityBehavior() :
BehaviorFacet(), parentWaitBusActivity(nullptr)
{
}

WaitBusActivityBehavior::~WaitBusActivityBehavior()
{
}

WaitBusActivityMovement::WaitBusActivityMovement() :
MovementFacet(), parentWaitBusActivity(nullptr)
{
}

WaitBusActivityMovement::~WaitBusActivityMovement()
{
}

void WaitBusActivityMovement::setParentWaitBusActivity(sim_mob::medium::WaitBusActivity* parentWaitBusActivity)
{
    this->parentWaitBusActivity = parentWaitBusActivity;
}

void WaitBusActivityBehavior::setParentWaitBusActivity(sim_mob::medium::WaitBusActivity* parentWaitBusActivity)
{
    this->parentWaitBusActivity = parentWaitBusActivity;
}

void WaitBusActivityMovement::frame_init()
{
    if(parentWaitBusActivity)
    {
        UpdateParams& params = parentWaitBusActivity->getParams();
        Person* person = parentWaitBusActivity->parent;
        person->setStartTime(params.now.ms());
    }
}

void WaitBusActivityMovement::frame_tick()
{
    unsigned int tickMS = ConfigManager::GetInstance().FullConfig().baseGranMS();
    if(parentWaitBusActivity)
    {
        parentWaitBusActivity->increaseWaitingTime(tickMS);
        parentWaitBusActivity->setTravelTime(parentWaitBusActivity->getWaitingTime());
    }
    parentWaitBusActivity->parent->setRemainingTimeThisTick(0);
}

std::string WaitBusActivityMovement::frame_tick_output()
{
    return std::string();
}

Conflux* WaitBusActivityMovement::getStartingConflux() const
{
    const sim_mob::medium::BusStopAgent* stopAg = sim_mob::medium::BusStopAgent::getBusStopAgentForStop(parentWaitBusActivity->getStop());
    if(stopAg)
    {
        return stopAg->getParentSegmentStats()->getParentConflux();
    }
    return nullptr;
}

TravelMetric& WaitBusActivityMovement::startTravelTimeMetric()
{
    return travelMetric;
}

TravelMetric& WaitBusActivityMovement::finalizeTravelTimeMetric()
{
    return travelMetric;
}
