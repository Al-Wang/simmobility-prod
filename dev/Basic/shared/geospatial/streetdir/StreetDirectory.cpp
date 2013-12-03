//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "StreetDirectory.hpp"

#include <stdexcept>
#include <vector>
#include <iostream>
#include <limits>
#include <boost/unordered_map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include "geospatial/RoadRunnerRegion.hpp"

#include <cmath>

//TEMP
#include "geospatial/aimsun/Loader.hpp"

//TODO: Prune this include list later; it should be mostly moved out into the various Impl classes.
#include "geospatial/Lane.hpp"
#include "geospatial/Link.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/RoadNetwork.hpp"
#include "geospatial/Point2D.hpp"
#include "geospatial/Node.hpp"
#include "geospatial/LaneConnector.hpp"
#include "geospatial/BusStop.hpp"
#include "geospatial/Crossing.hpp"
#include "geospatial/ZebraCrossing.hpp"
#include "geospatial/UniNode.hpp"
#include "logging/Log.hpp"
#include "util/GeomHelpers.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "entities/signal/Signal.hpp"

#include "entities/TrafficWatch.hpp"
#include "A_StarShortestPathImpl.hpp"
#include "AStarShortestTravelTimePathImpl.hpp"
#include "GridStreetDirectoryImpl.hpp"


using std::map;
using std::vector;

using namespace sim_mob;


StreetDirectory sim_mob::StreetDirectory::instance_;


void sim_mob::StreetDirectory::init(const RoadNetwork& network, bool keepStats, centimeter_t gridWidth, centimeter_t gridHeight)
{
    if (keepStats) {
        //stats_ = new Stats;
    }
    pimpl_ = new GridStreetDirectoryImpl(network, gridWidth, gridHeight);
    spImpl_ = new A_StarShortestPathImpl(network);
    if(ConfigManager::GetInstance().FullConfig().PathSetMode())
    {
    	sttpImpl_ = new A_StarShortestTravelTimePathImpl(network,PathSetManager::getInstance()->getHighwayBias());
    }

    //Save a cache of Nodes to Links
	const std::vector<sim_mob::Link*>& links = network.getLinks();
	for (std::vector<sim_mob::Link*>::const_iterator it=links.begin(); it!=links.end(); it++) {
		//Just overwrite the saved value for that Node; this is why node_link_loc is an arbitrary field.
		node_link_loc_cache[(*it)->getStart()] = *it;
		node_link_loc_cache[(*it)->getEnd()] = *it;
	}

	//Save a cache of start,end Nodes to Links
	for (std::vector<sim_mob::Link*>::const_iterator it=links.begin(); it!=links.end(); it++) {
		//Just overwrite the saved value for that Node; this is why node_link_loc is an arbitrary field.
		links_by_node[std::make_pair((*it)->getStart(), (*it)->getEnd())] = *it;
	}



}

void sim_mob::StreetDirectory::updateDrivingMap()
{
	if(spImpl_) {
		spImpl_->updateEdgeProperty();
	}
}

std::pair<RoadRunnerRegion, bool> sim_mob::StreetDirectory::getRoadRunnerRegion(const sim_mob::RoadSegment* seg)
{
	return pimpl_ ? pimpl_->getRoadRunnerRegion(seg) : std::make_pair(RoadRunnerRegion(), false);
}

const sim_mob::BusStop* sim_mob::StreetDirectory::getBusStop(const Point2D& point) const
{
    return pimpl_ ? pimpl_->getBusStop(point) : nullptr;
}

const sim_mob::Node* sim_mob::StreetDirectory::getNode(const int id) const
{
	return pimpl_ ? pimpl_->getNode(id) : nullptr;
}

StreetDirectory::LaneAndIndexPair sim_mob::StreetDirectory::getLane(const Point2D& point) const
{
    return pimpl_ ? pimpl_->getLane(point) : LaneAndIndexPair();
}

