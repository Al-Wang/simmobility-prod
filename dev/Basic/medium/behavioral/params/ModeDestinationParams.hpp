//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * TourModeDestinationParams.hpp
 *
 *  Created on: Nov 30, 2013
 *      Author: Harish Loganathan
 */

#pragma once
#include <boost/unordered_map.hpp>
#include <cmath>
#include <string>
#include "behavioral/params/ZoneCostParams.hpp"
#include "behavioral/PredayClasses.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "conf/Constructs.hpp"
#include "database/ZoneCostMongoDao.hpp"
#include "mongo/client/dbclient.h"
#include "logging/Log.hpp"

namespace sim_mob {
namespace medium {

/**
 * Base class for tour and stop mode destination params
 */
class ModeDestinationParams {
protected:
	typedef boost::unordered_map<int, ZoneParams*> ZoneMap;
	typedef boost::unordered_map<int, boost::unordered_map<int, CostParams*> > CostMap;

	StopType purpose;
	int origin;
	const double OPERATIONAL_COST;
	const double MAX_WALKING_DISTANCE;
	const ZoneMap& zoneMap;
	const CostMap& amCostsMap;
	const CostMap& pmCostsMap;

public:
	ModeDestinationParams(const ZoneMap& zoneMap, const CostMap& amCostsMap, const CostMap& pmCostsMap, StopType purpose, int originCode)
	: zoneMap(zoneMap), amCostsMap(amCostsMap), pmCostsMap(pmCostsMap), purpose(purpose), origin(originCode), OPERATIONAL_COST(0.147), MAX_WALKING_DISTANCE(2)
	{}

	virtual ~ModeDestinationParams() {}

	/**
	 * Returns the mode of a particular choice
	 *
	 * @param choice an integer ranging from 1 to (numZones*numModes = 9828) representing a unique combination of mode and destination
	 * 			(1 to numZones) = mode 1, (numZones+1 to 2*numZones) = mode 2, (2*numZones+1 to 3*numZones) = mode 3, and so on for 9 modes.
	 * @return the mode represented in choice
	 */
	int getMode(int choice) const {
		int nZones = zoneMap.size();
		int nModes = 9;
		if (choice < 1 || choice > nZones*nModes) {
			throw std::runtime_error("getMode()::invalid choice id for mode-destination model");
		}
		return ((choice-1)/nZones + 1);
	}

	/**
	 * Returns the destination of a particular choice
	 *
	 * @param choice an integer ranging from 1 to (numZones*numModes = 9828) representing a unique combination of mode and destination
	 * @return the destination represented in choice
	 */
	int getDestination(int choice) const {
		int nZones = zoneMap.size();
		int nModes = 9;
		if (choice < 1 || choice > nZones*nModes) {
			throw std::runtime_error("getDestination()::invalid choice id for mode-destination model");
		}
		int zoneId = choice % nZones;
		if(zoneId == 0) { // zoneId will become zero for zone 1092.
			zoneId = nZones;
		}
		return zoneId;
	}
};

class TourModeDestinationParams : public ModeDestinationParams {
public:
	TourModeDestinationParams(const ZoneMap& zoneMap, const CostMap& amCostsMap, const CostMap& pmCostsMap, const PersonParams& personParams, StopType tourType)
	: ModeDestinationParams(zoneMap, amCostsMap, pmCostsMap, tourType, personParams.getHomeLocation()),
	  drive1Available(personParams.hasDrivingLicence() * personParams.getCarOwn()), modeForParentWorkTour(0)
	{}

	virtual ~TourModeDestinationParams() {}

	double getCostPublicFirst(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return amCostsMap.at(origin).at(destination)->getPubCost();
	}

	double getCostPublicSecond(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return pmCostsMap.at(destination).at(origin)->getPubCost();
	}

	double getCostCarERPFirst(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return amCostsMap.at(origin).at(destination)->getCarCostErp();
	}

	double getCostCarERPSecond(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return pmCostsMap.at(destination).at(origin)->getCarCostErp();
	}

	double getCostCarOPFirst(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return (amCostsMap.at(origin).at(destination)->getDistance() * OPERATIONAL_COST);
	}

	double getCostCarOPSecond(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return (pmCostsMap.at(destination).at(origin)->getDistance() * OPERATIONAL_COST);
	}

	double getCostCarParking(int zoneId) const {
		return (8*zoneMap.at(zoneId)->getParkingRate());
	}

	double getWalkDistance1(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return amCostsMap.at(origin).at(destination)->getPubWalkt();
	}

	double getWalkDistance2(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return pmCostsMap.at(destination).at(origin)->getPubWalkt();
	}

	double getTT_PublicIvtFirst(int zoneId) {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return amCostsMap.at(origin).at(destination)->getPubIvt();
	}

