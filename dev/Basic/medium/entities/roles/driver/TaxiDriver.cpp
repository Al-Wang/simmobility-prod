/*
 * TaxiDriver.cpp
 *
 *  Created on: 5 Nov 2016
 *      Author: jabir
 */

#include "entities/roles/driver/TaxiDriver.hpp"
#include "Driver.hpp"
#include "entities/controllers/MobilityServiceControllerManager.hpp"
#include "geospatial/network/RoadNetwork.hpp"
#include "logging/ControllerLog.hpp"
#include "message/MessageBus.hpp"
#include "message/MobilityServiceControllerMessage.hpp"
#include "path/PathSetManager.hpp"

using namespace sim_mob;
using namespace medium;
using namespace messaging;

TaxiDriver::TaxiDriver(Person_MT* parent, const MutexStrategy& mtxStrat, TaxiDriverBehavior* behavior,
                       TaxiDriverMovement* movement, std::string roleName, Role<Person_MT>::Type roleType) :
		Driver(parent, behavior, movement, roleName, roleType)
{
	taxiPassenger = nullptr;
	taxiDriverMovement = movement;
	taxiDriverBehaviour = behavior;
}

TaxiDriver::TaxiDriver(Person_MT* parent, const MutexStrategy& mtx) :
		Driver(parent, nullptr, nullptr, "", RL_TAXIDRIVER)
{

}

bool TaxiDriver::addPassenger(Passenger *passenger)
{
	//Assign the taxiPassenger, this will be used by on hail drivers to alight the passenger
	if (!taxiPassenger)
	{
		taxiPassenger = passenger;
	}

	const string &personId = passenger->getParent()->getDatabaseId();

	if(taxiPassengers.find(personId) == taxiPassengers.end())
	{
		taxiPassengers[personId] = passenger;
	}
	else
	{
		stringstream msg;
		msg << "Driver " << parent->getDatabaseId()
		    << " is trying to add passenger " << personId << "that has already been picked-up.";
		throw runtime_error(msg.str());
	}

	return true;
}

const Node *TaxiDriver::getCurrentNode() const
{
	if (taxiDriverMovement)
	{
		return taxiDriverMovement->getCurrentNode();
	}
	return nullptr;
}

const MobilityServiceDriver *TaxiDriver::exportServiceDriver() const
{
#ifndef NDEBUG
	return dynamic_cast<const MobilityServiceDriver*> (this);
#else
	return this;
#endif
}

Passenger *TaxiDriver::getPassenger()
{
	return taxiPassenger;
}

void TaxiDriver::alightPassenger()
{
	if (taxiPassenger != nullptr)
	{
		Passenger *passenger = taxiPassenger;
		taxiPassenger = nullptr;
		taxiPassengers.erase(passenger->getParent()->getDatabaseId());
		Person_MT *parentPerson = passenger->getParent();

		if (parentPerson)
		{
			MesoPathMover &pathMover = taxiDriverMovement->getMesoPathMover();
			const SegmentStats *segStats = pathMover.getCurrSegStats();
			Conflux *parentConflux = segStats->getParentConflux();
			parentConflux->dropOffTaxiTraveler(parentPerson);

			if(!taxiDriverMovement->isSubscribedToOnHail())
			{
				ControllerLog() << "Drop-off of user" << parentPerson->getDatabaseId() << " at time "
				                << parentPerson->currTick
				                << ". Message was sent at ??? with startNodeId ???, destinationNodeId "
				                << parentConflux->getConfluxNode()->getNodeId()
				                << ", and driverId " << getParent()->getDatabaseId() << std::endl;

				//Drop-off schedule complete, process next item
				processNextScheduleItem();
			}
			else
			{
				if (taxiDriverMovement->CruiseOnlyOrMoveToTaxiStand())      //Decision point.Logic Would be Replaced as per Bathen's Input
				{
					setDriverStatus(CRUISING);
					taxiDriverMovement->selectNextLinkWhileCruising();
				}
				else
				{
					taxiDriverMovement->driveToTaxiStand();
				}
			}
		}
	}
}

const unsigned long TaxiDriver::getPassengerCount() const
{
	return taxiPassengers.size();
}

