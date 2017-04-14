/*
 * TaxiDriver.cpp
 *
 *  Created on: 5 Nov 2016
 *      Author: jabir
 */

#include <entities/roles/driver/TaxiDriver.hpp>
#include "path/PathSetManager.hpp"
#include "Driver.hpp"
#include "entities/controllers/VehicleControllerManager.hpp"
#include "geospatial/network/RoadNetwork.hpp"
#include "message/MessageBus.hpp"
#include "message/VehicleControllerMessage.hpp"

namespace sim_mob
{
namespace medium
{


TaxiDriver::TaxiDriver(Person_MT* parent, const MutexStrategy& mtxStrat,
		TaxiDriverBehavior* behavior, TaxiDriverMovement* movement,
		std::string roleName, Role<Person_MT>::Type roleType) :
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
	if (taxiPassenger == nullptr)
	{
		taxiPassenger = passenger;
		return true;
	}
	return false;
}

Passenger* TaxiDriver::getPassenger()
{
	return taxiPassenger;
}

void TaxiDriver::alightPassenger()
{
	if (taxiPassenger != nullptr)
	{
		Passenger *passenger = taxiPassenger;
		taxiPassenger = nullptr;
		Person_MT *parentPerson = passenger->getParent();
		if (parentPerson)
		{
			MesoPathMover &pathMover = taxiDriverMovement->getMesoPathMover();
			const SegmentStats* segStats = pathMover.getCurrSegStats();
			Conflux *parentConflux = segStats->getParentConflux();
			parentConflux->dropOffTaxiTraveler(parentPerson);

			Print() << "Drop-off for " << parentPerson->getDatabaseId() << " at time " << parentPerson->currTick.frame()
				<< ". Message was sent at null with startNodeId null, destinationNodeId " << parentConflux->getConfluxNode()->getNodeId()
				<< ", and taxiDriverId null" << std::endl;
		}
	}
}

void TaxiDriver::passengerChoiceModel(const Node *origin,const Node *destination, std::vector<WayPoint> &currentRouteChoice)
{
	std::vector<WayPoint> res;
	bool useInSimulationTT = parent->usesInSimulationTravelTime();
	SubTrip currSubTrip;
	currSubTrip.origin = WayPoint(origin);
	currSubTrip.destination = WayPoint(destination);
	const Lane * currentLane = taxiDriverMovement->getCurrentlane();
	currentRouteChoice = PrivateTrafficRouteChoice::getInstance()->getPathAfterPassengerPickup(currSubTrip, false, nullptr, currentLane,useInSimulationTT);
}

void TaxiDriver::HandleParentMessage(messaging::Message::MessageType type, const messaging::Message& message)
{
	switch (type)
	{
		case MSG_VEHICLE_ASSIGNMENT:
		{
			const VehicleAssignmentMessage& msg = MSG_CAST(VehicleAssignmentMessage, message);

			Print() << "Assignment received for " << msg.personId << " at time " << parent->currTick.frame()
				<< ". Message was sent at " << msg.currTick.frame() << " with startNodeId " << msg.startNodeId
				<< ", destinationNodeId " << msg.destinationNodeId << ", and taxiDriverId null" << std::endl;

			std::map<unsigned int, Node*> nodeIdMap = RoadNetwork::getInstance()->getMapOfIdvsNodes();

			std::map<unsigned int, Node*>::iterator it = nodeIdMap.find(msg.destinationNodeId); 
			if (it == nodeIdMap.end()) {
				Print() << "Message contains bad destination node" << std::endl;

				if (VehicleControllerManager::HasVehicleControllerManager())
				{
					std::map<unsigned int, VehicleController*> controllers = VehicleControllerManager::GetInstance()->getControllers();
				
					messaging::MessageBus::PostMessage(controllers[1], MSG_VEHICLE_ASSIGNMENT_RESPONSE,
						messaging::MessageBus::MessagePtr(new VehicleAssignmentResponseMessage(parent->currTick, false, msg.personId, parent->getDatabaseId(),
							msg.startNodeId, msg.destinationNodeId)));

					Print() << "Assignment response sent for " << msg.personId << " at time " << parent->currTick.frame()
						<< ". Message was sent at " << msg.currTick.frame() << " with startNodeId " << msg.startNodeId
						<< ", destinationNodeId " << msg.destinationNodeId << ", and taxiDriverId null" << std::endl;
				}

				return;
			}
			Node* node = it->second;

			const bool success = taxiDriverMovement->driveToNodeOnCall(msg.personId, node);

			if (VehicleControllerManager::HasVehicleControllerManager())
			{
				std::map<unsigned int, VehicleController*> controllers = VehicleControllerManager::GetInstance()->getControllers();

				messaging::MessageBus::PostMessage(controllers[1], MSG_VEHICLE_ASSIGNMENT_RESPONSE,
					messaging::MessageBus::MessagePtr(new VehicleAssignmentResponseMessage(parent->currTick, success, msg.personId, parent->getDatabaseId(),
						msg.startNodeId, msg.destinationNodeId)));

				Print() << "Assignment response sent for " << msg.personId << " at time " << parent->currTick.frame()
					<< ". Message was sent at " << msg.currTick.frame() << " with startNodeId " << msg.startNodeId
					<< ", destinationNodeId " << msg.destinationNodeId << ", and taxiDriverId " << parent->getDatabaseId() << std::endl;
			}

			break;
		}
		default:
		{
			break;
		}
	}
}

void TaxiDriver::setTaxiDriveMode(const DriverMode &mode)
{
	taxiDriverMode = mode;
	driverMode = mode;
}

const DriverMode & TaxiDriver::getDriverMode() const
{
	return taxiDriverMode;
}

Person_MT *TaxiDriver::getParent()
{
	return parent;
}

TaxiDriverMovement *TaxiDriver::getMovementFacet()
{
	return taxiDriverMovement;
}

void TaxiDriver::pickUpPassngerAtNode(Conflux *parentConflux, std::string* personId)
{
	if (!parentConflux)
	{
		return;
	}
	Person_MT *personToPickUp = parentConflux->pickupTaxiTraveler(personId);
	if (personToPickUp)
	{
		std::string id = personToPickUp->getDatabaseId();
		Role<Person_MT>* curRole = personToPickUp->getRole();
		sim_mob::medium::Passenger* passenger = dynamic_cast<sim_mob::medium::Passenger*>(curRole);
		if (passenger)
		{
			std::vector<SubTrip>::iterator subTripItr = personToPickUp->currSubTrip;
			WayPoint personTravelDestination = (*subTripItr).destination;
			const Node * personDestinationNode = personTravelDestination.node;
			std::vector<WayPoint> currentRouteChoice;
			const Node * currentNode = taxiDriverMovement->getDestinationNode();
			if (currentNode == personDestinationNode)
			{
				return;
			}
			passengerChoiceModel(currentNode, personDestinationNode, currentRouteChoice);
			if (currentRouteChoice.size() > 0)
			{
				bool isAdded = addPassenger(passenger);
				if (isAdded)
				{
					const Lane * currentLane = taxiDriverMovement->getCurrentlane();
					const Link* currentLink = currentLane->getParentSegment()->getParentLink();
					currentRouteChoice.insert(currentRouteChoice.begin(), WayPoint(currentLink));
					taxiDriverMovement->setDestinationNode(personDestinationNode);
					taxiDriverMovement->setCurrentNode(currentNode);;
					taxiDriverMovement->addRouteChoicePath(currentRouteChoice);
					//passenger->setService(currentRouteChoice);
					passenger->setStartPoint(WayPoint(taxiDriverMovement->getCurrentNode()));
					passenger->setEndPoint(WayPoint(taxiDriverMovement->getDestinationNode()));
					setTaxiDriveMode(DRIVE_WITH_PASSENGER);
				}
			}
			/*else
			{
				sim_mob::BasicLogger& ptMoveLogger = sim_mob::Logger::log("nopathAfterPickupInCruising.csv");
				const SegmentStats* currentStats = taxiDriverMovement->getMesoPathMover().getCurrSegStats();
				if(currentStats)
				{
					ptMoveLogger << passenger->getParent()->getDatabaseId()<<",";
					ptMoveLogger << currentStats->getRoadSegment()->getLinkId()<<",";
					ptMoveLogger << currentStats->getRoadSegment()->getRoadSegmentId()<<",";
					ptMoveLogger << taxiDriverMovement->getCurrentNode()->getNodeId()<<",";
					ptMoveLogger << personDestinationNode->getNodeId()<<std::endl;
				}
			}*/
		}
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

TaxiDriver::~TaxiDriver()
{
}
}
}








