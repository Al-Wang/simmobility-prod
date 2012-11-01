/* Copyright Singapore-MIT Alliance for Research and Technology */

#include "BusDriver.hpp"
#include <vector>

#include "DriverUpdateParams.hpp"

#include "entities/Person.hpp"
#include "entities/vehicle/BusRoute.hpp"
#include "entities/vehicle/Bus.hpp"
#include "geospatial/Point2D.hpp"
#include "geospatial/BusStop.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#include "util/DebugFlags.hpp"

using namespace sim_mob;
using std::vector;
using std::map;
using std::string;

namespace {
const int BUS_STOP_WAIT_PASSENGER_TIME_SEC = 2;
} //End anonymous namespace

sim_mob::BusDriver::BusDriver(Person* parent, MutexStrategy mtxStrat)
	: Driver(parent, mtxStrat), nextStop(nullptr), waitAtStopMS(-1) , lastTickDistanceToBusStop(-1)
, lastVisited_BusStop(mtxStrat,nullptr), lastVisited_BusStopSequenceNum(mtxStrat,0), real_DepartureTime(mtxStrat,0)
, real_ArrivalTime(mtxStrat,0)
{
}


Role* sim_mob::BusDriver::clone(Person* parent) const
{
	return new BusDriver(parent, parent->getMutexStrategy());
}


Vehicle* sim_mob::BusDriver::initializePath_bus(bool allocateVehicle)
{
	Vehicle* res = nullptr;

	//Only initialize if the next path has not been planned for yet.
	if(!parent->getNextPathPlanned()) {
		//Save local copies of the parent's origin/destination nodes.
//		origin.node = parent->originNode;
//		origin.point = origin.node->location;
//		goal.node = parent->destNode;
//		goal.point = goal.node->location;
		vector<const RoadSegment*> path;
		Person* person = dynamic_cast<Person*>(parent);
		int vehicle_id = 0;
		if(person) {
			const BusTrip* bustrip = dynamic_cast<const BusTrip*>(person->currTripChainItem);
			if(bustrip && person->currTripChainItem->itemType==TripChainItem::IT_BUSTRIP) {
				path = bustrip->getBusRouteInfo().getRoadSegments();
				vehicle_id = bustrip->getVehicleID();
			}
		}

		//Retrieve the shortest path from origin to destination and save all RoadSegments in this path.
//		vector<WayPoint> path;
//		Person* parentP = dynamic_cast<Person*> (parent);
//		if (!parentP || parentP->specialStr.empty()) {
//			path = StreetDirectory::instance().shortestDrivingPath(*origin.node, *goal.node);
//		} else {
//			//Retrieve the special string.
//			size_t cInd = parentP->specialStr.find(':');
//			string specialType = parentP->specialStr.substr(0, cInd);
//			string specialValue = parentP->specialStr.substr(cInd, std::string::npos);
//			if (specialType=="loop") {
//				initLoopSpecialString(path, specialValue);
//			} else if (specialType=="tripchain") {
//				path = StreetDirectory::instance().shortestDrivingPath(*origin.node, *goal.node);
//				int x = path.size();
//				initTripChainSpecialString(specialValue);
//			} else {
//				throw std::runtime_error("Unknown special string type.");
//			}
//		}

		//TODO: Start in lane 0?
		int startlaneID = 0;

		// Bus should be at least 1200 to be displayed on Visualizer
		const double length = dynamic_cast<BusDriver*>(this) ? 1200 : 400;
		const double width = 200;

		//A non-null vehicle means we are moving.
		if (allocateVehicle) {
			res = new Vehicle(path, startlaneID, vehicle_id, length, width);
		}
	}

	//to indicate that the path to next activity is already planned
	parent->setNextPathPlanned(true);
	return res;
}

