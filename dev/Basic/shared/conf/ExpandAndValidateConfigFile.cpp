//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "ExpandAndValidateConfigFile.hpp"

#include <sstream>

#include "conf/settings/DisableMPI.h"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "conf/PrintNetwork.hpp"
#include "entities/Entity.hpp"
#include "entities/Agent.hpp"
#include "entities/Person.hpp"
#include "entities/BusController.hpp"
#include "entities/fmodController/FMOD_Controller.hpp"
#include "entities/amodController/AMODController.hpp"
#include "geospatial/Node.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "geospatial/Link.hpp"
#include "geospatial/RoadNetwork.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/RoadItem.hpp"
#include "geospatial/Incident.hpp"
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "geospatial/xmlLoader/geo10.hpp"
#include "geospatial/xmlWriter/boostXmlWriter.hpp"
//#include "geospatial/xmlWriter/xmlWriter.hpp"
#include "partitions/PartitionManager.hpp"
#include "util/ReactionTimeDistributions.hpp"
#include "util/Utils.hpp"
#include "workers/WorkGroup.hpp"

using namespace sim_mob;


namespace {

ReactionTimeDist* GenerateReactionTimeDistribution(SimulationParams::ReactionTimeDistDescription rdist)  {
	if (rdist.typeId==0) {
		return new NormalReactionTimeDist(rdist.mean, rdist.stdev);
	} else if (rdist.typeId==1) {
		return new LognormalReactionTimeDist(rdist.mean, rdist.stdev);
	} else {
		throw std::runtime_error("Unknown reaction time magic number.");
	}
}

void InformLoadOrder(const std::vector<SimulationParams::LoadAgentsOrderOption>& order) {
	std::cout <<"Agent Load order: ";
	if (order.empty()) {
		std::cout <<"<N/A>";
	} else {
		for (std::vector<SimulationParams::LoadAgentsOrderOption>::const_iterator it=order.begin(); it!=order.end(); it++) {
			if ((*it)==SimulationParams::LoadAg_Drivers) {
				std::cout <<"drivers";
			} else if ((*it)==SimulationParams::LoadAg_Database) {
				std::cout <<"database";
			} else if ((*it)==SimulationParams::LoadAg_Pedestrians) {
				std::cout <<"pedestrians";
			} else {
				std::cout <<"<unknown>";
			}
			std::cout <<"  ";
		}
	}
	std::cout <<std::endl;
}


////
//// TODO: Eventually, we need to re-write WorkGroup to encapsulate the functionality of "addOrStash()".
////       For now, just make sure that if you add something to all_agents manually, you call "load()" before.
////
void addOrStashEntity(Agent* p, std::set<Entity*>& active_agents, StartTimePriorityQueue& pending_agents)
{
	//Only agents with a start time of zero should start immediately in the all_agents list.
	if (p->getStartTime()==0) {
		p->load(p->getConfigProperties());
		p->clearConfigProperties();
		active_agents.insert(p);
	} else {
		//Start later.
		pending_agents.push(p);
	}
}



} //End un-named namespace


sim_mob::ExpandAndValidateConfigFile::ExpandAndValidateConfigFile(ConfigParams& result, std::set<sim_mob::Entity*>& active_agents, StartTimePriorityQueue& pending_agents) : cfg(result), active_agents(active_agents), pending_agents(pending_agents)
{
	ProcessConfig();
}

