/* Copyright Singapore-MIT Alliance for Research and Technology */

#include <cmath>

#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random.hpp>

#include "geospatial/network/Lane.hpp"
#include "geospatial/network/Link.hpp"
#include "geospatial/network/RoadNetwork.hpp"
#include "geospatial/network/Node.hpp"
#include "geospatial/network/LaneConnector.hpp"
#include "geospatial/network/PT_Stop.hpp"
#include "geospatial/network/Node.hpp"
#include "geospatial/network/PolyLine.hpp"
#include "geospatial/network/TurningGroup.hpp"

#include "logging/Log.hpp"
#include "path/PathSetManager.hpp"
#include "conf/ConfigParams.hpp"
#include "conf/ConfigManager.hpp"
#include "A_StarShortestTravelTimePathImpl.hpp"

using std::map;
using std::set;
using std::vector;
using std::string;

using namespace sim_mob;

namespace sim_mob
{

namespace
{
	//create all time helpers objects outside loop
	const std::string MORNING_PEAK_START = "06:00:00";
	const std::string MORNING_PEAK_END = "10:00:00";
	const std::string EVENING_PEAK_START = "17:00:00";
	const std::string EVENING_PEAK_END = "20:00:00";
	const std::string OFF_PEAK1_START = "00:00:00";
	const std::string OFF_PEAK3_START = "24:00:00";

