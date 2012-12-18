#include <math.h>
#include "entities/roles/pedestrian/Pedestrian.hpp"
//#include "entities/roles/driver/BusDriver.hpp"
#include "Driver.hpp"
#include "entities/Person.hpp"
#include "entities/UpdateParams.hpp"
#include "entities/misc/TripChain.hpp"
#include "entities/conflux/Conflux.hpp"
#include "buffering/BufferedDataManager.hpp"
#include "geospatial/Link.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/Lane.hpp"
#include "geospatial/Node.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/MultiNode.hpp"
#include "geospatial/LaneConnector.hpp"
#include "geospatial/Point2D.hpp"
#include "util/OutputUtil.hpp"
//#include "util/DynamicVector.hpp"
//#include "util/GeomHelpers.hpp"
#include "util/DebugFlags.hpp"
#include "partitions/PartitionManager.hpp"
#include "entities/AuraManager.hpp"
#include <ostream>
#include <algorithm>

#ifndef SIMMOB_DISABLE_PI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#include "partitions/ParitionDebugOutput.hpp"
#endif

using namespace sim_mob;

using std::max;
using std::vector;
using std::set;
using std::map;
using std::string;
using std::endl;

//Helper functions
namespace {
//Helpful constants


//TODO:I think lane index should be a data member in the lane class
size_t getLaneIndex(const Lane* l) {
	if (l) {
		const RoadSegment* r = l->getRoadSegment();
		for (size_t i = 0; i < r->getLanes().size(); i++) {
			if (r->getLanes().at(i) == l) {
				return i;
			}
		}
	}
	return -1; //NOTE: This might not do what you expect! ~Seth
}
} //end of anonymous namespace

//Initialize
sim_mob::medium::Driver::Driver(Agent* parent, MutexStrategy mtxStrat) :
	Role(parent), /*remainingTimeToComplete(0),*/ currLane(nullptr), vehicle(nullptr),
	nextLaneInNextSegment(nullptr), params(parent->getGenerator())
{

//	if (Debug::Drivers) {
//		DebugStream << "Driver starting: " << parent->getId() << endl;
//	}
}

///Note that Driver's destructor is only for reclaiming memory.
///  If you want to remove its registered properties from the Worker (which you should do!) then
///  this should occur elsewhere.
sim_mob::medium::Driver::~Driver() {
	//Our vehicle
	safe_delete_item(vehicle);
	std::cout << ss.str() << endl;
}

vector<BufferedBase*> sim_mob::medium::Driver::getSubscriptionParams() {
	vector<BufferedBase*> res;
	//res.push_back(&(currLane_));
	//res.push_back(&(currLaneOffset_));
	//res.push_back(&(currLaneLength_));
	return res;
}

Role* sim_mob::medium::Driver::clone(Person* parent) const
{
	return new Driver(parent, parent->getMutexStrategy());
}

void sim_mob::medium::Driver::frame_init(UpdateParams& p)
{
	//Save the path from orign to next activity location in allRoadSegments
	if (!currResource) {
		Vehicle* veh = initializePath(true);
		if (veh) {
			safe_delete_item(vehicle);
			//To Do: Better to use currResource instead of vehicle, when handling other roles ~melani
			vehicle = veh;
			currResource = veh;
		}
	}
	else {
		initializePath(false);
	}

}

double sim_mob::medium::Driver::getTimeSpentInTick(DriverUpdateParams& p) {
	return p.timeThisTick;
}

void sim_mob::medium::Driver::stepFwdInTime(DriverUpdateParams& p,
		double time) {
	p.timeThisTick = p.timeThisTick + time;
}