//We're recreating the parent class's frame_init() method here.
// Our goal is to reuse as much of Driver as possible, and then
// refactor common code out later. We could just call the frame_init() method
// directly, but there's some unexpected interdependencies.
void sim_mob::BusDriver::frame_init(UpdateParams& p)
{
	//TODO: "initializePath()" in Driver mixes initialization of the path and
	//      creation of the Vehicle (e.g., its width/height). These are both
	//      very different for Cars and Buses, but until we un-tangle the code
	//      we'll need to rely on hacks like this.
	//Vehicle* newVeh = initializePath(true);
	Vehicle* newVeh = initializePath_bus(true);

	//Save the path, create a vehicle.
	if (newVeh) {
		//Use this sample vehicle to build our Bus, then delete the old vehicle.
		BusRoute nullRoute; //Buses don't use the route at the moment.
		vehicle = new Bus(nullRoute, newVeh);
		delete newVeh;

		//This code is used by Driver to set a few properties of the Vehicle/Bus.
		if (!vehicle->hasPath()) {
			throw std::runtime_error("Vehicle could not be created for bus driver; no route!");
		}

		//Set the bus's origin and set of stops.
		setOrigin(params);
		busStops = findBusStopInPath(vehicle->getCompletePath());

		//Unique to BusDrivers: reset your route
		waitAtStopMS = 0.0;
	}
}

