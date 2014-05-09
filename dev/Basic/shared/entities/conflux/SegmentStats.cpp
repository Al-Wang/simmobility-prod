//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "SegmentStats.hpp"

#include <cmath>
#include <algorithm>

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "entities/vehicle/VehicleBase.hpp"
#include "logging/Log.hpp"


using std::string;

namespace{
    const double INFINITESIMAL_DOUBLE = 0.000001;
    const double SHORT_SEGMENT_LENGTH_LIMIT = 5 * sim_mob::PASSENGER_CAR_UNIT; // 5 times a car's length
    const double LARGE_OUTPUT_FLOW_RATE = 1000.0;

    /**
     * converts the unit of speed from Km/h to cm/s
     * @param speedInKmph spped in Km/h
     * @return speed in cm/s
     */
    inline double convertKmphToCmps(double speedInKmph) {
    	return (speedInKmph / 3.6 * 100);
    }
}

namespace sim_mob {

bool cmp_person_distToSegmentEnd::operator ()
		(const Person* x, const Person* y) const {
	if ((!x) || (!y)) {
		std::stringstream debugMsgs;
		debugMsgs
				<< "cmp_person_remainingTimeThisTick: Comparison failed because at least one of the arguments is null"
				<< "|x: " << (x ? x->getId() : 0) << "|y: "
				<< (y ? y->getId() : 0);
		throw std::runtime_error(debugMsgs.str());
	}
	//person x > y iff x's distance to end of segment is greater than y's
	return (x->distanceToEndOfSegment > y->distanceToEndOfSegment);
}

/**
 * The parameters - min density, jam density, alpha and beta -
 * must be obtained for each road segment from an external source (XML/Database)
 * Since we don't have this data, we have taken the average values from
 * supply parameters of Singapore expressways.
 *
 * TODO: This must be changed when we have this information for each
 * road segment in the network.
 */
SupplyParams::SupplyParams(const sim_mob::RoadSegment* rdSeg, double statsLength)
	: freeFlowSpeed(convertKmphToCmps(rdSeg->maxSpeed)),
	  minSpeed(0.3 * freeFlowSpeed), /*30% of free flow speed as suggested by Yang Lu*/
	  jamDensity(0.2), /*density during traffic jam in veh/meter*/
	  minDensity(0.0048), /*minimum traffic density in veh/meter*/
	  capacity(rdSeg->getCapacity() / 3600.0), /*converting capacity to vehicles/hr to vehicles/s*/
	  alpha(1.8),
	  beta(1.9)
{}

SegmentStats::SegmentStats(const sim_mob::RoadSegment* rdSeg, double statslength)
	: roadSegment(rdSeg), length(statslength), segDensity(0.0), segPedSpeed(0.0),
	segFlow(0), numPersons(0), positionInRoadSegment(1),
	debugMsgs(std::stringstream::out), supplyParams(rdSeg, statslength),
	orderBySetting(SEGMENT_ORDERING_BY_DISTANCE_TO_INTERSECTION)
{
	segVehicleSpeed = convertKmphToCmps(getRoadSegment()->maxSpeed);
	numVehicleLanes = 0;

	// initialize LaneAgents in the map
	std::vector<sim_mob::Lane*>::const_iterator lane = rdSeg->getLanes().begin();
	while (lane != rdSeg->getLanes().end()) {
		laneStatsMap.insert(std::make_pair(*lane, new sim_mob::LaneStats(*lane, length)));
		laneStatsMap[*lane]->initLaneParams(segVehicleSpeed, supplyParams.getCapacity());
		if (!(*lane)->is_pedestrian_lane()) {
			numVehicleLanes++;
		}
		lane++;
	}

	/*
	 * Any lane with an id ending with 9 is laneInfinity of the road segment.
	 * TODO: Must check if we can have a bit pattern (Refer lane constructor) for laneInfinity.
	 */
	laneInfinity = new sim_mob::Lane(const_cast<sim_mob::RoadSegment*>(rdSeg), 9);
	laneStatsMap.insert(std::make_pair(laneInfinity, new sim_mob::LaneStats(laneInfinity, statslength, true)));
}

SegmentStats::~SegmentStats() {
	for(LaneStatsMap::iterator i=laneStatsMap.begin(); i!=laneStatsMap.end(); i++) {
		safe_delete_item(i->second);
	}
	safe_delete_item(laneInfinity);
}

void SegmentStats::addAgent(const sim_mob::Lane* lane, sim_mob::Person* p) {
	laneStatsMap.find(lane)->second->addPerson(p);
	numPersons++; //record addition to segment
}

void SegmentStats::removeAgent(const sim_mob::Lane* lane, sim_mob::Person* p, bool wasQueuing) {
	laneStatsMap.find(lane)->second->removePerson(p, wasQueuing);
	numPersons--; //record removal from segment
}

void SegmentStats::updateQueueStatus(const sim_mob::Lane* lane, sim_mob::Person* p) {
	laneStatsMap.find(lane)->second->updateQueueStatus(p);
}

std::deque<sim_mob::Person*>& SegmentStats::getPersons(const sim_mob::Lane* lane) {
	return laneStatsMap.find(lane)->second->laneAgents;
}

std::deque<sim_mob::Person*> SegmentStats::getPersons() {
	PersonList segAgents;
	for(std::vector<sim_mob::Lane*>::const_iterator lnIt=roadSegment->getLanes().begin();
			lnIt != roadSegment->getLanes().end(); lnIt++) {
		PersonList& lnAgents = laneStatsMap.find(*lnIt)->second->laneAgents;
		segAgents.insert(segAgents.end(), lnAgents.begin(), lnAgents.end());
	}
	PersonList& lnAgents = laneStatsMap.find(laneInfinity)->second->laneAgents;
	segAgents.insert(segAgents.end(), lnAgents.begin(), lnAgents.end());

	return segAgents;
}

void SegmentStats::topCMergeLanesInSegment(PersonList& mergedPersonList) {
	int capacity = (int)(ceil(roadSegment->getCapacityPerInterval()));
	std::vector<PersonList::iterator> iteratorLists;

	//init iterator list to the front of each lane
	for(LaneStatsMap::iterator lnIt = laneStatsMap.begin(); lnIt != laneStatsMap.end(); lnIt++) {
		iteratorLists.push_back(lnIt->second->laneAgents.begin());
	}

	//pick the Top C
	for (int c = 0; c < capacity; c++) {
		int dequeIndex = -1;
		double minDistance = std::numeric_limits<double>::max();
		sim_mob::Person* minPerson = nullptr;
		int i = 0;
		for(LaneStatsMap::iterator lnIt = laneStatsMap.begin(); lnIt != laneStatsMap.end(); lnIt++) {
			PersonList& personsInLane = lnIt->second->laneAgents;
			//order by location
			if (orderBySetting == SEGMENT_ORDERING_BY_DISTANCE_TO_INTERSECTION) {
				if (iteratorLists[i] != personsInLane.end() && (*iteratorLists[i])->distanceToEndOfSegment < minDistance) {
					dequeIndex = i;
					minPerson = (*(iteratorLists[i]));
					minDistance = minPerson->distanceToEndOfSegment;

				}
			}
			//order by time
			else if (orderBySetting == SEGMENT_ORDERING_BY_DRIVING_TIME_TO_INTERSECTION) {
				if (iteratorLists[i] != personsInLane.end() && (*iteratorLists[i])->drivingTimeToEndOfLink < minDistance) {
					dequeIndex = i;
					minPerson = (*(iteratorLists[i]));
					minDistance = minPerson->drivingTimeToEndOfLink;
				}
			}
			i++;
		}

		if (dequeIndex < 0) {
			return; //no more vehicles
		} else {
			iteratorLists.at(dequeIndex)++;
			mergedPersonList.push_back(minPerson);
		}
	}

	//After picking the Top C, just append the remaining vehicles in the output list
	int i = 0;
	for(LaneStatsMap::iterator lnIt = laneStatsMap.begin(); lnIt != laneStatsMap.end(); lnIt++) {
		PersonList& personsInLane = lnIt->second->laneAgents;
		if (iteratorLists[i] != personsInLane.end()) {
			mergedPersonList.insert(mergedPersonList.end(), iteratorLists[i], personsInLane.end());
		}
		i++;
	}
}


std::pair<unsigned int, unsigned int> SegmentStats::getLaneAgentCounts(const sim_mob::Lane* lane) const {
	return std::make_pair(laneStatsMap.at(lane)->getQueuingAgentsCount(), laneStatsMap.at(lane)->getMovingAgentsCount());
}

unsigned int SegmentStats::numAgentsInLane(const sim_mob::Lane* lane) const {
	return laneStatsMap.at(lane)->getNumPersons();
}

unsigned int SegmentStats::numMovingInSegment(bool hasVehicle) const {
	unsigned int movingCounts = 0;
	const std::vector<sim_mob::Lane*>& segLanes = roadSegment->getLanes();
	std::vector<sim_mob::Lane*>::const_iterator laneIt = segLanes.begin();
	while (laneIt != segLanes.end()) {
		if ((hasVehicle && !(*laneIt)->is_pedestrian_lane()) || (!hasVehicle && (*laneIt)->is_pedestrian_lane())) {
			LaneStatsMap::const_iterator laneStatsIt = laneStatsMap.find(*laneIt);
			if (laneStatsIt != laneStatsMap.end()) {
				movingCounts = movingCounts + laneStatsIt->second->getMovingAgentsCount();
			}
			else {
				throw std::runtime_error("SegmentStats::numMovingInSegment called with invalid laneStats.");
			}
		}
		laneIt++;
	}
	return movingCounts;
}

//density will be computed in vehicles/meter
double SegmentStats::getDensity(bool hasVehicle) {
	double density = 0.0;
	unsigned int queueCount = numQueuingInSegment(true);
	double movingLength = length*numVehicleLanes - queueCount*PASSENGER_CAR_UNIT;
	if(movingLength > 0) {
		/*Some lines in this if section are commented as per Yang Lu's suggestion */
		//if (roadSegment->getLaneZeroLength() > 10*vehicle_length) {
			density = numMovingInSegment(true)/(movingLength/100.0);
		//}
		//else {
		//	density = queueCount/(movingLength/100.0);
		//}
	}
	else {
		density = 1/(PASSENGER_CAR_UNIT/100.0);
	}
	return density;
}

unsigned int SegmentStats::numQueuingInSegment(bool hasVehicle) const {
	unsigned int queuingCounts = 0;
	const std::vector<sim_mob::Lane*>& segLanes = roadSegment->getLanes();
	std::vector<sim_mob::Lane*>::const_iterator lane = segLanes.begin();
	while (lane != segLanes.end()) {
		if ((hasVehicle && !(*lane)->is_pedestrian_lane())
				|| (!hasVehicle && (*lane)->is_pedestrian_lane())) {
			LaneStatsMap::const_iterator laneStatsIt = laneStatsMap.find(*lane);
			if (laneStatsIt != laneStatsMap.end()) {
				queuingCounts = queuingCounts + laneStatsIt->second->getQueuingAgentsCount();
			} else {
				throw std::runtime_error("SegmentStats::numQueueingInSegment was called with invalid laneStats!");
			}
		}
		lane++;
	}
	return queuingCounts;
}

sim_mob::Person* SegmentStats::personClosestToSegmentEnd() {
	sim_mob::Person* person = nullptr;
	const sim_mob::Lane* personLane = nullptr;
	double minDistance = std::numeric_limits<double>::max();

	std::map<const sim_mob::Lane*, sim_mob::Person*>::iterator i =
			frontalAgents.begin();
	while (i != frontalAgents.end()) {
		if (i->second) {
			if (minDistance == i->second->distanceToEndOfSegment) {
				// If current person and (*i) are at equal distance to the stop line, we 'toss a coin' and choose one of them
				bool coinTossResult = ((rand() / (double) RAND_MAX) < 0.5);
				if (coinTossResult) {
					personLane = i->first;
					person = i->second;
				}
			} else if (minDistance > i->second->distanceToEndOfSegment) {
				minDistance = i->second->distanceToEndOfSegment;
				personLane = i->first;
				person = i->second;
			}
		}
		i++;
	}

	if (person) { // frontalAgents could possibly be all nullptrs
		frontalAgents.erase(personLane);
		frontalAgents.insert(std::make_pair(personLane, laneStatsMap.at(personLane)->next()));
	}
	return person;
}

void SegmentStats::resetFrontalAgents() {
	frontalAgents.clear();
	for (LaneStatsMap::iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		i->second->resetIterator();
		Person* person = i->second->next();
		frontalAgents.insert(std::make_pair(i->first, person));
	}
}

void SegmentStats::addBusStop(const sim_mob::BusStop* stop) {
	if(stop) {
		busStops.push_back(stop);
	}
	else {
		throw std::runtime_error("addBusStop(): stop to be added is NULL");
	}
}

sim_mob::Person* LaneStats::next() {
	sim_mob::Person* person = nullptr;
	if (laneAgentsIt != laneAgentsCopy.end()) {
		person = *laneAgentsIt;
		laneAgentsIt++;
	}
	return person;
}

unsigned int sim_mob::LaneStats::getQueuingAgentsCount() const{
	return queueCount;
}

unsigned int sim_mob::LaneStats::getMovingAgentsCount() const {
	if (numPersons < queueCount) {
		printAgents();
		std::stringstream debugMsgs;
		debugMsgs
				<< "number of lane agents cannot be less than the number of queuing agents."
				<< "\nlane" << getLane()->getLaneID()
				<< "|queueCount: " << queueCount
				<< "|laneAgents count: " << numPersons
				<< std::endl;
		throw std::runtime_error(debugMsgs.str());
	}
	return (numPersons - queueCount);
}

void sim_mob::LaneStats::addPerson(sim_mob::Person* p) {
	if(laneInfinity) {
		laneAgents.push_back(p);
	}
	else {
		if(laneAgents.size() > 0) {
			std::deque<Person*>::iterator i=laneAgents.end()-1; // last person's iterator
			while(i != laneAgents.begin() && (*i)->distanceToEndOfSegment > p->distanceToEndOfSegment) {
				i--;
			}
			if(i == laneAgents.begin() && (*i)->distanceToEndOfSegment > p->distanceToEndOfSegment) {
				laneAgents.push_front(p);
			}
			else {
				laneAgents.insert(i+1,p); //deque is optimized for insertions and removals.
			}
		}
		else {
			laneAgents.push_back(p);
		}
		if (p->isQueuing) {
			queueCount++;
		}
	}
	numPersons++; // record addition
}

void sim_mob::LaneStats::updateQueueStatus(sim_mob::Person* p) {
	if (!laneInfinity) {
		if (p->isQueuing) {
			queueCount++;
		}
		else {
			if (queueCount > 0) {
				queueCount--;
			}
			else {
				std::stringstream debugMsgs;
				debugMsgs
						<< "Error in updateQueueStatus(): queueCount cannot be lesser than 0 in lane."
						<< "\nlane:" << lane->getLaneID() << "|Segment: "
						<< lane->getRoadSegment()->getStartEnd() << "|Person: "
						<< p->getId() << "\nQueuing: " << queueCount
						<< "|Total: " << numPersons << std::endl;
				Print() << debugMsgs.str();
				throw std::runtime_error(debugMsgs.str());

			}
		}
	}
}

void sim_mob::LaneStats::removePerson(sim_mob::Person* p, bool wasQueuing) {
	PersonList::iterator pIt = std::find(laneAgents.begin(),laneAgents.end(), p);
	if (pIt != laneAgents.end()) {
		laneAgents.erase(pIt);
		numPersons--; //record removal
	}
	if (wasQueuing && !laneInfinity) {
		if (queueCount > 0) {
			queueCount--;
		}
		else {
			std::stringstream debugMsgs;
			debugMsgs
					<< "Error in removePerson(): queueCount cannot be lesser than 0 in lane."
					<< "\nlane:" << lane->getLaneID() << "|Segment: "
					<< lane->getRoadSegment()->getStartEnd() << "|Person: "
					<< p->getId() << "\nQueuing: " << queueCount << "|Total: "
					<< laneAgents.size() << std::endl;
			Print() << debugMsgs.str();
			throw std::runtime_error(debugMsgs.str());
		}
	}
}

void LaneStats::resetIterator() {
	laneAgentsCopy = laneAgents;
	laneAgentsIt = laneAgentsCopy.begin();
}

void sim_mob::LaneStats::initLaneParams(double vehSpeed, const double capacity) {
	size_t numLanes = lane->getRoadSegment()->getLanes().size();
	if (numLanes > 0) {
		double orig = capacity/numLanes;
		laneParams->setOrigOutputFlowRate(orig);
	}
	laneParams->outputFlowRate = laneParams->origOutputFlowRate;

	// As per Yang Lu's suggestion for short segment correction
	if(length < SHORT_SEGMENT_LENGTH_LIMIT){
		laneParams->outputFlowRate = LARGE_OUTPUT_FLOW_RATE; //some large number
	}

	updateOutputCounter();
	updateAcceptRate(vehSpeed);
}

void sim_mob::LaneStats::updateOutputFlowRate(double newFlowRate) {
	laneParams->outputFlowRate = newFlowRate;
}

void sim_mob::LaneStats::updateOutputCounter() {
	double tick_size = ConfigManager::GetInstance().FullConfig().baseGranSecond();
	int tmp = int(laneParams->outputFlowRate * tick_size);
	laneParams->fraction += laneParams->outputFlowRate * tick_size - float(tmp);
	if (laneParams->fraction >= 1.0) {
		laneParams->fraction -= 1.0;
		laneParams->outputCounter = float(tmp) + 1.0;
	} else {
		laneParams->outputCounter = float(tmp);
	}
}

void sim_mob::LaneStats::updateAcceptRate(double upSpeed) {
	const double omega = 0.01;
	double tick_size = ConfigManager::GetInstance().FullConfig().baseGranSecond();
	double capacity = laneParams->outputFlowRate * tick_size;
	double acceptRateA = (capacity > 0) ? tick_size / capacity : 0;
	double acceptRateB = (omega * PASSENGER_CAR_UNIT) / upSpeed;
	laneParams->acceptRate = std::max(acceptRateA, acceptRateB);
}

sim_mob::LaneParams* sim_mob::SegmentStats::getLaneParams(const Lane* lane) const {
	return laneStatsMap.find(lane)->second->laneParams;
}

double sim_mob::SegmentStats::speedDensityFunction(const double segDensity) const {
	//maxSpeed according to AIMSUN
	const double freeFlowSpeed = supplyParams.getFreeFlowSpeed();
	const double minSpeed = supplyParams.getMinSpeed();
	const double jamDensity = supplyParams.getJamDensity();
	const double alpha = supplyParams.getAlpha();
	const double beta = supplyParams.getBeta();
	const double minDensity = supplyParams.getMinDensity();

	double speed = 0.0;
	//Speed-Density function same as in DynaMIT
	if (segDensity >= jamDensity) {
		speed = minSpeed;
	} else if (segDensity >= minDensity) {
		speed = freeFlowSpeed * pow((1 - pow((segDensity - minDensity) / jamDensity, beta)), alpha);
	} else {
		speed = freeFlowSpeed;
	}
	speed = std::max(speed, minSpeed);

	// As per Yang Lu's suggestion for short segment correction
	if (length < SHORT_SEGMENT_LENGTH_LIMIT){
		speed = freeFlowSpeed;
	}
	return speed;
}

void sim_mob::SegmentStats::restoreLaneParams(const Lane* lane) {
	LaneStats* laneStats = laneStatsMap.find(lane)->second;
	laneStats->updateOutputFlowRate(getLaneParams(lane)->origOutputFlowRate);
	laneStats->updateOutputCounter();
	segDensity = getDensity(true);
	double upSpeed = speedDensityFunction(segDensity);
	laneStats->updateAcceptRate(upSpeed);
}

void sim_mob::SegmentStats::updateLaneParams(const Lane* lane, double newOutputFlowRate) {
	LaneStats* laneStats = laneStatsMap.find(lane)->second;
	laneStats->updateOutputFlowRate(newOutputFlowRate);
	laneStats->updateOutputCounter();
	segDensity = getDensity(true);
	double upSpeed = speedDensityFunction(segDensity);
	laneStats->updateAcceptRate(upSpeed);
}

void sim_mob::SegmentStats::updateLaneParams(timeslice frameNumber) {
	segDensity = getDensity(true);
	segVehicleSpeed = speedDensityFunction(segDensity);
	//need to update segPedSpeed in future
	LaneStatsMap::iterator it = laneStatsMap.begin();
	for (; it != laneStatsMap.end(); ++it) {
		//filtering out the pedestrian lanes for now
		if (!(it->first)->is_pedestrian_lane()) {
			(it->second)->updateOutputCounter();
			(it->second)->updateAcceptRate(segVehicleSpeed);
			(it->second)->setInitialQueueCount(it->second->getQueuingAgentsCount());
		}
	}
}

std::string sim_mob::SegmentStats::reportSegmentStats(timeslice frameNumber){
	if (ConfigManager::GetInstance().CMakeConfig().OutputEnabled()) {
		std::stringstream msg;
		double density = segDensity
							* 1000.0 /* Density is converted to veh/km/lane for the output */
							* numVehicleLanes; /* Multiplied with number of lanes to get the density in veh/km/segment*/

#define SHOW_NUMBER_VEHICLE_ON_SEGMENT
#ifdef SHOW_NUMBER_VEHICLE_ON_SEGMENT

		msg <<"(\"segmentState\""
			<<","<<frameNumber.frame()
			<<","<<roadSegment
			<<",{"
			<<"\"speed\":\""<< segVehicleSpeed
			<<"\",\"flow\":\""<< segFlow
			<<"\",\"density\":\""<< density
			<<"\",\"total_vehicles\":\""<< numPersons
			<<"\",\"moving_vehicles\":\""<< numMovingInSegment(true)
			<<"\",\"queue_vehicles\":\""<< numQueuingInSegment(true)
			<<"\",\"numVehicleLanes\":\""<< numVehicleLanes
			<<"\",\"segment_length\":\""<< length
			<<"\"})"<<std::endl;
#else

		msg <<"(\"segmentState\""
			<<","<<frameNumber.frame()
			<<","<<roadSegment
			<<",{"
			<<"\"speed\":\""<< segVehicleSpeed
			<<"\",\"flow\":\""<< segFlow
			<<"\",\"density\":\""<< density
			<<"\"})"<<std::endl;
#endif
		return msg.str();
	}
	return "";
}

double sim_mob::SegmentStats::getSegSpeed(bool hasVehicle) const {
	if (hasVehicle) {
		return segVehicleSpeed;
	}
	return segPedSpeed;
}

bool SegmentStats::hasAgents() const {
	return (numPersons > 0);
}

double SegmentStats::getPositionOfLastUpdatedAgentInLane(const Lane* lane) const{
	return laneStatsMap.find(lane)->second->getPositionOfLastUpdatedAgent();
}

void SegmentStats::setPositionOfLastUpdatedAgentInLane(double positionOfLastUpdatedAgentInLane, const Lane* lane) {
		laneStatsMap.find(lane)->second->setPositionOfLastUpdatedAgent(positionOfLastUpdatedAgentInLane);
}

unsigned int sim_mob::SegmentStats::getInitialQueueCount(const Lane* lane) const {
	return laneStatsMap.find(lane)->second->getInitialQueueCount();
}

void SegmentStats::resetPositionOfLastUpdatedAgentOnLanes() {
	for (LaneStatsMap::iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		i->second->setPositionOfLastUpdatedAgent(-1.0);
	}
}

unsigned int SegmentStats::getSegFlow() {
	return segFlow;
}

void SegmentStats::incrementSegFlow() {
	segFlow++;
}

void SegmentStats::resetSegFlow() {
	segFlow = 0;
}

unsigned int SegmentStats::computeExpectedOutputPerTick() {
	float count = 0;
	for (LaneStatsMap::iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		count += i->second->laneParams->getOutputFlowRate()
				* ConfigManager::GetInstance().FullConfig().baseGranSecond();
	}
	return std::floor(count);
}

void SegmentStats::updateLinkDrivingTimes(double drivingTimeToEndOfLink) {
	double speed = getSegSpeed(true);
	//If speed is 0, treat it as a very small value
	if(speed < INFINITESIMAL_DOUBLE) {
		speed = INFINITESIMAL_DOUBLE;
	}

	for(std::vector<sim_mob::Lane*>::const_iterator lnIt=roadSegment->getLanes().begin();
			lnIt!=roadSegment->getLanes().end(); lnIt++) {
		PersonList& lnAgents = laneStatsMap.find(*lnIt)->second->laneAgents;
		for(PersonList::const_iterator pIt=lnAgents.begin();
				pIt!=lnAgents.end(); pIt++) {
			Person* person = (*pIt);
			person->drivingTimeToEndOfLink =
					(person->distanceToEndOfSegment/speed) + drivingTimeToEndOfLink;
		}
	}
	PersonList& lnAgents = laneStatsMap.find(laneInfinity)->second->laneAgents;
	for(PersonList::const_iterator pIt=lnAgents.begin();
					pIt!=lnAgents.end(); pIt++) {
		Person* person = (*pIt);
		person->drivingTimeToEndOfLink =
				(person->distanceToEndOfSegment/speed) + drivingTimeToEndOfLink;
	}
}

void SegmentStats::printAgents() {
	Print() << "\nSegment: " << roadSegment->getStartEnd() << "|length "
			<< roadSegment->getLaneZeroLength() << std::endl;
	for (LaneStatsMap::const_iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		(*i).second->printAgents();
	}
	for (LaneStatsMap::const_iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		(*i).second->printAgents(true);
	}
}

void LaneStats::printAgents(bool copy) const {
	std::stringstream debugMsgs;
	if (!copy) {
		debugMsgs << "Segment:" << lane->getRoadSegment()->getStartEnd() << "|Lane: " << lane->getLaneID();
		for (PersonList::const_iterator i = laneAgents.begin(); i != laneAgents.end(); i++) {
			debugMsgs << "|" << (*i)->getId();
		}
	} else {
		debugMsgs << "Segment:" << lane->getRoadSegment()->getStartEnd() << "|LaneCopy: " << lane->getLaneID();
		for (PersonList::const_iterator i = laneAgentsCopy.begin(); i != laneAgentsCopy.end(); i++) {
			debugMsgs << "|" << (*i)->getId();
		}
	}
	debugMsgs << std::endl;
	Print() << debugMsgs.str();
}

void LaneStats::verifyOrdering() {
	double distance = -1.0;
	for (PersonList::const_iterator i = laneAgents.begin(); i != laneAgents.end(); i++) {
		if (distance >= (*i)->distanceToEndOfSegment) {
			std::stringstream debugMsgs;
			debugMsgs
					<< "Invariant violated: Ordering of laneAgents does not reflect ordering w.r.t. distance to end of segment."
					<< "\nSegment: "
					<< lane->getRoadSegment()->getStartEnd()
					<< " length = "
					<< lane->getRoadSegment()->getLaneZeroLength()
					<< "\nLane: " << lane->getLaneID() << "\nCulprit Person: "
					<< (*i)->getId();
			debugMsgs << "\nAgents ";
			for (PersonList::const_iterator j = laneAgents.begin(); j != laneAgents.end(); j++) {
				debugMsgs << "|" << (*j)->getId() << "--" << (*j)->distanceToEndOfSegment;
			}
			throw std::runtime_error(debugMsgs.str());
		}
		else {
			distance = (*i)->distanceToEndOfSegment;
		}
	}
}

sim_mob::Person* SegmentStats::dequeue(const sim_mob::Person* person, const sim_mob::Lane* lane, bool isQueuingBfrUpdate) {
	sim_mob::Person* dequeuedPerson =  laneStatsMap.find(lane)->second->dequeue(person, isQueuingBfrUpdate);
	if(dequeuedPerson) { numPersons--; } // record removal from segment
	return dequeuedPerson;
}

sim_mob::Person* sim_mob::LaneStats::dequeue(const sim_mob::Person* person, bool isQueuingBfrUpdate) {
	if (laneAgents.size() == 0) {
		std::stringstream debugMsgs;
		debugMsgs << "Trying to dequeue Person " << person->getId() << " from empty lane." << std::endl;
		throw std::runtime_error(debugMsgs.str());
	}
	sim_mob::Person* p = nullptr;
	if(person == laneAgents.front()){
		p = laneAgents.front();
		laneAgents.pop_front();
		numPersons--; // record removal
	}
	else if (laneInfinity) {
		PersonList::iterator it;
		for (it = laneAgents.begin(); it != laneAgents.end() ; it++) {
			if ((*it) == person){
				p = (*it);
				it = laneAgents.erase(it); // erase returns the next iterator
				numPersons--; //record removal
				break; //exit loop
			}
		}
	}
	if (isQueuingBfrUpdate) {
		if (queueCount > 0) {
			// we have removed a queuing agent
			queueCount--;
		}
		else {
			std::stringstream debugMsgs;
			debugMsgs
					<< "Error in dequeue(): queueCount cannot be lesser than 0 in lane."
					<< "\nlane:" << lane->getLaneID() << "|Segment: "
					<< lane->getRoadSegment()->getStartEnd() << "|Person: "
					<< p->getId() << "\nQueuing: " << queueCount
					<< "|Total: " << laneAgents.size() << std::endl;
			Print() << debugMsgs.str();
			throw std::runtime_error(debugMsgs.str());
		}
	}
	return p;
}

} // end of namespace sim_mob
