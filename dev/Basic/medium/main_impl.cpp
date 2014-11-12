//Copyright (c) 2014 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include <vector>
#include <string>
#include <cstdlib>

//TODO: Replace with <chrono> or something similar.
#include <sys/time.h>

//main.cpp (top-level) files can generally get away with including GenConfig.h
#include "GenConfig.h"

#include "behavioral/PredayManager.hpp"
#include "buffering/BufferedDataManager.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "conf/ParseConfigFile.hpp"
#include "conf/ExpandAndValidateConfigFile.hpp"
#include "database/DB_Connection.hpp"
#include "entities/incident/IncidentManager.hpp"
#include "entities/AuraManager.hpp"
#include "entities/Agent.hpp"
#include "entities/BusController.hpp"
#include "entities/Person.hpp"
#include "entities/roles/activityRole/ActivityPerformer.hpp"
#include "entities/roles/driver/Biker.hpp"
#include "entities/roles/driver/Driver.hpp"
#include "entities/roles/driver/BusDriver.hpp"
#include "entities/roles/pedestrian/Pedestrian.hpp"
#include "entities/roles/waitBusActivity/waitBusActivity.hpp"
#include "entities/roles/passenger/Passenger.hpp"
#include "entities/BusStopAgent.hpp"
#include "entities/PersonLoader.hpp"
#include "entities/profile/ProfileBuilder.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "geospatial/RoadNetwork.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "geospatial/Lane.hpp"
#include "geospatial/PathSetManager.hpp"
#include "logging/Log.hpp"
#include "partitions/PartitionManager.hpp"
#include "util/DailyTime.hpp"
#include "util/LangHelpers.hpp"
#include "util/Utils.hpp"
#include "workers/Worker.hpp"
#include "workers/WorkGroup.hpp"
#include "workers/WorkGroupManager.hpp"
#include "config/MT_Config.hpp"
#include "config/ParseMidTermConfigFile.hpp"

//If you want to force a header file to compile, you can put it here temporarily:
//#include "entities/BusController.hpp"

//Note: This must be the LAST include, so that other header files don't have
//      access to cout if output is disabled.
#include <iostream>

using std::cout;
using std::endl;
using std::vector;
using std::string;

using namespace sim_mob;
using namespace sim_mob::medium;

//Start time of program
timeval start_time_med;

namespace
{
const std::string MT_CONFIG_FILE = "data/medium/mt-config.xml";
} //End anonymous namespace

//Current software version.
const string SIMMOB_VERSION = string(SIMMOB_VERSION_MAJOR) + ":" + SIMMOB_VERSION_MINOR;

/**
 * Main simulation loop for the supply simulator
 * @param configFileName name of the input config xml file
 * @param resLogFiles name of the output log file
 * @return true if the function finishes execution normally
 */