vector<const BusStop*> sim_mob::BusDriver::findBusStopInPath(const vector<const RoadSegment*>& path) const
{
	//NOTE: Use typedefs instead of defines.
	typedef vector<const BusStop*> BusStopVector;

	BusStopVector res;
	int busStopAmount = 0;
	vector<const RoadSegment*>::const_iterator it;
	for( it = path.begin(); it != path.end(); ++it)
	{
		// get obstacles in road segment
		const RoadSegment* rs = (*it);
		const std::map<centimeter_t, const RoadItem*> & obstacles = rs->obstacles;

		//Check each of these.
		std::map<centimeter_t, const RoadItem*>::const_iterator ob_it;
		for(ob_it = obstacles.begin(); ob_it != obstacles.end(); ++ob_it)
		{
			RoadItem* ri = const_cast<RoadItem*>(ob_it->second);
			BusStop *bs = dynamic_cast<BusStop*>(ri);
			if(bs) {
				// calculate bus stops point
				//NOTE: This is already calculated in the offset (RoadSegment obstacles list) ~Seth
				/*DynamicVector SegmentLength(rs->getEnd()->location.getX(),rs->getEnd()->location.getY(),rs->getStart()->location.getX(),rs->getStart()->location.getY());
				DynamicVector BusStopDistfromStart(bs->xPos,bs->yPos,rs->getStart()->location.getX(),rs->getStart()->location.getY());
				DynamicVector BusStopDistfromEnd(rs->getEnd()->location.getX(),rs->getEnd()->location.getY(),bs->xPos,bs->yPos);
				double a = BusStopDistfromStart.getMagnitude();
				double b = BusStopDistfromEnd.getMagnitude();
				double c = SegmentLength.getMagnitude();
				bs->stopPoint = (-b*b + a*a + c*c)/(2.0*c);*/
				//std::cout<<"stopPoint: "<<bs->stopPoint/100.0<<std::endl;
				res.push_back(bs);
			}
		}
	}
	return res;
}
double sim_mob::BusDriver::linkDriving(DriverUpdateParams& p) {

	if (!vehicle->hasNextSegment(true)) {
		p.dis2stop = vehicle->getAllRestRoadSegmentsLength()
				   - vehicle->getDistanceMovedInSegment() - vehicle->length / 2 - 300;
		if (p.nvFwd.distance < p.dis2stop)
			p.dis2stop = p.nvFwd.distance;
		p.dis2stop /= 100;
	} else {
		p.nextLaneIndex = std::min<int>(p.currLaneIndex, vehicle->getNextSegment()->getLanes().size() - 1);
		if (vehicle->getNextSegment()->getLanes().at(p.nextLaneIndex)->is_pedestrian_lane()) {
			p.nextLaneIndex--;
			p.dis2stop = vehicle->getCurrPolylineLength() - vehicle->getDistanceMovedInSegment() + 1000;
		} else
			p.dis2stop = 1000;//defalut 1000m
	}

	//when vehicle stops, don't do lane changing
	if (vehicle->getVelocity() <= 0) {
		vehicle->setLatVelocity(0);
	}
	p.turningDirection = vehicle->getTurningDirection();

	//hard code , need solution
	p.space = 50;

	//get nearest car, if not making lane changing, the nearest car should be the leading car in current lane.
	//if making lane changing, adjacent car need to be taken into account.
	NearestVehicle & nv = nearestVehicle(p);
	if (nv.distance <= 0) {
		//if (nv.driver->parent->getId() > this->parent->getId())
		if (getDriverParent(nv.driver)->getId() > this->parent->getId()) {
			nv = NearestVehicle();
		}
	}

	perceivedDataProcess(nv, p);

	//bus approaching bus stop reduce speed
	//and if its left has lane, merge to left lane
	if (isBusFarawayBusStop()) {
		busAccelerating(p);
	}

	//NOTE: Driver already has a lcModel; we should be able to just use this. ~Seth
	LANE_CHANGE_SIDE lcs = LCS_SAME;
	MITSIM_LC_Model* mitsim_lc_model = dynamic_cast<MITSIM_LC_Model*> (lcModel);
	if (mitsim_lc_model) {
		lcs = mitsim_lc_model->makeMandatoryLaneChangingDecision(p);
	} else {
		throw std::runtime_error("TODO: BusDrivers currently require the MITSIM lc model.");
	}

	vehicle->setTurningDirection(lcs);
	double newLatVel;
	newLatVel = lcModel->executeLaneChanging(p, vehicle->getAllRestRoadSegmentsLength(), vehicle->length, vehicle->getTurningDirection());
	vehicle->setLatVelocity(newLatVel * 10);
//	std::cout << "BusDriver::updatePositionOnLink:current lane: "
//			  << p.currLaneIndex << " lat velo: " << newLatVel / 100.0 << "m/s"
//			  << std::endl;
	if (isBusApproachingBusStop()) {
		busAccelerating(p);

		//move to most left lane
		p.nextLaneIndex = vehicle->getCurrSegment()->getLanes().back()->getLaneID();
		LANE_CHANGE_SIDE lcs = mitsim_lc_model->makeMandatoryLaneChangingDecision(p);
		vehicle->setTurningDirection(lcs);
		double newLatVel;
		newLatVel = mitsim_lc_model->executeLaneChanging(p, vehicle->getAllRestRoadSegmentsLength(), vehicle->length, vehicle->getTurningDirection());
		vehicle->setLatVelocity(newLatVel*5);


//		std::cout << "BusDriver::updatePositionOnLink: bus approaching current lane: "
//				  << p.currLaneIndex << std::endl;

		// reduce speed
		if (vehicle->getVelocity() / 100.0 > 10)
			vehicle->setAcceleration(-500);

		waitAtStopMS = 0;
	}
	if (isBusArriveBusStop() && waitAtStopMS >= 0 && waitAtStopMS
			< BUS_STOP_WAIT_PASSENGER_TIME_SEC) {
//		std::cout
//				<< "BusDriver::updatePositionOnLink: bus isBusArriveBusStop velocity: "
//				<< vehicle->getVelocity() / 100.0 << std::endl;
//		real_ArrivalTime.set(p.currTimeMS);// BusDriver set RealArrival Time, set once(the first time comes in)
//		if(!BusController::all_busctrllers_.empty()) {
//			BusController::all_busctrllers_[0]->receiveBusInformation(1, 0, 0, p.currTimeMS);
//			unsigned int departureTime = BusController::all_busctrllers_[0]->decisionCalculation(1, 0, 0, p.currTimeMS,lastVisited_BusStopSequenceNum.get());// need to be changed, only calculate once(no need every time calculation)
//			real_DepartureTime.set(departureTime);// BusDriver set RealDeparture Time
//		}
		if (vehicle->getVelocity() > 0)
			vehicle->setAcceleration(-5000);
		if (vehicle->getVelocity() < 0.1 && waitAtStopMS < BUS_STOP_WAIT_PASSENGER_TIME_SEC) {
			waitAtStopMS = waitAtStopMS + p.elapsedSeconds;
//			std::cout << "BusDriver::updatePositionOnLink: waitAtStopMS: " << waitAtStopMS << " p.elapsedSeconds: " << p.elapsedSeconds << std::endl;

			//Pick up a semi-random number of passengers
			Bus* bus = dynamic_cast<Bus*>(vehicle);
			if ((waitAtStopMS == p.elapsedSeconds) && bus) {
//				std::cout << "BusDriver::updatePositionOnLink: pich up passengers" << std::endl;
				real_ArrivalTime.set(p.currTimeMS);// BusDriver set RealArrival Time, set once(the first time comes in)

				int pCount = reinterpret_cast<intptr_t> (vehicle) % 50;
				bus->setPassengerCount(pCount);

				if(!BusController::all_busctrllers_.empty()) {
					BusController::all_busctrllers_[0]->receiveBusInformation("", 0, 0, p.currTimeMS);
					//unsigned int departureTime = BusController::all_busctrllers_[0]->decisionCalculation(1, 0, 0, p.currTimeMS,lastVisited_BusStopSequenceNum.get());// need to be changed, only calculate once(no need every time calculation)
					//real_DepartureTime.set(departureTime);// BusDriver set RealDeparture Time
				}
			}
		}
	} else if (isBusArriveBusStop()) {
		vehicle->setAcceleration(3000);
	}

	//TODO: Please check from here; the merge was not 100% clean. ~Seth
	if (isBusLeavingBusStop() || waitAtStopMS >= BUS_STOP_WAIT_PASSENGER_TIME_SEC) {
		std::cout << "BusDriver::updatePositionOnLink: bus isBusLeavingBusStop" << std::endl;
		waitAtStopMS = -1;

		busAccelerating(p);
	}

	//Update our distance
	lastTickDistanceToBusStop = distanceToNextBusStop();

/*	std::cout<<"BusDriver::updatePositionOnLink:tick: "<<p.currTimeMS/1000.0<<std::endl;
	std::cout<<"BusDriver::updatePositionOnLink:busvelocity: "<<vehicle->getVelocity()/100.0<<std::endl;
	std::cout<<"BusDriver::updatePositionOnLink:busacceleration: "<<vehicle->getAcceleration()/100.0<<std::endl;
	std::cout<<"BusDriver::updatePositionOnLink:buslateralvelocity: "<<vehicle->getLatVelocity()/100.0<<std::endl;
	std::cout<<"BusDriver::updatePositionOnLink:busstopdistance: "<<distanceToNextBusStop()<<std::endl;
	std::cout<<"my bus distance moved in segment: "<<vehicle->getDistanceToSegmentStart()/100.0<<std::endl;
	std::cout<<"but DistanceMovedInSegment"<<vehicle->getDistanceMovedInSegment()/100.0<<std::endl;
	std::cout<<"current polyline length: "<<vehicle->getCurrPolylineLength()/100.0<<std::endl;*/
	DynamicVector segmentlength(vehicle->getCurrSegment()->getStart()->location.getX(),vehicle->getCurrSegment()->getStart()->location.getY(),
			vehicle->getCurrSegment()->getEnd()->location.getX(),vehicle->getCurrSegment()->getEnd()->location.getY());
//	std::cout<<"current segment length: "<<segmentlength.getMagnitude()/100.0<<std::endl;

//	double fwdDistance = vehicle->getVelocity() * p.elapsedSeconds + 0.5 * vehicle->getAcceleration() * p.elapsedSeconds * p.elapsedSeconds;
//	if (fwdDistance < 0)
//		fwdDistance = 0;

	//double fwdDistance = vehicle->getVelocity()*p.elapsedSeconds;
//	double latDistance = vehicle->getLatVelocity() * p.elapsedSeconds;

	//Increase the vehicle's velocity based on its acceleration.


	//Return the remaining amount (obtained by calling updatePositionOnLink)
	return updatePositionOnLink(p);
}

