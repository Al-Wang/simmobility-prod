#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <stdlib.h>

#include "ParseShortTermConfigFile.hpp"
#include "entities/AuraManager.hpp"
#include "path/ParsePathXmlConfig.hpp"
#include "logging/Log.hpp"
#include "util/GeomHelpers.hpp"
#include "util/XmlParseHelper.hpp"

using namespace xercesc;

namespace
{

NetworkSource ParseNetSourceEnum(const XMLCh* srcX, NetworkSource* defValue)
{
	if (srcX)
	{
		std::string src = TranscodeString(srcX);
		if (src == "xml")
		{
			return NETSRC_XML;
		}
		else if (src == "database")
		{
			return NETSRC_DATABASE;
		}
		throw std::runtime_error("Expected SystemParams::NetworkSource value.");
	}

	///Wasn't found.
	if (!defValue)
	{
		throw std::runtime_error("Mandatory SystemParams::NetworkSource variable; no default available.");
	}
	return *defValue;
}

AuraManager::AuraManagerImplementation ParseAuraMgrImplEnum(const XMLCh* srcX, AuraManager::AuraManagerImplementation* defValue)
{
	if (srcX)
	{
		std::string src = TranscodeString(srcX);
		if (src == "rdu")
		{
			return AuraManager::IMPL_RDU;
		}
		else if (src == "rstar")
		{
			return AuraManager::IMPL_RSTAR;
		}
		else if(src == "simtree")
		{
			return AuraManager::IMPL_SIMTREE;
		}
		else if(src == "packing-tree")
		{
			return AuraManager::IMPL_PACKING;
		}
		throw std::runtime_error("Expected AuraManager::AuraManagerImplementation value.");
	}

	///Wasn't found.
	if (!defValue)
	{
		throw std::runtime_error("Mandatory AuraManager::AuraManagerImplementation variable; no default available.");
	}
	return *defValue;
}

Point ParsePoint(const XMLCh* srcX, Point* defValue)
{
	if (srcX)
	{
		std::string src = TranscodeString(srcX);
		return parse_point(src);
	}

	///Wasn't found.
	if (!defValue)
	{
		throw std::runtime_error("Mandatory Point variable; no default available.");
	}
	return *defValue;
}

AuraManager::AuraManagerImplementation ParseAuraMgrImplEnum(const XMLCh* srcX, AuraManager::AuraManagerImplementation defValue)
{
	return ParseAuraMgrImplEnum(srcX, &defValue);
}

AuraManager::AuraManagerImplementation ParseAuraMgrImplEnum(const XMLCh* srcX)
{
	return ParseAuraMgrImplEnum(srcX, nullptr);
}

void splitRoleString(std::string& roleString, std::vector<std::string>& roles)
{
	std::string delimiter = "|";
	size_t pos = 0;
	std::string token;
	while ((pos = roleString.find(delimiter)) != std::string::npos)
	{
		token = roleString.substr(0, pos);
		roles.push_back(token);
		roleString.erase(0, pos + delimiter.length());
	}
}

unsigned int ParseGranularitySingle(const XMLCh* srcX, unsigned int* defValue)
{
	if (srcX)
	{
		///Search for "[0-9]+ ?[^0-9]+), roughly.
		std::string src = TranscodeString(srcX);
		size_t digStart = src.find_first_of("1234567890");
		size_t digEnd = src.find_first_not_of("1234567890", digStart + 1);
		size_t unitStart = src.find_first_not_of(" ", digEnd);
		if (digStart != 0 || digStart == std::string::npos || digEnd == std::string::npos || unitStart == std::string::npos)
		{
			throw std::runtime_error("Badly formatted single-granularity string.");
		}

		///Now split/parse it.
		double value = boost::lexical_cast<double>(src.substr(digStart, (digEnd - digStart)));
		std::string units = src.substr(unitStart, std::string::npos);

		return GetValueInMs(value, units, defValue);
	}

	///Wasn't found.
	if (!defValue)
	{
		throw std::runtime_error("Mandatory integer (granularity) variable; no default available.");
	}
	return *defValue;
}

unsigned int ParseGranularitySingle(const XMLCh* src, unsigned int defValue)
{
	return ParseGranularitySingle(src, &defValue);
}

unsigned int ParseGranularitySingle(const XMLCh* src)
{ 
	//No default
	return ParseGranularitySingle(src, nullptr);
}

NetworkSource ParseNetSourceEnum(const XMLCh* srcX, NetworkSource defValue)
{
	return ParseNetSourceEnum(srcX, &defValue);
}

NetworkSource ParseNetSourceEnum(const XMLCh* srcX)
{
	return ParseNetSourceEnum(srcX, nullptr);
}

///How to do defaults
Point ParsePoint(const XMLCh* src, Point defValue)
{
	return ParsePoint(src, &defValue);
}

Point ParsePoint(const XMLCh* src)
{ 
	///No default
	return ParsePoint(src, nullptr);
}

}