bool performMainSupply(const std::string& configFileName, std::list<std::string>& resLogFiles)
{
	ProfileBuilder* prof = nullptr;
	if (ConfigManager::GetInstance().CMakeConfig().ProfileOn())
	{
		ProfileBuilder::InitLogFile("profile_trace.txt");
		prof = new ProfileBuilder();
	}

	//Loader params for our Agents
	WorkGroup::EntityLoadParams entLoader(Agent::pending_agents, Agent::all_agents);

	//Register our Role types.
	//TODO: Accessing ConfigParams before loading it is technically safe, but we
	//      should really be clear about when this is not ok.
	const MutexStrategy& mtx = ConfigManager::GetInstance().FullConfig().mutexStategy();
	RoleFactory& rf = ConfigManager::GetInstanceRW().FullConfig().getRoleFactoryRW();
	rf.registerRole("driver", new sim_mob::medium::Driver(nullptr, mtx));
	rf.registerRole("activityRole", new sim_mob::ActivityPerformer(nullptr));
	rf.registerRole("busdriver", new sim_mob::medium::BusDriver(nullptr, mtx));
	rf.registerRole("waitBusActivity", new sim_mob::medium::WaitBusActivity(nullptr, mtx));
	rf.registerRole("pedestrian", new sim_mob::medium::Pedestrian(nullptr, mtx));
	rf.registerRole("passenger", new sim_mob::medium::Passenger(nullptr, mtx));
	rf.registerRole("biker", new sim_mob::medium::Biker(nullptr, mtx));

	//Load our user config file, which is a time costly function
	ExpandAndValidateConfigFile expand(ConfigManager::GetInstanceRW().FullConfig(), Agent::all_agents, Agent::pending_agents);
	cout<<"performMainSupply: trip chain pool size "
		<<ConfigManager::GetInstance().FullConfig().getTripChains().size()
		<<endl;

	//insert bus stop agent to segmentStats;
	std::set<sim_mob::SegmentStats*>& segmentStatsWithStops = ConfigManager::GetInstanceRW().FullConfig().getSegmentStatsWithBusStops();
	std::set<sim_mob::SegmentStats*>::iterator itSegStats;
	std::vector<const sim_mob::BusStop*>::iterator itBusStop;
	StreetDirectory& strDirectory= StreetDirectory::instance();
	for (itSegStats = segmentStatsWithStops.begin(); itSegStats != segmentStatsWithStops.end(); itSegStats++)
	{
		sim_mob::SegmentStats* stats = *itSegStats;
		std::vector<const sim_mob::BusStop*>& busStops = stats->getBusStops();
		for (itBusStop = busStops.begin(); itBusStop != busStops.end(); itBusStop++)
		{
			const sim_mob::BusStop* stop = *itBusStop;
			sim_mob::medium::BusStopAgent* busStopAgent = new sim_mob::medium::BusStopAgent(mtx, -1, stop, stats);
			stats->addBusStopAgent(busStopAgent);
			BusStopAgent::registerBusStopAgent(busStopAgent);
			strDirectory.registerStopAgent(stop, busStopAgent);
		}
	}
	//Save a handle to the shared definition of the configuration.
	const ConfigParams& config = ConfigManager::GetInstance().FullConfig();

	//Start boundaries
#ifndef SIMMOB_DISABLE_MPI
	if (config.using_MPI)
	{
		PartitionManager& partitionImpl = PartitionManager::instance();
		partitionImpl.initBoundaryTrafficItems();
	}
#endif

	PartitionManager* partMgr = nullptr;
	if (!config.MPI_Disabled() && config.using_MPI)
	{
		partMgr = &PartitionManager::instance();
	}

	PeriodicPersonLoader periodicPersonLoader(Agent::all_agents, Agent::pending_agents);

	{ //Begin scope: WorkGroups
	WorkGroupManager wgMgr;
	wgMgr.setSingleThreadMode(config.singleThreaded());

	//Work Group specifications
	//Mid-term is not using Aura Manager at the moment. Therefore setting it to nullptr
	WorkGroup* personWorkers = wgMgr.newWorkGroup(config.personWorkGroupSize(), config.totalRuntimeTicks, config.granPersonTicks,
			nullptr /*AuraManager is not used in mid-term*/, partMgr, &periodicPersonLoader);

	//Initialize all work groups (this creates barriers, and locks down creation of new groups).
	wgMgr.initAllGroups();

	//Load persons for 0th tick
	periodicPersonLoader.loadActivitySchedules();

	//Initialize each work group individually
	personWorkers->initWorkers(&entLoader);

	personWorkers->assignConfluxToWorkers();
	//personWorkers->findBoundaryConfluxes();

	//Anything in all_agents is starting on time 0, and should be added now.
	for (std::set<Entity*>::iterator it = Agent::all_agents.begin(); it != Agent::all_agents.end(); it++)
	{
		personWorkers->putAgentOnConflux(dynamic_cast<sim_mob::Person*>(*it));
	}

	if(BusController::HasBusControllers())
	{
		personWorkers->assignAWorker(BusController::TEMP_Get_Bc_1());

	}
	//incident
	personWorkers->assignAWorker(IncidentManager::getInstance());

	cout << "Initial Agents dispatched or pushed to pending.all_agents: " << Agent::all_agents.size() << " pending: " << Agent::pending_agents.size() << endl;

	//Start work groups and all threads.
	wgMgr.startAllWorkGroups();

	//
	if (!config.MPI_Disabled() && config.using_MPI)
	{
		PartitionManager& partitionImpl = PartitionManager::instance();
		partitionImpl.setEntityWorkGroup(personWorkers, nullptr);
		cout<< "partition_solution_id in main function:"
			<< partitionImpl.partition_config->partition_solution_id
			<< endl;
	}

	/////////////////////////////////////////////////////////////////
	// NOTE: WorkGroups are able to handle skipping steps by themselves.
	//       So, we simply call "wait()" on every tick, and on non-divisible
	//       time ticks, the WorkGroups will return without performing
	//       a barrier sync.
	/////////////////////////////////////////////////////////////////
	size_t numStartAgents = Agent::all_agents.size();
	size_t numPendingAgents = Agent::pending_agents.size();
	size_t maxAgents = Agent::all_agents.size();

	timeval loop_start_time;
	gettimeofday(&loop_start_time, nullptr);
	int loop_start_offset = ProfileBuilder::diff_ms(loop_start_time, start_time_med);

	int lastTickPercent = 0; //So we have some idea how much time is left.
	for (unsigned int currTick = 0; currTick < config.totalRuntimeTicks; currTick++)
	{
		//Flag
		bool warmupDone = (currTick >= config.totalWarmupTicks);

		//Get a rough idea how far along we are
		int currTickPercent = (currTick*100)/config.totalRuntimeTicks;

		//Save the maximum number of agents at any given time
		maxAgents = std::max(maxAgents, Agent::all_agents.size());

		//Output
		if (ConfigManager::GetInstance().CMakeConfig().OutputEnabled())
		{
			std::stringstream msg;
			msg << "Approximate Tick Boundary: " << currTick << ", ";
			msg << (currTick * config.baseGranSecond())
				<< "s   [" <<currTickPercent <<"%]" << endl;
			if (!warmupDone)
			{
				msg << "  Warmup; output ignored." << endl;
			}
			PrintOut(msg.str());
		}
		else
		{
			//We don't need to lock this output if general output is disabled, since Agents won't
			//  perform any output (and hence there will be no contention)
			if (currTickPercent-lastTickPercent>9)
			{
				lastTickPercent = currTickPercent;
				cout<< currTickPercent <<"%"
					<< ", Agents:" << Agent::all_agents.size() <<endl;
			}
		}

		//Agent-based cycle, steps 1,2,3,4 of 4
		wgMgr.waitAllGroups();

		BusController::CollectAndProcessAllRequests();
	}

	//Finalize partition manager
#ifndef SIMMOB_DISABLE_MPI
	if (config.using_MPI)
	{
		PartitionManager& partitionImpl = PartitionManager::instance();
		partitionImpl.stopMPIEnvironment();
	}
#endif

	//finalize
	if (ConfigManager::GetInstance().FullConfig().PathSetMode()) {
		PathSetManager::getInstance()->copyTravelTimeDataFromTmp2RealtimeTable();
	}
	cout <<"Database lookup took: " << (loop_start_offset/1000.0) <<" s" <<endl;
	cout << "Max Agents at any given time: " <<maxAgents <<endl;
	cout << "Starting Agents: " << numStartAgents
			<< ",     Pending: " << numPendingAgents << endl;

	if (Agent::all_agents.empty())
	{
		cout << "All Agents have left the simulation.\n";
	}
	else
	{
		size_t numPerson = 0;
		size_t numDriver = 0;
		size_t numPedestrian = 0;
		for (std::set<Entity*>::iterator it = Agent::all_agents.begin(); it != Agent::all_agents.end(); it++)
		{
			Person* person = dynamic_cast<Person*> (*it);
			if (person)
			{
				numPerson++;
				if(person->getRole())
				{
					if (dynamic_cast<sim_mob::medium::Driver*>(person->getRole()))
					{
						numDriver++;
					}
					if (dynamic_cast<sim_mob::medium::Pedestrian*>(person->getRole()))
					{
						numPedestrian++;
					}
				}
			}
		}
		cout<< "Remaining Agents: " << numPerson << " (Person)   "
			<< (Agent::all_agents.size() - numPerson) << " (Other)"
			<< endl;
		cout<< "   Person Agents: " << numDriver << " (Driver)   "
			<< numPedestrian << " (Pedestrian)   "
			<< (numPerson - numDriver - numPedestrian) << " (Other)"
			<< endl;
	}

	if (ConfigManager::GetInstance().FullConfig().numAgentsSkipped>0)
	{
		cout<<"Agents SKIPPED due to invalid route assignment: "
			<<ConfigManager::GetInstance().FullConfig().numAgentsSkipped
			<<endl;
	}

	if (!Agent::pending_agents.empty())
	{
		cout<< "WARNING! There are still " << Agent::pending_agents.size()
			<< " Agents waiting to be scheduled; next start time is: "
			<< Agent::pending_agents.top()->getStartTime() << " ms\n";
	}

	if(personWorkers->getNumAgentsWithNoPath() > 0)
	{
		cout<< personWorkers->getNumAgentsWithNoPath()
			<< " persons were not added to the simulation because they could not find a path."
			<< endl;
	}

	//Save our output files if we are merging them later.
	if (ConfigManager::GetInstance().CMakeConfig().OutputEnabled()
			&& ConfigManager::GetInstance().FullConfig().mergeLogFiles())
	{
		resLogFiles = wgMgr.retrieveOutFileNames();
	}

	}  //End scope: WorkGroups.

	//Test: At this point, it should be possible to delete all Signals and Agents.
	clear_delete_vector(Signal::all_signals_);
	clear_delete_vector(Agent::all_agents);

	cout << "Simulation complete; closing worker threads." << endl;

	//Delete our profile pointer (if it exists)
	safe_delete_item(prof);
	return true;
}