void TaxiDriver::passengerChoiceModel(const Node *origin,const Node *destination, std::vector<WayPoint> &currentRouteChoice)
{
	std::vector<WayPoint> res;
	SubTrip currSubTrip;
	currSubTrip.origin = WayPoint(origin);
	currSubTrip.destination = WayPoint(destination);
	const Lane *currentLane = taxiDriverMovement->getCurrentlane();

	/**On call drivers use in simulation travel-times, on hail drivers do not*/
	bool useInSimulationTT = !taxiDriverMovement->isSubscribedToOnHail();

	currentRouteChoice = PrivateTrafficRouteChoice::getInstance()->getPathAfterPassengerPickup(currSubTrip, false,
	                                                                                           nullptr, currentLane,
	                                                                                           useInSimulationTT);

	if(!currentRouteChoice.empty())
	{
		currentRouteChoice.insert(currentRouteChoice.begin(), WayPoint(currentLane->getParentSegment()->getParentLink()));
	}
}

void TaxiDriver::HandleParentMessage(messaging::Message::MessageType type, const messaging::Message& message)
{
	switch (type)
	{
	case MSG_SCHEDULE_PROPOSITION:
	{
		const SchedulePropositionMessage &msg = MSG_CAST(SchedulePropositionMessage, message);
		assignedSchedule = msg.getSchedule();
		controller = msg.GetSender();
		currScheduleItem = assignedSchedule.begin();
		processNextScheduleItem(false);
		break;
	}
	case MSG_UNSUBSCRIBE_SUCCESSFUL:
	{
		parent->setToBeRemoved();
		break;
	}
	default:
	{
		break;
	}
	}
}


Person_MT *TaxiDriver::getParent()
{
	return parent;
}

TaxiDriverMovement *TaxiDriver::getMovementFacet()
{
	return taxiDriverMovement;
}

void TaxiDriver::pickUpPassngerAtNode(const std::string personId)
{
	MesoPathMover &pathMover = taxiDriverMovement->getMesoPathMover();
	const SegmentStats *segStats = pathMover.getCurrSegStats();
	Conflux *parentConflux = segStats->getParentConflux();

	//Store the number of passengers currently in the vehicle
	unsigned long prevPassengerCount = getPassengerCount();

	if (!parentConflux)
	{
		return;
	}

	Person_MT *personToPickUp = parentConflux->pickupTaxiTraveler(personId);

#ifndef NDEBUG
	if (!personToPickUp && !taxiDriverMovement->isSubscribedToOnHail())
	{
		stringstream msg;
		msg << "Pickup failed for " << personId << " at time "
		    << parent->currTick
		    << ", and driverId " << parent->getDatabaseId() << ". personToPickUp is NULL" << std::endl;
		throw runtime_error(msg.str());
	}
#endif

	if (personToPickUp)
	{
		Role<Person_MT> *curRole = personToPickUp->getRole();
		sim_mob::medium::Passenger *passenger = dynamic_cast<sim_mob::medium::Passenger *>(curRole);

#ifndef NDEBUG
		if (!passenger)
		{
			stringstream msg;
			msg << "Pickup failed for " << personId << " at time "
			    << parent->currTick
			    << ", and driverId " << parent->getDatabaseId()
			    << ". personToPickUp is not a passenger"
			    << std::endl;
			throw runtime_error(msg.str());
		}
#endif

		if (taxiDriverMovement->isSubscribedToOnHail())
		{
			std::vector<SubTrip>::iterator subTripItr = personToPickUp->currSubTrip;
			WayPoint personTravelDestination = (*subTripItr).destination;
			const Node *personDestinationNode = personTravelDestination.node;
			std::vector<WayPoint> currentRouteChoice;
			const Node *currentNode = taxiDriverMovement->getDestinationNode();

			if (currentNode == personDestinationNode)
			{
				return;
			}

			passengerChoiceModel(currentNode, personDestinationNode, currentRouteChoice);

			if(currentRouteChoice.empty())
			{
				currentRouteChoice = StreetDirectory::Instance().SearchShortestDrivingPath<Link, Node>(
						*(segStats->getRoadSegment()->getParentLink()), *personDestinationNode);
			}

#ifndef NDEBUG
			if (currentRouteChoice.empty())
			{
				stringstream msg;
				msg << "Pickup failed for " << personId << " at time "
				    << parent->currTick
				    << ", and driverId " << parent->getDatabaseId() << ", No path found from "
				    << currentNode->getNodeId() << " to " << personDestinationNode->getNodeId()
				    << std::endl;
				throw runtime_error(msg.str());
			}
#endif
			addPassenger(passenger);
			taxiDriverMovement->setDestinationNode(personDestinationNode);
			taxiDriverMovement->setCurrentNode(currentNode);;
			taxiDriverMovement->addRouteChoicePath(currentRouteChoice);
			passenger->setStartPoint(WayPoint(taxiDriverMovement->getCurrentNode()));
			passenger->setEndPoint(WayPoint(taxiDriverMovement->getDestinationNode()));
			setDriverStatus(DRIVE_WITH_PASSENGER);
			passenger->Movement()->startTravelTimeMetric();
		}
		else
		{
			//On call passenger
			addPassenger(passenger);
			setDriverStatus(DRIVE_WITH_PASSENGER);
			passenger->Movement()->startTravelTimeMetric();
			passenger->setStartPoint(WayPoint(taxiDriverMovement->getCurrentNode()));

			ControllerLog() << "Pickup succeeded for " << personId << " at time "
			                << parent->currTick
			                << ". Message was sent at ??? with startNodeId "
			                << parentConflux->getConfluxNode()->getNodeId() << ", destinationNodeId "
			                << personToPickUp->currSubTrip->destination.node->getNodeId()
			                << ", and driverId " << parent->getDatabaseId() << std::endl;

			//Pick-up schedule is complete, process next schedule item
			processNextScheduleItem();
		}
	}//else I am a onHail driver and I found no passenger at the node
}

