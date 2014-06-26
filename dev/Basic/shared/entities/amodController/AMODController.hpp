//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * AMODController.hpp
 *
 *  Created on: Mar 13, 2014
 *      Author: Max
 */

#ifndef AMODController_HPP_
#define AMODController_HPP_

#include <boost/regex.hpp>
#include <map>

#include "entities/Agent.hpp"
//#include "../short/entities/roles/driver/Driver.hpp"
#include "event/EventPublisher.hpp"
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "event/args/EventArgs.hpp"
#include "AMODEvent.hpp"
#include "entities/conflux/Conflux.hpp"
#include "geospatial/PathSetManager.hpp"
#include "entities/Person.hpp"
#include <string>
#include <fstream>

namespace sim_mob {
class Person;

namespace AMOD {

class AMODController : public sim_mob::Agent{
public:
	virtual ~AMODController();

	/// create segment ,node pool
	void init();

	/**
	 * retrieve a singleton object
	 * @return a pointer to AMODController .
	 */
	static AMODController* instance();
	static void registerController(int id, const MutexStrategy& mtxStrat);
	/// pu new amod vh to car park
	/// id am vh id
	/// nodeId node id ,virtual car park
	void populateCarParks();
	// populateCarParks - add vehicles to the carParks which are at nodes
	// @param carparkIds - vector of strings of node ids where vehicles will be parked at the beginning of simulation
	// @param number ofVhAtCarpark - integer, number of vehicles at each carpark at the beginning of simulation
	void readDemandFile(std::ifstream& myFile, std::vector<std::string>& time, std::vector<std::string>& origin, std::vector<std::string>& destination);
	//read the demand from the txt file and outputs two vectors
	//@param origin - vector of strings, origin of the trip
	//@param destination - vector if strings, destination
	void addNewVh2CarPark(std::string& id,std::string& nodeId);
	// return false ,if no vh in car park

	typedef std::pair< double , std::string > distPair;
	bool distPairComparator ( const distPair& l, const distPair& r){ return l.first < r.first; }
	bool findNearestFreeVehicle(std::string originId, std::map<std::string, sim_mob::Node* > &nodePool, std::string &carParkId, Person**vh);
	// For finding the nearest free vehicle
	bool getVhFromCarPark(std::string& carParkId,Person** vh);
	//removes vehicle from the carpark
	void mergeWayPoints(const std::vector<sim_mob::WayPoint>& carparkToOrigin, const std::vector<sim_mob::WayPoint> &originToDestination, std::vector<sim_mob::WayPoint>& mergedWP);
	//mergeWayPoints ->merge waypoints carpark-origin and origin-destination
	// set predefined path
	bool setPath2Vh(Person* vh,std::vector<WayPoint>& path);
	// ad vh to main loop
	bool dispatchVh(Person* vh);

	void rerouteWithPath(Person* vh,std::vector<sim_mob::WayPoint>& path);
	void rerouteWithOriDest(Person* vh,Node* snode,Node* enode);
	/// snode: start node
	/// enode: end node
	/// path: new path

	//void testOneVh();
    void testSecondVh();
	void testVh();
	void testTravelTimePath();
	void assignVhs();
	int test;

	void handleAMODEvent(sim_mob::event::EventId id,
			sim_mob::event::Context ctxId,
			sim_mob::event::EventPublisher* sender,
			const AMOD::AMODEventArgs& args);

	void handleVHArrive(Person* vh);
protected:
	//override from the class agent, provide initilization chance to sub class
	virtual bool frame_init(timeslice now);
	//override from the class agent, do frame tick calculation
	virtual Entity::UpdateStatus frame_tick(timeslice now);
	//override from the class agent, print output information
	virtual void frame_output(timeslice now);

	//May implement later
	virtual void load(const std::map<std::string, std::string>& configProps){}
	//Signals are non-spatial in nature.
	virtual bool isNonspatial() { return true; }
	virtual void buildSubscriptionList(std::vector<BufferedBase*>& subsList){}
	//	override from the class agent, provide a chance to clear up a child pointer when it will be deleted from system
	//	virtual void unregisteredChild(Entity* child);

private:
	explicit AMODController(int id=-1,
			const MutexStrategy& mtxStrat = sim_mob::MtxStrat_Buffered);

private:

	/// key=node id, value= (key=vh id,value=vh)
	typedef boost::unordered_map<std::string,boost::unordered_map<std::string,Person*> > AMODVirtualCarPark;
	typedef boost::unordered_map<std::string,boost::unordered_map<std::string,Person*> >::iterator AMODVirtualCarParkItor;
	AMODVirtualCarPark virtualCarPark;

	boost::unordered_map<std::string,Person*> vhOnTheRoad;

	int frameTicks;

private:
	static AMODController* pInstance;

public:
	std::map<std::string,sim_mob::RoadSegment*> segPool; // store all segs ,key= aimsun id ,value = seg
	std::map<std::string,sim_mob::Node*> nodePool; // store all nodes ,key= aimsun id ,value = node
	AMODEventPublisher eventPub;

	StreetDirectory* stdir;
	std::map<const RoadSegment*, Conflux::rdSegTravelTimes> RdSegTravelTimesMap;

	void setRdSegTravelTimes(Person* ag, double rdSegExitTime);
	void updateTravelTimeGraph();
	bool insertTravelTime2TmpTable(timeslice frameNumber, std::map<const RoadSegment*, sim_mob::Conflux::rdSegTravelTimes>& rdSegTravelTimesMap);

	struct amod_passenger{
		std::string &passengerId; std::string &originNode; std::string &destNode;
		double &timeToPickUp; double waitingTime; double timeToBeAtDest;
	};
};


inline std::string getNumberFromAimsunId(std::string &aimsunid)
{
	//"aimsun-id":"69324",
	std::string number;
	boost::regex expr (".*\"aimsun-id\":\"([0-9]+)\".*$");
	boost::smatch matches;
	if (boost::regex_match(aimsunid, matches, expr))
	{
		number  = std::string(matches[1].first, matches[1].second);
		//		Print()<<"getNumberFromAimsunId: "<<number<<std::endl;
	}
	else
	{
		Warn()<<"aimsun id not correct "+aimsunid<<std::endl;
	}

	return number;
}
} /* namespace AMOD */
} /* namespace sim_mob */
#endif /* AMODController_HPP_ */