void sim_mob::ExpandAndValidateConfigFile::ProcessConfig()
{
	//Set reaction time distributions
	//TODO: Refactor to avoid magic numbers
	cfg.reactDist1 = GenerateReactionTimeDistribution(cfg.system.simulation.reactTimeDistribution1);
	cfg.reactDist2 = GenerateReactionTimeDistribution(cfg.system.simulation.reactTimeDistribution2);

	//Inform of load order (drivers, database, pedestrians, etc.).
	InformLoadOrder(cfg.system.simulation.loadAgentsOrder);

	//Set the auto-incrementing ID.
	if (cfg.system.simulation.startingAutoAgentID<0) {
		throw std::runtime_error("Agent auto-id must start from >0.");
	}
	Agent::SetIncrementIDStartValue(cfg.system.simulation.startingAutoAgentID, true);

	//Print schema file.
	const std::string schem = cfg.roadNetworkXsdSchemaFile();
	Print() <<"XML (road network) schema file: "  <<(schem.empty()?"<default>":schem) <<std::endl;

	//Ensure granularities are multiples of each other. Then set the "ticks" based on each granularity.
	CheckGranularities();
	SetTicks();

	//Set PartitionManager instance (if using MPI and it's enabled).
	if (cfg.MPI_Enabled() && cfg.using_MPI) {
		int partId = cfg.system.simulation.partitioningSolutionId;
		PartitionManager::instance().partition_config->partition_solution_id = partId;
		std::cout << "partition_solution_id in configuration:" <<partId << std::endl;
	}

	//Load from database or XML.
	//TODO: This should be moved into its own class; we should NOT be doing loading in ExpandAndValidate()
	//      (it is here now to maintain compatibility with the old order or loading things).
	LoadNetworkFromDatabase();

	//TEMP: Test network output via boost.
	//todo: enable/disble through cinfig
	BoostSaveXML(cfg.networkXmlOutputFile(), cfg.getNetworkRW());

	//Detect sidewalks in the middle of the road.
	WarnMidroadSidewalks();

 	//Generate lanes, before StreetDirectory::init()
 	RoadNetwork::ForceGenerateAllLaneEdgePolylines(cfg.getNetworkRW());

    //Seal the network; no more changes can be made after this.
 	cfg.sealNetwork();
    std::cout << "Network Sealed" << std::endl;

    //Write the network (? This is weird. ?)
    if (cfg.XmlWriterOn()) {
    	throw std::runtime_error("Old WriteXMLInput function deprecated; use boost instead.");
    	//sim_mob::WriteXMLInput("TEMP_TEST_OUT.xml");
    	std::cout << "XML input for SimMobility Created....\n";
    }

 	//Initialize the street directory.
    StreetDirectory::instance().init(cfg.getNetwork(), true);
    std::cout << "Street Directory initialized" <<std::endl;

    //Process Confluxes if required
    if(cfg.RunningMidSupply()) {
		size_t sizeBefore = cfg.getConfluxes().size();
		sim_mob::aimsun::Loader::ProcessConfluxes(ConfigManager::GetInstance().FullConfig().getNetwork());
		std::cout <<"Confluxes size before(" <<sizeBefore <<") and after(" <<cfg.getConfluxes().size() <<")\n";
    }

    //Maintain unique/non-colliding IDs.
    ConfigParams::AgentConstraints constraints;
    constraints.startingAutoAgentID = cfg.system.simulation.startingAutoAgentID;

    //Start all "BusController" entities.
    for (std::vector<EntityTemplate>::const_iterator it=cfg.busControllerTemplates.begin(); it!=cfg.busControllerTemplates.end(); it++) {
    	sim_mob::BusController::RegisterNewBusController(it->startTimeMs, cfg.mutexStategy());
	}

    //Start all "FMOD" entities.
    LoadFMOD_Controller();

    LoadAMOD_Controller();

	//combine incident information to road network
	verifyIncidents();

    //Initialize all BusControllers.
	if(BusController::HasBusControllers()) {
		BusController::InitializeAllControllers(active_agents, cfg.getPT_bus_dispatch_freq());
	}

	//Load Agents, Pedestrians, and Trip Chains as specified in loadAgentOrder
	LoadAgentsInOrder(constraints);

    //Load signals, which are currently agents
    GenerateXMLSignals();

    //Print some of the settings we just generated.
    PrintSettings();

    //Start the BusCotroller
    if(BusController::HasBusControllers()) {
    	BusController::DispatchAllControllers(active_agents);
    }
}