	double getTT_PublicIvtSecond(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return pmCostsMap.at(destination).at(origin)->getPubIvt();
	}

	double getTT_CarIvtFirst(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return amCostsMap.at(origin).at(destination)->getCarIvt();
	}

	double getTT_CarIvtSecond(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return pmCostsMap.at(destination).at(origin)->getCarIvt();
	}

	double getTT_PublicOutFirst(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return amCostsMap.at(origin).at(destination)->getPubOut();
	}

	double getTT_PublicOutSecond(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return pmCostsMap.at(destination).at(origin)->getPubOut();
	}

	double getAvgTransferNumber(int zoneId) const {
		int destination = zoneMap.at(zoneId)->getZoneCode();
		if (origin == destination) { return 0; }
		return (amCostsMap.at(origin).at(destination)->getAvgTransfer() + pmCostsMap.at(destination).at(origin)->getAvgTransfer())/2;
	}

	int getCentralDummy(int zone) {
		return zoneMap.at(zone)->getCentralDummy();
	}

	StopType getTourPurpose() const {
		return purpose;
	}

	double getShop(int zone) {
		return zoneMap.at(zone)->getShop();
	}

	double getEmployment(int zone) {
		return zoneMap.at(zone)->getEmployment();
	}

	double getPopulation(int zone) {
		return zoneMap.at(zone)->getPopulation();
	}

	double getArea(int zone) {
		return zoneMap.at(zone)->getArea();
	}

	void setDrive1Available(bool drive1Available) {
		this->drive1Available = drive1Available;
	}

	int isAvailable_TMD(int choiceId) const {
		/* 1. if the destination == origin, the destination is not available.
		 * 2. public bus, private bus and MRT/LRT are only available if AM[(origin,destination)][’pub_ivt’]>0 and PM[(destination,origin)][’pub_ivt’]>0
		 * 3. shared2, shared3+, taxi and motorcycle are available to all.
		 * 4. Walk is only avaiable if (AM[(origin,destination)][’distance’]<=2 and PM[(destination,origin)][’distance’]<=2)
		 * 5. drive alone is available when for the agent, has_driving_license * one_plus_car == True
		 */
		int numZones = zoneMap.size();
		int numModes = 9;
		if (choiceId < 1 || choiceId > numModes*numZones) {
			throw std::runtime_error("isAvailable()::invalid choice id for mode-destination model");
		}

		int zoneId = choiceId % numZones;
		if(zoneId == 0) { // zoneId will become zero for the last zone
			zoneId = numZones;
		}
		int destination = zoneMap.at(zoneId)->getZoneCode();
		// the destination same as origin is not available
		if (origin == destination) {
			return 0;
		}
		// bus 1-1092; mrt 1093 - 2184; private bus 2185 - 3276; same result for the three modes
		if (choiceId <= 3 * numZones) {
			return (pmCostsMap.at(destination).at(origin)->getPubIvt() > 0
					&& amCostsMap.at(origin).at(destination)->getPubIvt() > 0);
		}
		// drive1 3277 - 4368
		if (choiceId <= 4 * numZones) {
			return drive1Available;
		}
		// share2 4369 - 5460
		if (choiceId <= 5 * numZones) {
			// share2 is available to all
			return 1;
		}
		// share3 5461 - 6552
		if (choiceId <= 6 * numZones) {
			// share3 is available to all
			return 1;
		}
		// motor 6553 - 7644
		if (choiceId <= 7 * numZones) {
			// share3 is available to all
			return 1;
		}
		// walk 7645 - 8736
		if (choiceId <= 8 * numZones) {
			return (amCostsMap.at(origin).at(destination)->getDistance() <= 2
					&& pmCostsMap.at(destination).at(origin)->getDistance() <= 2);
		}
		// taxi 8737 - 9828
		if (choiceId <= 9 * numZones) {
			// taxi is available to all
			return 1;
		}
		return 0;
	}

	int getModeForParentWorkTour() const
	{
		return modeForParentWorkTour;
	}

	void setModeForParentWorkTour(int modeForParentWorkTour)
	{
		this->modeForParentWorkTour = modeForParentWorkTour;
	}

private:
	bool drive1Available;
	/**mode for parent work tour in case of sub tours*/
	int modeForParentWorkTour;
};

class StopModeDestinationParams : public ModeDestinationParams {
public:
	StopModeDestinationParams(const ZoneMap& zoneMap, const CostMap& amCostsMap, const CostMap& pmCostsMap,
			const PersonParams& personParams, const Stop* stop, int originCode, const std::map<int, std::vector<int> >& unavailableODs)
	: ModeDestinationParams(zoneMap, amCostsMap, pmCostsMap, stop->getStopType(), originCode), homeZone(personParams.getHomeLocation()),
	  driveAvailable(personParams.hasDrivingLicence() * personParams.getCarOwn()), tourMode(stop->getParentTour().getTourMode()), firstBound(stop->isInFirstHalfTour()),
	  unavailableODs(unavailableODs)
	{}

