//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Person_ST.hpp"

#include "BusStopAgent.hpp"
#include "config/ST_Config.hpp"
#include "entities/roles/activityRole/ActivityPerformer.hpp"
#include "event/args/ReRouteEventArgs.hpp"
#include "message/MessageBus.hpp"
#include "message/ST_Message.hpp"
#include "path/PT_RouteChoiceLuaProvider.hpp"

#include <entities/roles/driver/OnCallDriverFacets.hpp>
#include "entities/roles/pedestrian/PedestrianFacets.hpp"

#include "geospatial/streetdir/RailTransit.hpp"
#include "util/GeomHelpers.hpp"
using namespace std;
using namespace sim_mob;

namespace
{
const string transitModeBus = "BusTravel";
const string transitModeTrain = "MRT";
const string transitModeUnknown = "PT";
const string travelModeWalk = "Walk";
}

Person_ST::Person_ST(const std::string &src, const MutexStrategy &mtxStrat, int id, std::string databaseID)
: Person(src, mtxStrat, id, databaseID), startLaneIndex(-1), boardingTimeSecs(0), alightingTimeSecs(0), 
prevRole(NULL), currRole(NULL), nextRole(NULL), commEventRegistered(false), amodId("-1"),
amodPickUpSegmentStr("-1"), startSegmentId(-1), segmentStartOffset(0), initialSpeed(0),
amodSegmLength(0.0), amodSegmLength2(0.0), client_id(0), isPositionValid(false), isVehicleInLoadingQueue(false),
rsTravelStats(nullptr),currFrame(0,0)
{
	setPersonCharacteristics();
}

Person_ST::Person_ST(const std::string &src, const MutexStrategy &mtxStrat, const std::vector<TripChainItem *> &tc)
: Person(src, mtxStrat, tc), startLaneIndex(-1), boardingTimeSecs(0), alightingTimeSecs(0), 
prevRole(NULL), currRole(NULL), nextRole(NULL), commEventRegistered(false), amodId("-1"),
amodPickUpSegmentStr("-1"), startSegmentId(-1), segmentStartOffset(0), initialSpeed(0), amodSegmLength(0.0), amodSegmLength2(0.0),
rsTravelStats(nullptr),currFrame(0,0)
{
	if (!tripChain.empty())
	{
		initTripChain();
	}
	
	setPersonCharacteristics();
}

Person_ST::~Person_ST()
{
	//Un-register event listeners.
	if (commEventRegistered)
	{
		messaging::MessageBus::UnSubscribeEvent(event::EVT_CORE_COMMSIM_ENABLED_FOR_AGENT, this, this);
	}
	
	safe_delete_item(prevRole);
	safe_delete_item(currRole);
	safe_delete_item(nextRole);
}

void Person_ST::setPath(std::vector<WayPoint> &path)
{
	if (path.size() == 0)
	{
		Print() << "Warning! Path size is zero!" << std::endl;
	}

	amodPath = path;
}

void Person_ST::setPersonCharacteristics()
{
	//Call the base class method
	Person::setPersonCharacteristics();
	
	const ConfigParams &config = ConfigManager::GetInstance().FullConfig();
	const std::map<int, PersonCharacteristics>& personCharacteristics = config.personCharacteristicsParams.personCharacteristics;
	
	const int defaultLowerSecs = config.personCharacteristicsParams.DEFAULT_LOWER_SECS;
	const int defaultUpperSecs = config.personCharacteristicsParams.DEFAULT_UPPER_SECS;
	
	boost::mt19937 gen(getId() * getId());

	if(!personCharacteristics.empty())
	{
		for (std::map<int, PersonCharacteristics>::const_iterator iter = personCharacteristics.begin();
			 iter != personCharacteristics.end(); ++iter)
		{
			if (this->getAge() >= iter->second.lowerAge && this->getAge() < iter->second.upperAge)
			{
				boost::uniform_int<> BoardingTime(iter->second.lowerSecs, iter->second.upperSecs);
				boost::variate_generator<boost::mt19937, boost::uniform_int<int> > varBoardingTime(gen, BoardingTime);
				boardingTimeSecs = varBoardingTime();

				boost::uniform_int<> AlightingTime(iter->second.lowerSecs, iter->second.upperSecs);
				boost::variate_generator<boost::mt19937, boost::uniform_int<int> > varAlightingTime(gen, AlightingTime);
				alightingTimeSecs = varAlightingTime();

				walkingSpeed = iter->second.walkSpeed;
			}
		}
	}
	else
	{
		boost::uniform_int<> BoardingTime(defaultLowerSecs, defaultUpperSecs);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varBoardingTime(gen, BoardingTime);
		boardingTimeSecs = varBoardingTime();

		boost::uniform_int<> AlightingTime(defaultLowerSecs, defaultUpperSecs);
		boost::variate_generator < boost::mt19937, boost::uniform_int<int> > varAlightingTime(gen, AlightingTime);
		alightingTimeSecs = varAlightingTime();
	}
}

void Person_ST::setStartTime(unsigned int value)
{
	Entity::setStartTime(value);
	if (currRole)
	{
		currRole->setArrivalTime(value + ConfigManager::GetInstance().FullConfig().simStartTime().getValue());
	}
}

void Person_ST::load(const map<string, string> &configProps)
{
}

void Person_ST::initTripChain()
{
	convertPublicTransitODsToTrips();
	convertToTaxiTrips();
	convertToSmartMobilityTrips();
	insertWaitingActivityToTrip();
	assignSubtripIds();
	currTripChainItem = tripChain.begin();
	const std::string& src = getAgentSrc();

	setStartTime((*currTripChainItem)->startTime.offsetMS_From(ConfigManager::GetInstance().FullConfig().simStartTime()));
	
	if ((*currTripChainItem)->itemType == TripChainItem::IT_TRIP)
	{
		currSubTrip = ((dynamic_cast<Trip*> (*currTripChainItem))->getSubTripsRW()).begin();
		
		if (!updateOD(*currTripChainItem))
		{ 
			//Offer some protection
			std::stringstream msg;
			msg << __func__ << ": Trip/Activity mismatch, or unknown TripChainItem subclass.";
			throw std::runtime_error(msg.str());
		}
	}

	setNextPathPlanned(false);
	isFirstTick = true;
}