void sim_mob::ExpandAndValidateConfigFile::verifyIncidents()
{
	std::vector<IncidentParams>& incidents = cfg.getIncidents();
	const unsigned int baseGranMS = cfg.system.simulation.simStartTime.getValue();

	for(std::vector<IncidentParams>::iterator incIt=incidents.begin(); incIt!=incidents.end(); incIt++){

		const RoadSegment* roadSeg = StreetDirectory::instance().getRoadSegment((*incIt).segmentId);

		if(roadSeg) {

			Incident* item = new sim_mob::Incident();
			item->accessibility = (*incIt).accessibility;
			item->capFactor = (*incIt).capFactor;
			item->compliance = (*incIt).compliance;
			item->duration = (*incIt).duration;
			item->incidentId = (*incIt).incidentId;
			item->position = (*incIt).position;
			item->segmentId = (*incIt).segmentId;
			item->length = (*incIt).length;
			item->severity = (*incIt).severity;
			item->startTime = (*incIt).startTime-baseGranMS;
			item->visibilityDistance = (*incIt).visibilityDistance;

			const std::vector<sim_mob::Lane*>& lanes = roadSeg->getLanes();
			for(std::vector<IncidentParams::LaneParams>::iterator laneIt=incIt->laneParams.begin(); laneIt!=incIt->laneParams.end(); laneIt++){
				Incident::LaneItem lane;
				lane.laneId = laneIt->laneId;
				lane.speedLimit = laneIt->speedLimit;
				item->laneItems.push_back(lane);
				if(lane.laneId<lanes.size() && lane.laneId<incIt->laneParams.size()){
					incIt->laneParams[lane.laneId].xLaneStartPos = lanes[lane.laneId]->polyline_[0].getX();
					incIt->laneParams[lane.laneId].yLaneStartPos = lanes[lane.laneId]->polyline_[0].getY();
					if(lanes[lane.laneId]->polyline_.size()>0){
						unsigned int sizePoly = lanes[lane.laneId]->polyline_.size();
						incIt->laneParams[lane.laneId].xLaneEndPos = lanes[lane.laneId]->polyline_[sizePoly-1].getX();
						incIt->laneParams[lane.laneId].yLaneEndPos = lanes[lane.laneId]->polyline_[sizePoly-1].getY();
					}
				}
			}

			RoadSegment* rs = const_cast<RoadSegment*>(roadSeg);
			float length = rs->getLengthOfSegment();
			centimeter_t pos = length*item->position/100.0;
			rs->addObstacle(pos, item);
		}
	}


}

void sim_mob::ExpandAndValidateConfigFile::CheckGranularities()
{
    //Granularity check
	const unsigned int baseGranMS = cfg.system.simulation.baseGranMS;
	const WorkerParams& workers = cfg.system.workers;

    if (cfg.system.simulation.totalRuntimeMS < baseGranMS) {
    	throw std::runtime_error("Total Runtime cannot be smaller than base granularity.");
    }
    if (cfg.system.simulation.totalWarmupMS != 0 && cfg.system.simulation.totalWarmupMS < baseGranMS) {
    	Warn() << "Warning! Total Warmup is smaller than base granularity.\n";
    }
    if (workers.person.granularityMs < baseGranMS) {
    	throw std::runtime_error("Person granularity cannot be smaller than base granularity.");
    }
    if (workers.signal.granularityMs < baseGranMS) {
    	throw std::runtime_error("Signal granularity cannot be smaller than base granularity.");
    }
    if (workers.communication.granularityMs < baseGranMS) {
    	throw std::runtime_error("Communication granularity cannot be smaller than base granularity.");
    }
}


bool sim_mob::ExpandAndValidateConfigFile::SetTickFromBaseGran(unsigned int& res, unsigned int tickLenMs)
{
	res = tickLenMs/cfg.system.simulation.baseGranMS;
	return tickLenMs%cfg.system.simulation.baseGranMS == 0;
}


