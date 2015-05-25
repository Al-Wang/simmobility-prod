//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Lane.hpp"

using namespace simmobility_network;

Lane::Lane() :
laneId(0), busLaneRules(BUS_LANE_RULES_CAR_AND_BUS), canVehiclePark(false), canVehicleStop(false), hasRoadShoulder(false),
isHOV_Allowed(false), laneIndex(0), polyLine(NULL), width(0)
{
}

Lane::Lane(const Lane& orig)
{
	this->laneId = orig.laneId;
	this->busLaneRules = orig.busLaneRules;
	this->canVehiclePark = orig.canVehiclePark;
	this->canVehicleStop = orig.canVehicleStop;
	this->hasRoadShoulder = orig.hasRoadShoulder;
	this->isHOV_Allowed = orig.isHOV_Allowed;
	this->laneConnectors = orig.laneConnectors;
	this->laneIndex = orig.laneIndex;
	this->polyLine = orig.polyLine;	
	this->tags = orig.tags;
	this->width = orig.width;
}

Lane::~Lane()
{
	//Delete the outgoing lane connectors
	for(std::vector<LaneConnector *>::iterator itConnectors = laneConnectors.begin(); itConnectors != laneConnectors.end(); ++itConnectors)
	{
		delete *itConnectors;
		*itConnectors = NULL;		
	}
	
	if(polyLine)
	{
		delete polyLine;
		polyLine = NULL;
	}
	
	tags.clear();
}

unsigned int Lane::getLaneId() const
{
	return laneId;
}

void Lane::setLaneId(unsigned int laneId)
{
	this->laneId = laneId;
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

void Lane::setLaneConnectors(std::vector<LaneConnector*>& laneConnectors)
{
	this->laneConnectors = laneConnectors;
}

unsigned int Lane::getLaneIndex() const
{
	return laneIndex;
}

void Lane::setLaneIndex(unsigned int laneIndex)
{
	this->laneIndex = laneIndex;
}

PolyLine* Lane::getPolyLine() const
{
	return polyLine;
}

void Lane::setPolyLine(PolyLine* polyLine)
{
	this->polyLine = polyLine;
}

const std::vector<Tag>& Lane::getTags() const
{
	return tags;
}

void Lane::setTags(std::vector<Tag>& tags)
{
	this->tags = tags;
}

double Lane::getWidth() const
{
	return width;
}

void Lane::setWidth(double width)
{
	this->width = width;
}

