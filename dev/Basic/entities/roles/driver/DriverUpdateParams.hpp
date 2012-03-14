/* Copyright Singapore-MIT Alliance for Research and Technology */

#pragma once

#include "GenConfig.h"
#include "entities/UpdateParams.hpp"
#include "util/DynamicVector.hpp"
#include <boost/random.hpp>
#include "util/LangHelpers.hpp"
#include "entities/Signal.hpp"

namespace sim_mob
{

//Forward declarations
class Lane;
class Driver;

#ifndef SIMMOB_DISABLE_MPI
class PackageUtils;
class UnPackageUtils;
#endif


struct LaneSide {
	bool left;
	bool right;
	bool both() const { return left && right; }
	bool leftOnly() const { return left && !right; }
	bool rightOnly() const { return right && !left; }
};

enum LANE_CHANGE_MODE {	//as a mask
	DLC = 0,
	MLC = 2
};

enum LANE_CHANGE_SIDE {
	LCS_LEFT = -1,
	LCS_SAME = 0,
	LCS_RIGHT = 1
};


//Struct for holding data about the "nearest" vehicle.
struct NearestVehicle {
	NearestVehicle() : driver(nullptr), distance(5000) {}

	//TODO: This is probably not needed. We should really set "distance" to DOUBLE_MAX.
	bool exists() const { return distance < 5000; }
	const Driver* driver;
	double distance;
};

//Similar, but for pedestrians
struct NearestPedestrian {
	NearestPedestrian() : distance(5000) {}

	//TODO: This is probably not needed. We should really set "distance" to DOUBLE_MAX.
	bool exists() { return distance < 5000; }

	double distance;
};


///Simple struct to hold parameters which only exist for a single update tick.
/// \author Wang Xinyuan
/// \author Li Zhemin
/// \author Seth N. Hetu
///NOTE: Constructor is currently implemented in Driver.cpp. Feel free to shuffle this around if you like.
struct DriverUpdateParams : public UpdateParams {
	explicit DriverUpdateParams(boost::mt19937& gen) : UpdateParams(gen) {}

	virtual void reset(frame_t frameNumber, unsigned int currTimeMS, const Driver& owner);

	const Lane* currLane;  //TODO: This should really be tied to PolyLineMover, but for now it's not important.
	size_t currLaneIndex; //Cache of currLane's index.
	size_t fromLaneIndex; //for lane changing model
	const Lane* leftLane;
	const Lane* rightLane;
	const Lane* leftLane2; //the second left lane
	const Lane* rightLane2;

	double currSpeed;

	double currLaneOffset;
	double currLaneLength;
	bool isTrafficLightStop;
	double trafficSignalStopDistance;
	double elapsedSeconds;

	double perceivedFwdVelocity;
	double perceivedLatVelocity;

	double perceivedFwdVelocityOfFwdCar;
	double perceivedLatVelocityOfFwdCar;
	double perceivedAccelerationOfFwdCar;
	double perceivedDistToFwdCar;

	bool perceivedTrafficSignal;
	Signal::TrafficColor perceivedTrafficColor;

	LANE_CHANGE_SIDE turningDirection;

	//Nearest vehicles in the current lane, and left/right (including fwd/back for each).
	//Nearest vehicles' distances are initialized to threshold values.
	NearestVehicle nvFwd;
	NearestVehicle nvBack;
	NearestVehicle nvLeftFwd;
	NearestVehicle nvLeftBack;
	NearestVehicle nvRightFwd;
	NearestVehicle nvRightBack;
	NearestVehicle nvLeftFwd2; //the second adjacent lane
	NearestVehicle nvLeftBack2;
	NearestVehicle nvRightFwd2;
	NearestVehicle nvRightBack2;



	NearestPedestrian npedFwd;

	double laneChangingVelocity;

	bool isCrossingAhead;
	bool isApproachingToIntersection;
	int crossingFwdDistance;

	//Related to our car following model.
	double space;
	double a_lead;
	double v_lead;
	double space_star;
	double distanceToNormalStop;

	//Related to our lane changing model.
	double dis2stop;
	bool isWaiting;

	//Handles state information
	bool justChangedToNewSegment;
	DPoint TEMP_lastKnownPolypoint;
	bool justMovedIntoIntersection;
	double overflowIntoIntersection;

public:
#ifndef SIMMOB_DISABLE_MPI
	static void pack(PackageUtils& package, const DriverUpdateParams* params);

	static void unpack(UnPackageUtils& unpackage, DriverUpdateParams* params);
#endif
};


}