namespace sim_mob
{

ParseShortTermConfigFile::ParseShortTermConfigFile(const std::string &configFileName, ConfigParams& sharedCfg, ST_Config &result) :
ParseConfigXmlBase(configFileName), cfg(sharedCfg), stCfg(result)
{
	parseXmlAndProcess();
}

void ParseShortTermConfigFile::processXmlFile(XercesDOMParser& parser)
{
	DOMElement* rootNode = parser.getDocument()->getDocumentElement();
	///Verify that the root node is "config"
	if (TranscodeString(rootNode->getTagName()) != "config")
	{
		throw std::runtime_error("xml parse error: root node must be \"config\"");
	}

	processProcMapNode(GetSingleElementByName(rootNode, "db_proc_groups", true));
	processSystemNode(GetSingleElementByName(rootNode, "system", true));
	cfg.withindayLuaScriptsMap = processModelScriptsNode(GetSingleElementByName(rootNode, "model_scripts", true));
	processWorkersNode(GetSingleElementByName(rootNode, "workers", true));
	processAmodControllerNode(GetSingleElementByName(rootNode, "amodcontroller"));
	processFmodControllerNode(GetSingleElementByName(rootNode, "fmodcontroller"));
	processVehicleTypesNode(GetSingleElementByName(rootNode, "vehicleTypes"));
	processTripFilesNode(GetSingleElementByName(rootNode, "tripFiles"));
	processPersonCharacteristicsNode(GetSingleElementByName(rootNode, "person_characteristics"));
	processBusControllerNode(GetSingleElementByName(rootNode, "busController"));
	processBusCapacityNode(GetSingleElementByName(rootNode, "bus_default_capacity"));
	processPublicTransit(GetSingleElementByName(rootNode, "public_transit", true));
	processOutputStatistics(GetSingleElementByName(rootNode, "output_statistics", true));
	processPathSetFileName(GetSingleElementByName(rootNode, "path-set-config-file"));
	processTT_Update(GetSingleElementByName(rootNode, "travel_time_update", true));	
	
	//Take care of path-set manager configuration in here
    ParsePathXmlConfig(cfg.pathsetFile, cfg.getPathSetConf());
}

void ParseShortTermConfigFile::processProcMapNode(DOMElement* node)
{
	for (DOMElement* item = node->getFirstElementChild(); item; item = item->getNextElementSibling())
	{
		if (TranscodeString(item->getNodeName()) != "proc_map")
		{
			Warn() << "Invalid db_proc_groups child node.\n";
			continue;
		}

		///Retrieve some attributes from the Node itself.
		StoredProcedureMap pm(ParseString(GetNamedAttributeValue(item, "id")));
		pm.dbFormat = ParseString(GetNamedAttributeValue(item, "format"), "");
		if (pm.dbFormat != "aimsun" && pm.dbFormat != "long-term")
		{
			throw std::runtime_error("Stored procedure map format not supported.");
		}

		///Loop through and save child attributes.
		for (DOMElement* mapItem = item->getFirstElementChild(); mapItem; mapItem = mapItem->getNextElementSibling())
		{
			if (TranscodeString(mapItem->getNodeName()) != "mapping")
			{
				Warn() << "Invalid proc_map child node.\n";
				continue;
			}

			std::string key = ParseString(GetNamedAttributeValue(mapItem, "name"), "");
			std::string val = ParseString(GetNamedAttributeValue(mapItem, "procedure"), "");
			if (key.empty() || val.empty())
			{
				Warn() << "Invalid mapping; missing \"name\" or \"procedure\".\n";
				continue;
			}

			pm.procedureMappings[key] = val;
		}

		cfg.procedureMaps[pm.getId()] = pm;
	}
}

void ParseShortTermConfigFile::processAmodControllerNode(DOMElement* node)
{
	if (node)
	{
		///Read the attribute value indicating whether AMOD is enabled or disabled
		stCfg.amod.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), false);
		stCfg.amod.fileName = ParseString(GetNamedAttributeValue(node, "config_file"), "amod_config.xml");
	}
}

class FmodParser : public ParseConfigXmlBase
{
public:

	FmodParser(std::map<std::string, TripChainItem*>& items, const std::string& configFileName) : allItems(items), ParseConfigXmlBase(configFileName)
	{
		parseXmlAndProcess();
	}