double sim_mob::BusDriver::getPositionX() const
{
	return vehicle ? vehicle->getX() : 0;
}

double sim_mob::BusDriver::getPositionY() const
{
	return vehicle ? vehicle->getY() : 0;
}


void sim_mob::BusDriver::busAccelerating(DriverUpdateParams& p)
{
	//Retrieve a new acceleration value.
	double newFwdAcc = 0;

	//Convert back to m/s
	//TODO: Is this always m/s? We should rename the variable then...
	p.currSpeed = vehicle->getVelocity() / 100;

	//Call our model
	newFwdAcc = cfModel->makeAcceleratingDecision(p, targetSpeed, maxLaneSpeed);

	//Update our chosen acceleration; update our position on the link.
	vehicle->setAcceleration(newFwdAcc * 100);
}

bool sim_mob::BusDriver::isBusFarawayBusStop() const
{
	bool res = false;
	double distance = distanceToNextBusStop();
	if (distance < 0 || distance > 50)
		res = true;

	return res;
}

bool sim_mob::BusDriver::isBusApproachingBusStop() const
{
	double distance = distanceToNextBusStop();
	if (distance >=10 && distance <= 50) {
		if (lastTickDistanceToBusStop < 0) {
			return true;
		} else if (lastTickDistanceToBusStop > distance) {
			return true;
		}
	}
	return false;
}

