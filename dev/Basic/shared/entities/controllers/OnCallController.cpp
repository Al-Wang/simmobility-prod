/*
 * OnCallController.cpp
 *
 *  Created on: Feb 20, 2017
 *      Author: Akshay Padmanabha
 */

#include "OnCallController.hpp"

#include "geospatial/network/RoadNetwork.hpp"
#include "path/PathSetManager.hpp"
#include "util/GeomHelpers.hpp"
#include "util/Utils.hpp"

using namespace sim_mob;
using namespace messaging;
using namespace std;

OnCallController::OnCallController(const MutexStrategy &mtxStrat, unsigned int computationPeriod,
                                   MobilityServiceControllerType type_, unsigned id, std::string tripSupportMode_, TT_EstimateType ttEstimateType_,unsigned maxAggregatedRequests_,bool studyAreaEnabledController)
		: MobilityServiceController(mtxStrat, type_, id, tripSupportMode_,maxAggregatedRequests_,studyAreaEnabledController), scheduleComputationPeriod(computationPeriod),
		  ttEstimateType(ttEstimateType_),studyAreaEnabledController(studyAreaEnabledController)
{
	rebalancer = new LazyRebalancer(this); //jo SimpleRebalancer(this);
#ifndef NDEBUG
	isComputingSchedules = false;
#endif
}

OnCallController::~OnCallController()
{
	safe_delete_item(rebalancer);
}

void OnCallController::subscribeDriver(Person *driver)
{
#ifndef NDEBUG
	consistencyChecks("before subscription");
	if (isComputingSchedules)
		throw std::runtime_error("Trying to subscribe a driver while computing schedules. This should not happen");
#endif

	MobilityServiceController::subscribeDriver(driver);
	availableDrivers.insert(driver);

#ifndef NDEBUG
	if (driverSchedules.find(driver) != driverSchedules.end() )
		throw std::runtime_error("Trying to subscribe a driver already subscribed");
#endif

	driverSchedules.emplace(driver, Schedule());

#ifndef NDEBUG
	ControllerLog() << "After the subscription, subscribedDrivers.size()=" << subscribedDrivers.size()
	                << ", availableDrivers.size()=" << availableDrivers.size()
	                << ", driverSchedules.size()=" << driverSchedules.size() << std::endl;
	consistencyChecks("after subscription");
#endif
}

void OnCallController::unsubscribeDriver(Person *driver)
{
	MobilityServiceController::unsubscribeDriver(driver);

#ifndef NDEBUG
	if (driverSchedules.find(driver) == driverSchedules.end())
	{
		std::stringstream msg;
		msg << "Driver " << driver->getDatabaseId() << " has been subscribed but had no "
			<< "schedule associated. This is impossible. It should have had at least an empty schedule";
		throw std::runtime_error(msg.str());
	}

	unsigned scheduleSize = driverSchedules.at(driver).size();

	if (scheduleSize > 0)
	{
		std::stringstream msg;
		msg << "Driver " << driver->getDatabaseId()<< "(" << driver << ")" << " has a non empty schedule and she sent a message "
			<< "to unsubscribe. This is not admissible";
		throw std::runtime_error(msg.str());
	}
#endif

	driverSchedules.erase(driver);

	availableDrivers.erase(driver);
	partiallyAvailableDrivers.erase(driver);
    driverServingSharedRequests.erase(driver);

#ifndef NDEBUG
	consistencyChecks("unsubscribeDriver: end");
#endif
}

void OnCallController::driverAvailable(const Person *driver)
{
#ifndef NDEBUG
	consistencyChecks("driverAvailable: start");
	if(availableDrivers.find(driver) != availableDrivers.end())
	{
		std::stringstream msg;
		msg << "Driver id " << driver->getDatabaseId() << " is already present in availableDrivers";
		throw runtime_error(msg.str());
	}

	if(driverSchedules.find(driver) == driverSchedules.end())
	{
		std::stringstream msg;
		msg << "Driver id " << driver->getDatabaseId() << " is not present in driverSchedules";
		throw runtime_error(msg.str());
	}
#endif

	availableDrivers.insert(driver);

	// The driver has an empty schedule now
	driverSchedules[driver] = Schedule();

	//Remove from the partially available drivers
	partiallyAvailableDrivers.erase(driver);
    driverServingSharedRequests.erase(driver);

#ifndef NDEBUG
	consistencyChecks("driverAvailable: end");
#endif
}

void OnCallController::driverUnavailable(Person *person)
{
#ifndef NDEBUG
	consistencyChecks("driverUnavailable: start");
#endif

	availableDrivers.erase(person);

#ifndef NDEBUG
	consistencyChecks("driverUnavailable: end");
#endif
}

