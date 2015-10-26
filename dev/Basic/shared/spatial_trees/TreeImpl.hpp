//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <set>

#include "metrics/Length.hpp"

namespace sim_mob {

//Forward declarations
class Entity;
class Agent;
class Lane;
class Point;

struct TreeItem;

/**
 * Parent (abstract) class for new tree functionality.
 */
class TreeImpl {
public:
	virtual ~TreeImpl() {}

	///Perform any necessary initialization required by this Tree. Called once, after construction. Optional.
	virtual void init() {}

	///Register a new Agent, so that the spatial index is aware of this person. Optional.
	virtual void registerNewAgent(const Agent* ag) {}

	///Update the structure.
	//Note: The pointers in removedAgentPointers will be deleted after this time tick; do *not*
	//      save them anywhere.
	virtual void update(int time_step, const std::set<sim_mob::Entity*>& removedAgentPointers) = 0;

	///Return the Agents within a given rectangle.
	virtual std::vector<Agent const *> agentsInRect(const Point& lowerLeft, const Point& upperRight, const sim_mob::Agent* refAgent) const = 0;

	///Return Agents near to a given Position, with offsets (and Lane) taken into account.
	virtual std::vector<Agent const *> nearbyAgents(const Point& position, const Lane& lane, centimeter_t distanceInFront, centimeter_t distanceBehind, const sim_mob::Agent* refAgent) const = 0;
};



}