bool sim_mob::BusDriver::isBusArriveBusStop() const
{
	double distance = distanceToNextBusStop();
	return  (distance>0 && distance <10);
}

bool sim_mob::BusDriver::isBusLeavingBusStop() const
{
	double distance = distanceToNextBusStop();
	return distance>=10 && distance<50 && lastTickDistanceToBusStop<distance;
}

double sim_mob::BusDriver::distanceToNextBusStop() const
{
	double distanceToCurrentSegmentBusStop = getDistanceToBusStopOfSegment(vehicle->getCurrSegment());
	double distanceToNextSegmentBusStop;
	if (vehicle->hasNextSegment(true))
		distanceToNextSegmentBusStop = getDistanceToBusStopOfSegment(vehicle->getNextSegment(true));

	if (distanceToCurrentSegmentBusStop >= 0 && distanceToNextSegmentBusStop >= 0) {
		return ((distanceToCurrentSegmentBusStop<=distanceToNextSegmentBusStop) ? distanceToCurrentSegmentBusStop: distanceToNextSegmentBusStop);
	} else if (distanceToCurrentSegmentBusStop > 0) {
		return distanceToCurrentSegmentBusStop;
	}

	return distanceToNextSegmentBusStop;
}

double sim_mob::BusDriver::getDistanceToBusStopOfSegment(const RoadSegment* rs) const
{

	double distance = -100;
	double currentX = vehicle->getX();
	double currentY = vehicle->getY();
//	bus->getRoute().getCurrentStop();

	std::cout.precision(10);
//		std::cout<<"BusDriver::DistanceToNextBusStop : current bus position: "<<currentX<<"  "<<currentY<<std::endl;
//		std::cout<<"BusDriver::DistanceToNextBusStop : seg start: "<<rs->getStart()->location.getX()<<"  "
//		        		<<rs->getStart()->location.getY()<<" rs end: "<<rs->getEnd()->location.getX()<<"  "
//		        		<<rs->getEnd()->location.getY()<<std::endl;
		const std::map<centimeter_t, const RoadItem*> & obstacles = rs->obstacles;
//		int i = 1;
		for(std::map<centimeter_t, const RoadItem*>::const_iterator o_it = obstacles.begin(); o_it != obstacles.end() ; o_it++)
		{
		   RoadItem* ri = const_cast<RoadItem*>(o_it->second);
		   BusStop *bs = dynamic_cast<BusStop *>(ri);
		   int stopPoint = o_it->first;

		   if(bs)
		   {
			   if (rs == vehicle->getCurrSegment())
			   {

				   if (stopPoint < 0)
				   {
					   throw std::runtime_error("BusDriver offset in obstacles list should never be <0");
					   /*std::cout<<"BusDriver::DistanceToNextBusStop :stopPoint < 0"<<std::endl;
					   DynamicVector SegmentLength(rs->getEnd()->location.getX(),rs->getEnd()->location.getY(),rs->getStart()->location.getX(),rs->getStart()->location.getY());
					   DynamicVector BusStopDistfromStart(bs->xPos,bs->yPos,rs->getStart()->location.getX(),rs->getStart()->location.getY());
					   DynamicVector BusStopDistfromEnd(rs->getEnd()->location.getX(),rs->getEnd()->location.getY(),bs->xPos,bs->yPos);
					   double a = BusStopDistfromStart.getMagnitude();
					   double b = BusStopDistfromEnd.getMagnitude();
					   double c = SegmentLength.getMagnitude();
					   bs->stopPoint = (-b*b + a*a + c*c)/(2.0*c);*/
				   }

				   if (stopPoint >= 0)
				   {
						DynamicVector BusDistfromStart(vehicle->getX(),vehicle->getY(),rs->getStart()->location.getX(),rs->getStart()->location.getY());
						//std::cout<<"BusDriver::DistanceToNextBusStop: bus move in segment: "<<BusDistfromStart.getMagnitude()<<std::endl;
						//distance = bs->stopPoint - BusDistfromStart.getMagnitude();
						distance = stopPoint - vehicle->getDistanceMovedInSegment();
						//std::cout<<"BusDriver::DistanceToNextBusStop :distance: "<<distance<<std::endl;
					}
			   }
			   else
			   {
				   DynamicVector busToSegmentStartDistance(currentX,currentY,
						   rs->getStart()->location.getX(),rs->getStart()->location.getY());
//				   DynamicVector busToSegmentEndDistance(currentX,currentY,
//				   						   rs->getEnd()->location.getX(),rs->getEnd()->location.getY());
				   //distance = busToSegmentStartDistance.getMagnitude() + bs->stopPoint;
				   distance = vehicle->getCurrentSegmentLength() - vehicle->getDistanceMovedInSegment() + stopPoint;
/*				   std::cout<<"BusDriver::DistanceToNextBusStop :not current segment distance:stopPoint "<<bs->stopPoint<<std::endl;
				   std::cout<<"BusDriver::DistanceToNextBusStop :not current segment distance:busToSegmentStartDistance: "<<busToSegmentStartDistance.getMagnitude()<<std::endl;
				   std::cout<<"BusDriver::DistanceToNextBusStop :not current segment distance: "<<distance<<std::endl;*/
			   }

////			   std::cout<<"BusDriver::DistanceToNextBusStop: find bus stop <"<<i<<"> in segment"<<std::endl;
//			   double busStopX = bs->xPos;
//			   double busStopY = bs->yPos;
////				std::cout<<"BusDriver::DistanceToNextBusStop : bus stop position: "<<busStopX<<"  "<<busStopY<<std::endl;
//			   double dis = sqrt((currentX-busStopX)*(currentX-busStopX) + (currentY-busStopY)*(currentY-busStopY));
//			   if (distance < 0 || dis < distance) // in case more than one stop at the segment
//				   distance = dis;
////				std::cout<<"BusDriver::DistanceToNextBusStop : distance: "<<distance/100.0<<std::endl;
//			   i++;
		   }
		}

		return distance/100.0;
//	typedef std::map<centimeter_t, const RoadItem*>::const_iterator RoadObstIt;
//	if (!roadSegment) { return -1; }
//
//	//The obstacle offset now correctly returns the BusStop's distance down the segment.
//	for(RoadObstIt o_it = roadSegment->obstacles.begin(); o_it!=roadSegment->obstacles.end(); o_it++) {
//		const BusStop *bs = dynamic_cast<const BusStop *>(o_it->second);
//		if (bs) {
//			DynamicVector BusDistfromStart(getPositionX(), getPositionY(),roadSegment->getStart()->location.getX(),roadSegment->getStart()->location.getY());
//			std::cout<<"BusDriver::DistanceToNextBusStop: bus move in segment: "<<BusDistfromStart.getMagnitude()<<std::endl;
//			return  (o_it->first - BusDistfromStart.getMagnitude()) / 100.0;
//		}
//	}
//
//	return -1;
}

