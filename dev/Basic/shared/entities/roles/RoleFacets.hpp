/* Copyright Singapore-MIT Alliance for Research and Technology */

/*
 * RoleFacet.h
 *
 *  Created on: Mar 21, 2013
 *      Author: harish
 */

#pragma once

#include "conf/settings/DisableMPI.h"
#include "util/LangHelpers.hpp"
#include "entities/Agent.hpp"
#include "entities/vehicle/Vehicle.hpp"
#include "entities/UpdateParams.hpp"
#include "entities/misc/BusTrip.hpp"
#include "boost/thread/thread.hpp"
#include "boost/thread/locks.hpp"
#include "util/OutputUtil.hpp"
#include <boost/random.hpp>

namespace sim_mob {

#ifndef SIMMOB_DISABLE_MPI
class PartitionManager;
class PackageUtils;
class UnPackageUtils;
#endif

/* The BehaviorFacet and MovementFacet abstract base classes are pretty much identical. They are kept separate for semantic reasons.
 * The Role class will just serve as a container for these two classes. Each subclass of role (Driver, Pedestrian, Passenger, ActivityRole etc.)
 * must contain pointers/references to respective specializations of BehaviorFacet and MovementFacet classes.
 */

class BehaviorFacet {

public:
	//NOTE: Don't forget to call this from sub-classes!
	explicit BehaviorFacet(sim_mob::Agent* parentAgent = nullptr, std::string roleName = std::string()) :
	parentAgent(parentAgent), name(roleName), dynamic_seed(0) { }

	//Allow propagating destructors
	virtual ~BehaviorFacet() {}

	std::string getRoleName()const {return name;}

	///Called the first time an Agent's update() method is successfully called.
	/// This will be the tick of its startTime, rounded down(?).
	virtual void frame_init(UpdateParams& p) = 0;

	///Perform each frame's update tick for this Agent.
	virtual void frame_tick(UpdateParams& p) = 0;

	///Generate output for this frame's tick for this Agent.
	virtual void frame_tick_output(const UpdateParams& p) = 0;

	//generate output with fake attributes for MPI
	virtual void frame_tick_output_mpi(timeslice now) = 0;

	///Create the UpdateParams (or, more likely, sub-class) which will hold all
	///  the temporary information for this time tick.
	virtual UpdateParams& make_frame_tick_params(timeslice now) = 0;


	/* NOTE: There is no resource defined in the base class BehaviorFacet. For role facets of drivers, the vehicle of the parent Role could be
	 * shared between behavior and movement facets. This getter must be overridden in the derived classes to return appropriate resource.
	 */
	virtual Vehicle* getResource() { return nullptr; }

	Agent* getParent()
	{
		return parentAgent;
	}

	void setParent(Agent* parent)
	{
		this->parentAgent = parent;
	}

	int getOwnRandomNumber(boost::mt19937& gen)
	{
		int one_try = -1;
		int second_try = -2;
		int third_try = -3;
		//		int forth_try = -4;

		while (one_try != second_try || third_try != second_try)
		{
			//TODO: I've replaced your calls to srand() and rand() (which are not
			//      thread-safe) with boost::random.
			//      This is likely to not work the way you want it to.
			//      Please read the boost::random docs. ~Seth
			boost::uniform_int<> dist(0, RAND_MAX);

			one_try = dist(gen);

			second_try = dist(gen);

			third_try = dist(gen);

//			if (one_try != second_try || third_try != second_try)
//			{
//				LogOut("Random:" << this->getParent()->getId() << "," << one_try << "," << second_try << "," << third_try << "\n");
//			}
//			else
//			{
//				LogOut("Random:" << this->getParent()->getId() << ",Use Seed:" << dynamic_seed << ", Get:" << one_try << "," << second_try<< "," << third_try<< "\n");
//			}
		}

		dynamic_seed = one_try;
		return one_try;
	}

	const std::string name;
protected:
	Agent* parentAgent; ///<The owner of this role. Usually a Person, but I could see it possibly being another Agent.

	//add by xuyan
protected:
	int dynamic_seed;