void TaxiDriver::processNextScheduleItem(bool isMoveToNextScheduleItem)
{
	if(isMoveToNextScheduleItem)
	{
		//Move to next schedule item
		++currScheduleItem;
	}

	//If entire schedule is complete, cruise around unless we're in a parking
	if(currScheduleItem == assignedSchedule.end())
	{
		//Remove the taxi driver from the simulation if the shift has ended
		if((parent->currTick.ms() / 1000) >= taxiDriverMovement->getCurrentFleetItem().endTime)
		{
			ControllerLog() << "Driver " << parent->getDatabaseId() << " has completed the schedule and "
			                << "is at the end of its shift. Time = " << parent->currTick << std::endl;

			MessageBus::PostMessage(controller, MSG_DRIVER_SHIFT_END,
			                        MessageBus::MessagePtr(new DriverShiftCompleted(parent)));

			//Assuming that the controller always sends a schedulw with a park item at the end.
			//So, we would have parked the vehicle at this point and now the shift has ended
			//No need to do anything, as we set the vehicle to be removed after the controller
			//responds to the above message
		}
		else
		{
			if((currScheduleItem - 1)->scheduleItemType != PARK)
			{
				taxiDriverMovement->setCruisingMode();
			}
		}

		return;
	}

	switch (currScheduleItem->scheduleItemType)
	{
	case PICKUP:
	{
		if (driverStatus == MobilityServiceDriverStatus::PARKED)
		{
			getResource()->setMoving(true);

			Conflux *conflux = Conflux::getConfluxFromNode(taxiDriverMovement->getCurrentNode());
			MessageBus::PostMessage(conflux, MSG_PERSON_LOAD, MessageBus::MessagePtr(new PersonMessage(parent)));

			//Clear previous path
			taxiDriverMovement->getMesoPathMover().eraseFullPath();
		}

		const TripRequestMessage &tripRequest = currScheduleItem->tripRequest;
		const std::map<unsigned int, Node *> &nodeIdMap = RoadNetwork::getInstance()->getMapOfIdvsNodes();
		std::map<unsigned int, Node *>::const_iterator it = nodeIdMap.find(tripRequest.startNodeId);

#ifndef NDEBUG
		if (it == nodeIdMap.end())
		{
			std::stringstream msg;
			msg << "The schedule received start with node " << it->first << " which is not valid";
			throw std::runtime_error(msg.str());
		}
		if (!MobilityServiceControllerManager::HasMobilityServiceControllerManager())
		{
			throw std::runtime_error(
					"MobilityServiceControllerManager::HasMobilityServiceControllerManager() == false");
		}
#endif

		ControllerLog() << "Processing pick-up for " << tripRequest
		                << ". This assignment is started by driver "
		                << this->getParent()->getDatabaseId() << " at time " << parent->currTick << std::endl;

		const Node *node = it->second;

		if(taxiDriverMovement->getMesoPathMover().getCurrSegStats()->getParentConflux()->getConfluxNode() == node)
		{
			pickUpPassngerAtNode(tripRequest.userId);
			return;
		}

		const bool success = taxiDriverMovement->driveToNodeOnCall(tripRequest, node);

#ifndef NDEBUG
		if (!success)
		{
			std::stringstream msg;
			msg << __FILE__ << ":" << __LINE__ << ": taxiDriverMovement->processNextScheduleItem("
			    << tripRequest.userId << "," << node->getNodeId() << ");" << std::endl;
			WarnOut(msg.str());
		}
#endif

		break;
	}
	case DROPOFF:
	{
		const TripRequestMessage &tripRequest = currScheduleItem->tripRequest;

		//Check which passenger is to be dropped-off
		Passenger *passengerToDrop = taxiPassenger = taxiPassengers[tripRequest.userId];

		if(!passengerToDrop)
		{
			std::stringstream msg;
			msg << "The passenger " << tripRequest.userId << " has not been picked-up by driver " <<
					parent->getDatabaseId();
			throw std::runtime_error(msg.str());
		}

		const Person_MT *personToDrop = passengerToDrop->getParent();
		std::vector<SubTrip>::iterator subTripItr = personToDrop->currSubTrip;
		WayPoint personTravelDestination = (*subTripItr).destination;
		const Node *personDestinationNode = personTravelDestination.node;
		std::vector<WayPoint> currentRouteChoice;
		const Node *currentNode = taxiDriverMovement->getDestinationNode();

		if (currentNode == personDestinationNode)
		{
			alightPassenger();
			return;
		}

		passengerChoiceModel(currentNode, personDestinationNode, currentRouteChoice);

		if (currentRouteChoice.empty())
		{
			const Link *currentLink = taxiDriverMovement->getCurrentlane()->getParentSegment()->getParentLink();
			currentRouteChoice = StreetDirectory::Instance().SearchShortestDrivingPath<Link, Node>(*currentLink,
			                                                                                       *personDestinationNode);
		}

		if(!currentRouteChoice.empty())
		{
			taxiDriverMovement->setDestinationNode(personDestinationNode);
			taxiDriverMovement->setCurrentNode(currentNode);;
			taxiDriverMovement->addRouteChoicePath(currentRouteChoice);
			passengerToDrop->setEndPoint(WayPoint(taxiDriverMovement->getDestinationNode()));
			setDriverStatus(DRIVE_WITH_PASSENGER);

			ControllerLog() << "Processing drop-off for " << tripRequest
			                << ". This assignment is started by driver " <<
			                this->getParent()->getDatabaseId() << " at time " << parent->currTick << std::endl;
		}
		else
		{
			stringstream msg;
			msg << "Processing drop-off for " << tripRequest
			    << " failed as no route found by driver " <<
			    this->getParent()->getDatabaseId() << " at time " << parent->currTick << std::endl;
			throw runtime_error(msg.str());
		}

		break;
	}
	case CRUISE:
	{
		try
		{
			taxiDriverMovement->cruiseToNode(currScheduleItem->nodeToCruiseTo);
			ControllerLog() << "Driver " << parent->getDatabaseId() << " received CRUISE to node "
			                << currScheduleItem->nodeToCruiseTo->getNodeId() << std::endl;
		}
		catch (exception &ex)
		{
			ControllerLog() << ex.what();
		}

		break;
	}

	case PARK:
	{
		if (!getPassengerCount())
		{
			const SMSVehicleParking *destinationParking = currScheduleItem->parking;
			ControllerLog() << "Taxi driver " << getParent()->getDatabaseId()
			                << " received a Park command with Parking ID " << destinationParking->getParkingId()
			                << std::endl;

			const Node *destination = destinationParking->getAccessNode();
			const SegmentStats *currSegStat = taxiDriverMovement->getParentDriver()->getParent()->getCurrSegStats();
			const Link *link = currSegStat->getRoadSegment()->getParentLink();
			taxiDriverMovement->setCurrentNode(link->getToNode());
			const Node *thisNode = taxiDriverMovement->getCurrentNode();

			if (thisNode == destination)
			{
				ControllerLog() << "Taxi driver " << getParent()->getDatabaseId()
				                << "already in requested parking location" << std::endl;
				setDriverStatus(PARKED);
				getResource()->setMoving(false);
				parent->setRemainingTimeThisTick(0.0);
				taxiDriverMovement->setCurrentNode(thisNode);
				assignedSchedule = Schedule();

				MessageBus::PostMessage(controller, MSG_DRIVER_AVAILABLE,
				                        MessageBus::MessagePtr(new DriverAvailableMessage(
						                        taxiDriverMovement->getParentDriver()->parent)));
				return;
			}

			const bool success = taxiDriverMovement->driveToParkingNode(destination);

#ifndef NDEBUG
			if (!success)
			{
				std::stringstream msg;
				msg << __FILE__ << ":" << __LINE__ << ": taxiDriverMovement->driveToParkingNode("
				    << destination->getNodeId() << ");" << std::endl;
				msg << "Taxi with Driver " << parent->getDatabaseId() << " can not be parked at "
				    << destinationParking->getParkingId() << std::endl;
				WarnOut(msg.str());
			}
#endif

			ControllerLog() << "Assignment response sent for Parking command for Parking ID "
			                << destinationParking->getParkingId() << ". This response is sent by driver "
			                << parent->getDatabaseId() << " at time " << parent->currTick << std::endl;
			break;

		}
		else
		{
			ControllerLog() << "Taxi driver " << getParent()->getDatabaseId()
			                << " can not go for parking as some passenger left to drop off." << std::endl;
			throw runtime_error("All Passengers should be Dropped Off before Parking");
		}

	}

	default:
		throw runtime_error("Invalid Schedule item type");
	}
}

