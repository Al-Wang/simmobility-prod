//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "RoadItem.hpp"

using namespace sim_mob;

RoadItem::RoadItem() :
roadItemId(0), geometryId(0), polyLineId(0), roadSegmentId(0)
{
}

RoadItem::RoadItem(unsigned int id, unsigned int geomteryId, unsigned int polyLineId, unsigned int roadSectionId) :
roadItemId(id), geometryId(geomteryId), polyLineId(polyLineId), roadSegmentId(roadSectionId)
{
}

RoadItem::~RoadItem()
{
}

unsigned int RoadItem::getRoadItemId() const
{
	return roadItemId;
}

void RoadItem::setRoadItemId(unsigned int roadItemId)
{
	this->roadItemId = roadItemId;
}

unsigned int RoadItem::getGeometryId() const
{
	return geometryId;
}

void RoadItem::setGeometryId(unsigned int geometryId)
{
	this->geometryId = geometryId;
}

unsigned int RoadItem::getPolyLineId() const
{
	return polyLineId;
}

void RoadItem::setPolyLineId(unsigned int polyLineId)
{
	this->polyLineId = polyLineId;
}

unsigned int RoadItem::getRoadSegmentId() const
{
	return roadSegmentId;
}

void RoadItem::setRoadSegmentId(unsigned int roadSegmentId)
{
	this->roadSegmentId = roadSegmentId;
}