void sim_mob::medium::Driver::setOrigin(DriverUpdateParams& p) {

	//Vehicles start at rest
	vehicle->setVelocity(0);

	//set time to start - to accommodate drivers starting during the frame
	stepFwdInTime(p, parent->getStartTime() - p.now.ms());

	const sim_mob::RoadSegment* nextRdSeg = nullptr;
	if (vehicle->hasNextSegment(true))
		nextRdSeg = vehicle->getNextSegment(true);

	else if (vehicle->hasNextSegment(false))
		nextRdSeg = vehicle->getNextSegment(false);

	nextLaneInNextSegment = getBestTargetLane(vehicle->getCurrSegment(), nextRdSeg);

	double departTime = getLastAccept(nextLaneInNextSegment) + getAcceptRate(nextLaneInNextSegment);

	double t = std::max(p.timeThisTick, (departTime - p.now.ms())/1000.0);

	if(canGoToNextRdSeg(p, t))
	{
		//set position to start
		if(vehicle->getCurrSegment())
				vehicle->setPositionInSegment(vehicle->getCurrLinkLaneZeroLength());

		currLane = nextLaneInNextSegment;
		double actualT = t + p.now.ms();
		parent->linkEntryTime = actualT;
		setLastAccept(currLane, actualT);
		std::cout<<actualT<<std::endl;
		std::cout<<"lane: "<<currLane->getLaneID_str()<<" lastAccept: "<<getLastAccept(currLane)<<std::endl;
		setParentData();
	}
	else
	{
		std::cout<<"cangoto failed"<<std::endl;
#ifndef SIMMOB_DISABLE_OUTPUT
		boost::mutex::scoped_lock local_lock(sim_mob::Logger::global_mutex);
		std::cout << "Driver cannot be started in new segment, will remain in lane infinity!\n";
#endif
	}
}

sim_mob::UpdateParams& sim_mob::medium::Driver::make_frame_tick_params(timeslice now)
{
	params.reset(now, *this);
	return params;
}

void sim_mob::medium::DriverUpdateParams::reset(timeslice now, const Driver& owner)
{
	UpdateParams::reset(now);

	//Reset; these will be set before they are used; the values here represent either default
	//       values or are unimportant.

	elapsedSeconds = ConfigParams::GetInstance().baseGranMS / 1000.0;

	timeThisTick = 0.0;
}

void sim_mob::medium::Driver::setParentData() {
	Person* parentP = dynamic_cast<Person*> (parent);
	if(!vehicle->isDone()) {
		if (parentP){
		//	parentP->isQueuing = vehicle->isQueuing;
			parentP->distanceToEndOfSegment = vehicle->getPositionInSegment();
			parentP->movingVelocity = vehicle->getVelocity();
		}
		parent->setCurrLane(currLane);
		parent->setCurrSegment(vehicle->getCurrSegment());
	}
	else {
		if (parentP){
			//	parentP->isQueuing = vehicle->isQueuing;
			parentP->distanceToEndOfSegment = 0.0;
			parentP->movingVelocity = 0.0;
		}
		parent->setCurrLane(nullptr);
		parent->setCurrSegment(nullptr);
	}
}

void sim_mob::medium::Driver::frame_tick_output(const UpdateParams& p)
{
	//Skip?
	if (vehicle->isDone() || ConfigParams::GetInstance().is_run_on_many_computers) {
		return;
	}

#ifndef SIMMOB_DISABLE_OUTPUT
	std::stringstream logout;
	logout << "(\"Driver\""
			<<","<<parent->getId()
			<<","<<p.now.frame()
			<<",{"
			<<"\"RoadSegment\":\""<<static_cast<int>(vehicle->getCurrSegment()->getSegmentID())
			<<"\",\"Lane\":\""<<static_cast<int>(vehicle->getCurrLane()->getLaneID())
			<<"\",\"DownNode\":\""<<(vehicle->getCurrSegment()->getEnd()->getID())
			<<"\",\"DistanceToEndSeg\":\""<<vehicle->getPositionInSegment();

	if (this->parent->isQueuing) {
			logout << "\",\"queuing\":\"" << "true";
		} else {
			logout << "\",\"queuing\":\"" << "false";
		}

		logout << "\"})" << std::endl;

		LogOut(logout.str());
#endif
}

