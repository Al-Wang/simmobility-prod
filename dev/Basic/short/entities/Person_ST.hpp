//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once
#include <map>
#include <string>
#include <vector>
#include "entities/Person.hpp"
#include "entities/roles/Role.hpp"
#include "entities/amodController/AMODEvent.hpp"
#include "entities/TravelTimeManager.hpp"
#include "event/args/EventArgs.hpp"
#include "event/EventListener.hpp"
#include "event/EventPublisher.hpp"
#include "entities/FleetController.hpp"

namespace sim_mob
{

class AMODController;

class Person_ST : public Person
{
private:
	/**Time taken by the person to board a bus*/
	double boardingTimeSecs;

	/**Time taken by the person to alight from a bus*/
	double alightingTimeSecs;
	
	/**The walking speed of the person (in m/s)*/
	double walkingSpeed;

	/**The previous role that was played by the person.*/
	Role<Person_ST>* prevRole;

	/**The current role being played by the person*/
	Role<Person_ST>* currRole;

	/**The next role that the person will play. However, this variable is only temporary and will not be used to update the currRole*/
	Role<Person_ST>* nextRole;

	/**Indicates whether we have registered to receive communication simulator related messages*/
	bool commEventRegistered;

	/**Road Segment Travel time collector*/
	SegmentTravelStats rsTravelStats;

	/**Stores the configuration properties of the agent loaded from the XML configuration file*/
	std::map<std::string, std::string> configProperties;

    /**Stores the service vehicle information*/
    FleetController::FleetItem serviceVehicle;
	/**
	 * Converts the trips that have travel mode as bus travel into a trip chain than contains the detailed public transit trip
	 */
	void convertPublicTransitODsToTrips();

    /**Alters trip chain in accordance to route choice for taxi trip*/
    void convertToTaxiTrips();
	/**
	 * Inserts a waiting activity before bus travel
	 */
	void insertWaitingActivityToTrip();

	/**
	 * Assigns id to sub-trips
	 */
	void assignSubtripIds();

	/**
	 * Advances the current trip chain item to the next item if all the sub-trips in the trip have been completed.
	 * If not, calls the advanceCurrentTripChainItem() method
     *
	 * @return true, if the trip chain item is advanced
     */
	bool advanceCurrentTripChainItem();

    /**Adds the walk and wait legs for the travel by smart mobility*/
    void addWalkAndWaitLegs(std::vector<SubTrip> &subTrips, const std::vector<SubTrip>::iterator &itSubTrip,
                            const Node *destination) const;
	/**
	 * Assigns a person waiting at a bus stop to the bus stop agent
	 */
	void assignPersonToBusStopAgent();

	/**
	 * Enable Region support
	 * See RegionAndPathTracker for more information.
	 */
	void enableRegionSupport()
	{
		regionAndPathTracker.enable();
	}

    void convertToSmartMobilityTrips();


protected:
	/**
	 * Called during the first call to update() for the person
	 *
	 * @param now The timeslice representing the time frame for which this method is called
	 *
	 * @return UpdateStatus with the value of 'status' set to 'RS_CONTINUE', 'RS_CONTINUE_INCOMPLETE' or 'RS_DONE' to indicate the 
	 * result of the execution.
	 */
	virtual Entity::UpdateStatus frame_init(timeslice now);

	/**
	 * Called during every call to update() for a given person. This is called after frame_tick()
	 * for the first call to update().
	 *
	 * @param now The timeslice representing the time frame for which this method is called
	 *
	 * @return an UpdateStatus indicating future action
	 */
	virtual Entity::UpdateStatus frame_tick(timeslice now);

	/**
	 * Called after frame_tick() for every call to update() for a given agent.
	 * Use this method to display output for this time tick.
	 *
	 * @param now The timeslice representing the time frame for which this method is called
	 */
	virtual void frame_output(timeslice now);
	
	/**Inherited from EventListener*/
	virtual void onEvent(event::EventId eventId, event::Context ctxId, event::EventPublisher *sender, const event::EventArgs &args);

	/**Inherited from MessageHandler.*/
	virtual void HandleMessage(messaging::Message::MessageType type, const messaging::Message &message);


public:
	/**The lane in which the person starts*/
	int startLaneIndex;

	/**The segment in which the person starts*/
	int startSegmentId;

	/**The offset from the segment at which the person starts*/
	int segmentStartOffset;

	/**The speed at which the person starts (m/s)*/
    int initialSpeed;
	
	/**FMOD client id*/
	int client_id;

	/**Id of the autonomous vehicle. If it is a normal vehicle, this id is -1*/
	std::string amodId;

	/**The path selected by the autonomous vehicle*/
	std::vector<WayPoint> amodPath;