void OnCallController::onDriverShiftEnd(Person *driver)
{
	ControllerLog() << "Shift end msg received from driver " << driver->getDatabaseId()<<"(" << driver<< ")" << " at " << currTick<<std::endl;

	if(driverSchedules.at(driver).empty())
	{
		unsubscribeDriver(driver);

		MessageBus::PostMessage((MessageHandler *) driver, MSG_UNSUBSCRIBE_SUCCESSFUL,
		                        MessageBus::MessagePtr(new DriverUnsubscribeMessage(driver)));
	}
	else
	{
		//Driver has a non-empty schedule. This can only happen in the following case at the moment - the driver
		//sends the shift-end message at time t and the controller also sends a schedule proposition message at time t.
		//So, we send a delay shift-end message that forces the drivers to end their shifts only after finishing
		//the assigned schedule
		MessageBus::PostMessage((MessageHandler *) driver, MSG_DELAY_SHIFT_END,
		                        MessageBus::MessagePtr(new DelayShiftEndMessage(driver)));
	}
}

void OnCallController::onDriverScheduleStatus(Person *driver)
{
#ifndef NDEBUG
	ControllerLog() << "onDriverScheduleStatus(): driverSchedules.size() = " << driverSchedules.size() << endl;
#endif

	Schedule &schedule = driverSchedules[driver];

	//aa!!: If schedule is empty, this would be an error, right? Why would a driver update an empty schedule.
	//			We should rise an appropriate NDEBUG exception here.
	if (!schedule.empty())
	{
		schedule.erase(schedule.begin());
	}

#ifndef NDEBUG
	ControllerLog() << "onDriverScheduleStatus(): driverSchedules.size() = " << driverSchedules.size() << endl;
#endif
}

Entity::UpdateStatus OnCallController::frame_tick(timeslice now)
{
	currTick = now;

	if (localTick == scheduleComputationPeriod)
	{
		localTick = 0;

#ifndef NDEBUG
		if (isComputingSchedules)
		{
			throw std::runtime_error("At this point, the controller should not be computing schedules");
		}
		isComputingSchedules = true;
#endif
        if(!(requestQueue.empty()))
        {
            ControllerLog() << "Computing schedule: " << requestQueue.size() << " requests are in the queue, available drivers "
                            << availableDrivers.size() <<", partiallyAvailableDrivers.size()="<< partiallyAvailableDrivers.size()
                            << ", driverServingSharedRequests.size() "<<driverServingSharedRequests.size() <<" , "<< currTick
                            << std::endl;

            computeSchedules();
            ControllerLog() << "Computation schedule done: now " << requestQueue.size() << " requests are in the queue, available drivers "
                            << availableDrivers.size() <<", partiallyAvailableDrivers.size()="<< partiallyAvailableDrivers.size()
                            << ", driverServingSharedRequests.size() "<<driverServingSharedRequests.size() <<" , "<<currTick
                            << std::endl;
        }
#ifndef NDEBUG
		isComputingSchedules = false;
#endif
		if (currTick.frame() >= rebalancingInterval && currTick.frame() >= nextRebalancingFrame)
		{
			nextRebalancingFrame = currTick.frame() + rebalancingInterval;
			rebalancer->rebalance(vector<const Person *>(availableDrivers.begin(), availableDrivers.end()), currTick);
		}
	}
	else
	{
		localTick += 1;
	}

	return Entity::UpdateStatus::Continue;
}

void OnCallController::frame_output(timeslice now)
{
}

void OnCallController::HandleMessage(messaging::Message::MessageType type, const messaging::Message &message)
{
#ifndef NDEBUG
	if (isComputingSchedules)
	{
		throw std::runtime_error("At this point, the controller should not be computing schedules");
	}
#endif

	switch (type)
	{
	case MSG_DRIVER_AVAILABLE:
	{
		const DriverAvailableMessage &availableArgs = MSG_CAST(DriverAvailableMessage, message);
		driverAvailable(availableArgs.person);
#ifndef NDEBUG
		ControllerLog()<<"Driver "<< availableArgs.person->getDatabaseId()<<" is available again"<<std::endl;
#endif
		break;
	}
	case MSG_TRIP_REQUEST:
	{
		const TripRequestMessage &requestArgs = MSG_CAST(TripRequestMessage, message);

		ControllerLog() << "Request received by the controller of type "<< sim_mob::toString(controllerServiceType)<<" :" << requestArgs << ". This request is received at " <<
		                currTick << std::endl;

#ifndef NDEBUG
		if (currTick < requestArgs.timeOfRequest)
		{
			std::stringstream msg; msg<<"Request "<< requestArgs << " received at time "<< currTick<<
			". It means it has been received before it was issued: impossible";
			throw std::runtime_error(msg.str());
		}
#endif

		requestQueue.push_back(requestArgs);
		rebalancer->onRequestReceived(requestArgs.startNode);

		break;
	}
	case MSG_SCHEDULE_PROPOSITION_REPLY:
	{
		const SchedulePropositionReplyMessage &replyArgs = MSG_CAST(SchedulePropositionReplyMessage, message);
		if (!replyArgs.success)
		{
			ControllerLog() << "Assignment failure received from " << replyArgs.personId << " at time "
			                << currTick.frame() << ". Message was sent at " << replyArgs.currTick.frame()
			                << " with startNodeId "
			                << replyArgs.startNode->getNodeId() << ", destinationNodeId "
			                << replyArgs.destinationNode->getNodeId() << ", and driverId "
			                << replyArgs.driver->getDatabaseId() << std::endl;

			TripRequestMessage r;
			r.timeOfRequest = replyArgs.currTick;
			r.userId = replyArgs.personId;
			r.startNode = replyArgs.startNode;
			r.destinationNode = replyArgs.destinationNode;
			r.extraTripTimeThreshold = replyArgs.extraTripTimeThreshold;
			requestQueue.push_back(r);

			driverAvailable(replyArgs.driver);
		}
		else
		{
			ControllerLog() << "Assignment success received from " << replyArgs.personId << " at time "
			                << currTick.frame() << ". Message was sent at " << replyArgs.currTick.frame()
			                << " with startNodeId " << replyArgs.startNode->getNodeId()
			                << ", destinationNodeId " << replyArgs.destinationNode->getNodeId()
			                << ", and driverId "
			                << replyArgs.driver->getDatabaseId() << std::endl;

			driverUnavailable(replyArgs.driver);
		}
		break;
	}
	case MSG_DRIVER_SHIFT_END:
	{
		const DriverShiftCompleted &shiftCompletedArgs = MSG_CAST(DriverShiftCompleted, message);
		onDriverShiftEnd(shiftCompletedArgs.person);
		break;
	}
	case MSG_DRIVER_SCHEDULE_STATUS:
	{
		const DriverScheduleStatusMsg &statusMsgArgs = MSG_CAST(DriverScheduleStatusMsg, message);
		onDriverScheduleStatus(statusMsgArgs.person);
		break;
	}
	default:
		// If it is not a message specific to this controller, let the generic controller handle it
		MobilityServiceController::HandleMessage(type, message);
		break;
	};

}


