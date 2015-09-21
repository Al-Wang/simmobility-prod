//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "BusController.hpp"

#include <stdexcept>

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "entities/Person.hpp"
#include "entities/roles/Role.hpp"
#include "entities/misc/BusTrip.hpp"
#include "geospatial/BusStop.hpp"
#include "geospatial/Link.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "workers/Worker.hpp"
#include "workers/WorkGroup.hpp"
#include "util/LangHelpers.hpp"
#include "boost/algorithm/string.hpp"

using std::vector;
using std::string;
using std::map;

using namespace sim_mob;

typedef Entity::UpdateStatus UpdateStatus;

/// Temporary saved all the buscontroller, eventually it will go to all agent stream
vector<BusController*> BusController::all_busctrllers_;

bool BusController::busBreak = false;
int BusController::busstopindex = 1;

namespace {
	// planned headway(ms) of the buses
	const double PLANNED_HEADWAY_MS = 480000;
	// secs conversion unit from milliseconds
	const double SECS_CONVERT_UNIT = 0.001;
	// millisecs conversion unit from seconds
	const double MILLISECS_CONVERT_UNIT = 1000.0;
}

void sim_mob::BusController::RegisterNewBusController(unsigned int startTime, const MutexStrategy& mtxStrat)
{
	BusController* busctrller = new sim_mob::BusController(-1, mtxStrat);
	busctrller->setStartTime(startTime);
	all_busctrllers_.push_back(busctrller);
}

bool sim_mob::BusController::HasBusControllers()
{
	return !all_busctrllers_.empty();
}

void sim_mob::BusController::InitializeAllControllers(std::set<Entity*>& agents_list, const vector<PT_bus_dispatch_freq>& busdispatch_freq)
{
	//Check: Do we have exactly one BusController?
	if (all_busctrllers_.size()>1) {
		throw std::runtime_error("Currently, we only support zero or one Bus Controller");
	}

	//Initialize every item in the list.

	for (vector<BusController*>::iterator it=all_busctrllers_.begin(); it!=all_busctrllers_.end(); it++) {
		(*it)->setPTScheduleFromConfig(busdispatch_freq);
		(*it)->assignBusTripChainWithPerson(agents_list);
	}
}


void sim_mob::BusController::DispatchAllControllers(std::set<Entity*>& agents_list)
{
	//Push every item on the list into the agents array as an active agent
	for (std::vector<BusController*>::iterator it=all_busctrllers_.begin(); it!=all_busctrllers_.end(); it++) {
		agents_list.insert(*it);
	}
}


BusController* sim_mob::BusController::TEMP_Get_Bc_1()
{
	if (all_busctrllers_.size()!=1) {
		throw std::runtime_error("BusControllers array is empty.");
	}
	return all_busctrllers_.front();
}

std::vector<BufferedBase *> sim_mob::BusController::buildSubscriptionList()
{
	return Agent::buildSubscriptionList();
}

void sim_mob::BusController::CollectAndProcessAllRequests()
{
	for (std::vector<BusController*>::iterator it=all_busctrllers_.begin(); it!=all_busctrllers_.end(); it++) {
		(*it)->handleDriverRequest();
	}
}


void sim_mob::BusController::addBus(Bus* bus)
{
	managedBuses.push_back(bus);
}

void sim_mob::BusController::remBus(Bus* bus)
{
	vector<Bus*>::iterator it = std::find(managedBuses.begin(), managedBuses.end(), bus);
	if (it!=managedBuses.end()) {
		managedBuses.erase(it);
	}
}

void sim_mob::BusController::assignBusTripChainWithPerson(std::set<sim_mob::Entity*>& active_agents)
{
	const ConfigParams& config = ConfigManager::GetInstance().FullConfig();
	const map<string, Busline*>& buslines = pt_schedule.getBusLines();
	if(0 == buslines.size()) {
		throw std::runtime_error("Error:  No busline in the PT_Schedule, please check the setPTSchedule.");
	}

	for(map<string, Busline*>::const_iterator buslinesIt = buslines.begin();buslinesIt!=buslines.end();buslinesIt++) {
		Busline* busline = buslinesIt->second;
		const vector<BusTrip>& busTrip_vec = busline->queryBusTrips();

		for (vector<BusTrip>::const_iterator tripIt=busTrip_vec.begin(); tripIt!=busTrip_vec.end(); tripIt++) {
			if(tripIt->startTime.isAfterEqual(ConfigManager::GetInstance().FullConfig().simStartTime())) {// in case sometimes BusTrip startTime is smaller than simStartTime to skip some BusTrips
				Person* currAg = new Person("BusController", config.mutexStategy(), -1, tripIt->getPersonID());
				currAg->setPersonCharacteristics();

				vector<TripChainItem*> currAgTripChain;
				currAgTripChain.push_back(const_cast<BusTrip*>(&(*tripIt)));// one person for one busTrip, currently not considering Activity for BusDriver
				currAg->setTripChain(currAgTripChain);
				currAg->initTripChain();
				currAg->busLine = busline->getBusLineID();
				currAg->bustripnum = (const_cast<BusTrip*>(&(*tripIt)))->getBusTripRun_SequenceNum();
				// scheduled for dispatch
				addOrStashBuses(currAg, active_agents);
			}
		}
	}

	for (std::set<Entity*>::iterator it=active_agents.begin(); it!=active_agents.end(); it++) {
		(*it)->parentEntity = this;
		all_children.push_back( (*it) );
	}
}