Vehicle* sim_mob::medium::Driver::initializePath(bool allocateVehicle) {
	Vehicle* res = nullptr;

	//Only initialize if the next path has not been planned for yet.
	if(!parent->getNextPathPlanned()){
		//Save local copies of the parent's origin/destination nodes.
		origin.node = parent->originNode;
		origin.point = origin.node->location;
		goal.node = parent->destNode;
		goal.point = goal.node->location;

		//Retrieve the shortest path from origin to destination and save all RoadSegments in this path.
		vector<WayPoint> path;
		Person* parentP = dynamic_cast<Person*> (parent);
		if (!parentP || parentP->specialStr.empty()) {
			path = StreetDirectory::instance().SearchShortestDrivingPath(*origin.node, *goal.node);
		}

		ss << "\nPath: ";
		for(int i = 1; i!=path.size(); i++) {
			if(path[i].type_ == WayPoint::ROAD_SEGMENT) {
				path.at(i).roadSegment_->getEnd()->getID();
			}
		}
		std::cout << ss.str();
		ss.str("");
		//TODO: Start in lane 0?
		int startlaneID = 0;

		// Bus should be at least 1200 to be displayed on Visualizer
		const double length = 400;
		const double width = 200;

		//A non-null vehicle means we are moving.
		if (allocateVehicle) {
			res = new Vehicle(path, startlaneID, length, width);
		}
	}

	//to indicate that the path to next activity is already planned
	parent->setNextPathPlanned(true);
	return res;
}

bool sim_mob::medium::Driver::moveToNextSegment(DriverUpdateParams& p, unsigned int currTimeMS, double timeSpent)
{
//1.	copy logic from DynaMIT::moveToNextSegment
//2.	add the corresponding changes from justLeftIntersection()
//3.	updateVelocity for the new segment
//4. 	vehicle->moveFwd()

	bool res = false;

	bool isNewLinkNext = ( !vehicle->hasNextSegment(true) && vehicle->hasNextSegment(false));

	const sim_mob::RoadSegment* nextRdSeg = nullptr;

	if (isNewLinkNext)
		nextRdSeg = vehicle->getNextSegment(false);

	else
		nextRdSeg = vehicle->getNextSegment(true);

	if ( !nextRdSeg) {
		//vehicle is done
		vehicle->actualMoveToNextSegmentAndUpdateDir_med();
		if (vehicle->isDone()) {
				parent->setToBeRemoved();
			}
		return false;
	}



	const sim_mob::RoadSegment* nextToNextRdSeg = vehicle->getSecondSegmentAhead();

	nextLaneInNextSegment = getBestTargetLane(nextRdSeg, nextToNextRdSeg);

	//getLastAccept not implemented yet
	double departTime = getLastAccept(nextLaneInNextSegment) + getAcceptRate(nextLaneInNextSegment);
	double t = std::max(p.timeThisTick, (departTime - p.now.ms())/1000.0);
	ss<<"Time: "<<t<<endl;
	std::cout<<ss.str();
	if (canGoToNextRdSeg(p, t)){
		if (vehicle->isQueuing){
			removeFromQueue();
		}
		else{
		//	removeFromMovingList();
		}

		currLane = nextLaneInNextSegment;
	//	p.timeThisTick = p.timeThisTick + t;
		unsigned int linkExitTimeMS = p.now.ms() + p.timeThisTick * 1000;

		if (isNewLinkNext)
		{
			//Updating location information for agent for density calculations
		//	parent->setCurrLink((currLane)->getRoadSegment()->getLink());
			//set Link Travel time for previous link
			const RoadSegment* prevSeg = vehicle->getPrevSegment(false);
		if (prevSeg){
				const Link* prevLink = prevSeg->getLink();
				parent->setTravelStats(prevLink, linkExitTimeMS, parent->linkEntryTime, true);
			}
		parent->linkEntryTime = linkExitTimeMS;
		}

		vehicle->actualMoveToNextSegmentAndUpdateDir_med();
		addToMovingList();

		vehicle->setPositionInSegment(vehicle->getCurrLinkLaneZeroLength());
		setLastAccept(currLane, linkExitTimeMS);
		res = advance(p, currTimeMS);
	}

	else{
		std::cout<<"canGoTo failed!"<<std::endl;
		if (vehicle->isQueuing){
			moveInQueue();
		}
		else{
			addToQueue(currLane);
		}
	}

	return res;
}