Entity::UpdateStatus Person_ST::checkTripChain(unsigned int currentTime)
{
	if (tripChain.empty())
	{
#ifndef NDEBUG
        if ( exportServiceDriver() )
        {
            Warn()<<__FILE__<<":"<< __LINE__<<": The driver "<<getDatabaseId() << " with pointer "<< this << " is done"<<std::endl;
        }
#endif
		return UpdateStatus::Done;
	}

	//advance the trip, sub-trip or activity....

    //advance the trip, sub-trip or activity....
    TripChainItem *chainItem=*(tripChain.begin());
    if(chainItem->itemType != TripChainItem::IT_ON_CALL_TRIP)
    {
        if (!isFirstTick)
        {
            if (!(advanceCurrentTripChainItem()))
            {
#ifndef NDEBUG
                if (exportServiceDriver())
                {
                    Warn() << __FILE__ << ":" << __LINE__ << ": The driver " << getDatabaseId() << " with pointer "
                           << this << " is done" << std::endl;
                }
#endif
                return UpdateStatus::Done;
            }
            if (isTripValid())
            {
                currSubTrip->startTime = DailyTime(currentTime);
            }
        }
    }
    else
    {
        if (!isFirstTick)
        {
#ifndef NDEBUG
            if ( exportServiceDriver() )
            {
                Warn()<<__FILE__<<":"<< __LINE__<<": The driver "<<getDatabaseId() << " with pointer "<< this << " is done"<<std::endl;
            }
#endif
            return UpdateStatus::Done;
        }
    }
	
	//must be set to false whenever trip chain item changes. And it has to happen before a probable creation of (or changing to) a new role
	setNextPathPlanned(false);

	//Create a new Role based on the trip chain type
	updatePersonRole();

	//Update our origin/destination pair.
	if ((*currTripChainItem)->itemType == TripChainItem::IT_TRIP)
	{ 
		//put if to avoid & evade bus trips, can be removed when everything is ok
		updateOD(*currTripChainItem, &(*currSubTrip));
	}

	//currentTipchainItem or current sub-trip are changed
	//so OD will be changed too,
	//therefore we need to call frame_init regardless of change in the role
	unsetInitialized();

	//Create a return type based on the differences in these Roles
	vector<BufferedBase*> prevParams;
	vector<BufferedBase*> currParams;
	
	if (prevRole)
	{
		prevParams = prevRole->getSubscriptionParams();
	}
	
	if (currRole)
	{
		currParams = currRole->getSubscriptionParams();
	}
	
	if (isFirstTick && currRole)
	{
		currRole->setArrivalTime(startTime + ConfigManager::GetInstance().FullConfig().simStartTime().getValue());
	}
	
	isFirstTick = false;

	//Null out our trip chain, remove the "removed" flag, and return
	clearToBeRemoved();
	return UpdateStatus(UpdateStatus::RS_CONTINUE, prevParams, currParams);
}

bool Person_ST::findPersonNextRole()
{
	if (!updateNextTripChainItem())
	{
		safe_delete_item(nextRole);
		return false;
	}

	//Prepare to delete the previous Role. We _could_ delete it now somewhat safely, but
	//it's better to avoid possible errors (e.g., if the equality operator is defined)
	//by saving it until the next time tick.
	//safe_delete_item(prevRole);
	safe_delete_item(nextRole);
	const RoleFactory<Person_ST> *rf = RoleFactory<Person_ST>::getInstance();

	const TripChainItem* tci = *(this->nextTripChainItem);
	
	if (tci->itemType == TripChainItem::IT_TRIP)
	{
		nextRole = rf->createRole(tci, &(*nextSubTrip), this);
	}

	return true;
}

bool Person_ST::updatePersonRole()
{
	if (!nextRole)
	{
		const RoleFactory<Person_ST> *rf = RoleFactory<Person_ST>::getInstance();
		const TripChainItem *tci = *(this->currTripChainItem);
		const SubTrip* subTrip = NULL;

		if (tci->itemType == TripChainItem::IT_TRIP)
		{
			subTrip = &(*currSubTrip);
			(*currSubTrip).startTime = DailyTime(currTick.ms());
		}
		nextRole = rf->createRole(tci, subTrip, this);
	}

	changeRole();
	return true;
}

void Person_ST::changeRole()
{
	safe_delete_item(prevRole);
	prevRole = currRole;
	currRole = nextRole;
	nextRole = nullptr;
}

vector<BufferedBase *> Person_ST::buildSubscriptionList()
{
	//First, add the x and y co-ordinates
	vector<BufferedBase *> subsList;
	subsList.push_back(&xPos);
	subsList.push_back(&yPos);

	//Now, add our own properties.
	if(this->getPrevRole())
	{
		vector<BufferedBase*> roleParams = this->getPrevRole()->getSubscriptionParams();

		//Append the subsList with all elements in roleParams
		subsList.insert(subsList.end(), roleParams.begin(), roleParams.end());
	}

	if (this->getRole())
	{
		vector<BufferedBase*> roleParams = this->getRole()->getSubscriptionParams();

		//Append the subsList with all elements in roleParams
		subsList.insert(subsList.end(), roleParams.begin(), roleParams.end());
	}

	return subsList;
}

Entity::UpdateStatus Person_ST::frame_init(timeslice now)
{
	Entity::UpdateStatus result(Entity::UpdateStatus::RS_CONTINUE);
	
	//Call its "load" function
	load(configProperties);
	clearConfigProperties();

	messaging::MessageBus::RegisterHandler(this);
	currTick = now;

	//Reset the Region tracking data structures, if applicable.
	//regionAndPathTracker.reset();

	//Register for communication simulator messages, if applicable.
	if (!commEventRegistered && ST_Config::getInstance().commsim.enabled)
	{
		commEventRegistered = true;
		messaging::MessageBus::SubscribeEvent(event::EVT_CORE_COMMSIM_ENABLED_FOR_AGENT, this, this);
	}

	//Agents may be created with a null Role and a valid trip chain
	if (!currRole)
	{
		setStartTime(now.ms());

		result = checkTripChain();

		if (currRole)
		{
			currRole->setArrivalTime(now.ms() + ConfigManager::GetInstance().FullConfig().baseGranMS());
		}

		//Nothing left to do?
		if (result.status == Entity::UpdateStatus::RS_DONE)
		{
			return result;
		}
	}

	//Failsafe
	if (!currRole)
	{
		std::ostringstream txt;
		txt << __func__ << ": Person " << this->getId() << " has no Role.";
		throw std::runtime_error(txt.str());
	}

	//Get an UpdateParams instance.
	currRole->make_frame_tick_params(now);

	//Now that the Role has been fully constructed, initialise it.
	if ((*currTripChainItem))
    {
           currRole->Movement()->frame_init();
    }
    /* check to add from MT: revisit*/
    setInitialized(true);
	ConfigManager::GetInstanceRW().FullConfig().numTripsLoaded++;
	
	return result;
}

