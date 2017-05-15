//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <vector>
#include <map>

#include "conf/settings/DisableMPI.h"
#include "entities/Person_MT.hpp"
#include "entities/roles/Role.hpp"
#include "entities/Vehicle.hpp"
#include "geospatial/network/WayPoint.hpp"
#include "util/DynamicVector.hpp"
#include "DriverUpdateParams.hpp"

#ifndef SIMMOB_DISABLE_MPI
class PackageUtils;
class UnPackageUtils;
#endif

namespace sim_mob
{

class Agent;
class Person;
class Link;
class RoadSegment;
class Lane;
class Node;
class MultiNode;
class Point;
class UpdateParams;

namespace medium
{

class DriverBehavior;
class DriverMovement;

/**
 * Medium-term Driver.
 * \author Seth N. Hetu
 * \author Melani Jayasuriya
 * \author Harish Loganathan
 */
enum DriverMode
{
	DRIVE_START =0,
	CRUISE,
	DRIVE_TO_TAXISTAND,
	DRIVE_WITH_PASSENGER,
	DRIVE_FOR_DRIVER_CHANGE_SHIFT,
	QUEUING_AT_TAXISTAND,
	DRIVE_FOR_BREAK,
	DRIVER_IN_BREAK,
	DRIVE_ON_CALL
};

class Driver : public sim_mob::Role<Person_MT>, public UpdateWrapper<DriverUpdateParams>
{
public:
	Driver(Person_MT* parent, 
		sim_mob::medium::DriverBehavior* behavior = nullptr,
		sim_mob::medium::DriverMovement* movement = nullptr,
		std::string roleName = std::string(),
		Role<Person_MT>::Type roleType = Role<Person_MT>::RL_DRIVER);
	virtual ~Driver();

	virtual Role<Person_MT>* clone(Person_MT *parent) const;

	//Virtual overrides
	virtual void make_frame_tick_params(timeslice now);
	virtual std::vector<sim_mob::BufferedBase*> getSubscriptionParams();
	virtual void HandleParentMessage(messaging::Message::MessageType type, const messaging::Message& message);


	DriverMode driverMode;
	const DriverMode & getDriveMode() const;
	void setDriveMode(const DriverMode &driveMode);
	//to be moved to a DriverUpdateParam later
	const Lane* currLane;

protected:
	WayPoint origin;
	WayPoint goal;

	friend class DriverBehavior;
	friend class DriverMovement;
	//remove this immediately after debug
	friend Conflux;
};


} // namespace medium
} // namespace sim_mob