bool sim_mob::medium::Driver::canGoToNextRdSeg(DriverUpdateParams& p, double t)
{
	if (t >= p.elapsedSeconds) return false;

	const RoadSegment* nextRdSeg = nextLaneInNextSegment->getRoadSegment();

	if ( !nextRdSeg) return false;

	unsigned int total = nextRdSeg->getParentConflux()->numMovingInSegment(nextRdSeg, true)
			+ nextRdSeg->getParentConflux()->numQueueingInSegment(nextRdSeg, true);

	return total < nextRdSeg->getLanes().size() * nextRdSeg->computeLaneZeroLength()/vehicle->length;
}

void sim_mob::medium::Driver::moveInQueue()
{
	//1.update position in queue (vehicle->setPosition(distInQueue))
	//2.update p.timeThisTick

	vehicle->setPositionInSegment(getQueueLength(currLane));
}

const sim_mob::Lane* sim_mob::medium::Driver::getBestTargetLane(const RoadSegment* nextRdSeg, const RoadSegment* nextToNextRdSeg){

	//1. Get queueing counts for all lanes of the next Segment
	//2. Select the lane with the least queue length
	//3. Update nextLaneInNextLink and targetLaneIndex accordingly
	if(!nextRdSeg)
		return nullptr;

	const sim_mob::Lane* minQueueLengthLane = nullptr;
	unsigned int minQueueLength = std::numeric_limits<int>::max();
	unsigned int que = 0;

	vector<sim_mob::Lane* >::const_iterator i = nextRdSeg->getLanes().begin();

	for ( ; i != nextRdSeg->getLanes().end(); ++i){
		if ( !((*i)->is_pedestrian_lane())){
			if(nextToNextRdSeg) {
				if( !isConnectedToNextSeg(*i, nextToNextRdSeg)) continue;
			}
			que = nextRdSeg->getParentConflux()->getLaneAgentCounts(*i).first;
			if (minQueueLength > que){
				minQueueLength = que;
				minQueueLengthLane = *i;
			}
		}
	}

	if( !minQueueLengthLane){
#ifndef SIMMOB_DISABLE_OUTPUT
		boost::mutex::scoped_lock local_lock(sim_mob::Logger::global_mutex);
		std::cout << "ERROR: best target lane was not set!\n";
#endif
	}
	return minQueueLengthLane;
}

void sim_mob::medium::Driver::addToQueue(const Lane* lane) {
	/* 1. set position to queue length in front
	 * 2. set isQueuing = true
	*/
	vehicle->setPositionInSegment(getQueueLength(lane));

	Person* parentP = dynamic_cast<Person*> (parent);
	if (parentP)
		parentP->isQueuing = vehicle->isQueuing = true;
}

void sim_mob::medium::Driver::addToMovingList() {

	Person* parentP = dynamic_cast<Person*> (parent);
	if (parentP)
		parentP->isQueuing = vehicle->isQueuing = false;
}

void sim_mob::medium::Driver::removeFromQueue() {

	Person* parentP = dynamic_cast<Person*> (parent);
	if (parentP)
		parentP->isQueuing = vehicle->isQueuing = false;
}

void sim_mob::medium::Driver::removeFromMovingList() {
	throw std::runtime_error("Driver::removeFromMovingList() not implemented");
/*	sim_mob::SegmentStats* segStats = nullptr;

	if (currResource)
		segStats = currResource->getCurrSegment()->getParentConflux()->
			getSegmentAgents()[currResource->getCurrSegment()];
	else{
#ifndef SIMMOB_DISABLE_OUTPUT
		boost::mutex::scoped_lock local_lock(sim_mob::Logger::global_mutex);
		std::cout << "ERROR: currResource is not set for Driver! \n";
#endif
	}
	if (segStats)
		segStats->removeAgent(vehicle->getCurrLane(), parent);
*/
}