void sim_mob::BusController::dynamicalGenerateAgent(unsigned int preTicks, unsigned int curTicks, std::vector<Entity*>& active_agents)
{
	const ConfigParams& config = ConfigManager::GetInstance().FullConfig();
	const map<string, Busline*>& buslines = pt_schedule.getBusLines();
	if(0 == buslines.size()) {
		throw std::runtime_error("Error: No busline in the PT_Schedule, please check the setPTSchedule.");
	}

	for(map<string, Busline*>::const_iterator buslinesIt = buslines.begin();buslinesIt!=buslines.end();buslinesIt++) {
		Busline* busline = buslinesIt->second;
		const vector<BusTrip>& busTrip_vec = busline->queryBusTrips();

		for (vector<BusTrip>::const_iterator tripIt=busTrip_vec.begin(); tripIt!=busTrip_vec.end(); tripIt++) {
			if(tripIt->startTime.isAfterEqual(config.simStartTime())) {// in case sometimes BusTrip startTime is smaller than simStartTime to skip some BusTrips

				unsigned int tripStartTime = tripIt->startTime.offsetMS_From(config.simStartTime());

				if( tripStartTime>=preTicks && tripStartTime<curTicks)
				{
					Person* currAg = new Person("BusController", config.mutexStategy(), -1, tripIt->getPersonID());
					currAg->setStartTime(tripStartTime);

					vector<TripChainItem*> currAgTripChain;
					currAgTripChain.push_back(const_cast<BusTrip*>(&(*tripIt)));// one person for one busTrip, currently not considering Activity for BusDriver
					currAg->setTripChain(currAgTripChain);
					currAg->initTripChain();

					// scheduled for dispatch
					active_agents.push_back(currAg);
				}
			}
		}
	}

	for (vector<Entity*>::iterator it=active_agents.begin(); it!=active_agents.end(); it++) {
		(*it)->parentEntity = this;
		all_children.push_back( (*it) );
	}
}

struct RouteInfo{
	std::string line;
	unsigned int id;
	unsigned int index;
	std::string start;
	std::string end;
	unsigned int startPosX;
	unsigned int startPosY;
	unsigned int endPosX;
	unsigned int endPosY;
};
struct StopInfo{
	std::string line;
	std::string id;
	unsigned int posX;
	unsigned int posY;
	int index;
};

