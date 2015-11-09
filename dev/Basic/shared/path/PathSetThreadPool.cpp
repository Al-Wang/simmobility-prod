/**
 * PathSetThreadPool.cpp
 *
 *  Created on: Feb 18, 2014
 *      Author: redheli
 */


#include "PathSetThreadPool.hpp"
#include "Path.hpp"
#include "geospatial/streetdir/A_StarShortestTravelTimePathImpl.hpp"
#include "geospatial/streetdir/A_StarShortestPathImpl.hpp"

#include <pthread.h>
#include <semaphore.h>
#include <cstdlib>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/astar_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <iterator>
#include <algorithm>
#include <vector>

using namespace std;

namespace
{
	//sim_mob::BasicLogger & logger = sim_mob::Logger::log("pathset.log");
}

sim_mob::PathSetWorkerThread::PathSetWorkerThread() :
		path(NULL), linkLookup(NULL), fromNode(NULL), toNode(NULL), hasPath(false), timeBased(false), dbgStr(std::string()), graph(NULL)
{
}

sim_mob::PathSetWorkerThread::~PathSetWorkerThread() { }

//1.Create Blacklist
//2.clear the shortestWayPointpath
//3.Populate a vector<WayPoint> with a blacklist involved
//  or
//  Populate a vector<WayPoint> without blacklist involvement
//4.populate a singlepath instance
void sim_mob::PathSetWorkerThread::run()
{
	//Convert the blacklist into a list of blocked Vertices.
	std::set<StreetDirectory::Edge> blacklistEdges;
	std::map<const Link*, std::set<StreetDirectory::Edge> >::const_iterator lookIt;
	for(std::set<const Link*>::iterator it=excludedLinks.begin(); it!=excludedLinks.end(); it++)
	{
		lookIt = linkLookup->find(*it);
		if(lookIt != linkLookup->end())
		{
			blacklistEdges.insert(lookIt->second.begin(), lookIt->second.end());
		}
	}
	//used for error checking and validation
	unsigned long dbgPrev = 0;
	std::pair<StreetDirectory::Edge, bool> dbgPrevEdge;
	//output container
	vector<WayPoint> wps;

	if (blacklistEdges.empty())
	{
		std::list<StreetDirectory::Vertex> partialRes;
		//Use A* to search for a path
		//Taken from: http://www.boost.org/doc/libs/1_38_0/libs/graph/example/astar-cities.cpp
		vector<StreetDirectory::Vertex> p(boost::num_vertices(*graph)); //Output variable
		vector<double> d(boost::num_vertices(*graph));  //Output variable
		try
		{
			boost::astar_search(*graph, fromVertex, sim_mob::A_StarShortestTravelTimePathImpl::DistanceHeuristicGraph(graph, toVertex),
					boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(sim_mob::A_StarShortestTravelTimePathImpl::GoalVisitor(toVertex)));
		}
		catch (sim_mob::A_StarShortestTravelTimePathImpl::Goal& goal)
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
					std::pair<StreetDirectory::Edge, bool> edge = boost::edge(*prev, *it, *graph);
					if (!edge.second) {
						Warn()
								<< "ERROR: Boost can't find an edge that it should know about."
								<< std::endl;
					}
					//Retrieve, add this edge's WayPoint.
					WayPoint wp = boost::get(boost::edge_name, *graph,edge.first);
					//todo this problem occurs during "highway bias distance" generation. dont know why, discarding the repeated segment
					if (wp.type == WayPoint::ROAD_SEGMENT && wp.roadSegment->getRoadSegmentId() == dbgPrev)
					{
//							logger << dbgStr
//									<< " 1ERROR-exeThis:: repeating segment found in path from "
//									<< " seg: " << dbgPrev << " edge: " <<  edge.first << "  prev edge:" <<
//									dbgPrevEdge.first << "   " << edge.second << "  " << dbgPrevEdge.second << "  " <<
//									WayPoint(boost::get(boost::edge_name, *graph,dbgPrevEdge.first)).roadSegment->getRoadSegmentId() << "\n";
						dbgPrev = wps.rbegin()->roadSegment->getRoadSegmentId();
						dbgPrevEdge = edge;
					}
					else
					{
						wps.push_back(wp);
					}
				}
				//Save for later.
				prev = it;
			}
		}
	}
	else
	{
		if(timeBased)
		{
			//logger << "Blacklist NOT empty" << blacklistV.size() << std::endl;
			//Filter it.

			sim_mob::A_StarShortestTravelTimePathImpl::BlackListEdgeConstraint filter(blacklistEdges);
			boost::filtered_graph<StreetDirectory::Graph,sim_mob::A_StarShortestTravelTimePathImpl::BlackListEdgeConstraint> filtered(*graph, filter);
			////////////////////////////////////////
			// TODO: This code is copied (since filtered_graph is not the same as adjacency_list) from searchShortestPath.
			////////////////////////////////////////
			std::list<StreetDirectory::Vertex> partialRes;

			vector<StreetDirectory::Vertex> p(boost::num_vertices(filtered)); //Output variable
			vector<double> d(boost::num_vertices(filtered));  //Output variable

			//Use A* to search for a path
			//Taken from: http://www.boost.org/doc/libs/1_38_0/libs/graph/example/astar-cities.cpp
			//...which is available under the terms of the Boost Software License, 1.0
			try
			{
				boost::astar_search(filtered, fromVertex,
						sim_mob::A_StarShortestTravelTimePathImpl::DistanceHeuristicFiltered(&filtered, toVertex),
						boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(sim_mob::A_StarShortestTravelTimePathImpl::GoalVisitor(toVertex)));
			}
			catch (sim_mob::A_StarShortestTravelTimePathImpl::Goal& goal)
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
						std::pair<StreetDirectory::Edge, bool> edge = boost::edge(*prev, *it, filtered);
						if (!edge.second) {
							std::cerr << "ERROR: Boost can't find an edge that it should know about." << std::endl;
						}
						//Retrieve, add this edge's WayPoint.
						WayPoint wp = boost::get(boost::edge_name, filtered,edge.first);
						//todo this problem occurs during "highway bias distance" generation. dont know why, discarding the repeated segment
						if (wp.type == WayPoint::ROAD_SEGMENT && wp.roadSegment->getRoadSegmentId() == dbgPrev)
						{
//							logger << dbgStr
//									<< " 1ERROR-exeThis:: repeating segment found in path from "
//									<< " seg: " << dbgPrev << " edge: " <<  edge.first << "  prev edge:" <<
//									dbgPrevEdge.first << "   " << edge.second << "  " << dbgPrevEdge.second << "  " <<
//									WayPoint(boost::get(boost::edge_name, *graph,dbgPrevEdge.first)).roadSegment->getRoadSegmentId() << "\n";
							dbgPrev = wps.rbegin()->roadSegment->getRoadSegmentId();
							dbgPrevEdge = edge;
						}
						else
						{
							wps.push_back(wp);
						}
					}

					//Save for later.
					prev = it;
				}
			}				//catch
		}
		else
		{
			//logger << "Blacklist NOT empty" << blacklistV.size() << std::endl;
			//Filter it.
			sim_mob::A_StarShortestPathImpl::BlackListEdgeConstraint filter(blacklistEdges);
			boost::filtered_graph<StreetDirectory::Graph,sim_mob::A_StarShortestPathImpl::BlackListEdgeConstraint> filtered(*graph, filter);
			////////////////////////////////////////
			// TODO: This code is copied (since filtered_graph is not the same as adjacency_list) from searchShortestPath.
			////////////////////////////////////////
			std::list<StreetDirectory::Vertex> partialRes;

			vector<StreetDirectory::Vertex> p(boost::num_vertices(filtered)); //Output variable
			vector<double> d(boost::num_vertices(filtered));  //Output variable

			//Use A* to search for a path
			//Taken from: http://www.boost.org/doc/libs/1_38_0/libs/graph/example/astar-cities.cpp
			//...which is available under the terms of the Boost Software License, 1.0
			try
			{
				boost::astar_search(filtered, fromVertex,
						sim_mob::A_StarShortestPathImpl::DistanceHeuristicFiltered(&filtered, toVertex),
						boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(sim_mob::A_StarShortestPathImpl::GoalVisitor(toVertex)));
			}
			catch (sim_mob::A_StarShortestPathImpl::Goal& goal)
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
						std::pair<StreetDirectory::Edge, bool> edge = boost::edge(*prev, *it, filtered);
						if (!edge.second) {
							std::cerr << "ERROR: Boost can't find an edge that it should know about." << std::endl;
						}
						//Retrieve, add this edge's WayPoint.
						WayPoint wp = boost::get(boost::edge_name, filtered,edge.first);
						//todo this problem occurs during "highway bias distance" generation. dont know why, discarding the repeated segment
						if (wp.type == WayPoint::ROAD_SEGMENT && wp.roadSegment->getRoadSegmentId() == dbgPrev)
						{
//							logger << dbgStr
//									<< " 1ERROR-exeThis:: repeating segment found in path from "
//									<< " seg: " << dbgPrev << " edge: " <<  edge.first << "  prev edge:" <<
//									dbgPrevEdge.first << "   " << edge.second << "  " << dbgPrevEdge.second << "  " <<
//									WayPoint(boost::get(boost::edge_name, *graph,dbgPrevEdge.first)).roadSegment->getRoadSegmentId() << "\n";
							dbgPrev = wps.rbegin()->roadSegment->getRoadSegmentId();
							dbgPrevEdge = edge;
						}
						else
						{
							wps.push_back(wp);
						}
					}

					//Save for later.
					prev = it;
				}
			}				//catch
		}
	}


	if (wps.empty()) {
		hasPath = false;
	} else {
		// make sp id
		std::string id = sim_mob::makePathString(wps);
		if(!id.size()){
			hasPath = false;
//			logger << "Error: Empty choice!!! yet valid=>" << dbgStr <<  "\n";
		}
		else{
//			logger << "Path generated through:" << dbgStr <<  ":" << id << "\n" ;
			path = new sim_mob::SinglePath();
			// fill data
			path->isNeedSave2DB = true;
			hasPath = true;
			path->pathSetId = pathSet->id;
			path->scenario = pathSet->scenario + dbgStr;
			path->init(wps);
			path->id = id;
			path->pathSize = 0;
			if(this->path->path.begin()->roadSegment->getParentLink()->getFromNodeId() != this->pathSet->subTrip.origin.node->getNodeId())
			{
				safe_delete_item(s);
				hasPath = false;
			}
		}
	}
}

