//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * SystemOfModels.cpp
 *
 *  Created on: Nov 7, 2013
 *      Author: Harish Loganathan
 */

#include "PredaySystem.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <cstdlib>
#include <string>
#include <sstream>
#include "behavioral/lua/PredayLuaProvider.hpp"
#include "behavioral/params/ModeDestinationParams.hpp"
#include "behavioral/params/StopGenerationParams.hpp"
#include "behavioral/params/TimeOfDayParams.hpp"
#include "behavioral/params/TourModeParams.hpp"
#include "behavioral/params/TripChainItemParams.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "conf/Constructs.hpp"
#include "database/DB_Connection.hpp"
#include "logging/Log.hpp"
#include "mongo/client/dbclient.h"
#include "PredayClasses.hpp"
#include "util/Utils.hpp"

using namespace std;
using namespace sim_mob;
using namespace sim_mob::medium;
using namespace mongo;

namespace {
	const double HIGH_TRAVEL_TIME = 999.0;
	const double WALKABLE_DISTANCE = 3.0;

	const int AM_PEAK_LOW = 10;
	const int AM_PEAK_HIGH = 14;
	const int PM_PEAK_LOW = 30;
	const int PM_PEAK_HIGH = 34;
	const int MAX_STOPS_IN_HALF_TOUR = 3;

	// ISG choices
	const int WORK_CHOICE_ISG = 1;
	const int EDU_CHOICE_ISG = 2;
	const int SHOP_CHOICE_ISG = 3;
	const int OTHER_CHOICE_ISG = 4;
	const int QUIT_CHOICE_ISG = 5;

	//Time related
	const double FIRST_WINDOW = 3.25;
	const int FIRST_INDEX = 1;
	const double LAST_WINDOW = 26.75;
	const int LAST_INDEX = 48;

	std::map<int, std::string> setModeMap() {
		// 1 for public bus; 2 for MRT/LRT; 3 for private bus; 4 for drive1;
		// 5 for shared2; 6 for shared3+; 7 for motor; 8 for walk; 9 for taxi
		//mode_idx_ref = { 1 : 3, 2 : 5, 3 : 3, 4 : 1, 5 : 6, 6 : 6, 7 : 8, 8 : 2, 9 : 4 }
		std::map<int, std::string> res;
		res[1] = "Bus";
		res[2] = "MRT";
		res[3] = "Bus";
		res[4] = "Car";
		res[5] = "Car Sharing";
		res[6] = "Car Sharing";
		res[7] = "Bike";
		res[8] = "Walk";
		res[9] = "Taxi";
		return res;
	}

	const std::map<int,std::string> modeMap = setModeMap();

	inline double getTimeWindowFromIndex(const double index) {
		return (index * 0.5 /*half hour windows*/) + 2.75 /*the day starts at 3.25*/;
	}

	inline double getIndexFromTimeWindow(const double window) {
		return (window - 2.75 /*the day starts at 3.25*/) / 0.5;
	}

	double alignTime(double time) {
		// align to corresponding time window
		//1. split the computed tour end time into integral and fractional parts
		double intPart,fractPart;
		fractPart = std::modf(time, &intPart);

		//2. perform sanity checks on the integral part and align the fractional part to nearest time window
		if (intPart < FIRST_WINDOW) {
			time = FIRST_WINDOW;
		}
		else if (intPart > LAST_WINDOW) {
			time = LAST_WINDOW;
		}
		else if(std::abs(fractPart) < 0.5) {
			time = intPart + 0.25;
		}
		else {
			time = intPart + 0.75;
		}

		return time;
	}
}

PredaySystem::PredaySystem(PersonParams& personParams,
		const ZoneMap& zoneMap, const boost::unordered_map<int,int>& zoneIdLookup,
		const CostMap& amCostMap, const CostMap& pmCostMap, const CostMap& opCostMap,
		const std::map<std::string, db::MongoDao*>& mongoDao)
: personParams(personParams), zoneMap(zoneMap), zoneIdLookup(zoneIdLookup),
  amCostMap(amCostMap), pmCostMap(pmCostMap), opCostMap(opCostMap),
  mongoDao(mongoDao), logStream(std::stringstream::out)
{}

PredaySystem::~PredaySystem()
{
	for(TourList::iterator tourIt=tours.begin(); tourIt!=tours.end(); tourIt++) {
		safe_delete_item(*tourIt);
	}
	tours.clear();
}

bool sim_mob::medium::PredaySystem::predictUsualWorkLocation(bool firstOfMultiple) {
    UsualWorkParams usualWorkParams;
	usualWorkParams.setFirstOfMultiple((int) firstOfMultiple);
	usualWorkParams.setSubsequentOfMultiple((int) !firstOfMultiple);

	usualWorkParams.setZoneEmployment(zoneMap.at(zoneIdLookup.at(personParams.getFixedWorkLocation()))->getEmployment());

	if(personParams.getHomeLocation() != personParams.getFixedWorkLocation()) {
		usualWorkParams.setWalkDistanceAm(amCostMap.at(personParams.getHomeLocation()).at(personParams.getFixedWorkLocation())->getDistance());
		usualWorkParams.setWalkDistancePm(pmCostMap.at(personParams.getHomeLocation()).at(personParams.getFixedWorkLocation())->getDistance());
	}
	else {
		usualWorkParams.setWalkDistanceAm(0);
		usualWorkParams.setWalkDistancePm(0);
	}
	return PredayLuaProvider::getPredayModel().predictUsualWorkLocation(personParams, usualWorkParams);
}

void PredaySystem::predictTourMode(Tour* tour) {
	TourModeParams tmParams;
	tmParams.setStopType(tour->getTourType());

	ZoneParams* znOrgObj = zoneMap.at(zoneIdLookup.at(personParams.getHomeLocation()));
	ZoneParams* znDesObj = zoneMap.at(zoneIdLookup.at(tour->getTourDestination()));
	tmParams.setCostCarParking(znDesObj->getParkingRate());
	tmParams.setCentralZone(znDesObj->getCentralDummy());
	tmParams.setResidentSize(znOrgObj->getResidentWorkers());
	tmParams.setWorkOp(znDesObj->getEmployment());
	tmParams.setEducationOp(znDesObj->getTotalEnrollment());
	tmParams.setOriginArea(znOrgObj->getArea());
	tmParams.setDestinationArea(znDesObj->getArea());
	if(personParams.getHomeLocation() != tour->getTourDestination()) {
		CostParams* amObj = amCostMap.at(personParams.getHomeLocation()).at(tour->getTourDestination());
		CostParams* pmObj = pmCostMap.at(tour->getTourDestination()).at(personParams.getHomeLocation());
		tmParams.setCostPublicFirst(amObj->getPubCost());
		tmParams.setCostPublicSecond(pmObj->getPubCost());
		tmParams.setCostCarErpFirst(amObj->getCarCostErp());
		tmParams.setCostCarErpSecond(pmObj->getCarCostErp());
		//TODO: I do not know what 0.147 means in the following lines.
		//		Must check with Siyu. ~ Harish
		tmParams.setCostCarOpFirst(amObj->getDistance() * 0.147);
		tmParams.setCostCarOpSecond(pmObj->getDistance() * 0.147);
		tmParams.setWalkDistance1(amObj->getDistance());
		tmParams.setWalkDistance2(pmObj->getDistance());
		tmParams.setTtPublicIvtFirst(amObj->getPubIvt());
		tmParams.setTtPublicIvtSecond(pmObj->getPubIvt());
		tmParams.setTtPublicWaitingFirst(amObj->getPubWtt());
		tmParams.setTtPublicWaitingSecond(pmObj->getPubWtt());
		tmParams.setTtPublicWalkFirst(amObj->getPubWalkt());
		tmParams.setTtPublicWalkSecond(pmObj->getPubWalkt());
		tmParams.setTtCarIvtFirst(amObj->getCarIvt());
		tmParams.setTtCarIvtSecond(pmObj->getCarIvt());
		tmParams.setAvgTransfer((amObj->getAvgTransfer() + pmObj->getAvgTransfer())/2);
		switch(tmParams.getStopType()){
		case WORK:
			tmParams.setDrive1Available(personParams.hasDrivingLicence() * personParams.getCarOwnNormal());
			tmParams.setShare2Available(1);
			tmParams.setShare3Available(1);
			tmParams.setPublicBusAvailable(amObj->getPubIvt() > 0 && pmObj->getPubIvt() > 0);
			tmParams.setMrtAvailable(amObj->getPubIvt() > 0 && pmObj->getPubIvt() > 0);
			tmParams.setPrivateBusAvailable(amObj->getPubIvt() > 0 && pmObj->getPubIvt() > 0);
			tmParams.setWalkAvailable(amObj->getPubIvt() <= WALKABLE_DISTANCE && pmObj->getPubIvt() <= WALKABLE_DISTANCE);
			tmParams.setTaxiAvailable(1);
			tmParams.setMotorAvailable(1);
			break;
		case EDUCATION:
			tmParams.setDrive1Available(personParams.hasDrivingLicence() * personParams.getCarOwnNormal());
			tmParams.setShare2Available(1);
			tmParams.setShare3Available(1);
			tmParams.setPublicBusAvailable(amObj->getPubIvt() > 0 && pmObj->getPubIvt() > 0);
			tmParams.setMrtAvailable(amObj->getPubIvt() > 0 && pmObj->getPubIvt() > 0);
			tmParams.setPrivateBusAvailable(amObj->getPubIvt() > 0 && pmObj->getPubIvt() > 0);
			tmParams.setWalkAvailable(amObj->getPubIvt() <= WALKABLE_DISTANCE && pmObj->getPubIvt() <= WALKABLE_DISTANCE);
			tmParams.setTaxiAvailable(1);
			tmParams.setMotorAvailable(1);
			break;
		}
	}
	else {
		tmParams.setCostPublicFirst(0);
		tmParams.setCostPublicSecond(0);
		tmParams.setCostCarErpFirst(0);
		tmParams.setCostCarErpSecond(0);
		tmParams.setCostCarOpFirst(0);
		tmParams.setCostCarOpSecond(0);
		tmParams.setCostCarParking(0);
		tmParams.setWalkDistance1(0);
		tmParams.setWalkDistance2(0);
		tmParams.setTtPublicIvtFirst(0);
		tmParams.setTtPublicIvtSecond(0);
		tmParams.setTtPublicWaitingFirst(0);
		tmParams.setTtPublicWaitingSecond(0);
		tmParams.setTtPublicWalkFirst(0);
		tmParams.setTtPublicWalkSecond(0);
		tmParams.setTtCarIvtFirst(0);
		tmParams.setTtCarIvtSecond(0);
		tmParams.setAvgTransfer(0);
	}

	tour->setTourMode(PredayLuaProvider::getPredayModel().predictTourMode(personParams, tmParams));
}