bool searchBusRoutes(const vector<const BusStop*>& stops,
		const std::string& busLine, std::deque<RouteInfo>& allRoutes,
		std::deque<StopInfo>& allStops) {

	const BusStop* start;
	const BusStop* end;
	const BusStop* nextEnd;
	bool isFound = true;
	if (stops.size() > 0) {
		start = nullptr;
		end = nullptr;
		nextEnd = nullptr;
		std::deque<RouteInfo> routeIDs;
		std::deque<StopInfo> stopIDs;
		for (int k = 0; k < stops.size(); k++) {
			isFound = false;
			const BusStop* busStop = stops[k];
			if (k == 0) {
				start = busStop;
				StopInfo stopInfo;
				stopInfo.id = start->getBusstopno_();
				stopInfo.line = busLine;
				stopInfo.posX = start->xPos;
				stopInfo.posY = start->yPos;
				stopIDs.push_back(stopInfo);
			} else {
				end = busStop;
				const StreetDirectory& stdir = StreetDirectory::instance();
				StreetDirectory::VertexDesc startDes = stdir.DrivingVertex(*start);
				StreetDirectory::VertexDesc endDes = stdir.DrivingVertex(*end);
				vector<WayPoint> path;
				if (start->getParentSegment() == end->getParentSegment()) {
					path.push_back(WayPoint(start->getParentSegment()));
				} else {
					path = stdir.SearchShortestDrivingPath(startDes, endDes);
				}

				for (std::vector<WayPoint>::const_iterator it = path.begin();
						it != path.end(); it++) {
					if (it->type_ == WayPoint::ROAD_SEGMENT) {
						unsigned int id =
								(*it).roadSegment_->getSegmentAimsunId();
						if (routeIDs.size() == 0 || routeIDs.back().id != id) {
							RouteInfo route;
							route.id = id;
							route.start = start->getBusstopno_();
							route.end = end->getBusstopno_();
							route.startPosX = start->xPos;
							route.startPosY = start->yPos;
							route.endPosX = end->xPos;
							route.endPosY = end->yPos;
							routeIDs.push_back(route);
						}
						isFound = true;
					}
				}

				if (!isFound) {
					std::cout << "can not find bus route in bus line:" << busLine
							<< " start stop:" << start->getBusstopno_()
							<< "  end stop:" << end->getBusstopno_()
							<< std::endl;
					routeIDs.clear();
					stopIDs.clear();
					break;
				} else {
					StopInfo stopInfo;
					stopInfo.id =   end->getBusstopno_();
					stopInfo.line = busLine;
					stopInfo.posX = end->xPos;
					stopInfo.posY = end->yPos;
					stopIDs.push_back(stopInfo);
				}

				start = end;
			}
		}

		if (routeIDs.size() > 0 && stopIDs.size() > 0) {
			unsigned int index = 0;
			for (std::deque<RouteInfo>::const_iterator it = routeIDs.begin();
					it != routeIDs.end(); it++) {
				RouteInfo routeInfo;
				routeInfo.line = busLine;
				routeInfo.id = it->id;
				routeInfo.start = it->start;
				routeInfo.end = it->end;
				routeInfo.startPosX = it->startPosX;
				routeInfo.startPosY = it->startPosY;
				routeInfo.endPosX = it->endPosX;
				routeInfo.endPosY = it->endPosY;
				routeInfo.index = index;
				allRoutes.push_back(routeInfo);
				index++;
			}

			index = 0;
			for (std::deque<StopInfo>::const_iterator it = stopIDs.begin();
					it != stopIDs.end(); it++) {
				StopInfo stopInfo;
				stopInfo.line = it->line;
				stopInfo.id = it->id;
				stopInfo.index = index;
				stopInfo.posX = it->posX;
				stopInfo.posY = it->posY;
				allStops.push_back(stopInfo);
				index++;
			}
		}
	}

	return isFound;
}

