//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <string>

#include "soci.h"
#include "soci-postgresql.h"

#include "entities/misc/TripChain.hpp"

namespace sim_mob
{
class Entity;
class Agent;
class Person;
class StartTimePriorityQueue;
//FDs for RestrictedRegion
class Node;
class TripChainItem;
class WayPoint;

class RestrictedRegion : private boost::noncopyable
{
	/**
	 * Nodes in the restricted area
	 * key: restricted Node
	 * value: peripheryNode(non-restricted node nearest to the restricted node).
	 * map<restricted Node,peripheryNode>
	 */

	std::map<const Node*,const Node*> restrictedZoneBorder;
	/**
	 * does the given node(or nde wrapped in a WayPoint) lie in the restricted area,
	 * returns the periphery node if the target is in the restricted zone
	 * returns null if Node not found in the restricted region.
	 */
	 const Node* isInRestrictedZone(const Node* target) const;
	 const Node* isInRestrictedZone(const WayPoint& target) const;
	/**
	 * Function to split the subtrips crossing the restricted Areas
	 */
	void processSubTrips(std::vector<sim_mob::SubTrip>& subTrips);
	sim_mob::OneTimeFlag populated;
public:
	static boost::shared_ptr<RestrictedRegion> instance;
	/**
	 * returns the singletone instance
	 */
	static RestrictedRegion & getInstance()
	{
		if(!instance)
		{
			instance.reset(new RestrictedRegion());
		}
		populate();
		return *instance;
	}
	/**
	 * fill in the input data-restrictedZoneBorder
	 */
	void populate();
	/**
	 * modify trips whose orgigin and/or destination lies in a restricted area
	 */
	void processTripChains(std::map<std::string, std::vector<TripChainItem*> > &tripchains);

};

class PeriodicPersonLoader :  private boost::noncopyable
{
private:
	/** our active agents list*/
	std::set<sim_mob::Entity*>& activeAgents;

	/** out pending agents list*/
	StartTimePriorityQueue& pendingAgents;

	/** data load interval in seconds*/
	unsigned dataLoadInterval;

	/** start time of next load interval*/
	double nextLoadStart;

	/** database session */
	soci::session sql_;

	/** stored procedure to periodically load data*/
	std::string storedProcName;

	/** time elapsed since previous load in seconds*/
	unsigned elapsedTimeSinceLastLoad;

	/**
	 * adds person to active or pending agents list depending on start time
	 */
	void addOrStashPerson(Person* p);
public:
	PeriodicPersonLoader(std::set<sim_mob::Entity*>& activeAgents, StartTimePriorityQueue& pendinAgents);
	virtual ~PeriodicPersonLoader();

	unsigned getDataLoadInterval() const
	{
		return dataLoadInterval;
	}

	unsigned getNextLoadStart() const
	{
		return nextLoadStart;
	}

	const std::string& getStoredProcName() const
	{
		return storedProcName;
	}

	/**
	 * load activity schedules for next interval
	 */
	void loadActivitySchedules();

	/**
	 * tracks the number of ticks elapsed since last load
	 * this function *must* be called in every tick
	 * @return true if data has to be loaded in this tick; false otherwise
	 */
	bool checkTimeForNextLoad();
};
}