/**
 * The preday demand simulator
 */
bool performMainDemand()
{
	std::srand(clock()); // set random seed for RNGs in preday
	const MT_Config& mtConfig = MT_Config::getInstance();
	PredayManager predayManager;
	predayManager.loadZones(db::MONGO_DB);
	predayManager.loadCosts(db::MONGO_DB);
	predayManager.loadPersonIds(db::MONGO_DB);
	predayManager.loadUnavailableODs(db::MONGO_DB);
	if(mtConfig.isOutputTripchains())
	{
		predayManager.loadZoneNodes(db::MONGO_DB);
	}
	if(mtConfig.runningPredayCalibration())
	{
		Print() << "Preday mode: calibration" << std::endl;
		predayManager.calibratePreday();
	}
	else
	{
		Print() << "Preday mode: " << (mtConfig.runningPredaySimulation()? "simulation":"logsum computation")  << std::endl;
		predayManager.dispatchPersons();
	}
	return true;
}

/**
 * Main simulation loop.
 * \note
 * For doxygen, we are setting the variable JAVADOC AUTOBRIEF to "true"
 * This isn't necessary for class-level documentation, but if we want
 * documentation for a short method (like "get" or "set") then it makes sense to
 * have a few lines containing brief/full comments. (See the manual's description
 * of JAVADOC AUTOBRIEF). Of course, we can discuss this first.
 *
 * \par
 * See Buffered.hpp for an example of this in action.
 *
 * \par
 * ~Seth
 *
 * This function is separate from main() to allow for easy scoping of WorkGroup objects.
 */