void OnCallController::assignSchedule(const Person *driver, const Schedule &schedule, bool isUpdatedSchedule)
{
#ifndef NDEBUG
	if (!driver)
	{
		std::stringstream msg; msg<<__FILE__<<":"<<__LINE__<<": Trying to assign a schedule to a NULL driver. The schedule is "
		<<schedule;
		throw std::runtime_error(msg.str() );
	}

	if(driverSchedules.find(driver) == driverSchedules.end())
	{
		std::stringstream msg; msg<<__FILE__<<":"<<__LINE__<<": Trying to assign a schedule to an unknown driver "
		<< driver <<". The schedule is "<<schedule;
		Warn()<< msg.str();

		msg << " The id is "<< driver->getDatabaseId();
		throw std::runtime_error(msg.str() );
	}
#endif

	if (!isUpdatedSchedule)
	{
		MessageBus::PostMessage((MessageHandler *) driver, MSG_SCHEDULE_PROPOSITION, MessageBus::MessagePtr(
				new SchedulePropositionMessage(currTick, schedule, (MessageHandler *) this)));
	}
	else
	{
		MessageBus::PostMessage((MessageHandler *) driver, MSG_SCHEDULE_UPDATE, MessageBus::MessagePtr(
				new SchedulePropositionMessage(currTick, schedule, (MessageHandler *) this)));
	}

#ifndef NDEBUG
	if (driverSchedules.find(driver) == driverSchedules.end() ||
		(availableDrivers.find(driver) == availableDrivers.end() &&
		 (!partiallyAvailableDrivers.empty() && partiallyAvailableDrivers.find(driver) == partiallyAvailableDrivers.end())))
	{
		std::string answer1 = (driverSchedules.find(driver) != driverSchedules.end() ? "yes" : "no");
		std::string answer2 = (availableDrivers.find(driver) != availableDrivers.end() ? "yes" : "no");
		std::string driverId;
		try
		{
			driverId = driver->getDatabaseId();
		}
		catch (const std::exception &e)
		{
			std::stringstream msg;
			msg << __FILE__ << ":" << __LINE__ << ":Exception in retrieving the ID of the driver with pointer "
				<< driver << ". Check in warn if the driver has been removed. The exception is " << e.what();
			throw std::runtime_error(msg.str());
		}

		std::stringstream msg;
		msg << "Assigning a schedule to driver " << driverId
			<< ". She should be present both in availableDrivers and driverSchedules but is she present in driverSchedules? "
			<< answer1 << " and is she present in availableDrivers? " << answer2 << "Driver true Schedule: " << driver->exportServiceDriver()->getAssignedSchedule() <<std::endl
            << " Copy of schedules: "<< driverSchedules[driver];
		throw std::runtime_error(msg.str());
	}

	if (!isUpdatedSchedule && !driverSchedules[driver].empty())
	{
		std::stringstream msg; msg<<"Trying to assign a schedule to driver "<< driver->getDatabaseId() << " who already has one."<<
		" If you are using the greedy controller, this is not possible. Otherwise, please disable this error";
		throw runtime_error(msg.str());
	}
	unsigned availableDriversBeforeTheRemoval = availableDrivers.size();
#endif

	// We have just assigned a new schedule to the driver.
	// We remove the first item of the schedule from the controller's copy, so that the controller
	// know what part of the schedule is remaining. We do this because we want to prevent the controller
	// from modifying the scheduleItem that the driver is currently executing. Therefore, we give the 
	// controller the visibility only of the part of the schedule that the controller can modify, namely the entire
	// schedule minus the first schedule item
    Schedule controllersCopy = schedule;
	controllersCopy.erase(controllersCopy.begin());


	driverSchedules[driver] = controllersCopy;

	// The driver is not available anymore
	availableDrivers.erase(driver);

	//If this schedule only caters to 1 person, the add the driver to the list of partially available drivers
	//Schedule size 3 indicates a schedule for 1 person: pick-up, drop-off and park
    if (driverSchedules[driver].begin()->tripRequest.requestType == RequestType::TRIP_REQUEST_SHARED &&
        this->controllerServiceType == MobilityServiceControllerType::SERVICE_CONTROLLER_AMOD )
    {
        if (schedule.size() <= 3 && driverServingSharedRequests.count(driver) == 0)
        {
            driverServingSharedRequests.insert(driver);
        }
        else
        {
            driverServingSharedRequests.erase(driver);
        }
    }

    if (driverSchedules[driver].begin()->tripRequest.requestType == RequestType::TRIP_REQUEST_SHARED &&
        this->controllerServiceType != MobilityServiceControllerType::SERVICE_CONTROLLER_AMOD )
    {
	if (schedule.size() <= 3 && partiallyAvailableDrivers.count(driver) == 0)
	{
		partiallyAvailableDrivers.insert(driver);
	}
	else
	{
		partiallyAvailableDrivers.erase(driver);
	}
    }
#ifndef NDEBUG
	if (!isUpdatedSchedule && availableDrivers.size() != availableDriversBeforeTheRemoval-1)
	{
		std::stringstream msg; msg<<"The removal of driver "<<driver->getDatabaseId()<<" from the availableDrivers "
		<<"was not successful. In fact availableDriversBeforeTheRemoval="<<availableDriversBeforeTheRemoval<<" and "<<
		"availableDrivers.size()="<<availableDrivers.size();
		throw std::runtime_error(msg.str());
	}
#endif

	ControllerLog() << sim_mob::toString(this->getServiceType() )<< " controller ("<<toString()<<")sent this assignment to driver :" << driver->getDatabaseId() <<"("<<driver<<")"<< schedule <<". The assignement is sent at " <<
	                currTick << "to ";
#ifndef NDEBUG
	ControllerLog() <<", (MessageHandler *) driver="<<(MessageHandler *) driver;
#endif
	ControllerLog() << std::endl;
}

