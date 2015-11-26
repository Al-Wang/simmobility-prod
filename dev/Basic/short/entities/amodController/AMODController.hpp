//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * AMODController.hpp
 *
 *  Created on: Mar 13, 2014
 *      Author: Max
 */
#pragma once

#include <boost/regex.hpp>
#include <fstream>
#include <map>
#include <string>
#include "AMODEvent.hpp"
#include "entities/Agent.hpp"
#include "event/args/EventArgs.hpp"
#include "event/EventPublisher.hpp"
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "path/PathSetManager.hpp"

namespace sim_mob
{
class Person_ST;

namespace AMOD
{

class AMODController : public Agent
{
public:

	struct AmodTrip
	{
		std::string tripID;
		std::string origin;
		std::string destination;
		std::string carParkIdDeparture;
		std::string carParkIdArrival;
		std::string assignedAmodId;

		int requestTime;
		int dispatchTime;
		int pickUpTime;
		int arrivalTime;

		double tripDistanceInM;
		bool tripError; //true if the trip ended in an errro (vehicle was destroyed before arriving)
		int pickUpSegment; //segment right after the pick up node (to check whether we have passed the pick up point)
		bool pickedUp;
		int finalSegment;
		int stuckCount;

	};

	struct EmptyVhTrip
	{
		std::string tripID;
		std::string origin;
		std::string destination;
		double tripDistanceInM;
		int numberOfEmptyTrips;
		int stuckCount;
	};


public:
	virtual ~AMODController();

	/// create segment ,node pool
	void init();

	/**
	 * retrieve a singleton object
	 * @return a pointer to AMODController .
	 */
	static AMODController* instance();
	static void deleteInstance();
	static bool instanceExists();
	static void registerController(int id, const MutexStrategy& mtxStrat);
	/// pu new amod vh to car park
	/// id am vh id
	/// nodeId node id ,virtual car park
	void populateCarParks(int numberOfVhsAtNode);
	// populateCarParks - add vehicles to the carParks which are at nodes
	// @param carparkIds - vector of strings of node ids where vehicles will be parked at the beginning of simulation
	// @param number ofVhAtCarpark - integer, number of vehicles at each carpark at the beginning of simulation
	void readDemandFile(std::vector<std::string>& tripID, int current_time, std::vector<std::string>& origin, std::vector<std::string>& destination);
	//read the demand from the txt file and outputs two vectors
	//@param origin - vector of strings, origin of the trip
	//@param destination - vector if strings, destination
	//@param current_time - int, current time of the simulation
	//@param k - int, stores current number of line which has been read from the file
	void addNewVh2CarPark(std::string& id, std::string& nodeId);
	// return false ,if no vh in car park

	// For finding the nearest free vehicle
	typedef std::pair< double, std::string > distPair;

	bool distPairComparator(const distPair& l, const distPair& r)
	{
		return l.first < r.first;
	}
	bool findNearestFreeVehicle(std::string originId, std::map<std::string, Node* > &nodePool, std::string &carParkId, Person_ST**vh);
	//void findAllFreeVhs(std::vector<std::string> carParkIdsWithVhs);
	void findNearestCarPark(std::string& originId, const RoadSegment* lastWPrs, std::vector<std::string>& AllCarParks, std::string &carParkIdArrival, std::vector<const RoadSegment*> &blacklist);
	void findNearestCarParkToNode(std::string& destinationNode, const RoadSegment* lastWPrs, std::vector<std::string>& AllCarParks, std::string &carParkIdArrival, std::vector<const RoadSegment*> &blacklist);
	void calculateThePath(std::vector<WayPoint>& wp1, std::string& toNode, std::vector<const RoadSegment*> blacklist, std::string& pickUpSegmentStr, const RoadSegment* &StopPointRS, std::vector < WayPoint > &routeWP);
	void blacklistForbiddenSegments(const RoadSegment* lastWPrs, const Node* startNode, std::vector<const RoadSegment*> &blacklist);

	// For finding "best" free vehicle
	bool getBestFreeVehicle(std::string originId, Person_ST **vh, std::string &carParkId, std::vector < WayPoint > &leastCostPath, double &bestTravelCost);
	double getTravelTimePath(std::string startNodeId, std::string endNodeId, std::vector < WayPoint > &leastCostPath);

	// Calculates the remaining travel time
	double calculateTravelTime(std::vector < WayPoint > &wPs);

	void precomputeAllPairsShortestPaths(void);
	bool hasShortestPath(std::string origNodeID, std::string destNodeID);
	std::vector < WayPoint > getShortestPath(std::string origNodeID, std::string destNodeID);
	std::vector <WayPoint> getShortestPathWBlacklist(std::string origNodeID, std::string destNodeID, std::vector<const RoadSegment*> blacklist);

	bool getVhFromCarPark(std::string& carParkId, Person_ST** vh);
	bool removeVhFromCarPark(std::string& carParkId, Person_ST** vh); //removes a specific vehicle from the car park
	//removes vehicle from the carpark
	void mergeWayPoints(const std::vector<WayPoint>& carparkToOrigin, const std::vector<WayPoint> &originToDestination, std::vector<WayPoint>& mergedWP);
	//mergeWayPoints ->merge waypoints carpark-origin and origin-destination
	// set predefined path
	bool setPath2Vh(Person_ST* vh, std::vector<WayPoint>& path);
	// ad vh to main loop
	bool dispatchVh(Person_ST* vh);