	const sim_mob::DailyTime morningPeakStartTime(MORNING_PEAK_START);
	const sim_mob::DailyTime morningPeakEndTime(MORNING_PEAK_END);
	const sim_mob::DailyTime eveningPeakStartTime(EVENING_PEAK_START);
	const sim_mob::DailyTime eveningPeakEndTime(EVENING_PEAK_END);
	const sim_mob::DailyTime offPeak1StartTime(OFF_PEAK1_START);
	const sim_mob::DailyTime offPeak3EndTime(OFF_PEAK3_START);
}


A_StarShortestTravelTimePathImpl::A_StarShortestTravelTimePathImpl(const sim_mob::RoadNetwork& network)
{
	//initialize random graph pool
	int randomCount = sim_mob::ConfigManager::GetInstance().FullConfig().getPathSetConf().perturbationIteration;
    for (int i = 0; i < randomCount; ++i)
    {
		drivingMapRandomPool.push_back(StreetDirectory::Graph());
		drivingNodeLookupRandomPool.push_back(NodeVertexLookup());
		drivingLinkLookupRandomPool.push_back(LinkEdgeLookup());
		drivingLinkVertexLookupRandomPool.push_back(LinkVertexLookup());
	}
	initDrivingNetwork(network);
}

void A_StarShortestTravelTimePathImpl::initDrivingNetwork(const sim_mob::RoadNetwork& network)
{
	const std::map<unsigned int, sim_mob::Link *>& links =	network.getMapOfIdVsLinks();

	//Various lookup structures
	NodeLookup nodeLookupMorningPeak;
	NodeLookup nodeLookupEveningPeak;
	NodeLookup nodeLookupNormalTime;
	NodeLookup nodeLookupDefault;
	NodeLookup nodeLookupHighwayBiasDistance;
	NodeLookup nodeLookupHighwayBiasMorningPeak;
	NodeLookup nodeLookupHighwayBiasEveningPeak;
	NodeLookup nodeLookupHighwayBiasNormalTime;
	NodeLookup nodeLookupHighwayBiasDefault;
	std::vector<NodeLookup> nodeLookupRandomPool;
	for (size_t i = 0; i < drivingMapRandomPool.size(); ++i) {
		nodeLookupRandomPool.push_back(NodeLookup());
	}

	//Add our initial set of vertices. Iterate through Links to ensure no un-used Node are added.
	std::map<unsigned int, sim_mob::Link *>::const_iterator iter;
	for (iter = links.begin(); iter != links.end(); ++iter) {
//		procAddDrivingNodes(drivingMapMorningPeak, iter->second, nodeLookupMorningPeak);
//		procAddDrivingNodes(drivingMapEveningPeak, iter->second, nodeLookupEveningPeak);
//		procAddDrivingNodes(drivingMapNormalTime, iter->second,	nodeLookupNormalTime);
		procAddDrivingNodes(drivingMapDefault, iter->second, nodeLookupDefault);
		procAddDrivingNodes(drivingMapHighwayBiasDistance, iter->second, nodeLookupHighwayBiasDistance);
//		procAddDrivingNodes(drivingMapHighwayBiasMorningPeak, iter->second, nodeLookupHighwayBiasMorningPeak);
//		procAddDrivingNodes(drivingMapHighwayBiasEveningPeak, iter->second, nodeLookupHighwayBiasEveningPeak);
//		procAddDrivingNodes(drivingMapHighwayBiasNormalTime, iter->second, nodeLookupHighwayBiasNormalTime);
		procAddDrivingNodes(drivingMapHighwayBiasDefault, iter->second,	nodeLookupHighwayBiasDefault);
		for (size_t i = 0; i < drivingMapRandomPool.size(); ++i) {
			procAddDrivingNodes(drivingMapRandomPool[i], iter->second, nodeLookupRandomPool[i]);
		}
	}

	//Proceed through our Links, adding each Link to the graph.
	for (iter = links.begin(); iter != links.end(); ++iter) {
//		procAddDrivingLinks(drivingMapMorningPeak, iter->second,nodeLookupMorningPeak, drivingLinkLookupMorningPeak,getEdgeWeight(iter->second,MorningPeak));
//		procAddDrivingLinks(drivingMapEveningPeak, iter->second,nodeLookupEveningPeak, drivingLinkLookupEveningPeak,getEdgeWeight(iter->second,EveningPeak));
//		procAddDrivingLinks(drivingMapNormalTime, iter->second,	nodeLookupNormalTime, drivingLinkLookupNormalTime,getEdgeWeight(iter->second,OffPeak));
		procAddDrivingLinks(drivingMapDefault, iter->second, nodeLookupDefault,	drivingLinkLookupDefault, drivingLinkVertexLookupDefault, getEdgeWeight(iter->second,Default));
		procAddDrivingLinks(drivingMapHighwayBiasDistance, iter->second,nodeLookupHighwayBiasDistance,drivingLinkLookupHighwayBiasDistance,	drivingLinkVertexLookupHighwayBiasDistance, getEdgeWeight(iter->second,HighwayBiasDistance));
//		procAddDrivingLinks(drivingMapHighwayBiasMorningPeak, iter->second,	nodeLookupHighwayBiasMorningPeak,drivingLinkLookupHighwayBiasMorningPeak,getEdgeWeight(iter->second,HighwayBiasMorningPeak));
//		procAddDrivingLinks(drivingMapHighwayBiasEveningPeak, iter->second,	nodeLookupHighwayBiasEveningPeak,drivingLinkLookupHighwayBiasEveningPeak,getEdgeWeight(iter->second,HighwayBiasEveningPeak));
//		procAddDrivingLinks(drivingMapHighwayBiasNormalTime, iter->second, nodeLookupHighwayBiasNormalTime,	drivingLinkLookupHighwayBiasNormalTime,getEdgeWeight(iter->second,HighwayBiasOffPeak));
		procAddDrivingLinks(drivingMapHighwayBiasDefault, iter->second,	nodeLookupHighwayBiasDefault,drivingLinkLookupHighwayBiasDefault, drivingLinkVertexLookupHighwayBiasDefault, getEdgeWeight(iter->second,HighwayBiasDefault));
		for (size_t i = 0; i < drivingMapRandomPool.size(); ++i) {
		procAddDrivingLinks(drivingMapRandomPool[i], iter->second,nodeLookupRandomPool[i], drivingLinkLookupRandomPool[i], drivingLinkVertexLookupRandomPool[i], getEdgeWeight(iter->second,Random));
		}
	}

	//Now add all Intersection edges (link connectors)
	for (NodeLookup::const_iterator it = nodeLookupDefault.begin();	it != nodeLookupDefault.end(); it++) {
//		procAddDrivingLinkConnectors(drivingMapMorningPeak, (it->first),nodeLookupMorningPeak);
//		procAddDrivingLinkConnectors(drivingMapEveningPeak, (it->first),nodeLookupEveningPeak);
//		procAddDrivingLinkConnectors(drivingMapNormalTime, (it->first),nodeLookupNormalTime);
		procAddDrivingLinkConnectors(drivingMapDefault, (it->first),nodeLookupDefault);
		procAddDrivingLinkConnectors(drivingMapHighwayBiasDistance, (it->first),nodeLookupHighwayBiasDistance);
//		procAddDrivingLinkConnectors(drivingMapHighwayBiasMorningPeak, (it->first), nodeLookupHighwayBiasMorningPeak);
//		procAddDrivingLinkConnectors(drivingMapHighwayBiasEveningPeak, (it->first), nodeLookupHighwayBiasEveningPeak);
//		procAddDrivingLinkConnectors(drivingMapHighwayBiasNormalTime, (it->first), nodeLookupHighwayBiasNormalTime);
		procAddDrivingLinkConnectors(drivingMapHighwayBiasDefault, (it->first),	nodeLookupHighwayBiasDefault);
		for (size_t i = 0; i < drivingMapRandomPool.size(); ++i) {
			procAddDrivingLinkConnectors(drivingMapRandomPool[i], (it->first), nodeLookupRandomPool[i]);
		}
	}

//	procAddStartNodesAndEdges(drivingMapMorningPeak, nodeLookupMorningPeak,	&drivingNodeLookupMorningPeak);
//	procAddStartNodesAndEdges(drivingMapEveningPeak, nodeLookupEveningPeak,	&drivingNodeLookupEveningPeak);
//	procAddStartNodesAndEdges(drivingMapNormalTime, nodeLookupNormalTime, &drivingNodeLookupNormalTime);
	procAddStartNodesAndEdges(drivingMapDefault, nodeLookupDefault,	&drivingNodeLookupDefault);
	procAddStartNodesAndEdges(drivingMapHighwayBiasDistance, nodeLookupHighwayBiasDistance,	&drivingNodeLookupHighwayBiasDistance);
//	procAddStartNodesAndEdges(drivingMapHighwayBiasMorningPeak,	nodeLookupHighwayBiasMorningPeak, &drivingNodeLookupHighwayBiasMorningPeak);
//	procAddStartNodesAndEdges(drivingMapHighwayBiasEveningPeak,	nodeLookupHighwayBiasEveningPeak, &drivingNodeLookupHighwayBiasEveningPeak);
//	procAddStartNodesAndEdges(drivingMapHighwayBiasNormalTime, nodeLookupHighwayBiasNormalTime,	&drivingNodeLookupHighwayBiasNormalTime);
	procAddStartNodesAndEdges(drivingMapHighwayBiasDefault,	nodeLookupHighwayBiasDefault, &drivingNodeLookupHighwayBiasDefault);
	for (size_t i = 0; i < drivingMapRandomPool.size(); ++i) {
		procAddStartNodesAndEdges(drivingMapRandomPool[i], nodeLookupRandomPool[i], &drivingNodeLookupRandomPool[i]);
	}
}

double A_StarShortestTravelTimePathImpl::getEdgeWeight(const sim_mob::Link* link, sim_mob::TimeRange timeRange)
{
	const TravelTimeManager* ttMgr = TravelTimeManager::getInstance();
	double highwayBias = PathSetParam::getInstance()->getHighwayBias();
	double edgeWeight = std::numeric_limits<double>::max();
	switch (timeRange)
	{
//	case sim_mob::MorningPeak:
//	{
//		edgeWeight = sim_mob::PathSetParam::getInstance()->getSegRangeTT(rs, "Car", morningPeakStartTime, morningPeakEndTime);
//		break;
//	}
//	case sim_mob::EveningPeak:
//	{
//		edgeWeight = PathSetParam::getInstance()->getSegRangeTT(rs, "Car", eveningPeakStartTime, eveningPeakEndTime);
//		break;
//	}
//	case sim_mob::OffPeak:
//	{
//		double key1 = PathSetParam::getInstance()->getSegRangeTT(rs, "Car", offPeak1StartTime, morningPeakStartTime); //Off-peak 1
//		double key2 = PathSetParam::getInstance()->getSegRangeTT(rs, "Car", morningPeakEndTime, eveningPeakStartTime); //Off-peak 2
//		double key3 = PathSetParam::getInstance()->getSegRangeTT(rs, "Car", eveningPeakEndTime, offPeak3EndTime); //Off-peak 3
//		edgeWeight = (key1 + key2 + key3) / 3.0;
//		break;
//	}
	case sim_mob::Default:
	{
		edgeWeight = ttMgr->getDefaultLinkTT(link);
		break;
	}
	case sim_mob::HighwayBiasDistance:
	{
		edgeWeight = link->getLength();
		if (link->getLinkType() != LinkType::LINK_TYPE_EXPRESSWAY)
		{
			edgeWeight = edgeWeight / highwayBias;//if not highway, increase edge weight
		}
		break;
	}
//	case sim_mob::HighwayBiasMorningPeak:
//	{
//		edgeWeight = sim_mob::PathSetParam::getInstance()->getSegRangeTT(rs, "Car", morningPeakStartTime, morningPeakEndTime);
//		if (!link->getLinkType() != LinkType::LINK_TYPE_EXPRESSWAY)
//		{
//			edgeWeight = edgeWeight / highwayBias;
//		} //if not highway, increase edge weight
//		break;
//	}
//	case sim_mob::HighwayBiasEveningPeak:
//	{
//		edgeWeight = PathSetParam::getInstance()->getSegRangeTT(rs, "Car", eveningPeakStartTime, eveningPeakEndTime);
//		if (!link->getLinkType() != LinkType::LINK_TYPE_EXPRESSWAY)
//		{
//			edgeWeight = edgeWeight / highwayBias;
//		} //if not highway, increase edge weight
//		break;
//	}
//	case sim_mob::HighwayBiasOffPeak:
//	{
//		double key1 = PathSetParam::getInstance()->getSegRangeTT(rs, "Car", offPeak1StartTime, morningPeakStartTime); //Off-peak 1
//		double key2 = PathSetParam::getInstance()->getSegRangeTT(rs, "Car", morningPeakEndTime, eveningPeakStartTime); //Off-peak 2
//		double key3 = PathSetParam::getInstance()->getSegRangeTT(rs, "Car", eveningPeakEndTime, offPeak3EndTime); //Off-peak 3
//		edgeWeight = (key1 + key2 + key3) / 3.0;
//		if (link->getLinkType() != LinkType::LINK_TYPE_EXPRESSWAY)
//		{
//			edgeWeight = edgeWeight / highwayBias;
//		} //if not highway, increase edge weight
//		break;
//	}
	case sim_mob::HighwayBiasDefault:
	{
		edgeWeight = ttMgr->getDefaultLinkTT(link);
		if (link->getLinkType() != LinkType::LINK_TYPE_EXPRESSWAY)
		{
			edgeWeight = edgeWeight / highwayBias;
		} //if not highway, increase edge weight
		break;
	}
	case sim_mob::Random:
	{
		boost::random_device seedGen;
		boost::mt19937 rng(seedGen());
		const std::pair<int, int> &range = sim_mob::ConfigManager::GetInstance().FullConfig().getPathSetConf().perturbationRange;
		boost::uniform_int<> uniformInt(range.first, range.second);
		boost::variate_generator<boost::mt19937, boost::uniform_int<> > rollDice(rng, uniformInt);
		edgeWeight = rollDice() * ttMgr->getDefaultLinkTT(link); //randomNo * defTT
		if (edgeWeight <= 0)
		{
			std::stringstream out("");
			out << "Invalid random perturbation key for segment " << link->getLinkId();
			throw std::runtime_error(out.str());
		}
		break;
	}
	default:
	{
		throw std::runtime_error("edge weight requested for unsupported graph type");
	}
	}
	return edgeWeight;
}

StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertex(const sim_mob::Node& node, TimeRange timeRange, int randomGraphIdx) const
{
	StreetDirectory::VertexDesc res;
	switch (timeRange)
    	{
    	case Default:
        	res = DrivingVertexDefault(node);
        	break;
    	case HighwayBiasDefault:
        	res = DrivingVertexHighwayBiasDefault(node);
        	break;
    	case HighwayBiasDistance:
        	res = DrivingVertexHighwayBiasDistance(node);
        	break;
    	case Random:
        	res = DrivingVertexRandom(node, randomGraphIdx);
        	break;
    	}
    	return res;
}
StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertex(const sim_mob::Link& link, TimeRange timeRange, int randomGraphIdx) const
{
	StreetDirectory::VertexDesc res;
	switch (timeRange)
	{
		case Default:
	        res = DrivingVertexDefault(link);
        	break;
    	case HighwayBiasDefault:
        	res = DrivingVertexHighwayBiasDefault(link);
        	break;
    	case HighwayBiasDistance:
        	res = DrivingVertexHighwayBiasDistance(link);
        	break;
    	case Random:
       		res = DrivingVertexRandom(link, randomGraphIdx);
        	break;
    	}
    	return res;
}
//StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexMorningPeak(const sim_mob::Node& n) const
//{
//	StreetDirectory::VertexDesc res;
//	NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupMorningPeak.find(&n);
//	if (vertexIt != drivingNodeLookupMorningPeak.end()) {
//		res.valid = true;
//		res.source = vertexIt->second.first;
//		res.sink = vertexIt->second.second;
//		return res;
//	}
//	return res;
//}
//StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexEveningPeak(const sim_mob::Node& n) const
//{
//	StreetDirectory::VertexDesc res;
//	NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupEveningPeak.find(&n);
//	if (vertexIt != drivingNodeLookupEveningPeak.end()) {
//		res.valid = true;
//		res.source = vertexIt->second.first;
//		res.sink = vertexIt->second.second;
//		return res;
//	}
//	return res;
//}
//StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexNormalTime(const sim_mob::Node& n) const
//{
//	StreetDirectory::VertexDesc res;
//	NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupNormalTime.find(&n);
//	if (vertexIt != drivingNodeLookupNormalTime.end()) {
//		res.valid = true;
//		res.source = vertexIt->second.first;
//		res.sink = vertexIt->second.second;
//		return res;
//	}
//	return res;
//}
StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexDefault(const sim_mob::Node& n) const
{
	StreetDirectory::VertexDesc res;
	NodeVertexLookup::const_iterator vertexIt = drivingNodeLookupDefault.find(&n);
    	if (vertexIt != drivingNodeLookupDefault.end())
    	{
		res.valid = true;
		res.source = vertexIt->second.first;
		res.sink = vertexIt->second.second;
		return res;
	}
    	return res;
}

StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexDefault(const Link &link) const
{
    	StreetDirectory::VertexDesc res;
    	LinkVertexLookup::const_iterator vertexIt = drivingLinkVertexLookupDefault.find(&link);
    	if (vertexIt != drivingLinkVertexLookupDefault.end())
    	{
        	res.valid = true;
        	res.source = vertexIt->second.first;
        	res.sink = vertexIt->second.second;
        	return res;
    	}
    	return res;
}

StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexHighwayBiasDistance(const sim_mob::Node& n) const
{
	StreetDirectory::VertexDesc res;
	NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupHighwayBiasDistance.find(&n);
    	if (vertexIt != drivingNodeLookupHighwayBiasDistance.end())
    	{
		res.valid = true;
		res.source = vertexIt->second.first;
		res.sink = vertexIt->second.second;
		return res;
	}
    	return res;
}

StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexHighwayBiasDistance(const Link &link) const
{
    	StreetDirectory::VertexDesc res;
    	LinkVertexLookup::const_iterator vertexIt = drivingLinkVertexLookupHighwayBiasDistance.find(&link);
    	if (vertexIt != drivingLinkVertexLookupHighwayBiasDistance.end())
    	{
        	res.valid = true;
        	res.source = vertexIt->second.first;
        	res.sink = vertexIt->second.second;
        	return res;
    	}
    	return res;
}
//StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexHighwayBiasMorningPeak(const sim_mob::Node& n) const
//{
//	StreetDirectory::VertexDesc res;
//	NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupHighwayBiasMorningPeak.find(&n);
//	if (vertexIt != drivingNodeLookupHighwayBiasMorningPeak.end()) {
//		res.valid = true;
//		res.source = vertexIt->second.first;
//		res.sink = vertexIt->second.second;
//		return res;
//	}
//
//	return res;
//}
//StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexHighwayBiasEveningPeak(const sim_mob::Node& n) const
//{
//	StreetDirectory::VertexDesc res;
//	NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupHighwayBiasEveningPeak.find(&n);
//	if (vertexIt != drivingNodeLookupHighwayBiasEveningPeak.end()) {
//		res.valid = true;
//		res.source = vertexIt->second.first;
//		res.sink = vertexIt->second.second;
//		return res;
//	}
//
//	return res;
//}
//StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexHighwayBiasNormalTIme(const sim_mob::Node& n) const
//{
//	StreetDirectory::VertexDesc res;
//	NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupHighwayBiasNormalTime.find(&n);
//	if (vertexIt != drivingNodeLookupHighwayBiasNormalTime.end()) {
//		res.valid = true;
//		res.source = vertexIt->second.first;
//		res.sink = vertexIt->second.second;
//		return res;
//	}
//	return res;
//}
StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexHighwayBiasDefault(const sim_mob::Node& n) const
{
	StreetDirectory::VertexDesc res;
	NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupHighwayBiasDefault.find(&n);
    	if (vertexIt != drivingNodeLookupHighwayBiasDefault.end())
    	{
		res.valid = true;
		res.source = vertexIt->second.first;
		res.sink = vertexIt->second.second;
		return res;
	}
    	return res;
}

StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexHighwayBiasDefault(const sim_mob::Link &link) const
{
    	StreetDirectory::VertexDesc res;
    	LinkVertexLookup::const_iterator vertexIt =	drivingLinkVertexLookupHighwayBiasDefault.find(&link);
    	if (vertexIt != drivingLinkVertexLookupHighwayBiasDefault.end())
    	{
        	res.valid = true;
        	res.source = vertexIt->second.first;
        	res.sink = vertexIt->second.second;
        	return res;
    	}
    	return res;
}

StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexRandom(const sim_mob::Node& node, unsigned int randomGraphId) const
{
	StreetDirectory::VertexDesc res;
	if (randomGraphId < drivingNodeLookupRandomPool.size()) {
		NodeVertexLookup::const_iterator vertexIt =	drivingNodeLookupRandomPool[randomGraphId].find(&node);
        if (vertexIt != drivingNodeLookupRandomPool[randomGraphId].end())
        {
			res.valid = true;
			res.source = vertexIt->second.first;
			res.sink = vertexIt->second.second;
			return res;
		}
	}
    	return res;
}

StreetDirectory::VertexDesc A_StarShortestTravelTimePathImpl::DrivingVertexRandom(const sim_mob::Link &link, unsigned int randomGraphId) const
{
    	StreetDirectory::VertexDesc res;
    	if (randomGraphId < drivingLinkVertexLookupRandomPool.size())
    	{
        	LinkVertexLookup::const_iterator vertexIt =	drivingLinkVertexLookupRandomPool[randomGraphId].find(&link);
        	if (vertexIt != drivingLinkVertexLookupRandomPool[randomGraphId].end())
        	{
            		res.valid = true;
            		res.source = vertexIt->second.first;
            		res.sink = vertexIt->second.second;
            		return res;
        	}
    	}
   	return res;
}

std::vector<WayPoint> A_StarShortestTravelTimePathImpl::searchShortestTTPathWithBlackList(const StreetDirectory::Graph& graph, const StreetDirectory::Vertex& fromVertex,
																			  const StreetDirectory::Vertex& toVertex,
																			  const std::set<StreetDirectory::Edge>& blacklist)
{
	//Lock for read access.
	boost::shared_lock<boost::shared_mutex> lock(GraphSearchMutex);

	//Filter it.
	BlackListEdgeConstraint filter(blacklist);
	boost::filtered_graph<StreetDirectory::Graph, BlackListEdgeConstraint> filtered(graph, filter);

	vector<WayPoint> res;
	std::list<StreetDirectory::Vertex> partialRes;
	//Output variable for A* searching
	vector<StreetDirectory::Vertex> p(boost::num_vertices(filtered));
	//Output variable for A* searching
	vector<double> d(boost::num_vertices(filtered));

	//Use A* to search for a path
	try
	{
		boost::astar_search(filtered, fromVertex, A_StarShortestTravelTimePathImpl::DistanceHeuristicFiltered(&filtered, toVertex),
							boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(GoalVisitor(toVertex)));
	}
	catch (Goal& goal)
	{
		//Build backwards.
		for (StreetDirectory::Vertex v = toVertex;; v = p[v])
		{
			partialRes.push_front(v);
			if (p[v] == v)
			{
				break;
			}
		}

		//Now build forwards.
		std::list<StreetDirectory::Vertex>::const_iterator prev = partialRes.end();
		for (std::list<StreetDirectory::Vertex>::const_iterator it = partialRes.begin(); it != partialRes.end(); it++)
		{
			//Add this edge.
			if (prev != partialRes.end())
			{
				//This shouldn't fail.
				std::pair < StreetDirectory::Edge, bool> edge = boost::edge(*prev, *it, filtered);
				if (!edge.second)
				{
					Warn() << "ERROR: Boost can't find an edge that it should know about."
							<< std::endl;
					return std::vector<WayPoint>();
				}

				//Retrieve, add this edge's WayPoint.
				WayPoint wp = boost::get(boost::edge_name, filtered, edge.first);
				res.push_back(wp);
			}

			//Save for later.
			prev = it;
		}
	}

	return res;
}

vector<WayPoint> A_StarShortestTravelTimePathImpl::searchShortestTTPath(const StreetDirectory::Graph& graph, const StreetDirectory::Vertex& fromVertex,
															const StreetDirectory::Vertex& toVertex)
{
	vector<WayPoint> res;
	std::list<StreetDirectory::Vertex> partialRes;

	//Lock for read access.
	boost::shared_lock<boost::shared_mutex> lock(GraphSearchMutex);
	//Output variable for A* searching
	vector<StreetDirectory::Vertex> p(boost::num_vertices(graph));
	//Output variable for A* searching
	vector<double> d(boost::num_vertices(graph));

	//Use A* to search for a path
	try
	{
		boost::astar_search(graph, fromVertex, A_StarShortestTravelTimePathImpl::DistanceHeuristicGraph(&graph, toVertex),
							boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(GoalVisitor(toVertex)));
	}
	catch (Goal& goal)
	{
		//Build backwards.
		for (StreetDirectory::Vertex v = toVertex;; v = p[v])
		{
			partialRes.push_front(v);
			if (p[v] == v)
			{
				break;
			}
		}

		//Now build forwards.
		std::list<StreetDirectory::Vertex>::const_iterator prev = partialRes.end();
		for (std::list<StreetDirectory::Vertex>::const_iterator it = partialRes.begin(); it != partialRes.end(); it++)
		{
			//Add this edge.
			if (prev != partialRes.end())
			{
				//This shouldn't fail.
				std::pair < StreetDirectory::Edge, bool> edge = boost::edge(*prev, *it, graph);
				if (!edge.second)
				{
					Warn() << "ERROR: Boost can't find an edge that it should know about."
							<< std::endl;
					return std::vector<WayPoint>();
				}

				//Retrieve, add this edge WayPoint.
				WayPoint wp = boost::get(boost::edge_name, graph, edge.first);
				res.push_back(wp);
			}

			//Save for later.
			prev = it;
		}
	}

	return res;
}

vector<sim_mob::WayPoint> A_StarShortestTravelTimePathImpl::GetShortestDrivingPath(
        const StreetDirectory::VertexDesc& from, const StreetDirectory::VertexDesc& to,
		const vector<const sim_mob::Link*>& blacklist, TimeRange tmRange,int randomGraphId) const
{
	if (!(from.valid && to.valid))
	{
		return vector<sim_mob::WayPoint>();
	}

	StreetDirectory::Vertex fromV = from.source;
	StreetDirectory::Vertex toV = to.sink;
	if (fromV == toV)
	{
		return vector<sim_mob::WayPoint>();
	}

	//Convert the blacklist into a list of blocked Vertices.
	set<StreetDirectory::Edge> blacklistV;
	for (vector<const sim_mob::Link*>::const_iterator it = blacklist.begin(); it != blacklist.end(); it++)
	{
//		if (tmRange == MorningPeak)
//		{
//			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupMorningPeak.find(*it);
//			if (lookIt != drivingLinkLookupMorningPeak.end())
//			{
//				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
//			}
//		}
//		else if (tmRange == EveningPeak)
//		{
//			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupEveningPeak.find(*it);
//			if (lookIt != drivingLinkLookupEveningPeak.end())
//			{
//				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
//			}
//		}
//		else if (tmRange == OffPeak)
//		{
//			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupNormalTime.find(*it);
//			if (lookIt != drivingLinkLookupNormalTime.end())
//			{
//				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
//			}
//		}
//		else
		if (tmRange == Default)
		{
			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupDefault.find(*it);
			if (lookIt != drivingLinkLookupDefault.end())
			{
				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
			}
		}
		else if (tmRange == HighwayBiasDistance)
		{
			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupHighwayBiasDistance.find(*it);
			if (lookIt != drivingLinkLookupHighwayBiasDistance.end())
			{
				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
			}
		}
		else
//		if (tmRange == HighwayBiasMorningPeak)
//		{
//			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupHighwayBiasMorningPeak.find(*it);
//			if (lookIt != drivingLinkLookupHighwayBiasMorningPeak.end())
//			{
//				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
//			}
//		}
//		else if (tmRange == HighwayBiasEveningPeak)
//		{
//			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupHighwayBiasEveningPeak.find(*it);
//			if (lookIt != drivingLinkLookupHighwayBiasEveningPeak.end())
//			{
//				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
//			}
//		}
//		else if (tmRange == HighwayBiasOffPeak)
//		{
//			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupHighwayBiasNormalTime.find(*it);
//			if (lookIt != drivingLinkLookupHighwayBiasNormalTime.end())
//			{
//				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
//			}
//		}
//		else
		if (tmRange == HighwayBiasDefault)
		{
			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupHighwayBiasDefault.find(*it);
			if (lookIt != drivingLinkLookupHighwayBiasDefault.end())
			{
				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
			}
		}
		else if (tmRange == Random)
		{
			if (randomGraphId >= drivingMapRandomPool.size())
			{
				randomGraphId = 0;
			}
			if (drivingMapRandomPool.size() == 0)
			{
				return vector<sim_mob::WayPoint>();
			}
			LinkEdgeLookup::const_iterator lookIt = drivingLinkLookupRandomPool[randomGraphId].find(*it);
			if (lookIt != drivingLinkLookupRandomPool[randomGraphId].end())
			{
				blacklistV.insert(lookIt->second.begin(), lookIt->second.end());
			}
		}
	}

	if (blacklistV.empty())
	{
//		if (tmRange == MorningPeak)
//		{
//			return searchShortestPath(drivingMapMorningPeak, fromV, toV);
//		}
//		else if (tmRange == EveningPeak)
//		{
//			return searchShortestPath(drivingMapEveningPeak, fromV, toV);
//		}
//		else if (tmRange == OffPeak)
//		{
//			return searchShortestPath(drivingMapNormalTime, fromV, toV);
//		}
//		else
		if (tmRange == Default)
		{
			return searchShortestTTPath(drivingMapDefault, fromV, toV);
		}
		else if (tmRange == HighwayBiasDistance)
		{
			return searchShortestTTPath(drivingMapHighwayBiasDistance, fromV, toV);
		}
		else
//		if (tmRange == HighwayBiasMorningPeak)
//		{
//			return searchShortestPath(drivingMapHighwayBiasMorningPeak, fromV, toV);
//		}
//		else if (tmRange == HighwayBiasEveningPeak)
//		{
//			return searchShortestPath(drivingMapHighwayBiasEveningPeak, fromV, toV);
//		}
//		else if (tmRange == HighwayBiasOffPeak)
//		{
//			return searchShortestPath(drivingMapHighwayBiasNormalTime, fromV, toV);
//		}
//		else
		if (tmRange == HighwayBiasDefault)
		{
			return searchShortestTTPath(drivingMapHighwayBiasDefault, fromV, toV);
		}
		else if (tmRange == Random)
		{
			if (randomGraphId >= drivingMapRandomPool.size())
			{
				return vector<sim_mob::WayPoint>();
			}
			return searchShortestTTPath(drivingMapRandomPool[randomGraphId], fromV, toV);
		}
		else
		{
			throw std::runtime_error("A_StarShortestTravelTimePathImpl: unknown time range");
		}
	}
	else
	{
//		if (tmRange == MorningPeak)
//		{
//			return searchShortestPathWithBlackList(drivingMapMorningPeak, fromV, toV, blacklistV);
//		}
//		else if (tmRange == EveningPeak)
//		{
//			return searchShortestPathWithBlackList(drivingMapEveningPeak, fromV, toV, blacklistV);
//		}
//		else if (tmRange == OffPeak)
//		{
//			return searchShortestPathWithBlackList(drivingMapNormalTime, fromV, toV, blacklistV);
//		}
//		else
		if (tmRange == Default)
		{
			return searchShortestTTPathWithBlackList(drivingMapDefault, fromV, toV, blacklistV);
		}
		else if (tmRange == HighwayBiasDistance)
		{
			return searchShortestTTPathWithBlackList(drivingMapHighwayBiasDistance, fromV, toV, blacklistV);
		}
		else
//		if (tmRange == HighwayBiasMorningPeak)
//		{
//			return searchShortestPathWithBlackList(drivingMapHighwayBiasMorningPeak, fromV, toV, blacklistV);
//		}
//		else if (tmRange == HighwayBiasEveningPeak)
//		{
//			return searchShortestPathWithBlackList(drivingMapHighwayBiasEveningPeak, fromV, toV, blacklistV);
//		}
//		else if (tmRange == HighwayBiasOffPeak)
//		{
//			return searchShortestPathWithBlackList(drivingMapHighwayBiasNormalTime, fromV, toV, blacklistV);
//		}
//		else
		if (tmRange == HighwayBiasDefault)
		{
			return searchShortestTTPathWithBlackList(drivingMapHighwayBiasDefault, fromV, toV, blacklistV);
		}
		else if (tmRange == Random)
		{
			if (randomGraphId >= drivingMapRandomPool.size())
			{
				return vector<sim_mob::WayPoint>();
			}
			return searchShortestTTPathWithBlackList(drivingMapRandomPool[randomGraphId], fromV, toV, blacklistV);
		}
		else
		{
			throw std::runtime_error("A_StarShortestTravelTimePathImpl: unknown time range2");
		}
	}
}
}