bool sim_mob::medium::Driver::moveInSegment(DriverUpdateParams& p2, double distance)
{
	try {
		vehicle->moveFwd_med(distance);
	} catch (std::exception& ex) {
		if (Debug::Drivers) {

#ifndef SIMMOB_DISABLE_OUTPUT
			DebugStream << ">>>Exception: " << ex.what() << endl;
			boost::mutex::scoped_lock local_lock(sim_mob::Logger::global_mutex);
			std::cout << DebugStream.str();
#endif
			return false;
		}

		std::stringstream msg;
		msg << "Error moving vehicle forward for Agent ID: " << parent->getId() << ","
				<< this->vehicle->getX() << "," << this->vehicle->getY() << "\n" << ex.what();
		throw std::runtime_error(msg.str().c_str());
	}

	return true;
}

void sim_mob::medium::Driver::frame_tick(UpdateParams& p)
{
	DriverUpdateParams& p2 = dynamic_cast<DriverUpdateParams&>(p);

	const Lane* laneInfinity = nullptr;
	laneInfinity = vehicle->getCurrSegment()->getParentConflux()->
			getSegmentAgents()[vehicle->getCurrSegment()]->laneInfinity;

	if (vehicle && vehicle->hasPath()) {
		//at start vehicle will be in lane infinity. set origin will move it to the correct lane
		if (parent->getCurrLane() == laneInfinity){ //for now
			setOrigin(params);
		}
	} else {
		LogOut("ERROR: Vehicle could not be created for driver; no route!" <<std::endl);
	}

	//Are we done already?
	if (vehicle->isDone()) {
		std::cout << "removed when frame_tick is called" << std::endl;
		parent->setToBeRemoved();
		return;
	}

	//if vehicle is still in lane infinity, it shouldn't be advanced
	if (parent->getCurrLane() != laneInfinity)
	{
		advance(p2, p.now.ms());
		//Update parent data. Only works if we're not "done" for a bad reason.
		setParentData();

	}
	else{
		std::cout << "lane or vehicle is not set for driver " <<parent->getId() << std::endl;
	}
}

bool sim_mob::medium::Driver::advance(DriverUpdateParams& p, unsigned int currTimeMS){
	if (vehicle->isDone()) {
		parent->setToBeRemoved();
		return false;
	}

	if (vehicle->isQueuing)
	{
		return advanceQueuingVehicle(p, currTimeMS);
	}
	else //vehicle is moving
	{
		return advanceMovingVehicle(p, currTimeMS);
	}
}

bool sim_mob::medium::Driver::advanceQueuingVehicle(DriverUpdateParams& p, unsigned int currTimeMS){

	bool res = false;

	double t0 = p.timeThisTick;
	double x0 = vehicle->getPositionInSegment();
	double xf = 0.0;
	double tf = 0.0;

	//not implemented
	double output = getOutputCounter(currLane);
	//not implemented
	double outRate = getOutputFlowRate(currLane);
	//getCurrSegment length to be tested
	//getDistanceMovedinSeg to be updated with Max's method
	tf = t0 + x0/(vehicle->length*outRate); //assuming vehicle length is in cm
	if (output > 0 && tf < p.elapsedSeconds)
	{
		//not implemented
		res = moveToNextSegment(p, currTimeMS, tf);
		xf = (vehicle->isQueuing) ? vehicle->getPositionInSegment() : 0.0;
	}
	else
	{
		//not implemented
		moveInQueue();
		xf = vehicle->getPositionInSegment();
		tf = p.elapsedSeconds;
	}
	//unless it is handled previously;
	//1. update current position of vehicle/driver with xf
	//2. update current time, p.timeThisTick, with tf
	vehicle->setPositionInSegment(xf);
	p.timeThisTick = tf;

	return res;
}

