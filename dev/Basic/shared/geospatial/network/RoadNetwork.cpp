//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include <stdexcept>
#include <sstream>

#include "RoadNetwork.hpp"
#include "Link.hpp"
#include "logging/Log.hpp"
#include "util/GeomHelpers.hpp"
#include "util/LangHelpers.hpp"
#include "RoadItem.hpp"

using namespace sim_mob;

RoadNetwork* RoadNetwork::roadNetwork = nullptr;

RoadNetwork::RoadNetwork()
{
}

RoadNetwork::~RoadNetwork()
{
	clear_delete_map(mapOfIdvsNodes);
	clear_delete_map(mapOfIdVsLinks);
	clear_delete_map(mapOfIdVsTurningConflicts);

	//All other maps can simply be cleared as the 'Node' and 'Link' classes contain the others.
	//So, when they get destroyed, the objects contained within them will be destroyed 
	mapOfIdVsLanes.clear();
	mapOfIdVsRoadSegments.clear();
	mapOfIdvsTurningGroups.clear();
	mapOfIdvsTurningPaths.clear();
	mapOfIdvsBusStops.clear();

	roadNetwork = NULL;
}

const std::map<unsigned int, Link *>& RoadNetwork::getMapOfIdVsLinks() const
{
	return mapOfIdVsLinks;
}

const std::map<unsigned int, Node *>& RoadNetwork::getMapOfIdvsNodes() const
{
	return mapOfIdvsNodes;
}

const std::map<unsigned int, Lane*>& RoadNetwork::getMapOfIdVsLanes() const
{
	return mapOfIdVsLanes;
}

const std::map<unsigned int, RoadSegment*>& RoadNetwork::getMapOfIdVsRoadSegments() const
{
	return mapOfIdVsRoadSegments;
}

const std::map<unsigned int, TurningGroup *>& RoadNetwork::getMapOfIdvsTurningGroups() const
{
	return mapOfIdvsTurningGroups;
}

const std::map<unsigned int, TurningPath *>& RoadNetwork::getMapOfIdvsTurningPaths() const
{
	return mapOfIdvsTurningPaths;
}

const std::map<unsigned int, TurningConflict *>& RoadNetwork::getMapOfIdvsTurningConflicts() const
{
	return mapOfIdVsTurningConflicts;
}

const std::map<unsigned int, BusStop *>& RoadNetwork::getMapOfIdvsBusStops() const
{
	return mapOfIdvsBusStops;
}

void RoadNetwork::addLane(Lane* lane)
{
	//Find the segment to which the lane belongs
	std::map<unsigned int, RoadSegment *>::iterator itSegments = mapOfIdVsRoadSegments.find(lane->getRoadSegmentId());

	//Check if the segment exists in the map
	if (itSegments != mapOfIdVsRoadSegments.end())
	{
		//Link the lane and its parent segment
		lane->setParentSegment(itSegments->second);
		
		//Add the lane to the road segment
		itSegments->second->addLane(lane);

		//Add the lane to the map of lanes
		mapOfIdVsLanes.insert(std::make_pair(lane->getLaneId(), lane));
	}
	else
	{
		std::stringstream msg;
		msg << "\nLane " << lane->getRoadSegmentId() << " refers to an invalid segment " << lane->getRoadSegmentId();
		Print() << msg.str();
		safe_delete_item(lane);
		//throw std::runtime_error(msg.str());		
	}
}