std::vector<StreetDirectory::RoadSegmentAndIndexPair> sim_mob::StreetDirectory::closestRoadSegments(const Point2D& point, centimeter_t halfWidth, centimeter_t halfHeight) const
{
    return pimpl_ ? pimpl_->closestRoadSegments(point, halfWidth, halfHeight) : std::vector<RoadSegmentAndIndexPair>();
}

const MultiNode* sim_mob::StreetDirectory::GetCrossingNode(const Crossing* cross) const
{
	return pimpl_ ? pimpl_->GetCrossingNode(cross) : nullptr;
}

const Signal* sim_mob::StreetDirectory::signalAt(Node const & node) const
{

	map<const Node *, Signal const *>::const_iterator iter = signals_.find(&node);
    if (signals_.end() == iter) {
        return nullptr;
    }
    return iter->second;
}


StreetDirectory::VertexDesc sim_mob::StreetDirectory::DrivingVertex(const Node& n) const
{
	if (!spImpl_) { return StreetDirectory::VertexDesc(false); }

	return spImpl_->DrivingVertex(n);
}
StreetDirectory::VertexDesc sim_mob::StreetDirectory::DrivingTimeVertex(const sim_mob::Node& n,sim_mob::TimeRange tr,int random_graph_idx) const
{
	if (!sttpImpl_) { return StreetDirectory::VertexDesc(false); }

	if(tr == sim_mob::MorningPeak)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexMorningPeak(n);
	}
	else if(tr == sim_mob::EveningPeak)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexEveningPeak(n);
	}
	else if(tr == sim_mob::OffPeak)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexNormalTime(n);
	}
	else if(tr == sim_mob::Default)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexNormalTime(n);
	}
	else if(tr == sim_mob::HighwayBias_Distance)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexHighwayBiasDistance(n);
	}
	else if(tr == sim_mob::HighwayBias_MorningPeak)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexHighwayBiasMorningPeak(n);
	}
	else if(tr == sim_mob::HighwayBias_EveningPeak)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexHighwayBiasEveningPeak(n);
	}
	else if(tr == sim_mob::HighwayBias_OffPeak)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexHighwayBiasNormalTIme(n);
	}
	else if(tr == sim_mob::HighwayBias_Default)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexHighwayBiasDefault(n);
	}
	else if(tr == sim_mob::Random)
	{
		return ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->DrivingVertexRandom(n,random_graph_idx);
	}

	return StreetDirectory::VertexDesc(false);
}

StreetDirectory::VertexDesc sim_mob::StreetDirectory::WalkingVertex(const Node& n) const
{
	if (!spImpl_) { return StreetDirectory::VertexDesc(false); }

	return spImpl_->WalkingVertex(n);
}

StreetDirectory::VertexDesc sim_mob::StreetDirectory::DrivingVertex(const BusStop& b) const
{
	if (!spImpl_) { return StreetDirectory::VertexDesc(false); }

	return spImpl_->DrivingVertex(b);
}

StreetDirectory::VertexDesc sim_mob::StreetDirectory::WalkingVertex(const BusStop& b) const
{
	if (!spImpl_) { return StreetDirectory::VertexDesc(false); }

	return spImpl_->WalkingVertex(b);
}


vector<WayPoint> sim_mob::StreetDirectory::SearchShortestDrivingPath(VertexDesc from, VertexDesc to, std::vector<const sim_mob::RoadSegment*> blacklist) const
{
	if (!spImpl_) { return vector<WayPoint>(); }

	return spImpl_->GetShortestDrivingPath(from, to, blacklist);
}
std::vector<WayPoint> sim_mob::StreetDirectory::SearchShortestDrivingTimePath(VertexDesc from,
		VertexDesc to,
    		std::vector<const sim_mob::RoadSegment*> blacklist,
    		sim_mob::TimeRange tr,
    		int random_graph_idx) const
{
	if (!sttpImpl_) { return vector<WayPoint>(); }
	std::vector<WayPoint> res;
	res = ( (A_StarShortestTravelTimePathImpl *)sttpImpl_)->GetShortestDrivingPath(from, to, blacklist,tr,random_graph_idx);
	return res;
//	return sttpImpl_->GetShortestDrivingPath(from, to, blacklist);
}
vector<WayPoint> sim_mob::StreetDirectory::SearchShortestWalkingPath(VertexDesc from, VertexDesc to) const
{
	if (!spImpl_) { return vector<WayPoint>(); }

	return spImpl_->GetShortestWalkingPath(from, to);
}