void PredaySystem::predictTourModeDestination(Tour* tour) {
	TourModeDestinationParams tmdParams(zoneMap, amCostMap, pmCostMap, personParams, tour->getTourType());
	int modeDest = PredayLuaProvider::getPredayModel().predictTourModeDestination(personParams, tmdParams);
	tour->setTourMode(tmdParams.getMode(modeDest));
	int zone_id = tmdParams.getDestination(modeDest);
	tour->setTourDestination(zoneMap.at(zone_id)->getZoneCode());
}

TimeWindowAvailability PredaySystem::predictTourTimeOfDay(Tour* tour) {
	if(!tour) {
		throw std::runtime_error("predictTourTimeOfDay():: nullptr was passed for tour");
	}
	int timeWndw;
	if(!tour->isSubTour()) {
		int origin = tour->getTourDestination();
		int destination = personParams.getHomeLocation();
		std::vector<double> ttFirstHalfTour, ttSecondHalfTour;
		if(origin != destination) {
			BSONObj bsonObjTT = BSON("origin" << origin << "destination" << destination);
			BSONObj tCostBusDoc;
			mongoDao["tcost_bus"]->getOne(bsonObjTT, tCostBusDoc);
			BSONObj tCostCarDoc;
			mongoDao["tcost_car"]->getOne(bsonObjTT, tCostCarDoc);

			CostParams* amDistanceObj = amCostMap.at(destination).at(origin);
			CostParams* pmDistanceObj = pmCostMap.at(destination).at(origin);

			for (uint32_t i = FIRST_INDEX; i <= LAST_INDEX; i++) {
				switch (tour->getTourMode()) {
				case 1: // Fall through
				case 2:
				case 3:
				{
					std::stringstream arrivalField, departureField;
					arrivalField << "TT_bus_arrival_" << i;
					departureField << "TT_bus_departure_" << i;
					if(tCostBusDoc.getField(arrivalField.str()).isNumber()) {
						ttFirstHalfTour.push_back(tCostBusDoc.getField(arrivalField.str()).Number());
					}
					else {
						ttFirstHalfTour.push_back(HIGH_TRAVEL_TIME);
					}
					if(tCostBusDoc.getField(departureField.str()).isNumber()) {
						ttSecondHalfTour.push_back(tCostBusDoc.getField(departureField.str()).Number());
					}
					else {
						ttSecondHalfTour.push_back(HIGH_TRAVEL_TIME);
					}
					break;
				}
				case 4: // Fall through
				case 5:
				case 6:
				case 7:
				case 9:
				{
					std::stringstream arrivalField, departureField;
					arrivalField << "TT_car_arrival_" << i;
					departureField << "TT_car_departure_" << i;
					if(tCostCarDoc.getField(arrivalField.str()).isNumber()) {
						ttFirstHalfTour.push_back(tCostCarDoc.getField(arrivalField.str()).Number());
					}
					else {
						ttFirstHalfTour.push_back(HIGH_TRAVEL_TIME);
					}
					if(tCostCarDoc.getField(departureField.str()).isNumber()) {
						ttSecondHalfTour.push_back(tCostCarDoc.getField(departureField.str()).Number());
					}
					else {
						ttSecondHalfTour.push_back(HIGH_TRAVEL_TIME);
					}
					break;
				}
				case 8: {
					// TODO: Not sure why we divide by 5. Must check with Siyu. ~ Harish
					double travelTime = (amDistanceObj->getDistance() - pmDistanceObj->getDistance())/5.0;
					ttFirstHalfTour.push_back(travelTime);
					ttSecondHalfTour.push_back(travelTime);
					break;
				}
				}
			}
		}
		else {
			for (uint32_t i = FIRST_INDEX; i <= LAST_INDEX; i++) {
				ttFirstHalfTour.push_back(0);
				ttSecondHalfTour.push_back(0);
			}
		}
		TourTimeOfDayParams todParams(ttFirstHalfTour, ttSecondHalfTour);
		timeWndw = PredayLuaProvider::getPredayModel().predictTourTimeOfDay(personParams, todParams, tour->getTourType());
	}
	return TimeWindowAvailability::timeWindowsLookup.at(timeWndw - 1); //timeWndw ranges from 1 - 1176. Vector starts from 0.
}

