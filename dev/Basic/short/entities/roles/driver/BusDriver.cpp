//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "BusDriver.hpp"
#include "BusDriverFacets.hpp"

#include <vector>
#include <iostream>
#include <cmath>

#include "DriverUpdateParams.hpp"
#include "entities/Person.hpp"
#include "entities/vehicle/BusRoute.hpp"
#include "entities/vehicle/Bus.hpp"
#include "entities/BusController.hpp"
#include "entities/BusStopAgent.hpp"
#include "entities/roles/passenger/Passenger.hpp"
#include "logging/Log.hpp"

#include "geospatial/network/Point.hpp"
#include "geospatial/network/BusStop.hpp"
#include "geospatial/network/RoadSegment.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#include "entities/AuraManager.hpp"
#include "util/PassengerDistribution.hpp"
#include "entities/roles/waitBusActivityRole/WaitBusActivityRole.hpp"

using namespace sim_mob;
using std::vector;
using std::map;
using std::string;

BusDriver::BusDriver(Person* parent, MutexStrategy mtxStrat, BusDriverBehavior* behavior, BusDriverMovement* movement, Role::type roleType_) :
Driver(parent, mtxStrat, behavior, movement, roleType_), existed_Request_Mode(mtxStrat, 0), waiting_Time(mtxStrat, 0),
lastVisited_Busline(mtxStrat, "0"), lastVisited_BusTrip_SequenceNo(mtxStrat, 0), lastVisited_BusStop(mtxStrat, nullptr), lastVisited_BusStopSequenceNum(mtxStrat, 0),
real_DepartureTime(mtxStrat, 0), real_ArrivalTime(mtxStrat, 0), DwellTime_ijk(mtxStrat, 0), busstop_sequence_no(mtxStrat, 0),
xpos_approachingbusstop(-1), ypos_approachingbusstop(-1)
{
	last_busStopRealTimes = new Shared<BusStop_RealTimes>(mtxStrat, BusStop_RealTimes());
	if (parent)
	{
		if (parent->getAgentSrc() == "BusController")
		{
			BusTrip* bustrip = dynamic_cast<BusTrip*> (*(parent->currTripChainItem));
			if (bustrip && bustrip->itemType == TripChainItem::IT_BUSTRIP)
			{
				std::vector<const BusStop*> busStops_temp = bustrip->getBusRouteInfo().getBusStops();
				std::cout << "busStops_temp.size() " << busStops_temp.size() << std::endl;
				for (int i = 0; i < busStops_temp.size(); i++)
				{
					Shared<BusStop_RealTimes>* pBusStopRealTimes = new Shared<BusStop_RealTimes>(mtxStrat, BusStop_RealTimes());
					busStopRealTimes_vec_bus.push_back(pBusStopRealTimes);
				}
			}
		}
	}
}

Role* BusDriver::clone(Person* parent) const
{
	BusDriverBehavior* behavior = new BusDriverBehavior(parent);
	BusDriverMovement* movement = new BusDriverMovement(parent);
	BusDriver* busdriver = new BusDriver(parent, parent->getMutexStrategy(), behavior, movement);
	behavior->setParentDriver(busdriver);
	movement->setParentDriver(busdriver);
	behavior->setParentBusDriver(busdriver);
	movement->setParentBusDriver(busdriver);
	movement->init();
	return busdriver;
}

double BusDriver::getPositionX() const
{
	return currPos.getX();
}

double BusDriver::getPositionY() const
{
	return currPos.getY();
}

vector<BufferedBase*> BusDriver::getSubscriptionParams()
{
	vector<BufferedBase*> res;
	res = Driver::getSubscriptionParams();

	// BusDriver's features
	res.push_back(&(lastVisited_BusStop));
	res.push_back(&(real_DepartureTime));
	res.push_back(&(real_ArrivalTime));
	res.push_back(&(DwellTime_ijk));
	res.push_back(&(busstop_sequence_no));
	res.push_back(last_busStopRealTimes);

	for (int j = 0; j < busStopRealTimes_vec_bus.size(); j++)
	{
		res.push_back(busStopRealTimes_vec_bus[j]);
	}

	return res;
}

void BusDriver::setBusStopRealTimes(const int& busStopSeqNum, const BusStop_RealTimes& busStopRealTimes)
{
	// busStopRealTimes_vec_bus empty validation
	if (!busStopRealTimes_vec_bus.empty())
	{
		// busstop_sequence_no range validation
		if (busStopSeqNum >= 0 && busStopSeqNum < busStopRealTimes_vec_bus.size())
		{
			// if the range is reasonable, set the BusStopRealTime for this bus stop
			busStopRealTimes_vec_bus[busStopSeqNum]->set(busStopRealTimes);
		}
	}
}

DriverRequestParams BusDriver::getDriverRequestParams()
{
	//	Person* person = dynamic_cast<Person*>(parent);
	DriverRequestParams res;

	res.existedRequest_Mode = &existed_Request_Mode;
	res.lastVisited_Busline = &lastVisited_Busline;
	res.lastVisited_BusTrip_SequenceNo = &lastVisited_BusTrip_SequenceNo;
	res.busstop_sequence_no = &busstop_sequence_no;
	res.real_ArrivalTime = &real_ArrivalTime;
	res.DwellTime_ijk = &DwellTime_ijk;
	res.lastVisited_BusStop = &lastVisited_BusStop;
	res.last_busStopRealTimes = last_busStopRealTimes;
	res.waiting_Time = &waiting_Time;

	return res;
}