void sim_mob::ExpandAndValidateConfigFile::SetTicks()
{
	if (!SetTickFromBaseGran(cfg.totalRuntimeTicks, cfg.system.simulation.totalRuntimeMS)) {
		Warn() <<"Total runtime will be truncated by the base granularity\n";
	}
	if (!SetTickFromBaseGran(cfg.totalWarmupTicks, cfg.system.simulation.totalWarmupMS)) {
		Warn() <<"Total warm-up will be truncated by the base granularity\n";
	}
	if (!SetTickFromBaseGran(cfg.granPersonTicks, cfg.system.workers.person.granularityMs)) {
		throw std::runtime_error("Person granularity not a multiple of base granularity.");
	}
	if (!SetTickFromBaseGran(cfg.granSignalsTicks, cfg.system.workers.signal.granularityMs)) {
		throw std::runtime_error("Signal granularity not a multiple of base granularity.");
	}
	if (!SetTickFromBaseGran(cfg.granCommunicationTicks, cfg.system.workers.communication.granularityMs)) {
		throw std::runtime_error("Communication granularity not a multiple of base granularity.");
	}
}


void sim_mob::ExpandAndValidateConfigFile::LoadNetworkFromDatabase()
{
	//Load from the database or from XML, depending.
	if (ConfigManager::GetInstance().FullConfig().networkSource()==SystemParams::NETSRC_DATABASE) {
		std::cout <<"Loading Road Network from the database.\n";
		sim_mob::aimsun::Loader::LoadNetwork(cfg.getDatabaseConnectionString(false), cfg.getDatabaseProcMappings().procedureMappings, cfg.getNetworkRW(), cfg.getTripChains(), nullptr);
	} else {
		std::cout <<"Loading Road Network from XML.\n";
		// load segment,node type from db ,TODO add type attribute to xml file
		sim_mob::aimsun::Loader::loadSegNodeType(cfg.getDatabaseConnectionString(false),
				cfg.getDatabaseProcMappings().procedureMappings,
				cfg.getNetworkRW());

//		DatabaseLoader loader(cfg.getDatabaseConnectionString(false));
//		loader.loadObjectType(cfg.getDatabaseProcMappings().procedureMappings,cfg.getNetworkRW());
		if (!sim_mob::xml::InitAndLoadXML(cfg.networkXmlInputFile(), cfg.getNetworkRW(), cfg.getTripChains())) {
			throw std::runtime_error("Error loading/parsing XML file (see stderr).");
		}
	}
}


void sim_mob::ExpandAndValidateConfigFile::WarnMidroadSidewalks()
{
	const std::vector<Link*>& links = cfg.getNetwork().getLinks();
	for(std::vector<Link*>::const_iterator linkIt=links.begin(); linkIt!=links.end(); linkIt++) {
		const std::set<RoadSegment*>& segs = (*linkIt)->getUniqueSegments();
		for (std::set<RoadSegment*>::const_iterator segIt=segs.begin(); segIt!=segs.end(); segIt++) {
			const std::vector<Lane*>& lanes = (*segIt)->getLanes();
			for (std::vector<Lane*>::const_iterator laneIt=lanes.begin(); laneIt!=lanes.end(); laneIt++) {
				//Check it.
				if((*laneIt)->is_pedestrian_lane() &&
					((*laneIt) != (*segIt)->getLanes().front()) &&
					((*laneIt) != (*segIt)->getLanes().back()))
				{
					Warn() << "Pedestrian lane is located in the middle of segment" <<(*segIt)->getSegmentID() <<"\n";
				}
			}
		}
	}
}

void sim_mob::ExpandAndValidateConfigFile::LoadFMOD_Controller()
{
	if (cfg.fmod.enabled) {
		sim_mob::FMOD::FMOD_Controller::registerController(-1, cfg.mutexStategy());
		//sim_mob::FMOD::FMOD_Controller::instance()->settings(cfg.fmod.ipAddress, cfg.fmod.port, cfg.fmod.updateTravelMS, cfg.fmod.updatePosMS, cfg.fmod.mapfile, cfg.fmod.blockingTimeSec);
		sim_mob::FMOD::FMOD_Controller::instance()->connectFmodService();
	}
}
void sim_mob::ExpandAndValidateConfigFile::LoadAMOD_Controller()
{
	if (cfg.amod.enabled) {
		sim_mob::AMOD::AMODController::registerController(-1, cfg.mutexStategy());
		sim_mob::AMOD::AMODController::instance();
//		sim_mob::AMOD::AMODController::instance()->connectAmodService();
	}
}


