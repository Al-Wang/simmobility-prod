/*
 * PedestrainFacets.cpp
 *
 *  Created on: Mar 13, 2014
 *      Author: zhang huai peng
 */

#include "PedestrianFacets.hpp"

#include "conf/ConfigManager.hpp"
#include "config/MT_Config.hpp"
#include "logging/ControllerLog.hpp"
#include "message/MessageBus.hpp"
#include "Pedestrian.hpp"
#include "util/Utils.hpp"
#include "entities/controllers/MobilityServiceController.hpp"

using namespace sim_mob;
using namespace sim_mob::medium;
using namespace messaging;

PedestrianBehavior::PedestrianBehavior() : BehaviorFacet(), parentPedestrian(nullptr)
{
}

PedestrianBehavior::~PedestrianBehavior()
{
}

PedestrianMovement::PedestrianMovement(double speed) :
		MovementFacet(), parentPedestrian(nullptr), walkSpeed(speed), destinationNode(nullptr), totalTimeToCompleteSec(10),
		secondsInTick(ConfigManager::GetInstance().FullConfig().baseGranSecond())
{
}

PedestrianMovement::~PedestrianMovement()
{
}

void PedestrianMovement::setParentPedestrian(medium::Pedestrian* parentPedestrian)
{
	this->parentPedestrian = parentPedestrian;
}

TravelMetric& PedestrianMovement::startTravelTimeMetric()
{
	return travelMetric;
}

TravelMetric& PedestrianMovement::finalizeTravelTimeMetric()
{
	return travelMetric;
}

void PedestrianBehavior::setParentPedestrian(medium::Pedestrian* parentPedestrian)
{
	this->parentPedestrian = parentPedestrian;
}

