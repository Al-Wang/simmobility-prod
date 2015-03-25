//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "RawConfigParams.hpp"

using namespace sim_mob;

sim_mob::RawConfigParams::RawConfigParams() : cbd(false)
{}

sim_mob::EntityTemplate::EntityTemplate() : startTimeMs(0), laneIndex(0),originNode(-1),destNode(-1),initSegId(-1),initDis(-1),initSpeed(0),angentId(-1)
{}

sim_mob::SystemParams::SystemParams() : singleThreaded(false), mergeLogFiles(false), networkSource(NETSRC_XML)
{}

sim_mob::WorkerParams::Worker::Worker() : count(0), granularityMs(0)
{}

sim_mob::SimulationParams::SimulationParams() :
	baseGranMS(0), baseGranSecond(0), totalRuntimeMS(0), totalWarmupMS(0), auraManagerImplementation(AuraManager::IMPL_RSTAR),
	workGroupAssigmentStrategy(WorkGroup::ASSIGN_ROUNDROBIN), partitioningSolutionId(0), startingAutoAgentID(0),
	mutexStategy(MtxStrat_Buffered), passenger_distribution_busstop(0),
    passenger_mean_busstop(0), passenger_standardDev_busstop(0), passenger_percent_boarding(0),
    passenger_percent_alighting(0), passenger_min_uniform_distribution(0), passenger_max_uniform_distribution(0)
{}


sim_mob::LongTermParams::LongTermParams(): enabled(false), workers(0), days(0), tickStep(0), maxIterations(0){}
sim_mob::LongTermParams::DeveloperModel::DeveloperModel(): enabled(false), timeInterval(0), initialPostcode(0) {}
sim_mob::LongTermParams::HousingModel::HousingModel(): enabled(false), timeInterval(0), timeOnMarket(0), timeOffMarket(0), initialHouseholdsOnMarket(0), vacantUnitActivationProbability(0),
													   housingMarketSearchPercentage(0), housingMoveInDaysInterval(0){}
sim_mob::LongTermParams::VehicleOwnershipModel::VehicleOwnershipModel():enabled(false), vehicleBuyingWaitingTimeInDays(0){}