void PredaySystem::generateIntermediateStops(Tour* tour) {
	if(tour->stops.size() != 1) {
		stringstream ss;
		ss << "generateIntermediateStops()|tour contains " << tour->stops.size() << " stops. Exactly 1 stop (primary activity) was expected.";
		throw runtime_error(ss.str());
	}
	Stop* primaryStop = tour->stops.front(); // The only stop at this point is the primary activity stop
	Stop* generatedStop = nullptr;

	if ((dayPattern.at("WorkI") + dayPattern.at("EduI") + dayPattern.at("ShopI") + dayPattern.at("OthersI")) > 0 ) {
		//if any stop type was predicted in the day pattern
		StopGenerationParams isgParams(tour, primaryStop, dayPattern);
		int origin = personParams.getHomeLocation();
		int destination = primaryStop->getStopLocation();
		isgParams.setFirstTour(tours.front() == tour);
		TourList::iterator currTourIterator = std::find(tours.begin(), tours.end(), tour);
		size_t numToursAfter = tours.size() - (std::distance(tours.begin(), currTourIterator) /*num. of tours before this tour*/ + 1 /*this tour*/);
		isgParams.setNumRemainingTours(numToursAfter);

		//First half tour
		if(origin != destination) {
			CostParams* amDistanceObj = amCostMap.at(origin).at(destination);
			isgParams.setDistance(amDistanceObj->getDistance());
		}
		else {
			isgParams.setDistance(0.0);
		}
		isgParams.setFirstHalfTour(true);

		double prevDepartureTime = FIRST_INDEX; // first window; start of day
		double nextArrivalTime = primaryStop->getArrivalTime();
		if (tours.front() != tour) { // if this tour is not the first tour of the day
			Tour* previousTour = *(currTourIterator-1);
			prevDepartureTime = previousTour->getEndTime(); // departure time id taken as the end time of the previous tour
		}

		int stopCounter = 0;
		isgParams.setStopCounter(stopCounter);
		int choice = 0; //not a valid choice; just initializing here.

		Stop* nextStop = primaryStop;
		while(choice != QUIT_CHOICE_ISG && stopCounter<MAX_STOPS_IN_HALF_TOUR){
			choice = PredayLuaProvider::getPredayModel().generateIntermediateStop(personParams, isgParams);
			if(choice != QUIT_CHOICE_ISG) {
				StopType stopType;
				switch(choice) {
				case WORK_CHOICE_ISG: stopType = WORK; break;
				case EDU_CHOICE_ISG: stopType = EDUCATION; break;
				case SHOP_CHOICE_ISG: stopType = SHOP; break;
				case OTHER_CHOICE_ISG: stopType = OTHER; break;
				}
				generatedStop = new Stop(stopType, tour, false /*not primary*/, true /*in first half tour*/);
				tour->addStop(generatedStop);
				predictStopModeDestination(generatedStop, nextStop->getStopLocation());
				calculateDepartureTime(generatedStop, nextStop);
				if(generatedStop->getDepartureTime() <= FIRST_INDEX)
				{
					tour->removeStop(generatedStop);
					safe_delete_item(generatedStop);
					stopCounter = stopCounter + 1;
					continue;
				}
				predictStopTimeOfDay(generatedStop, true);
				if(generatedStop->getArrivalTime() > generatedStop->getDepartureTime()) {
					logStream << "Discarding generated stop|"
							<< "|arrival: " << generatedStop->getArrivalTime()
							<< "|departure: " << generatedStop->getDepartureTime()
							<< "|1st HT"
							<< "|arrival time is greater than departure time";
					tour->removeStop(generatedStop);
					safe_delete_item(generatedStop);
					stopCounter = stopCounter + 1;
					continue;
				}
				nextStop = generatedStop;
				personParams.blockTime(generatedStop->getArrivalTime(), generatedStop->getDepartureTime());
				nextArrivalTime = generatedStop->getArrivalTime();
				stopCounter = stopCounter + 1;
				logStream << "Generated stop|type: " << generatedStop->getStopTypeID()
						<< "|mode: " << generatedStop->getStopMode()
						<< "|destination: " << generatedStop->getStopLocation()
						<< "|1st HT "
						<< "|arrival: " << generatedStop->getArrivalTime()
						<< "|departure: " << generatedStop->getDepartureTime()
						<< std::endl;
			}
		}

		generatedStop = nullptr;
		// Second half tour
		if(origin != destination) {
			CostParams* pmDistanceObj = pmCostMap.at(origin).at(destination);
			isgParams.setDistance(pmDistanceObj->getDistance());
		}
		else {
			isgParams.setDistance(0.0);
		}
		isgParams.setFirstHalfTour(false);

		prevDepartureTime = primaryStop->getDepartureTime();
		nextArrivalTime = LAST_WINDOW; // end of day

		stopCounter = 0;
		isgParams.setStopCounter(stopCounter);
		choice = 0;
		Stop* prevStop = primaryStop;
		while(choice != QUIT_CHOICE_ISG && stopCounter<MAX_STOPS_IN_HALF_TOUR){
			choice = PredayLuaProvider::getPredayModel().generateIntermediateStop(personParams, isgParams);
			if(choice != QUIT_CHOICE_ISG) {
				StopType stopType;
				switch(choice) {
				case WORK_CHOICE_ISG: stopType = WORK; break;
				case EDU_CHOICE_ISG: stopType = EDUCATION; break;
				case SHOP_CHOICE_ISG: stopType = SHOP; break;
				case OTHER_CHOICE_ISG: stopType = OTHER; break;
				}
				generatedStop = new Stop(stopType, tour, false /*not primary*/, false  /*not in first half tour*/);
				tour->addStop(generatedStop);
				predictStopModeDestination(generatedStop, prevStop->getStopLocation());
				calculateArrivalTime(generatedStop, prevStop);
				if(generatedStop->getArrivalTime() >=  LAST_INDEX)
				{
					tour->removeStop(generatedStop);
					safe_delete_item(generatedStop);
					stopCounter = stopCounter + 1;
					continue;
				}
				predictStopTimeOfDay(generatedStop, false);
				if(generatedStop->getArrivalTime() > generatedStop->getDepartureTime()) {
					logStream << "Discarding generated stop|"
							<< "|arrival: " << generatedStop->getArrivalTime()
							<< "|departure: " << generatedStop->getDepartureTime()
							<< "|2nd HT"
							<< "|arrival time is greater than departure time";
					tour->removeStop(generatedStop);
					safe_delete_item(generatedStop);
					stopCounter = stopCounter + 1;
					continue;
				}
				prevStop = generatedStop;
				personParams.blockTime(generatedStop->getArrivalTime(), generatedStop->getDepartureTime());
				prevDepartureTime = generatedStop->getDepartureTime();
				stopCounter = stopCounter + 1;
				logStream << "Generated stop|type: " << generatedStop->getStopTypeID()
						<< "|mode: " << generatedStop->getStopMode()
						<< "|destination: " << generatedStop->getStopLocation()
						<< "|2nd HT "
						<< "|arrival: " << generatedStop->getArrivalTime()
						<< "|departure: " << generatedStop->getDepartureTime()
						<< std::endl;
			}
		}
	}
}

void PredaySystem::predictStopModeDestination(Stop* stop, int origin)
{
	StopModeDestinationParams imdParams(zoneMap, amCostMap, pmCostMap, personParams, stop->getStopType(), origin, stop->getParentTour()->getTourMode());
	int modeDest = PredayLuaProvider::getPredayModel().predictStopModeDestination(personParams, imdParams);
	stop->setStopMode(imdParams.getMode(modeDest));
	int zone_id = imdParams.getDestination(modeDest);
	stop->setStopLocationId(zone_id);
	stop->setStopLocation(zoneMap.at(zone_id)->getZoneCode());
}