Entity::UpdateStatus Person_ST::frame_tick(timeslice now)
{
	const ConfigParams& config = ConfigManager::GetInstance().FullConfig();

	currTick = now;
	//TODO: Here is where it gets risky.
	if (isResetParamsRequired())
	{
		currRole->make_frame_tick_params(now);
		setResetParamsRequired(false);
	}

	Entity::UpdateStatus retVal(UpdateStatus::RS_CONTINUE);

	if (!isToBeRemoved())
	{
		currRole->Movement()->frame_tick();
	}

	//If we're "done", try checking to see if we have any more items in our Trip Chain.
	// This is not strictly the right way to do things (we shouldn't use "isToBeRemoved()"
	// in this manner), but it's the easiest solution that uses the current API.
	//TODO: This section should technically go after frame_output(), but doing that
	//      (by overriding Person::update() and calling Agent::update() then inserting this code)
	//      will bring us outside the bounds of our try {} catch {} statement. We might move this
	//      statement into the worker class, but I don't want to change too many things
	//      about Agent/Person at once. ~Seth
	if (isToBeRemoved())
	{
		//Reset the start time (to the NEXT time tick) so our dispatcher doesn't complain.
		setStartTime(now.ms() + config.baseGranMS());

		retVal = checkTripChain();

		if (currTripChainItem != tripChain.end())
		{
			TripChainItem *tcItem = *currTripChainItem;
			
			// if currTripChain has not ended and has value, switch roles and call frame_init
			if (tcItem)
			{
				if (tcItem->itemType == TripChainItem::IT_ACTIVITY)
				{
					//IT_ACTIVITY as of now is just a matter of waiting for a period of time(between its start and end time)
					//since start time of the activity is usually later than what is configured initially,
					//we have to make adjustments so that the person waits for exact amount of time
					ActivityPerformer<Person_ST> *ap = dynamic_cast<ActivityPerformer<Person_ST>* > (currRole);
					Activity* acItem = dynamic_cast<Activity *>(tcItem);
					ap->setActivityStartTime(DailyTime(now.ms() + config.baseGranMS()));
					ap->setActivityEndTime(DailyTime(now.ms() + config.baseGranMS() + (tcItem->endTime.getValue() - tcItem->startTime.getValue())));
					ap->setLocation(acItem->destination.node);
				}
                //register the person as a message handler if required
                if (!GetContext())
                {
                    messaging::MessageBus::RegisterHandler(currRole->getParent());
                }
				
				if (!isInitialized())
				{
					currRole->Movement()->frame_init();
					setInitialized(true); // set to be false so later no need to frame_init later
				}
				
				if(currRole->roleType == Role<Person_ST>::RL_WAITBUSACTIVITY)
				{
					assignPersonToBusStopAgent();
				}
                else if(currRole->roleType == Role<Person_ST>::RL_ON_CALL_DRIVER)
                {
                    auto *onCallDrvMvt = dynamic_cast<const OnCallDriverMovement *>(currRole->Movement());
                    if(onCallDrvMvt)
                    {
                        retVal = UpdateStatus::Continue;
                    }
                    else
                    {
                        throw std::runtime_error("OnCallDriver role facets not/incorrectly initialized");
                    }
                }
                else if(currRole->roleType == Role<Person_ST>::RL_DRIVER)
                {
                    const DriverMovement *driverMvt = dynamic_cast<const DriverMovement *>(currRole->Movement());
                    if (driverMvt)
                    {
                        retVal = UpdateStatus::Continue;
                    }
                    else
                    {
                        throw std::runtime_error("Driver role facets not/incorrectly initialized");
                    }
                }
                else if(currRole->roleType ==Role<Person_ST>::RL_PEDESTRIAN || currRole->roleType == Role<Person_ST>::RL_TRAVELPEDESTRIAN)
                {
                    const PedestrianMovement *pedestrianMvt = dynamic_cast<const PedestrianMovement *>(currRole->Movement());
                    if (pedestrianMvt)
                    {
                        retVal = UpdateStatus::Continue;
                    }
                    else
                    {
                        throw std::runtime_error("Pedestrian role facets not/incorrectly initialized");
                    }
                }
                else if(currRole->roleType == Role<Person_ST>::RL_WAITTAXIACTIVITY)
                {
                    retVal = UpdateStatus::Continue;
                }
            }
        }
        else
        {
            ConfigManager::GetInstanceRW().FullConfig().numTripsCompleted++;
        }
    }
	return retVal;
}

void Person_ST::frame_output(timeslice now)
{
	//Save the output
	if (!isToBeRemoved())
	{
		LogOut(currRole->Movement()->frame_tick_output());
	}

	setResetParamsRequired(true);
}

bool Person_ST::advanceCurrentTripChainItem()
{
	if (currTripChainItem == tripChain.end()) /*just a harmless basic check*/
	{
		return false;
	}

	// current role (activity or sub-trip level role)[for now: only subtrip] is about to change,
	// time to collect its movement metrics(even activity performer)
	if (currRole != nullptr)
	{
		currRole->collectTravelTime();
		TravelMetric currRoleMetrics = currRole->Movement()->finalizeTravelTimeMetric();
		currRole->Movement()->resetTravelTimeMetric(); //sorry for manual reset, just a precaution for now
		serializeSubTripChainItemTravelTimeMetrics(currRoleMetrics, currTripChainItem, currSubTrip);
	}

	//first check if you just need to advance the subtrip
	if ((*currTripChainItem)->itemType == TripChainItem::IT_TRIP)
	{
		//don't advance to next tripchainItem immediately, check the subtrip first
		bool res = advanceCurrentSubTrip();

		//subtrip advanced successfully, no need to advance currTripChainItem
		if (res)
		{
			return res;
		}
	}

	//do the increment
	++currTripChainItem;

	if (currTripChainItem == tripChain.end())
	{
		//but tripchain items are also over, get out !
		return false;
	}

	//so far, advancing the tripchainitem has been successful
	//Also set the currSubTrip to the beginning of trip , just in case
	if ((*currTripChainItem)->itemType == TripChainItem::IT_TRIP)
	{
		currSubTrip = resetCurrSubTrip();
	}

	return true;
}