void sim_mob::ExpandAndValidateConfigFile::LoadAgentsInOrder(ConfigParams::AgentConstraints& constraints)
{
	typedef std::vector<SimulationParams::LoadAgentsOrderOption> LoadOrder;
	const LoadOrder& order = cfg.system.simulation.loadAgentsOrder;
	for (LoadOrder::const_iterator it = order.begin(); it!=order.end(); it++) {
		switch (*it) {
			case SimulationParams::LoadAg_Database: //fall-through
			case SimulationParams::LoadAg_XmlTripChains:
				//Create an agent for each Trip Chain in the database.
				GenerateAgentsFromTripChain(constraints);

				//Initialize the FMOD controller now that all entities have been loaded.
				if( sim_mob::FMOD::FMOD_Controller::instanceExists() ) {
					sim_mob::FMOD::FMOD_Controller::instance()->initialize();
				}
				std::cout <<"Loaded Database Agents (from Trip Chains).\n";
				break;
			case SimulationParams::LoadAg_Drivers:
				GenerateXMLAgents(cfg.driverTemplates, "driver", constraints);
				GenerateXMLAgents(cfg.taxiDriverTemplates, "taxidriver", constraints);
				GenerateXMLAgents(cfg.busDriverTemplates, "busdriver", constraints);
				std::cout <<"Loaded Driver Agents (from config file).\n";
				break;
			case SimulationParams::LoadAg_Pedestrians:
				GenerateXMLAgents(cfg.pedestrianTemplates, "pedestrian", constraints);
				std::cout <<"Loaded Pedestrian Agents (from config file).\n";
				break;
			case SimulationParams::LoadAg_Passengers:
				GenerateXMLAgents(cfg.passengerTemplates, "passenger", constraints);
				std::cout << "Loaded Passenger Agents (from config file).\n";
				break;
			default:
				throw std::runtime_error("Unknown item in load_agents");
		}
	}
	std::cout << "Loading Agents, Pedestrians, and Trip Chains as specified in loadAgentOrder: Success!\n";
}


void sim_mob::ExpandAndValidateConfigFile::GenerateAgentsFromTripChain(ConfigParams::AgentConstraints& constraints)
{
	//NOTE: "constraints" are not used here, but they could be (for manual ID specification).
	typedef std::map<std::string, std::vector<TripChainItem*> > TripChainMap;
	const TripChainMap& tcs = cfg.getTripChains();

	//The current agent we are working on.
	for (TripChainMap::const_iterator it_map=tcs.begin(); it_map!=tcs.end(); it_map++) {
		TripChainItem* tc = it_map->second.front();
		if( tc->itemType != TripChainItem::IT_FMODSIM){
			Person* person = new sim_mob::Person("XML_TripChain", cfg.mutexStategy(), it_map->second);
			person->setPersonCharacteristics();
			addOrStashEntity(person, active_agents, pending_agents);
		} else {
			//insert to FMOD controller so that collection of requests
			if (sim_mob::FMOD::FMOD_Controller::instanceExists()) {
				sim_mob::FMOD::FMOD_Controller::instance()->insertFmodItems(it_map->first, tc);
			} else {
				Warn() <<"Skipping FMOD agent; FMOD controller is not active.\n";
			}
		}
	}
}