void PredaySystem::predictStopTimeOfDay(Stop* stop, bool isBeforePrimary) {
	if(!stop) {
		throw std::runtime_error("predictStopTimeOfDay()::nullptr was passed for stop");
	}
	StopTimeOfDayParams stodParams(stop->getStopTypeID(), isBeforePrimary);
	int origin = stop->getStopLocation();
	int destination = personParams.getHomeLocation();

	if(origin != destination) {
		BSONObj bsonObjTT = BSON("origin" << origin << "destination" << destination);
		BSONObj tCostBusDoc;
		mongoDao["tcost_bus"]->getOne(bsonObjTT, tCostBusDoc);
		BSONObj tCostCarDoc;
		mongoDao["tcost_car"]->getOne(bsonObjTT, tCostCarDoc);

		CostParams* amDistanceObj = amCostMap.at(destination).at(origin);
		CostParams* pmDistanceObj = pmCostMap.at(destination).at(origin);

		for (uint32_t i = FIRST_INDEX; i <= LAST_INDEX; i++) {
			switch (stop->getStopMode()) {
			case 1: // Fall through
			case 2:
			case 3:
			{
				std::stringstream fieldName;
				if(stodParams.getFirstBound()) {
					fieldName << "TT_bus_arrival_" << i;
				}
				else {
					fieldName << "TT_bus_departure_" << i;
				}
				if(tCostBusDoc.getField(fieldName.str()).isNumber()) {
					stodParams.travelTimes.push_back(tCostBusDoc.getField(fieldName.str()).Number());
				}
				else {
					stodParams.travelTimes.push_back(HIGH_TRAVEL_TIME);
				}
				break;
			}
			case 4: // Fall through
			case 5:
			case 6:
			case 7:
			case 9:
			{
				std::stringstream fieldName;
				if(stodParams.getFirstBound()) {
					fieldName << "TT_car_arrival_" << i;
				}
				else {
					fieldName << "TT_car_departure_" << i;
				}
				if(tCostCarDoc.getField(fieldName.str()).isNumber()) {
					stodParams.travelTimes.push_back(tCostCarDoc.getField(fieldName.str()).Number());
				}
				else {
					stodParams.travelTimes.push_back(HIGH_TRAVEL_TIME);
				}
				break;
			}
			case 8: {
				double distanceMin = amDistanceObj->getDistance() - pmDistanceObj->getDistance();
				// TODO: Not sure why we are dividing by 5. Must check with Siyu. ~ Harish
				stodParams.travelTimes.push_back(distanceMin/5);
				break;
			}
			}
		}
	}
	else {
		for(int i=FIRST_INDEX; i<=LAST_INDEX; i++) {
			stodParams.travelTimes.push_back(0.0);
		}
	}

	// high and low tod
	if(isBeforePrimary) {
		stodParams.setTodHigh(stop->getDepartureTime());
		if(stop->getParentTour() == tours.front()) {
			stodParams.setTodLow(1);
		}
		else {
			Tour* prevTour = *(std::find(tours.begin(), tours.end(), stop->getParentTour()) - 1);
			stodParams.setTodLow(prevTour->getEndTime());
		}
	}
	else {
		stodParams.setTodLow(stop->getArrivalTime());
		stodParams.setTodHigh(LAST_INDEX); // end of day
	}

	ZoneParams* zoneDoc = zoneMap.at(zoneIdLookup.at(origin));
	if(origin != destination) {
		// calculate costs
		CostParams* amDoc = amCostMap.at(origin).at(destination);
		CostParams* pmDoc = pmCostMap.at(origin).at(destination);
		CostParams* opDoc = opCostMap.at(origin).at(destination);
		double duration, parkingRate, costCarParking, costCarERP, costCarOP, walkDistance;
		//TODO: Not sure what the magic numbers mean in the following lines.
		//		Must check with Siyu. ~ Harish
		for(int i=FIRST_INDEX; i<=LAST_INDEX; i++) {

			if(stodParams.getFirstBound()) {
				duration = stodParams.getTodHigh() - i + 1;
			}
			else { //if(stodParams.getSecondBound())
				duration = i - stodParams.getTodLow() + 1;
			}
			duration = 0.25+(duration-1)*0.5;
			parkingRate = zoneDoc->getParkingRate();
			costCarParking = (8*(duration>8)+duration*(duration<=8))*parkingRate;

			if(i >= AM_PEAK_LOW && i <= AM_PEAK_HIGH) { // time window indexes 10 to 14 are AM Peak windows
				costCarERP = amDoc->getCarCostErp();
				costCarOP = amDoc->getDistance() * 0.147;
				walkDistance = amDoc->getDistance();
			}
			else if (i >= PM_PEAK_LOW && i <= PM_PEAK_HIGH) { // time window indexes 30 to 34 are PM Peak indexes
				costCarERP = pmDoc->getCarCostErp();
				costCarOP = pmDoc->getDistance() * 0.147;
				walkDistance = pmDoc->getDistance();
			}
			else { // other time window indexes are Off Peak indexes
				costCarERP = opDoc->getCarCostErp();
				costCarOP = opDoc->getDistance() * 0.147;
				walkDistance = opDoc->getDistance();
			}

			switch (stop->getStopMode()) {
			case 1: // Fall through
			case 2:
			case 3:
			{
				if(i >= AM_PEAK_LOW && i <= AM_PEAK_HIGH) { // time window indexes 10 to 14 are AM Peak windows
					stodParams.travelCost.push_back(amDoc->getPubCost());
				}
				else if (i >= PM_PEAK_LOW && i <= PM_PEAK_HIGH) { // time window indexes 30 to 34 are PM Peak indexes
					stodParams.travelCost.push_back(pmDoc->getPubCost());
				}
				else { // other time window indexes are Off Peak indexes
					stodParams.travelCost.push_back(opDoc->getPubCost());
				}
				break;
			}
			case 4: // Fall through
			case 5:
			case 6:
			{
				stodParams.travelCost.push_back((costCarParking+costCarOP+costCarERP)/(stop->getStopMode()-3.0));
				break;
			}
			case 7:
			{
				stodParams.travelCost.push_back((0.5*costCarERP+0.5*costCarOP+0.65*costCarParking));
				break;
			}
			case 9:
			{
				stodParams.travelCost.push_back(3.4+costCarERP
						+3*personParams.getIsFemale()
						+((walkDistance*(walkDistance>10)-10*(walkDistance>10))/0.35 + (walkDistance*(walkDistance<=10)+10*(walkDistance>10))/0.4)*0.22);
				break;
			}
			case 8: {
				stodParams.travelCost.push_back(0);
				break;
			}
			}
		}
	}
	else { // if origin and destination are same
		double duration, parkingRate, costCarParking, costCarERP, costCarOP, walkDistance;
		for(int i=FIRST_INDEX; i<=LAST_INDEX; i++) {

			if(stodParams.getFirstBound()) {
				duration = stodParams.getTodHigh() - i + 1;
			}
			else { //if(stodParams.getSecondBound())
				duration = i - stodParams.getTodLow() + 1;
			}
			duration = 0.25+(duration-1)*0.5;
			parkingRate = zoneDoc->getParkingRate();
			costCarParking = (8*(duration>8)+duration*(duration<=8))*parkingRate;

			costCarERP = 0;
			costCarOP = 0;
			walkDistance = 0;

			switch (stop->getStopMode()) {
			case 1: // Fall through
			case 2:
			case 3:
			{
				if(i >= AM_PEAK_LOW && i <= AM_PEAK_HIGH) { // time window indexes 10 to 14 are AM Peak windows
					stodParams.travelCost.push_back(0);
				}
				else if (i >= PM_PEAK_LOW && i <= PM_PEAK_HIGH) { // time window indexes 30 to 34 are PM Peak indexes
					stodParams.travelCost.push_back(0);
				}
				else { // other time window indexes are Off Peak indexes
					stodParams.travelCost.push_back(0);
				}
				break;
			}
			case 4: // Fall through
			case 5:
			case 6:
			{
				stodParams.travelCost.push_back((costCarParking+costCarOP+costCarERP)/(stop->getStopMode()-3.0));
				break;
			}
			case 7:
			{
				stodParams.travelCost.push_back((0.5*costCarERP+0.5*costCarOP+0.65*costCarParking));
				break;
			}
			case 9:
			{
				stodParams.travelCost.push_back(3.4+costCarERP
						+3*personParams.getIsFemale()
						+((walkDistance*(walkDistance>10)-10*(walkDistance>10))/0.35
								+ (walkDistance*(walkDistance<=10)+10*(walkDistance>10))/0.4)*0.22);
				break;
			}
			case 8: {
				stodParams.travelCost.push_back(0);
				break;
			}
			}
		}
	}

	stodParams.updateAvailabilities();

	int timeWindowIdx = PredayLuaProvider::getPredayModel().predictStopTimeOfDay(personParams, stodParams);
	if(isBeforePrimary) {
		if(timeWindowIdx > stop->getDepartureTime()) {
			logStream << "Predicted arrival time must not be greater than the estimated departure time";
		}
		stop->setArrivalTime(timeWindowIdx);
	}
	else {
		if(timeWindowIdx < stop->getArrivalTime()) {
			logStream << "Predicted departure time must not be greater than the estimated arrival time";
		}
		stop->setDepartureTime(timeWindowIdx);
	}
}

