//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License";" as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/**
 * Pt_network_entities.hpp
 *
 *  Created on: Feb 24, 2015
 *  \author Prabhuraj
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include "geospatial/network/RoadSegment.hpp"
#include "geospatial/network/Node.hpp"
#include "geospatial/network/PT_Stop.hpp"
#include "path/Common.hpp"

namespace sim_mob
{

class PT_NetworkEdge
{
public:
	PT_NetworkEdge();

	virtual ~PT_NetworkEdge();

	double getDayTransitTimeSecs() const
	{
		return dayTransitTimeSecs;
	}

	void setDayTransitTimeSecs(double dayTransitTimeSecs)
	{
		this->dayTransitTimeSecs = dayTransitTimeSecs;
	}

	double getDistKms() const
	{
		return distKMs;
	}

	void setDistKms(double distKms)
	{
		distKMs = distKms;
	}

	int getEdgeId() const
	{
		return edgeId;
	}

	void setEdgeId(int edgeId)
	{
		this->edgeId = edgeId;
	}

	const std::string &getEndStop() const
	{
		return endStop;
	}

	void setEndStop(const std::string &endStop)
	{
		this->endStop = endStop;
	}

	double getLinkTravelTimeSecs() const
	{
		return linkTravelTimeSecs;
	}

	void setLinkTravelTimeSecs(double linkTravelTimeSecs)
	{
		this->linkTravelTimeSecs = linkTravelTimeSecs;
	}

	const std::string &getRoadIndex() const
	{
		return road_index;
	}

	void setRoadIndex(const std::string &roadIndex)
	{
		road_index = roadIndex;
	}

	const std::string &getRoadEdgeId() const
	{
		return roadEdgeId;
	}

	void setRoadEdgeId(const std::string &roadEdgeId)
	{
		this->roadEdgeId = roadEdgeId;
	}

	const std::string &getServiceLines() const
	{
		return rServiceLines;
	}

	void setServiceLines(const std::string &serviceLines)
	{
		rServiceLines = serviceLines;
	}

	const std::string &getTypeStr() const
	{
		return rType;
	}

	sim_mob::PT_EdgeType getType() const
	{
		return edgeType;
	}

	void setType(const std::string &type)
	{
		rType = type;
		if (rType == "Bus")
		{
			edgeType = sim_mob::BUS_EDGE;
		}
		else if (rType == "Walk")
		{
			edgeType = sim_mob::WALK_EDGE;
		}
		else if (rType == "RTS")
		{
			edgeType = sim_mob::TRAIN_EDGE;
		}
		else if (rType == "SMS")
		{
			edgeType = sim_mob::SMS_EDGE;
		}
	}

	const std::string &getStartStop() const
	{
		return startStop;
	}

	void setServiceLine(const std::string &line)
	{
		serviceLine = line;
	}

	const std::string getServiceLine() const
	{
		return serviceLine;
	}

	void setStartStop(const std::string &startStop)
	{
		this->startStop = startStop;
	}

	double getTransferPenaltySecs() const
	{
		return transferPenaltySecs;
	}

	void setTransferPenaltySecs(double transferPenaltySecs)
	{
		this->transferPenaltySecs = transferPenaltySecs;
	}

	double getTransitTimeSecs() const
	{
		return transitTimeSecs;
	}

	void setTransitTimeSecs(double transitTimeSecs)
	{
		this->transitTimeSecs = transitTimeSecs;
	}

	double getWaitTimeSecs() const
	{
		return waitTimeSecs;
	}

	void setWaitTimeSecs(double waitTimeSecs)
	{
		this->waitTimeSecs = waitTimeSecs;
	}

	double getWalkTimeSecs() const
	{
		return walkTimeSecs;
	}

	void setWalkTimeSecs(double walkTimeSecs)
	{
		this->walkTimeSecs = walkTimeSecs;
	}
	// Overloading == operator

	bool operator==(const PT_NetworkEdge &edge) const
	{
		return (edgeId == edge.getEdgeId());
	}

	bool operator!=(const PT_NetworkEdge &edge) const
	{
		return !(*this == edge);
	}

private:
	std::string startStop;       // Alphanumeric id
	std::string endStop;         // Alphanumeric id
	std::string rType;           // Service Line type, can be "BUS","LRT","WALK"
	std::string serviceLine;     // Service Line
	PT_EdgeType edgeType;        // Edge type inferred from