void sim_mob::StreetDirectory::printStatistics() const
{
    //if (stats_) {
        //stats_->printStatistics();
    //} else {
        std::cout << "No statistics was collected by the StreetDirectory singleton." << std::endl;
    //}
}

void sim_mob::StreetDirectory::registerSignal(const Signal& signal)
{
    const Node* node = &(signal.getNode());

    if (signals_.count(node) == 0) {
        signals_.insert(std::make_pair(node, &signal));
//        std::cout << "Signal at node: " << node->getID() << " was added" << std::endl;
    }
}

void sim_mob::StreetDirectory::printDrivingGraph(std::ostream& outFile)
{
	if (spImpl_) {
		spImpl_->printDrivingGraph(outFile);
	}
}

void sim_mob::StreetDirectory::printWalkingGraph(std::ostream& outFile)
{
	if (spImpl_) {
		spImpl_->printWalkingGraph(outFile);
	}
}

const sim_mob::Link* sim_mob::StreetDirectory::getLinkLoc(const sim_mob::Node* node) const
{
	std::map<const sim_mob::Node*, const sim_mob::Link*>::const_iterator it = node_link_loc_cache.find(node);
	if (it!=node_link_loc_cache.end()) {
		return it->second;
	}
	return nullptr;
}

const sim_mob::Link* sim_mob::StreetDirectory::searchLink(const sim_mob::Node* start, const sim_mob::Node* end)
{
	if (!pimpl_) {
		throw std::runtime_error("Can't call searchLink; StreetDirectory has not been initialized yet.");
	}

	std::map< std::pair<const sim_mob::Node*, const sim_mob::Node*>, sim_mob::Link*>::iterator it = links_by_node.find(std::make_pair(start, end));
	if (it!=links_by_node.end()) {
		return it->second;
	}
	return nullptr;
}

const sim_mob::RoadSegment* sim_mob::StreetDirectory::getRoadSegment(const unsigned int id){

	if (!pimpl_) {
	//	throw std::runtime_error("Can't call searchLink; StreetDirectory has not been initialized yet.");
		return nullptr;
	}

	return pimpl_->getRoadSegment(id);
}

double sim_mob::StreetDirectory::GetShortestDistance(const Point2D& origin, const Point2D& p1, const Point2D& p2, const Point2D& p3, const Point2D& p4)
{
	double res = sim_mob::dist(origin, p1);
	res = std::min(res, sim_mob::dist(origin, p2));
	res = std::min(res, sim_mob::dist(origin, p3));
	res = std::min(res, sim_mob::dist(origin, p4));
	return res;
}

const MultiNode* sim_mob::StreetDirectory::FindNearestMultiNode(const RoadSegment* seg, const Crossing* cr)
{
	//Error case:
	const MultiNode* start = dynamic_cast<const MultiNode*>(seg->getStart());
	const MultiNode* end   = dynamic_cast<const MultiNode*>(seg->getEnd());
	if (!start && !end) {
		return nullptr;
	}

	//Easy case
	if (start && !end) {
		return start;
	}
	if (end && !start) {
		return end;
	}

	//Slightly harder case: compare distances.
	double dStart = GetShortestDistance(start->getLocation(), cr->nearLine.first, cr->nearLine.second, cr->farLine.first, cr->farLine.second);
	double dEnd   = GetShortestDistance(end->getLocation(),   cr->nearLine.first, cr->nearLine.second, cr->farLine.first, cr->farLine.second);
	return dStart < dEnd ? start : end;
}