void PredaySystem::calculateArrivalTime(Stop* currStop,  Stop* prevStop) { // this function sets the arrival time for currStop
	/*
	 * There are 48 half-hour time windows in a day from 3.25 to 26.75.
	 * Given a time window x, its choice index can be determined by ((x - 2.75) / 0.5) + 1
	 */
	double prevActivityDepartureIndex = prevStop->getDepartureTime();
	double timeWindow = getTimeWindowFromIndex(prevActivityDepartureIndex);
	double travelTime;

	if(currStop->getStopLocation() != prevStop->getStopLocation()) {
		travelTime = HIGH_TRAVEL_TIME; // initializing to a high value just in case something goes wrong. tcost_bus and tcost_car has lot of inadmissible data ("NULL")
		std::stringstream fieldName;
		BSONObj bsonObj = BSON("origin" << currStop->getStopLocation() << "destination" << prevStop->getStopLocation());

		switch(prevStop->getStopMode()) {
		case 1: // Fall through
		case 2:
		case 3:
		{
			BSONObj tCostBusDoc;
			mongoDao["tcost_bus"]->getOne(bsonObj, tCostBusDoc);
			fieldName << "TT_bus_departure_" << prevActivityDepartureIndex;
			if(tCostBusDoc.getField(fieldName.str()).isNumber()) {
				travelTime = tCostBusDoc.getField(fieldName.str()).Number();
			}
			break;
		}
		case 4: // Fall through
		case 5:
		case 6:
		case 7:
		case 9:
		{
			BSONObj tCostCarDoc;
			mongoDao["tcost_car"]->getOne(bsonObj, tCostCarDoc);
			fieldName << "TT_car_departure_" << prevActivityDepartureIndex;
			if(tCostCarDoc.getField(fieldName.str()).isNumber()) {
				travelTime = tCostCarDoc.getField(fieldName.str()).Number();
			}
			break;
		}
		case 8:
		{
			double distanceMin = amCostMap.at(currStop->getStopLocation()).at(prevStop->getStopLocation())->getDistance()
												- pmCostMap.at(currStop->getStopLocation()).at(prevStop->getStopLocation())->getDistance();
			travelTime = distanceMin/5;
			break;
		}
		}
	}
	else {
		travelTime = 0.0;
	}

	double currStopArrTime = timeWindow + travelTime;

	// travel time can be unreasonably high sometimes
	// E.g. when the travel time is unknown, the default is set to 999
	currStopArrTime = alignTime(currStopArrTime);

	currStopArrTime = getIndexFromTimeWindow(currStopArrTime);
	currStop->setArrivalTime(currStopArrTime);
}

void PredaySystem::calculateDepartureTime(Stop* currStop,  Stop* nextStop) { // this function sets the departure time for the currStop
	/*
	 * There are 48 half-hour time windows in a day from 3.25 to 26.75.
	 * Given a time window i, its choice index can be determined by (i * 0.5 + 2.75)
	 */
	double nextActivityArrivalIndex = nextStop->getArrivalTime();
	double timeWindow = getTimeWindowFromIndex(nextActivityArrivalIndex);
	double travelTime;
	if(currStop->getStopLocation() != nextStop->getStopLocation()) {
		travelTime = HIGH_TRAVEL_TIME; // initializing to a high value just in case something goes wrong. tcost_bus and tcost_car has lot of inadmissable data ("NULL")
		std::stringstream fieldName;
		BSONObj queryObj = BSON("origin" << currStop->getStopLocation() << "destination" << nextStop->getStopLocation());
		switch(nextStop->getStopMode()) {
		case 1: // Fall through
		case 2:
		case 3:
		{
			BSONObj tCostBusDoc;
			mongoDao["tcost_bus"]->getOne(queryObj, tCostBusDoc);
			fieldName << "TT_bus_arrival_" << nextActivityArrivalIndex;
			if(tCostBusDoc.getField(fieldName.str()).isNumber()) {
				travelTime = tCostBusDoc.getField(fieldName.str()).Number();
			}
			break;
		}
		case 4: // Fall through
		case 5:
		case 6:
		case 7:
		case 9:
		{
			BSONObj tCostCarDoc;
			mongoDao["tcost_car"]->getOne(queryObj, tCostCarDoc);
			fieldName << "TT_car_arrival_" << nextActivityArrivalIndex;
			if(tCostCarDoc.getField(fieldName.str()).isNumber()) {
				travelTime = tCostCarDoc.getField(fieldName.str()).Number();
			}
			break;
		}
		case 8:
		{
			double distanceMin = amCostMap.at(currStop->getStopLocation()).at(nextStop->getStopLocation())->getDistance()
									- pmCostMap.at(currStop->getStopLocation()).at(nextStop->getStopLocation())->getDistance();
			travelTime = distanceMin/5;
			break;
		}
		}
	}
	else {
		travelTime = 0.0;
	}

	double currStopDepTime = timeWindow - travelTime;

	// travel time can be unreasonably high sometimes
	// E.g. when the travel time is unknown, the default is set to 999
	currStopDepTime = alignTime(currStopDepTime);

	currStopDepTime = getIndexFromTimeWindow(currStopDepTime);
	currStop->setDepartureTime(currStopDepTime);
}

void PredaySystem::calculateTourStartTime(Tour* tour) {
	/*
	 * There are 48 half-hour time windows in a day from 3.25 to 26.75.
	 * Given a time window i, its choice index can be determined by (i * 0.5 + 2.75)
	 */
	Stop* firstStop = tour->stops.front();
	double firstActivityArrivalIndex = firstStop->getArrivalTime();
	double timeWindow = getTimeWindowFromIndex(firstActivityArrivalIndex);
	double travelTime;
	if(personParams.getHomeLocation() != firstStop->getStopLocation()) {
		travelTime = HIGH_TRAVEL_TIME; // initializing to a high value just in case something goes wrong. tcost_bus and tcost_car has lot of inadmissable data ("NULL")
		std::stringstream fieldName;
		BSONObj bsonObj = BSON("origin" << personParams.getHomeLocation() << "destination" << firstStop->getStopLocation());
		switch(firstStop->getStopMode()) {
		case 1: // Fall through
		case 2:
		case 3:
		{
			BSONObj tCostBusDoc;
			mongoDao["tcost_bus"]->getOne(bsonObj, tCostBusDoc);
			fieldName << "TT_bus_arrival_" << firstActivityArrivalIndex;
			if(tCostBusDoc.getField(fieldName.str()).isNumber()) {
				travelTime = tCostBusDoc.getField(fieldName.str()).Number();
			}
			break;
		}
		case 4: // Fall through
		case 5:
		case 6:
		case 7:
		case 9:
		{
			BSONObj tCostCarDoc;
			mongoDao["tcost_car"]->getOne(bsonObj, tCostCarDoc);
			fieldName << "TT_car_arrival_" << firstActivityArrivalIndex;
			if(tCostCarDoc.getField(fieldName.str()).isNumber()) {
				travelTime = tCostCarDoc.getField(fieldName.str()).Number();
			}
			break;
		}
		case 8:
		{
			double distanceMin = amCostMap.at(personParams.getHomeLocation()).at(firstStop->getStopLocation())->getDistance()
									- pmCostMap.at(personParams.getHomeLocation()).at(firstStop->getStopLocation())->getDistance();
			travelTime = distanceMin/5;
			break;
		}
		}
	}
	else {
		travelTime = 0.0;
	}

	double tourStartTime = timeWindow - travelTime;

	// travel time can be unreasonably high sometimes
	// E.g. when the travel time is unknown, the default is set to 999
	tourStartTime = alignTime(tourStartTime);

	tourStartTime = getIndexFromTimeWindow(tourStartTime);
	tour->setStartTime(tourStartTime);
}