bool sim_mob::medium::Driver::advanceMovingVehicle(DriverUpdateParams& p, unsigned int currTimeMS){

	bool res = false;
	double t0 = p.timeThisTick;
	double x0 = vehicle->getPositionInSegment();/* vehicle->getCurrSegment()->length - vehicle->getDistanceToSegmentStart();*/
	double xf = 0.0;
	double tf = 0.0;

	if(!currLane)
		throw std::runtime_error("agent's current lane is not set!");

	getSegSpeed();

	double vu = vehicle->getVelocity();

	//not implemented
	double output = getOutputCounter(currLane);

	//get current location
	//before checking if the vehicle should be added to a queue, it's re-assigned to the best lane
	double laneQueueLength = getQueueLength(currLane);

	if (laneQueueLength > vehicle->getCurrLinkLaneZeroLength() )
	{
		addToQueue(currLane);
		p.timeThisTick = p.elapsedSeconds;
	}
	else if (laneQueueLength > 0)
	{
		tf = t0 + (x0-laneQueueLength)/vu; //time to reach end of queue

		if (tf < p.elapsedSeconds)
		{
			addToQueue(currLane);
			p.timeThisTick = p.elapsedSeconds;
		}
		else
		{
			xf = x0 - vu * (tf - t0);
			res = moveInSegment(p, x0 - xf);
			vehicle->setPositionInSegment(xf);
			p.timeThisTick = p.elapsedSeconds;
		}
	}
	else if (getInitialQueueLength(currLane) > 0)
	{
		res = advanceMovingVehicleWithInitialQ(p, currTimeMS);
	}
	else //no queue or no initial queue
	{
		std::cout << "no queue" << std::endl;
		tf = t0 + x0/vu;
		if (tf < p.elapsedSeconds)
		{
			ss << "tf less than tick | output: " << output << endl;
			std::cout<<ss.str();
			ss.str("");

			if (output > 0)
			{
				p.timeThisTick = tf;
				res = moveToNextSegment(p, currTimeMS, tf);
			}
			else
			{
				std::cout<<"add to queue"<<std::endl;
				addToQueue(currLane);
				p.timeThisTick = p.elapsedSeconds;
			}
		}
		else
		{
			std::cout << "tf more than tick" << std::endl;
			tf = p.elapsedSeconds;
			xf = x0-vu*(tf-t0);
			res = moveInSegment(p, x0-xf);
			vehicle->setPositionInSegment(xf);
			p.timeThisTick = tf;
			//p2.currLaneOffset = vehicle->getDistanceMovedInSegment();
		}
	}
	//unless it is handled previously;
	//1. update current position of vehicle/driver with xf
	//2. update current time with tf
	//3.vehicle->moveFwd();
//	vehicle->setPositionInSegment(xf);
//	p.timeThisTick = tf;

	std::cout<< "end of advanceMoving - res:"<< (res? "True":"False") << std::endl;
	return res;
}

bool sim_mob::medium::Driver::advanceMovingVehicleWithInitialQ(DriverUpdateParams& p, unsigned int currTimeMS){

	bool res = false;
	double t0 = p.timeThisTick;
	double x0 = vehicle->getPositionInSegment(); /*vehicle->getCurrSegment()->length - vehicle->getDistanceToSegmentStart();*/
	double xf = 0.0;
	double tf = 0.0;

	getSegSpeed();
	double vu = vehicle->getVelocity();

	//not implemented yet
	//double laneQueueLength = getQueueLength(p2.currLane);

	//not implemented
	double output = getOutputCounter(currLane);
	double outRate = getOutputFlowRate(currLane);

	double timeToDissipateQ = getInitialQueueLength(currLane)/(outRate*vehicle->length); //assuming vehicle length is in cm
	double timeToReachEndSeg = t0 + x0/vu;
	tf = std::max(timeToDissipateQ, timeToReachEndSeg);

	if (tf < p.elapsedSeconds)
	{
		if (output > 0)
		{
			res = moveToNextSegment(p, currTimeMS, tf);
		}
		else
		{
			addToQueue(currLane);
		}
	}
	else
	{
		//cannot use == operator since it is double variable. tzl, Oct 18, 02
		if( fabs(tf-timeToReachEndSeg) < 0.001 && timeToReachEndSeg > p.elapsedSeconds)
		{
			tf = p.elapsedSeconds;
			xf = x0-vu*(tf-t0);
			res = moveInSegment(p, x0-xf);
			//p.currLaneOffset = vehicle->getDistanceMovedInSegment();
		}
		else
		{
			xf = 0.0 ;
			res = moveInSegment(p, x0-xf);
		}
	}
	//1. update current position of vehicle/driver with xf
	//2. update current time with tf
	vehicle->setPositionInSegment(xf);
	p.timeThisTick = tf;

	return res;
}

