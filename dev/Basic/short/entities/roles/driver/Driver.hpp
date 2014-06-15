//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <vector>
#include <math.h>
#include <set>

#include "conf/settings/DisableMPI.h"

#include "entities/roles/Role.hpp"
#include "buffering/Shared.hpp"
#include "perception/FixedDelayed.hpp"
#include "entities/vehicle/Vehicle.hpp"
#include "util/DynamicVector.hpp"

#include "entities/models/CarFollowModel.hpp"
#include "entities/models/LaneChangeModel.hpp"
#include "entities/models/IntersectionDrivingModel.hpp"
#include "DriverUpdateParams.hpp"
#include "DriverFacets.hpp"
#include "util/Math.hpp"


namespace sim_mob
{

//Forward declarations
class Pedestrian;
class Signal;
class Link;
class RoadSegment;
class Lane;
class Node;
class MultiNode;
class DPoint;
class UpdateParams;
class DriverBehavior;
class DriverMovement;

#ifndef SIMMOB_DISABLE_MPI
class PackageUtils;
class UnPackageUtils;
#endif





/**
 * \author Wang Xinyuan
 * \author Li Zhemin
 * \author Runmin Xu
 * \author Seth N. Hetu
 * \author Luo Linbo
 * \author LIM Fung Chai
 * \author Zhang Shuai
 * \author Xu Yan
 */
class Driver : public sim_mob::Role , public UpdateWrapper<DriverUpdateParams>{
//Internal classes
private:
	//Helper class for grouping a Node and a Point2D together.
	class NodePoint {
	public:
		Point2D point;
		const Node* node;
		NodePoint() : point(0,0), node(nullptr) {}
	};


//Constructor and overridden methods.
public:
	const static int distanceInFront = 3000;
	const static int distanceBehind = 500;
	const static int maxVisibleDis = 5000;

	Driver(Person* parent, sim_mob::MutexStrategy mtxStrat, sim_mob::DriverBehavior* behavior = nullptr, sim_mob::DriverMovement* movement = nullptr, Role::type roleType_ = RL_DRIVER, std::string roleName_ = "driver");
	void initReactionTime();
	virtual ~Driver();

	virtual sim_mob::Role* clone(sim_mob::Person* parent) const;

	//Virtual implementations
	virtual void make_frame_tick_params(timeslice now);
	virtual std::vector<sim_mob::BufferedBase*> getSubscriptionParams();
	virtual std::vector<sim_mob::BufferedBase*> getDriverInternalParams();
	void handleUpdateRequest(MovementFacet* mFacet);
	bool isBus();
//Buffered data
public:
	Shared<const Lane*> currLane_;
	Shared<double> currLaneOffset_;
	Shared<double> currLaneLength_;
	Shared<bool> isInIntersection;

	//need to store these values in the double buffer, because it is needed by other drivers.
	Shared<double> latMovement;
	const double getFwdVelocityM();
	/*
	 *  /brief Find the distance from front vehicle.
	 *        CAUTION: TS_Vehicles "front" and this vehicle may not be in the same
	 * lane (could be in the left or right neighbor lane), but they have
	 * to be in either the same segment or in a downstream NEIGHBOR
	 * segment.
	 */
	double gapDistance(const Driver* front);
	Shared<double> fwdVelocity;
	Shared<double> latVelocity;
	Shared<double> fwdAccel;
	Shared<LANE_CHANGE_SIDE> turningDirection;

	//for fmod request
	Shared<std::string> stop_event_time;
	Shared<int> stop_event_type;
	Shared<int> stop_event_scheduleid;
	Shared<int> stop_event_nodeid;
	Shared< std::vector<int> > stop_event_lastBoardingPassengers;
	Shared< std::vector<int> > stop_event_lastAlightingPassengers;

	Vehicle* getVehicle() { return vehicle; }

	///Reroute around a blacklisted set of RoadSegments. See Role's comments for more information.
	virtual void rerouteWithBlacklist(const std::vector<const sim_mob::RoadSegment*>& blacklisted);
	// for path-mover splitting purpose
	void setCurrPosition(DPoint& currPosition);
	const DPoint& getCurrPosition() const;

public:
	double startTime;
	bool isAleadyStarted;
	double currDistAlongRoadSegment;

	// me is doing yielding, and yieldVehicle is doing nosing
	Driver* yieldVehicle;

//Basic data
public:
	//Pointer to the vehicle this driver is controlling.
	Vehicle* vehicle;
	// driver path-mover split purpose, we save the currPos in the Driver
	DPoint currPos;
	//This should be done through the Role class itself; for now, I'm just forcing
	//  it so that we can get the mid-term working. ~Seth
	virtual Vehicle* getResource() { return vehicle; }

private:
//	//Sample stored data which takes reaction time into account.
	size_t reacTime;
	FixedDelayed<double> *perceivedFwdVel;
	FixedDelayed<double> *perceivedFwdAcc;
	FixedDelayed<double> *perceivedVelOfFwdCar;
	FixedDelayed<double> *perceivedAccOfFwdCar;
	FixedDelayed<double> *perceivedDistToFwdCar;
	FixedDelayed<sim_mob::TrafficColor> *perceivedTrafficColor;
	FixedDelayed<double> *perceivedDistToTrafficSignal;

	NodePoint origin;
	NodePoint goal;    //first, assume that each vehicle moves towards a goal

public:
	/**
	 * /brief reset reaction time
	 * /param t time in ms
	 */
	void resetReacTime(double t);

public:
	Agent* getDriverParent(const Driver *self) { return self->parent; }

public:
	//TODO: This may be risky, as it exposes non-buffered properties to other vehicles.
	const Vehicle* getVehicle() const { return vehicle; }

	//This is probably ok.
	const double getVehicleLengthCM() const { return vehicle->getLengthCm(); }
	const double getVehicleLengthM() const { return getVehicleLengthCM()/100.0; }

private:
	friend class DriverBehavior;
	friend class DriverMovement;

	//Serialization
#ifndef SIMMOB_DISABLE_MPI
public:
	virtual void pack(PackageUtils& packageUtil);
	virtual void unpack(UnPackageUtils& unpackageUtil);

	virtual void packProxy(PackageUtils& packageUtil);
	virtual void unpackProxy(UnPackageUtils& unpackageUtil);
#endif

};



}