	virtual void processXmlFile(xercesc::XercesDOMParser& parser)
	{
		DOMElement* rootNode = parser.getDocument()->getDocumentElement();
		
		if (TranscodeString(rootNode->getTagName()) != "requests")
		{
			throw std::runtime_error("xml parse error: root node must be \"requests\"");
		}
		for (DOMElement* item = rootNode->getFirstElementChild(); item; item = item->getNextElementSibling())
		{
			if (TranscodeString(item->getNodeName()) == "request")
			{
				TripChainItem* trip = new Trip();
				trip->startTime = DailyTime(ParseString(GetNamedAttributeValue(item, "startTime"), ""));
				trip->endTime = DailyTime(ParseString(GetNamedAttributeValue(item, "endTime"), ""));
				trip->requestTime = ParseUnsignedInt(GetNamedAttributeValue(item, "timeWin"));
				trip->sequenceNumber = ParseUnsignedInt(GetNamedAttributeValue(item, "frequency"));
				trip->startLocationId = ParseString(GetNamedAttributeValue(item, "originNode"), "");
				trip->endLocationId = ParseString(GetNamedAttributeValue(item, "destNode"), "");
				std::string startId = ParseString(GetNamedAttributeValue(item, "startId"), "");
				ConfigParams& cfg = ConfigManager::GetInstanceRW().FullConfig();
				DailyTime simStart = cfg.simulation.simStartTime;
				DailyTime simEnd = DailyTime(simStart.getValue() + cfg.simulation.totalRuntimeMS);
				DailyTime tripStart(trip->startTime);
				
				if (tripStart.getValue() > simStart.getValue() && tripStart.getValue() < simEnd.getValue())
				{
					allItems[startId] = trip;
				}
				else
				{
					delete trip;
				}
			}
		}
	}
public:
	std::map<std::string, TripChainItem*>& allItems;
};

void sim_mob::ParseShortTermConfigFile::processFmodControllerNode(xercesc::DOMElement* node)
{
	if (!node)
	{
		return;
	}

	//The fmod tag has an attribute
	stCfg.fmod.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), false);
	if (stCfg.fmod.enabled)
	{
		//Now set the rest.
		stCfg.fmod.ipAddress = ParseString(GetNamedAttributeValue(GetSingleElementByName(node, "ip_address"), "value"), "");
		stCfg.fmod.port = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "port"), "value"), static_cast<unsigned int> (0));
		stCfg.fmod.updateTimeMS = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "update_time_ms"), "value"), static_cast<unsigned int> (0));
		stCfg.fmod.mapfile = ParseString(GetNamedAttributeValue(GetSingleElementByName(node, "map_file"), "value"), "");
		stCfg.fmod.blockingTimeSec = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "blocking_time_sec"), "value"), static_cast<unsigned int> (0));
		xercesc::DOMElement* resNodes = GetSingleElementByName(node, "requests");
		
		if (resNodes)
		{
			for (DOMElement* item = resNodes->getFirstElementChild(); item; item = item->getNextElementSibling())
			{
				if (TranscodeString(item->getNodeName()) == "request")
				{
					TripChainItem* trip = new Trip();
					trip->startTime = DailyTime(ParseString(GetNamedAttributeValue(item, "startTime"), ""));
					trip->endTime = DailyTime(ParseString(GetNamedAttributeValue(item, "endTime"), ""));
					trip->requestTime = ParseUnsignedInt(GetNamedAttributeValue(item, "timeWin"));
					trip->sequenceNumber = ParseUnsignedInt(GetNamedAttributeValue(item, "frequency"));
					trip->startLocationId = ParseString(GetNamedAttributeValue(item, "originNode"), "");
					trip->endLocationId = ParseString(GetNamedAttributeValue(item, "destNode"), "");
					std::string startId = ParseString(GetNamedAttributeValue(item, "startId"), "");
					stCfg.fmod.allItems[startId] = trip;
				}
			}
		}
		DOMElement* element = GetSingleElementByName(node, "requests_file");
		if (element)
		{
			std::string filename = ParseString(GetNamedAttributeValue(element, "value"), "");
			FmodParser fmodParser(stCfg.fmod.allItems, filename);
		}
	}
}

void ParseShortTermConfigFile::processSegmentDensityNode(DOMElement* node)
{
	if (node)
	{
		stCfg.outputStats.segDensityMap.outputEnabled = ParseBoolean(GetNamedAttributeValue(node, "outputEnabled"), "false");
		if (stCfg.outputStats.segDensityMap.outputEnabled)
		{
			stCfg.outputStats.segDensityMap.updateInterval = ParseUnsignedInt(GetNamedAttributeValue(node, "updateInterval"), 1000);
			stCfg.outputStats.segDensityMap.fileName = ParseString(GetNamedAttributeValue(node, "file-name"), "private/DensityMap.csv");

			if (stCfg.outputStats.segDensityMap.updateInterval == 0)
			{
				throw std::runtime_error("ParseConfigFile::ProcessShortDensityMapNode - Update interval for aggregating density is 0");
			}

			if (stCfg.outputStats.segDensityMap.fileName.empty())
			{
				throw std::runtime_error("ParseConfigFile::ProcessShortDensityMapNode - File name is empty");
			}
		}
	}
}

void ParseShortTermConfigFile::processSystemNode(DOMElement *node)
{
	if (node)
	{
		processNetworkNode(GetSingleElementByName(node, "network", true));
		processAuraManagerImpNode(GetSingleElementByName(node, "aura_manager_impl", true));
		processLoadAgentsOrder(GetSingleElementByName(node, "load_agents", true));
		processCommSimNode(GetSingleElementByName(node, "commsim"));
		processXmlSchemaFilesNode(GetSingleElementByName(node, "xsd_schema_files"));
		processGenericPropsNode(GetSingleElementByName(node, "generic_props"));
	}
	else
	{
		throw std::runtime_error("processSystemNode : System node not defined");
	}
}