void Person_ST::convertPublicTransitODsToTrips()
{
	vector<TripChainItem *>::iterator tripChainItemIt;
	
	//Iterate over the trip-chain to identify the public transit trips
	for (tripChainItemIt = tripChain.begin(); tripChainItemIt != tripChain.end(); ++tripChainItemIt)
	{
		if ((*tripChainItemIt)->itemType == TripChainItem::IT_TRIP)
		{			
			TripChainItem *trip = (*tripChainItemIt);
			
			string originId = boost::lexical_cast<string>(trip->origin.node->getNodeId());
			string destId = boost::lexical_cast<string>(trip->destination.node->getNodeId());
			
			trip->startLocationId = originId;
			trip->endLocationId = destId;
			
			vector<SubTrip> &subTrips = (dynamic_cast<Trip *> (*tripChainItemIt))->getSubTripsRW();
			vector<SubTrip>::iterator itSubTrip = subTrips.begin();
			vector<SubTrip> newSubTrips;
			
			while (itSubTrip != subTrips.end())
			{
				if (itSubTrip->origin.type == WayPoint::NODE && itSubTrip->destination.type == WayPoint::NODE)
				{
					if (itSubTrip->getMode() == transitModeUnknown || itSubTrip->getMode() == transitModeBus ||
							itSubTrip->getMode() == transitModeTrain)
					{
						vector<OD_Trip> odTrips;
						const string &dbid = this->getDatabaseId();
						
						const string &src = getAgentSrc();
						DailyTime subTripStartTime = itSubTrip->startTime;
						const ConfigParams &cfgParams = ConfigManager::GetInstance().FullConfig();
						
						const std::string ptPathsetStoredProcName =
								cfgParams.getDatabaseProcMappings().procedureMappings["pt_pathset"];

						bool ret = PT_RouteChoiceLuaProvider::getPTRC_Model().getBestPT_Path(itSubTrip->origin.node->getNodeId(),
										itSubTrip->destination.node->getNodeId(), subTripStartTime.getValue(), odTrips, dbid,
										itSubTrip->startTime.getValue(), ptPathsetStoredProcName);
						
						if (ret)
						{
							ret = makeODsToTrips(&(*itSubTrip), newSubTrips, odTrips,
							                     PT_NetworkCreater::getInstance(), itSubTrip->getMode());
						}

						if (!ret)
						{
							tripChain.clear();
							ConfigManager::GetInstanceRW().FullConfig().numPathNotFound++;
							setToBeRemoved();
							return;
						}
					}
					else if(itSubTrip->getMode() == travelModeWalk)
					{
						string originId = boost::lexical_cast<string>(itSubTrip->origin.node->getNodeId());
						string destId = boost::lexical_cast<string>(itSubTrip->destination.node->getNodeId());

						itSubTrip->startLocationId = originId;
						itSubTrip->endLocationId = destId;
						itSubTrip->startLocationType = "NODE";
						itSubTrip->endLocationType = "NODE";
					}
					else if (itSubTrip->getMode().find("Car Sharing") != string::npos ||
							itSubTrip->getMode() == "PrivateBus")
					{
						string originId = boost::lexical_cast<string>(itSubTrip->origin.node->getNodeId());
						string destId = boost::lexical_cast<string>(itSubTrip->destination.node->getNodeId());

						itSubTrip->startLocationId = originId;
						itSubTrip->endLocationId = destId;
						itSubTrip->startLocationType = "NODE";
						itSubTrip->endLocationType = "NODE";

						if(itSubTrip->getMode() != "PrivateBus")
						{
							//modify the mode name for RoleFactory
							itSubTrip->travelMode = "Sharing";
						}

						const StreetDirectory& streetDirectory = StreetDirectory::Instance();
						vector<WayPoint> wayPoints =
								streetDirectory.SearchShortestDrivingPath<Node, Node>(*itSubTrip->origin.node,*itSubTrip->destination.node);

						double travelTime = 0.0;
						const TravelTimeManager* ttMgr = TravelTimeManager::getInstance();

						for (std::vector<WayPoint>::iterator it = wayPoints.begin(); it != wayPoints.end(); it++)
						{
							if (it->type == WayPoint::LINK)
							{
								travelTime += ttMgr->getDefaultLinkTT(it->link);
							}
						}
						itSubTrip->endTime = DailyTime(travelTime * 1000);
					}
				}
				++itSubTrip;
			}

			if (!newSubTrips.empty())
			{
				subTrips.clear();
				subTrips = newSubTrips;
			}
		}
	}
}

void Person_ST::insertWaitingActivityToTrip()
{
	vector<TripChainItem *>::iterator tripChainItem;
	for (tripChainItem = tripChain.begin(); tripChainItem != tripChain.end(); ++tripChainItem)
	{
		if ((*tripChainItem)->itemType == TripChainItem::IT_TRIP)
		{
			vector<SubTrip>::iterator itSubTrip[2];
			vector<SubTrip> &subTrips = (dynamic_cast<Trip *> (*tripChainItem))->getSubTripsRW();

			itSubTrip[1] = subTrips.begin();
			itSubTrip[0] = subTrips.begin();
			while (itSubTrip[1] != subTrips.end())
			{
				if (itSubTrip[1]->getMode() == transitModeBus && itSubTrip[0]->getMode() != "WaitingBusActivity")
				{
					if (itSubTrip[1]->origin.type == WayPoint::BUS_STOP)
					{
						SubTrip subTrip;
						subTrip.itemType = TripChainItem::getItemType("WaitingBusActivity");
						subTrip.origin = itSubTrip[1]->origin;
						subTrip.originType = itSubTrip[1]->originType;
						subTrip.destination = itSubTrip[1]->destination;
						subTrip.destinationType = itSubTrip[1]->destinationType;
						subTrip.startLocationId = itSubTrip[1]->origin.busStop->getStopCode();
						subTrip.endLocationId = itSubTrip[1]->destination.busStop->getStopCode();
						subTrip.startLocationType = "BUS_STOP";
						subTrip.endLocationType = "BUS_STOP";
						subTrip.travelMode = "WaitingBusActivity";
						subTrip.ptLineId = itSubTrip[1]->ptLineId;
						subTrip.edgeId = itSubTrip[1]->edgeId;
						itSubTrip[1] = subTrips.insert(itSubTrip[1], subTrip);
					}
				}

				itSubTrip[0] = itSubTrip[1];
				itSubTrip[1]++;
			}
		}
	}
}

void Person_ST::assignSubtripIds()
{
	for (vector<TripChainItem *>::iterator tcIt = tripChain.begin(); tcIt != tripChain.end(); tcIt++)
	{
		if ((*tcIt)->itemType == TripChainItem::IT_TRIP)
		{
			Trip *trip = dynamic_cast<Trip *> (*tcIt);
			string tripId = trip->tripID;
			stringstream stIdstream;
			vector<SubTrip> &subTrips = trip->getSubTripsRW();
			int stNo = 0;
			for (vector<SubTrip>::iterator stIt = subTrips.begin(); stIt != subTrips.end(); stIt++)
			{
				stNo++;
				stIdstream << tripId << "_" << stNo;
				(*stIt).tripID = stIdstream.str();
				stIdstream.str(string());
			}
		}
	}
}

