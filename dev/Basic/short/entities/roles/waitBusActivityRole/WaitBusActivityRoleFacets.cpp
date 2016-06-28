//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * WaitBusActivityRoleFacets.cpp
 *
 *  Created on: Jun 17th, 2013
 *      Author: Yao Jin
 */

#include "WaitBusActivityRoleFacets.hpp"

#include "entities/Person.hpp"
#include "entities/roles/Role.hpp"
#include "geospatial/network/PT_Stop.hpp"
#include "geospatial/network/Link.hpp"
#include "geospatial/network/RoadSegment.hpp"
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "util/GeomHelpers.hpp"

using namespace sim_mob;

BusStop* getbusStop(const Node* node, RoadSegment* segment)
{
	std::map<double, RoadItem*>::const_iterator ob_it;
	const std::map<double, RoadItem*> & obstacles = segment->getObstacles();
	for (ob_it = obstacles.begin(); ob_it != obstacles.end(); ++ob_it)
	{
		RoadItem* ri = const_cast<RoadItem*> (ob_it->second);
		BusStop *bs = dynamic_cast<BusStop*> (ri);
		/*if (bs && ((segment->getStart() == node) || (segment->getEnd() == node) )) {
			return bs;
		}*/
	}
	return nullptr;
}

WaitBusActivityRoleBehavior::WaitBusActivityRoleBehavior() :
BehaviorFacet(), parentWaitBusActivityRole(nullptr)
{
}

WaitBusActivityRoleBehavior::~WaitBusActivityRoleBehavior()
{
}

WaitBusActivityRoleMovement::WaitBusActivityRoleMovement(std::string buslineid) :
MovementFacet(), parentWaitBusActivityRole(nullptr), busStopAgent(nullptr), registered(false),
buslineId(buslineid), boardingMS(0), busDriver(nullptr), isTagged(false), isBoarded(false)
{
}

WaitBusActivityRoleMovement::~WaitBusActivityRoleMovement()
{

}

BusStop* WaitBusActivityRoleMovement::setBusStopXY(const Node* node)//to find the nearest busstop to a node
{
	const Node* currEndNode = node;
	double dist = 0;
	BusStop*bs1 = 0;

	/*
	if(currEndNode)
	{
		const std::set<RoadSegment*>& segments_ = currEndNode->getRoadSegments();
		BusStop* busStop_ptr = nullptr;
		for(std::set<RoadSegment*>::const_iterator i = segments_.begin();i !=segments_.end();i++)
		{
		   BusStop* bustop_ = (*i)->getBusStop();
		   busStop_ptr = getbusStop(node,(*i));
		   if(busStop_ptr)
		   {
		   double newDist = dist(busStop_ptr->xPos, busStop_ptr->yPos,node->location.getX(), node->location.getY());
		   if((newDist<dist || dist==0)&& busStop_ptr->BusLines.size()!=0)
			  {
				dist=newDist;
				bs1=busStop_ptr;
			  }
		   }
		}
	}
	else
	{
		Point point = node->location;
		const StreetDirectory::LaneAndIndexPair lane_index =  StreetDirectory::Instance().getLane(point);
		if(lane_index.lane_)
		{
			Link* link_= lane_index.lane_->getRoadSegment()->getLink();
			const Link* link_2 = StreetDirectory::Instance().searchLink(link_->getEnd(),link_->getStart());
			BusStop* busStop_ptr = nullptr;

			std::vector<RoadSegment*> segments_ ;

			if(link_)
			{
				segments_= const_cast<Link*>(link_)->getSegments();
				for(std::vector<RoadSegment*>::const_iterator i = segments_.begin();i != segments_.end();i++)
				{
				   BusStop* bustop_ = (*i)->getBusStop();
				   busStop_ptr = getbusStop(node,(*i));
				   if(busStop_ptr)
				   {
					   double newDist = dist(busStop_ptr->xPos, busStop_ptr->yPos,point.getX(), point.getY());
					   if((newDist<dist || dist==0)&& busStop_ptr->BusLines.size()!=0)
						{
						   dist=newDist;
						   bs1=busStop_ptr;
						}
				   }
				}
			}

			if(link_2)
			{
				segments_ = const_cast<Link*>(link_2)->getSegments();
				for(std::vector<RoadSegment*>::const_iterator i = segments_.begin();i != segments_.end();i++)
				{
				   BusStop* bustop_ = (*i)->getBusStop();
				   busStop_ptr = getbusStop(node,(*i));
				   if(busStop_ptr)
				   {
					   double newDist = dist(busStop_ptr->xPos, busStop_ptr->yPos,point.getX(), point.getY());
					   std::cout << "busStop_ptr->BusLines.size(): " << busStop_ptr->BusLines.size() << std::endl;
					   if((newDist<dist || dist==0)&& busStop_ptr->BusLines.size()!=0)
					   {
						  dist=newDist;
						  bs1=busStop_ptr;
						}
				   }
				}
			}
		}

	}*/
	return bs1;
}
