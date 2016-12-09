//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <set>
#include <map>
#include <string>
#include <vector>

#include "conf/RawConfigParams.hpp"
#include "entities/roles/RoleFactory.hpp"
#include "entities/misc/PublicTransit.hpp"
#include "geospatial/network/PT_Stop.hpp"
#include "util/DailyTime.hpp"
#include "util/Factory.hpp"


namespace sim_mob {

//Forward declarations
class BusStop;
class CommunicationDataManager;
class ControlManager;
class ConfigManager;
class RoadSegment;
class RoadNetwork;
class PassengerDist;
class ProfileBuilder;
class TripChainItem;
class Broker;



/**
 * Contains our ConfigParams class. Replaces simpleconf.hpp, providing a more logical ordering (e.g., the file
 *    is parsed FIRST) and some other benefits (fewer include dependencies).
 *
 * To load this class, you can use something like this:
 *
 *   \code
 *   ParseConfigFile parse(configPath, ConfigParams::GetInstanceRW());
 *   ExpandAndValidateConfigFile expand(ConfigParams::GetInstanceRW(), active_agents, pending_agents);
 *   \endcode
 *
 * Note that the separate "Parse" and "Expand" steps exist to allow config-file parameters (like ns3/android brokerage)
 *  to be loaded without requiring anything to be constructed (since this might rely on parsed parameters).
 *
 * \note
 * Developers should make sure NOT to include "GenConfig.h" here; otherwise, this forces most of the source
 * tree to be rebuilt whenever a single parameter changes. (In other words, don't implement any functions in
 * this header file, except perhaps constructors.)
 *
 * \author Seth N. Hetu
 * \author LIM Fung Chai
 * \author Luo Linbo
 * \author Wang Xinyuan
 * \author Runmin Xu
 * \author Zhang Shuai
 * \author Li Zhemin
 * \author Matthew Bremer Bruchon
 * \author Xu Yan
 */
class ConfigParams : public RawConfigParams {
public:
	friend class ConfigManager;

	~ConfigParams();

    /**
     * If any Agents specify manual IDs, we must ensure that:
     * the ID is < startingAutoAgentID
     * all manual IDs are unique.
     * We do this using the Agent constraints struct
     */
	struct AgentConstraints {
		AgentConstraints() : startingAutoAgentID(0) {}

		int startingAutoAgentID;
		std::set<unsigned int> manualAgentIDs;
	};

    /**
     * Retrieves the Broker Factory
     *
     * @return broker factory
     */
    sim_mob::Factory<sim_mob::Broker>& getBrokerFactoryRW();

    /// Number of ticks to run the simulation for. (Includes "warmup" ticks.)
    unsigned int totalRuntimeTicks;

    /// Number of ticks considered "warmup".
    unsigned int totalWarmupTicks;

    /// Output network file name
	std::string outNetworkFileName;

	/// Output simulation information file name
	std::string outSimInfoFileName;

    /// Is the simulation run using MPI?
	bool using_MPI;

    /// Is the simulation repeatable?
	bool is_simulation_repeatable;

    /// Number of agents skipped in loading
    unsigned int numAgentsSkipped;

public:
    /**
     * Retrieves/Builds the database connection string
     *
     * @param maskPassword is the password masked/encoded (default is true)
     *
     * @return database connection string
     */
    std::string getDatabaseConnectionString(bool maskPassword=true) const;

    /**
     * Retrieves the stored procedure mappings
     *
     * @return stored procedure map
     */
    StoredProcedureMap getDatabaseProcMappings() const;

	/**
	 * Seal the network. After this, no more editing of the network can take place.
	 */
	void sealNetwork();

    /**
     * Retrieves a reference to the current communication data manager
     *
     * @return Reference to the current communication data manager
     */
    //sim_mob::CommunicationDataManager&  getCommDataMgr() const ;

    /**
     * Retrieves a reference to the current control manager
     *
     * @return Reference to the current control manager
     */
	sim_mob::ControlManager* getControlMgr() const;

    /**
     * Retrives a reference to the bus stop num to bus stop map
     *
     * @return reference to the bus stop num to bus stop map
     */
	std::map<std::string, sim_mob::BusStop*>& getBusStopNo_BusStops();

    /**
     * Retrives a const reference to the bus stop num to bus stop map
     *
     * @return const reference to the bus stop num to bus stop map
     */
	const std::map<std::string, sim_mob::BusStop*>& getBusStopNo_BusStops() const;

    /**
     * Retrives a const reference to the lua scripts map
     *
     * @return const refernence to the lua scripts map
     */
	const ModelScriptsMap& getLuaScriptsMap() const;

    /**
     * Checks whether pathset mode is enabled
     *
     * @return true if enabled, else false
     */
	bool PathSetMode() const;

    /**
     * Retrieves a const reference to the pathset configuration
     *
     * @return const reference to the pathset configuration
     */
    PathSetConf& getPathSetConf();

    const PathSetConf& getPathSetConf() const;

    bool RunningMidTerm() const;

    bool RunningShortTerm() const;

    bool RunningLongTerm() const;

private:
    /**
     * Constructor
     */
	ConfigParams();

    /// Broker Factory
    sim_mob::Factory<sim_mob::Broker> brokerFact;

    /// Bus stop number to Bus stop mapping
	std::map<std::string, sim_mob::BusStop*> busStopNo_busStops;

    /// Mutable because they are set when retrieved.
	mutable ControlManager* controlMgr;

    /// Flag to indicate whether the network is sealed after loading
	bool sealedNetwork;

    bool workerPublisherEnabled;

    /// is public transit enabled
    bool publicTransitEnabled;

    /// pt edge travel time generation
    bool enabledEdgeTravelTime;