ModelScriptsMap ParseShortTermConfigFile::processModelScriptsNode(xercesc::DOMElement* node)
{
	std::string format = ParseString(GetNamedAttributeValue(node, "format"), "");
	if (format.empty() || format != "lua")
	{
		throw std::runtime_error("Unsupported script format");
	}

	std::string scriptsDirectoryPath = ParseString(GetNamedAttributeValue(node, "path"), "");
	if (scriptsDirectoryPath.empty())
	{
		throw std::runtime_error("path to scripts is not provided");
	}
	if ((*scriptsDirectoryPath.rbegin()) != '/')
	{
		//add a / to the end of the path string if it is not already there
		scriptsDirectoryPath.push_back('/');
	}
	ModelScriptsMap scriptsMap(scriptsDirectoryPath, format);
	for (DOMElement* item = node->getFirstElementChild(); item; item = item->getNextElementSibling())
	{
		std::string name = TranscodeString(item->getNodeName());
		if (name != "script")
		{
			Warn() << "Invalid db_proc_groups child node.\n";
			continue;
		}

		std::string key = ParseString(GetNamedAttributeValue(item, "name"), "");
		std::string val = ParseString(GetNamedAttributeValue(item, "file"), "");
		if (key.empty() || val.empty())
		{
			Warn() << "Invalid script; missing \"name\" or \"file\".\n";
			continue;
		}

		scriptsMap.addScriptFileName(key, val);
	}
	return scriptsMap;
}

void ParseShortTermConfigFile::processNetworkNode(DOMElement *node)
{
	if (node)
	{
		processNetworkSourceNode(GetSingleElementByName(node, "network_source", true));
		processDatabaseNode(GetSingleElementByName(node, "network_database", true));
		processNetworkXmlInputNode(GetSingleElementByName(node, "network_xml_file_input", true));
		processNetworkXmlOutputNode(GetSingleElementByName(node, "network_xml_file_output", true));
	}
	else
	{
		throw std::runtime_error("processNetworkNode : network node not defined");
	}
}

void ParseShortTermConfigFile::processAuraManagerImpNode(DOMElement *node)
{
	stCfg.auraManagerImplementation = ParseAuraMgrImplEnum(GetNamedAttributeValue(node, "value"), AuraManager::IMPL_RSTAR);
}

void ParseShortTermConfigFile::processLoadAgentsOrder(DOMElement *node)
{
	///Separate into a string array.
	std::string value = ParseString(GetNamedAttributeValue(node, "order"), "");
	std::vector<std::string> valArray;
	boost::split(valArray, value, boost::is_any_of(", "), boost::token_compress_on);

	///Now, turn into an enum array.
	for (std::vector<std::string>::const_iterator it = valArray.begin(); it != valArray.end(); ++it)
	{
		LoadAgentsOrderOption opt(LoadAg_Database);
		if ((*it) == "database")
		{
			opt = LoadAg_Database;
		}
		else if ((*it) == "drivers")
		{
			opt = LoadAg_Drivers;
		}
		else if ((*it) == "pedestrians")
		{
			opt = LoadAg_Pedestrians;
		}
		else if ((*it) == "passengers")
		{
			opt = LoadAg_Passengers;
		}
		else
		{
			std::stringstream out;
			out.str("");
			out << "Unexpected load_agents order param." << "[" << *it << "]";
			throw std::runtime_error(out.str());
		}
		stCfg.loadAgentsOrder.push_back(opt);
	}
}

int ParseShortTermConfigFile::processValueInteger(xercesc::DOMElement* node)
{
	return ParseInteger(GetNamedAttributeValue(node, "value"));
}

bool ParseShortTermConfigFile::processValueBoolean(xercesc::DOMElement* node)
{
	return ParseBoolean(GetNamedAttributeValue(node, "value"));
}

void ParseShortTermConfigFile::processCommSimNode(DOMElement *node)
{
	if (!node)
	{
		return;
	}

	///Enabled?
	stCfg.commsim.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), false);

	///Number of threads assigned to the boost I/O service that reads from Android clients.
	stCfg.commsim.numIoThreads = processValueInteger(GetSingleElementByName(node, "io_threads", true));

	///Minimum clients
	stCfg.commsim.minClients = processValueInteger(GetSingleElementByName(node, "min_clients", true));

	///Hold tick
	stCfg.commsim.holdTick = processValueInteger(GetSingleElementByName(node, "hold_tick", true));

	///Use ns-3 for routing?
	stCfg.commsim.useNs3 = processValueBoolean(GetSingleElementByName(node, "use_ns3", true));
}