void PedestrianMovement::frame_init()
{
	destinationNode = getDestNode();

	if(!destinationNode)
	{
		throw std::runtime_error("destination segment not found");
	}

	SubTrip& subTrip = *(parentPedestrian->parent->currSubTrip);
	double walkTime = 0.0;

	if(subTrip.isPT_Walk)
	{
		walkTime = subTrip.walkTime; //walk time comes from db for PT pedestrians
	}
	else if(subTrip.isTT_Walk)
	{
		isOnDemandTraveler = false;
		Person_MT *person = parentPedestrian->getParent();

		if (subTrip.origin.type == WayPoint::NODE && subTrip.destination.type == WayPoint::NODE)
		{
			isOnDemandTraveler = true;
			const Node *taxiStartNode = subTrip.destination.node;

			if (MobilityServiceControllerManager::HasMobilityServiceControllerManager())
			{
				std::vector<SubTrip>::iterator subTripItr = person->currSubTrip;


				if ((*subTripItr).travelMode == "TravelPedestrian" && subTrip.origin.node == subTrip.destination.node)
				{
					std::vector<SubTrip>::iterator taxiTripItr = subTripItr + 1;
					const Node *taxiEndNode = (*taxiTripItr).destination.node;
					TripChainItem *tcItem = *(person->currTripChainItem);

					//Choose the controller based on the stop mode in das (i.e. SMS/SMS_POOL,AMOD,RAIL_SMS,etc..)
					auto controllers = MobilityServiceControllerManager::GetInstance()->getControllers();
					MobilityServiceController *controller = nullptr;
					const ConfigParams &cfg = ConfigManager::GetInstance().FullConfig();
					auto enabledCtrlrs = cfg.mobilityServiceController.enabledControllers;
					//auto itCtrlr = controllers.get<ctrlrType>().find(SERVICE_CONTROLLER_AMOD);
					//auto itCtrlr = controllers.get<ctrlTripSupportMode>().find(tcItem->getMode());
					std::map<unsigned int, MobilityServiceControllerConfig>:: iterator itr ;

					for (itr = enabledCtrlrs.begin(); itr != enabledCtrlrs.end(); itr++)
					{
						std::string currentTripChainMode = boost::to_upper_copy(tcItem->getMode());
						if (boost::to_upper_copy(itr->second.tripSupportMode).find(currentTripChainMode.insert(0,"|").append("|"))!= std::string::npos)
						{
							auto itCtrlr = controllers.get<ctrlTripSupportMode>().find(itr->second.tripSupportMode);
							controller = *itCtrlr;
							break;
						}
					}

					if (!controller)
					{
						std::stringstream msg;
						msg << "Controller for person travelmode " <<tcItem->getMode()
						<< " is not present in config file,while the demand (DAS) have this mode";
						throw std::runtime_error(msg.str());
					}

					/*
					if((*taxiTripItr).travelMode.find("AMOD") != std::string::npos)
					{
						//If the person is taking an AMOD service, get the AMOD controller
						auto itCtrlr = controllers.get<ctrlrType>().find(SERVICE_CONTROLLER_AMOD);

						if (itCtrlr == controllers.get<ctrlrType>().end())
						{
							std::stringstream msg;
							msg << "Controller of type " << toString(SERVICE_CONTROLLER_AMOD)
							    << " has not been added, but "
							    << "the demand contains persons taking AMOD service";
							throw std::runtime_error(msg.str());
						}

						controller = *itCtrlr;
					}
					else
					{
						//Choose randomly from available controllers
						const ConfigParams &cfg = ConfigManager::GetInstance().FullConfig();
						auto enabledCtrlrs = cfg.mobilityServiceController.enabledControllers;
						auto it = enabledCtrlrs.begin();
						auto randomNum = Utils::generateInt(0, enabledCtrlrs.size() - 1);

						std::advance(it, randomNum);

						//Here we have to search by id, as the enabled controllers map has id as the key
						auto itCtrlr = controllers.get<ctrlrId>().find(it->first);
						controller = *itCtrlr;
					}
					 */

#ifndef NDEBUG
					consistencyChecks(controller->getServiceType());
					controller->consistencyChecks();
#endif

					//If the the request is a pool request, set type as shared. Else it is a single request
					RequestType reqType = RequestType::TRIP_REQUEST_SINGLE;
					if((*taxiTripItr).travelMode.find("Pool") != std::string::npos)
					{
						reqType = RequestType::TRIP_REQUEST_SHARED;
					}

					TripRequestMessage *request = new TripRequestMessage(person->currTick, person,
					                                                     person->getDatabaseId(), taxiStartNode,
					                                                     taxiEndNode,
					                                                     MobilityServiceController::toleratedExtraTime,
					                                                     reqType);
					MessageBus::PostMessage(controller, MSG_TRIP_REQUEST, MessageBus::MessagePtr(request));


					ControllerLog() << "Request sent to controller of type " << toString(controller->getServiceType())
					                << ": ID : " << controller->getControllerId() << ": " << *request << std::endl;
				}
			}
		}

		if(!isOnDemandTraveler)
		{
			const Node* source = subTrip.origin.node;
			const TaxiStand* stand = subTrip.destination.taxiStand;
			const Node* destination = stand->getRoadSegment()->getParentLink()->getFromNode();
			std::vector<WayPoint> path = StreetDirectory::Instance().SearchShortestDrivingPath<Node, Node>(*(source), *(destination));
			for (auto itWayPts = path.begin(); itWayPts != path.end(); ++itWayPts)
			{
				if (itWayPts->type == WayPoint::LINK)
				{
					TravelTimeAtNode item;
					item.node = itWayPts->link->getToNode();
					item.travelTime = itWayPts->link->getLength() / walkSpeed;
					travelPath.push(item);
				}
			}
		}

		Conflux* startConflux = this->getStartConflux();

		if (startConflux)
		{
			MessageBus::PostMessage(startConflux, MSG_TRAVELER_TRANSFER,
			                        MessageBus::MessagePtr(new PersonMessage(person)));
		}
	}
	else // both origin and destination must be nodes
	{
		if(subTrip.origin.type != WayPoint::NODE || subTrip.destination.type != WayPoint::NODE)
		{
			throw std::runtime_error("non node O/D for not PT pedestrian");
		}
		const Node* srcNode = subTrip.origin.node;
		const Node* destNode = subTrip.destination.node;

		DynamicVector distVector(srcNode->getLocation().getX(),srcNode->getLocation().getY(),destNode->getLocation().getX(),destNode->getLocation().getY());
		double distance = distVector.getMagnitude();
		walkTime = distance / walkSpeed;
	}

	parentPedestrian->setTravelTime(walkTime*1000);
}