void PredaySystem::calculateTourEndTime(Tour* tour) {
	/*
	 * There are 48 half-hour time windows in a day from 3.25 to 26.75.
	 * Given a time window x, its choice index can be determined by ((x - 3.25) / 0.5) + 1
	 */
	Stop* lastStop = tour->stops.back();
	double lastActivityDepartureIndex = lastStop->getDepartureTime();
	double timeWindow = getTimeWindowFromIndex(lastActivityDepartureIndex);
	double travelTime;
	if(personParams.getHomeLocation() != lastStop->getStopLocation()) {
		travelTime = HIGH_TRAVEL_TIME; // initializing to a high value just in case something goes wrong. tcost_bus and tcost_car has lot of inadmissable data ("NULL")
		std::stringstream fieldName;
		BSONObj bsonObj = BSON("origin" << personParams.getHomeLocation() << "destination" << lastStop->getStopLocation());

		switch(lastStop->getStopMode()) {
		case 1: // Fall through
		case 2:
		case 3:
		{
			BSONObj tCostBusDoc;
			mongoDao["tcost_bus"]->getOne(bsonObj, tCostBusDoc);
			fieldName << "TT_bus_departure_" << lastActivityDepartureIndex;
			if(tCostBusDoc.getField(fieldName.str()).isNumber()) {
				travelTime = tCostBusDoc.getField(fieldName.str()).Number();
			}
			break;
		}
		case 4: // Fall through
		case 5:
		case 6:
		case 7:
		case 9:
		{
			BSONObj tCostCarDoc;
			mongoDao["tcost_car"]->getOne(bsonObj, tCostCarDoc);
			fieldName << "TT_car_departure_" << lastActivityDepartureIndex;
			if(tCostCarDoc.getField(fieldName.str()).isNumber()) {
				travelTime = tCostCarDoc.getField(fieldName.str()).Number();
			}
			break;
		}
		case 8:
		{
			double distanceMin = amCostMap.at(personParams.getHomeLocation()).at(lastStop->getStopLocation())->getDistance()
													- pmCostMap.at(personParams.getHomeLocation()).at(lastStop->getStopLocation())->getDistance();
			travelTime = distanceMin/5;
			break;
		}
		}
	}
	else {
		travelTime = 0.0;
	}

	double tourEndTime = timeWindow + travelTime;

	// travel time can be unreasonably high sometimes
	// E.g. when the travel time is unknown, the default is set to 999
	tourEndTime = alignTime(tourEndTime);

	tourEndTime = getIndexFromTimeWindow(tourEndTime);
	tour->setEndTime(tourEndTime);
}

void PredaySystem::constructTours() {
	if(numTours.size() != 4) {
		// Probably predictNumTours() was not called prior to this function
		throw std::runtime_error("Tours cannot be constructed before predicting number of tours for each tour type");
	}

	//Construct work tours
	bool firstOfMultiple = true;
	for(int i=0; i<numTours["WorkT"]; i++) {
		bool attendsUsualWorkLocation = false;
		if(!(personParams.isStudent() == 1) && (personParams.getFixedWorkLocation() != 0)) {
			//if person not a student and has a fixed work location
			attendsUsualWorkLocation = predictUsualWorkLocation(firstOfMultiple); // Predict if this tour is to a usual work location
			firstOfMultiple = false;
		}
		logStream << "Attends usual work location: " << attendsUsualWorkLocation << std::endl;
		Tour* workTour = new Tour(WORK);
		workTour->setUsualLocation(attendsUsualWorkLocation);
		if(attendsUsualWorkLocation) {
			workTour->setTourDestination(personParams.getFixedWorkLocation());
		}
		tours.push_back(workTour);
	}

	// Construct education tours
	for(int i=0; i<numTours["EduT"]; i++) {
		Tour* eduTour = new Tour(EDUCATION);
		eduTour->setUsualLocation(true); // Education tours are always to usual locations
		eduTour->setTourDestination(personParams.getFixedSchoolLocation());
		if(personParams.isStudent()) {
			// if the person is a student, his education tours should be processed before other tour types.
			tours.push_front(eduTour); // if the person is a student, his education tours should be processed before other tour types.
		}
		else {
			tours.push_back(eduTour);
		}
	}

	// Construct shopping tours
	for(int i=0; i<numTours["ShopT"]; i++) {
		tours.push_back(new Tour(SHOP));
	}

	// Construct other tours
	for(int i=0; i<numTours["OthersT"]; i++) {
		tours.push_back(new Tour(OTHER));
	}
}

void PredaySystem::planDay() {
	personParams.initTimeWindows();
	logStream << std::endl << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
	//Predict day pattern
	logStream << "Person: " << personParams.getPersonId() << "| home: " << personParams.getHomeLocation() << std:: endl;
	logStream << "Day Pattern: " ;
	PredayLuaProvider::getPredayModel().predictDayPattern(personParams, dayPattern);
	logStream << dayPattern["WorkT"] << dayPattern["EduT"] << dayPattern["ShopT"] << dayPattern["OthersT"]
	        << dayPattern["WorkI"] << dayPattern["EduI"] << dayPattern["ShopI"] << dayPattern["OthersI"] << std::endl;

	//Predict number of Tours
	if(dayPattern.size() <= 0) {
		throw std::runtime_error("Cannot invoke number of tours model without a day pattern");
	}
	logStream << "Num. Tours: ";
	PredayLuaProvider::getPredayModel().predictNumTours(personParams, dayPattern, numTours);
	logStream << numTours["WorkT"] << numTours["EduT"] << numTours["ShopT"] << numTours["OthersT"] << std::endl;

	//Construct tours.
	constructTours();

	//Process each tour
	logStream << "Tours: " << tours.size() << std::endl;
	for(TourList::iterator tourIt=tours.begin(); tourIt!=tours.end(); tourIt++) {
		Tour* tour = *tourIt;
		if(tour->isUsualLocation()) {
			// Predict just the mode for tours to usual location
			predictTourMode(tour);
			logStream << "Tour|type: " << tour->getTourType()
					<< "(TM) Tour mode: " << tour->getTourMode() << "|Tour destination: " << tour->getTourDestination();
		}
		else {
			// Predict mode and destination for tours to not-usual locations
			predictTourModeDestination(tour);
			logStream << "Tour|type: " << tour->getTourType()
					<< "(TMD) Tour mode: " << tour->getTourMode() << "|Tour destination: " << tour->getTourDestination();
		}

		// Predict time of day for this tour
		TimeWindowAvailability timeWindow = predictTourTimeOfDay(tour);
		Stop* primaryActivity = new Stop(tour->getTourType(), tour, true /*primary activity*/, true /*stop in first half tour*/);
		primaryActivity->setStopMode(tour->getTourMode());
		primaryActivity->setStopLocation(tour->getTourDestination());
		primaryActivity->setStopLocationId(zoneIdLookup.at(tour->getTourDestination()));
		primaryActivity->allotTime(timeWindow.getStartTime(), timeWindow.getEndTime());
		tour->setPrimaryStop(primaryActivity);
		tour->addStop(primaryActivity);
		personParams.blockTime(timeWindow.getStartTime(), timeWindow.getEndTime());
		logStream << "|primary activity|arrival: " << primaryActivity->getArrivalTime() << "|departure: " << primaryActivity->getDepartureTime() << std::endl;

		//Generate stops for this tour
		generateIntermediateStops(tour);

		calculateTourStartTime(tour);
		calculateTourEndTime(tour);
		personParams.blockTime(tour->getStartTime(), tour->getEndTime());
		logStream << "Tour|start time: " << tour->getStartTime() << "|end time: " << tour->getEndTime() << std::endl;
	}
}

void sim_mob::medium::PredaySystem::insertDayPattern()
{
	int tourCount = numTours["WorkT"] + numTours["EduT"] + numTours["ShopT"] + numTours["OthersT"];
	std::stringstream dpStream;
	dpStream << dayPattern["WorkT"] << "," << dayPattern["EduT"] << "," << dayPattern["ShopT"] << "," << dayPattern["OthersT"]
	        << "," << dayPattern["WorkI"] << "," << dayPattern["EduI"] << "," << dayPattern["ShopI"] << "," << dayPattern["OthersI"];
	BSONObj dpDoc = BSON(
					"_id" << personParams.getPersonId() <<
					"num_tours" << tourCount <<
					"day_pattern" << dpStream.str() <<
					"person_type_id" << personParams.getPersonTypeId() <<
					"hhfactor" << personParams.getHouseholdFactor()
					);
	mongoDao["Output_DayPattern"]->insert(dpDoc);
}

void sim_mob::medium::PredaySystem::insertTour(Tour* tour, int tourNumber) {
	int tourCount = numTours["WorkT"] + numTours["EduT"] + numTours["ShopT"] + numTours["OthersT"];
	BSONObj tourDoc = BSON(
		"person_type_id" << personParams.getPersonTypeId() <<
		"num_stops" << (int)tour->stops.size() <<
		"prim_arr" << tour->getPrimaryStop()->getArrivalTime() <<
		"start_time" << tour->getStartTime() <<
		"destination" << tour->getTourDestination() <<
		"tour_type" << tour->getTourTypeStr() <<
		"num_tours" << tourCount <<
		"prim_dept" << tour->getPrimaryStop()->getDepartureTime() <<
		"end_time" << tour->getEndTime() <<
		"person_id" << personParams.getPersonId() <<
		"tour_mode" << tour->getTourMode() <<
		"tour_num" << tourNumber <<
		"usual_location" << (tour->isUsualLocation()? 1 : 0) <<
		"hhfactor" << personParams.getHouseholdFactor()
	);
	mongoDao["Output_Tour"]->insert(tourDoc);
}