void ParseShortTermConfigFile::processLoopDetectorCountNode(DOMElement *node)
{
	if (node)
	{
		stCfg.outputStats.loopDetectorCounts.outputEnabled = ParseBoolean(GetNamedAttributeValue(node, "outputEnabled"), "false");
		if (stCfg.outputStats.loopDetectorCounts.outputEnabled)
		{
			stCfg.outputStats.loopDetectorCounts.frequency = ParseUnsignedInt(GetNamedAttributeValue(node, "frequency"), 600000);
			stCfg.outputStats.loopDetectorCounts.fileName = ParseString(GetNamedAttributeValue(node, "file-name"), "private/VehCounts.csv");

			if (stCfg.outputStats.loopDetectorCounts.frequency == 0)
			{
				throw std::runtime_error("ParseConfigFile::ProcessLoopDetectorCountsNode - "
										 "Update frequency for aggregating vehicle counts is 0");
			}

			if (stCfg.outputStats.loopDetectorCounts.fileName.empty())
			{
				throw std::runtime_error("ParseConfigFile::ProcessLoopDetectorCountsNode - File name is empty");
			}
		}
	}
}

void ParseShortTermConfigFile::processXmlSchemaFilesNode(DOMElement *node)
{
	///For now, only the Road Network has an XSD file (doing this for the config file from within it would be difficult).
	DOMElement* rn = GetSingleElementByName(node, "road_network");
	if (rn)
	{
		std::vector<DOMElement*> options = GetElementsByName(rn, "option");
		for (std::vector<DOMElement*>::const_iterator it = options.begin(); it != options.end(); ++it)
		{
			std::string path = ParseString(GetNamedAttributeValue(*it, "value"), "");
			if (!path.empty())
			{
				///See if the file exists.
				if (boost::filesystem::exists(path))
				{
					///Convert it to an absolute path.
					boost::filesystem::path abs_path = boost::filesystem::absolute(path);
					stCfg.getRoadNetworkXsdSchemaFile() = abs_path.string();
					break;
				}
			}
		}

		///Did we try and find nothing?
		if (!options.empty() && stCfg.roadNetworkXsdSchemaFile.empty())
		{
			Warn() << "Warning: No viable options for road_network schema file." << std::endl;
		}
	}
}

void ParseShortTermConfigFile::processNetworkXmlOutputNode(DOMElement *node)
{
	stCfg.getNetworkXmlOutputFile() = ParseNonemptyString(GetNamedAttributeValue(node, "value"), "");
}

void ParseShortTermConfigFile::processNetworkXmlInputNode(DOMElement *node)
{
	stCfg.getNetworkXmlInputFile() = ParseNonemptyString(GetNamedAttributeValue(node, "value"), "private/SimMobilityInput.xml");
}

void ParseShortTermConfigFile::processNetworkSourceNode(DOMElement *node)
{
	stCfg.networkSource = ParseNetSourceEnum(GetNamedAttributeValue(node, "value"), NETSRC_XML);
}

void ParseShortTermConfigFile::processDatabaseNode(DOMElement *node)
{
	if (node)
	{
		cfg.networkDatabase.database = ParseString(GetNamedAttributeValue(node, "database"), "");
		cfg.networkDatabase.credentials = ParseString(GetNamedAttributeValue(node, "credentials"), "");
		cfg.networkDatabase.procedures = ParseString(GetNamedAttributeValue(node, "proc_map"), "");
	}
	else
	{
		throw std::runtime_error("processDatabaseNode : Network database configuration not defined");
	}
}

void ParseShortTermConfigFile::processWorkersNode(xercesc::DOMElement* node)
{
	if (node)
	{
		processWorkerPersonNode(GetSingleElementByName(node, "person", true));
		processWorkerSignalNode(GetSingleElementByName(node, "signal", true));
		processWorkerIntMgrNode(GetSingleElementByName(node, "intersection_manager", true));
		processWorkerCommunicationNode(GetSingleElementByName(node, "communication", true));
	}
	else
	{
		throw std::runtime_error("processWorkerParamsNode : Workers configuration not defined");
	}
}

void ParseShortTermConfigFile::processWorkerPersonNode(xercesc::DOMElement* node)
{
	if (node)
	{
		stCfg.workers.person.count = ParseInteger(GetNamedAttributeValue(node, "count"));
		stCfg.workers.person.granularityMs = ParseGranularitySingle(GetNamedAttributeValue(node, "granularity"));
	}
}

void ParseShortTermConfigFile::processWorkerSignalNode(xercesc::DOMElement* node)
{
	if (node)
	{
		stCfg.workers.signal.count = ParseInteger(GetNamedAttributeValue(node, "count"));
		stCfg.workers.signal.granularityMs = ParseGranularitySingle(GetNamedAttributeValue(node, "granularity"));
	}
}

void ParseShortTermConfigFile::processWorkerIntMgrNode(xercesc::DOMElement* node)
{
	if (node)
	{
		stCfg.workers.intersectionMgr.count = ParseInteger(GetNamedAttributeValue(node, "count"));
		stCfg.workers.intersectionMgr.granularityMs = ParseGranularitySingle(GetNamedAttributeValue(node, "granularity"));
	}
}

