//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * \file Bus.hpp
 *
 * \author Seth N. Hetu
 */

#pragma once

#include "Vehicle.hpp"
#include "BusRoute.hpp"

namespace sim_mob {

class Person;
class BusController;
class PackageUtils;
class UnPackageUtils;

/**
 * A bus is similar to a Vehicle, except that it follows a BusRoute and has a passenger count.
 */
class Bus : public sim_mob::Vehicle {
public:
	Bus(const BusRoute& route, const Vehicle* clone, std::string busLine_)
	: Vehicle(*clone), passengerCount(0), passengerCount_Old(0),busline(busLine_),
	  route(route), ptCheck(0,0), DistThreshold(2000), busCapacity(40),
	  TimeOfBusreachingBusstop(0)
	{}

	int getPassengerCount() const { return passengerCount; }
	int getBusCapacity() const { return busCapacity; }
	void setPassengerCount(int val) { passengerCount = val; }
	void setBusNumber(int &n) { busNumber = n; }
	int getBusNumber() { return busNumber; }
	int getPassengerCountOld() const { return passengerCount_Old; }
	void setPassengerCountOld(int val) { passengerCount_Old = val; }
	std::vector<const sim_mob::Agent*> passengers_distribition;//added by Meenu
	std::string getBusLineID()
	{
		return busline;
	}

	double TimeOfBusreachingBusstop;
	std::vector<sim_mob::Person*> passengers_inside_bus;//added by Meenu
private:
	int passengerCount;
	int passengerCount_Old;// record for dwell time
	int busCapacity;
	BusRoute route;
	DPoint ptCheck;// Later connect to Stops, calculate the position to some stops
	double DistThreshold;
	std::string busline;

	int busNumber;

	friend class BusController;
	//Serialization-related friends
	friend class PackageUtils;
	friend class UnPackageUtils;
};

} // namespace sim_mob