bool OnCallController::isCruising(const Person *driver) const
{
	const MobilityServiceDriver *currDriver = driver->exportServiceDriver();
	if (currDriver)
	{
		if (currDriver->getDriverStatus() == MobilityServiceDriverStatus::CRUISING)
		{
			return true;
		}
	}
#ifndef NDEBUG
	else throw std::runtime_error("Error in getting the MobilityServiceDriver");
#endif

	return false;
}

bool OnCallController::isParked(const Person *driver) const
{
	const MobilityServiceDriver *currDriver = driver->exportServiceDriver();
	if (currDriver)
	{
		if (currDriver->getDriverStatus() == MobilityServiceDriverStatus::PARKED)
		{
			return true;
		}
	}
#ifndef NDEBUG
	else throw std::runtime_error("Error in getting the MobilityServiceDriver");
#endif

	return false;
}

const Node *OnCallController::getCurrentNode(const Person *driver) const
{
	const MobilityServiceDriver *currDriver = driver->exportServiceDriver();
	if (currDriver)
	{
		const Node *currentNode = currDriver->getCurrentNode();
		return currentNode;
	}
	return nullptr;
}


const Person *OnCallController::findClosestDriver(const Node *node) const
{
	double bestDistance = std::numeric_limits<double>::max();
	double bestX, bestY;

	const Person *bestDriver = NULL;
	auto driver = availableDrivers.begin();

#ifndef NDEBUG
	unsigned nonCruisingDrivers = 0;
#endif

	while (driver != availableDrivers.end())
	{
#ifndef NDEBUG
		if ( driverSchedules.find(*driver) == driverSchedules.end()  )
		{
			std::stringstream msg;
			msg << "Driver " << (*driver)->getDatabaseId() << " and pointer " << *driver
				<< " exists in availableDrivers but not in driverSchedules";
			throw std::runtime_error(msg.str());
		}
#endif
		if (isCruising(*driver) || isParked(*driver))
		{
			const Node *driverNode = getCurrentNode(*driver);
			double currDistance = dist(node->getLocation(), driverNode->getLocation());

			if (currDistance < bestDistance)
			{
				bestDriver = *driver;
				bestDistance = currDistance;
				bestX = driverNode->getPosX();
				bestY = driverNode->getPosY();
			}
		}
#ifndef NDEBUG
		else{
			nonCruisingDrivers++;

			const MobilityServiceDriver* mobilityServiceDriver = (*driver)->exportServiceDriver();
			const std::string driverStatusStr = mobilityServiceDriver->getDriverStatusStr();
			std::stringstream msg; msg<<"Error: "<<__FILE__<<":" <<__LINE__<< ":Driver " << (*driver)->getDatabaseId() <<
				" is among the available drivers of a controller of type "<<
				sim_mob::toString(controllerServiceType) <<", but her state is "<<
				driverStatusStr<<
				" This driver is subscribed to the following controller types "<< mobilityServiceDriver->getSubscribedControllerTypesStr()<<
				". In the scenarios where a driver subscribed to an OnCall service is only subscribed to that service, "<<
					"ALL the available drivers MUST be cruising. If it is not the case, there is a bug. If you are running a more complex scenario, where a driver can be "
					<<"subscribed to different services at the same time, please remove this exception, compile and run again";
			throw std::runtime_error(msg.str() );
		}
#endif
		driver++;
	}

	std::stringstream msg;
	if (bestDriver != NULL)
	{
		msg << "Closest vehicle is at (" << bestX << ", " << bestY << ")" << std::endl;
	}
	else
	{
		msg << "No available driver, availableDrivers.size()=" << availableDrivers.size();
#ifndef NDEBUG
		msg <<", cruisingDrivers="<<nonCruisingDrivers;
#endif
		ControllerLog() << msg.str() << std::endl;
#ifndef NDEBUG
		if (! availableDrivers.empty() )
		{
			msg<<". In the scenarios where a driver subscribed to an OnCall service is only subscribed to that service, "<<
			"ALL the available drivers MUST be cruising. If it is not the case, there is a bug. If you are running a more complex scenario, where a driver can be "
			<<"subscribed to different services at the same time, please remove this exception, compile and run again";
			throw std::runtime_error(msg.str() );
		}
#endif
	}

	return bestDriver;
}