void ParseShortTermConfigFile::processWorkerCommunicationNode(xercesc::DOMElement* node)
{
	if (node)
	{
		stCfg.workers.communication.count = ParseInteger(GetNamedAttributeValue(node, "count"));
		stCfg.workers.communication.granularityMs = ParseGranularitySingle(GetNamedAttributeValue(node, "granularity"));
	}
}

void ParseShortTermConfigFile::processPersonCharacteristicsNode(DOMElement *node)
{
	if (!node)
	{
		return;
	}

	///Loop through all children
	int count = 0;
	for (DOMElement* item = node->getFirstElementChild(); item; item = item->getNextElementSibling())
	{
		if (TranscodeString(item->getNodeName()) != "person")
		{
			Warn() << "Invalid personCharacteristics child node.\n";
			continue;
		}

		///Retrieve properties, add a new item to the vector.
		PersonCharacteristics res;
		res.lowerAge = ParseUnsignedInt(GetNamedAttributeValue(item, "lowerAge"), static_cast<unsigned int> (0));
		res.upperAge = ParseUnsignedInt(GetNamedAttributeValue(item, "upperAge"), static_cast<unsigned int> (0));
		res.lowerSecs = ParseInteger(GetNamedAttributeValue(item, "lowerSecs"), static_cast<int> (0));
		res.upperSecs = ParseInteger(GetNamedAttributeValue(item, "upperSecs"), static_cast<int> (0));
		res.walkSpeed = ParseFloat(GetNamedAttributeValue(item, "walkSpeed_kmph"), static_cast<float> (0));
		
		//Convert walking speed to m/s (from km/h)
		res.walkSpeed *= 0.277778;
		
		cfg.personCharacteristicsParams.personCharacteristics[count++] = res;
	}

	std::map<int, PersonCharacteristics> personCharacteristics = cfg.personCharacteristicsParams.personCharacteristics;
	
	/// calculate lowest age and highest age in the ranges
	for (std::map<int, PersonCharacteristics>::const_iterator iter = personCharacteristics.begin(); iter != personCharacteristics.end(); ++iter)
	{
		if (cfg.personCharacteristicsParams.lowestAge > iter->second.lowerAge)
		{
			cfg.personCharacteristicsParams.lowestAge = iter->second.lowerAge;
		}
		if (cfg.personCharacteristicsParams.highestAge < iter->second.upperAge)
		{
			cfg.personCharacteristicsParams.highestAge = iter->second.upperAge;
		}
	}
}

void ParseShortTermConfigFile::processGenericPropsNode(DOMElement *node)
{
	if (!node)
	{
		return;
	}

	std::vector<DOMElement*> properties = GetElementsByName(node, "property");
	for (std::vector<DOMElement*>::const_iterator it = properties.begin(); it != properties.end(); ++it)
	{
		std::string key = ParseString(GetNamedAttributeValue(*it, "key"), "");
		std::string val = ParseString(GetNamedAttributeValue(*it, "value"), "");
		if (!(key.empty() && val.empty()))
		{
			stCfg.genericProps[key] = val;
		}
	}
}

void ParseShortTermConfigFile::processVehicleTypesNode(DOMElement *node)
{
	if (node)
	{
		std::vector<DOMElement*> vehicles = GetElementsByName(node, "vehicleType");
		for (std::vector<DOMElement*>::const_iterator it = vehicles.begin(); it != vehicles.end(); it++)
		{
			VehicleType vehicleType;
			vehicleType.name = ParseString(GetNamedAttributeValue(*it, "name", ""));
			if (vehicleType.name.empty())
			{
				throw std::runtime_error("ProcessVehicleTypesNode : Vehicle name cannot be empty");
			}

			vehicleType.length = ParseFloat(GetNamedAttributeValue(*it, "length"), 4.0);
			vehicleType.width = ParseFloat(GetNamedAttributeValue(*it, "width"), 2.0);
			vehicleType.capacity = ParseInteger(GetNamedAttributeValue(*it, "capacity"), 4);

			stCfg.vehicleTypes.push_back(vehicleType);
		}

		if (stCfg.vehicleTypes.empty())
		{
			throw std::runtime_error("ProcessVehicleTypesNode : No vehicle type is defined");
		}
	}
	else
	{
		throw std::runtime_error("ProcessVehicleTypeNode : VehicleTypes node not defined in the configuration");
	}
}

void ParseShortTermConfigFile::processTripFilesNode(DOMElement *node)
{
	if (node)
	{
		std::vector<DOMElement*> tripFiles = GetElementsByName(node, "tripFile");
		for (std::vector<DOMElement*>::const_iterator it = tripFiles.begin(); it != tripFiles.end(); it++)
		{
			std::string name = ParseString(GetNamedAttributeValue(*it, "name"), "");
			std::string file = ParseString(GetNamedAttributeValue(*it, "fileName"), "");
			if (!(name.empty() && file.empty()))
			{
				stCfg.tripFiles[name] = file;
			}
		}

		for (std::map<std::string, std::string>::const_iterator it = stCfg.tripFiles.begin(); it != stCfg.tripFiles.end(); it++)
		{
			ParseShortTermTripFile parse(it->second, it->first, stCfg);
		}
	}
}