void sim_mob::BusController::setPTScheduleFromConfig(const vector<PT_bus_dispatch_freq>& busdispatch_freq)
{
	const ConfigParams& config = ConfigManager::GetInstance().FullConfig();
	vector<const BusStop*> stops;
	sim_mob::Busline* busline = nullptr;
	int step = 0;
	all_children.clear();
	bool busstop_busline_registered=false;
	// record the last dispatching time
	DailyTime lastBusDispatchTime;
	int amount = 0;
	int amountFailed =0;
	std::deque<RouteInfo> allRoutes;
	std::deque<StopInfo> allStops;

	for (vector<sim_mob::PT_bus_dispatch_freq>::const_iterator curr=busdispatch_freq.begin(); curr!=busdispatch_freq.end(); curr++) {
		vector<sim_mob::PT_bus_dispatch_freq>::const_iterator next = curr+1;

		//If we're on a new BusLine, register it with the scheduler.
		if(!busline || (curr->route_id != busline->getBusLineID())) {
			busline = new sim_mob::Busline(curr->route_id,config.busline_control_type());

			pt_schedule.registerBusLine(curr->route_id, busline);
			pt_schedule.registerControlType(curr->route_id, busline->getControlType());
			step = 0; //NOTE: I'm fairly sure this needs to be reset here. ~Seth
			busstop_busline_registered = true;
		}

		// define frequency_busline for one busline
		busline->addFrequencyBusline(Frequency_Busline(curr->start_time,curr->end_time,curr->headway_sec));

		//Set nextTime to the next frequency bus line's start time or the current line's end time if this is the last line.
		sim_mob::DailyTime nextTime = curr->end_time;

		//We use a trick to "advance" the time by a given amount; just create a DailyTime with that advance value
		//  and add it during each time step.
		DailyTime advance(curr->headway_sec*1000);// curr->headway_sec*200
		for(DailyTime startTime = curr->start_time; startTime.isBeforeEqual(nextTime); startTime += advance) {
			// deal with small gaps between the group dispatching times
			if((startTime - lastBusDispatchTime).isBeforeEqual(advance)) {
				startTime = lastBusDispatchTime + advance;
			}

			//TODO: I am setting the Vehicle ID to -1 for now; it *definitely* shouldn't be the same as the Agent ID.
			BusTrip bustrip("", "BusTrip", 0, -1, startTime, DailyTime("00:00:00"), step++, busline, -1, curr->route_id, nullptr, "node", nullptr, "node");

			//Try to find our data.
			map<string, vector<const RoadSegment*> >::const_iterator segmentsIt = config.getRoadSegments_Map().find(curr->route_id);
			map<string, vector<const BusStop*> >::const_iterator stopsIt = config.getBusStops_Map().find(curr->route_id);

			vector<const RoadSegment*> segments = vector<const RoadSegment*>();
			const std::map<std::string, std::vector<const sim_mob::RoadSegment*> >& routeID_roadSegments = config.getRoadSegments_Map();
			map<string, vector<const RoadSegment*> >::const_iterator itSeg = routeID_roadSegments.find(curr->route_id);
			if(itSeg!=routeID_roadSegments.end())
			{
				segments = itSeg->second;
			}

			stops = vector<const BusStop*>();
			const std::map<std::string, std::vector<const sim_mob::BusStop*> >& routeID_busStops = config.getBusStops_Map();
			map<string, vector<const BusStop*> >::const_iterator itStop = routeID_busStops.find(curr->route_id);
			if(itStop!=routeID_busStops.end())
			{
				stops = itStop->second;
			}

			if (busstop_busline_registered && sim_mob::ConfigManager::GetInstance().FullConfig().isGenerateBusRoutes()) {
				if (searchBusRoutes(stops, curr->route_id, allRoutes,
						allStops)) {
					amount++;
				} else {
					amountFailed++;
				}
			}

			//Our algorithm expects empty vectors in some cases.
			//TODO: Clean this up! Logic for dealing with null cases should go here, not in the subroutine.
			if(busstop_busline_registered) // for each busline, only push once
			{
			  for(int k=0;k<stops.size();k++)//to store the bus line info at each bus stop
			  {
				 BusStop* busStop=const_cast<BusStop*>(stops[k]);
				 busStop->BusLines.push_back(busline);
			  }
		     busstop_busline_registered = false;
			}
			if(bustrip.setBusRouteInfo(segments, stops)) {
				busline->addBusTrip(bustrip);
			}
			lastBusDispatchTime = startTime;
		}
	}

	if(sim_mob::ConfigManager::GetInstance().FullConfig().isGenerateBusRoutes()){

		std::ofstream outputRoutes("routes.csv");
		for (std::deque<RouteInfo>::const_iterator it =
				allRoutes.begin(); it != allRoutes.end(); it++) {
			if (outputRoutes.is_open()){
				outputRoutes << it->line << ","
						<< it->index << ","
						<< it->id
						<< std::endl;
			}
		}
		outputRoutes.close();

		std::ofstream outputStop("stops.csv");
		for (std::deque<StopInfo>::const_iterator it =
				allStops.begin(); it != allStops.end(); it++) {
			if (outputStop.is_open()){
				outputStop << it->line << ","
						<< it->id << ","
						<< it->index
						<< std::endl;
			}
		}
		outputStop.close();
	}
}


void sim_mob::BusController::storeRealTimes_eachBusStop(const std::string& busline_i, int trip_k, int busstopSequence_j, double ATijk, double DTijk, const BusStop* lastVisited_BusStop, BusStop_RealTimes& realTime)
{
	Busline* busline = pt_schedule.findBusline(busline_i);
	if(!busline) {
		std::cout << "wrong busline assigned:" << std::endl;
		return;
	}

	double ETijk = 0;
	double departureTime = 0;
	departureTime = ATijk + (DTijk * MILLISECS_CONVERT_UNIT);
	BusStop_RealTimes busStop_RealTimes(ConfigManager::GetInstance().FullConfig().simStartTime() + DailyTime(ATijk), ConfigManager::GetInstance().FullConfig().simStartTime() + DailyTime(departureTime));
	busStop_RealTimes.setReal_BusStop(lastVisited_BusStop);
	realTime = (busStop_RealTimes);

	// here need test, need add fake RealTimes first
	const ConfigParams& config = ConfigManager::GetInstance().FullConfig();
	Shared<BusStop_RealTimes>* busStopRealTimes = new Shared<BusStop_RealTimes>(config.mutexStategy(), busStop_RealTimes);
	busline->resetBusTrip_StopRealTimes(trip_k, busstopSequence_j, busStopRealTimes);// set this value for next step

	if(trip_k > 0){
		const vector<BusTrip>& BusTrips = busline->queryBusTrips();
		const vector<Shared<BusStop_RealTimes>* >& busStopRealTimeTripkMinusOne = BusTrips[trip_k - 1].getBusStopRealTimes();

		std::cout << "busline: " << busline->getBusLineID() << " trip_k - 1 = " << trip_k - 1 << " busStopRealTimeTripkMinusOne  size(): " << busStopRealTimeTripkMinusOne.size() << std::endl;
		size_t iSize = busStopRealTimeTripkMinusOne.size();
		for(size_t i = 0; i < iSize; i++) {
			if(busStopRealTimeTripkMinusOne[i]->get().Real_busStop) {
				std::cout << "real_ArrivalTime: " << i << " " << busStopRealTimeTripkMinusOne[i]->get().real_ArrivalTime.getStrRepr() << " bus stop no: " << busStopRealTimeTripkMinusOne[i]->get().Real_busStop->busstopno_ << std::endl;
			}
		}
	}
}

