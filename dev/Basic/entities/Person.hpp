/* Copyright Singapore-MIT Alliance for Research and Technology */

#pragma once

#include <vector>

#include "Agent.hpp"
#include "roles/Role.hpp"
#include "roles/driver/Driver.hpp"
#include "buffering/Shared.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#endif

namespace sim_mob
{

class TripChain;

#ifndef SIMMOB_DISABLE_MPI
class PartitionManager;
#endif

/**
 * Basic Person class. A person may perform one of several roles which
 *  change over time. For example: Drivers, Pedestrians, and Passengers are
 *  all roles which a Person may fulfill.
 */
class Person : public sim_mob::Agent {
public:
	Person(const MutexStrategy& mtxStrat, int id=-1);
	virtual ~Person();

	///Update Person behavior
	virtual bool update(frame_t frameNumber);

	virtual void output(frame_t frameNumber);

	///Update a Person's subscription list.
	virtual void buildSubscriptionList();

	///Change the role of this person: Driver, Passenger, Pedestrian
	void changeRole(sim_mob::Role* newRole);
	sim_mob::Role* getRole() const;

	///Set this person's trip chain
	void setTripChain(sim_mob::TripChain* newTripChain) { currTripChain = newTripChain; }
	sim_mob::TripChain* getTripChain() { return currTripChain; }

	//Used for passing various debug data. Do not rely on this for anything long-term.
	std::string specialStr;

private:
	//Properties
	sim_mob::Role* currRole;
	sim_mob::TripChain* currTripChain;

	//add by xuyan
#ifndef SIMMOB_DISABLE_MPI
public:
	friend class PartitionManager;

public:
	virtual void package(PackageUtils& packageUtil);
	virtual void unpackage(UnPackageUtils& unpackageUtil);

	virtual void packageProxy(PackageUtils& packageUtil);
	virtual void unpackageProxy(UnPackageUtils& unpackageUtil);

#endif

};





}