//Main update functionality
void sim_mob::BusDriver::frame_tick(UpdateParams& p)
{
	//NOTE: If this is all that is doen, we can simply delete this function and
	//      let its parent handle it automatically. ~Seth
	Driver::frame_tick(p);
}

void sim_mob::BusDriver::frame_tick_output(const UpdateParams& p)
{
	//Skip?
	if (vehicle->isDone() || ConfigParams::GetInstance().is_run_on_many_computers) {
		return;
	}

#ifndef SIMMOB_DISABLE_OUTPUT
	double baseAngle = vehicle->isInIntersection() ? intModel->getCurrentAngle() : vehicle->getAngle();
	Bus* bus = dynamic_cast<Bus*>(vehicle);
	LogOut("(\"BusDriver\""
			<<","<<p.frameNumber
			<<","<<parent->getId()
			<<",{"
			<<"\"xPos\":\""<<static_cast<int>(bus->getX())
			<<"\",\"yPos\":\""<<static_cast<int>(bus->getY())
			<<"\",\"angle\":\""<<(360 - (baseAngle * 180 / M_PI))
			<<"\",\"length\":\""<<static_cast<int>(3*bus->length)
			<<"\",\"width\":\""<<static_cast<int>(2*bus->width)
			<<"\",\"passengers\":\""<<(bus?bus->getPassengerCount():0)
			<<"\"})"<<std::endl);
#endif
}