	virtual ~StopModeDestinationParams() {}

	double getCostCarParking(int zone) {
		return 0; // parking cost is always 0 for intermediate stops
	}

	double getCostCarOP(int zone) {
		int destination = zoneMap.at(zone)->getZoneCode();
		if(origin != destination && destination != homeZone && origin != homeZone) {
			return ((amCostsMap.at(origin).at(destination)->getDistance() * OPERATIONAL_COST + pmCostsMap.at(origin).at(destination)->getDistance() * OPERATIONAL_COST)/2
					+(amCostsMap.at(destination).at(homeZone)->getDistance() * OPERATIONAL_COST + pmCostsMap.at(destination).at(homeZone)->getDistance() * OPERATIONAL_COST )/2
					-(amCostsMap.at(origin).at(homeZone)->getDistance() * OPERATIONAL_COST + pmCostsMap.at(origin).at(homeZone)->getDistance() * OPERATIONAL_COST)/2);
		}
		return 0;
	}

	double getCarCostERP(int zone) {
		int destination = zoneMap.at(zone)->getZoneCode();
		if(origin != destination && destination != homeZone && origin != homeZone) {
			return ((amCostsMap.at(origin).at(destination)->getCarCostErp() + pmCostsMap.at(origin).at(destination)->getCarCostErp())/2
					+(amCostsMap.at(destination).at(homeZone)->getCarCostErp() + pmCostsMap.at(destination).at(homeZone)->getCarCostErp())/2
					-(amCostsMap.at(origin).at(homeZone)->getCarCostErp() + pmCostsMap.at(origin).at(homeZone)->getCarCostErp())/2);
		}
		return 0;
	}

	double getCostPublic(int zone) {
		int destination = zoneMap.at(zone)->getZoneCode();
		if(origin != destination && destination != homeZone && origin != homeZone) {
			return ((amCostsMap.at(origin).at(destination)->getPubCost() + pmCostsMap.at(origin).at(destination)->getPubCost())/2
					+(amCostsMap.at(destination).at(homeZone)->getPubCost() + pmCostsMap.at(destination).at(homeZone)->getPubCost())/2
					-(amCostsMap.at(origin).at(homeZone)->getPubCost() + pmCostsMap.at(origin).at(homeZone)->getPubCost())/2);
		}
		return 0;
	}

	double getTT_CarIvt(int zone) {
		int destination = zoneMap.at(zone)->getZoneCode();
		if(origin != destination && destination != homeZone && origin != homeZone) {
			return ((amCostsMap.at(origin).at(destination)->getCarIvt() + pmCostsMap.at(origin).at(destination)->getCarIvt())/2
					+(amCostsMap.at(destination).at(homeZone)->getCarIvt() + pmCostsMap.at(destination).at(homeZone)->getCarIvt())/2
					-(amCostsMap.at(origin).at(homeZone)->getCarIvt() + pmCostsMap.at(origin).at(homeZone)->getCarIvt())/2);
		}
		return 0;
	}

	double getTT_PubIvt(int zone) {
		int destination = zoneMap.at(zone)->getZoneCode();
		if(origin != destination && destination != homeZone && origin != homeZone) {
			return ((amCostsMap.at(origin).at(destination)->getPubIvt() + pmCostsMap.at(origin).at(destination)->getPubIvt())/2
					+(amCostsMap.at(destination).at(homeZone)->getPubIvt() + pmCostsMap.at(destination).at(homeZone)->getPubIvt())/2
					-(amCostsMap.at(origin).at(homeZone)->getPubIvt() + pmCostsMap.at(origin).at(homeZone)->getPubIvt())/2);
		}
		return 0;
	}

	double getTT_PubOut(int zone) {
		int destination = zoneMap.at(zone)->getZoneCode();
		if(origin != destination && destination != homeZone && origin != homeZone) {
			return ((amCostsMap.at(origin).at(destination)->getPubOut() + pmCostsMap.at(origin).at(destination)->getPubOut())/2
					+(amCostsMap.at(destination).at(homeZone)->getPubOut() + pmCostsMap.at(destination).at(homeZone)->getPubOut())/2
					-(amCostsMap.at(origin).at(homeZone)->getPubOut() + pmCostsMap.at(origin).at(homeZone)->getPubOut())/2);
		}
		return 0;
	}

	double getWalkDistanceFirst(int zone) {
		int destination = zoneMap.at(zone)->getZoneCode();
		if(origin != destination && destination != homeZone && origin != homeZone) {
			return (amCostsMap.at(origin).at(destination)->getDistance()
					+ amCostsMap.at(destination).at(homeZone)->getDistance()
					- amCostsMap.at(origin).at(homeZone)->getDistance());
		}
		return 0;
	}