void sim_mob::medium::PredaySystem::insertStop(Stop* stop, int stopNumber, int tourNumber)
{
	BSONObj stopDoc = BSON(
	"arrival" << stop->getArrivalTime() <<
	"destination" << stop->getStopLocation() <<
	"primary" << stop->isPrimaryActivity() <<
	"departure" << stop->getDepartureTime() <<
	"stop_ctr" << stopNumber <<
	"stop_type" << stop->getStopTypeStr() <<
	"person_id" << personParams.getPersonId() <<
	"tour_num" << tourNumber <<
	"stop_mode" << stop->getStopMode() <<
	"hhfactor" << personParams.getHouseholdFactor()
	);
	mongoDao["Output_Activity"]->insert(stopDoc);
}

std::string sim_mob::medium::PredaySystem::getRandomTimeInWindow(double mid) {
	int hour = int(std::floor(mid));
	int minute = (Utils::generateInt(0,29)) + ((mid - hour - 0.25)*60);
	std::stringstream random_time;
	hour = hour % 24;
	if (hour < 10) {
		random_time << "0" << hour << ":";
	}
	else {
		random_time << hour << ":";
	}
	if (minute < 10) {
		random_time << "0" << minute << ":";
	}
	else {
		random_time << minute << ":";
	}
	random_time << "00"; //seconds
	return random_time.str();
}

long sim_mob::medium::PredaySystem::getRandomNodeInZone(const std::vector<ZoneNodeParams*>& nodes) const {
	size_t numNodes = nodes.size();
	if(numNodes == 0) { return 0; }
	if(numNodes == 1)
	{
		const ZoneNodeParams* znNdPrms = nodes.front();
		if(znNdPrms->isSinkNode() || znNdPrms->isSourceNode()) { return 0; }
		return znNdPrms->getAimsunNodeId();
	}

	int offset = Utils::generateInt(0,numNodes-1);
	std::vector<ZoneNodeParams*>::const_iterator it = nodes.begin();
	std::advance(it, offset);
	size_t numAttempts = 1;
	while(numAttempts <= numNodes)
	{
		const ZoneNodeParams* znNdPrms = (*it);
		if(znNdPrms->isSinkNode() || znNdPrms->isSourceNode())
		{
			it++; // check the next one
			if(it==nodes.end()) { it = nodes.begin(); } // loop around
			numAttempts++;
		}
		else { return znNdPrms->getAimsunNodeId(); }
	}
	return 0;
}

void sim_mob::medium::PredaySystem::computeLogsums()
{
	TourModeDestinationParams tmdParams(zoneMap, amCostMap, pmCostMap, personParams, NULL_STOP);
	PredayLuaProvider::getPredayModel().computeTourModeDestinationLogsum(personParams, tmdParams);
	logStream << "Person: " << personParams.getPersonId()
			<< "|updated logsums- work: " << personParams.getWorkLogSum()
			<< ", shop: " << personParams.getShopLogSum()
			<< ", other: " << personParams.getOtherLogSum()
			<<std::endl;
}

void sim_mob::medium::PredaySystem::outputPredictionsToMongo() {
	insertDayPattern();
	int tourNum=0;
	Tour* currTour = nullptr;
	for(TourList::iterator tourIt=tours.begin(); tourIt!=tours.end(); tourIt++) {
		tourNum++;
		currTour=*tourIt;
		insertTour(currTour, tourNum);
		int stopNum=0;
		for(StopList::iterator stopIt=currTour->stops.begin(); stopIt!=currTour->stops.end(); stopIt++) {
			stopNum++;
			insertStop(*stopIt, stopNum, tourNum);
		}
	}
}

void sim_mob::medium::PredaySystem::updateLogsumsToMongo()
{
	BSONObj query = BSON("_id" << personParams.getPersonId());
	BSONObj updateObj = BSON("$set" << BSON(
			"worklogsum"<< personParams.getWorkLogSum() <<
			"shoplogsum" << personParams.getShopLogSum() <<
			"otherlogsum" << personParams.getOtherLogSum()
			));
	mongoDao["population"]->update(query, updateObj);
}

void sim_mob::medium::PredaySystem::constructTripChains(const ZoneNodeMap& zoneNodeMap, long hhFactor, std::list<TripChainItemParams>& tripChain)
{
	std::srand(clock());
	std::string personId = personParams.getPersonId();
	for(long k=1; k<=hhFactor; k++)
	{
		std::string pid;
		{
			std::stringstream sclPersonIdStrm;
			sclPersonIdStrm << personId << "-" << k;
			pid = sclPersonIdStrm.str();
		}
		int seqNum = 0;
		int prevNode = 0;
		int nextNode = 0;
		std::string prevDeptTime = "";
		std::string primaryMode = "";
		bool atHome = true;
		int homeNode = 0;
		if(zoneNodeMap.find(personParams.getHomeLocation()) != zoneNodeMap.end())
		{
			homeNode =  getRandomNodeInZone(zoneNodeMap.at(personParams.getHomeLocation()));
		}
		if(homeNode == 0) { return; } //do not insert this person at all
		int tourNum = 0;
		for(TourList::iterator tourIt = tours.begin(); tourIt != tours.end(); tourIt++)
		{
			tourNum = tourNum + 1;
			Tour* tour = *tourIt;
			int stopNum = 0;
			bool nodeMappingFailed = false;
			for(StopList::iterator stopIt = tour->stops.begin(); stopIt != tour->stops.end(); stopIt++)
			{
				stopNum = stopNum + 1;
				Stop* stop = *stopIt;
				int nextNode = 0;
				if(zoneNodeMap.find(stop->getStopLocation()) != zoneNodeMap.end())
				{
					nextNode = getRandomNodeInZone(zoneNodeMap.at(stop->getStopLocation()));
				}
				if(nextNode == 0) { nodeMappingFailed = true; break; } // if there is no next node, cut the trip chain for this tour here
				seqNum = seqNum + 1;
				TripChainItemParams tcTrip;
				tcTrip.setPersonId(pid);
				tcTrip.setTcSeqNum(seqNum);
				tcTrip.setTcItemType("Trip");
				std::stringstream tripId;
				tripId << pid << "-" << tourNum << "-" << seqNum;
				tcTrip.setTripId(tripId.str());
				tripId << "-1"; //first and only subtrip
				tcTrip.setSubtripId(tripId.str());
				//tcTrip.setSubtripMode(modeMap.at(stop->getStopMode()));
				//tcTrip.setPrimaryMode((tour->getTourMode() == stop->getStopMode()));
				tcTrip.setSubtripMode(modeMap.at(4)); /*~ all trips are made to car trips. Done for running mid-term for TRB paper. ~*/
				tcTrip.setPrimaryMode(true); /*~ running mid-term for TRB paper. ~*/
				tcTrip.setActivityId("0");
				tcTrip.setActivityType("dummy");
				tcTrip.setActivityLocation(0);
				tcTrip.setPrimaryActivity(false);
				if(atHome)
				{
					// first trip in the tour
					tcTrip.setTripOrigin(homeNode);
					tcTrip.setTripDestination(nextNode);
					tcTrip.setSubtripOrigin(homeNode);
					tcTrip.setSubtripDestination(nextNode);
					std::string startTimeStr = getRandomTimeInWindow(getTimeWindowFromIndex(tour->getStartTime()));
					tcTrip.setStartTime(startTimeStr);
					tripChain.push_back(tcTrip);
					atHome = false;
				}
				else
				{
					tcTrip.setTripOrigin(prevNode);
					tcTrip.setTripDestination(nextNode);
					tcTrip.setSubtripOrigin(prevNode);
					tcTrip.setSubtripDestination(nextNode);
					tcTrip.setStartTime(prevDeptTime);
					tripChain.push_back(tcTrip);
				}
				seqNum = seqNum + 1;
				TripChainItemParams tcActivity;
				tcActivity.setPersonId(pid);
				tcActivity.setTcSeqNum(seqNum);
				tcActivity.setTcItemType("Activity");
				tcActivity.setTripId("0");
				tcActivity.setSubtripId("0");
				tcActivity.setPrimaryMode(false);
				tcActivity.setTripOrigin(0);
				tcActivity.setTripDestination(0);
				tcActivity.setSubtripOrigin(0);
				tcActivity.setSubtripDestination(0);
				std::stringstream actId;
				actId << pid << "-" << tourNum << "-" << seqNum;
				tcActivity.setActivityId(actId.str());
				tcActivity.setActivityType(stop->getStopTypeStr());
				tcActivity.setActivityLocation(nextNode);
				tcActivity.setPrimaryActivity(stop->isPrimaryActivity());
				std::string arrTimeStr = getRandomTimeInWindow(getTimeWindowFromIndex(stop->getArrivalTime()));
				std::string deptTimeStr = getRandomTimeInWindow(getTimeWindowFromIndex(stop->getDepartureTime()));
				tcActivity.setActivityStartTime(arrTimeStr);
				tcActivity.setActivityEndTime(deptTimeStr);
				tripChain.push_back(tcActivity);
				prevNode = nextNode; //activity location
				prevDeptTime = deptTimeStr;
			}
			if(nodeMappingFailed)
			{
				break; // ignore remaining tours as well.
			}
			else
			{
				// insert last trip in tour
				seqNum = seqNum + 1;
				TripChainItemParams tcTrip;
				tcTrip.setPersonId(pid);
				tcTrip.setTcSeqNum(seqNum);
				tcTrip.setTcItemType("Trip");
				std::stringstream tripId;
				tripId << personId << "_" << tourNum << "_" << seqNum;
				tcTrip.setTripId(tripId.str());
				tripId << "_1"; //first and only subtrip
				tcTrip.setSubtripId(tripId.str());
				//tcTrip.setSubtripMode(modeMap.at(tour->stops.back()->getStopMode()));
				//tcTrip.setPrimaryMode((tour->getTourMode() == tour->stops.back()->getStopMode()));
				tcTrip.setSubtripMode(modeMap.at(4)); /*~ all trips are made to car trips. Done for running mid-term for TRB paper. ~*/
				tcTrip.setPrimaryMode(true); /*~ running mid-term for TRB paper. ~*/
				tcTrip.setStartTime(prevDeptTime);
				tcTrip.setTripOrigin(prevNode);
				tcTrip.setTripDestination(homeNode);
				tcTrip.setSubtripOrigin(prevNode);
				tcTrip.setSubtripDestination(homeNode);
				tcTrip.setActivityId("0");
				tcTrip.setActivityType("dummy");
				tcTrip.setActivityLocation(0);
				tcTrip.setPrimaryActivity(false);
				tripChain.push_back(tcTrip);
				atHome = true;
			}
		}
	}
}