void RoadNetwork::addLaneConnector(LaneConnector* connector)
{
	//Find the lanes to which the lane connector belongs
	std::map<unsigned int, Lane *>::iterator itFromLanes = mapOfIdVsLanes.find(connector->getFromLaneId());
	std::map<unsigned int, Lane *>::iterator itToLanes = mapOfIdVsLanes.find(connector->getToLaneId());

	//Check if the lane exists in the map
	if (itFromLanes != mapOfIdVsLanes.end() && itToLanes != mapOfIdVsLanes.end())
	{
		//Add the outgoing lane connector to the lane
		itFromLanes->second->setLaneConnector(connector);

		//Add the from and to lanes to the lane connector
		connector->setFromLane(itFromLanes->second);
		connector->setToLane(itToLanes->second);
	}
	else
	{
		std::stringstream msg;
		msg << "\nLane connector " << connector->getLaneConnectionId() << " refers to an invalid lane - " << connector->getFromLaneId()
			<< " or " << connector->getToLaneId();
		Print() << msg.str();
		safe_delete_item(connector);
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addLanePolyLine(PolyPoint point)
{
	//Find the lane to which the poly-line belongs
	std::map<unsigned int, Lane *>::iterator itLanes = mapOfIdVsLanes.find(point.getPolyLineId());

	//Check if the lane exists in the map
	if (itLanes != mapOfIdVsLanes.end())
	{
		//Check if the poly-line exists for this segment
		PolyLine *polyLine = itLanes->second->getPolyLine();

		if (polyLine == NULL)
		{
			//No poly-line exists, so create a new one
			polyLine = new PolyLine();
			polyLine->setPolyLineId(point.getPolyLineId());

			//Add poly-line to the map
			itLanes->second->setPolyLine(polyLine);
		}
		else
		{
			//Update the length of the poly-line

			//Get the length calculated till the last added point
			double length = polyLine->getLength();
			const PolyPoint& lastPoint = polyLine->getLastPoint();

			//Add the distance between the new point and the previously added point
			length += sim_mob::dist(lastPoint.getX(), lastPoint.getY(), point.getX(), point.getY());

			//Set the length
			polyLine->setLength(length);
		}

		//Add the point to the poly-line
		polyLine->addPoint(point);
	}
	else
	{
		std::stringstream msg;
		msg << "\nLane poly-line " << point.getPolyLineId() << " refers to an invalid lane " << point.getPolyLineId();
		Print() << msg.str();
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addLink(Link *link)
{
	//Set the from node of the link
	std::map<unsigned int, Node *>::iterator itNodes = mapOfIdvsNodes.find(link->getFromNodeId());

	if (itNodes != mapOfIdvsNodes.end())
	{
		link->setFromNode(itNodes->second);
	}
	else
	{
		std::stringstream msg;
		msg << "\nLink " << link->getLinkId() << " refers to an invalid fromNode " << link->getFromNodeId();
		Print() << msg.str();
		safe_delete_item(link);
		return;
		//throw std::runtime_error(msg.str());
	}

	//Set the to node of the link
	itNodes = mapOfIdvsNodes.find(link->getToNodeId());

	if (itNodes != mapOfIdvsNodes.end())
	{
		link->setToNode(itNodes->second);

		//Add link to the map of links
		mapOfIdVsLinks.insert(std::make_pair(link->getLinkId(), link));
	}
	else
	{
		std::stringstream msg;
		msg << "\nLink " << link->getLinkId() << " refers to an invalid toNode " << link->getToNodeId();
		Print() << msg.str();
		safe_delete_item(link);
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addNode(Node *node)
{
	mapOfIdvsNodes.insert(std::make_pair(node->getNodeId(), node));
}

void RoadNetwork::addRoadSegment(RoadSegment* segment)
{
	//Find the link to which the segment belongs
	std::map<unsigned int, Link *>::iterator itLinks = mapOfIdVsLinks.find(segment->getLinkId());

	//Check if the link exists in the map
	if (itLinks != mapOfIdVsLinks.end())
	{
		//Link the segment and its parent link
		segment->setParentLink(itLinks->second);
		
		//Add the road segment to the link
		itLinks->second->addRoadSegment(segment);

		//Add the road segment to the map of road segments
		mapOfIdVsRoadSegments.insert(std::make_pair(segment->getRoadSegmentId(), segment));
	}
	else
	{
		std::stringstream msg;
		msg << "\nRoadSegment " << segment->getRoadSegmentId() << " refers to an invalid link " << segment->getLinkId();
		Print() << msg.str();
		safe_delete_item(segment);
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addSegmentPolyLine(PolyPoint point)
{
	//Find the road segment to which the poly-line belongs
	std::map<unsigned int, RoadSegment *>::iterator itSegments = mapOfIdVsRoadSegments.find(point.getPolyLineId());

	//Check if the segment exists in the map
	if (itSegments != mapOfIdVsRoadSegments.end())
	{
		//Check if the poly-line exists for this segment
		PolyLine *polyLine = itSegments->second->getPolyLine();

		if (polyLine == NULL)
		{
			//No poly-line exists, so create a new one
			polyLine = new PolyLine();
			polyLine->setPolyLineId(point.getPolyLineId());

			//Add poly-line to the map
			itSegments->second->setPolyLine(polyLine);
		}
		else
		{
			//Update the length of the poly-line

			//Get the length calculated till the last added point
			double length = polyLine->getLength();
			const PolyPoint& lastPoint = polyLine->getLastPoint();

			//Add the distance between the new point and the previously added point
			length += sim_mob::dist(lastPoint.getX(), lastPoint.getY(), point.getX(), point.getY());

			//Set the length
			polyLine->setLength(length);
		}

		//Add the point to the poly-line
		polyLine->addPoint(point);
	}
	else
	{
		std::stringstream msg;
		msg << "\nSegment poly-line " << point.getPolyLineId() << " refers to an invalid road segment " << point.getPolyLineId();
		Print() << msg.str();
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addTurningConflict(TurningConflict* turningConflict)
{
	TurningPath *first = NULL, *second = NULL;

	//First turning
	//Find the turning path to which the conflict belongs
	std::map<unsigned int, TurningPath *>::iterator itPaths = mapOfIdvsTurningPaths.find(turningConflict->getFirstTurningId());

	//Check if the turning path exists in the map
	if (itPaths != mapOfIdvsTurningPaths.end())
	{
		first = itPaths->second;

		//Set the turning path to the conflict
		turningConflict->setFirstTurning(itPaths->second);
	}
	else
	{
		std::stringstream msg;
		msg << "\nTurning conflict " << turningConflict->getConflictId() << " refers to an invalid turning path " << turningConflict->getFirstTurningId();
		Print() << msg.str();
		safe_delete_item(turningConflict);
		return;
		//throw std::runtime_error(msg.str());
	}

	//Second turning
	//Find the turning path to which the conflict belongs
	itPaths = mapOfIdvsTurningPaths.find(turningConflict->getSecondTurningId());

	//Check if the turning path exists in the map
	if (itPaths != mapOfIdvsTurningPaths.end())
	{
		second = itPaths->second;

		//Set the turning path to the conflict
		turningConflict->setSecondTurning(itPaths->second);

		//Add the conflict to the turning - to both the turnings
		first->addTurningConflict(second, turningConflict);
		second->addTurningConflict(first, turningConflict);

		//Add the conflict to the map of conflicts
		mapOfIdVsTurningConflicts.insert(std::make_pair(turningConflict->getConflictId(), turningConflict));
	}
	else
	{
		std::stringstream msg;
		msg << "\nTurning conflict " << turningConflict->getConflictId() << " refers to an invalid turning path " << turningConflict->getSecondTurningId();
		Print() << msg.str();
		safe_delete_item(turningConflict);
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addTurningGroup(TurningGroup *turningGroup)
{
	//Find the node to which the turning group belongs
	std::map<unsigned int, Node *>::iterator itNodes = mapOfIdvsNodes.find(turningGroup->getNodeId());

	//Check if the links that the turning group connects exist
	if (mapOfIdVsLinks.count(turningGroup->getFromLinkId()) && mapOfIdVsLinks.count(turningGroup->getToLinkId()))
	{
		//Check if the node exists in the map
		if (itNodes != mapOfIdvsNodes.end())
		{
			//Add the turning to the node
			itNodes->second->addTurningGroup(turningGroup);

			//Add the turning group to the map of turning groups
			mapOfIdvsTurningGroups.insert(std::make_pair(turningGroup->getTurningGroupId(), turningGroup));
		}
		else
		{
			std::stringstream msg;
			msg << "\nTurning group " << turningGroup->getTurningGroupId() << " refers to an invalid Node " << turningGroup->getNodeId();
			Print() << msg.str();
			safe_delete_item(turningGroup);
			//throw std::runtime_error(msg.str());
		}
	}
	else
	{
		std::stringstream msg;
		msg << "\nTurning group " << turningGroup->getTurningGroupId() << " refers to an invalid Link " << turningGroup->getFromLinkId() << " or "
				<< turningGroup->getToLinkId();
		Print() << msg.str();
		safe_delete_item(turningGroup);
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addTurningPath(TurningPath* turningPath)
{
	//Find the turning group to which the turning path belongs
	std::map<unsigned int, TurningGroup *>::iterator itGroups = mapOfIdvsTurningGroups.find(turningPath->getTurningGroupId());

	//Find the lanes which the turning path connects
	std::map<unsigned int, Lane *>::iterator itFromLanes = mapOfIdVsLanes.find(turningPath->getFromLaneId());
	std::map<unsigned int, Lane *>::iterator itToLanes = mapOfIdVsLanes.find(turningPath->getToLaneId());

	//Check if the turning group and the lanes exists in the map
	if (itGroups != mapOfIdvsTurningGroups.end())
	{
		if (itFromLanes != mapOfIdVsLanes.end() && itToLanes != mapOfIdVsLanes.end())
		{			
			//Add the from and to lanes
			turningPath->setFromLane(itFromLanes->second);
			turningPath->setToLane(itToLanes->second);
			
			//Add the turning path to the turning group
			itGroups->second->addTurningPath(turningPath);

			//Add the turning path to the map of turning paths
			mapOfIdvsTurningPaths.insert(std::make_pair(turningPath->getTurningPathId(), turningPath));
		}
		else
		{
			std::stringstream msg;
			msg << "\nTurning path " << turningPath->getTurningPathId() << " refers to an invalid lane " << turningPath->getFromLaneId() << " or "
					<< turningPath->getToLaneId();
			Print() << msg.str();
			safe_delete_item(turningPath);
			//throw std::runtime_error(msg.str());
		}
	}
	else
	{
		std::stringstream msg;
		msg << "\nTurning path " << turningPath->getTurningPathId() << " refers to an invalid turning group " << turningPath->getTurningGroupId();
		Print() << msg.str();
		safe_delete_item(turningPath);
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addTurningPolyLine(PolyPoint point)
{
	//Find the turning path to which the poly-line belongs
	std::map<unsigned int, TurningPath *>::iterator itTurnings = mapOfIdvsTurningPaths.find(point.getPolyLineId());

	//Check if the turning path exists in the map
	if (itTurnings != mapOfIdvsTurningPaths.end())
	{
		//Check if the poly-line exists for this turning
		PolyLine *polyLine = itTurnings->second->getPolyLine();

		if (polyLine == NULL)
		{
			//No poly-line exists, so create a new one
			polyLine = new PolyLine();
			polyLine->setPolyLineId(point.getPolyLineId());

			//Add poly-line to the map
			itTurnings->second->setPolyLine(polyLine);
		}
		else
		{
			//Update the length of the poly-line

			//Get the length calculated till the last added point
			double length = polyLine->getLength();
			const PolyPoint& lastPoint = polyLine->getLastPoint();

			//Add the distance between the new point and the previously added point
			length += sim_mob::dist(lastPoint.getX(), lastPoint.getY(), point.getX(), point.getY());

			//Set the length
			polyLine->setLength(length);
		}

		//Add the point to the poly-line
		polyLine->addPoint(point);
	}
	else
	{
		std::stringstream msg;
		msg << "\nTurning poly-line " << point.getPolyLineId() << " refers to an invalid turning path " << point.getPolyLineId();
		Print() << msg.str();
		//throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addBusStop(BusStop* stop)
{
	//Check if the bus stop has already been added to the map
	std::map<unsigned int, BusStop *>::iterator itStop = mapOfIdvsBusStops.find(stop->getRoadItemId());

	if (itStop != mapOfIdvsBusStops.end())
	{
		std::stringstream msg;
		msg << "Bus stop " << stop->getRoadItemId()  << " with stop code " << stop->getStopCode() << " has already been added!";
		safe_delete_item(stop);
		throw std::runtime_error(msg.str());
	}
	else
	{			
		//Get the road segment to which the bus stop belongs
		std::map<unsigned int, RoadSegment *>::iterator itSegments = mapOfIdVsRoadSegments.find(stop->getRoadSegmentId());

		if (itSegments != mapOfIdVsRoadSegments.end())
		{
			double offset = stop->getOffset();
			double stopHalfLength = stop->getLength() / 2;
			
			//Ensure that the stop doesn't go beyond the road segment
			if ((offset + stopHalfLength) > itSegments->second->getLength())
			{
				offset = itSegments->second->getLength() - stopHalfLength;
				stop->setOffset(offset);
			}

			//Set the parent segment of the bus stop
			stop->setParentSegment(itSegments->second);

			//Add the stop to the segment
			itSegments->second->addObstacle(offset, stop);
			
			//Insert the stop into the map
			mapOfIdvsBusStops.insert(std::make_pair(stop->getRoadItemId(), stop));
			BusStop::registerBusStop(stop);
		}
		else
		{
			std::stringstream msg;
			msg << "Bus stop " << stop->getRoadItemId() << " refers to an invalid road segment " << stop->getRoadSegmentId();
			safe_delete_item(stop);
			throw std::runtime_error(msg.str());
		}
	}
}

const RoadNetwork* sim_mob::RoadNetwork::getInstance()
{
	if(!roadNetwork)
	{
		roadNetwork = new RoadNetwork();
	}
	return roadNetwork;
}

RoadNetwork* RoadNetwork::getWritableInstance()
{
	if (!roadNetwork)
	{
		roadNetwork = new RoadNetwork();
	}
	return roadNetwork;
}