double sim_mob::BusController::decisionCalculation(const string& busline_i, int trip_k, int busstopSequence_j, double ATijk, double DTijk, BusStop_RealTimes& realTime, const BusStop* lastVisited_BusStop)
{
	ControlTypes controlType = pt_schedule.findBuslineControlType(busline_i);
	Busline* busline = pt_schedule.findBusline(busline_i);
	if(!busline) {
		std::cout << "wrong busline assigned:" << std::endl;
		return -1;
	}
	const vector<BusTrip>& BusTrips = busline->queryBusTrips();

	double departureTime = 0; // If we use Control, since the busstopSequence_j is in the middle, so should not be 0
	double waitTimeBusStop = 0;

	switch(controlType) {
	case SCHEDULE_BASED:
		departureTime = scheduledDecision(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, realTime, lastVisited_BusStop);
		waitTimeBusStop = (departureTime - ATijk) * SECS_CONVERT_UNIT;
		break;
	case HEADWAY_BASED:
		departureTime = headwayDecision(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, realTime, lastVisited_BusStop);
		waitTimeBusStop = (departureTime - ATijk) * SECS_CONVERT_UNIT;
		break;
	case EVENHEADWAY_BASED:
		departureTime = evenheadwayDecision(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, realTime, lastVisited_BusStop);
		waitTimeBusStop = (departureTime - ATijk) * SECS_CONVERT_UNIT;
		break;
	case HYBRID_BASED:
		departureTime = hybridDecision(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, realTime, lastVisited_BusStop);
		waitTimeBusStop = (departureTime - ATijk) * SECS_CONVERT_UNIT;
		break;
	default:
		// may add default scheduled departure time here
		storeRealTimes_eachBusStop(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, lastVisited_BusStop, realTime);
		waitTimeBusStop = DTijk;
		break;
	}
	return waitTimeBusStop;
}

double sim_mob::BusController::scheduledDecision(const string& busline_i, int trip_k, int busstopSequence_j, double ATijk, double DTijk, BusStop_RealTimes& realTime, const BusStop* lastVisited_busStop)
{
	Busline* busline = pt_schedule.findBusline(busline_i);
	if(!busline) {
		std::cout << "wrong busline assigned:" << std::endl;
		return -1;
	}
	const vector<Frequency_Busline>& freq_busline = busline->query_Frequency_Busline();// query different headways for different times

	//unsigned int DTijk = 0;
	double SETijk = 0;
	double ETijk = 0;
	double sij = 0;// slack size(should be zero)

	//Fwd_ATijk = ATijk;// assign value
	const vector<BusTrip>& BusTrips = busline->queryBusTrips();

	//StopInformation(Times)
	const vector<BusStop_ScheduledTimes>& busStopScheduledTime_tripK = BusTrips[trip_k].getBusStopScheduledTimes();
	SETijk = busStopScheduledTime_tripK[busstopSequence_j].scheduled_DepartureTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime());
	ETijk = std::max(SETijk - sij, ATijk + (DTijk * MILLISECS_CONVERT_UNIT));

	storeRealTimes_eachBusStop(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, lastVisited_busStop, realTime);

	return ETijk;
}