void sim_mob::medium::PredaySystem::outputTripChainsToPostgreSQL(const ZoneNodeMap& zoneNodeMap, TripChainSqlDao& tripChainDao)
{
	size_t numTours = tours.size();
	if (numTours == 0) { return; }
	long hhFactor = (long)std::ceil(personParams.getHouseholdFactor());
	std::list<TripChainItemParams> tripChain;
	constructTripChains(zoneNodeMap, hhFactor, tripChain);
	for(std::list<TripChainItemParams>::iterator tcIt=tripChain.begin(); tcIt!=tripChain.end();tcIt++)
	{
		tripChainDao.insert(*tcIt);
	}
}

void sim_mob::medium::PredaySystem::outputTripChainsToStream(const ZoneNodeMap& zoneNodeMap, std::stringstream& tripChainStream)
{
	size_t numTours = tours.size();
	if (numTours == 0) { return; }
	long hhFactor = (long)std::ceil(personParams.getHouseholdFactor());
	std::list<TripChainItemParams> tripChains;
	constructTripChains(zoneNodeMap, hhFactor, tripChains);
	for(std::list<TripChainItemParams>::const_iterator tcIt=tripChains.begin(); tcIt!=tripChains.end();tcIt++)
	{
		/*
		  ------------------ DATABASE preday_trip_chain_flat FIELDS for reference --------------------------------
		  person_id character varying NOT NULL,
		  tc_seq_no integer NOT NULL,
		  tc_item_type character varying,
		  trip_id character varying,
		  trip_origin integer,
		  trip_from_loc_type character varying DEFAULT 'node'::character varying,
		  trip_destination integer,
		  trip_to_loc_type character varying DEFAULT 'node'::character varying,
		  subtrip_id character varying,
		  subtrip_origin integer,
		  subtrip_from_loc_type character varying DEFAULT 'node'::character varying,
		  subtrip_destination integer,
		  subtrip_to_loc_type character varying DEFAULT 'node'::character varying,
		  subtrip_mode character varying,
		  is_primary_mode boolean,
		  start_time character varying,
		  pt_line_id character varying DEFAULT ''::character varying,
		  activity_id character varying,
		  activity_type character varying,
		  is_primary_activity boolean,
		  flexible_activity boolean DEFAULT false,
		  mandatory_activity boolean DEFAULT true,
		  activity_location integer,
		  activity_loc_type character varying DEFAULT 'node'::character varying,
		  activity_start_time character varying,
		  activity_end_time character varying
		*/
		const TripChainItemParams& data = (*tcIt);
		tripChainStream << data.getPersonId() << ",";
		tripChainStream << data.getTcSeqNum() << ",";
		tripChainStream << data.getTcItemType() << ",";
		tripChainStream << data.getTripId() << ",";
		tripChainStream << data.getTripOrigin() << "," << "node" << ",";
		tripChainStream << data.getTripDestination() << "," << "node" << ",";
		tripChainStream << data.getSubtripId() << ",";
		tripChainStream << data.getSubtripOrigin() << "," << "node" << ",";
		tripChainStream << data.getSubtripDestination() << "," << "node" << ",";
		tripChainStream << data.getSubtripMode() << ",";
		tripChainStream << (data.isPrimaryMode()? "True":"False") << ",";
		tripChainStream << data.getStartTime() << ",";
		tripChainStream << "\"\"" << ","; //public transit line id
		tripChainStream << data.getActivityId() << ",";
		tripChainStream << data.getActivityType() << ",";
		tripChainStream << (data.isPrimaryActivity()? "True":"False") << ",";
		tripChainStream << "False" << "," << "True" << ","; //flexible and mandatory activity
		tripChainStream << data.getActivityLocation() << "," << "node" << ",";
		tripChainStream << data.getActivityStartTime() << ",";
		tripChainStream << data.getActivityEndTime() << "\n";
	}
}

void sim_mob::medium::PredaySystem::printLogs()
{
	Print() << logStream.str();
}

void sim_mob::medium::PredaySystem::updateStatistics(CalibrationStatistics& statsCollector) const
{
	double householdFactor = personParams.getHouseholdFactor();
	statsCollector.addToTourCountStats(tours.size(), householdFactor);
	for(TourList::const_iterator tourIt=tours.begin(); tourIt!=tours.end(); tourIt++)
	{
		const Tour* tour = (*tourIt);
		statsCollector.addToStopCountStats(tour->stops.size()-1, householdFactor);
		statsCollector.addToTourModeShareStats(tour->getTourMode(), householdFactor);
		const StopList& stops = tour->stops;
		int origin = personParams.getHomeLocation();
		int destination = 0;
		for(StopList::const_iterator stopIt=stops.begin(); stopIt!=stops.end(); stopIt++)
		{
			const Stop* stop = (*stopIt);
			if(!stop->isPrimaryActivity())
			{
				statsCollector.addToTripModeShareStats(stop->getStopMode(), householdFactor);
			}
			destination = stop->getStopLocation();
			if(origin != destination) { statsCollector.addToTravelDistanceStats(opCostMap.at(origin).at(destination)->getDistance(), householdFactor); }
			else { statsCollector.addToTravelDistanceStats(0, householdFactor); }
			origin = destination;
		}
		//There is still one more trip from last stop to home
		destination = personParams.getHomeLocation();
		if(origin != destination) { statsCollector.addToTravelDistanceStats(opCostMap.at(origin).at(destination)->getDistance(), householdFactor); }
		else { statsCollector.addToTravelDistanceStats(0, householdFactor); }
	}
}