	void rerouteWithPath(Person_ST* vh, std::vector<WayPoint>& path);
	void rerouteWithOriDest(Person_ST* vh, Node* snode, Node* enode);
	/// snode: start node
	/// enode: end node
	/// path: new path

	void saveTripStat(AmodTrip &a);

	//void testOneVh();
	void testSecondVh();
	void testVh();
	void testTravelTimePath();

	//Assign vehicle
	void assignVhsFast(std::vector<std::string>& tripID, std::vector<std::string>& origin, std::vector<std::string>& destination, int currTime);
	void assignVhToEmptyTrip(std::string& origin, std::string& destination, int currTime);
	void rebalanceVhs();
	void calculateDistancesBetweenCarparks(std::vector<std::string> carParkIds, std::vector<std::vector < double > > &distancesBetweenCarparks);

	int test;

	const Node* getNodeFrmPool(const std::string& nodeId);

	void handleAMODEvent(event::EventId id,
						event::Context ctxId,
						event::EventPublisher* sender,
						const AMOD::AMODEventArgs& args);

	void handleVHArrive(Person_ST* vh);
	void handleVHDestruction(Person_ST *vh);
	void handleVHPickup(Person_ST *vh);

	void processArrival(Person_ST *vh);
	boost::thread mThread;

protected:
	//override from the class agent, provide initilization chance to sub class
	virtual Entity::UpdateStatus frame_init(timeslice now);
	//override from the class agent, do frame tick calculation
	virtual Entity::UpdateStatus frame_tick(timeslice now);
	//override from the class agent, print output information
	virtual void frame_output(timeslice now);

	//May implement later

	virtual void load(const std::map<std::string, std::string>& configProps)
	{
	}
	//Signals are non-spatial in nature.

	virtual bool isNonspatial()
	{
		return true;
	}

	virtual std::vector<BufferedBase *> buildSubscriptionList()
	{
		return std::vector<BufferedBase *>();
	}
	//	override from the class agent, provide a chance to clear up a child pointer when it will be deleted from system
	virtual void unregisteredChild(Entity* child);

private:
	explicit AMODController(int id = -1,
							const MutexStrategy& mtxStrat = MtxStrat_Buffered);

private:

	/// key=node id, value= (key=vh id,value=vh)
	typedef boost::unordered_map<std::string, boost::unordered_map<std::string, Person_ST*> > AMODVirtualCarPark;
	typedef boost::unordered_map<std::string, boost::unordered_map<std::string, Person_ST*> >::iterator AMODVirtualCarParkItor;
	AMODVirtualCarPark virtualCarPark;

	boost::unordered_map<std::string, Person_ST*> vhOnTheRoad;
	boost::unordered_map<std::string, Person_ST*> EmptyVhOnTheRoad;
	boost::unordered_map<std::string, Person_ST*> vhInCarPark;
	boost::unordered_map<std::string, Person_ST*> allAMODCars;
	std::vector<std::string> carParkIds;
	std::vector<std::vector < double > > distancesBetweenCarparks; //inCM
	std::string emptyTripId;

	boost::unordered_map<Person_ST*, AmodTrip > vhTripMap;
	typedef boost::unordered_map< Person_ST*, AmodTrip >::iterator TripMapIterator;

	std::list < AmodTrip > serviceBuffer;
	typedef std::list< AmodTrip >::iterator ServiceIterator;
	int nFreeCars;
	int frameTicks;
	int currTime;

	//shortest paths
	boost::unordered_map<std::string, std::vector <WayPoint> > shortestPaths;

private:
	static AMODController* pInstance;

public:
	std::map<std::string, const RoadSegment*> segPool; // store all segs ,key= aimsun id ,value = seg
	std::map<std::string, const Node*> nodePool; // store all nodes ,key= aimsun id ,value = node
	AMODEventPublisher eventPub;

	StreetDirectory* stdir;
	std::map<const RoadSegment*, RdSegTravelTimes> RdSegTravelTimesMap;

	void setRdSegTravelTimes(Person_ST* ag, double rdSegExitTime);
	void updateTravelTimeGraph();
	bool insertTravelTime2TmpTable(timeslice frameNumber, std::map<const RoadSegment*, RdSegTravelTimes>& rdSegTravelTimesMap);

	struct amod_passenger
	{
		std::string &passengerId;
		std::string &originNode;
		std::string &destNode;
		double &timeToPickUp;
		double waitingTime;
		double timeToBeAtDest;
	};

	std::ifstream myFile;
	std::ifstream carParkFile;
	std::ofstream out_demandStat;
	std::ofstream out_vhsStat;
	std::ofstream out_tripStat;

	std::string lastReadLine;
	int nCarsPerCarPark;
	int vehStatOutputModulus;
	int rebalancingModulus;
	boost::mutex allAMODCarsMutex;
	boost::mutex carParksMutex;
	boost::mutex vhTripMapMutex;

	std::time_t startRunTime;
	std::time_t endRunTime;
};

inline std::string getNumberFromAimsunId(std::string &aimsunid)
{
	//"aimsun-id":"69324",
	std::string number;
	boost::regex expr(".*\"aimsun-id\":\"([0-9]+)\".*$");
	boost::smatch matches;
	if (boost::regex_match(aimsunid, matches, expr))
	{
		number = std::string(matches[1].first, matches[1].second);
	}
	else
	{
		// Warn()<<"aimsun id not correct "+aimsunid << std::endl;
	}

	return number;
}
} /* namespace AMOD */
} /* namespace sim_mob */

