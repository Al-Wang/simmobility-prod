//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Lane.hpp"

using namespace sim_mob;

Lane::Lane() :
laneId(0), busLaneRules(BUS_LANE_RULES_CAR_AND_BUS), canVehiclePark(false), canVehicleStop(false), hasRoadShoulder(false),
isHOV_Allowed(false), laneIndex(0), parentSegment(NULL), polyLine(NULL), roadSegmentId(0), vehicleMode(0), width(0)
{
}

Lane::~Lane()
{
	//Delete the outgoing lane connectors
	for (std::vector<LaneConnector *>::iterator itConnectors = laneConnectors.begin(); itConnectors != laneConnectors.end(); ++itConnectors)
	{
		delete *itConnectors;
		*itConnectors = NULL;
	}

	if (polyLine)
	{
		delete polyLine;
		polyLine = NULL;
	}
}

unsigned int Lane::getLaneId() const
{
	return laneId;
}

void Lane::setLaneId(unsigned int laneId)
{
	this->laneId = laneId;
	this->laneIndex = laneId % 10;
}

BusLaneRules Lane::getBusLaneRules() const
{
	return busLaneRules;
}

void Lane::setBusLaneRules(BusLaneRules busLaneRules)
{
	this->busLaneRules = busLaneRules;
}

bool Lane::isParkingAllowed() const
{
	return canVehiclePark;
}

void Lane::setCanVehiclePark(bool canVehiclePark)
{
	this->canVehiclePark = canVehiclePark;
}

bool Lane::isStoppingAllowed() const
{
	return canVehicleStop;
}

void Lane::setCanVehicleStop(bool canVehicleStop)
{
	this->canVehicleStop = canVehicleStop;
}

bool Lane::doesLaneHaveRoadShoulder() const
{
	return hasRoadShoulder;
}

void Lane::setHasRoadShoulder(bool hasRoadShoulder)
{
	this->hasRoadShoulder = hasRoadShoulder;
}

bool Lane::isHighOccupancyVehicleAllowed() const
{
	return isHOV_Allowed;
}

void Lane::setHighOccupancyVehicleAllowed(bool HighOccupancyVehicleAllowed)
{
	isHOV_Allowed = HighOccupancyVehicleAllowed;
}

const std::vector<LaneConnector*>& Lane::getLaneConnectors() const
{
	return laneConnectors;
}

unsigned int Lane::getLaneIndex() const
{
	return laneIndex;
}

void Lane::setParentSegment(RoadSegment* parentSegment)
{
	this->parentSegment = parentSegment;
}

const RoadSegment* Lane::getParentSegment() const
{
	return parentSegment;
}

PolyLine* Lane::getPolyLine() const
{
	return polyLine;
}

void Lane::setPolyLine(PolyLine* polyLine)
{
	this->polyLine = polyLine;
}

unsigned int Lane::getRoadSegmentId() const
{
	return roadSegmentId;
}

void Lane::setRoadSegmentId(unsigned int roadSegmentId)
{
	this->roadSegmentId = roadSegmentId;
}

double Lane::getWidth() const
{
	return width;
}

void Lane::setWidth(double width)
{
	this->width = width;
}

bool Lane::isPedestrianLane() const
{
	return (vehicleMode & PEDESTRIAN_LANE);
}

bool Lane::isBicycleLane() const
{
	return (vehicleMode & BICYCLE_LANE);
}

void Lane::addLaneConnector(LaneConnector *laneConnector)
{
	this->laneConnectors.push_back(laneConnector);
}