void ParseShortTermConfigFile::processBusControllerNode(DOMElement *node)
{
	if (node)
	{
		cfg.busController.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), "false");
		cfg.busController.busLineControlType = ParseString(GetNamedAttributeValue(node, "busline_control_type"), "");
	}
}

void ParseShortTermConfigFile::processBusCapacityNode(xercesc::DOMElement* node)
{
	if(node)
	{
		stCfg.defaultBusCapacity = ParseUnsignedInt(GetNamedAttributeValue(node, "value"), 50);
	}
}

void ParseShortTermConfigFile::processPublicTransit(xercesc::DOMElement* node)
{
	if (!node)
	{
		cfg.setPublicTransitEnabled(false);
	}
	else
	{
		cfg.setPublicTransitEnabled(ParseBoolean(GetNamedAttributeValue(node, "enabled"), false));
		if (cfg.isPublicTransitEnabled())
		{
			const std::string& key = cfg.networkDatabase.procedures;
			std::map<std::string, StoredProcedureMap>::const_iterator procMapIt = cfg.procedureMaps.find(key);
			if (procMapIt->second.procedureMappings.count("pt_vertices") == 0 || procMapIt->second.procedureMappings.count("pt_edges") == 0)
			{
				throw std::runtime_error("Public transit is enabled , but stored procedures not defined");
			}
		}
	}
}

void ParseShortTermConfigFile::processOutputStatistics(xercesc::DOMElement* node)
{
	if(node)
	{
		processJourneyTimeNode(GetSingleElementByName(node, "journey_time"));
		processWaitingTimeNode(GetSingleElementByName(node, "waiting_time"));
		processWaitingCountsNode(GetSingleElementByName(node, "waiting_count"));
		processTravelTimeNode(GetSingleElementByName(node, "travel_time"));
		processPT_StopStatsNode(GetSingleElementByName(node, "pt_stop_stats"));
		processODTravelTimeNode(GetSingleElementByName(node, "od_travel_time"));
		processSegmentTravelTimeNode(GetSingleElementByName(node, "segment_travel_time"));
		processSegmentDensityNode(GetSingleElementByName(node, "segment_density"));
		processLoopDetectorCountNode(GetSingleElementByName(node, "loop-detector_counts"));
		processAssignmentMatrixNode(GetSingleElementByName(node, "assignment_matrix"));
	}
}

void ParseShortTermConfigFile::processPathSetFileName(DOMElement* node)
{
	if (!node)
	{
		return;
	}
	cfg.pathsetFile = ParseString(GetNamedAttributeValue(node, "value"));
}

void ParseShortTermConfigFile::processTT_Update(xercesc::DOMElement* node)
{
	if (!node)
	{
		throw std::runtime_error("Path-set travel_time_interval not found\n");
	}
	else
	{
		cfg.getPathSetConf().interval = ParseInteger(GetNamedAttributeValue(node, "interval"), 300);
	}
}

void ParseShortTermConfigFile::processJourneyTimeNode(xercesc::DOMElement* node)
{
	if(node)
	{		
		cfg.setJourneyTimeStatsFilename(ParseString(GetNamedAttributeValue(node, "file"), "journey_time.csv"));
	}
}

void ParseShortTermConfigFile::processWaitingTimeNode(xercesc::DOMElement* node)
{
	if(node)
	{
		cfg.setWaitingTimeStatsFilename(ParseString(GetNamedAttributeValue(node, "file"), "waiting_time.csv"));
	}
}

void ParseShortTermConfigFile::processWaitingCountsNode(xercesc::DOMElement* node)
{
	if(node)
	{
		cfg.setWaitingCountStatsFilename(ParseString(GetNamedAttributeValue(node, "file"), "waiting_count.csv"));
		cfg.setWaitingCountStatsInterval(ParseUnsignedInt(GetNamedAttributeValue(node, "interval"), 900000));
	}
}

void ParseShortTermConfigFile::processTravelTimeNode(xercesc::DOMElement* node)
{
	if(node)
	{
		cfg.setTravelTimeStatsFilename(ParseString(GetNamedAttributeValue(node, "file"), "travel_time.csv"));
	}
}

void ParseShortTermConfigFile::processPT_StopStatsNode(xercesc::DOMElement* node)
{
	if(node)
	{
		cfg.setPT_StopStatsFilename(ParseString(GetNamedAttributeValue(node, "file"), "pt_stop_stats.csv"));
	}
}

void ParseShortTermConfigFile::processAssignmentMatrixNode(xercesc::DOMElement* node)
{
	if(node)
	{
		bool enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"));
		if (enabled)
		{
			stCfg.outputStats.assignmentMatrix.enabled = true;
			stCfg.outputStats.assignmentMatrix.fileName = ParseString(GetNamedAttributeValue(node, "file-name"), "assignment_matrix.csv");
		}
	}
}