	std::string road_index;      // Index for road type 0 for BUS , 1 for LRT , 2 for Walk
	std::string roadEdgeId;      // Strings of passing road segments Ex: 4/15/35/43
	std::string rServiceLines;     //If the edge is a route segment, it will have bus service lines
	//in that route segment. If it is a walking leg, it will have
	//string "Walk".
	double linkTravelTimeSecs;   // Link travel time in seconds
	int edgeId;                 // Id for the current edge
	double waitTimeSecs;         // Estimated waiting time to begin on the current edge in seconds
	double walkTimeSecs;         // Estimated walk time in the current edge , estimated based
	//	on direct distance and 4km/h walking speed in seconds
	double transitTimeSecs;      // Estimated travel time on transit legs for bus and MRT/LRT in seconds
	double transferPenaltySecs;  // Transfer penalty used to impose penalty on transfers for
	//	shortest path searching which assumes path cost is the addition
	// of edge attributes in seconds
	double dayTransitTimeSecs;   // Estimated transit time for buses and MRT/LRT in day time. Night
	//bus services will have large value in the column in seconds
	double distKMs;               // Estimated distance from travel time table in kilometers
};

class PT_NetworkVertex
{
public:
	PT_NetworkVertex();

	virtual ~PT_NetworkVertex();

	const std::string &getEzlinkName() const
	{
		return ezlinkName;
	}

	void setEzlinkName(const std::string &ezlinkName)
	{
		this->ezlinkName = ezlinkName;
	}

	const std::string &getStopCode() const
	{
		return stopCode;
	}

	void setStopCode(const std::string &stopCode)
	{
		this->stopCode = stopCode;
	}

	const std::string &getStopDesc() const
	{
		return stopDesc;
	}

	void setStopDesc(const std::string &stopDesc)
	{
		this->stopDesc = stopDesc;
	}

	const std::string &getRoadItemId() const
	{
		return stopId;
	}

	void setStopId(const std::string &stopId)
	{
		this->stopId = stopId;
	}

	double getStopLatitude() const
	{
		return stopLatitude;
	}

	void setStopLatitude(double stopLatitude)
	{
		this->stopLatitude = stopLatitude;
	}

	double getStopLongitude() const
	{
		return stopLongitude;
	}

	void setStopLongitude(double stopLongitude)
	{
		this->stopLongitude = stopLongitude;
	}

	const std::string &getStopName() const
	{
		return stopName;
	}

	void setStopName(const std::string &stopName)
	{
		this->stopName = stopName;
	}

	int getStopType() const
	{
		return stopType;
	}

	void setStopType(int stopType)
	{
		this->stopType = stopType;
	}

private:
	std::string stopId;     // Stop id for stops as specified by LTA. It can bus stops , train
	// stations or simMobility nodes(Starts with N_)
	std::string stopCode;   // stop codes for stops specified by LTA
	std::string stopName;   // Name of stop as per LTA
	double stopLatitude;    // Latitude of the stop
	double stopLongitude;   // Longitude of the stop
	std::string ezlinkName; // eZlinkName of the stop, usually same as stop code
	int stopType;           // 0 --- SimMobility nodes
	// 1 --- Bus stops
	// 2 --- MRT/LRT Stations
	std::string stopDesc;   // Description of stops . Usually street where the stop is located
};

class PT_Network
{
public:
	enum NetworkType
	{
		TYPE_DEFAULT = 0,
		TYPE_RAIL_SMS = 1
	};

	PT_Network()
	{

	}

	virtual ~PT_Network();

	/* map of edge id and edge */
	std::map<int, PT_NetworkEdge> PT_NetworkEdgeMap;

	/* map of vertex id and vertex */
	std::map<std::string, PT_NetworkVertex> PT_NetworkVertexMap;

	/* map of stop id and TrainStop */
	std::map<std::string, TrainStop *> MRTStopsMap;

	/* map of mrt stop and edges,the edge between train stops*/
	std::map<std::string, std::map<std::string, std::vector<PT_NetworkEdge>>> MRTStopdgesMap;

	/*
	 * This function creates the public transit network with edges and vertices
	 * @param storedProcForVertex is store procedure for vertex
	 * @param storeProceForEdges is store procedure for edges
	 */
	void initialise(const std::string &storedProcForVertex, const std::string &storeProceForEdges);

	/* this function gets the pointer to a particular train stop by its stop id
	 * @param stopId is the id of the stop
	 * @return  the pointer to train stop
	 */
	TrainStop *findMRT_Stop(const std::string &stopId) const;

	/*
	 * This function returns vertex type from stop is
	 * @param stopId is the id of the stop
	 * @return type of vertex
	 */
	int getVertexTypeFromStopId(std::string stopId);
};

class PT_NetworkCreater
{
public:
	static void createNetwork(const std::string &storedProcForVertex, const std::string &storeProceForEdges,
	                          PT_Network::NetworkType type = PT_Network::TYPE_DEFAULT);

	static PT_Network &getInstance(PT_Network::NetworkType type = PT_Network::TYPE_DEFAULT)
	{
		auto itInstance = mapOfInstances.find(type);

		if(itInstance != mapOfInstances.end())
		{
			return *(itInstance->second);
		}
		else
		{
			std::stringstream msg;
			msg << "Undefined PT_Network type: " << type << ". Unable to retrieve PT_Network instance";
			throw std::runtime_error(msg.str());
		}
	}

private:
	static std::map<PT_Network::NetworkType, PT_Network *>  mapOfInstances;

	static void init();
};
}//End of namespace sim_mob
