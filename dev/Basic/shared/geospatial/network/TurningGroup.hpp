//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <map>
#include <vector>

#include "TurningPath.hpp"

namespace sim_mob
{

/**Defines the rules vehicles must observe at all the turnings in the same group*/
enum TurningGroupRules
{
	/**No stop sign at the turning group*/
	TURNING_GROUP_RULE_NO_STOP_SIGN = 0,

	/**Stop sign present at the turning group*/
	TURNING_GROUP_RULE_STOP_SIGN = 1
};

/**
 * A turning group is a group of turning paths that connect the same links across a node.
 * This class defines the structure of a turning group.
 * \author Neeraj D
 * \author Harish L
 */
class TurningGroup
{
public:

	/**Unique identifier for the turning group*/
	unsigned int turningGroupId;

	/**Indicates the link from which this turning group originates*/
	unsigned int fromLinkId;

	/**The id of the node to which this turning group belongs*/
	unsigned int nodeId;

	/**Indicates the phases of the traffic light during which the vehicles can pass*/
	std::string phases;

	/**Stores the turning group rules*/
	TurningGroupRules rules;

	/**Indicates the link at which this turning group terminates*/
	unsigned int toLinkId;

	/**The turning paths located in a turning group. The outer map stores the 'from lane id' vs
	 * an inner map, which store the 'to lane id' vs the turning path
	 */
	std::map<unsigned int, std::map<unsigned int, TurningPath *> > turningPaths;

	/**Defines the visibility of the intersection from the turning group (m/s)*/
	double visibility;

public:

	TurningGroup();
	
	virtual ~TurningGroup();

	unsigned int getTurningGroupId() const;
	void setTurningGroupId(unsigned int turningGroupId);

	unsigned int getFromLinkId() const;
	void setFromLinkId(unsigned int fromLinkId);

	unsigned int getNodeId() const;
	void setNodeId(unsigned int nodeId);

	std::string getPhases() const;
	void setPhases(std::string phases);

	TurningGroupRules getRules() const;
	void setRules(TurningGroupRules rules);

	unsigned int getToLinkId() const;
	void setToLinkId(unsigned int toLinkId);

	double getVisibility() const;
	void setVisibility(double visibility);

	/**
	 * Adds the turning path into the map of turningPaths
	 * @param turningPath - turning path to be added to the turning group
	 */
	void addTurningPath(TurningPath *turningPath);
};
}