	double getWalkDistanceSecond(int zone) {
		int destination = zoneMap.at(zone)->getZoneCode();
		if(origin != destination && destination != homeZone && origin != homeZone) {
			return (pmCostsMap.at(origin).at(destination)->getDistance()
					+ pmCostsMap.at(destination).at(homeZone)->getDistance()
					- pmCostsMap.at(origin).at(homeZone)->getDistance());
		}
		return 0;
	}

	int getCentralDummy(int zone) {
		return zoneMap.at(zone)->getCentralDummy();
	}

	StopType getTourPurpose() const {
		return purpose;
	}

	double getShop(int zone) {
		return zoneMap.at(zone)->getShop();
	}

	double getEmployment(int zone) {
		return zoneMap.at(zone)->getEmployment();
	}

	double getPopulation(int zone) {
		return zoneMap.at(zone)->getPopulation();
	}

	double getArea(int zone) {
		return zoneMap.at(zone)->getArea();
	}

	int isAvailable_IMD(int choiceId) {
		/* 1. if the destination == origin, the destination is not available.
		 * 2. public bus, private bus and MRT/LRT are only available if AM[(origin,destination)][’pub_ivt’]>0 and PM[(destination,origin)][’pub_ivt’]>0
		 * 3. shared2, shared3+, taxi and motorcycle are available to all.
		 * 4. Walk is only avaiable if (AM[(origin,destination)][’distance’]<=2 and PM[(destination,origin)][’distance’]<=2)
		 * 5. drive alone is available when for the agent, has_driving_license * one_plus_car == True
		 */
		if (choiceId < 1 || choiceId > 9828) {
			throw std::runtime_error("isAvailable()::invalid choice id for mode-destination model");
		}
		int numZones = zoneMap.size();
		int zoneId = choiceId % numZones;
		if(zoneId == 0) { zoneId = numZones; } // zoneId will become zero for the last zone
		int destination = zoneMap.at(zoneId)->getZoneCode();

		if (origin == destination) { return 0; } // the destination same as origin is not available
		UnavailableODs::const_iterator unavailableODIt = unavailableODs.find(origin);
		// check if destination is unavailable due to lack of travel cost data
		if(unavailableODIt!=unavailableODs.end() && std::binary_search(unavailableODIt->second.begin(), unavailableODIt->second.end(), destination)) { return 0; } // destination is unavailable due to lack of cost data

		// bus 1-1092; mrt 1093 - 2184; private bus 2185 - 3276; same result for the three modes
		if (choiceId <= 3 * numZones) {
			bool avail = (pmCostsMap.at(destination).at(origin)->getPubIvt() > 0
					&& amCostsMap.at(origin).at(destination)->getPubIvt() > 0);
			switch(tourMode) {
			case 1:
			case 2:
			case 3: return avail;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9: return 0;
			}
		}

		// drive1 3277 - 4368
		if (choiceId <= 4 * numZones) {
			switch(tourMode) {
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6: return driveAvailable;
			case 7:
			case 8:
			case 9: return 0;
			}
		}
		// share2 4369 - 5460
		if (choiceId <= 6 * numZones) {
			// share2 is available to all
			switch(tourMode) {
			case 1:
			case 2:
			case 3:
			case 5:
			case 6: return 1;
			case 4:
			case 7:
			case 8:
			case 9: return 0;
			}
		}

		// motor 6553 - 7644
		if (choiceId <= 7 * numZones) {
			// share3 is available to all
			switch(tourMode) {
			case 1:
			case 2:
			case 3:
			case 5:
			case 6:
			case 4:
			case 7:
			case 9: return 1;
			case 8: return 0;
			}
		}
		// walk 7645 - 8736
		if (choiceId <= 8 * numZones) {
			return (amCostsMap.at(origin).at(destination)->getDistance() <= MAX_WALKING_DISTANCE
					&& pmCostsMap.at(destination).at(origin)->getDistance() <= MAX_WALKING_DISTANCE);
		}
		// taxi 8737 - 9828
		if (choiceId <= 9 * numZones) {
			// taxi is available to all
			switch(tourMode) {
			case 1:
			case 2:
			case 3:
			case 5:
			case 6:
			case 4:
			case 9: return 1;
			case 7:
			case 8: return 0;
			}
		}
		return 0;
	}

	int isFirstBound() const
	{
		return firstBound;
	}

	int isSecondBound() const
	{
		return !firstBound;
	}

protected:
	typedef std::map<int, std::vector<int> > UnavailableODs;
	int homeZone;
	int driveAvailable;
	int tourMode;
	bool firstBound;
	const UnavailableODs& unavailableODs;
};

} // end namespace medium
} // end namespace sim_mob