bool performMainMed(const std::string& configFileName, std::list<std::string>& resLogFiles)
{
	cout <<"Starting SimMobility, version " <<SIMMOB_VERSION <<endl;
	cout << "Main Thread[ " << boost::this_thread::get_id() << "]" << std::endl;

	//Parse the config file (this *does not* create anything, it just reads it.).
	ParseConfigFile parse(configFileName, ConfigManager::GetInstanceRW().FullConfig());

	//load configuration file for mid-term
	ParseMidTermConfigFile parseMT_Cfg(MT_CONFIG_FILE, MT_Config::getInstance(), ConfigManager::GetInstanceRW().FullConfig());

	//Enable or disable logging (all together, for now).
	//NOTE: This may seem like an odd place to put this, but it makes sense in context.
	//      OutputEnabled is always set to the correct value, regardless of whether ConfigParams()
	//      has been loaded or not. The new Config class makes this much clearer.
	if (ConfigManager::GetInstance().CMakeConfig().OutputEnabled())
	{
		//Log::Init("out.txt");
		Warn::Init("warn.log");
		Print::Init("<stdout>");
	}
	else
	{
		//Log::Ignore();
		Warn::Ignore();
		Print::Ignore();
	}

	if (ConfigManager::GetInstance().FullConfig().RunningMidSupply())
	{
		Print() << "Mid-term run mode: supply" << endl;
		return performMainSupply(configFileName, resLogFiles);
	}
	else if (ConfigManager::GetInstance().FullConfig().RunningMidDemand())
	{
		Print() << "Mid-term run mode: preday" << endl;
		return performMainDemand();
	}
	else
	{
		throw std::runtime_error("Invalid Mid-term run mode. Admissible values are \"demand\" and \"supply\"");
	}
}