double sim_mob::BusController::headwayDecision(const string& busline_i, int trip_k, int busstopSequence_j, double ATijk, double DTijk, BusStop_RealTimes& realTime, const BusStop* lastVisited_busStop)
{
	Busline* busline = pt_schedule.findBusline(busline_i);
	if(!busline) {
		std::cout << "wrong busline assigned:" << std::endl;
		return -1;
	}
	const vector<Frequency_Busline>& freq_busline = busline->query_Frequency_Busline();// query different headways for different times

	//unsigned int DTijk = 0;
	double ETijk = 0;
	double ATijkMinusOne = 0;
	double Hi = 0;
	double alpha = 0.8;// 0.7(Hi = 100000) range from 0.6 to 0.8

	if (trip_k <= 18) { // 857_1, first trip RunNum = 18
		// the first trip just use Dwell Time, no holding strategy
		ETijk = ATijk + (DTijk * MILLISECS_CONVERT_UNIT);
	} else {
		const vector<BusTrip>& BusTrips = busline->queryBusTrips();
		const vector<Shared<BusStop_RealTimes>* >& busStopRealTimeTripkMinusOne = BusTrips[trip_k - 1].getBusStopRealTimes();

		std::cout << "busStopRealTime_tripK_1  size(): " << busStopRealTimeTripkMinusOne.size() << std::endl;
		std::cout << "real_ArrivalTime in headway Decision: " << " trip_k - 1 " << trip_k - 1 << " busstopSequence_j " << busstopSequence_j << "	" << busStopRealTimeTripkMinusOne[busstopSequence_j]->get().real_ArrivalTime.getStrRepr() << std::endl;
		if(busStopRealTimeTripkMinusOne[busstopSequence_j]->get().Real_busStop) { // data has already updated
			ATijkMinusOne = busStopRealTimeTripkMinusOne[busstopSequence_j]->get().real_ArrivalTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime());// there are some cases that buses are bunched together so that k-1 has no values updated yet
			Hi = PLANNED_HEADWAY_MS;// 857_1(headway: 480000ms) 444300(test holding), 143000(best), 142000, 140000(headway*100), 138000, 181000(bad effect) ;60000(headway*50)
			ETijk = std::max(ATijkMinusOne + alpha*Hi, ATijk + (DTijk * MILLISECS_CONVERT_UNIT)); // DTijk unit is sec, so change to ms by multiplying 1000
		} else {// data has not yet updated, sometimes happens especially buses are bunched together(trip_k bus can overtake tripk_1 bus)
			ETijk = ATijk + (DTijk * MILLISECS_CONVERT_UNIT); // immediately leaving
		}


	}

	storeRealTimes_eachBusStop(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, lastVisited_busStop, realTime);

	return ETijk;
}

double sim_mob::BusController::evenheadwayDecision(const string& busline_i, int trip_k, int busstopSequence_j, double ATijk, double DTijk, BusStop_RealTimes& realTime, const BusStop* lastVisited_busStop)
{
	Busline* busline = pt_schedule.findBusline(busline_i);
	if(!busline) {
		std::cout << "wrong busline assigned:" << std::endl;
		return -1;
	}
	const vector<Frequency_Busline>& freq_busline = busline->query_Frequency_Busline();// query different headways for different times
	const vector<BusTrip>& BusTrips = busline->queryBusTrips();

	//unsigned int DTijk = 0;
	double ETijk = 0;
	double ATijkMinusOne = 0;
	double ATimkPlusOne = 0;
	double SRTmj = 0;

    bool lastTrip = ((BusTrips.size()-1) == trip_k);//check if last trip
    int lastVisitedStopNum = 0;// check whether last visited Stop num is valid or not
    if(!lastTrip)
    {
    	lastVisitedStopNum = BusTrips[trip_k+1].lastVisitedStop_SequenceNumber;
    }

	if (0 == trip_k) {
		// the first trip just use Dwell Time, no holding strategy
		ETijk = ATijk + (DTijk * MILLISECS_CONVERT_UNIT);

	}
	else if(lastTrip || lastVisitedStopNum == -1){
		// If last trip or if next trip k+1 is not dispatched yet then use single headway
		return headwayDecision(busline_i,trip_k,busstopSequence_j,ATijk,DTijk, realTime, lastVisited_busStop);
	}
	else {
		lastVisitedStopNum = BusTrips[trip_k+1].lastVisitedStop_SequenceNumber;//last stop visited by bus trip k+1
		const vector <Shared<BusStop_RealTimes>* >& busStopRealTimeTripkMinusOne = BusTrips[trip_k - 1].getBusStopRealTimes();
		ATijkMinusOne = busStopRealTimeTripkMinusOne[busstopSequence_j]->get().real_ArrivalTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime());

		const vector <Shared<BusStop_RealTimes>* >& busStopRealTime_tripKplus1 = BusTrips[trip_k + 1].getBusStopRealTimes();
		ATimkPlusOne = busStopRealTime_tripKplus1[lastVisitedStopNum]->get().real_ArrivalTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime());

		const vector<BusStop_ScheduledTimes>& busStopScheduledTime_tripKplus1 = BusTrips[trip_k + 1].getBusStopScheduledTimes();
		SRTmj = busStopScheduledTime_tripKplus1[busstopSequence_j].scheduled_ArrivalTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime())
				- busStopScheduledTime_tripKplus1[lastVisitedStopNum].scheduled_DepartureTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime());

		ETijk = std::max(ATijkMinusOne + (ATimkPlusOne + SRTmj - ATijkMinusOne)/2.0, ATijk + (DTijk * MILLISECS_CONVERT_UNIT)); // need some changes for precision

	}

	std::cout<<"YaoJinTest:  busstop "<<busstopSequence_j<<" trip "<<trip_k<<" arrival time "<<ATijk<<" Departure time "<<ETijk<<std::endl;
	storeRealTimes_eachBusStop(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, lastVisited_busStop, realTime);

	return ETijk;
}