void sim_mob::ExpandAndValidateConfigFile::GenerateXMLAgents(const std::vector<EntityTemplate>& xmlItems, const std::string& roleName, ConfigParams::AgentConstraints& constraints)
{
	//Do nothing for empty roles.
	if (xmlItems.empty()) {
		return;
	}

	//At the moment, we only load *Roles* from the config file. So, check if this is a valid role.
	// This will only generate an error if someone actually tries to load an agent of this type.
	const RoleFactory& rf = cfg.getRoleFactory();
	bool knownRole = rf.isKnownRole(roleName);

	//If at least one elemnt of an unknown type exists, it's an error.
	if (!knownRole) {
		std::stringstream msg;
		msg <<"Unexpected agent type: " <<roleName;
		throw std::runtime_error(msg.str().c_str());
	}

	//Loop through all agents of this type.
	for (std::vector<EntityTemplate>::const_iterator it=xmlItems.begin(); it!=xmlItems.end(); it++) {
		//Keep track of the properties we have found.
		//TODO: Currently, this is only used for the "#mode" flag, and for forwarding the "originPos" and "destPos"
		std::map<std::string, std::string> props;



		props["lane"] = Utils::toStr<unsigned int>(it->laneIndex);
		props["initSegId"] = Utils::toStr<unsigned int>(it->initSegId);
		props["initDis"] = Utils::toStr<unsigned int>(it->initDis);
		props["initSpeed"] = Utils::toStr<unsigned int>(it->initSpeed);
		if(it->originNode>0 && it->destNode>0) {
			props["originNode"] = Utils::toStr<unsigned int>(it->originNode);
			props["destNode"] = Utils::toStr<unsigned int>(it->destNode);
		}
		else {
			//TODO: This is very wasteful
			{
			std::stringstream msg;
			msg <<it->originPos.getX() <<"," <<it->originPos.getY();
			props["originPos"] = msg.str();
			}
			{
			std::stringstream msg;
			msg <<it->destPos.getX() <<"," <<it->destPos.getY();
			props["destPos"] = msg.str();
			}
		}
//		props["lane"] = boost::lexical_cast<std::string>(it->laneIndex);
		{
			//Loop through attributes, ensuring that all required attributes are found.
			//std::map<std::string, bool> propLookup = rf.getRequiredAttributes(roleName);
			//size_t propsLeft = propLookup.size();
			//TODO: For now, we always check attributes anyway. We can change this later
			//      to check based on the actual role factory, and to add additional property types.
			//      (This was already checked in ParseConfigFile).
		}

		//We should generate the Agent's ID here (since otherwise Agents
		//  will have seemingly random IDs that do not reflect their order in the config file).
		//It is generally preferred to use the automatic IDs, but if a manual ID is specified we
		//  must deal with it here.
		//TODO: At the moment, manual IDs don't work. We can easily re-add them if required.
		int manualID = -1;
		if(it->angentId != 0)
			manualID = it->angentId;
		//map<string, string>::iterator propIt = props.find("id");
		/*if (propIt != props.end()) {
			//Convert the ID to an integer.
			std::istringstream(propIt->second) >> manualID;

			//Simple constraint check.
			if (manualID<0 || manualID>=constraints.startingAutoAgentID) {
				throw std::runtime_error("Manual ID must be within the bounds specified in the config file.");
			}

			//Ensure agents are created with unique IDs
			if (constraints.manualAgentIDs.count(manualID)>0) {
				std::stringstream msg;
				msg <<"Duplicate manual ID: " <<manualID;
				throw std::runtime_error(msg.str().c_str());
			}

			//Mark it, save it, remove it from the list
			constraints.manualAgentIDs.insert(manualID);
			props.erase(propIt);
		}*/

		//Finally, set the "#mode" flag in the configProps array.
		// (XML can't have # inside tag names, so this will never be overwritten)
		//
		//TODO: We should just be able to save "driver" and "pedestrian", but we are
		//      using different vocabulary for modes and roles. We need to change this.
		if(roleName == "driver")
		{
			props["#mode"] = "Car";
		}
		else if(roleName == "taxidriver")
		{
			props["#mode"] = "Taxi";
		}
		else if(roleName == "pedestrian")
		{
			props["#mode"] = "Walk";
		}
		else if (roleName == "busdriver")
		{
			props["#mode"] = "Bus";
		}
		else if (roleName == "passenger")
		{
			props["#mode"] = "BusTravel";
		}
		else
		{
			props["#mode"] = "Unknown";
		}

		//Create the Person agent with that given ID (or an auto-generated one)
		Person* agent = new Person("XML_Def", cfg.mutexStategy(), manualID);
		agent->setConfigProperties(props);
		agent->setStartTime(it->startTimeMs);

		//Add it or stash it
		addOrStashEntity(agent, active_agents, pending_agents);
	}
}


