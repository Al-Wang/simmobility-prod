//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "RoadNetwork.hpp"

#include <stdexcept>
#include <sstream>
#include <limits>

#include "Link.hpp"
#include "logging/Log.hpp"
#include "Node.hpp"
#include "ParkingArea.hpp"
#include "ParkingSlot.hpp"
#include "SMSVehicleParking.hpp"
#include "Point.hpp"
#include "PolyLine.hpp"
#include "RoadItem.hpp"
#include "TurningGroup.hpp"
#include "TurningPath.hpp"
#include "util/GeomHelpers.hpp"
#include "util/LangHelpers.hpp"

using namespace sim_mob;

RoadNetwork* RoadNetwork::roadNetwork = nullptr;

RoadNetwork::RoadNetwork(): turningPathFromLanes(std::map<const Lane*,std::map<const Lane*,const TurningPath *>>())
{
}

RoadNetwork::~RoadNetwork()
{
    clear_delete_map(mapOfIdvsNodes);
    clear_delete_map(mapOfIdVsLinks);
    clear_delete_map(mapOfIdVsTurningConflicts);
	clear_delete_map(mapOfIdVsParkingAreas);

    //All other maps can simply be cleared as the 'Node' and 'Link' classes contain the others.
	//So, when they get destroyed, the objects contained within them will be destroyed 
	mapOfIdVsLanes.clear();
	mapOfIdVsRoadSegments.clear();
	mapOfIdvsTurningGroups.clear();
	mapOfIdvsTurningPaths.clear();
	mapOfIdvsBusStops.clear();
	mapOfIdVsParkingSlots.clear();
	mapOfIdVsSMSVehiclesParking.clear();
	mapOfDownstreamLinks.clear();
	mapOfUpstreamLinks.clear();

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

const std::map<unsigned int, ParkingSlot *>& RoadNetwork::getMapOfIdVsParkingSlots() const
{
	return mapOfIdVsParkingSlots;
}

const map<unsigned int, TaxiStand *>& RoadNetwork::getMapOfIdvsTaxiStands() const
{
	return mapOfIdvsTaxiStands;
}

const std::map<unsigned int, ParkingArea *>& RoadNetwork::getMapOfIdVsParkingAreas() const
{
	return mapOfIdVsParkingAreas;
}

const std::map<unsigned int, SMSVehicleParking *>& RoadNetwork::getMapOfIdvsSMSVehicleParking() const
{
	return mapOfIdVsSMSVehiclesParking;
}

const std::vector<const Link *>& RoadNetwork::getDownstreamLinks(unsigned int fromNodeId) const
{
	return mapOfDownstreamLinks.at(fromNodeId);
}

const std::vector<const Link *>& RoadNetwork::getUpstreamLinks(unsigned int fromNodeId) const
{
	return mapOfUpstreamLinks.at(fromNodeId);
}

void RoadNetwork::addLane(Lane* lane)
{
	//Find the segment to which the lane belongs
	std::map<unsigned int, RoadSegment *>::iterator itAccSegments = mapOfIdVsRoadSegments.find(lane->getRoadSegmentId());

	//Check if the segment exists in the map
	if (itAccSegments != mapOfIdVsRoadSegments.end())
	{
		//Link the lane and its parent segment
		lane->setParentSegment(itAccSegments->second);
		
		//Add the lane to the road segment
		itAccSegments->second->addLane(lane);

		//Add the lane to the map of lanes
		mapOfIdVsLanes.insert(std::make_pair(lane->getLaneId(), lane));
	}
	else
	{
		std::stringstream msg;
		msg << "\nLane " << lane->getLaneId() << " refers to an invalid segment " << lane->getRoadSegmentId();
		safe_delete_item(lane);
		throw std::runtime_error(msg.str());
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
		itFromLanes->second->addLaneConnector(connector);

		//Add the from and to lanes to the lane connector
		connector->setFromLane(itFromLanes->second);
		connector->setToLane(itToLanes->second);
	}
	else
	{
		std::stringstream msg;
		msg << "\nLane connector " << connector->getLaneConnectionId() << " refers to an invalid lane - " << connector->getFromLaneId() << " or "
				<< connector->getToLaneId();
		safe_delete_item(connector);
		throw std::runtime_error(msg.str());
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
		throw std::runtime_error(msg.str());
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
		safe_delete_item(link);
		throw std::runtime_error(msg.str());
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
		safe_delete_item(link);
		throw std::runtime_error(msg.str());
	}

	//Update map of upstream and downstream links
	mapOfDownstreamLinks[link->getFromNodeId()].push_back((const Link*)link);
	mapOfUpstreamLinks[link->getToNodeId()].push_back((const Link*)link);
}

void RoadNetwork::addNode(Node *node)
{
	mapOfIdvsNodes.insert(std::make_pair(node->getNodeId(), node));
}

bool RoadNetwork::checkSegmentCapacity() const
{
	for(auto i=mapOfIdVsRoadSegments.begin(); i != mapOfIdVsRoadSegments.end(); i++)
	{
		if((*i).second->getCapacity()<=0.0)
		{
			Print()<<"no capacity in segment "<<(*i).second->getRoadSegmentId()<<std::endl;
			return false;
		}
	}
	return true;
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
		safe_delete_item(segment);
		throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addSegmentPolyLine(PolyPoint point)
{
	//Find the road segment to which the poly-line belongs
	std::map<unsigned int, RoadSegment *>::iterator itAccSegments = mapOfIdVsRoadSegments.find(point.getPolyLineId());

	//Check if the segment exists in the map
	if (itAccSegments != mapOfIdVsRoadSegments.end())
	{
		//Check if the poly-line exists for this segment
		PolyLine *polyLine = itAccSegments->second->getPolyLine();

		if (polyLine == NULL)
		{
			//No poly-line exists, so create a new one
			polyLine = new PolyLine();
			polyLine->setPolyLineId(point.getPolyLineId());

			//Add poly-line to the map
			itAccSegments->second->setPolyLine(polyLine);
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
		throw std::runtime_error(msg.str());
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
		safe_delete_item(turningConflict);
		throw std::runtime_error(msg.str());
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
		safe_delete_item(turningConflict);
		throw std::runtime_error(msg.str());
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
			safe_delete_item(turningGroup);
			throw std::runtime_error(msg.str());
		}
	}
	else
	{
		std::stringstream msg;
		msg << "\nTurning group " << turningGroup->getTurningGroupId() << " refers to an invalid Link " << turningGroup->getFromLinkId() << " or "
				<< turningGroup->getToLinkId();
		safe_delete_item(turningGroup);
		throw std::runtime_error(msg.str());
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
			
			//Set the turning group
			turningPath->setTurningGroup(itGroups->second);
			
			//Add the turning path to the turning group
			itGroups->second->addTurningPath(turningPath);

			//Add the turning path to the map of turning paths
			mapOfIdvsTurningPaths.insert(std::make_pair(turningPath->getTurningPathId(), turningPath));
			const Lane *fromLane = turningPath->getFromLane();
			const Lane *toLane = turningPath->getToLane();
			if (turningPathFromLanes.find(fromLane) == turningPathFromLanes.end())
			{
				std::map<const Lane*,const TurningPath*> mapOfLaneTurningGroup;
				turningPathFromLanes[fromLane] = mapOfLaneTurningGroup;
			}
			std::map<const Lane*,const TurningPath*> &mapOfLanetoTurningGroups = turningPathFromLanes[fromLane];
			mapOfLanetoTurningGroups[toLane] = turningPath;
		}
		else
		{
			std::stringstream msg;
			msg << "\nTurning path " << turningPath->getTurningPathId() << " refers to an invalid lane " << turningPath->getFromLaneId() << " or "
					<< turningPath->getToLaneId();
			safe_delete_item(turningPath);
			throw std::runtime_error(msg.str());
		}
	}
	else
	{
		std::stringstream msg;
		msg << "\nTurning path " << turningPath->getTurningPathId() << " refers to an invalid turning group " << turningPath->getTurningGroupId();
		safe_delete_item(turningPath);
		throw std::runtime_error(msg.str());
	}
}

const std::map<const Lane*,std::map<const Lane*,const TurningPath *>> & RoadNetwork::getTurningPathsFromLanes() const
{
	return turningPathFromLanes;
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
		throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addTaxiStand(TaxiStand* stand)
{
	//Check if the taxi stand has already been added to the map
	std::map<unsigned int, TaxiStand*>::iterator itStand = mapOfIdvsTaxiStands.find(stand->getStandId());
	if (itStand != mapOfIdvsTaxiStands.end())
	{
		std::stringstream msg;
		msg << "Taxi stand " << stand->getStandId() << " has already been added!";
		safe_delete_item(stand);
		throw std::runtime_error(msg.str());
	}
	else
	{
		//Get the road segment to which the taxi stand belongs
		std::map<unsigned int, RoadSegment *>::iterator itSegments = mapOfIdVsRoadSegments.find(stand->getRoadSegmentId());

		if (itSegments != mapOfIdVsRoadSegments.end())
		{
			RoadSegment* standSegment = itSegments->second;
			double offset = stand->getOffset();

			//Set the parent segment of the taxi stand
			stand->setRoadSegment(standSegment);

			//Add the stand to the segment
			standSegment->addObstacle(offset, stand);

			//Insert the stand into the map
			mapOfIdvsTaxiStands.insert(std::make_pair(stand->getStandId(), stand));
		}
		else
		{
			std::stringstream msg;
			msg << "Taxi Stand " << stand->getStandId() << " refers to an invalid road segment " << stand->getRoadSegmentId();
			safe_delete_item(stand);
			throw std::runtime_error(msg.str());
		}
	}
}

const Node* RoadNetwork::getNodeById(int id) const
{
	return mapOfIdvsNodes.find(id)->second;
}

void RoadNetwork::addSurveillenceStn(SurveillanceStation *station)
{
	std::map<unsigned int, RoadSegment *>::iterator itRoadSeg = mapOfIdVsRoadSegments.find(station->getSegmentId());

	if(itRoadSeg != mapOfIdVsRoadSegments.end())
	{
		RoadSegment *segment = itRoadSeg->second;

		//Set the road segment in the surveillance station and set the surveillance station
		//in the road segment
		station->setSegment(segment);
		segment->addSurveillanceStation(station);
	}
	else
	{
		std::stringstream msg;
		msg << "Surveillance Stn " << station->getSurveillanceStnId() << " refers to an invalid road segment " << station->getSegmentId();
		safe_delete_item(station);
		throw std::runtime_error(msg.str());
	}
}

void RoadNetwork::addBusStop(BusStop* stop)
{
	//Check if the bus stop has already been added to the map
	std::map<unsigned int, BusStop *>::iterator itStop = mapOfIdvsBusStops.find(stop->getStopId());

	if (itStop != mapOfIdvsBusStops.end())
	{
		std::stringstream msg;
		msg << "Bus stop " << stop->getStopId() << " with stop code " << stop->getStopCode() << " has already been added!";
		safe_delete_item(stop);
		throw std::runtime_error(msg.str());
	}
	else
	{			
		//Get the road segment to which the bus stop belongs
		std::map<unsigned int, RoadSegment *>::iterator itAccSegments = mapOfIdVsRoadSegments.find(stop->getRoadSegmentId());

		if (itAccSegments != mapOfIdVsRoadSegments.end())
		{
			RoadSegment* stopSegment = itAccSegments->second;
			double offset = stop->getOffset();
			double stopHalfLength = stop->getLength() / 2;
			
			//Ensure that the stop doesn't go beyond the road segment
			if ((offset + stopHalfLength) > stopSegment->getLength())
			{
				offset = stopSegment->getLength() - stopHalfLength;
				if (offset > 0)
				{
					stop->setOffset(offset);
				}
				else
				{
					offset = stop->getOffset();
				}
			}

			//Set the parent segment of the bus stop
			stop->setParentSegment(stopSegment);

			//Add the stop to the segment
			stopSegment->addObstacle(offset, stop);
			if(stop->getTerminusType() != sim_mob::NOT_A_TERMINUS)
			{
				stopSegment->setBusTerminusSegment();
			}
			
			//Insert the stop into the map
			mapOfIdvsBusStops.insert(std::make_pair(stop->getStopId(), stop));
			BusStop::registerBusStop(stop);
		}
		else
		{
			std::stringstream msg;
			msg << "Bus stop " << stop->getStopId() << " refers to an invalid road segment " << stop->getRoadSegmentId();
			safe_delete_item(stop);
			throw std::runtime_error(msg.str());
		}
	}
}

void RoadNetwork::addSMSVehicleParking(SMSVehicleParking *smsVehicleParking)
{
	//Check if the parking  has already been added to the map
	map<unsigned int, SMSVehicleParking *>::iterator itPark = mapOfIdVsSMSVehiclesParking.find(smsVehicleParking->getParkingId());

	if (itPark != mapOfIdVsSMSVehiclesParking.end())
	{
		stringstream msg;
		msg << "Parking " << smsVehicleParking->getParkingId() << " has already been added!";
		safe_delete_item(smsVehicleParking);
		throw runtime_error(msg.str());
	}
	else
	{
		//Set the road segment
		auto itSegment = mapOfIdVsRoadSegments.find(smsVehicleParking->getSegmentId());

		if(itSegment != mapOfIdVsRoadSegments.end())
		{
			smsVehicleParking->setParkingSegment(itSegment->second);

			//Insert the smsVehicleParking into the map
			mapOfIdVsSMSVehiclesParking.insert(make_pair(smsVehicleParking->getParkingId(), smsVehicleParking));
		}
		else
		{
			stringstream msg;
			msg << "Parking " << smsVehicleParking->getParkingId() << " refers to invalid segment "
			    << smsVehicleParking->getSegmentId();

			safe_delete_item(smsVehicleParking);
			throw runtime_error(msg.str());
		}
	}
}

void RoadNetwork::addParking(ParkingSlot *parkingSlot)
{
	//Check if the parking slot has already been added to the network
	std::map<unsigned int, ParkingSlot *>::iterator itParking = mapOfIdVsParkingSlots.find(parkingSlot->getRoadItemId());

	if(itParking == mapOfIdVsParkingSlots.end())
	{
		//Get the road segments to which the parking slot belongs
		std::map<unsigned int, RoadSegment *>::iterator itAccSegments = mapOfIdVsRoadSegments.find(parkingSlot->getAccessSegmentId());
		std::map<unsigned int, RoadSegment *>::iterator itEgrSegments = mapOfIdVsRoadSegments.find(parkingSlot->getEgressSegmentId());

		if (itAccSegments != mapOfIdVsRoadSegments.end() && itEgrSegments != mapOfIdVsRoadSegments.end())
		{
			RoadSegment *accessSegment = itAccSegments->second;
			RoadSegment *egressSegment = itEgrSegments->second;

			//Set the access segment of the parkingSlot
			parkingSlot->setAccessSegment(accessSegment);

			//Add the parkingSlot to the segment
			accessSegment->addObstacle(parkingSlot->getOffset(), parkingSlot);

			//Set the egress segment of the parking slot
			parkingSlot->setEgressSegment(egressSegment);

			//Check if the Parking area for this slot exists
			ParkingArea *pkArea = nullptr;
			std::map<unsigned int, ParkingArea *>::iterator itPkAreas = mapOfIdVsParkingAreas.find(parkingSlot->getParkingAreaId());

			//Create a new Parking Area if it does not exist
			if (itPkAreas == mapOfIdVsParkingAreas.end())
			{
			    pkArea = new ParkingArea(parkingSlot->getParkingAreaId());

				//Add the parking area to the network
				mapOfIdVsParkingAreas.insert(std::make_pair(pkArea->getParkingAreaId(), pkArea));
			}
			else
			{
				pkArea = itPkAreas->second;
			}

			//Add parking slot to the parking area
			pkArea->addParkingSlot(parkingSlot);

			//Add the parking slot to the network
			mapOfIdVsParkingSlots.insert(std::make_pair(parkingSlot->getRoadItemId(), parkingSlot));
		}
		else
		{
			std::stringstream msg;
			unsigned int invId = (itAccSegments == mapOfIdVsRoadSegments.end()) ? parkingSlot->getAccessSegmentId() : parkingSlot->getEgressSegmentId();
			msg << "Parking slot " << parkingSlot->getRoadItemId() << " refers to an invalid road segment " << invId;
			safe_delete_item(parkingSlot);
			throw std::runtime_error(msg.str());
		}
	}
	else
	{
		std::stringstream msg;
		msg << "Parking slot " << parkingSlot->getRoadItemId() << " has already been added!";
		safe_delete_item(parkingSlot);
		throw std::runtime_error(msg.str());
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

Node* RoadNetwork::locateNearestNode(const Point& position) const
{
	double minDistance = std::numeric_limits<double>::max();
	Node* candidate = nullptr;
	std::map<unsigned int, Node *>::const_iterator it;
	for (it = mapOfIdvsNodes.begin(); it != mapOfIdvsNodes.end(); it++)
	{
		double distance = sim_mob::dist((it->second)->getLocation(), position);
		if (distance < minDistance)
		{
			minDistance = distance;
			candidate = it->second;
		}
	}
	return candidate;
}