void Person_ST::assignPersonToBusStopAgent()
{
	const BusStop *stop = nullptr;

	if (this->currSubTrip->origin.type == WayPoint::BUS_STOP)
	{
		stop = this->currSubTrip->origin.busStop;
	}

	if (!stop)
	{
		return;
	}

	//Make sure we dispatch this person only to SOURCE_TERMINUS or NOT_A_TERMINUS stops
	if (stop->getTerminusType() == sim_mob::SINK_TERMINUS)
	{
		stop = stop->getTwinStop();
		
		if (stop->getTerminusType() == sim_mob::SINK_TERMINUS)
		{
			std::stringstream msg;
			msg << __func__ << ": Both twin stops - " << stop->getStopCode() << ", " << stop->getTwinStop()->getStopCode() << " - are SINKs";
			throw std::runtime_error(msg.str());
		}
	}

	BusStopAgent *busStopAgent = BusStopAgent::getBusStopAgentForStop(stop);
	
	if (busStopAgent)
	{
		messaging::MessageBus::SendMessage(busStopAgent, MSG_WAITING_PERSON_ARRIVAL, messaging::MessageBus::MessagePtr(new ArrivalAtStopMessage(this)));
	}
}

void Person_ST::onEvent(event::EventId eventId, event::Context ctxId, event::EventPublisher *sender, const event::EventArgs &args)
{
	Agent::onEvent(eventId, ctxId, sender, args);
//	//Some events only matter if they are for us.
//	if (ctxId == this)
//	{
//		if (eventId == event::EVT_CORE_COMMSIM_ENABLED_FOR_AGENT)
//		{
//			//Was communication simulator enabled for us? If so, start tracking Regions.
//			Print() << "Enabling Region support for agent: " << this << "\n";
//			enableRegionSupport();
//
//			//This requires us to now listen for a new set of events.
//			messaging::MessageBus::SubscribeEvent(event::EVT_CORE_COMMSIM_REROUTING_REQUEST,
//												this, //Only when we are the Agent being requested to re-route..
//												this //Return this event to us (the agent).
//												);
//		}
//		else if (eventId == event::EVT_CORE_COMMSIM_REROUTING_REQUEST)
//		{
//			//Were we requested to re-route?
//			const event::ReRouteEventArgs &rrArgs = MSG_CAST(event::ReRouteEventArgs, args);
//			const std::map<int, RoadRunnerRegion>& regions = ConfigManager::GetInstance().FullConfig().getNetwork().roadRunnerRegions;
//			std::map<int, RoadRunnerRegion>::const_iterator it = regions.find(boost::lexical_cast<int>(rrArgs.getBlacklistRegion()));
//			if (it != regions.end())
//			{
//				std::vector<const RoadSegment*> blacklisted = StreetDirectory::instance().getSegmentsFromRegion(it->second);
//				rerouteWithBlacklist(blacklisted);
//			}
//		}
//	}

	if (currRole)
	{
		currRole->onParentEvent(eventId, ctxId, sender, args);
	}
}

void Person_ST::rerouteWithBlacklist(const std::vector<const Link *> &blacklisted)
{
	//This requires the Role's intervention.
	if (currRole)
	{
		currRole->rerouteWithBlacklist(blacklisted);
	}
}


void Person_ST::HandleMessage(messaging::Message::MessageType type, const messaging::Message &message)
{
	if (currRole)
	{
		currRole->HandleParentMessage(type, message);
	}
}

void Person_ST::handleAMODEvent(event::EventId id, event::Context ctxId, event::EventPublisher *sender, const amod::AMODEventArgs &args)
{
	if (id == event::EVT_AMOD_REROUTING_REQUEST_WITH_PATH)
	{
		//role gets chance to handle event
		if (currRole)
		{
			currRole->onParentEvent(id, ctxId, sender, args);
		}
	}
}

void Person_ST::handleAMODArrival()
{
	//ask the AMODController to handle the arrival
	sim_mob::amod::AMODController::getInstance()->handleVHArrive(this);

}

void Person_ST::handleAMODPickup()
{
	//ask the AMODController to handle the arrival
	sim_mob::amod::AMODController::getInstance()->handleVHPickup(this);
}


SegmentTravelStats& Person_ST::startCurrRdSegTravelStat(const RoadSegment* rdSeg, double entryTime) {
	rsTravelStats.start(rdSeg,entryTime);
	return rsTravelStats;
}

SegmentTravelStats& Person_ST::finalizeCurrRdSegTravelStat(const RoadSegment* rdSeg,
		double exitTime, const std::string travelMode)
{
	if(rdSeg != rsTravelStats.roadSegment)
	{
		std::stringstream msg;
		msg << __func__ << ": Road segment mis-match";
		throw std::runtime_error(msg.str());
	}
	rsTravelStats.finalize(rdSeg,exitTime, travelMode);
	return rsTravelStats;
}

void Person_ST::convertToSmartMobilityTrips()
{
    ConfigParams& cfg = ConfigManager::GetInstanceRW().FullConfig();
    std::string railFLMPathsetStoredProcName = cfg.getDatabaseProcMappings().procedureMappings["rail_sms_pathset"];
	vector<TripChainItem *>::iterator tripChainItemIt;
	for (auto tripChainItemIt = tripChain.begin(); tripChainItemIt != tripChain.end(); ++tripChainItemIt)
	{
		if ((*tripChainItemIt)->itemType == sim_mob::TripChainItem::IT_TRIP)
		{
			TripChainItem *trip = (*tripChainItemIt);

			if (trip->origin.type == WayPoint::NODE && trip->destination.type == WayPoint::NODE)
			{
				std::string originId = boost::lexical_cast<std::string>(trip->origin.node->getNodeId());
				std::string destId = boost::lexical_cast<std::string>(trip->destination.node->getNodeId());
				trip->startLocationId = originId;
				trip->endLocationId = destId;
			}

			auto& subTrips = (dynamic_cast<sim_mob::Trip*>(*tripChainItemIt))->getSubTripsRW();
			std::vector<sim_mob::SubTrip> smartMobilityTrips;

			for (auto itSubTrip = subTrips.begin(); itSubTrip != subTrips.end(); ++itSubTrip)
			{
				if (itSubTrip->origin.type == WayPoint::NODE && itSubTrip->destination.type == WayPoint::NODE)
				{
                    if(itSubTrip->getMode() == "SMS" || itSubTrip->getMode() == "SMS_Pool" ||
                       itSubTrip->getMode() == "AMOD" || itSubTrip->getMode() == "AMOD_Pool")
					{
						addWalkAndWaitLegs(smartMobilityTrips, itSubTrip, (*itSubTrip).destination.node);
						//Ride to destination
						SubTrip subTrip;
						subTrip.setPersonID(-1);
						subTrip.itemType = TripChainItem::getItemType("Trip");
						subTrip.startTime = itSubTrip->startTime;
						subTrip.origin = itSubTrip->origin;
						subTrip.destination = itSubTrip->destination;
						subTrip.startLocationId = boost::lexical_cast<string>(itSubTrip->origin.node->getNodeId());
						subTrip.startLocationType = "NODE";
						subTrip.endLocationId = boost::lexical_cast<string>(
								itSubTrip->destination.node->getNodeId());
						subTrip.endLocationType = "NODE";
                        subTrip.travelMode = itSubTrip->getMode() + "_Taxi";
						smartMobilityTrips.push_back(subTrip);
					}
                    else if(itSubTrip->getMode() == "Rail_SMS" || itSubTrip->getMode() == "Rail_SMS_Pool" ||
                             itSubTrip->getMode() == "Rail_AMOD" || itSubTrip->getMode() == "Rail_AMOD_Pool")
                    {
                        std::vector<OD_Trip> odTrips;
                        std::string personDbId = this->getDatabaseId();
                        unsigned int start_time = ((*tripChainItemIt)->startTime.offsetMS_From(
                                ConfigManager::GetInstance().FullConfig().simStartTime()) / 1000); // start time in seconds

                        bool ret = PT_RouteChoiceLuaProvider::getPTRC_Model().getBestPT_Path(
                                itSubTrip->origin.node->getNodeId(),
                                itSubTrip->destination.node->getNodeId(), itSubTrip->startTime.getValue(), odTrips,
                                personDbId, start_time, railFLMPathsetStoredProcName, PT_Network::TYPE_RAIL_SMS);

                        findMrtTripsAndPerformRailTransitRoute(odTrips);

                        if (ret)
                        {
                            ret = makeODsToTrips(&(*itSubTrip), smartMobilityTrips, odTrips, PT_NetworkCreater::getInstance(PT_Network::TYPE_RAIL_SMS), itSubTrip->getMode());
                            processRAIL_SMSTrips(smartMobilityTrips);
                        }

                        if (!ret)
                        {
                            tripChain.clear();
                            setToBeRemoved();
                            ConfigManager::GetInstanceRW().FullConfig().numPathNotFound++;
                            return;
                        }
                    }
				}
			}
			if (!smartMobilityTrips.empty())
			{
				subTrips = smartMobilityTrips;
			}
		}
	}
}

