#include "ExpandMidTermConfigFile.hpp"
#include "entities/params/PT_NetworkEntities.hpp"
#include "entities/BusController.hpp"
#include "entities/BusControllerMT.hpp"
#include "path/PathSetManager.hpp"

namespace sim_mob
{
namespace medium
{

ExpandMidTermConfigFile::ExpandMidTermConfigFile(MT_Config &mtCfg, ConfigParams &cfg) :
    cfg(cfg), mtCfg(mtCfg)
{
    processConfig();
}

void ExpandMidTermConfigFile::processConfig()
{
    cfg.simMobRunMode = ConfigParams::SimMobRunMode::MID_TERM;
    cfg.setWorkerPublisherEnabled(false);

    //Set the auto-incrementing ID.
    if (cfg.simulation.startingAutoAgentID < 0)
    {
	    throw std::runtime_error("Agent auto-id must start from >0.");
    }

    Agent::setIncrementIDStartValue(cfg.simulation.startingAutoAgentID, true);

    //Ensure granularities are multiples of each other. Then set the "ticks" based on each granularity.
    checkGranularities();
    setTicks();

    //Maintain unique/non-colliding IDs.
    ConfigParams::AgentConstraints constraints;
    constraints.startingAutoAgentID = cfg.simulation.startingAutoAgentID;

    if (mtCfg.RunningMidSupply())
    {
        sim_mob::RestrictedRegion::getInstance().populate();
    }

    //Detect sidewalks in the middle of the road.
    WarnMidroadSidewalks();

    if (mtCfg.publicTransitEnabled)
    {
        loadPublicTransitNetworkFromDatabase();
    }

    if (ConfigManager::GetInstance().FullConfig().pathSet().privatePathSetMode == "generation")
    {
        Print() << "bulk profiler start: " << std::endl;
        sim_mob::Profiler profile("bulk profiler start", true);
        //	This mode can be executed in the main function also but we need the street directory to be initialized first
        //	to be least intrusive to the rest of the code, we take a safe approach and run this mode from here, although a lot of
        //	unnecessary code will be executed.
        sim_mob::PathSetManager::getInstance()->bulkPathSetGenerator();
        Print() << "Bulk Generation Done " << profile.tick().first.count() << std::endl;
        exit(1);
    }
    if (ConfigManager::GetInstance().FullConfig().pathSet().publicPathSetMode == "generation")
    {
        Print() << "Public Transit bulk pathSet Generation started: " << std::endl;
        sim_mob::PT_PathSetManager::Instance().PT_BulkPathSetGenerator();
        Print() << "Public Transit bulk pathSet Generation Done: " << std::endl;
        exit(1);
    }

    //TODO: put its option in config xml
    //generateOD("/home/fm-simmobility/vahid/OD.txt", "/home/fm-simmobility/vahid/ODs.xml");
    //Process Confluxes if required
    if (mtCfg.RunningMidSupply())
    {
        size_t sizeBefore = cfg.getConfluxes().size();
        sim_mob::aimsun::Loader::ProcessConfluxes(ConfigManager::GetInstance().FullConfig().getNetwork());
        std::cout << cfg.getConfluxes().size() << " Confluxes created" << std::endl;
    }

    //register and initialize BusController
	if (cfg.busController.enabled)
	{
		sim_mob::BusControllerMT::RegisterBusController(-1, cfg.mutexStategy());
		sim_mob::BusController* busController = sim_mob::BusController::GetInstance();
		busController->initializeBusController(active_agents, cfg.getPT_BusDispatchFreq());
		active_agents.insert(busController);
	}

    /// Enable/Disble restricted region support based on configuration
    setRestrictedRegionSupport();

    //combine incident information to road network
    verifyIncidents();

    //Print some of the settings we just generated.
    printSettings();
}

void ExpandMidTermConfigFile::loadNetworkFromDatabase()
{
    std::cout << "Loading Road Network from the database.\n";
    sim_mob::aimsun::Loader::LoadNetwork(cfg.getDatabaseConnectionString(false),
                     cfg.procedureMaps,
					 cfg.getNetworkRW(), cfg.getTripChains(), nullptr);
}

void ExpandMidTermConfigFile::loadPublicTransitNetworkFromDatabase()
{
    PT_Network::getInstance().init();
}

void ExpandMidTermConfigFile::WarnMidroadSidewalks()
{
    const std::vector<Link*>& links = cfg.getNetwork().getLinks();
    for (std::vector<Link*>::const_iterator linkIt = links.begin(); linkIt != links.end(); ++linkIt)
    {
	const std::set<RoadSegment*>& segs = (*linkIt)->getUniqueSegments();
	for (std::set<RoadSegment*>::const_iterator segIt = segs.begin(); segIt != segs.end(); ++segIt)
	{
	    const std::vector<Lane*>& lanes = (*segIt)->getLanes();
	    for (std::vector<Lane*>::const_iterator laneIt = lanes.begin(); laneIt != lanes.end(); ++laneIt)
	    {
		//Check it.
		if ((*laneIt)->is_pedestrian_lane() &&
			    ((*laneIt) != (*segIt)->getLanes().front()) &&
			    ((*laneIt) != (*segIt)->getLanes().back()))
		{
		    Warn() << "Pedestrian lane is located in the middle of segment" << (*segIt)->getId() << "\n";
		}
	    }
	}
    }
}

void ExpandMidTermConfigFile::verifyIncidents()
{
    std::vector<IncidentParams>& incidents = cfg.getIncidents();
    const unsigned int baseGranMS = cfg.system.simulation.simStartTime.getValue();

    for (std::vector<IncidentParams>::iterator incIt = incidents.begin(); incIt != incidents.end(); ++incIt)
    {
	const RoadSegment* roadSeg = StreetDirectory::instance().getRoadSegment((*incIt).segmentId);

	if (roadSeg)
	{
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
	    item->startTime = (*incIt).startTime - baseGranMS;
	    item->visibilityDistance = (*incIt).visibilityDistance;

	    const std::vector<sim_mob::Lane*>& lanes = roadSeg->getLanes();
	    for (std::vector<IncidentParams::LaneParams>::iterator laneIt = incIt->laneParams.begin(); laneIt != incIt->laneParams.end(); ++laneIt)
	    {
		Incident::LaneItem lane;
		lane.laneId = laneIt->laneId;
		lane.speedLimit = laneIt->speedLimit;
		item->laneItems.push_back(lane);
		if (lane.laneId < lanes.size() && lane.laneId < incIt->laneParams.size())
		{
		    incIt->laneParams[lane.laneId].xLaneStartPos = lanes[lane.laneId]->polyline_[0].getX();
		    incIt->laneParams[lane.laneId].yLaneStartPos = lanes[lane.laneId]->polyline_[0].getY();
		    if (lanes[lane.laneId]->polyline_.size() > 0)
		    {
			unsigned int sizePoly = lanes[lane.laneId]->polyline_.size();
			incIt->laneParams[lane.laneId].xLaneEndPos = lanes[lane.laneId]->polyline_[sizePoly - 1].getX();
			incIt->laneParams[lane.laneId].yLaneEndPos = lanes[lane.laneId]->polyline_[sizePoly - 1].getY();
		    }
		}
	    }

	    RoadSegment* rs = const_cast<RoadSegment*> (roadSeg);
	    float length = rs->getLengthOfSegment();
	    centimeter_t pos = length * item->position / 100.0;
	    rs->addObstacle(pos, item);
	}
    }
}

void ExpandMidTermConfigFile::setRestrictedRegionSupport()
{
    PathSetManager::getInstance()->setRegionRestrictonEnabled(mtCfg.cbd);
}

void ExpandMidTermConfigFile::checkGranularities()
{
    //Granularity check
    const unsigned int baseGranMS = cfg.simulation.baseGranMS;
    const WorkerParams& workers = mtCfg.workers;

    if (cfg.system.simulation.totalRuntimeMS < baseGranMS)
    {
	    throw std::runtime_error("Total Runtime cannot be smaller than base granularity.");
    }
    if (cfg.system.simulation.totalWarmupMS != 0 && cfg.system.simulation.totalWarmupMS < baseGranMS)
    {
	    Warn() << "Warning! Total Warmup is smaller than base granularity.\n";
    }
    if (workers.person.granularityMs < baseGranMS)
    {
	    throw std::runtime_error("Person granularity cannot be smaller than base granularity.");
    }
}

bool ExpandMidTermConfigFile::setTickFromBaseGran(unsigned int& res, unsigned int tickLenMs)
{
	res = tickLenMs / cfg.system.simulation.baseGranMS;
	return tickLenMs % cfg.system.simulation.baseGranMS == 0;
}

void ExpandMidTermConfigFile::setTicks()
{
    if (!setTickFromBaseGran(cfg.totalRuntimeTicks, cfg.system.simulation.totalRuntimeMS))
    {
	Warn() << "Total runtime will be truncated by the base granularity\n";
    }
    if (!setTickFromBaseGran(cfg.totalWarmupTicks, cfg.system.simulation.totalWarmupMS))
    {
	Warn() << "Total warm-up will be truncated by the base granularity\n";
    }
    if (!setTickFromBaseGran(cfg.granPersonTicks, cfg.system.workers.person.granularityMs))
    {
	throw std::runtime_error("Person granularity not a multiple of base granularity.");
    }
}

void ExpandMidTermConfigFile::printSettings()
{
    std::cout << "Config parameters:\n";
    std::cout << "------------------\n";

    //Print the WorkGroup strategy.
    std::cout << "WorkGroup assignment: ";
    switch (cfg.defaultWrkGrpAssignment())
    {
    case WorkGroup::ASSIGN_ROUNDROBIN:
	std::cout << "roundrobin" << std::endl;
	break;
    case WorkGroup::ASSIGN_SMALLEST:
	std::cout << "smallest" << std::endl;
	break;
    default:
	std::cout << "<unknown>" << std::endl;
	break;
    }

    //Basic statistics
    std::cout << "  Base Granularity: " << cfg.baseGranMS() << " " << "ms" << "\n";
    std::cout << "  Total Runtime: " << cfg.totalRuntimeTicks << " " << "ticks" << "\n";
    std::cout << "  Total Warmup: " << cfg.totalWarmupTicks << " " << "ticks" << "\n";
    std::cout << "  Person Granularity: " << cfg.granPersonTicks << " " << "ticks" << "\n";
    std::cout << "  Start time: " << cfg.simStartTime().getStrRepr() << "\n";
    std::cout << "  Mutex strategy: " << (cfg.mutexStategy() == MtxStrat_Locked ? "Locked" : cfg.mutexStategy() == MtxStrat_Buffered ? "Buffered" : "Unknown") << "\n";

//Output Database details
//	if (cfg.system.networkSource == SystemParams::NETSRC_XML)
//	{
//		std::cout << "Network details loaded from xml file: " << cfg.system.networkXmlInputFile << "\n";
//	}
//	if (cfg.system.networkSource == SystemParams::NETSRC_DATABASE)
//	{
//		std::cout << "Network details loaded from database connection: " << cfg.getDatabaseConnectionString() << "\n";
//	}

    std::cout << "Network details loaded from database connection: " << cfg.getDatabaseConnectionString() << "\n";

    //Print the network (this will go to a different output file...)
    std::cout << "------------------\n";
    PrintNetwork(cfg, cfg.outNetworkFileName);
    std::cout << "------------------\n";
}


}
}
