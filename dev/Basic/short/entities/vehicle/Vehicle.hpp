//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * \file Vehicle.hpp
 *
 * \author Li Zhemin
 * \author Seth N. Hetu
 * \author Zhang Shuai
 * \author Xu Yan
 */

#pragma once

#include <stdexcept>
#include <sstream>
#include <vector>

#include "conf/settings/DisableMPI.h"
#include "entities/vehicle/VehicleBase.hpp"
#include "entities/models/Constants.h"
#include "geospatial/Lane.hpp"
#include "entities/roles/driver/GeneralPathMover.hpp"
#include "geospatial/streetdir/WayPoint.hpp"
#include "util/MovementVector.hpp"
#include "util/DynamicVector.hpp"

namespace sim_mob {

class PackageUtils;
class UnPackageUtils;
class FMODSchedule;

/**
 * The Vehicle class has vehicle Id, position, forward velocity, lat velocity and acceleration parameters etc for Driver use
 * Each Driver object has a vehicle to move in the network
 **/
class Vehicle : public sim_mob::VehicleBase {
public:
	Vehicle(const VehicleType vehType, double lengthCM, double widthCM); //TODO: now that the constructor is non-default, we might be able to remove throw_if_error()
	Vehicle(const VehicleType vehType, int vehicleId, double lengthCM, double widthCM); //Test
	Vehicle(const Vehicle& copy); ///<Copy constructor

	//Enable polymorphism
	virtual ~Vehicle(){}

	double getLateralMovement() const;         ///<Retrieve a value representing how far to the LEFT of the current lane the vehicle has moved.
	double getVelocity() const;      ///<Retrieve forward velocity.
	double getLatVelocity() const;   ///<Retrieve lateral velocity.
	double getAcceleration() const;  ///<Retrieve forward acceleration.
	bool isDone() const; ///<Are we fully done with our path?
	bool hasPath() const; ///<Do we have a path to move on?

	void resetPath(std::vector<sim_mob::WayPoint> wp_path);

	//Special
	LANE_CHANGE_SIDE getTurningDirection() const;

	//More stuff; some might be optional.
	const sim_mob::RoadSegment* getCurrSegment() const;
	const sim_mob::RoadSegment* getNextSegment(bool inSameLink=true) const;
	const sim_mob::RoadSegment* getSecondSegmentAhead();
	const sim_mob::RoadSegment* getPrevSegment(bool inSameLink=true) const;
	const sim_mob::RoadSegment* hasNextSegment(bool inSameLink) const;

	std::vector<const sim_mob::RoadSegment*>::iterator getPathIterator();
	std::vector<const sim_mob::RoadSegment*>::iterator getPathIteratorEnd();

	const sim_mob::Lane* getCurrLane() const;
	void setPositionInIntersection(double x, double y);
	const DPoint& getPositionInIntersection();
	void setTurningDirection(LANE_CHANGE_SIDE direction);
	//Modifiers
	void setVelocity(double value);      ///<Set the forward velocity.
	void setLatVelocity(double value);   ///<Set the lateral velocity.
	void setAcceleration(double value);  ///<Set the forward acceleration.
	// for path-mover splitting purpose
	void setCurrPosition(DPoint currPosition);
	const DPoint& getCurrPosition() const;

	void actualMoveToNextSegmentAndUpdateDir_med();		//~melani for mid-term
	void moveLat(double amt);            ///<Move this car laterally. NOTE: This will _add_ the amt to the current value.
	void resetLateralMovement();         ///<Put this car back in the center of the current lane.

	/*needed by mid-term*/
	double getPositionInSegmentCM();
	void setPositionInSegmentCM(double newDistToEndCM);
	//unit cm, this is based on lane zero's polypoints

#ifndef SIMMOB_DISABLE_MPI
public:
	///Serialization
	static void pack(PackageUtils& package, Vehicle* one_vehicle);

	static Vehicle* unpack(UnPackageUtils& unpackage);
#endif

public:
//	const double lengthCM;  ///<length(CM) of the vehicle
//	const double widthCM;   ///<width(CM) of the vehicle
	bool isQueuing; 	 ///<for mid-term use
	FMODSchedule* schedule;
	double stoppedtimecounter;

public:
	//Trying a slightly more dynamic moving model.
	int vehicleId;
	GeneralPathMover fwdMovement;
	double latMovement; // latMovement not equal to lateral position
	double fwdVelocity;
	double latVelocity;
	double fwdAccel;
	LANE_CHANGE_SIDE turningDirection;
	//Override for when we're in an intersection.
	DPoint posInIntersection;
	// driver path-mover split purpose, we save the currPos in the Vehicle
	DPoint currPos;

	//NOTE: The error state is a temporary sanity check to help me debug this class. There are certainly
	//      better ways to handle this (e.g., non-default constructor).
	bool errorState;
	void throw_if_error() const {
		if (errorState) {
			throw std::runtime_error("Error: can't perform certain actions on an uninitialized vehicle.");
		}
	}

	//Serialization-related friends
	friend class PackageUtils;
	friend class UnPackageUtils;
};

} // namespace sim_mob