void Person_ST::convertToTaxiTrips()
{
	for (auto tripChainItemIt = tripChain.begin(); tripChainItemIt != tripChain.end(); ++tripChainItemIt)
	{
		if ((*tripChainItemIt)->itemType == sim_mob::TripChainItem::IT_TRIP)
		{
			TripChainItem* trip = (*tripChainItemIt);
			std::string originId = boost::lexical_cast<std::string>(trip->origin.node->getNodeId());
			std::string destId = boost::lexical_cast<std::string>(trip->destination.node->getNodeId());
			trip->startLocationId = originId;
			trip->endLocationId = destId;

			auto& subTrips = (dynamic_cast<sim_mob::Trip*>(*tripChainItemIt))->getSubTripsRW();
			auto itSubTrip = subTrips.begin();
			std::vector<sim_mob::SubTrip> taxiTrip;
			while (itSubTrip != subTrips.end())
			{
				if (itSubTrip->origin.type == WayPoint::NODE && itSubTrip->destination.type == WayPoint::NODE)
				{
					if (itSubTrip->getMode() == "Taxi")
					{
						double x = itSubTrip->origin.node->getLocation().getX();
						double y = itSubTrip->origin.node->getLocation().getY();
						const TaxiStand *stand = TaxiStand::allTaxiStandMap.searchNearestObject(x, y);

						if (!stand)
						{
							tripChain.clear();
							return;
						}

						//Walk to taxi stand
						SubTrip subTrip;
						subTrip.setPersonID(-1);
						subTrip.itemType = TripChainItem::getItemType("Trip");
						subTrip.startTime = itSubTrip->startTime;
						subTrip.origin = itSubTrip->origin;
						subTrip.destination = WayPoint(stand);
						subTrip.startLocationId = boost::lexical_cast<std::string>(itSubTrip->origin.node->getNodeId());
						subTrip.startLocationType = "NODE";
						subTrip.endLocationId = boost::lexical_cast<std::string>(stand->getStandId());
						subTrip.endLocationType = "TAXI_STAND";
						subTrip.travelMode = "TravelPedestrian";
						subTrip.isTT_Walk = true;
						taxiTrip.push_back(subTrip);

						//Wait for taxi
						subTrip.origin = WayPoint(stand);
						subTrip.destination = itSubTrip->destination;
						subTrip.startLocationId = boost::lexical_cast<std::string>(stand->getStandId());
						subTrip.startLocationType = "TAXI_STAND";
						subTrip.endLocationId = boost::lexical_cast<std::string>(stand->getStandId());
						subTrip.endLocationType = "TAXI_STAND";
						subTrip.travelMode = "WaitingTaxiActivity";
						taxiTrip.push_back(subTrip);

						//Ride taxi to destination
						subTrip.origin = WayPoint(stand);
						subTrip.destination = itSubTrip->destination;
						subTrip.startLocationId = boost::lexical_cast<std::string>(stand->getStandId());
						subTrip.startLocationType = "TAXI_STAND";
						subTrip.endLocationId = boost::lexical_cast<std::string>(
								itSubTrip->destination.node->getNodeId());
						subTrip.endLocationType = "NODE";
						subTrip.travelMode = "TaxiTravel";
						taxiTrip.push_back(subTrip);
					}
				}
				++itSubTrip;
			}

			if (!taxiTrip.empty())
			{
				subTrips = taxiTrip;
			}
		}
	}
}
void Person_ST::addWalkAndWaitLegs(vector<SubTrip> &subTrips, const vector<SubTrip>::iterator &itSubTrip,const Node *destination) const
{
	//'Walk' from current node to current node
	SubTrip subTrip;
	subTrip.setPersonID(-1);
	subTrip.itemType = TripChainItem::getItemType("Trip");
	subTrip.startTime = itSubTrip->startTime;
	subTrip.origin = itSubTrip->origin;
	subTrip.destination = itSubTrip->origin;
	subTrip.startLocationId = boost::lexical_cast<string>(itSubTrip->origin.node->getNodeId());
	subTrip.startLocationType = "NODE";
	subTrip.endLocationId = boost::lexical_cast<string>(itSubTrip->origin.node->getNodeId());
	subTrip.endLocationType = "NODE";
	subTrip.travelMode = "TravelPedestrian";
	subTrip.isTT_Walk = true;
	subTrips.push_back(subTrip);

	//Wait for pick-up
	subTrip.origin = itSubTrip->origin;
	subTrip.destination = WayPoint(destination);
	subTrip.startLocationId = boost::lexical_cast<string>(itSubTrip->origin.node->getNodeId());
	subTrip.startLocationType = "NODE";
	subTrip.endLocationId = boost::lexical_cast<string>(destination->getNodeId());
	subTrip.endLocationType = "NODE";
	subTrip.travelMode = "WaitingTaxiActivity";
	subTrips.push_back(subTrip);
}
void Person_ST::findMrtTripsAndPerformRailTransitRoute(std::vector<sim_mob::OD_Trip>& matchedTrips)
{
    std::vector<sim_mob::OD_Trip>::iterator itr = matchedTrips.begin();
    std::vector<sim_mob::OD_Trip> newODTrips;

    while (itr != matchedTrips.end())
    {
        if ((*itr).tType == TRAIN_EDGE)
        {
            std::string src = (*itr).startStop;
            std::string end = (*itr).endStop;
            size_t pos = 0;

            if ((pos = (src.find("/"))) != std::string::npos)
            {
                src = src.substr(0, pos);
            }

            if ((pos = (end.find("/"))) != std::string::npos)
            {
                end = end.substr(0, pos);
            }

            std::vector<std::string> railPath = RailTransit::getInstance().fetchBoardAlightStopSeq(src, end);
            if (railPath.empty())
            {
                return;
            }

            std::vector<sim_mob::OD_Trip> odTrips = splitMrtTrips(railPath);
            newODTrips.insert(newODTrips.end(), odTrips.begin(), odTrips.end());
        }
        else
        {
            newODTrips.push_back(*itr);
        }
        itr++;
    }

    matchedTrips = newODTrips;
}
std::vector<sim_mob::OD_Trip>  Person_ST::splitMrtTrips(std::vector<std::string> railPath)
{
    vector<std::string>::iterator it=railPath.begin();
    std::string first = (*it);
    std::string second =* (it+1);
    sim_mob::TrainStop* firststop = sim_mob::PT_NetworkCreater::getInstance().MRTStopsMap[first];
    sim_mob::TrainStop* nextstop = sim_mob::PT_NetworkCreater::getInstance().MRTStopsMap[second];
    if((boost::iequals(firststop->getStopName(),nextstop->getStopName())))
    {
        railPath.erase(it);
        it = railPath.begin();
    }
    vector<std::string>::iterator itrEnd=railPath.end();
    std::string last = *(itrEnd-1);
    std::string seclast = *(itrEnd-2);
    firststop = sim_mob::PT_NetworkCreater::getInstance().MRTStopsMap[seclast];
    nextstop = sim_mob::PT_NetworkCreater::getInstance().MRTStopsMap[last];
    if((boost::iequals(firststop->getStopName(),nextstop->getStopName())))
    {
        railPath.erase(itrEnd-1);
    }

    std::vector<sim_mob::OD_Trip> odTrips;
    std::string src = (*it);
    std::string end = "";
    std::string prev = "";
    while(it != railPath.end())
    {
        /** hardcoding the line now since all lines are not implemented */
        if((*it).find("NE") != std::string::npos || (*it).find("EW") !=  std::string::npos||(*it).find("CG") !=  std::string::npos)
        {
            if( !boost::iequals(prev, "") )
            {
                end=prev;
                //make subtrip of start and end
                odTrips.push_back(CreateMRTSubTrips(src,end));
            }
            src = (*it);
            end = *(++it);
            //make subtrip;
            odTrips.push_back(CreateMRTSubTrips(src,end));
            ++it;
            if(it == railPath.end())
            {
                break;
            }
            src = (*it);
            prev = "";
            continue;
        }

        if(it+1 == railPath.end())
        {
            end = (*it);
            //make subtrip
            odTrips.push_back( CreateMRTSubTrips(src,end) );
        }
        prev = (*it);
        it++;

    }
    return odTrips;
}

