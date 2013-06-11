/*
 * DriverRequestParams.hpp
 *
 *  Created on: Mar 21, 2013
 *      Author: huaipeng
 */
#include "entities/misc/BusTrip.hpp"
#include "entities/Agent.hpp"

namespace sim_mob {

namespace {
//Helper for DriverRequestParams::
void push_non_null(std::vector<sim_mob::BufferedBase*>& vect, sim_mob::BufferedBase* item) {
	if (item) { vect.push_back(item); }
}
} //End anon namespace


/**
 * Return type for DriverRequestParams::asVector()
 */
struct DriverRequestParams {
	DriverRequestParams() :
		existedRequest_Mode(nullptr), lastVisited_Busline(nullptr), lastVisited_BusTrip_SequenceNo(nullptr),
		busstop_sequence_no(nullptr), real_ArrivalTime(nullptr), DwellTime_ijk(nullptr),
		lastVisited_BusStop(nullptr), last_busStopRealTimes(nullptr), waiting_Time(nullptr),
		travelStatsMap(nullptr), agentID(nullptr), xPos(nullptr), yPos(nullptr), isAtStop(nullptr),
		boardingPassengers(nullptr), alightingPassengers(nullptr)
	{}

	sim_mob::Shared<int>* existedRequest_Mode;
	sim_mob::Shared<std::string>* lastVisited_Busline;
	sim_mob::Shared<int>* lastVisited_BusTrip_SequenceNo;
	sim_mob::Shared<int>* busstop_sequence_no;
	sim_mob::Shared<double>* real_ArrivalTime;
	sim_mob::Shared<double>* DwellTime_ijk;
	sim_mob::Shared<const sim_mob::BusStop*>* lastVisited_BusStop;
	sim_mob::Shared<BusStop_RealTimes>* last_busStopRealTimes;
	sim_mob::Shared<double>* waiting_Time;
	sim_mob::Shared< std::map<double, sim_mob::Agent::travelStats> >* travelStatsMap;
	sim_mob::Shared<int>* agentID;
	sim_mob::Shared<int>* xPos;
	sim_mob::Shared<int>* yPos;
	sim_mob::Shared<bool>* isAtStop;
	sim_mob::Shared< std::vector<int> >* boardingPassengers;
	sim_mob::Shared< std::vector<int> >* alightingPassengers;

	//Return all properties as a vector of BufferedBase types (useful for iteration)
	std::vector<sim_mob::BufferedBase*> asVector() const {
		std::vector<sim_mob::BufferedBase*> res;
		push_non_null(res, existedRequest_Mode);
		push_non_null(res, lastVisited_Busline);
		push_non_null(res, lastVisited_BusTrip_SequenceNo);
		push_non_null(res, busstop_sequence_no);
		push_non_null(res, real_ArrivalTime);
		push_non_null(res, DwellTime_ijk);
		push_non_null(res, lastVisited_BusStop);
		push_non_null(res, last_busStopRealTimes);
		push_non_null(res, waiting_Time);
		push_non_null(res, travelStatsMap);
		push_non_null(res, agentID);
		push_non_null(res, xPos);
		push_non_null(res, yPos);
		push_non_null(res, isAtStop);
		push_non_null(res, boardingPassengers);
		push_non_null(res, alightingPassengers);

		return res;
	}
};

}