int main_impl(int ARGC, char* ARGV[])
{
	std::vector<std::string> args = Utils::parseArgs(ARGC, ARGV);

	//Save start time
	gettimeofday(&start_time_med, nullptr);

	/**
	 * Check whether to run SimMobility or SimMobility-MPI
	 */
	ConfigParams& config = ConfigManager::GetInstanceRW().FullConfig();
	config.using_MPI = false;
#ifndef SIMMOB_DISABLE_MPI
	if (args.size() > 2 && args[2]=="mpi")
	{
		config.using_MPI = true;
	}
#endif

	/**
	 * set random be repeatable
	 */
	config.is_simulation_repeatable = true;

	/**
	 * Start MPI if using_MPI is true
	 */
#ifndef SIMMOB_DISABLE_MPI
	if (config.using_MPI)
	{
		PartitionManager& partitionImpl = PartitionManager::instance();
		std::string mpi_result = partitionImpl.startMPIEnvironment(ARGC, ARGV);
		if (mpi_result.compare("") != 0)
		{
			Warn() << "MPI Error:" << mpi_result << endl;
			exit(1);
		}
	}
#endif

	//Argument 1: Config file
	//Note: Don't change this here; change it by supplying an argument on the
	//      command line, or through Eclipse's "Run Configurations" dialog.
	std::string configFileName = "data/config.xml";

	if (args.size() > 1)
	{
		configFileName = args[1];
	}
	else
	{
		Print() << "No config file specified; using default." << endl;
	}
	Print() << "Using config file: " << configFileName << endl;

	//This should be moved later, but we'll likely need to manage random numbers
	//ourselves anyway, to make simulations as repeatable as possible.
	//if (config.is_simulation_repeatable)
	//{
		//TODO: Output the random seed here (and only here)
	//}

	//Perform main loop
	timeval simStartTime;
	gettimeofday(&simStartTime, nullptr);

	std::list<std::string> resLogFiles;
	int returnVal = performMainMed(configFileName, resLogFiles) ? 0 : 1;

	//Concatenate output files?
	if (!resLogFiles.empty())
	{
		resLogFiles.insert(resLogFiles.begin(), ConfigManager::GetInstance().FullConfig().outNetworkFileName);
		Utils::printAndDeleteLogFiles(resLogFiles);
	}

	timeval simEndTime;
	gettimeofday(&simEndTime, nullptr);

	Print() << "Done" << endl;
	cout << "Total simulation time: "<< (ProfileBuilder::diff_ms(simEndTime, simStartTime))/1000.0 << " seconds." << endl;

	return returnVal;
}