sim_mob::OD_Trip Person_ST::CreateMRTSubTrips(std::string src,std::string dest)
{
    sim_mob::OD_Trip odTrip;
    sim_mob::TrainStop* destStop = sim_mob::PT_NetworkCreater::getInstance().findMRT_Stop(dest);
    WayPoint wayPointDestStop;//=WayPoint(destStop);
    WayPoint wayPointSrcStop;
    if (destStop)
    {
        wayPointDestStop = WayPoint(destStop);
    }

    sim_mob::TrainStop* srcStop = sim_mob::PT_NetworkCreater::getInstance().findMRT_Stop(src);

    if (srcStop)
    {
        wayPointSrcStop = WayPoint(srcStop);
    }

    sim_mob::SubTrip subTrip;
    odTrip.startStop=srcStop->getStopName();;
    odTrip.endStop=destStop->getStopName();
    odTrip.sType=2;
    odTrip.eType=2;
    odTrip.tType=TRAIN_EDGE;
    std::map<std::string ,std::map<std::string ,std::vector<PT_NetworkEdge>>> mrtEdgeMap=PT_NetworkCreater::getInstance().MRTStopdgesMap;
    std::vector<PT_NetworkEdge> edgeVector=mrtEdgeMap[odTrip.startStop][odTrip.endStop];
    typename std::vector<PT_NetworkEdge>::iterator iter=edgeVector.begin();
    std::string selServiceLine="";

    if(iter!=edgeVector.end())
    {
        PT_NetworkEdge selEdge=*iter;
        if(edgeVector.size()>1)
        {
            while(iter!=edgeVector.end())
            {
                std::string serviceLine=iter->getServiceLine();
                if(boost::iequals(src.substr(0,2),serviceLine.substr(0,2)))
                {
                    selServiceLine=serviceLine;
                    selEdge=*iter;
                }
                iter++;
            }
        }

        odTrip.id=selEdge.getEdgeId();
        odTrip.serviceLine=selEdge.getServiceLine();
        odTrip.serviceLines=selEdge.getServiceLines();
        odTrip.travelTime = selEdge.getTransitTimeSecs();
        odTrip.walkTime = selEdge.getWalkTimeSecs();
    }

    return odTrip;

}
void Person_ST::processRAIL_SMSTrips(std::vector<SubTrip> &subTrips)
{
    std::vector<SubTrip> modifiedSubTrips;

    for(auto itSubTrip = subTrips.begin(); itSubTrip != subTrips.end(); ++itSubTrip)
    {
        if (((*itSubTrip).travelMode == "Rail_SMS" || (*itSubTrip).travelMode == "Rail_SMS_Pool" ||
             (*itSubTrip).travelMode == "Rail_AMOD" || (*itSubTrip).travelMode == "Rail_AMOD_Pool") &&
            (*itSubTrip).originType == TripChainItem::LT_NODE)
        {
            //Ride to node near the train station
            const Node *nodeNearStation =
                    itSubTrip->destination.trainStop->getRandomStationSegment()->getParentLink()->getToNode();

            double distanceToNode =dist((*itSubTrip).origin.node->getLocation(), nodeNearStation->getLocation());
            SubTrip subTrip;

            //Walk to the train station if it is with-in a radius of 1km
            if(distanceToNode <= 1000)
            {
                subTrip.setPersonID(-1);
                subTrip.itemType = TripChainItem::getItemType("Trip");
                subTrip.startTime = itSubTrip->startTime;
                subTrip.origin = itSubTrip->origin;
                subTrip.destination = WayPoint(nodeNearStation);
                subTrip.startLocationId = boost::lexical_cast<string>(itSubTrip->origin.node->getNodeId());
                subTrip.startLocationType = "NODE";
                subTrip.endLocationId = boost::lexical_cast<string>(nodeNearStation->getNodeId());
                subTrip.endLocationType = "NODE";
                subTrip.travelMode = "Walk";
                subTrip.isPT_Walk = false;
                modifiedSubTrips.push_back(subTrip);

                Warn() << "Replaced " << (*itSubTrip).travelMode << " trip from node " << (*itSubTrip).origin.node->getNodeId()
                       << " to node near station " << itSubTrip->destination.trainStop->getStopName()
                       << " with Walk trip\n";
            }
            else
            {
                //Travel mode is RAIL_SMS for access, so split this sub-trip (node-train station) into following sub-trips
                //walk (node-node), wait(node-node), passenger(node-node), walk(node-train stn)
                addWalkAndWaitLegs(modifiedSubTrips, itSubTrip, nodeNearStation);

                subTrip.setPersonID(-1);
                subTrip.itemType = TripChainItem::getItemType("Trip");
                subTrip.startTime = itSubTrip->startTime;
                subTrip.origin = itSubTrip->origin;
                subTrip.destination = WayPoint(nodeNearStation);
                subTrip.startLocationId = boost::lexical_cast<string>(itSubTrip->origin.node->getNodeId());
                subTrip.startLocationType = "NODE";
                subTrip.endLocationId = boost::lexical_cast<string>(nodeNearStation->getNodeId());
                subTrip.endLocationType = "NODE";
                subTrip.travelMode = (*itSubTrip).travelMode + "_Taxi";
                modifiedSubTrips.push_back(subTrip);
            }

            //Walk to train station
            subTrip.origin = subTrip.destination;
            subTrip.originType = TripChainItem::LT_NODE;
            subTrip.destination = itSubTrip->destination;
            subTrip.destinationType = TripChainItem::LT_PUBLIC_TRANSIT_STOP;
            subTrip.startLocationId = boost::lexical_cast<string>(nodeNearStation->getNodeId());
            subTrip.startLocationType = "NODE";
            subTrip.endLocationId = itSubTrip->destination.trainStop->getStopName();
            subTrip.endLocationType = "MS";
            subTrip.travelMode = "Walk";
            subTrip.isPT_Walk = true;
            subTrip.walkTime = 60;
            modifiedSubTrips.push_back(subTrip);
        }
        else if(((*itSubTrip).travelMode == "Rail_SMS" || (*itSubTrip).travelMode == "Rail_SMS_Pool" ||
                 (*itSubTrip).travelMode == "Rail_AMOD" || (*itSubTrip).travelMode == "Rail_AMOD_Pool") &&
                (*itSubTrip).originType == TripChainItem::LT_PUBLIC_TRANSIT_STOP)
        {
            //Travel mode is RAIL_SMS for egress, so split this sub-trip (train station-node) into following sub-trips
            //walk (train station-node), walk(node-node), wait(node-node), passenger(node-node)

            //Walk to node near the train station
            const Node *nodeNearStation =
                    itSubTrip->origin.trainStop->getRandomStationSegment()->getParentLink()->getToNode();

            SubTrip subTrip;
            subTrip.setPersonID(-1);
            subTrip.itemType = TripChainItem::getItemType("Trip");
            subTrip.startTime = itSubTrip->startTime;
            subTrip.origin = itSubTrip->origin;
            subTrip.originType = TripChainItem::LT_PUBLIC_TRANSIT_STOP;
            subTrip.destination = WayPoint(nodeNearStation);
            subTrip.destinationType = TripChainItem::LT_NODE;
            subTrip.startLocationId = subTrip.origin.trainStop->getStopName();
            subTrip.startLocationType = "MS";
            subTrip.endLocationId = boost::lexical_cast<string>(nodeNearStation->getNodeId());
            subTrip.endLocationType = "NODE";
            subTrip.travelMode = "Walk";
            subTrip.isPT_Walk = true;
            subTrip.walkTime = 60;
            modifiedSubTrips.push_back(subTrip);

            if((*itSubTrip).destination.node->getNodeId() != nodeNearStation->getNodeId())
            {
                double distanceToNode = dist((*itSubTrip).destination.node->getLocation(), nodeNearStation->getLocation());

                if (distanceToNode > 1000)
                {
                    subTrip.origin = WayPoint(nodeNearStation);
                    subTrip.originType = TripChainItem::LT_NODE;
                    subTrip.destination = subTrip.origin;
                    subTrip.destinationType = TripChainItem::LT_NODE;
                    subTrip.startLocationId = boost::lexical_cast<string>(subTrip.origin.node->getNodeId());
                    subTrip.startLocationType = "NODE";
                    subTrip.endLocationId = boost::lexical_cast<string>(subTrip.destination.node->getNodeId());
                    subTrip.endLocationType = "NODE";
                    subTrip.travelMode = "TravelPedestrian";
                    subTrip.isTT_Walk = true;
                    modifiedSubTrips.push_back(subTrip);

                    //Wait for pick-up
                    subTrip.destination = itSubTrip->destination;
                    subTrip.endLocationId = boost::lexical_cast<string>(itSubTrip->destination.node->getNodeId());
                    subTrip.travelMode = "WaitingTaxiActivity";
                    modifiedSubTrips.push_back(subTrip);

                    //Ride to destination
                    subTrip.travelMode = (*itSubTrip).travelMode + "_Taxi";
                    modifiedSubTrips.push_back(subTrip);
                }
                else
                {
                    subTrip.origin = WayPoint(nodeNearStation);
                    subTrip.originType = TripChainItem::LT_NODE;
                    subTrip.destination = itSubTrip->destination;
                    subTrip.destinationType = TripChainItem::LT_NODE;
                    subTrip.startLocationId = boost::lexical_cast<string>(subTrip.origin.node->getNodeId());
                    subTrip.startLocationType = "NODE";
                    subTrip.endLocationId = boost::lexical_cast<string>(subTrip.destination.node->getNodeId());
                    subTrip.endLocationType = "NODE";
                    subTrip.travelMode = "Walk";
                    subTrip.isTT_Walk = false;
                    modifiedSubTrips.push_back(subTrip);

                    Warn() << "Replaced " << (*itSubTrip).travelMode << " trip from node near station "
                           << itSubTrip->origin.trainStop->getStopName() << " to node " << subTrip.destination.node->getNodeId()
                           << " with Walk trip\n";
                }
            }
        }
        else
        {
            modifiedSubTrips.push_back(*itSubTrip);
        }
    }

    if(!modifiedSubTrips.empty())
    {
        subTrips = modifiedSubTrips;
    }
}