double sim_mob::BusController::hybridDecision(const string& busline_i, int trip_k, int busstopSequence_j, double ATijk, double DTijk, BusStop_RealTimes& realTime, const BusStop* lastVisited_busStop)
{
	Busline* busline = pt_schedule.findBusline(busline_i);
		if(!busline) {
			std::cout << "wrong busline assigned:" << std::endl;
			return -1;
		}
		const vector<Frequency_Busline>& freq_busline = busline->query_Frequency_Busline();// query different headways for different times
		const vector<BusTrip>& BusTrips = busline->queryBusTrips();

		double ETijk = 0;
		double ATijkMinusOne = 0;
		double ATimkPlusOne = 0;
		double SRTmj = 0;
		double Hi = 0;

	    bool lastTrip = ((BusTrips.size()-1) == trip_k);//check if last trip
	    int lastVisitedStopNum = 0;// check whether last visited Stop num is valid or not
	    if(!lastTrip)
	    {
	    	lastVisitedStopNum = BusTrips[trip_k+1].lastVisitedStop_SequenceNumber;
	    }

		if (0 == trip_k) {
			// the first trip just use Dwell Time, no holding strategy
			ETijk = ATijk + (DTijk * MILLISECS_CONVERT_UNIT);

		}
		else if(lastTrip || lastVisitedStopNum == -1){
			// If last trip or if next trip k+1 is not dispatched yet then use single headway
			return headwayDecision(busline_i,trip_k,busstopSequence_j,ATijk,DTijk, realTime, lastVisited_busStop);
		}
		else {
			lastVisitedStopNum = BusTrips[trip_k+1].lastVisitedStop_SequenceNumber;//last stop visited by bus trip k+1
			const vector <Shared<BusStop_RealTimes>* >& busStopRealTimeTripkMinusOne = BusTrips[trip_k - 1].getBusStopRealTimes();
			ATijkMinusOne = busStopRealTimeTripkMinusOne[busstopSequence_j]->get().real_ArrivalTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime());

			const vector <Shared<BusStop_RealTimes>* >& busStopRealTime_tripKplus1 = BusTrips[trip_k + 1].getBusStopRealTimes();
			ATimkPlusOne = busStopRealTime_tripKplus1[lastVisitedStopNum]->get().real_ArrivalTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime());

			const vector<BusStop_ScheduledTimes>& busStopScheduledTime_tripKplus1 = BusTrips[trip_k + 1].getBusStopScheduledTimes();
			SRTmj = busStopScheduledTime_tripKplus1[busstopSequence_j].scheduled_ArrivalTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime())
					- busStopScheduledTime_tripKplus1[lastVisitedStopNum].scheduled_DepartureTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime());
			Hi = PLANNED_HEADWAY_MS;
			ETijk = std::max(std::min(ATijkMinusOne + (ATimkPlusOne + SRTmj - ATijkMinusOne)/2.0, (ATijkMinusOne + Hi)),(double)(ATijk) + (DTijk * MILLISECS_CONVERT_UNIT)); // need some changes for precision
		}

		storeRealTimes_eachBusStop(busline_i, trip_k, busstopSequence_j, ATijk, DTijk, lastVisited_busStop, realTime);

		return ETijk;
}



void sim_mob::BusController::addOrStashBuses(Agent* p, std::set<Entity*>& active_agents)
{
	//Only agents with a start time of zero should start immediately in the all_agents list.
	if (p->getStartTime() == 0)
	{
		Person *person = dynamic_cast<Person *>(p);
		
		if(person != nullptr)
		{
			person->load(person->getConfigProperties());
			person->clearConfigProperties();
		}

		active_agents.insert(p);
	}
	else
	{
		//Start later.
		pending_buses.push(p);
	}
}


