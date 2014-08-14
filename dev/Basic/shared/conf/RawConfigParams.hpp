//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <vector>
#include <map>
#include <string>

#include "buffering/Shared.hpp"
#include "conf/CMakeConfigParams.hpp"
#include "conf/Constructs.hpp"
#include "entities/AuraManager.hpp"
#include "geospatial/Point2D.hpp"
#include "workers/WorkGroup.hpp"
#include "util/DailyTime.hpp"

//TODO: Need to move the useful "Constructs" out of this file (e.g., Passwords)
#include "conf/Constructs.hpp"

namespace sim_mob {

///Represents the FMOD controller section of the config file.
struct FMOD_ControllerParams {
	FMOD_ControllerParams() : enabled(false), port(0), updateTimeMS(0), blockingTimeSec(0) {}

	bool enabled;
	std::string ipAddress;
	unsigned int port;
	unsigned int updateTimeMS;
	std::string mapfile;
	unsigned int blockingTimeSec;
};

//Represents the long-term developer model of the config file
struct LongTermParams{
	LongTermParams();
	bool enabled;
	unsigned int workers;
	unsigned int days;
	unsigned int tickStep;
	unsigned int maxIterations;

	struct DeveloperModel{
		DeveloperModel();
		bool enabled;
		unsigned int timeInterval;
	} developerModel;

	struct HousingModel{
		HousingModel();
		bool enabled;
		unsigned int timeInterval;
		unsigned int timeOnMarket;
	} housingModel;
};

struct PathSetParams
{
	bool enabled;
	std::string database;
	std::string credentials;
	std::string pathSetTableName;
	std::string singlePathTableName;
	std::string dbFunction;
	void setDefaultEnabled(bool value = false){
		enabled = value;
	}
	void setDefaultDB(std::string db = "fm_remote_path_choice", std::string choice = "fm_remote_path_choice"){
		database = db;
		credentials = choice;
	}
	void setDefaultTables(std::string pathset = "PathSet", std::string singlePath = "SinglePath"){
		pathSetTableName = pathset;
		singlePathTableName = singlePath;
	}
	void setDefaultFunction(std::string fn="get_path_set")
	{
		dbFunction = fn;
	}
	void setDefault(){
		setDefaultEnabled();
		setDefaultDB();
		setDefaultTables();
		setDefaultFunction();
	}
};

///represent the incident data section of the config file
struct IncidentParams {
	IncidentParams() : incidentId(-1), visibilityDistance(0), segmentId(-1), position(0), severity(0),
			capFactor(0), startTime(0), duration(0), length(0),	compliance(0), accessibility(0){}

	struct LaneParams {
		LaneParams() : laneId(0), speedLimit(0), xLaneStartPos(0), yLaneStartPos(0), xLaneEndPos(0),yLaneEndPos(0){}
		unsigned int laneId;
		float speedLimit;
		float xLaneStartPos;
		float yLaneStartPos;
		float xLaneEndPos;
		float yLaneEndPos;
	};

	unsigned int incidentId;
	float visibilityDistance;
	unsigned int segmentId;
	float position;
	unsigned int severity;
	float capFactor;
	unsigned int startTime;
	unsigned int duration;
	float length;
	float compliance;
	float accessibility;
	std::vector<LaneParams> laneParams;
};

///Represents a Person's Characteristic in the config file. (NOTE: Further documentation needed.)
struct PersonCharacteristics {
	PersonCharacteristics() : lowerAge(0), upperAge(0), lowerSecs(0), upperSecs(0) {}

	unsigned int lowerAge; // lowerAge
	unsigned int upperAge; // upperAge
	int lowerSecs;// lowerSecs
	int upperSecs;// upperSecs
};

///represent the person characteristics data section of the config file
struct PersonCharacteristicsParams {

	PersonCharacteristicsParams() : lowestAge(100), highestAge(0), DEFAULT_LOWER_SECS(3), DEFAULT_UPPER_SECS(10) {}
	int lowestAge;
	int highestAge;
	const int DEFAULT_LOWER_SECS;
	const int DEFAULT_UPPER_SECS;
	//Some settings for person characteristics(age range, boarding alighting secs)
	std::map<int, PersonCharacteristics> personCharacteristics;
};

///Represents a Bust Stop in the config file. (NOTE: Further documentation needed.)
struct BusStopScheduledTime {
	BusStopScheduledTime() : offsetAT(0), offsetDT(0) {}

	unsigned int offsetAT; //<Presumably arrival time?
	unsigned int offsetDT; //<Presumably departure time?
};

///Represents a complete connection to the database, via Construct ID.
struct DatabaseDetails {
	std::string database;
	std::string credentials;
	std::string procedures;
};


///Represents the "Constructs" section of the config file.
class Constructs {
public:
	//std::map<std::string, Distribution> distributions; //<TODO
	std::map<std::string, Database> databases;
	std::map<std::string, StoredProcedureMap> procedureMaps;
	std::map<std::string, Credential> credentials;
};


///Represents the "Simulation" section of the config file.
class SimulationParams {
public:
	SimulationParams();

	///Sources of Agents.
	enum LoadAgentsOrderOption {
		LoadAg_Drivers,       ///<Load Drivers from the config file.
		LoadAg_Pedestrians,   ///<Load Pedestrians from the config file.
		LoadAg_Passengers,    ///<Load Passengers from the config file.
		LoadAg_Database,      ///<Load Trip-Chain based entities from the database.
		LoadAg_XmlTripChains, ///<Not sure what this does exactly....
	};