void ParseShortTermConfigFile::processODTravelTimeNode(xercesc::DOMElement* node)
{
	if(node)
	{
		bool enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"));
		if (enabled)
		{
			cfg.odTTConfig.enabled = true;
			cfg.odTTConfig.intervalMS = ParseInteger(GetNamedAttributeValue(node, "interval"), 300000);
			cfg.odTTConfig.fileName = ParseString(GetNamedAttributeValue(node, "file-name"), "od_travel_time.csv");
		}
	}
}

void ParseShortTermConfigFile::processSegmentTravelTimeNode(xercesc::DOMElement* node)
{
	if(node)
	{
		bool enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"));
		if (enabled)
		{
			cfg.rsTTConfig.enabled = true;
			cfg.rsTTConfig.intervalMS = ParseInteger(GetNamedAttributeValue(node, "interval"), 300000);
			cfg.rsTTConfig.fileName = ParseString(GetNamedAttributeValue(node, "file-name"), "segment_travel_time.csv");
		}
	}
}

ParseShortTermTripFile::ParseShortTermTripFile(const std::string &tripFileName, const std::string &tripName_, ST_Config &stConfig) :
ParseConfigXmlBase(tripFileName), cfg(stConfig), tripName(tripName_)
{
	parseXmlAndProcess();
}

void ParseShortTermTripFile::processXmlFile(XercesDOMParser &parser)
{
	DOMElement* rootNode = parser.getDocument()->getDocumentElement();
	///Verify that the root node is "config"
	if (TranscodeString(rootNode->getTagName()) != "trips")
	{
		throw std::runtime_error("xml parse error: root node must be \"trips\"");
	}

	processTrips(rootNode);
}

void ParseShortTermTripFile::processTrips(DOMElement *node)
{
	if (node)
	{
		typedef std::vector<DOMElement*> DOMList;
		typedef std::vector<DOMElement*>::const_iterator DOMListIter;

		DOMList trips = GetElementsByName(node, "trip");
		unsigned int defaultTripId = 1;
		
		for (DOMListIter it = trips.begin(); it != trips.end(); ++it, ++defaultTripId)
		{
			defaultTripId = ParseInteger(GetNamedAttributeValue(*it, "id"), defaultTripId);
			unsigned int personId = ParseUnsignedInt(GetNamedAttributeValue(*it, "personId", false), static_cast<unsigned int> (0));
			std::stringstream tripIdStr;
			tripIdStr << defaultTripId;
			
			DOMList subTrips = GetElementsByName(*it, "subTrip");
			unsigned int defaultSubTripId = 1;
			
			for (DOMListIter stIter = subTrips.begin(); stIter != subTrips.end(); ++stIter, ++defaultSubTripId)
			{				
				defaultSubTripId = ParseUnsignedInt(GetNamedAttributeValue(*stIter, "id", false), defaultSubTripId);
				EntityTemplate ent;
				
				ent.startTimeMs = ParseUnsignedInt(GetNamedAttributeValue(*stIter, "time", true), static_cast<unsigned int> (0));
				ent.startLaneIndex = ParseInteger(GetNamedAttributeValue(*stIter, "startLaneIndex"), -1);
				ent.agentId = personId;
				ent.startSegmentId = ParseUnsignedInt(GetNamedAttributeValue(*stIter, "startSegmentId", false), static_cast<unsigned int> (0));
				ent.segmentStartOffset = ParseUnsignedInt(GetNamedAttributeValue(*stIter, "segmentStartOffset", false), static_cast<unsigned int> (0));
				ent.initialSpeed = ParseUnsignedInt(GetNamedAttributeValue(*stIter, "initialSpeed", false), static_cast<double> (0));
				ent.originNode = ParseUnsignedInt(GetNamedAttributeValue(*stIter, "originNode", true), static_cast<double> (0));
				ent.destNode = ParseUnsignedInt(GetNamedAttributeValue(*stIter, "destNode", true), static_cast<double> (0));				
				ent.tripId = std::make_pair(defaultTripId, defaultSubTripId);
				ent.mode = ParseString(GetNamedAttributeValue(*stIter, "mode"), "");
				
				if(!ent.mode.empty())
				{
					//Check if travel mode is public transport
					if(ent.mode != "PT")
					{
						//Identify vehicle type from the mode of travel
						std::vector<VehicleType>::iterator vehTypeIter = std::find(cfg.vehicleTypes.begin(), cfg.vehicleTypes.end(), ent.mode);

						if (vehTypeIter == cfg.vehicleTypes.end())
						{
							std::stringstream msg;
							msg << "Travel mode '" << ent.mode << "' specifed in file '" << inFilePath
									<< "' for trip '" << defaultTripId << "' is not defined";
							throw std::runtime_error(msg.str());
						}
					}
				}
				else
				{
					std::stringstream msg;
					msg << "Travel mode not specifed in file '" << inFilePath << "' for trip '" << defaultTripId << "'";
					throw std::runtime_error(msg.str());
				}
				
				cfg.futureAgents[tripIdStr.str()].push_back(ent);
			}
		}
	}
}

}