void sim_mob::BusController::handleRequestParams(sim_mob::DriverRequestParams rParams)
{
	//No reaction if request params is empty.
	if(rParams.asVector().empty() ) {
		return;
	}

	Shared<int>* existed_Request_Mode = rParams.existedRequest_Mode;
	if( existed_Request_Mode && (existed_Request_Mode->get()==Role::REQUEST_DECISION_TIME || existed_Request_Mode->get()==Role::REQUEST_STORE_ARRIVING_TIME)) {
		Shared<string>* lastVisited_Busline = rParams.lastVisited_Busline;
		Shared<int>* lastVisited_BusTrip_SequenceNo = rParams.lastVisited_BusTrip_SequenceNo;
		Shared<int>* busstop_sequence_no = rParams.busstop_sequence_no;
		Shared<double>* real_ArrivalTime = rParams.real_ArrivalTime;
		Shared<double>* DwellTime_ijk = rParams.DwellTime_ijk;
		Shared<const BusStop*>* lastVisited_BusStop = rParams.lastVisited_BusStop;
		Shared<BusStop_RealTimes>* last_busStopRealTimes = rParams.last_busStopRealTimes;
		Shared<double>* waiting_Time = rParams.waiting_Time;

		if(existed_Request_Mode && lastVisited_Busline && lastVisited_BusTrip_SequenceNo && busstop_sequence_no
		   && real_ArrivalTime && DwellTime_ijk && lastVisited_BusStop && last_busStopRealTimes && waiting_Time)
		{
			BusStop_RealTimes realTime;
			if(existed_Request_Mode->get() == Role::REQUEST_DECISION_TIME ){
				double waitingtime = decisionCalculation(lastVisited_Busline->get(),lastVisited_BusTrip_SequenceNo->get(),busstop_sequence_no->get(),real_ArrivalTime->get(),DwellTime_ijk->get(),realTime,lastVisited_BusStop->get());
				waiting_Time->set(waitingtime);
			}
			else if(existed_Request_Mode->get() == Role::REQUEST_STORE_ARRIVING_TIME ){
				storeRealTimes_eachBusStop(lastVisited_Busline->get(),lastVisited_BusTrip_SequenceNo->get(),busstop_sequence_no->get(),real_ArrivalTime->get(),DwellTime_ijk->get(),lastVisited_BusStop->get(), realTime);
			}
			last_busStopRealTimes->set(realTime);
		}
	}
}


void sim_mob::BusController::handleDriverRequest()
{
	for (vector<Entity*>::iterator it = all_children.begin(); it != all_children.end(); it++) {
	//for (vector<Entity*>::iterator it = Agent::all_agents.begin(); it != Agent::all_agents.end(); it++) {
		Person* person = dynamic_cast<sim_mob::Person*>(*it);
		if(person){
			Role* role = person->getRole();
			if(role){
				handleRequestParams(role->getDriverRequestParams());
			}
		}
	}
}

void sim_mob::BusController::unregisteredChild(Entity* child)
{
	if(child)
	{
		std::vector<Entity*>::iterator it = std::find(all_children.begin(), all_children.end(), child);
		if (it != all_children.end() ) {
			all_children.erase(it);
		}
	}
}

Entity::UpdateStatus sim_mob::BusController::frame_tick(timeslice now)
{
	nextTimeTickToStage += tickStep;
	unsigned int nextTickMS = (nextTimeTickToStage+3)*ConfigManager::GetInstance().FullConfig().baseGranMS();

	//Stage any pending entities that will start during this time tick.
	while (!pending_buses.empty() && pending_buses.top()->getStartTime() <= nextTickMS) {
		///////////////////////////////////////////////////////////////////
		//Ask the current worker's parent WorkGroup to schedule this Entity.
		///////////////////////////////////////////////////////////////////
		//
		// TODO: The use of "getParent()" in Worker is extremely dangerous.
		//       It only works because there is exactly 1 BusController, and that
		//       Controller is the only thing which accesses the "parent" class.
		//       To fix this, we should probably return a list of entities to schedule
		//       as a result of update(), and let the WorkGroup skim this list
		//       when it knows it's safe to. ~Seth
		//
		///////////////////////////////////////////////////////////////////
		//currWorker->getParent()->scheduleEntity(pending_buses.top());
		//pending_buses.pop();
		// use new method to schedule child agent
		Agent* child = pending_buses.top();
		pending_buses.pop();
		child->parentEntity = this;
		currWorkerProvider->scheduleForBred(child);
		all_children.push_back(child);
	}

	handleDriverRequest();

	return Entity::UpdateStatus::Continue;
}



bool sim_mob::BusController::frame_init(timeslice now)
{
	frameNumberCheck = 0;
	return true;
}

void sim_mob::BusController::frame_output(timeslice now)
{
	//if no buscontroller in the loadorder, no output
	if (!getToBeInList()) {
		return;
	}

	LogOut("(\"BusController\""
			<<","<<now.frame()
			<<","<<getId()
			<<",{"
			<<"\"managedBuses size\":\""<<static_cast<int>(managedBuses.size())
			<<"\",\"Bus_xPos\":\""<<static_cast<int>(posBus.x)
			<<"\",\"Bus_yPos\":\""<<static_cast<int>(posBus.y)
			<<"\"})"<<std::endl);
}




////////////////////////////////////////////////////////////////////////////////////////
// Trivial getters and setters
////////////////////////////////////////////////////////////////////////////////////////

sim_mob::Link* sim_mob::BusController::getCurrLink(){
	return currLink;
}
void sim_mob::BusController::setCurrLink(sim_mob::Link* link) {
	currLink = link;
}