void sim_mob::medium::Driver::getSegSpeed(){
	// Fetch number of moving vehicles in the segment and compute speed from the speed density function
	//just for testing. need to remove later
/*	double density = vehicle->getCurrSegment()->getParentConflux()->
			getSegmentAgents()[vehicle->getCurrSegment()]->getDensity(true);
	double speed = vehicle->getCurrSegment()->getParentConflux()->
				getSegmentAgents()[vehicle->getCurrSegment()]->speed_density_function(true, density);
	*/
	vehicle->setVelocity(vehicle->getCurrSegment()->
			getParentConflux()->getSegmentSpeed(vehicle->getCurrSegment(), true));
	//vehicle->setVelocity(speed);
}

int sim_mob::medium::Driver::getOutputCounter(const Lane* l) {
	return l->getRoadSegment()->getParentConflux()->getOutputCounter(l);
}

double sim_mob::medium::Driver::getOutputFlowRate(const Lane* l) {
	return l->getRoadSegment()->getParentConflux()->getOutputFlowRate(l);
}

double sim_mob::medium::Driver::getAcceptRate(const Lane* l) {
	return l->getRoadSegment()->getParentConflux()->getAcceptRate(l);
}

double sim_mob::medium::Driver::getQueueLength(const Lane* l) {
	return ((l->getRoadSegment()->getParentConflux()->getLaneAgentCounts(l)).first) * (vehicle->length);
}

bool sim_mob::medium::Driver::isConnectedToNextSeg(const Lane* lane, const RoadSegment* nextRdSeg){
	if( !nextRdSeg)
		throw std::runtime_error("nextRdSeg is not available!");

	if (nextRdSeg->getLink() != lane->getRoadSegment()->getLink()){
		const MultiNode* currEndNode = dynamic_cast<const MultiNode*> (lane->getRoadSegment()->getEnd());
		if (currEndNode && nextRdSeg) {
			const set<LaneConnector*>& lcs = currEndNode->getOutgoingLanes((lane->getRoadSegment()));
			for (set<LaneConnector*>::const_iterator it = lcs.begin(); it != lcs.end(); it++) {
				if ((*it)->getLaneTo()->getRoadSegment() == nextRdSeg && (*it)->getLaneFrom() == lane) {
					return true;
				}
			}
		}
	}
	else{ //handling uni-nodes - where lanes should be connected to the same outgoing segment
		if (lane->getRoadSegment()->getLink() == nextRdSeg->getLink())
			return true;
	}
	return false;
}

double sim_mob::medium::Driver::getInitialQueueLength(const Lane* l) {
	return l->getRoadSegment()->getParentConflux()->getInitialQueueCount(l) * vehicle->length;
}

double sim_mob::medium::Driver::getLastAccept(const Lane* l){
	return l->getRoadSegment()->getParentConflux()->getLastAccept(l);
}

void sim_mob::medium::Driver::setLastAccept(const Lane* l, double lastAccept){
	l->getRoadSegment()->getParentConflux()->setLastAccept(l, lastAccept);
}