double OnCallController::evaluateSchedule(const Node *initialPosition, const Schedule &schedule,
                                          double additionalDelayThreshold, double waitingTimeThreshold) const
{
	double initialScheduleTimeStamp = currTick.getSeconds(); // In seconds
	double scheduleTimeStamp = initialScheduleTimeStamp;
	const Node *latestNode = initialPosition;
	unsigned numberOfPassengers = 0;

	// In this for loop we are predicting how the schedule would unfold
	for (Schedule::const_iterator scheduleItemIt = schedule.begin(); scheduleItemIt != schedule.end(); scheduleItemIt++)
	{

		const ScheduleItem scheduleItem = *scheduleItemIt;
		switch (scheduleItem.scheduleItemType)
		{
		case (PICKUP):
		{
			const TripRequestMessage &request = scheduleItem.tripRequest;
			// I verify if the waiting time is ok
			scheduleTimeStamp += getTT(latestNode, scheduleItem.tripRequest.startNode, ttEstimateType);
			if (scheduleTimeStamp - request.timeOfRequest.ms() / 1000.0 > waitingTimeThreshold)
			{
				return -1;
			}
			else
			{
				numberOfPassengers++;
			}
			break;
		}
		case (DROPOFF):
		{
			// Find the time that this traveller would need, if he were alone
			const TripRequestMessage &request = scheduleItem.tripRequest;
			const Node *startNode = scheduleItem.tripRequest.startNode;
			const Node *nextNode = scheduleItem.tripRequest.destinationNode;
			scheduleTimeStamp += getTT(latestNode, nextNode, ttEstimateType);
			double timeIfHeWereAlone = waitingTimeThreshold + getTT(startNode, nextNode, ttEstimateType);
			const double sharedTravelDelay = 600; //seconds
			double rideTimeThreshold = schedule.size() > 2 ? timeIfHeWereAlone + sharedTravelDelay : timeIfHeWereAlone;
			if (scheduleTimeStamp - request.timeOfRequest.getSeconds() > rideTimeThreshold)
			{
				return -1;
			}
			// else do nothing and continue checking the other items
			break;
		}
		default:
		{
			throw std::runtime_error("Why would you want to check a schedule item that is neither PICKUP nor DROPOFF?");
		}
		}

		if (numberOfPassengers == 0 && scheduleItemIt != schedule.end())
		{
			// At this point I have 0 passengers in the vehicle and I have still other items to check. This is not a valid
			// schedule
			return -1;
		}

	}

	// If I am there and the function did not return before, it means it is feasible
	double expectedVehicleTravelTime = scheduleTimeStamp - initialScheduleTimeStamp;
	return expectedVehicleTravelTime;
}