	/**The segment at which an autonomous vehicle has been requested for a pick-up*/
	std::string amodPickUpSegmentStr;

	/**The segment at which an autonomous vehicle is to drop-off the passenger*/
	std::string amodDropOffSegmentStr;

	std::string parkingNode;

	/**The trip id of the AMOD system*/
	std::string amodTripId;

	double amodSegmLength;

	double amodSegmLength2;

	amod::AMODEventPublisher eventPub;

	bool isPositionValid;

    bool isVehicleInLoadingQueue; // to test stuck vehicles

	Person_ST(const std::string &src, const MutexStrategy &mtxStrat, int id = -1, std::string databaseID = "");
	Person_ST(const std::string &src, const MutexStrategy &mtxStrat, const std::vector<sim_mob::TripChainItem *> &tc);
	virtual ~Person_ST();

	/**Sets the person's characteristics by some distribution*/
	virtual void setPersonCharacteristics();

	/**
	 * Sets the simulation start time of the entity
	 *
     * @param value The simulation start time to be set
     */
	virtual void setStartTime(unsigned int value);

	/**
	 * Load a Person's properties (specified in the configuration file), creating a placeholder trip chain if
	 * requested.
     *
	 * @param configProps The properties specified in the configuration file
     */
	virtual void load(const std::map<std::string, std::string> &configProps);

	/**
	 * Initialises the trip chain
     */
	void initTripChain();

	/**
	 * Check if any role changing is required.

	 * @return
     */
	Entity::UpdateStatus checkTripChain(unsigned int currentTime=0);

	/**
	 * Finds the person's next role based on the person's trip-chain
	 *
     * @return true, if the next role is successfully found
     */
	bool findPersonNextRole();

	/**
	 * Updates the person's current role to the given role.
	 *
     * @param newRole Indicates the new role of the person. If the new role is not provided,
	 * a new role is created based on the current sub-trip
	 *
     * @return true, if role is updated successfully
     */
	bool updatePersonRole();

	/**
	 * Builds a subscriptions list to be added to the managed data of the parent worker
	 *
	 * @return the list of Buffered<> types this entity subscribes to
	 */
	virtual std::vector<BufferedBase *> buildSubscriptionList();

	/**
	 * Change the role of this person
	 *
     * @param newRole the new role to be assigned to the person
     */
	void changeRole();

	/**
	 * Ask this person to re-route to the destination with the given set of blacklisted RoadSegments
	 * If the Agent cannot complete this new route, it will fall back onto the old route.
	 *
	 * @param blacklisted the black-listed road segments
	 */
	virtual void rerouteWithBlacklist(const std::vector<const Link *> &blacklisted);

	void handleAMODArrival();
	
	void handleAMODPickup();

	void setPath(std::vector<WayPoint> &path);
	
	void handleAMODEvent(event::EventId id, event::Context ctxId, event::EventPublisher *sender, const amod::AMODEventArgs& args);

    timeslice currFrame;

	double getWalkingSpeed() const
	{
		return walkingSpeed;
	}
	
	double getBoardingCharacteristics() const
	{
		return boardingTimeSecs;
	}

	double getAlightingCharacteristics() const
	{
		return alightingTimeSecs;
	}

	Role<Person_ST>* getRole() const
	{
		return currRole;
	}

	Role<Person_ST>* getPrevRole() const
	{
		return prevRole;
	}

	void setNextRole(Role<Person_ST> *newRole)
	{
		nextRole = newRole;
	}

	Role<Person_ST>* getNextRole() const
	{
		return nextRole;
	}

	/** Clears the map configProperties which contains the configuration properties */
	void clearConfigProperties()
	{
		this->configProperties.clear();
	}

	void setConfigProperties(const std::map<std::string, std::string> &props)
	{
		this->configProperties = props;
	}

	const std::map<std::string, std::string>& getConfigProperties()
	{
		return this->configProperties;
	}

	SegmentTravelStats& getCurrRdSegTravelStats()
	{
		return this->rsTravelStats;
	}

    void setServiceVehicle(const FleetController::FleetItem &svcVehicle)
    {
        serviceVehicle = svcVehicle;
    }

    const FleetController::FleetItem& getServiceVehicle() const
    {
        return serviceVehicle;
    }
    const MobilityServiceDriver* exportServiceDriver() const
    {
        if(currRole)
        {
            return currRole->exportServiceDriver();
        }
        return nullptr;
    }

	SegmentTravelStats& startCurrRdSegTravelStat(const RoadSegment* rdSeg, double entryTime);

	SegmentTravelStats& finalizeCurrRdSegTravelStat(const RoadSegment* rdSeg,double exitTime,
			const std::string travelMode);
};
}
