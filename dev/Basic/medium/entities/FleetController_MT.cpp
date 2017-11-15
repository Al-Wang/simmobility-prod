//Copyright (c) 2015 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "FleetController_MT.hpp"

#include "conf/ConfigManager.hpp"
#include "entities/misc/TaxiTrip.hpp"
#include "entities/Person_MT.hpp"

using namespace std;
using namespace sim_mob;
using namespace medium;

FleetController_MT* FleetController_MT::fleetMgr = nullptr;

FleetController_MT::FleetController_MT() : FleetController()
{
}

FleetController_MT::~FleetController_MT()
{
}

FleetController_MT * FleetController_MT::getInstance()
{
	if(!fleetMgr)
	{
		fleetMgr = new FleetController_MT();
	}

	return fleetMgr;
}

void FleetController_MT::initialise(std::set<sim_mob::Entity *> &agentList)
{
	std::map<unsigned int, MobilityServiceControllerConfig>::const_iterator it = ConfigManager::GetInstance().FullConfig().mobilityServiceController.enabledControllers.begin();

    while(it !=  ConfigManager::GetInstance().FullConfig().mobilityServiceController.enabledControllers.end()) {

        std::pair<std::multimap<unsigned int, FleetItem>::iterator, std::multimap<unsigned int, FleetItem>::iterator> lTaxiFleetIt = taxiFleet.equal_range(
                it->first);

        const unsigned int maxFleetSize = it->second.maxFleetSize;

        std::multimap<unsigned int, FleetItem>::iterator lstart = lTaxiFleetIt.first;

        std::multimap<unsigned int, FleetItem>::iterator lend = lTaxiFleetIt.second;

        unsigned int currTaxi = 0;

        ControllerLog() << "Total number of service vehicles loaded from database: " << taxiFleet.count(it->first)
                        << std::endl;
        ControllerLog() << "Max. fleet size configured: " << maxFleetSize << std::endl;

        while (lstart != lend && currTaxi < maxFleetSize) {

                if ((*lstart).second.startNode) {
                    Person_MT *person = new Person_MT("FleetController",
                                                      ConfigManager::GetInstance().FullConfig().mutexStategy(), -1);
                    person->setServiceVehicle((*lstart).second);
                    person->setDatabaseId((*lstart).second.driverId);
                    person->setPersonCharacteristics();

                    string tripType;

                    MobilityServiceControllerType type = it->second.type;

                    switch (type) {
                        case SERVICE_CONTROLLER_ON_HAIL:
                            tripType = "OnHailTrip";
                            break;

                        case SERVICE_CONTROLLER_GREEDY:
                        case SERVICE_CONTROLLER_SHARED:
                        case SERVICE_CONTROLLER_FRAZZOLI:
                        case SERVICE_CONTROLLER_INCREMENTAL:
                        case SERVICE_CONTROLLER_PROXIMITY:
                            tripType = "OnCallTrip";
                            break;

                        default:
                            tripType = "TaxiTrip";
                    }

                    vector<TripChainItem *> tripChain;
                    TaxiTrip *taxiTrip = new TaxiTrip("0", tripType, 0, -1,
                                                      DailyTime((*lstart).second.startTime * 1000.0),
                                                      DailyTime((*lstart).second.endTime * 1000), 0,
                                                      (*lstart).second.startNode,
                                                      "node", nullptr, "node");
                    tripChain.push_back((TripChainItem *) taxiTrip);
                    person->setTripChain(tripChain);

                    addOrStashTaxis(person, agentList);

                    //Valid vehicle loaded

                }
                else {
                    Warn() << "Vehicle " << (*lstart).second.vehicleNo << ", with driver " << (*lstart).second.driverId
                           << " has invalid start node.";
                }

                currTaxi++;
                lstart++;
        }
        it++;
    }
}

void FleetController_MT::addOrStashTaxis(Person *person, std::set<Entity *> &activeAgents)
{
	if (person->getStartTime() == 0)
	{
		//Only agents with a start time of zero should start immediately in the all_agents list.
		activeAgents.insert((Entity *)person);
	}
	else
	{
		//Start later.
		pendingChildren.push((Entity *)person);
	}
}