/*
//TODO: in the request itself, the user should specify the earliest and latest pickup and dropoff times
double OnCallController::evaluateSchedule(const Node *initialPosition, const Schedule &schedule,
                                          double additionalDelayThreshold, double waitingTimeThreshold) const
{
	double scheduleTimeStamp = currTick.ms() / 1000.0; // In seconds
	const Node* latestNode = initialPosition;

	// Check that each user is picked up before being dropped off
	std::set<string> dropoffs;
	for (const ScheduleItem &scheduleItem : schedule)
	{
		switch (scheduleItem.scheduleItemType)
		{
		case (ScheduleItemType::DROPOFF):
		{
			dropoffs.insert(scheduleItem.tripRequest.userId);

			const Node* nextNode= nodeIdMap.find(scheduleItem.tripRequest.destinationNodeId);
			scheduleTimeStamp += getTT( latestNode, nextNode, ttEstimateType);
			latestNodeId = nextNodeId;

			double earliestPickupTimeStamp = scheduleItem.tripRequest.timeOfRequest.ms() / 1000.0; // in seconds
			double minimumTravelTime = PrivateTrafficRouteChoice::getInstance()->getOD_TravelTime(
					scheduleItem.tripRequest.startNodeId, scheduleItem.tripRequest.destinationNodeId,
					DailyTime(currTick.ms()));
			double latestDropoffTimeStamp = earliestPickupTimeStamp + minimumTravelTime + additionalDelayThreshold;
			if (scheduleTimeStamp > latestDropoffTimeStamp)
			{
				return -1;
			}
			break;
		};
		case (ScheduleItemType::PICKUP):
		{
			if (dropoffs.find(scheduleItem.tripRequest.userId) != dropoffs.end())
			{
				// Trying to pick up a user who is scheduled to be dropped off before
				return -1;
			}

			unsigned nextNodeId = scheduleItem.tripRequest.startNodeId;
			scheduleTimeStamp += PrivateTrafficRouteChoice::getInstance()->getOD_TravelTime(
					latestNodeId, nextNodeId, DailyTime(currTick.ms()));
			latestNodeId = nextNodeId;

			double earliestPickupTimeStamp = scheduleItem.tripRequest.timeOfRequest.ms() / 100.0; // in seconds
			if (scheduleTimeStamp > earliestPickupTimeStamp + waitingTimeThreshold)
			{
				return -1;
			}
			break;

		};
		case (ScheduleItemType::CRUISE):
		{
			throw std::runtime_error(
					"CRUISE is an \"instantaneous\" schedule item, meaning that after the controller sends it, there is no reason to keep memory of it. Therefore, it should not be there anymore");

		};
		default:
		{
			throw runtime_error("Unknown schedule item type");
		}
		}
	}

	double travelTime = scheduleTimeStamp - currTick.ms() / 1000.0;

#ifndef NDEBUG
	if (travelTime <= 1e-5 && !schedule.empty() )
	{
		std::stringstream msg; msg<<"The travel time for this schedule of "<< schedule.size()<<" schedule items is 0. Why? Is it an error?"<<
			" The schedule is "<<schedule;
		throw std::runtime_error(msg.str());
	}
#endif

	return travelTime;
}
*/

double OnCallController::computeSchedule(const Node *initialNode, const Schedule &currentSchedule,
                                         const Group<TripRequestMessage> &additionalRequests,
                                         Schedule &newSchedule, bool isOptimalityRequired) const
{
	double travelTime = std::numeric_limits<double>::max();
	bool isFeasible = false;

	//Contruct the required ScheduleItems for the new requests
	std::vector<ScheduleItem> additionalScheduleItems;
	for (const TripRequestMessage &request : additionalRequests.getElements())
	{
		additionalScheduleItems.push_back(ScheduleItem(ScheduleItemType::PICKUP, request));
		additionalScheduleItems.push_back(ScheduleItem(ScheduleItemType::DROPOFF, request));
	}

	//https://stackoverflow.com/a/201729/2110769
	Schedule tempSchedule(currentSchedule);
	tempSchedule.insert(tempSchedule.end(), additionalScheduleItems.begin(), additionalScheduleItems.end());
#ifndef NDEBUG
	if (tempSchedule.empty())
	{
		std::stringstream msg; msg<<__FILE__<<":"<<__LINE__<<": An empty schedule was created. This is an error. currentSchedule.size()="<<
			currentSchedule.size()<<", additionalRequests.size()="<<additionalRequests.size()<<", tempSchedule.size()"<<
			tempSchedule.size()<<", additionalScheduleItems.size()="<<additionalScheduleItems.size();
		Print()<<msg.str()<<std::endl;
		throw std::runtime_error(msg.str());
	}

	if (currentSchedule.size() + additionalRequests.size()*2 != tempSchedule.size() )
	{
		std::stringstream msg; msg<<"currentSchedule.size()="<<currentSchedule.size()<<
			", additionalRequests.size()="<<additionalRequests.size()<<
			", tempSchedule.size()="<<tempSchedule.size()<<
			", while the new schedule should have the old schedule items + 2 schedule items "<<
			" per each new additional requests";
		throw std::runtime_error(msg.str() );
	}
#endif

	// sorting is necessary to correctly compute the permutations (see https://www.topcoder.com/community/data-science/data-science-tutorials/power-up-c-with-the-standard-template-library-part-1/)
	std::sort(tempSchedule.begin(), tempSchedule.end());

	double tempTravelTime;
	do
	{
		tempTravelTime = evaluateSchedule(initialNode, tempSchedule, additionalDelayThreshold, waitingTimeThreshold);
		if (tempTravelTime >= 0 && tempTravelTime < travelTime)
		{
			isFeasible = true;
			travelTime = tempTravelTime;
			newSchedule = tempSchedule;
		}
	}
	while (
			std::next_permutation(tempSchedule.begin(), tempSchedule.end()) &&

			// If i) optimality is not required and ii) we already found a feasible solution, we can just return that and stop.
			// If those two conditions are not met at the same time, we can continue
			!(
					!isOptimalityRequired && isFeasible
			)

			);

#ifndef NDEBUG
	ControllerLog()<<"Current schedule: ";
	for (const ScheduleItem& item : currentSchedule) ControllerLog()<< item<<",";
	ControllerLog()<<". Trying to add requests [";
	for (const TripRequestMessage& request : additionalRequests.getElements() ) ControllerLog()<<request;
	ControllerLog()<<"]. The optimal schedule is ";
	for (const ScheduleItem& item : newSchedule) ControllerLog()<< item<<",";
	ControllerLog()<<std::endl;
#endif

	if (isFeasible)
	{
		return travelTime;
	}
	else
	{
		return -1;
	}
}