void sim_mob::BusDriver::frame_tick_output_mpi(frame_t frameNumber)
{
	//Skip output?
	if (frameNumber<parent->getStartTime() || vehicle->isDone()) {
		return;
	}

#ifndef SIMMOB_DISABLE_OUTPUT
	double baseAngle = vehicle->isInIntersection() ? intModel->getCurrentAngle() : vehicle->getAngle();

	//The BusDriver class will only maintain buses as the current vehicle.
	const Bus* bus = dynamic_cast<const Bus*>(vehicle);

	std::stringstream logout;
	logout << "(\"Driver\"" << "," << frameNumber << "," << parent->getId() << ",{" << "\"xPos\":\""
			<< static_cast<int> (bus->getX()) << "\",\"yPos\":\"" << static_cast<int> (bus->getY())
			<< "\",\"segment\":\"" << bus->getCurrSegment()->getId()
			<< "\",\"angle\":\"" << (360 - (baseAngle * 180 / M_PI)) << "\",\"length\":\""
			<< static_cast<int> (bus->length) << "\",\"width\":\"" << static_cast<int> (bus->width)
			<<"\",\"passengers\":\""<<(bus?bus->getPassengerCount():0);

	if (this->parent->isFake) {
		logout << "\",\"fake\":\"" << "true";
	} else {
		logout << "\",\"fake\":\"" << "false";
	}

	logout << "\"})" << std::endl;

	LogOut(logout.str());
#endif
}