Role<Person_MT>* TaxiDriver::clone(Person_MT *parent) const
{
	if (parent)
	{
		TaxiDriverBehavior* behavior = new TaxiDriverBehavior();
		TaxiDriverMovement* movement = new TaxiDriverMovement();
		TaxiDriver* driver = new TaxiDriver(parent, parent->getMutexStrategy(),behavior, movement, "TaxiDriver_");
		behavior->setParentDriver(driver);
		movement->setParentDriver(driver);
		movement->setParentTaxiDriver(driver);
		return driver;
	}
	return nullptr;
}

void TaxiDriver::make_frame_tick_params(timeslice now)
{
	getParams().reset(now);
}

std::vector<BufferedBase*> TaxiDriver::getSubscriptionParams()
{
	return std::vector<BufferedBase*>();
}

const std::vector<MobilityServiceController*>& TaxiDriver::getSubscribedControllers() const
{
	return taxiDriverMovement->getSubscribedControllers();
}

TaxiDriver::~TaxiDriver()
{
	if (MobilityServiceControllerManager::HasMobilityServiceControllerManager() &&
		(parent->currTick.ms() / 1000) < taxiDriverMovement->getCurrentFleetItem().endTime)
	{
		for(auto it = taxiDriverMovement->getSubscribedControllers().begin();
			it != taxiDriverMovement->getSubscribedControllers().end(); ++it)
		{
			MessageBus::PostMessage(*it, MSG_DRIVER_UNSUBSCRIBE,
			                        MessageBus::MessagePtr(new DriverUnsubscribeMessage(parent)));
#ifndef NDEBUG
						ControllerLog()<< __FILE__ <<":" <<__LINE__<<":" <<__FUNCTION__<< ": Driver with pointer "<< this <<
								" unsubscribed because it is being destroyed "<< std::endl;
#endif
		}
	}
}
