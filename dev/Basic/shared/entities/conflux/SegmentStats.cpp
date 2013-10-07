//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "SegmentStats.hpp"

#include <cmath>
#include <algorithm>

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "logging/Log.hpp"

using std::string;

namespace sim_mob {

SegmentStats::SegmentStats(const sim_mob::RoadSegment* rdSeg, bool isDownstream) :
			roadSegment(rdSeg), segDensity(0.0), segPedSpeed(0.0), segFlow(0.0), lastAcceptTime(0.0), debugMsgs(std::stringstream::out) {
	segVehicleSpeed = getRoadSegment()->maxSpeed / 3.6 * 100; //converting from kmph to m/s
	numVehicleLanes = 0;

	// initialize LaneAgents in the map
	std::vector<sim_mob::Lane*>::const_iterator lane =
			rdSeg->getLanes().begin();
	while (lane != rdSeg->getLanes().end()) {
		laneStatsMap.insert(std::make_pair(*lane, new sim_mob::LaneStats(*lane)));
		laneStatsMap[*lane]->initLaneParams(*lane, segVehicleSpeed, segPedSpeed);
		prevTickLaneCountsFromOriginal.insert(std::make_pair(*lane, std::make_pair(0, 0))); // initialized to zero (irrespective of whether this is downstreamCopy)
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
	laneStatsMap.insert(std::make_pair(laneInfinity, new sim_mob::LaneStats(laneInfinity, true)));
	prevTickLaneCountsFromOriginal.insert(std::make_pair(laneInfinity, std::make_pair(0, 0)));
	downstreamCopy = isDownstream;
}

void SegmentStats::addAgent(const sim_mob::Lane* lane, sim_mob::Person* p) {
	laneStatsMap.find(lane)->second->addPerson(p);
}

void SegmentStats::removeAgent(const sim_mob::Lane* lane, sim_mob::Person* p, bool wasQueuing) {
	laneStatsMap.find(lane)->second->removePerson(p, wasQueuing);
}

void SegmentStats::updateQueueStatus(const sim_mob::Lane* lane, sim_mob::Person* p) {
	laneStatsMap.find(lane)->second->updateQueueStatus(p);
}

sim_mob::Person* SegmentStats::dequeue(const sim_mob::Lane* lane, bool isQueuingBfrUpdate) {
	return laneStatsMap.find(lane)->second->dequeue(isQueuingBfrUpdate);
}

std::deque<sim_mob::Person*> SegmentStats::getAgents(const sim_mob::Lane* lane) {
	return laneStatsMap.find(lane)->second->laneAgents;
}

std::deque<sim_mob::Person*> SegmentStats::getAgents() {
	std::deque<sim_mob::Person*> segAgents, lnAgents;
	for(std::vector<sim_mob::Lane*>::const_iterator lnIt=roadSegment->getLanes().begin(); lnIt!=roadSegment->getLanes().end(); lnIt++) {
		lnAgents = laneStatsMap.find(*lnIt)->second->laneAgents;
		segAgents.insert(segAgents.end(), lnAgents.begin(), lnAgents.end());
	}
	lnAgents = laneStatsMap.find(laneInfinity)->second->laneAgents;
	segAgents.insert(segAgents.end(), lnAgents.begin(), lnAgents.end());

	return segAgents;
}

std::pair<unsigned int, unsigned int> SegmentStats::getLaneAgentCounts(const sim_mob::Lane* lane) {
	return std::make_pair(laneStatsMap.at(lane)->getQueuingAgentsCount(), laneStatsMap.at(lane)->getMovingAgentsCount());
}

std::map<const sim_mob::Lane*, std::pair<unsigned int, unsigned int> > SegmentStats::getAgentCountsOnLanes() {
	std::map<const sim_mob::Lane*, std::pair<unsigned int, unsigned int> > agentCountsOnLanes;
	std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::iterator laneStatsIt =
			laneStatsMap.begin();
	while (laneStatsIt != laneStatsMap.end()) {
		agentCountsOnLanes.insert(std::make_pair(laneStatsIt->first, getLaneAgentCounts(laneStatsIt->first)));
		laneStatsIt++;
	}
	return agentCountsOnLanes;
}

const sim_mob::RoadSegment* sim_mob::SegmentStats::getRoadSegment() const {
	return roadSegment;
}

bool SegmentStats::isFront(const sim_mob::Lane* lane, sim_mob::Person* person) {
	return (person == laneStatsMap.find(lane)->second->laneAgents.front());
}

unsigned int SegmentStats::numAgentsInLane(const sim_mob::Lane* lane) {
	return (laneStatsMap.at(lane)->getMovingAgentsCount()
			+ laneStatsMap.at(lane)->getQueuingAgentsCount());
}

unsigned int SegmentStats::numMovingInSegment(bool hasVehicle) {
	unsigned int movingCounts = 0;
	std::vector<sim_mob::Lane*>::const_iterator lane = roadSegment->getLanes().begin();
	while (lane != roadSegment->getLanes().end()) {
		if ((hasVehicle && !(*lane)->is_pedestrian_lane()) || (!hasVehicle && (*lane)->is_pedestrian_lane())) {
			std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::iterator laneStatsIt = laneStatsMap.find(*lane);
			if (laneStatsIt != laneStatsMap.end()) {
				movingCounts = movingCounts + laneStatsIt->second->getMovingAgentsCount();
			} else {
				throw std::runtime_error("SegmentStats::numMovingInSegment called with invalid laneStats.");
			}
		}
		lane++;
	}
	return movingCounts;
}

double SegmentStats::getDensity(bool hasVehicle) {
	int vehLaneCount = 0;
	double movingLength = 0.0;
	const int vehicle_length = 400;
	double density = 0.0;

	std::vector<sim_mob::Lane*>::const_iterator laneIt = roadSegment->getLanes().begin();
	while (laneIt != roadSegment->getLanes().end()) {
		if ((hasVehicle && !(*laneIt)->is_pedestrian_lane()) || (!hasVehicle && (*laneIt)->is_pedestrian_lane())) {
			vehLaneCount += 1;
		}
		laneIt++;
	}

	unsigned int queueCount = numQueueingInSegment(true);
	movingLength = roadSegment->getLaneZeroLength()*vehLaneCount - queueCount*vehicle_length;
	if(movingLength > 0) {
		if (roadSegment->getLaneZeroLength() > 10*vehicle_length) {
			density = numMovingInSegment(true)/(movingLength/100.0);
		}
		else {
			density = queueCount/(movingLength/100.0);
		}
	}
	else {
		density = 1/(vehicle_length/100.0);
	}
	return density;
}

unsigned int SegmentStats::numQueueingInSegment(bool hasVehicle) {
	unsigned int queuingCounts = 0;

	std::vector<sim_mob::Lane*>::const_iterator lane =
			roadSegment->getLanes().begin();
	while (lane != roadSegment->getLanes().end()) {
		if ((hasVehicle && !(*lane)->is_pedestrian_lane()) || (!hasVehicle && (*lane)->is_pedestrian_lane())) {
			std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::iterator laneStatsIt = laneStatsMap.find(*lane);
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

/* unused version based on remaining time at intersection
 sim_mob::Person* SegmentStats::agentClosestToStopLineFromFrontalAgents() {
 sim_mob::Person* person = nullptr;
 const sim_mob::Lane* personLane = nullptr;
 double maxRemainingTimeAtIntersection = std::numeric_limits<double>::min();
 double remainingTimeAtIntersection = 0.0;
 double timeToReachIntersection = 0.0;
 double remainingTimeThisTick = 0.0;
 double tickSize = ConfigParams::GetInstance().baseGranMS / 1000.0;

 std::map<const sim_mob::Lane*, sim_mob::Person* >::iterator i = frontalAgents.begin();
 while(i!=frontalAgents.end()) {
 if(i->second) {
 if (i->second->lastUpdatedFrame < roadSegment->getParentConflux()->currFrameNumber.frame()) {
 //if the person is moved for the first time in this tick
 remainingTimeThisTick = ConfigParams::GetInstance().baseGranMS / 1000.0;
 }
 else {
 remainingTimeThisTick = i->second->getRemainingTimeThisTick();
 }

 timeToReachIntersection = roadSegment->getParentConflux()->computeTimeToReachEndOfLink(roadSegment, i->second->distanceToEndOfSegment);
 remainingTimeAtIntersection =  - fmod(timeToReachIntersection - remainingTimeThisTick, tickSize); //Requires modulo computation because the person may take multiple ticks to reach the intersection.
 if(remainingTimeAtIntersection < 0) remainingTimeAtIntersection = remainingTimeAtIntersection + tickSize;
 if(maxRemainingTimeAtIntersection == remainingTimeAtIntersection) {
 // If current person and (*i) are at equal distance to the stop line, we 'toss a coin' and choose one of them
 bool coinTossResult = ((rand() / (double)RAND_MAX) < 0.5);
 if(coinTossResult) {
 personLane = i->first;
 person = i->second;
 }
 }
 else if (maxRemainingTimeAtIntersection < remainingTimeAtIntersection) {
 maxRemainingTimeAtIntersection = remainingTimeAtIntersection;
 personLane = i->first;
 person = i->second;
 person->remainingTimeAtIntersection = remainingTimeAtIntersection;
 }
 }
 i++;
 }

 if(person) { // frontalAgents could possibly be all nullptrs
 frontalAgents.erase(personLane);
 frontalAgents.insert(std::make_pair(personLane,laneStatsMap.at(personLane)->next()));
 }
 return person;
 }*/

sim_mob::Person* SegmentStats::agentClosestToStopLineFromFrontalAgents() {
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
	for (std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		i->second->resetIterator();
		Person* person = i->second->next();
		frontalAgents.insert(std::make_pair(i->first, person));
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

unsigned int sim_mob::LaneStats::getQueuingAgentsCount() {
	return queueCount;
}

unsigned int sim_mob::LaneStats::getMovingAgentsCount() {
	if (laneAgents.size() < queueCount) {
		printAgents();
		debugMsgs
				<< "number of lane agents cannot be less than the number of queuing agents."
				<< "\nlane" << getLane()->getLaneID() << "|queueCount: "
				<< queueCount << "|laneAgents count: " << laneAgents.size()
				<< "|moving: " << laneAgents.size() - queueCount << std::endl;
		throw std::runtime_error(debugMsgs.str());
	}
	return (laneAgents.size() - queueCount);
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
				debugMsgs
						<< "Error in updateQueueStatus(): queueCount cannot be lesser than 0 in lane."
						<< "\nlane:" << lane->getLaneID() << "|Segment: "
						<< lane->getRoadSegment()->getStartEnd() << "|Person: "
						<< p->getId() << "\nQueuing: " << queueCount
						<< "|Total: " << laneAgents.size() << std::endl;
				Print() << debugMsgs.str();
				throw std::runtime_error(debugMsgs.str());

			}
		}
	}
}

void sim_mob::LaneStats::removePerson(sim_mob::Person* p, bool wasQueuing) {
	std::deque<sim_mob::Person*>::iterator pIt = std::find(laneAgents.begin(),laneAgents.end(), p);
	if (pIt != laneAgents.end()) {
		laneAgents.erase(pIt);
	}
	if (wasQueuing && !laneInfinity) {
		if (queueCount > 0) {
			queueCount--;
		}
		else {
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

sim_mob::Person* sim_mob::LaneStats::dequeue(bool isQueuingBfrUpdate) {
	if (laneAgents.size() == 0) {
		throw std::runtime_error("Trying to dequeue from empty lane.");
	}
	sim_mob::Person* p = laneAgents.front();
	laneAgents.pop_front();
	if (!laneInfinity) {
		if (isQueuingBfrUpdate) {
			if (queueCount > 0) {
				// we have removed a queuing agent
				queueCount--;
			}
			else {
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
	}
	return p;
}

void LaneStats::resetIterator() {
	laneAgentsCopy = laneAgents;
	laneAgentsIt = laneAgentsCopy.begin();
}

void sim_mob::LaneStats::initLaneParams(const Lane* lane, double vehSpeed,
		double pedSpeed) {
	//laneParams = sim_mob::LaneParams();
	int numLanes = lane->getRoadSegment()->getLanes().size();
	if (numLanes > 0) {
		double orig = (lane->getRoadSegment()->capacity)*50
				/ (numLanes*3600.0);
		laneParams->setOrigOutputFlowRate(orig);
	}
	laneParams->outputFlowRate = laneParams->origOutputFlowRate;

	updateOutputCounter(lane);
	updateAcceptRate(lane, vehSpeed);
}

void sim_mob::LaneStats::updateOutputFlowRate(const Lane* lane,
		double newFlowRate) {
	laneParams->outputFlowRate = newFlowRate;
}

void sim_mob::LaneStats::updateOutputCounter(const Lane* lane) {
	double tick_size = ConfigManager::GetInstance().FullConfig().baseGranMS() / 1000.0;
	int tmp = int(laneParams->outputFlowRate * tick_size);
	laneParams->fraction += laneParams->outputFlowRate * tick_size - float(tmp);
	if (laneParams->fraction >= 1.0) {
		laneParams->fraction -= 1.0;
		laneParams->outputCounter = float(tmp) + 1.0;
	} else {
		laneParams->outputCounter = float(tmp);
	}
}

void sim_mob::LaneStats::updateAcceptRate(const Lane* lane, double upSpeed) {
	const double omega = 0.01;
	const double vehicle_length = 400;
	double tick_size = ConfigManager::GetInstance().FullConfig().baseGranMS() / 1000.0;
	double capacity = laneParams->outputFlowRate * tick_size;
	double acceptRateA = (capacity > 0) ? tick_size / capacity : 0;
	double acceptRateB = (omega * vehicle_length) / upSpeed;
	laneParams->acceptRate = std::max(acceptRateA, acceptRateB);
}

void LaneStats::clear() {
	laneAgents.clear();
}

sim_mob::LaneParams* sim_mob::SegmentStats::getLaneParams(const Lane* lane) {
	return laneStatsMap.find(lane)->second->laneParams;
}

double sim_mob::SegmentStats::speed_density_function(bool hasVehicle,
		double segDensity) {
	/**
	 * TODO: The parameters - min density, jam density, alpha and beta - for each road segment
	 * must be obtained from an external source (XML/Database)
	 * Since we don't have this data, we have taken the average values from supply parameters of Singapore expressways.
	 * This must be changed after we have this data for each road segment in the network.
	 *
	 * TODO: A params struct for these parameters is already defined in the RoadSegment class.
	 * This struct is to be used when we have actual values for the parameters.
	 */

	//double density = numVehicles / (getRoadSegment()->computeLaneZeroLength() / 100.0);
	double freeFlowSpeed = getRoadSegment()->maxSpeed / 3.6 * 100; // Converting from Kmph to cm/s
	double minSpeed = 1000.0;
	double jamDensity = 0.25; //density during traffic jam
	double alpha = 1.15; //Model parameter of speed density function
	double beta = 1.5; //Model parameter of speed density function
	double minDensity = 0.0048; // minimum traffic density
	double speed = 0.0;

	//Speed-Density function
	if (segDensity >= jamDensity) {
		speed = minSpeed;
	} else if (segDensity >= minDensity) {
		speed = freeFlowSpeed * pow((1 - pow((segDensity - minDensity) / jamDensity, beta)), alpha);
	} else {
		speed = freeFlowSpeed;
	}
	speed = std::max(speed, minSpeed);
	return speed;
}

void sim_mob::SegmentStats::restoreLaneParams(const Lane* lane) {
	LaneStats* laneStats = laneStatsMap.find(lane)->second;
	laneStats->updateOutputFlowRate(lane, getLaneParams(lane)->origOutputFlowRate);
	laneStats->updateOutputCounter(lane);
	segDensity = getDensity(true);
	double upSpeed = speed_density_function(true, segDensity);
	laneStats->updateAcceptRate(lane, upSpeed);
}

void sim_mob::SegmentStats::updateLaneParams(const Lane* lane, double newOutputFlowRate) {
	LaneStats* laneStats = laneStatsMap.find(lane)->second;
	laneStats->updateOutputFlowRate(lane, newOutputFlowRate);
	laneStats->updateOutputCounter(lane);
	segDensity = getDensity(true);
	double upSpeed = speed_density_function(true, segDensity);
	laneStats->updateAcceptRate(lane, upSpeed);
}

void sim_mob::SegmentStats::updateLaneParams(timeslice frameNumber) {
	segDensity = getDensity(true);
	segVehicleSpeed = speed_density_function(true, segDensity);
	//need to update segPedSpeed in future
	std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::iterator it = laneStatsMap.begin();
	for (; it != laneStatsMap.end(); ++it) {
		//filtering out the pedestrian lanes for now
		if (!(it->first)->is_pedestrian_lane()) {
			(it->second)->updateOutputCounter(it->first);
			(it->second)->updateAcceptRate(it->first, segVehicleSpeed);
			(it->second)->setInitialQueueCount(it->second->getQueuingAgentsCount());
		}
	}
}

std::string sim_mob::SegmentStats::reportSegmentStats(timeslice frameNumber){
	if (ConfigManager::GetInstance().CMakeConfig().OutputEnabled()) {
		std::stringstream msg;
		msg <<"(\"segmentState\""
			<<","<<frameNumber.frame()
			<<","<<roadSegment
			<<",{"
			<<"\"speed\":\""<<segVehicleSpeed
			<<"\",\"flow\":\""<<segFlow
			<<"\",\"density\":\""<<segDensity
			<<"\"})"<<std::endl;
		return msg.str();
	}
	return "";
}

double sim_mob::SegmentStats::getSegSpeed(bool hasVehicle) {
	if (hasVehicle) {
		return segVehicleSpeed;
	}
	return segPedSpeed;
}

bool SegmentStats::hasAgents() {
	std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::iterator laneIt = laneStatsMap.begin();
	while (laneIt != laneStatsMap.end()) {
		if (laneIt->second->laneAgents.size() > 0) {
			return true;
		}
		laneIt++;
	}
	return false;
}

double SegmentStats::getPositionOfLastUpdatedAgentInLane(const Lane* lane) {
	return laneStatsMap.find(lane)->second->getPositionOfLastUpdatedAgent();
}

void SegmentStats::setPositionOfLastUpdatedAgentInLane(double positionOfLastUpdatedAgentInLane, const Lane* lane) {
		laneStatsMap.find(lane)->second->setPositionOfLastUpdatedAgent(positionOfLastUpdatedAgentInLane);
}

unsigned int sim_mob::SegmentStats::getInitialQueueCount(const Lane* lane) {
	return laneStatsMap.find(lane)->second->getInitialQueueCount();
}

void SegmentStats::resetPositionOfLastUpdatedAgentOnLanes() {
	for (std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		i->second->setPositionOfLastUpdatedAgent(-1.0);
	}
}

double SegmentStats::getSegFlow() {
	return segFlow;
}

void SegmentStats::incrementSegFlow() {
	segFlow++;
}

void SegmentStats::resetSegFlow() {
	segFlow = 0.0;
}

unsigned int SegmentStats::computeExpectedOutputPerTick() {
	float count = 0;
	for (std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		count += (*i).second->laneParams->getOutputFlowRate() * ConfigManager::GetInstance().FullConfig().baseGranMS() / 1000.0;
	}
	return std::floor(count);
}

void SegmentStats::printAgents() {
	Print() << "\nSegment: " << roadSegment->getStartEnd() << "|length "
			<< roadSegment->getLaneZeroLength() << std::endl;
	for (std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::const_iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		(*i).second->printAgents();
	}
	for (std::map<const sim_mob::Lane*, sim_mob::LaneStats*>::const_iterator i = laneStatsMap.begin(); i != laneStatsMap.end(); i++) {
		(*i).second->printAgents(true);
	}
}

bool SegmentStats::canAccommodate(VehicleType type) {
	if (type == SegmentStats::car) {
		int lengthOccupied = (numMovingInSegment(true) + numQueueingInSegment(true)) * 400; // This must change to count cars and buses separately.
		int segLength = roadSegment->getLaneZeroLength();
		return (lengthOccupied <= segLength * numVehicleLanes);
	}
	else {
		throw std::runtime_error("Buses and Pedestrians are not implemented in medium term yet");
	}
}

void LaneStats::printAgents(bool copy) {
	if (!copy) {
		debugMsgs << "Segment:" << lane->getRoadSegment()->getStartEnd() << "|Lane: " << lane->getLaneID();
		for (std::deque<sim_mob::Person*>::const_iterator i = laneAgents.begin(); i != laneAgents.end(); i++) {
			debugMsgs << "|" << (*i)->getId();
		}
	} else {
		debugMsgs << "Segment:" << lane->getRoadSegment()->getStartEnd() << "|LaneCopy: " << lane->getLaneID();
		for (std::deque<sim_mob::Person*>::const_iterator i = laneAgentsCopy.begin(); i != laneAgentsCopy.end(); i++) {
			debugMsgs << "|" << (*i)->getId();
		}
	}
	debugMsgs << std::endl;
	Print() << debugMsgs.str();
	debugMsgs.str("");
}

void LaneStats::verifyOrdering() {
	double distance = -1.0;
	for (std::deque<sim_mob::Person*>::const_iterator i = laneAgents.begin(); i != laneAgents.end(); i++) {
		if (distance >= (*i)->distanceToEndOfSegment) {
			debugMsgs
					<< "Invariant violated: Ordering of laneAgents does not reflect ordering w.r.t. distance to end of segment."
					<< "\nSegment: "
					<< lane->getRoadSegment()->getStartEnd()
					<< " length = "
					<< lane->getRoadSegment()->getLaneZeroLength()
					<< "\nLane: " << lane->getLaneID() << "\nCulprit Person: "
					<< (*i)->getId();
			debugMsgs << "\nAgents ";
			for (std::deque<sim_mob::Person*>::const_iterator j = laneAgents.begin(); j != laneAgents.end(); j++) {
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
	return laneStatsMap.find(lane)->second->dequeue(person, isQueuingBfrUpdate);
}

sim_mob::Person* sim_mob::LaneStats::dequeue(const sim_mob::Person* person, bool isQueuingBfrUpdate) {
	if (laneAgents.size() == 0) {
		throw std::runtime_error("Trying to dequeue from empty lane.");
	}
	sim_mob::Person* p;
	if(person == laneAgents.front() || ( !laneInfinity)){
		p = this->dequeue(isQueuingBfrUpdate);
	}
	else{
		if (laneInfinity) {
			std::deque<sim_mob::Person*>::iterator it;
			for (it = laneAgents.begin(); it != laneAgents.end() ; /*NOTE: no incrementation of the iterator here*/) {
				if ((*it) == person){
					p = (*it);
					it = laneAgents.erase(it); // erase returns the next iterator
					if (isQueuingBfrUpdate) {
						if (queueCount > 0) {
							// we have removed a queuing agent
							queueCount--;
						}
						else {
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
				}
				else {
					++it; // otherwise increment it by yourself
				}
			}
		}
	}
	return p;
}

} // end of namespace sim_mob