	/** name of file to store journey statistics */
	std::string journeyTimeStatsFilename;

	/** name of file to store waiting time statistics*/
	std::string waitingTimeStatsFilename;

	/** name of file to store waiting count statistics*/
	std::string waitingCountStatsFilename;
	
	/** interval (ms) of storage of the waiting count statistics*/
	unsigned int waitingCountStatsStorageInterval;

	/** name of file to store travel time statistics*/
	std::string travelTimeStatsFilename;

	/** name of file to store PT stop statistics */
	std::string ptStopStatsFilename;

public:
	/////////////////////////////////////////////////////////////////////////////////////
    /// These are helper functions, to make compatibility between old/new parsing easier.
	/////////////////////////////////////////////////////////////////////////////////////

    /**
     * Retrives a reference to Base system granularity, in milliseconds. Each "tick" is this long.
     *
     * @return reference to base system granularity in milliseconds
     */
	unsigned int& baseGranMS();

    /**
     * Retrieves a const reference to base system granularity in milliseconds. Each "tick" is this long.
     *
     * @return const reference to base system granularity in milliseconds
     */
	const unsigned int& baseGranMS() const;

    /**
     * Retrievs a const reference to base system granularity, in seconds. Each "tick" is this long.
     *
     * @return const reference to base system granularity in seconds
     */
	const double& baseGranSecond() const;

    /**
     * Checks whether to merge log files at the end of simulation
     * If true, we take time to merge the output of the individual log files after the simulation is complete.
     *
     * @return true is allowed to merge, else false
     */
	bool& isMergeLogFiles();

    /**
     * Checks whether to merge log files at the end of simulation
     * If true, we take time to merge the output of the individual log files after the simulation is complete.
     *
     * @return true is allowed to merge, else false
     */
    const bool isMergeLogFiles() const;

    /**
     * Retrieves the default workgroup assignment stratergy
     *
     * @return workgroup assignment stratergy
     */
	WorkGroup::ASSIGNMENT_STRATEGY& defaultWrkGrpAssignment();

    /**
     * Retrieves the default workgroup assignment stratergy
     *
     * @return workgroup assignment stratergy
     */
	const WorkGroup::ASSIGNMENT_STRATEGY& defaultWrkGrpAssignment() const;

    /**
     * Retrieves the mutex stratergy
     *
     * @return mutex stratergy
     */
	sim_mob::MutexStrategy& mutexStategy();

    /**
     * Retrieves the mutex stratergy
     *
     * @return mutex stratergy
     */
    const sim_mob::MutexStrategy& mutexStategy() const;

    /**
     * Retrieves the simulation start time
     *
     * @return simulation start time
     */
	DailyTime& simStartTime();

    /**
     * Retrieves the simulation start time
     *
     * @return simulation start time (const reference)
     */
	const DailyTime& simStartTime() const;

    /**
     * retrieves realtime travel time table name for route choice model
     *
     * @return realtime travel time table
     */
	const std::string& getRTTT() const;

    /**
     * retrieves default travel time table name for route choice model
     *
     * @return default travel time table
     */
	const std::string& getDTT() const;

    /**
     * Retrieves the total runtime in milliseconds
     *
     * @return total runtime in milliseconds
     */
    unsigned int totalRuntimeInMilliSeconds() const;

    /**
     * Retrieves the warmup time in milliseconds
     *
     * @return warmup time in milliseconds
     */
	unsigned int warmupTimeInMilliSeconds() const;

    bool isWorkerPublisherEnabled() const;

    void setWorkerPublisherEnabled(bool value);

    bool isGenerateBusRoutes() const;

    bool isPublicTransitEnabled() const;

    void setPublicTransitEnabled(bool value);

    bool isEnabledEdgeTravelTime() const;

    void setEnabledEdgeTravelTime(bool enabledEdgeTravelTime);

	/**
     * Retrives journey time stats file name
     *
     * @return journey time file name
     */
	const std::string& getJourneyTimeStatsFilename() const;

	/**
     * Sets journey time stats file name
     *
     * @param str journey time stats file name to be set
     */
	void setJourneyTimeStatsFilename(const std::string& str);

    /**
     * Retrieves waiting time stats file name
     *
     * @return waiting time file name
     */
	const std::string& getWaitingTimeStatsFilename() const;

    /**
     * Sets waiting time stats file name
     *
     * @param str waiting time stats file name to be set
     */
	void setWaitingTimeStatsFilename(const std::string& str);

    /**
     * Retrieves waiting count stats file name
     *
     * @return waiting count stats file name
     */
	const std::string& getWaitingCountStatsFilename() const;

    /**
     * Sets waiting count stats file name
     *
     * @param str waiting count stats file name to be set
     */
	void setWaitingCountStatsFilename(const std::string& str);
	
	/**
	 * Retrieves the storage interval for waiting counts
	 * 
	 * @return storage interval for waiting counts
	 */
	unsigned int getWaitingCountStatsInterval() const;
	
	/**
	 * Sets the storage interval for the waiting counts statistics
	 * 
	 * @param interval the interval in milli-seconds
	 */
	void setWaitingCountStatsInterval(unsigned int interval);

    /**
     * Retrieves travel time stats file name
     *
     * @return travel time stats file name
     */
	const std::string& getTravelTimeStatsFilename() const;

    /**
     * Sets travel time stats file name
     *
     * @param str travel time stats file name to be set
     */
	void setTravelTimeStatsFilename(const std::string& str);

    /**
     * Retrieves PT stop stats file name
     *
     * @return PT stop  stats file name
     */
	const std::string& getPT_StopStatsFilename() const;

    /**
     * Sets PT stop  stats file name
     *
     * @param str PT stop  stats file name to be set
     */
	void setPT_StopStatsFilename(const std::string& str);
};


}
