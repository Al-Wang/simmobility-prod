//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * PassengerFacets.hpp
 *
 *  Created on: June 17th, 2013
 *      Author: Yao Jin
 */

#pragma once
#include "conf/settings/DisableMPI.h"
#include "entities/roles/RoleFacets.hpp"
#include "Passenger.hpp"

namespace sim_mob
{

class Passenger;
class Bus;
class BusDriver;

class PassengerBehavior : public sim_mob::BehaviorFacet
{
public:
	explicit PassengerBehavior();
	virtual ~PassengerBehavior();

	//Virtual overrides
	virtual void frame_init();
	virtual void frame_tick();
	virtual std::string frame_tick_output();

	Passenger* getParentPassenger() const
	{
		return parentPassenger;
	}

	void setParentPassenger(Passenger* parentPassenger)
	{
		this->parentPassenger = parentPassenger;
	}

private:
	Passenger* parentPassenger;

};

class PassengerMovement : public sim_mob::MovementFacet
{
public:
	explicit PassengerMovement();
	virtual ~PassengerMovement();

	//Virtual overrides
	void setParentBufferedData();
	virtual void frame_init();
	virtual void frame_tick();
	virtual std::string frame_tick_output();

	// mark startTimeand origin

	virtual TravelMetric & startTravelTimeMetric()
	{
	}

	//	mark the destination and end time and travel time

	virtual TravelMetric & finalizeTravelTimeMetric()
	{
	}

	bool isAtBusStop();
	bool isBusBoarded();
	bool isDestBusStopReached();
	Point getXYPosition();
	Point getDestPosition();

	const BusStop* getOriginBusStop()
	{
		return originBusStop;
	}

	const BusStop* getDestBusStop()
	{
		return destBusStop;
	}

	///NOTE: These boarding/alighting functions are called from BusDriver and used to transfer data.

	///passenger boards the approaching bus if it goes to the destination
	///.Decision to board is made when the bus approaches the busstop.So the first
	///bus which would take to the destination would be boarded
	bool PassengerBoardBus_Normal(BusDriver* busdriver, std::vector<const BusStop*> busStops);

	bool PassengerAlightBus(Driver* busdriver);

	///passenger has initially chosen which bus lines to board and passenger boards
	///the bus based on this pre-decision.Passenger makes the decision to board a bussline
	///based on the path of the bus if the bus goes to the destination and chooses the busline based on shortest distance
	bool PassengerBoardBus_Choice(Driver* busdriver);

	///to find waiting time for passengers who have boarded bus,time difference between
	/// time of reaching busstop and time bus reaches busstop
	void findWaitingTime(Bus* bus);

	///finds the nearest busstop for the given node,As passenger origin and destination is given in terms of nodes
	BusStop* setBusStopXY(const Node* node);

	///finds which bus lines the passenger should take after reaching the busstop
	///based on bussline info at the busstop
	void FindBusLines();

	std::vector<BusLine*> ReturnBusLines();

	//bool isOnCrossing() const;

	Passenger* getParentPassenger() const
	{
		return parentPassenger;
	}

	void setParentPassenger(Passenger* parentPassenger)
	{
		this->parentPassenger = parentPassenger;
	}

	const int getBusTripRunNum() const
	{
		return busTripRunNum;
	}

	const std::string& getBuslineId() const
	{
		return buslineId;
	}

public:
	// to record the alightingMS for each individual person
	uint32_t alightingMS;
	int busTripRunNum; // busTripRunNum to record the bus trip run num
	std::string buslineId; // busline_id to record the bus line id

private:
	Passenger* parentPassenger;
	BusStop* originBusStop; ///busstop passenger is starting the trip from
	BusStop* destBusStop; ///busstop passenger is ending the trip

	std::vector<Busline*> buslinesToTake; ///buslines passenger can take;decided by passenger upon reaching busstop

	double waitingTime;
	double timeOfReachingBusStop;
	uint32_t timeOfStartTrip;
	uint32_t travelTime;
	///For display purposes: offset this Passenger by a given +x, +y
	Point displayOffset;

	///for display purpose of alighting passengers
	int displayX;
	int displayY;

	///to display alighted passenger for certain no of frame ticks before removal
	int skip;
};
}