bool OnCallController::canBeShared(const TripRequestMessage &r1, const TripRequestMessage &r2,
                                   double additionalDelayThreshold, double waitingTimeThreshold) const
{
#ifndef NDEBUG
	if (r1==r2)
	{
		std::stringstream msg; msg<<__FILE__<<":"<<__LINE__<<": trying to share "<<r1<<" with "<<r2<<" but they are the same";
		throw std::runtime_error(msg.str());
	}
#endif

	// We check if, in case we have an empty vehicle that start in the position we want (we choose either of
	// the two pick up points), it can serve both request while respecting the constraints
	Schedule emptySchedule;
	const Node *initialPosition = r1.startNode;
	double travelTime = evaluateSchedule(initialPosition, emptySchedule, additionalDelayThreshold,
	                                     waitingTimeThreshold);
	if (travelTime >= 0)
	{
		return true;
	}

	initialPosition = r2.startNode;
	travelTime = evaluateSchedule(initialPosition, emptySchedule, additionalDelayThreshold, waitingTimeThreshold);

	if (travelTime >= 0)
	{
		return true;
	}

	return false;
}

#ifndef NDEBUG
void OnCallController::consistencyChecks(const std::string& label) const
{
	if (subscribedDrivers.size() != driverSchedules.size())
	{
		std::stringstream msg;
		msg << label << " subscribedDrivers.size()=" << subscribedDrivers.size()
			<< " and driverSchedules.size()=" << driverSchedules.size() << ". They should be equal. ";

		for (const Person *driver : subscribedDrivers)
		{
			if (driverSchedules.find(driver) == driverSchedules.end())
			{
				msg << "Driver " << driver->getDatabaseId() << " is in subscribedDrivers but not in driverSchedules";
			}
		}

		throw std::runtime_error(msg.str());
	}

	for (const Person *driver : availableDrivers)
	{
		if (!isMobilityServiceDriver(driver))
		{
			std::stringstream msg;
			msg << "Driver " << driver->getDatabaseId()
				<< " is not a MobilityServiceDriver" << std::endl;
			throw std::runtime_error(msg.str());
		}

		const MobilityServiceDriver *mobilityServiceDriver = driver->exportServiceDriver();
		const MobilityServiceDriverStatus status = driver->exportServiceDriver()->getDriverStatus();

		if (status != CRUISING && status != PARKED)
		{
			std::stringstream msg;
			msg << "Driver " << driver->getDatabaseId() << " is among the available drivers but his status is:"
				<< driver->exportServiceDriver()->getDriverStatusStr() << ". This is not admitted at the moment";
			throw std::runtime_error(msg.str());
		}

		if (!driverSchedules.at(driver).empty())
		{
			std::stringstream msg;
			msg << "Driver " << driver->getDatabaseId()
				<< " is among the available drivers but her schedule is not empty:"
				<< driverSchedules.at(driver);
			throw std::runtime_error(msg.str());
		}
	}

	// Check if the same request is present more than once
	std::vector<TripRequestMessage> requestQueueCopy{std::begin(requestQueue), std::end(requestQueue)};
	std::sort(requestQueueCopy.begin(), requestQueueCopy.end());
	for (int i = 0; i < requestQueueCopy.size(); i++)
	{
		for (int j = i + 1; j < requestQueueCopy.size(); j++)
		{
			if (requestQueueCopy[i].userId == requestQueueCopy[j].userId)
			{
				std::stringstream msg;
				msg << "There are two requests from the same users currently in the requestQueue. They are " <<
					requestQueueCopy[i] << " and " << requestQueueCopy[j] << std::endl;
				throw std::runtime_error(msg.str());
			}
		}
	}
}
#endif

const std::string OnCallController::getRequestQueueStr() const
{
	std::stringstream msg;
	for (const TripRequestMessage &r : requestQueue)
	{
		msg << r << ", ";
	}
	return msg.str();
}