const Node* PedestrianMovement::getDestNode()
{
	SubTrip& subTrip = *(parentPedestrian->parent->currSubTrip);
	const Node* destNd = nullptr;

	switch(subTrip.destination.type)
	{
	case WayPoint::NODE:
	{
		destNd = subTrip.destination.node;
		break;
	}
	case WayPoint::TRAIN_STOP:
	{
		const Node* srcNode = nullptr;
		switch(subTrip.origin.type)
		{
		case WayPoint::NODE:
		{
			srcNode = subTrip.origin.node;
			break;
		}
		case WayPoint::BUS_STOP:
		{
			srcNode = subTrip.origin.busStop->getParentSegment()->getParentLink()->getFromNode();
			break;
		}
		case WayPoint::TRAIN_STOP:
		{
			//this case should ideally not occur. handling just in case...
			srcNode = subTrip.origin.trainStop->getRandomStationSegment()->getParentLink()->getFromNode();
			break;
		}
		}
		destNd = subTrip.destination.trainStop->getStationSegmentForNode(srcNode)->getParentLink()->getToNode();
		break;
	}
	case WayPoint::BUS_STOP:
	{
		destNd = subTrip.destination.busStop->getParentSegment()->getParentLink()->getToNode();
		break;
	}
	case WayPoint::TAXI_STAND:
	{
		destNd = subTrip.destination.taxiStand->getRoadSegment()->getParentLink()->getFromNode();
		break;
	}
	}
	return destNd;
}

void PedestrianMovement::frame_tick()
{
	parentPedestrian->parent->setRemainingTimeThisTick(0);

	if (parentPedestrian->roleType == Role<Person_MT>::RL_TRAVELPEDESTRIAN)
	{
		unsigned int tickMS = ConfigManager::GetInstance().FullConfig().baseGranMS();
		parentPedestrian->setTravelTime(parentPedestrian->getTravelTime()+tickMS);
		double tickSec = ConfigManager::GetInstance().FullConfig().baseGranSecond();

		if (!isOnDemandTraveler)
		{
			TravelTimeAtNode& front = travelPath.front();
			if (front.travelTime < tickSec)
			{
				travelPath.pop();
				Conflux* start = this->getStartConflux();
				if (start)
				{
					MessageBus::PostMessage(start, MSG_TRAVELER_TRANSFER,
					                        MessageBus::MessagePtr(new PersonMessage(parentPedestrian->parent)));
				}
			}
			else
			{
				front.travelTime -= tickSec;
			}
			if (travelPath.size() == 0)
			{
				parentPedestrian->parent->setToBeRemoved();
			}
		}
		else
		{
			parentPedestrian->getParent()->setToBeRemoved();
		}
	}
}

std::string PedestrianMovement::frame_tick_output()
{
	return std::string();
}

Conflux* PedestrianMovement::getStartConflux() const
{
	if (parentPedestrian->roleType == Role<Person_MT>::RL_TRAVELPEDESTRIAN && travelPath.size() > 0)
	{
		const TravelTimeAtNode &front = travelPath.front();
		return MT_Config::getInstance().getConfluxForNode(front.node);
	}
	else if (parentPedestrian->roleType == Role<Person_MT>::RL_TRAVELPEDESTRIAN && isOnDemandTraveler)
	{
		SubTrip &subTrip = *(parentPedestrian->parent->currSubTrip);
		return MT_Config::getInstance().getConfluxForNode(subTrip.origin.node);
	}

	return nullptr;
}

Conflux* PedestrianMovement::getDestinationConflux() const
{
	if (destinationNode)
	{
		return MT_Config::getInstance().getConfluxForNode(destinationNode);
	}
	return nullptr;
}

bool PedestrianMovement::getOnDemandTraveller()
{
	return isOnDemandTraveler;
}