	//Random number generator
	//TODO: We need a policy on who can get a generator and why.
	//boost::mt19937 gen;

public:
#ifndef SIMMOB_DISABLE_MPI
	friend class sim_mob::PartitionManager;
#endif

	//Serialization
#ifndef SIMMOB_DISABLE_MPI
public:
	virtual void pack(PackageUtils& packageUtil) = 0;
	virtual void unpack(UnPackageUtils& unpackageUtil) = 0;

	virtual void packProxy(PackageUtils& packageUtil) = 0;
	virtual void unpackProxy(UnPackageUtils& unpackageUtil) = 0;
#endif
};

class MovementFacet {

public:
	//NOTE: Don't forget to call this from sub-classes!
	explicit MovementFacet(sim_mob::Agent* parentAgent = nullptr, std::string roleName = std::string()) :
		parentAgent(parentAgent), name(roleName), dynamic_seed(0) { }

	//Allow propagating destructors
	virtual ~MovementFacet() {}

	std::string getRoleName()const {return name;}

	///Called the first time an Agent's update() method is successfully called.
	/// This will be the tick of its startTime, rounded down(?).
	virtual void frame_init(UpdateParams& p) = 0;

	///Perform each frame's update tick for this Agent.
	virtual void frame_tick(UpdateParams& p) = 0;

	///Generate output for this frame's tick for this Agent.
	virtual void frame_tick_output(const UpdateParams& p) = 0;

	//generate output with fake attributes for MPI
	virtual void frame_tick_output_mpi(timeslice now) = 0;

	///Create the UpdateParams (or, more likely, sub-class) which will hold all
	///  the temporary information for this time tick.
	virtual UpdateParams& make_frame_tick_params(timeslice now) = 0;

	/* NOTE: There is no resource defined in the base class BehaviorFacet. This is kept virtual to be consistent with the Role.
	 * For role facets of drivers, the vehicle of the parent Role could be shared between behavior and movement facets.
	 * This getter must be overridden in the derived classes to return appropriate resource.
	 */
	virtual Vehicle* getResource() { return nullptr; }

	Agent* getParent()
	{
		return parentAgent;
	}

	void setParent(Agent* parent)
	{
		this->parentAgent = parent;
	}

	int getOwnRandomNumber(boost::mt19937& gen)
	{
		int one_try = -1;
		int second_try = -2;
		int third_try = -3;
		//		int forth_try = -4;

		while (one_try != second_try || third_try != second_try)
		{
			//TODO: I've replaced your calls to srand() and rand() (which are not
			//      thread-safe) with boost::random.
			//      This is likely to not work the way you want it to.
			//      Please read the boost::random docs. ~Seth
			boost::uniform_int<> dist(0, RAND_MAX);

			one_try = dist(gen);

			second_try = dist(gen);

			third_try = dist(gen);

//			if (one_try != second_try || third_try != second_try)
//			{
//				LogOut("Random:" << this->getParent()->getId() << "," << one_try << "," << second_try << "," << third_try << "\n");
//			}
//			else
//			{
//				LogOut("Random:" << this->getParent()->getId() << ",Use Seed:" << dynamic_seed << ", Get:" << one_try << "," << second_try<< "," << third_try<< "\n");
//			}
		}

		dynamic_seed = one_try;
		return one_try;
	}

	const std::string name;
protected:
	Agent* parentAgent; ///<The owner of this role. Usually a Person, but I could see it possibly being another Agent.

	//added by xuyan in Role.hpp. Copied here by Harish.
protected:
	int dynamic_seed;

	//Random number generator
	//TODO: We need a policy on who can get a generator and why.
	//boost::mt19937 gen;

public:
#ifndef SIMMOB_DISABLE_MPI
	friend class sim_mob::PartitionManager;
#endif

	//Serialization
#ifndef SIMMOB_DISABLE_MPI
public:
	virtual void pack(PackageUtils& packageUtil) = 0;
	virtual void unpack(UnPackageUtils& unpackageUtil) = 0;

	virtual void packProxy(PackageUtils& packageUtil) = 0;
	virtual void unpackProxy(UnPackageUtils& unpackageUtil) = 0;
#endif
};

} /* namespace sim_mob */