void sim_mob::ExpandAndValidateConfigFile::GenerateXMLSignals()
{
	if (cfg.signalTemplates.empty()) { return; }
	StreetDirectory& streetDirectory = StreetDirectory::instance();

	//Loop through all agents of this type
	for (std::vector<EntityTemplate>::const_iterator it=cfg.signalTemplates.begin(); it!=cfg.signalTemplates.end(); it++) {
		//Find the nearest Node for this Signal.
		Node* road_node = cfg.getNetwork().locateNode(it->originPos, true);
		if (!road_node) {
			Warn()  << "xpos=\"" <<it->originPos.getX() << "\" and ypos=\"" <<it->originPos.getY()
					<< "\" are not suitable attributes for Signal because there is no node there; correct the config file."
					<< std::endl;
			continue;
		}

		// See the comments in createSignals() in geospatial/aimsun/Loader.cpp.
		// At some point in the future, this function loadXMLSignals() will be removed
		// in its entirety, not just the following code fragment.
		std::set<const Link*> links;
		if (MultiNode const * multi_node = dynamic_cast<MultiNode const *>(road_node)) {
			std::set<RoadSegment*> const & roads = multi_node->getRoadSegments();
			std::set<RoadSegment*>::const_iterator iter;
			for (iter = roads.begin(); iter != roads.end(); ++iter) {
				RoadSegment const * road = *iter;
				links.insert(road->getLink());
			}
		}

		if (links.size() != 4) {
			Warn()  <<"the multi-node at " <<it->originPos << " does not have 4 links; "
					<< "no signal will be created here." << std::endl;
			continue;
		}

		const Signal* signal = streetDirectory.signalAt(*road_node);
		if (signal) {
			Warn()  << "signal at node(" <<it->originPos << ") already exists; "
					<< "skipping this config file entry" << std::endl;
		} else {
			Warn() <<"signal at node(" <<it->originPos << ") was not found; No more action will be taken\n ";
//          // The following call will create and register the signal with the
//          // street-directory.
//          std::cout << "register signal again!" << std::endl;
//          Signal::signalAt(*road_node, ConfigParams::GetInstance().mutexStategy);
		}
	}
}


void sim_mob::ExpandAndValidateConfigFile::PrintSettings()
{
    std::cout <<"Config parameters:\n";
    std::cout <<"------------------\n";
    std::cout <<"Force single-threaded: " <<(cfg.singleThreaded()?"yes":"no") <<"\n";

	//Print the WorkGroup strategy.
	std::cout <<"WorkGroup assignment: ";
	switch (cfg.defaultWrkGrpAssignment()) {
		case WorkGroup::ASSIGN_ROUNDROBIN:
			std::cout <<"roundrobin" <<std::endl;
			break;
		case WorkGroup::ASSIGN_SMALLEST:
			std::cout <<"smallest" <<std::endl;
			break;
		default:
			std::cout <<"<unknown>" <<std::endl;
			break;
	}

	//Basic statistics
	std::cout <<"  Base Granularity: " <<cfg.baseGranMS() <<" " <<"ms" <<"\n";
    std::cout <<"  Total Runtime: " <<cfg.totalRuntimeTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Total Warmup: " <<cfg.totalWarmupTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Person Granularity: " <<cfg.granPersonTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Signal Granularity: " <<cfg.granSignalsTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Communication Granularity: " <<cfg.granCommunicationTicks <<" " <<"ticks" <<"\n";
    std::cout <<"  Start time: " <<cfg.simStartTime().toString() <<"\n";
    std::cout <<"  Mutex strategy: " <<(cfg.mutexStategy()==MtxStrat_Locked?"Locked":cfg.mutexStategy()==MtxStrat_Buffered?"Buffered":"Unknown") <<"\n";

	//Output Database details
    if (cfg.system.networkSource==SystemParams::NETSRC_XML) {
    	std::cout <<"Network details loaded from xml file: " <<cfg.system.networkXmlInputFile <<"\n";
    }
    if (cfg.system.networkSource==SystemParams::NETSRC_DATABASE) {
    	std::cout <<"Network details loaded from database connection: " <<cfg.getDatabaseConnectionString() <<"\n";
    }

    //Print the network (this will go to a different output file...)
	std::cout <<"------------------\n";
	PrintNetwork(cfg, cfg.outNetworkFileName);
	std::cout <<"------------------\n";
}




