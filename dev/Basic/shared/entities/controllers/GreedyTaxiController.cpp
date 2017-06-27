/*
 * GreedyTaxiController.cpp
 *
 *  Created on: Apr 18, 2017
 *      Author: Akshay Padmanabha
 */

#include "GreedyTaxiController.hpp"
#include "geospatial/network/RoadNetwork.hpp"
#include "logging/ControllerLog.hpp"

using namespace sim_mob;

void GreedyTaxiController::computeSchedules()
{
#ifndef NDEBUG
	consistencyChecks("beginning");
#endif

	ControllerLog() << "Computing schedule: " << requestQueue.size() << " requests are in the queue" << std::endl;

	std::list<TripRequestMessage>::iterator request = requestQueue.begin();
	if (!availableDrivers.empty())
	{
		while (request != requestQueue.end())
		{
			//{ RETRIEVE NODES
			std::map<unsigned int, Node *> nodeIdMap = RoadNetwork::getInstance()->getMapOfIdvsNodes();
			std::map<unsigned int, Node *>::iterator itStart = nodeIdMap.find((*request).startNodeId);
			std::map<unsigned int, Node *>::iterator itDestination = nodeIdMap.find((*request).destinationNodeId);

#ifndef NDEBUG
			if (itStart == nodeIdMap.end())
			{
				std::stringstream msg;
				msg << "Request from person " << (*request).personId << " contains bad start node "
					<< (*request).startNodeId << std::endl;
				throw std::runtime_error(msg.str());
			}
#endif

			//Assign the start node
			const Node *startNode = itStart->second;

#ifndef NDEBUG
			if (itDestination == nodeIdMap.end())
			{
				std::stringstream msg;
				msg << "Request from person " << (*request).personId << " contains bad destination node "
					<< (*request).destinationNodeId << std::endl;
				throw std::runtime_error(msg.str());
			}
#endif
			//} RETRIEVE NODES

			const Person *bestDriver = findClosestDriver(startNode);

			if (bestDriver)
			{
				Schedule schedule;
				const ScheduleItem pickUpScheduleItem(ScheduleItemType::PICKUP, *request);
				const ScheduleItem dropOffScheduleItem(ScheduleItemType::DROPOFF, *request);

				ControllerLog() << "Items constructed" << std::endl;

				schedule.push_back(pickUpScheduleItem);
				schedule.push_back(dropOffScheduleItem);

				assignSchedule(bestDriver, schedule);



#ifndef NDEBUG
				if (currTick < request->timeOfRequest)
					throw std::runtime_error("The assignment is sent before the request has been issued: impossible");
#endif

				request = requestQueue.erase(request);



			}
			else
			{
				//no available drivers
				ControllerLog() << "Requests to be scheduled " << requestQueue.size() << ", available drivers "
				                << availableDrivers.size() << std::endl;

				//Move to next request. Leave the unassigned request in the queue, we will process again this next time
				++request;
			}
		}
	}
	else
	{
		//no available drivers
		ControllerLog() << "Requests to be scheduled " << requestQueue.size() << ", available drivers "
		                << availableDrivers.size() << std::endl;
	}

#ifndef NDEBUG
	consistencyChecks("end");
#endif

}
