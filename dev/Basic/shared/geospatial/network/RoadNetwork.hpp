//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <map>

#include "Link.hpp"
#include "Node.hpp"
#include "Point.hpp"
#include "PolyLine.hpp"
#include "TurningGroup.hpp"
#include "TurningPath.hpp"

namespace sim_mob
{

/**
 * class for holding the network for simulation
 * \author Neeraj D
 * \author Harish L
 */
class RoadNetwork
{
private:
	static RoadNetwork* roadNetwork;

	RoadNetwork();

	/**This map stores all the links in the network, with the Link id as the key for retrieval*/
	std::map<unsigned int, Link *> mapOfIdVsLinks;

	/**This map stores all the nodes in the network, with the Node id as the key for retrieval*/
	std::map<unsigned int, Node *> mapOfIdvsNodes;

	/**This map stores all the lanes in the network, with the lane id as the key for retrieval*/
	std::map<unsigned int, Lane *> mapOfIdVsLanes;

	/**This map stores all the segments in the network, with the segment id as the key for retrieval*/
	std::map<unsigned int, RoadSegment *> mapOfIdVsRoadSegments;

	/**This map stores all the turning conflicts in the network, with the conflict id as the key for retrieval*/
	std::map<unsigned int, TurningConflict *> mapOfIdVsTurningConflicts;

	/**This map stores all the turning groups in the network, with the group id as the key for retrieval*/
	std::map<unsigned int, TurningGroup *> mapOfIdvsTurningGroups;

	/**This map stores all the turning paths in the network, with the turning id as the key for retrieval*/
	std::map<unsigned int, TurningPath *> mapOfIdvsTurningPaths;

public:
	virtual ~RoadNetwork();

	/**Returns pointer to the singleton instance of RoadNetwork*/
	static RoadNetwork* getInstance();

	/**deletes the singleton instance of RoadNetwork*/
	static void deleteInstance();

	const std::map<unsigned int, Link *>& getMapOfIdVsLinks() const;

	const std::map<unsigned int, Node *>& getMapOfIdvsNodes() const;

	const std::map<unsigned int, TurningGroup *>& getMapOfIdvsTurningGroups() const;

	const std::map<unsigned int, TurningPath *>& getMapOfIdvsTurningPaths() const;

	/**
	 * Adds a lane to the road network
	 * @param lane - the lane to be added
	 */
	void addLane(Lane *lane);

	/**
	 * Adds a lane connector to the road network
	 * @param connector - the lane connector to be added
	 */
	void addLaneConnector(LaneConnector *connector);

	/**
	 * Adds a lane poly-line to the road network
	 * @param point - the poly-point to be added to the poly-line
	 */
	void addLanePolyLine(PolyPoint point);

	/**
	 * Adds a link to the road network
	 * @param link - the link to be added
	 */
	void addLink(Link *link);

	/**
	 * Adds a node to the road network
	 * @param node - the node to be added
	 */
	void addNode(Node *node);

	/**
	 * Adds a road segment to the road network
	 * @param segment - the road segment to be added
	 */
	void addRoadSegment(RoadSegment *segment);

	/**
	 * Adds a segment poly-line to the road network
	 * @param point - the poly-point to be added to the poly-line
	 */
	void addSegmentPolyLine(PolyPoint point);

	/**
	 * Adds a turning conflict to the road network
	 * @param turningConflict - the conflict to be added
	 */
	void addTurningConflict(TurningConflict *turningConflict);

	/**
	 * Adds a turning group to the road network
	 * @param turningGroup - the turning group to be added
	 */
	void addTurningGroup(TurningGroup *turningGroup);

	/**
	 * Adds a turning path to the road network
	 * @param turningPath - the turning path to be added
	 */
	void addTurningPath(TurningPath *turningPath);

	/**
	 * Adds a turning poly-line to the road network
	 * @param point - the poly-point to be added to the poly-line
	 */
	void addTurningPolyLine(PolyPoint point);

	/**
	 * Looks-up the required node from the map of nodes
	 * @param nodeId - the id of the required node
	 * @return a pointer to the node if found; NULL otherwise
	 */
	Node * getNodeById(unsigned int nodeId);
};
}

