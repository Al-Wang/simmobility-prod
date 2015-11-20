//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <boost/foreach.hpp>
#include <map>
#include <string>
#include <vector>

#include "conf/settings/DisableMPI.h"
#include "Agent.hpp"
#include "entities/misc/TripChain.hpp"
#include "entities/TravelTimeManager.hpp"
#include "entities/vehicle/VehicleBase.hpp"
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "util/LangHelpers.hpp"
#include "util/Profiler.hpp"
#include "workers/Worker.hpp"

namespace sim_mob
{

template <class PERSON> class Role;
class TripChainItem;
class SubTrip;
class PartitionManager;
class PackageUtils;
class UnPackageUtils;
class UpdateParams;
class OD_Trip;

/**
 * Basic Person class.
 *
 * \author Seth N. Hetu
 * \author Wang Xinyuan
 * \author Luo Linbo
 * \author Li Zhemin
 * \author Xu Yan
 * \author Harish Loganathan
 * \author zhang huai peng
 * \author Yao Jin
 * \author Vahid Saber
 * \author Neeraj Deshmukh
 *
 * A person may perform one of several roles which
 * change over time. For example: Drivers, Pedestrians, and Passengers are
 * all roles which a Person may fulfil.
 */
class Person : public sim_mob::Agent
{
private:
	/**The database id*/
	std::string databaseID;

	/**The age of the person*/
	unsigned int age;

	/**Indicates that Role's updateParams has to be reset*/
	bool resetParamsRequired;

	/**Indicates the source of creation of the person*/
	std::string agentSrc;

	/**Indicates if the detailed path for the current sub-trip is already planned*/
	bool nextPathPlanned;
	
	/**
	 * Ask this person to re-route to the destination with the given set of blacklisted links
	 * If the Agent cannot complete this new route, it will fall back onto the old route.
	 *
	 * @param blacklisted the black-listed links
	 */
	virtual void rerouteWithBlacklist(const std::vector<const sim_mob::Link *>& blacklisted);

protected:
	/**Holds the person's trip-chain*/
	std::vector<TripChainItem *> tripChain;

	/**Marks the first call to update function*/
	bool isFirstTick;

	/**
	 * Called during the first call to update() for the person
	 *
	 * @param now The timeslice representing the time frame for which this method is called
	 *
	 * @return false to indicate failure; The person will be removed from the simulation with no
	 * further processing.
	 */
	virtual bool frame_init(timeslice now);

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
	virtual void onEvent(event::EventId eventId, sim_mob::event::Context ctxId, event::EventPublisher *sender, const event::EventArgs &args);

	/**Inherited from MessageHandler.*/
	virtual void HandleMessage(messaging::Message::MessageType type, const messaging::Message &message);

	/**
	 * Advances the current sub-trip to the next item.
     *
	 * @return true, if the sub-trip is advanced
     */
	bool advanceCurrentSubTrip();

	/**
	 * Sets the current sub-trip to the first sub-trip of the current trip chain
	 *
     * @return an iterator pointing to the first sub-trip of the current trip chain \
     */
	std::vector<sim_mob::SubTrip>::iterator resetCurrSubTrip();

public:
	/**The agent's start node*/
	WayPoint originNode;

	/**The agent's end node*/
	WayPoint destNode;

	/**Holds the link travel time*/
	LinkTravelStats currLinkTravelStats;

	/**The current item in trip chain*/
	std::vector<TripChainItem *>::iterator currTripChainItem;

	/**The current sub-trip in the current trip (if the current item is a trip)*/
	std::vector<SubTrip>::iterator currSubTrip;

	/**The next item in trip chain*/
	std::vector<TripChainItem *>::iterator nextTripChainItem;

	/**The next sub-trip in the current trip (if the current item is a trip)*/
	std::vector<SubTrip>::const_iterator nextSubTrip;

	/**
	 * The bus line taken by the person
	 * tmp addition for debugging ~ Harish
	 */
	std::string busLine;

	/**Stores all the metrics of the sub-trips
	 * The implementation inserts information at sub-trip resolution while pre-day will require Trip-level metrics.
	 * So whenever all sub-trips of a trip are done (subTripTravelMetrics) and it is time to change the trip chain item
	 * (in Person class) an aggregate function will create a new entry in tripTravelMetrics from subTripTravelMetrics
	 * items. subTripTravelMetrics items are cleared then.
	 */
	std::vector<TravelMetric> tripTravelMetrics;

	/**Sub-trip level travel metrics*/
	std::vector<TravelMetric> subTripTravelMetrics;	

	/**The "src" variable is used to help flag how this person was created.*/
	explicit Person(const std::string &src, const MutexStrategy &mtxStrat, int id = -1, std::string databaseID = "");
	explicit Person(const std::string &src, const MutexStrategy &mtxStrat, const std::vector<sim_mob::TripChainItem *> &tc);
	virtual ~Person();

	/**Sets the person's characteristics by some distribution*/
	virtual void setPersonCharacteristics();

	/**
	 * Initialises the trip chain
     */
	virtual void initTripChain();

	/**
	 * Load a Person's properties (specified in the configuration file), creating a placeholder trip chain if
	 * requested.
     *
	 * @param configProps The properties specified in the configuration file
     */
	virtual void load(const std::map<std::string, std::string> &configProps);

