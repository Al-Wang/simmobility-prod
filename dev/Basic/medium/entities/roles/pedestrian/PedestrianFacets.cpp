/*
 * PedestrainFacets.cpp
 *
 *  Created on: Mar 13, 2014
 *      Author: zhang huai peng
 */

#include "PedestrianFacets.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "geospatial/BusStop.hpp"
#include "Pedestrian.hpp"
#include "entities/params/PT_NetworkEntities.hpp"

namespace sim_mob {
namespace medium {

PedestrianBehavior::PedestrianBehavior(sim_mob::Person* parentAgent) :
		BehaviorFacet(parentAgent), parentPedestrian(nullptr) {

}

PedestrianBehavior::~PedestrianBehavior() {

}

PedestrianMovement::PedestrianMovement(sim_mob::Person* parentAgent,double speed) :
		MovementFacet(parentAgent), parentPedestrian(nullptr),
		remainingTimeToComplete(0), walkSpeed(speed),
		startLink(nullptr), totalTimeToCompleteSec(10){
}

PedestrianMovement::~PedestrianMovement() {}

void PedestrianMovement::setParentPedestrian(
		sim_mob::medium::Pedestrian* parentPedestrian) {
	this->parentPedestrian = parentPedestrian;
}


TravelMetric& PedestrianMovement::startTravelTimeMetric()
{
	return  travelMetric;
}

TravelMetric& PedestrianMovement::finalizeTravelTimeMetric()
{
	return  travelMetric;
}
void PedestrianBehavior::setParentPedestrian(
		sim_mob::medium::Pedestrian* parentPedestrian) {
	this->parentPedestrian = parentPedestrian;
}

void PedestrianMovement::frame_init() {
	std::vector<const RoadSegment*> roadSegs;
	initializePath(roadSegs);

	sim_mob::SubTrip& subTrip = *(parent->currSubTrip);

	// To differentiate the logic for pedestrian in public transit and rest pedestrian
	if(subTrip.isPT_Walk)
	{
		double walkTime = subTrip.walkTime;
		std::vector<const RoadSegment*>::const_iterator it = roadSegs.begin();
		if (roadSegs.size()>0)
		{
			startLink = roadSegs.back()->getLink();
			remainingTimeToComplete = 0.0;
			trajectory.push_back((std::make_pair(startLink, walkTime)));
			parentPedestrian->setTravelTime(walkTime*1000);
			return;
		}
	}
	const RoadSegment* segStart = nullptr;
	const RoadSegment* segEnd = nullptr;
	double actualDistanceStart = 0.0;
	double actualDistanceEnd = 0.0;

	if (subTrip.origin.type_ == WayPoint::BUS_STOP) {
		segStart = subTrip.origin.busStop_->getParentSegment();
		const Node* node = segStart->getEnd();
		const BusStop* stop = subTrip.origin.busStop_;
		DynamicVector EstimateDist(stop->xPos, stop->yPos, node->location.getX(),
				node->location.getY());
		actualDistanceStart = EstimateDist.getMagnitude();
	}

	if (subTrip.destination.type_ == WayPoint::BUS_STOP) {
		segEnd = subTrip.destination.busStop_->getParentSegment();
		const Node* node = segEnd->getStart();
		const BusStop* stop = subTrip.destination.busStop_;
		DynamicVector EstimateDist(stop->xPos, stop->yPos, node->location.getX(), node->location.getY());
		actualDistanceEnd = EstimateDist.getMagnitude();
	}

	Link* currentLink = nullptr;
	double distanceInLink = 0.0;
	double curDistance = 0.0;
	std::vector<const RoadSegment*>::const_iterator it = roadSegs.begin();
	if (it != roadSegs.end()) {
		currentLink = (*it)->getLink();
	}

	for (; it != roadSegs.end(); it++) {
		const RoadSegment* rdSeg = *it;
		curDistance = rdSeg->getPolylineLength();

		if (rdSeg->getLink() == currentLink) {
			distanceInLink += curDistance;
		} else {
			double remainingTime = distanceInLink / walkSpeed;
			trajectory.push_back((std::make_pair(currentLink, remainingTime)));
			currentLink = rdSeg->getLink();
			distanceInLink = curDistance;
			if(remainingTime<1.0){
				int ii=0;
			}
		}
	}

	if (currentLink) {
		double remainingTime = distanceInLink / walkSpeed;
		trajectory.push_back((std::make_pair(currentLink, remainingTime)));
	}

	if (trajectory.size() > 0 && !startLink) {
		remainingTimeToComplete = trajectory.front().second;
		startLink = trajectory.front().first;
		trajectory.erase(trajectory.begin());
		totalTimeToCompleteSec += remainingTimeToComplete;
		parentPedestrian->setTravelTime(totalTimeToCompleteSec*1000);
	}
}

const sim_mob::RoadSegment* PedestrianMovement::choiceNearestSegmentToMRT(
		const sim_mob::Node* src, const sim_mob::MRT_Stop* stop) {
	const RoadSegment* res = nullptr;
	std::vector<int> segs = stop->getRoadSegments();
	double minDis = std::numeric_limits<double>::max();
	for (std::vector<int>::iterator i = segs.begin(); i != segs.end(); i++) {
		unsigned int id = *i;
		const sim_mob::RoadSegment* segment = StreetDirectory::instance().getRoadSegment(id);
		const sim_mob::Node* node = segment->getStart();
		DynamicVector EstimateDist(src->getLocation().getX(),src->getLocation().getY(),
				node->getLocation().getX(),	node->getLocation().getY());
		double actualDistanceStart = EstimateDist.getMagnitude();
		if (minDis > actualDistanceStart) {
			minDis = actualDistanceStart;
			res = segment;
		}
	}

	return res;
}

void PedestrianMovement::initializePath(std::vector<const RoadSegment*>& path) {
	sim_mob::SubTrip& subTrip = *(parent->currSubTrip);
	const StreetDirectory& streetDirectory = StreetDirectory::instance();

	StreetDirectory::VertexDesc source, destination;
	std::vector<WayPoint> wayPoints;
	Point2D src(0,0), dest(0,0);
	if (subTrip.origin.type_ == WayPoint::NODE
			&& subTrip.destination.type_ == WayPoint::MRT_STOP) {
		source = streetDirectory.DrivingVertex(*subTrip.origin.node_);
		const RoadSegment* seg = choiceNearestSegmentToMRT(subTrip.origin.node_,subTrip.destination.mrtStop_);
		const Node* node = seg->getStart();
		destination = streetDirectory.DrivingVertex(*node);
		path.push_back(seg);
		dest.setX(node->location.getX());
		dest.setY(node->location.getY());
	} else if (subTrip.origin.type_ == WayPoint::MRT_STOP
			&& subTrip.destination.type_ == WayPoint::NODE) {
		destination = streetDirectory.DrivingVertex(*subTrip.destination.node_);
		const RoadSegment* seg = choiceNearestSegmentToMRT(subTrip.destination.node_,	subTrip.origin.mrtStop_);
		const Node* node = seg->getStart();
		startLink = seg->getLink();
		source = streetDirectory.DrivingVertex(*node);
		path.push_back(seg);
		src.setX(node->location.getX());
		src.setY(node->location.getY());
	} else if((subTrip.origin.type_ == WayPoint::BUS_STOP
			&& subTrip.destination.type_ == WayPoint::MRT_STOP) || (subTrip.origin.type_ == WayPoint::MRT_STOP
					&& subTrip.destination.type_ == WayPoint::BUS_STOP) ) {
		const Node* src_node = nullptr;
		const Node* dest_node = nullptr;
		if(subTrip.origin.type_ == WayPoint::BUS_STOP)
		{
			src_node = subTrip.origin.busStop_->getParentSegment()->getStart();
			source = streetDirectory.DrivingVertex(*src_node);
		}
		else
		{
			const RoadSegment* seg = choiceNearestSegmentToMRT(subTrip.destination.busStop_->getParentSegment()->getEnd(),subTrip.origin.mrtStop_);
			src_node = seg->getStart();
			path.push_back(seg);
			source = streetDirectory.DrivingVertex(*src_node);
		}
		if(src_node)
		{
			src.setX(src_node->location.getX());
			src.setY(src_node->location.getY());
		}
		if(subTrip.destination.type_ == WayPoint::BUS_STOP)
		{
			dest_node = subTrip.destination.busStop_->getParentSegment()->getEnd();
			destination = streetDirectory.DrivingVertex(*dest_node);
		}
		else
		{
			const RoadSegment* seg = choiceNearestSegmentToMRT(subTrip.origin.busStop_->getParentSegment()->getEnd(),subTrip.destination.mrtStop_);
			dest_node = seg->getStart();
			path.push_back(seg);
			destination = streetDirectory.DrivingVertex(*dest_node);
		}
		if(dest_node)
		{
			dest.setX(dest_node->location.getX());
			dest.setY(dest_node->location.getY());
		}
	}

	else {
		const Node* node = nullptr;
		if (subTrip.origin.type_ == WayPoint::NODE) {
			source = streetDirectory.DrivingVertex(*subTrip.origin.node_);
			node = subTrip.origin.node_;
		} else if (subTrip.origin.type_ == WayPoint::BUS_STOP) {
			node = subTrip.origin.busStop_->getParentSegment()->getStart();
			source = streetDirectory.DrivingVertex(*node);
		} else if(subTrip.origin.type_ == WayPoint::MRT_STOP) {
			std::vector<int> segs = subTrip.origin.mrtStop_->getRoadSegments();
			if(segs.size()>0){
				unsigned int id = segs.front();
				const sim_mob::RoadSegment* seg = StreetDirectory::instance().getRoadSegment(id);
				node = seg->getStart();
				startLink = seg->getLink();
				source = streetDirectory.DrivingVertex(*node);
				path.push_back(seg);
			}
		}
		if(node){
			src.setX(node->location.getX());
			src.setY(node->location.getY());
		}

		node = nullptr;
		if (subTrip.destination.type_ == WayPoint::NODE) {
			node = subTrip.destination.node_;
			destination = streetDirectory.DrivingVertex(
					*subTrip.destination.node_);
		} else if (subTrip.destination.type_ == WayPoint::BUS_STOP) {
			node = subTrip.destination.busStop_->getParentSegment()->getEnd();
			destination = streetDirectory.DrivingVertex(*node);
		}else if(subTrip.destination.type_ == WayPoint::MRT_STOP) {
			std::vector<int> segs = subTrip.destination.mrtStop_->getRoadSegments();
			if(segs.size()>0){
				unsigned int id = segs.front();
				const sim_mob::RoadSegment* seg = StreetDirectory::instance().getRoadSegment(id);
				node = seg->getStart();
				destination = streetDirectory.DrivingVertex(*node);
				path.push_back(seg);
			}
		}
		if(node){
			dest.setX(node->location.getX());
			dest.setY(node->location.getY());
		}
	}

	DynamicVector EstimateDist(src.getX(),src.getY(),dest.getX(),dest.getY());
	double distance = EstimateDist.getMagnitude();
	double remainingTime = distance / walkSpeed;
	parentPedestrian->setTravelTime(totalTimeToCompleteSec*1000);

	wayPoints = streetDirectory.SearchShortestDrivingPath(source, destination);
	for (std::vector<WayPoint>::iterator it = wayPoints.begin();
			it != wayPoints.end(); it++) {
		if (it->type_ == WayPoint::ROAD_SEGMENT) {
			path.push_back(it->roadSegment_);
		}
	}
}

void PedestrianMovement::frame_tick() {
	double tickSec = ConfigManager::GetInstance().FullConfig().baseGranSecond();
	if (remainingTimeToComplete <= tickSec) {
		double lastRemainingTime = tickSec - remainingTimeToComplete;
		if (trajectory.empty()) {
			parent->setNextLinkRequired(nullptr);
			parent->setToBeRemoved();
		}
		else {
			Link* nextLink = trajectory.front().first;
			remainingTimeToComplete = trajectory.front().second - lastRemainingTime;
			trajectory.erase(trajectory.begin());
			parent->setNextLinkRequired(nextLink);
			totalTimeToCompleteSec += remainingTimeToComplete;
			parentPedestrian->setTravelTime(totalTimeToCompleteSec*1000);
		}
	}
	else {
		remainingTimeToComplete -= tickSec;
	}
	parent->setRemainingTimeThisTick(0);
}

void PedestrianMovement::frame_tick_output() {}

sim_mob::Conflux* PedestrianMovement::getStartingConflux() const
{
	if (startLink) {
		return startLink->getSegments().front()->getParentConflux();
	} else {
		return nullptr;
	}
}

} /* namespace medium */
} /* namespace sim_mob */