	///Our reaction time distributions.
	struct ReactionTimeDistDescription {
		ReactionTimeDistDescription() : typeId(0), mean(0), stdev(0) {}

		int typeId;
		int mean;
		int stdev;
	};

	unsigned int baseGranMS;       ///<Base system granularity, in milliseconds. Each "tick" is this long.
	double baseGranSecond;         ///<Base system granularity, in seconds. Each "tick" is this long.
	unsigned int totalRuntimeMS;   ///<Total time (in milliseconds) to run the simulation for. (Includes "warmup" time.)
	unsigned int totalWarmupMS;    ///<Total time (in milliseconds) considered "warmup".

	DailyTime simStartTime; ///<When the simulation begins(based on configuration)
	std::string travelTimeTmpTableName;
	AuraManager::AuraManagerImplementation auraManagerImplementation; ///<What type of Aura Manager we're using.

	WorkGroup::ASSIGNMENT_STRATEGY workGroupAssigmentStrategy;  ///<Defautl assignment strategy for Workgroups.

	int partitioningSolutionId;  ///<Property specific to MPI version; not fully documented.

	std::vector<LoadAgentsOrderOption> loadAgentsOrder; ///<What order to load agents in.

	int startingAutoAgentID; ///<Default starting ID for agents with auto-generated IDs.

	sim_mob::MutexStrategy mutexStategy; ///<Locking strategy for Shared<> properties.

	struct Commsim {
		bool enabled;  ///< True if commsim is enabled. If false, no Broker will be created.
		int numIoThreads; ///< How many threads to allocate to boost's io processor.
		int minClients;  ///< The minimum number of simultaneous clients required to proceed with the simulation.
		int holdTick;    ///< The simulation tick that we will pause on until minClients connections are made.
		bool useNs3;  ///< If true, waits for the ns-3 simulator to connect.
		Commsim() : enabled(false), numIoThreads(1), minClients(1), holdTick(1), useNs3(false) {}
	};
	Commsim commsim;

	//Reaction time parameters.
	//TODO: This should be one of the first areas we clean up.
	ReactionTimeDistDescription reactTimeDistribution1;
	ReactionTimeDistDescription reactTimeDistribution2;

    //Passenger distribution parameters.
    //TODO: This should be the second thing we clean up.
    int passenger_distribution_busstop;
    int passenger_mean_busstop;
    int passenger_standardDev_busstop;
    int passenger_percent_boarding;
    int passenger_percent_alighting;
    int passenger_min_uniform_distribution;
    int passenger_max_uniform_distribution;
};

///Represents the "Workers" section of the config file.
class WorkerParams {
public:
	struct Worker {
		Worker();
		unsigned int count;
		unsigned int granularityMs;
	};

	Worker person;
	Worker signal;
	Worker communication;
};


///Represents the "System" section of the config file.
class SystemParams {
public:
	SystemParams();

	enum NetworkSource {
		NETSRC_XML,
		NETSRC_DATABASE,
	};

	SimulationParams simulation;
	WorkerParams workers;

	bool singleThreaded; ///<If true, we are running everything on one thread.
	bool mergeLogFiles;  ///<If true, we take time to merge the output of the individual log files after the simulation is complete.

	NetworkSource networkSource; ///<Whethere to load the network from the database or from an XML file.
	std::string networkXmlInputFile;  ///<If loading the network from an XML file, which file? Empty=private/SimMobilityInput.xml
	std::string networkXmlOutputFile;  ///<If loading the network from an XML file, which file? Empty=private/SimMobilityInput.xml
	DatabaseDetails networkDatabase; //<If loading from the database, how do we connect?

	std::string roadNetworkXsdSchemaFile; ///<Valid path to a schema file for loading XML road network files.

	std::map<std::string, std::string> genericProps; ///<Generic properties, for testing new features.
};


///Represents an entity in the "Drivers" or "Pedestrians" section of the config file.
struct EntityTemplate {
	EntityTemplate();
	Point2D originPos;
	Point2D destPos;
	unsigned int startTimeMs;// default is zero
	unsigned int laneIndex;// default is zero
};


/**
 * Contains the properties of the config file as they appear in, e.g., test_road_network.xml, with
 *   minimal conversion.
 * Derived properties (such as the road network) are listed in ConfigParams.
 *
 * \author Seth N. Hetu
 */
class RawConfigParams : public sim_mob::CMakeConfigParams {
public:
	RawConfigParams();

	///"Constructs" for general re-use.
	Constructs constructs;

	///"Sytem" level settings, including simulation parameters and global flags.
	SystemParams system;

	///Available gemetries (currently only database geometries).
	//GeometryParams geometry;

	///Settings for the FMOD controller.
	FMOD_ControllerParams fmod;

	///Settings for Long Term Parameters
	LongTermParams ltParams;

	///Settings used for generation/retrieval of paths
	PathSetParams pathset;

	///setting for the incidents
	std::vector<IncidentParams> incidents;

	///Some settings for bus stop arrivals/departures.
	std::map<int, BusStopScheduledTime> busScheduledTimes; //The int is a "bus stop ID", starting from 0.

	//Person characteristics parameters
	PersonCharacteristicsParams personCharacteristicsParams;

	//@{
	///Templates for creating entities of various types.
	std::vector<EntityTemplate> driverTemplates;
	std::vector<EntityTemplate> pedestrianTemplates;
	std::vector<EntityTemplate> busDriverTemplates;
	std::vector<EntityTemplate> signalTemplates;
	std::vector<EntityTemplate> passengerTemplates;
	std::vector<EntityTemplate> busControllerTemplates;
	//@}
};


}