void
OnCallController::sendCruiseCommand(const Person *driver, const Node *nodeToCruiseTo, const timeslice currTick) const
{
#ifndef NDEBUG
	bool found = false;
	for (const Person* availableDriver : availableDrivers)
	{
		if (availableDriver == driver) found = true;
	}
	if (!found)
	{
		std::stringstream msg; msg<<"Trying to send a message to driver "<<driver->getDatabaseId()<<" pointer "<< driver<<
		" who is not among the available drivers";
		throw std::runtime_error(msg.str() );
	}

	for (const Person* availableDriver : subscribedDrivers)
	{
		if (availableDriver == driver) found = true;
	}
	if (!found)
	{
		std::stringstream msg; msg<<"Trying to send a message to driver "<<driver->getDatabaseId()<<" pointer "<< driver<<
		" who is not among the subscribed drivers";
		throw std::runtime_error(msg.str() );
	}

#endif

	ScheduleItem item(ScheduleItemType::CRUISE, nodeToCruiseTo);
	sim_mob::Schedule schedule;
	schedule.push_back(ScheduleItem(item));


	MessageBus::PostMessage((MessageHandler *) driver, MSG_SCHEDULE_PROPOSITION,
	                        MessageBus::MessagePtr(new SchedulePropositionMessage(currTick, schedule,
	                                                                              (MessageHandler *) this)));
}

double OnCallController::getTT(const Node *node1, const Node *node2, TT_EstimateType type) const
{
#ifndef NDEBUG
	if (
			(node1 == node2 && node1->getNodeId() !=  node2->getNodeId() ) ||
			(node1 != node2 && node1->getNodeId() ==  node2->getNodeId() )
	){
		throw std::runtime_error("Pointers of nodes do not correspond to their IDs for some weird reason");
	}
#endif

	double retValue;
	if (node1 == node2)
	{
		retValue = 0;
	}
	else
	{
		switch (type)
		{
		case (OD_ESTIMATION):
		{
			if(this->studyAreaEnabledController)
			{
				retValue = PrivateTrafficRouteChoice::getInstance()->getOD_TravelTime_StudyArea(
						node1->getNodeId(), node2->getNodeId(), DailyTime(currTick.ms()));
			}
			else
			{
				retValue = PrivateTrafficRouteChoice::getInstance()->getOD_TravelTime(
						node1->getNodeId(), node2->getNodeId(), DailyTime(currTick.ms()));
			}
			break;
		}
		case (SHORTEST_PATH_ESTIMATION):
		{
			retValue = PrivateTrafficRouteChoice::getInstance()->getShortestPathTravelTime(
					node1, node2, DailyTime(currTick.ms()));
			break;
		}
		case (EUCLIDEAN_ESTIMATION):
		{
			return getTT(node1->getLocation(), node2->getLocation());
			break;
		}
		default:
			throw std::runtime_error("Estimate type not recognized");
		}


		if (retValue <= 0)
		{    // The two nodes are different and the travel time should be non zero, if valid
			retValue = std::numeric_limits<double>::max();
		}
	}
	return retValue;
}

double OnCallController::getTT(const Point &point1, const Point &point2) const
{
	double squareDistance = pow(point1.getX() - point2.getX(), 2) + pow(
			point1.getY() - point2.getY(), 2);
	// We assume that the distance between node1 and node2 is a hypotenus of a right triangle and
	// that we go from a node to the other by crossing the two catheti
	double cathetus = sqrt(squareDistance) / sqrt(2.0);
	double distanceToCover = 2.0 * cathetus; // meters
	double speedAssumed = 30.0 * 1000 / 3600; //30Kmph converted in mps
	return distanceToCover / speedAssumed;
}

double OnCallController::toMs(int c) const
{
	return c / (CLOCKS_PER_SEC / 1000);
}

void OnCallController::assignSchedules(const unordered_map<const Person *, Schedule> &schedulesToAssign,
                                       bool isUpdatedSchedule)
{
	// After we decided all the schedules for all the drivers, we can send  them
	for (const pair<const Person *, Schedule> &p : schedulesToAssign)
	{
		const Person *driver = p.first;
		Schedule schedule = p.second;

		//Find where to park after the final drop off
		const Node *finalDropOffNode = schedule.back().tripRequest.destinationNode;

		//aa!!: Is the parking logic really only related to the IncrementalSharing. It seems to me that the parking
		//			logic is generally related to OnCallController. We should move that logic there, otherwise we will
		//			have to write always the same code for any controller.
		const SMSVehicleParking *parking =
				SMSVehicleParking::smsParkingRTree.searchNearestObject(finalDropOffNode->getPosX(),
				                                                       finalDropOffNode->getPosY());

		if (parking)
		{
			//Append the parking schedule item to the end
			const ScheduleItem parkingSchedule(PARK, parking);
			schedule.push_back(parkingSchedule);
		}
		else
		{
			ControllerLog()<<"Parking is not found near dropOff node "<<finalDropOffNode->getNodeId()<<" . trip is with Driver "<<driver->getDatabaseId()<<" and schedule is { "<<p.second<<" }"<<endl;
		}

		assignSchedule(driver, schedule, isUpdatedSchedule);
	}
}