	/**
	 * Builds a subscriptions list to be added to the managed data of the parent worker
	 * 
	 * @return the list of Buffered<> types this entity subscribes to
	 */
	virtual std::vector<BufferedBase *> buildSubscriptionList() = 0;

	/**
	 * Updates the person's current role to the given role.
	 *
     * @param newRole Indicates the new role of the person. If the new role is not provided,
	 * a new role is created based on the current sub-trip
	 *
     * @return true, if role is updated successfully
     */
	virtual bool updatePersonRole() = 0;

	/**
	 * Sets the simulation start time of the entity
	 *
     * @param value The simulation start time to be set
     */
	virtual void setStartTime(unsigned int value) = 0;
	
	/**
	 * Creates sub-trips for each leg of Public transit route choice made by the person
     *
	 * @param curSubTrip
     * @param newSubTrips
     * @param matchedTrips
     * 
	 * @return true, if successful
     */
	bool makeODsToTrips(SubTrip *curSubTrip, std::vector<SubTrip> &newSubTrips, const std::vector<OD_Trip> &matchedTrips);

	/**
	 * Updates the next trip chain, used only for NextRole
	 *
     * @return true if the next trip chain item is updated, else false
     */
	bool updateNextTripChainItem();

	/**
	 * Updates the next sub trip, used only for NextRole
     *
	 * @return true if the next sub trip is updated, else false
     */
	bool updateNextSubTrip();

	/**
	 * Check if any role changing is required.

	 * @return
     */
	virtual Entity::UpdateStatus checkTripChain() = 0;

	/**
	 * Update origin and destination node based on the trip, sub-trip or activity given
     *
	 * @param tc
     * @param subtrip
     *
	 * @return
     */
	bool updateOD(sim_mob::TripChainItem *tc, const sim_mob::SubTrip *subtrip = 0);

	/**
	 * A version of serializer for sub-trip level travel time.
	 *
	 * @param subtripMetrics input metrics
	 * @param currTripChainItem current TripChainItem
	 * @param currSubTrip current SubTrip for which subtripMetrics is collected
	 */
	void serializeSubTripChainItemTravelTimeMetrics(const TravelMetric &subtripMetrics, std::vector<TripChainItem *>::iterator currTripChainItem,
													std::vector<SubTrip>::iterator currSubTrip) const;

	/**
	 * Get the measurements stored in subTripTravelMetrics and add them up into a new entry in tripTravelMetrics.
	 * This method is to be called whenever a sub-trip is done.
	 * NOTE: Currently Unused
	 */
	void aggregateSubTripMetrics();

	/**
	 * Add the given TravelMetric to subTripTravelMetrics container
	 * NOTE: Currently Unused
	 */
	void addSubtripTravelMetrics(TravelMetric &value);

	/**
	 * Serializer for Trip level travel time
	 * NOTE: Currently Unused
	 */
	void serializeTripTravelTimeMetrics();

	/**
	 * This is called by the destructor of the movement facet of a non-activity role
	 * NOTE: Currently Unused
	 */
	void serializeCBD_SubTrip(const TravelMetric &metric);

	/**
	 * This is called by the destructor of the movement facet of an activity role
	 * NOTE: Currently Unused
	 */
	void serializeCBD_Activity(const TravelMetric &metric);

	/**
	 * Replaces the previous trip chain with the provided trip chain. Also initialises the new trip chain.
     * 
	 * @param tripChain the new trip chain
     */
	void setTripChain(const std::vector<TripChainItem *> &tripChain);

	/**
	 * Indicates whether an entity is non-spatial in nature
	 *
     * @return false, as Person objects are spatial in nature
     */
	virtual bool isNonspatial()
	{
		return false;
	}

	/**Clears the flag indicating that the agent is marked for removal*/
	void clearToBeRemoved()
	{
		toRemoved = false;
	}

	const std::vector<TripChainItem *>& getTripChain() const
	{
		return tripChain;
	}

	const std::string& getAgentSrc() const
	{
		return agentSrc;
	}

	const std::string& getDatabaseId() const
	{
		return databaseID;
	}

	void setDatabaseId(const std::string &databaseId)
	{
		databaseID = databaseId;
	}

	bool isResetParamsRequired() const
	{
		return resetParamsRequired;
	}

	void setResetParamsRequired(bool resetParamsRequired)
	{
		this->resetParamsRequired = resetParamsRequired;
	}

	boost::mt19937& getGenerator()
	{
		return currWorkerProvider->getGenerator();
	}

	void setNextPathPlanned(bool value)
	{
		nextPathPlanned = value;
	}

	bool getNextPathPlanned()
	{
		return nextPathPlanned;
	}

#ifndef SIMMOB_DISABLE_MPI

	virtual void pack(PackageUtils& packageUtil) CHECK_MPI_THROW;
	virtual void unpack(UnPackageUtils& unpackageUtil) CHECK_MPI_THROW;

	virtual void packProxy(PackageUtils& packageUtil) CHECK_MPI_THROW;
	virtual void unpackProxy(UnPackageUtils& unpackageUtil) CHECK_MPI_THROW;

	friend class PartitionManager;
	friend class BoundaryProcessor;

#endif
};

}
