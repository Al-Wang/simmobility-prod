//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "ParseConfigFile.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <xercesc/dom/DOM.hpp>

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "conf/RawConfigParams.hpp"
#include "geospatial/Point2D.hpp"
#include "util/GeomHelpers.hpp"
#include "util/XmlParseHelper.hpp"
#include "path/ParsePathXmlConfig.hpp"

namespace {
SystemParams::NetworkSource ParseNetSourceEnum(const XMLCh* srcX, SystemParams::NetworkSource* defValue) {
	if (srcX) {
		std::string src = TranscodeString(srcX);
		if (src=="xml") {
			return SystemParams::NETSRC_XML;
		} else if (src=="database") {
			return SystemParams::NETSRC_DATABASE;
		}
		throw std::runtime_error("Expected SystemParams::NetworkSource value.");
	}

	//Wasn't found.
	if (!defValue) {
		throw std::runtime_error("Mandatory SystemParams::NetworkSource variable; no default available.");
	}
	return *defValue;
}

AuraManager::AuraManagerImplementation ParseAuraMgrImplEnum(const XMLCh* srcX, AuraManager::AuraManagerImplementation* defValue) {
	if (srcX) {
		std::string src = TranscodeString(srcX);
		if (src=="simtree") {
			return AuraManager::IMPL_SIMTREE;
		} else if (src=="rdu") {
			return AuraManager::IMPL_RDU;
		} else if (src=="rstar") {
			return AuraManager::IMPL_RSTAR;
		}
		throw std::runtime_error("Expected AuraManager::AuraManagerImplementation value.");
	}

	//Wasn't found.
	if (!defValue) {
		throw std::runtime_error("Mandatory AuraManager::AuraManagerImplementation variable; no default available.");
	}
	return *defValue;
}

WorkGroup::ASSIGNMENT_STRATEGY ParseWrkGrpAssignEnum(const XMLCh* srcX, WorkGroup::ASSIGNMENT_STRATEGY* defValue) {
	if (srcX) {
		std::string src = TranscodeString(srcX);
		if (src=="roundrobin") {
			return WorkGroup::ASSIGN_ROUNDROBIN;
		} else if (src=="smallest") {
			return WorkGroup::ASSIGN_SMALLEST;
		}
		throw std::runtime_error("Expected WorkGroup::ASSIGNMENT_STRATEGY value.");
	}

	//Wasn't found.
	if (!defValue) {
		throw std::runtime_error("Mandatory WorkGroup::ASSIGNMENT_STRATEGY variable; no default available.");
	}
	return *defValue;
}

MutexStrategy ParseMutexStrategyEnum(const XMLCh* srcX, MutexStrategy* defValue) {
	if (srcX) {
		std::string src = TranscodeString(srcX);
		if (src=="buffered") {
			return MtxStrat_Buffered;
		} else if (src=="locked") {
			return MtxStrat_Locked;
		}
		throw std::runtime_error("Expected sim_mob::MutexStrategy value.");
	}

	//Wasn't found.
	if (!defValue) {
		throw std::runtime_error("Mandatory sim_mob::MutexStrategy variable; no default available.");
	}
	return *defValue;
}

Point2D ParsePoint2D(const XMLCh* srcX, Point2D* defValue) {
	if (srcX) {
		std::string src = TranscodeString(srcX);
		return parse_point(src);
	}

	//Wasn't found.
	if (!defValue) {
		throw std::runtime_error("Mandatory Point2D variable; no default available.");
	}
	return *defValue;
}

unsigned int ParseGranularitySingle(const XMLCh* srcX, unsigned int* defValue) {
	if (srcX) {
		//Search for "[0-9]+ ?[^0-9]+), roughly.
		std::string src = TranscodeString(srcX);
		size_t digStart = src.find_first_of("1234567890");
		size_t digEnd = src.find_first_not_of("1234567890", digStart+1);
		size_t unitStart = src.find_first_not_of(" ", digEnd);
		if (digStart!=0 || digStart==std::string::npos || digEnd==std::string::npos || unitStart==std::string::npos) {
			throw std::runtime_error("Badly formatted single-granularity string.");
		}

		//Now split/parse it.
		double value = boost::lexical_cast<double>(src.substr(digStart, (digEnd-digStart)));
		std::string units = src.substr(unitStart, std::string::npos);

		return GetValueInMs(value, units, defValue);
	}

	//Wasn't found.
	if (!defValue) {
		throw std::runtime_error("Mandatory integer (granularity) variable; no default available.");
	}
	return *defValue;
}

//How to do defaults
Point2D ParsePoint2D(const XMLCh* src, Point2D defValue) {
	return ParsePoint2D(src, &defValue);
}
Point2D ParsePoint2D(const XMLCh* src) { //No default
	return ParsePoint2D(src, nullptr);
}
unsigned int ParseGranularitySingle(const XMLCh* src, unsigned int defValue) {
	return ParseGranularitySingle(src, &defValue);
}
unsigned int ParseGranularitySingle(const XMLCh* src) { //No default
	return ParseGranularitySingle(src, nullptr);
}
SystemParams::NetworkSource ParseNetSourceEnum(const XMLCh* srcX, SystemParams::NetworkSource defValue) {
	return ParseNetSourceEnum(srcX, &defValue);
}
SystemParams::NetworkSource ParseNetSourceEnum(const XMLCh* srcX) {
	return ParseNetSourceEnum(srcX, nullptr);
}
AuraManager::AuraManagerImplementation ParseAuraMgrImplEnum(const XMLCh* srcX, AuraManager::AuraManagerImplementation defValue) {
	return ParseAuraMgrImplEnum(srcX, &defValue);
}
AuraManager::AuraManagerImplementation ParseAuraMgrImplEnum(const XMLCh* srcX) {
	return ParseAuraMgrImplEnum(srcX, nullptr);
}
WorkGroup::ASSIGNMENT_STRATEGY ParseWrkGrpAssignEnum(const XMLCh* srcX, WorkGroup::ASSIGNMENT_STRATEGY defValue) {
	return ParseWrkGrpAssignEnum(srcX, &defValue);
}
WorkGroup::ASSIGNMENT_STRATEGY ParseWrkGrpAssignEnum(const XMLCh* srcX) {
	return ParseWrkGrpAssignEnum(srcX, nullptr);
}
MutexStrategy ParseMutexStrategyEnum(const XMLCh* srcX, MutexStrategy defValue) {
	return ParseMutexStrategyEnum(srcX, &defValue);
}
MutexStrategy ParseMutexStrategyEnum(const XMLCh* srcX) {
	return ParseMutexStrategyEnum(srcX, nullptr);
}

const double MILLISECONDS_IN_SECOND = 1000.0;
} //End un-named namespace


sim_mob::ParseConfigFile::ParseConfigFile(const std::string& configFileName, RawConfigParams& result, bool longTerm) : cfg(result), ParseConfigXmlBase(configFileName), longTerm(longTerm)
{
	parseXmlAndProcess();
}

void sim_mob::ParseConfigFile::processXmlFile(XercesDOMParser& parser)
{
	//Verify that the root node is "config"
	DOMElement* rootNode = parser.getDocument()->getDocumentElement();
	if (TranscodeString(rootNode->getTagName()) != "config")
	{
		throw std::runtime_error("xml parse error: root node must be \"config\"");
	}

	//Make sure we don't have a geometry node.
	DOMElement* geom = GetSingleElementByName(rootNode, "geometry");
	if (geom)
	{
		throw std::runtime_error("Config file contains a <geometry> node, which is no longer allowed. See the <constructs> node for documentation.");
	}

	if( longTerm )
	{
		ProcessConstructsNode(GetSingleElementByName(rootNode,"constructs"));
		ProcessLongTermParamsNode( GetSingleElementByName(rootNode, "longTermParams"));
		ProcessModelScriptsNode(GetSingleElementByName(rootNode, "model_scripts"));
		return;
	}

	//Now just parse the document recursively.
	ProcessSystemNode(GetSingleElementByName(rootNode,"system", true));
	//ProcessGeometryNode(GetSingleElementByName(rootNode, "geometry", true));
	ProcessFMOD_Node(GetSingleElementByName(rootNode, "fmodcontroller"));
	ProcessAMOD_Node(GetSingleElementByName(rootNode, "amodcontroller"));
	ProcessIncidentsNode(GetSingleElementByName(rootNode, "incidentsData"));
	ProcessConstructsNode(GetSingleElementByName(rootNode,"constructs"));
	ProcessBusStopScheduledTimesNode(GetSingleElementByName(rootNode, "scheduledTimes"));
	ProcessPersonCharacteristicsNode(GetSingleElementByName(rootNode, "personCharacteristics"));


	//Agents all follow a template.

	ProcessPedestriansNode(GetSingleElementByName(rootNode, "pedestrians"));
	ProcessDriversNode(GetSingleElementByName(rootNode, "drivers"));
	ProcessTaxiDriversNode(GetSingleElementByName(rootNode, "taxidrivers"));
	ProcessBusDriversNode(GetSingleElementByName(rootNode, "busdrivers"));
	ProcessPassengersNode(GetSingleElementByName(rootNode, "passengers"));
	ProcessSignalsNode(GetSingleElementByName(rootNode, "signals"));
	ProcessBusControllersNode(GetSingleElementByName(rootNode, "buscontrollers"));
	ProcessCBD_Node(GetSingleElementByName(rootNode, "CBD"));	
	processPathSetFileName(GetSingleElementByName(rootNode, "path-set-config-file"));
	processTT_Update(GetSingleElementByName(rootNode, "travel_time_update"));
	processGeneratedRoutesNode(GetSingleElementByName(rootNode, "generateBusRoutes"));
	ProcessPublicTransit(GetSingleElementByName(rootNode, "public_transit"));
	
	//Read the settings for loop detector counts (optional node, short term)
	ProcessLoopDetectorCountsNode(GetSingleElementByName(rootNode, "loop-detector_counts"));
	//Read the settings for density counts (optional node, short term)
	ProcessShortDensityMapNode(GetSingleElementByName(rootNode, "short-term_density-map"));

	ProcessScreenLineNode(GetSingleElementByName(rootNode, "screen-line_count"));

	//Take care of pathset manager confifuration in here
	ParsePathXmlConfig(sim_mob::ConfigManager::GetInstance().FullConfig().pathsetFile, sim_mob::ConfigManager::GetInstanceRW().PathSetConfig());

	if(cfg.cbd && ConfigManager::GetInstance().FullConfig().pathSet().psRetrievalWithoutBannedRegion.empty())
	{
        throw std::runtime_error("Pathset without banned area stored procedure name not found\n");
	}
}

void sim_mob::ParseConfigFile::ProcessSystemNode(DOMElement* node)
{
	ProcessSystemSimulationNode(GetSingleElementByName(node, "simulation", true));
	ProcessSystemWorkersNode(GetSingleElementByName(node, "workers", true));
	ProcessSystemSingleThreadedNode(GetSingleElementByName(node, "single_threaded"));
	ProcessSystemMergeLogFilesNode(GetSingleElementByName(node, "merge_log_files"));
	ProcessSystemNetworkSourceNode(GetSingleElementByName(node, "network_source"));
	ProcessSystemNetworkXmlInputFileNode(GetSingleElementByName(node, "network_xml_file_input"));
	ProcessSystemNetworkXmlOutputFileNode(GetSingleElementByName(node, "network_xml_file_output"));
	ProcessSystemDatabaseNode(GetSingleElementByName(node, "network_database"));
	ProcessSystemXmlSchemaFilesNode(GetSingleElementByName(node, "xsd_schema_files", true));
	ProcessSystemGenericPropsNode(GetSingleElementByName(node, "generic_props"));

	//Warn against using the old network file name for xml_file.
	if (GetSingleElementByName(node, "network_xml_file", false)) {
		throw std::runtime_error("Using old parameter: \"network_xml_file\"; please change it to \"network_xml_file_input\".");
	}

	//Warn against the old workgroup sizes field.
	if (GetSingleElementByName(node, "workgroup_sizes", false)) {
		throw std::runtime_error("Using old parameter: \"workgroup_sizes\"; please change review the new \"workers\" field.");
	}
}


/*void sim_mob::ParseConfigFile::ProcessGeometryNode(xercesc::DOMElement* node)
{
	//The geometry tag has some attributes.
	std::string geomType = ParseString(GetNamedAttributeValue(node, "type"));
	std::string geomSource = ParseString(GetNamedAttributeValue(node, "source"));

	//Quick validation
	if (geomType=="simple") {
		throw std::runtime_error("\"simple\" geometry type no longer supported.");
	}
	if (geomType!="aimsun") {
		throw std::runtime_error("Unknown geometry type.");
	}
	if (geomSource!="database") {
		throw std::runtime_error("Unknown geometry source.");
	}

	//This is kind of backwards...
	cfg.geometry.procedures.dbFormat = "aimsun";

	//Now parse the rest.
	ProcessGeomDbConnection(GetSingleElementByName(node, "connection"));
	ProcessGeomDbMappings(GetSingleElementByName(node, "mappings"));
}*/


void sim_mob::ParseConfigFile::ProcessBusStopScheduledTimesNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	//Loop through all children
	int count=0;
	for (DOMElement* item=node->getFirstElementChild(); item; item=item->getNextElementSibling()) {
		if (TranscodeString(item->getNodeName())!="stop") {
			Warn() <<"Invalid busStopScheduledTimes child node.\n";
			continue;
		}

		//Retrieve properties, add a new item to the vector.
		BusStopScheduledTime res;
		res.offsetAT = ParseUnsignedInt(GetNamedAttributeValue(item, "offsetAT"), static_cast<unsigned int>(0));
		res.offsetDT = ParseUnsignedInt(GetNamedAttributeValue(item, "offsetDT"), static_cast<unsigned int>(0));
		cfg.busScheduledTimes[count++] = res;
	}
}

void sim_mob::ParseConfigFile::ProcessPersonCharacteristicsNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	//Loop through all children
	int count=0;
	for (DOMElement* item=node->getFirstElementChild(); item; item=item->getNextElementSibling()) {
		if (TranscodeString(item->getNodeName())!="person") {
			Warn() <<"Invalid personCharacteristics child node.\n";
			continue;
		}

		//Retrieve properties, add a new item to the vector.
		PersonCharacteristics res;
		res.lowerAge = ParseUnsignedInt(GetNamedAttributeValue(item, "lowerAge"), static_cast<unsigned int>(0));
		res.upperAge = ParseUnsignedInt(GetNamedAttributeValue(item, "upperAge"), static_cast<unsigned int>(0));
		res.lowerSecs = ParseInteger(GetNamedAttributeValue(item, "lowerSecs"), static_cast<int>(0));
		res.upperSecs = ParseInteger(GetNamedAttributeValue(item, "upperSecs"), static_cast<int>(0));
		cfg.personCharacteristicsParams.personCharacteristics[count++] = res;
	}

	std::map<int, PersonCharacteristics> personCharacteristics =  cfg.personCharacteristicsParams.personCharacteristics;
	// calculate lowest age and highest age in the ranges
	for(std::map<int, PersonCharacteristics>::const_iterator iter=personCharacteristics.begin();iter != personCharacteristics.end(); ++iter) {
		if(cfg.personCharacteristicsParams.lowestAge > iter->second.lowerAge) {
			cfg.personCharacteristicsParams.lowestAge = iter->second.lowerAge;
		}
		if(cfg.personCharacteristicsParams.highestAge < iter->second.upperAge) {
			cfg.personCharacteristicsParams.highestAge = iter->second.upperAge;
		}
	}
}

void sim_mob::ParseConfigFile::ProcessConstructsNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	//Process each item in order.
	//ProcessConstructDistributionsNode(GetSingleElementByName(node, "distributions")); //<TODO
	ProcessConstructDatabasesNode(GetSingleElementByName(node, "databases"));
	ProcessConstructDbProcGroupsNode(GetSingleElementByName(node, "db_proc_groups"));
	ProcessConstructCredentialsNode(GetSingleElementByName(node, "credentials"));
}


void sim_mob::ParseConfigFile::ProcessConstructDatabasesNode(xercesc::DOMElement* node)
{
	for (DOMElement* item=node->getFirstElementChild(); item; item=item->getNextElementSibling()) {
		if (TranscodeString(item->getNodeName())!="database") {
			Warn() <<"Invalid databases child node.\n";
			continue;
		}

		//Retrieve some attributes from the Node itself.
		Database db(ParseString(GetNamedAttributeValue(item, "id")));
		std::string dbType = ParseString(GetNamedAttributeValue(item, "dbtype"), "");
		if (dbType != "postgres" && dbType != "mongodb") {
			throw std::runtime_error("Database type not supported.");
		}

		//Now retrieve the required parameters from child nodes.
		db.host = ProcessValueString(GetSingleElementByName(item, "host"));
		db.port = ProcessValueString(GetSingleElementByName(item, "port"));
		db.dbName = ProcessValueString(GetSingleElementByName(item, "dbname"));

		cfg.constructs.databases[db.getId()] = db;
	}
}

void sim_mob::ParseConfigFile::ProcessConstructDbProcGroupsNode(xercesc::DOMElement* node)
{
	for (DOMElement* item=node->getFirstElementChild(); item; item=item->getNextElementSibling()) {
		if (TranscodeString(item->getNodeName())!="proc_map") {
			Warn() <<"Invalid db_proc_groups child node.\n";
			continue;
		}

		//Retrieve some attributes from the Node itself.
		StoredProcedureMap pm(ParseString(GetNamedAttributeValue(item, "id")));
		pm.dbFormat = ParseString(GetNamedAttributeValue(item, "format"), "");
		if (pm.dbFormat != "aimsun" && pm.dbFormat != "long-term") {
			throw std::runtime_error("Stored procedure map format not supported.");
		}

		//Loop through and save child attributes.
		for (DOMElement* mapItem=item->getFirstElementChild(); mapItem; mapItem=mapItem->getNextElementSibling()) {
			if (TranscodeString(mapItem->getNodeName())!="mapping") {
				Warn() <<"Invalid proc_map child node.\n";
				continue;
			}

			std::string key = ParseString(GetNamedAttributeValue(mapItem, "name"), "");
			std::string val = ParseString(GetNamedAttributeValue(mapItem, "procedure"), "");
			if (key.empty() || val.empty()) {
				Warn() <<"Invalid mapping; missing \"name\" or \"procedure\".\n";
				continue;
			}

			pm.procedureMappings[key] = val;
		}

		cfg.constructs.procedureMaps[pm.getId()] = pm;
	}
}

void sim_mob::ParseConfigFile::ProcessConstructCredentialsNode(xercesc::DOMElement* node)
{
	for (DOMElement* item=node->getFirstElementChild(); item; item=item->getNextElementSibling()) {
		std::string name = TranscodeString(item->getNodeName());
		if (name!="file-based-credential" && name!="plaintext-credential") {
			Warn() <<"Invalid db_proc_groups child node.\n";
			continue;
		}

		//Retrieve some attributes from the Node itself.
		Credential cred(ParseString(GetNamedAttributeValue(item, "id")));

		//Setting the actual credentials depends on the type of node.
		if (name=="file-based-credential") {
			//Scan children for "path" nodes.
			std::vector<std::string> paths;
			for (DOMElement* pathItem=item->getFirstElementChild(); pathItem; pathItem=pathItem->getNextElementSibling()) {
				if (TranscodeString(pathItem->getNodeName())!="file") {
					Warn() <<"file-based credentials contain invalid child node; expected path.\n";
					continue;
				}
				std::string path = ParseString(GetNamedAttributeValue(pathItem, "path"), "");
				if (!path.empty()) {
					paths.push_back(path);
				}
			}

			//Try setting it.
			cred.LoadFileCredentials(paths);
		} else if (name=="plaintext-credential") {
			//Retrieve children manually.
			std::string username = ParseString(GetNamedAttributeValue(GetSingleElementByName(item, "username"), "value"), "");
			std::string password = ParseString(GetNamedAttributeValue(GetSingleElementByName(item, "password"), "value"), "");
			cred.SetPlaintextCredentials(username, password);
		} else {
			throw std::runtime_error("Unexpected (but allowed) credentials.");
		}

		//Save it.
		cfg.constructs.credentials[cred.getId()] = cred;
	}
}

void sim_mob::ParseConfigFile::ProcessLongTermParamsNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	//The longtermParams tag has an attribute
	cfg.ltParams.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), false);

	if (!cfg.ltParams.enabled)
	{
		return;
	}

	//Now set the rest.
	cfg.ltParams.days 				 = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "days"), "value"), static_cast<unsigned int>(0));
	cfg.ltParams.maxIterations 		 = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "maxIterations"), "value"), static_cast<unsigned int>(0));
	cfg.ltParams.tickStep 			 = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "tickStep"), "value"), static_cast<unsigned int>(0));
	cfg.ltParams.workers 			 = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "workers"), "value"), static_cast<unsigned int>(0));

	LongTermParams::DeveloperModel developerModel;
	developerModel.enabled = ParseBoolean(GetNamedAttributeValue(GetSingleElementByName( node, "developerModel"), "enabled"), false );
	developerModel.timeInterval = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "developerModel"), "timeInterval"), "value"), static_cast<unsigned int>(0));
	developerModel.initialPostcode = ParseInteger(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "developerModel"), "InitialPostcode"), "value"), static_cast<int>(0));
	developerModel.initialUnitId = ParseInteger(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "developerModel"), "initialUnitId"), "value"), static_cast<int>(0));
	developerModel.initialBuildingId = ParseInteger(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "developerModel"), "initialBuildingId"), "value"), static_cast<int>(0));
	developerModel.initialProjectId = ParseInteger(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "developerModel"), "initialProjectId"), "value"), static_cast<int>(0));
	cfg.ltParams.developerModel = developerModel;


	LongTermParams::HousingModel housingModel;
	housingModel.enabled = ParseBoolean(GetNamedAttributeValue(GetSingleElementByName( node, "housingModel"), "enabled"), false);
	housingModel.timeInterval = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "housingModel"), "timeInterval"), "value"), static_cast<unsigned int>(0));
	housingModel.timeOnMarket = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "housingModel"), "timeOnMarket"), "value"), static_cast<unsigned int>(0));
	housingModel.timeOffMarket = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "housingModel"), "timeOffMarket"), "value"), static_cast<unsigned int>(0));
	housingModel.vacantUnitActivationProbability = ParseFloat(GetNamedAttributeValue(GetSingleElementByName(node, "vacantUnitActivationProbability"), "value"));
	housingModel.initialHouseholdsOnMarket = ParseInteger(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "housingModel"), "InitialHouseholdsOnMarket"), "value"), static_cast<int>(0));
	housingModel.housingMarketSearchPercentage = ParseFloat(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "housingModel"), "housingMarketSearchPercentage"), "value"));
	housingModel.housingMoveInDaysInterval = ParseFloat(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "housingModel"), "housingMoveInDaysInterval"), "value"));
	housingModel.outputHouseholdLogsums = ParseBoolean(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "housingModel"), "outputHouseholdLogsums"), "value"), false);
	cfg.ltParams.housingModel = housingModel;

	LongTermParams::VehicleOwnershipModel vehicleOwnershipModel;
	vehicleOwnershipModel.enabled = ParseBoolean(GetNamedAttributeValue(GetSingleElementByName( node, "vehicleOwnershipModel"), "enabled"), false);
	vehicleOwnershipModel.vehicleBuyingWaitingTimeInDays = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(GetSingleElementByName( node, "vehicleOwnershipModel"), "vehicleBuyingWaitingTimeInDays"), "value"), static_cast<unsigned int>(0));
	cfg.ltParams.vehicleOwnershipModel = vehicleOwnershipModel;
}


void sim_mob::ParseConfigFile::ProcessFMOD_Node(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	//The fmod tag has an attribute
	cfg.fmod.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), false);

	//Now set the rest.
	cfg.fmod.ipAddress = ParseString(GetNamedAttributeValue(GetSingleElementByName(node, "ip_address"), "value"), "");
	cfg.fmod.port = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "port"), "value"), static_cast<unsigned int>(0));
	//cfg.fmod.updateTravelMS = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "interval_travel_MS"), "value"), static_cast<unsigned int>(0));
	//cfg.fmod.updatePosMS = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "interval_pos_MS"), "value"), static_cast<unsigned int>(0));
	cfg.fmod.mapfile = ParseString(GetNamedAttributeValue(GetSingleElementByName(node, "map_file"), "value"), "");
	cfg.fmod.blockingTimeSec = ParseUnsignedInt(GetNamedAttributeValue(GetSingleElementByName(node, "blocking_time_Sec"), "value"), static_cast<unsigned int>(0));
}

void sim_mob::ParseConfigFile::ProcessAMOD_Node(xercesc::DOMElement* node)
{
	if (!node) 
	{
		return;
	}

	//Read the attribute value indicating whether AMOD is enabled or disabled
	cfg.amod.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), false);
}

void sim_mob::ParseConfigFile::ProcessDriversNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	ProcessFutureAgentList(node, "driver", cfg.driverTemplates);
}

void sim_mob::ParseConfigFile::ProcessTaxiDriversNode(xercesc::DOMElement* node)
{
	if(!node)
	{
		return;
	}

	ProcessFutureAgentList(node, "taxidriver", cfg.taxiDriverTemplates);
}

void sim_mob::ParseConfigFile::ProcessPedestriansNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	ProcessFutureAgentList(node, "pedestrian", cfg.pedestrianTemplates);
}

void sim_mob::ParseConfigFile::ProcessBusDriversNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	ProcessFutureAgentList(node, "busdriver", cfg.busDriverTemplates);
}

void sim_mob::ParseConfigFile::ProcessPassengersNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	ProcessFutureAgentList(node, "passenger", cfg.passengerTemplates);
}

void sim_mob::ParseConfigFile::ProcessSignalsNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	ProcessFutureAgentList(node, "signal", cfg.signalTemplates, true, true, false, false);
}

void sim_mob::ParseConfigFile::ProcessBusControllersNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	ProcessFutureAgentList(node, "buscontroller", cfg.busControllerTemplates, false, false, true, false);
}

void sim_mob::ParseConfigFile::ProcessCBD_Node(xercesc::DOMElement* node){

	if (!node) {

		cfg.cbd = false;
		return;
	}
	cfg.cbd = ParseBoolean(GetNamedAttributeValue(node, "enabled"), "false");
}
void sim_mob::ParseConfigFile::ProcessPublicTransit(xercesc::DOMElement* node)
{
	if(!node)
	{
		cfg.publicTransitEnabled = false;
	}
	else
	{
		cfg.publicTransitEnabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), "false");
		if(cfg.publicTransitEnabled)
		{
			const std::string& key = cfg.system.networkDatabase.procedures;
			std::map<std::string, StoredProcedureMap>::const_iterator procMapIt = cfg.constructs.procedureMaps.find(key);
			if(procMapIt->second.procedureMappings.count("pt_vertices")==0 || procMapIt->second.procedureMappings.count("pt_edges")==0)
			{
				throw std::runtime_error("Public transit is enabled , but stored procedures not defined");
			}
		}
	}

}

void sim_mob::ParseConfigFile::processPathSetFileName(xercesc::DOMElement* node){

	if (!node) {

		cfg.cbd = false;
		return;
	}
	cfg.pathsetFile = ParseString(GetNamedAttributeValue(node, "value"));
}

void sim_mob::ParseConfigFile::processTT_Update(xercesc::DOMElement* node){
	if(!node)
	{
		throw std::runtime_error("pathset travel_time_interval not found\n");
	}
	else
	{
		sim_mob::ConfigManager::GetInstanceRW().PathSetConfig().interval = ParseInteger(GetNamedAttributeValue(node, "interval"), 600);
		sim_mob::ConfigManager::GetInstanceRW().PathSetConfig().alpha = ParseFloat(GetNamedAttributeValue(node, "alpha"), 0.5);
	}
}

void sim_mob::ParseConfigFile::processGeneratedRoutesNode(xercesc::DOMElement* node){
	if (!node) {

		cfg.generateBusRoutes = false;
		return;
	}
	cfg.generateBusRoutes = ParseBoolean(GetNamedAttributeValue(node, "enabled"), "false");
}

void sim_mob::ParseConfigFile::ProcessLoopDetectorCountsNode(xercesc::DOMElement* node)
{
	if(node)
	{		
		cfg.loopDetectorCounts.outputEnabled = ParseBoolean(GetNamedAttributeValue(node, "outputEnabled"), "false");
		if(cfg.loopDetectorCounts.outputEnabled)
		{
			cfg.loopDetectorCounts.frequency = ParseUnsignedInt(GetNamedAttributeValue(node, "frequency"), 600000);
			cfg.loopDetectorCounts.fileName = ParseString(GetNamedAttributeValue(node, "file-name"), "private/VehCounts.csv");
			
			if(cfg.loopDetectorCounts.frequency == 0)
			{
				throw std::runtime_error("ParseConfigFile::ProcessLoopDetectorCountsNode - Update frequency for aggregating vehicle counts is 0");
			}
			
			if(cfg.loopDetectorCounts.fileName.empty())
			{
				throw std::runtime_error("ParseConfigFile::ProcessLoopDetectorCountsNode - File name is empty");
			}
		}
	}
}

void sim_mob::ParseConfigFile::ProcessShortDensityMapNode(xercesc::DOMElement* node)
{
	if(node)
	{		
		cfg.segDensityMap.outputEnabled = ParseBoolean(GetNamedAttributeValue(node, "outputEnabled"), "false");
		if(cfg.segDensityMap.outputEnabled)
		{
			cfg.segDensityMap.updateInterval = ParseUnsignedInt(GetNamedAttributeValue(node, "updateInterval"), 1000);
			cfg.segDensityMap.fileName = ParseString(GetNamedAttributeValue(node, "file-name"), "private/DensityMap.csv");
			
			if(cfg.segDensityMap.updateInterval == 0)
			{
				throw std::runtime_error("ParseConfigFile::ProcessShortDensityMapNode - Update interval for aggregating density is 0");
			}
			
			if(cfg.segDensityMap.fileName.empty())
			{
				throw std::runtime_error("ParseConfigFile::ProcessShortDensityMapNode - File name is empty");
			}
		}
	}
}

void sim_mob::ParseConfigFile::ProcessScreenLineNode(xercesc::DOMElement* node)
{
	if(node)
	{
		cfg.screenLineParams.outputEnabled = ParseBoolean(GetNamedAttributeValue(node, "screenLineCountEnabled"), "false");
		if(cfg.screenLineParams.outputEnabled)
		{
			cfg.screenLineParams.interval = ParseUnsignedInt(GetNamedAttributeValue(node, "interval"), 300);
			cfg.screenLineParams.fileName = ParseString(GetNamedAttributeValue(node, "file-name"), "screenLineCount.txt");

			if(cfg.screenLineParams.interval == 0)
			{
				throw std::runtime_error("ParseConfigFile::ProcessScreenLineNode - Interval for screen line count is 0");
			}
			if(cfg.screenLineParams.fileName.empty())
			{
				throw std::runtime_error("ParseConfigFile::ProcessScreenLineNode - File Name is empty");
			}
		}
	}
}

void sim_mob::ParseConfigFile::ProcessSystemSimulationNode(xercesc::DOMElement* node)
{
	//Several properties are set up as "x ms", or "x seconds", etc.
	cfg.system.simulation.baseGranMS = ProcessTimegranUnits(GetSingleElementByName(node, "base_granularity", true));
	cfg.system.simulation.baseGranSecond = cfg.system.simulation.baseGranMS / MILLISECONDS_IN_SECOND;
	cfg.system.simulation.totalRuntimeMS = ProcessTimegranUnits(GetSingleElementByName(node, "total_runtime", true));
	cfg.system.simulation.totalWarmupMS = ProcessTimegranUnits(GetSingleElementByName(node, "total_warmup"));

	//These should all be moved; for now we copy them over with one command.
	cfg.system.simulation.reactTimeDistribution1.typeId = ProcessValueInteger(GetSingleElementByName(node, "reacTime_distributionType1"));
	cfg.system.simulation.reactTimeDistribution2.typeId = ProcessValueInteger(GetSingleElementByName(node, "reacTime_distributionType2"));
	cfg.system.simulation.reactTimeDistribution1.mean = ProcessValueInteger(GetSingleElementByName(node, "reacTime_mean1"));
	cfg.system.simulation.reactTimeDistribution2.mean = ProcessValueInteger(GetSingleElementByName(node, "reacTime_mean2"));
	cfg.system.simulation.reactTimeDistribution1.stdev = ProcessValueInteger(GetSingleElementByName(node, "reacTime_standardDev1"));
	cfg.system.simulation.reactTimeDistribution2.stdev = ProcessValueInteger(GetSingleElementByName(node, "reacTime_standardDev2"));

	cfg.system.simulation.simStartTime = ProcessValueDailyTime(GetSingleElementByName(node, "start_time", true));
	//Now we're getting back to real properties.
	ProcessSystemAuraManagerImplNode(GetSingleElementByName(node, "aura_manager_impl"));
	ProcessSystemWorkgroupAssignmentNode(GetSingleElementByName(node, "workgroup_assignment"));
	cfg.system.simulation.partitioningSolutionId = ProcessValueInteger(GetSingleElementByName(node, "partitioning_solution_id"));
	ProcessSystemLoadAgentsOrderNode(GetSingleElementByName(node, "load_agents"));
	cfg.system.simulation.startingAutoAgentID = ProcessValueInteger2(GetSingleElementByName(node, "auto_id_start"), 0);
	ProcessSystemMutexEnforcementNode(GetSingleElementByName(node, "mutex_enforcement"));
	ProcessSystemCommsimNode(GetSingleElementByName(node, "commsim"));

	//Warn against the old communication format.
	if (GetSingleElementByName(node, "communication", false)) {
		throw std::runtime_error("Using old parameter: \"communication\"; please review the new \"XXX\" format.");
	}
}

void sim_mob::ParseConfigFile::ProcessSystemWorkersNode(xercesc::DOMElement* node)
{
	ProcessWorkerPersonNode(GetSingleElementByName(node, "person", true));
	ProcessWorkerSignalNode(GetSingleElementByName(node, "signal", true));
	ProcessWorkerIntMgrNode(GetSingleElementByName(node, "intersection_manager", true));
	ProcessWorkerCommunicationNode(GetSingleElementByName(node, "communication", true));

}

void sim_mob::ParseConfigFile::ProcessSystemSingleThreadedNode(xercesc::DOMElement* node)
{
	//TODO: GetAttribute not working; there's a null throwing it off.
	cfg.system.singleThreaded = ParseBoolean(GetNamedAttributeValue(node, "value"), false);
}


void sim_mob::ParseConfigFile::ProcessSystemMergeLogFilesNode(xercesc::DOMElement* node)
{
	cfg.system.mergeLogFiles = ParseBoolean(GetNamedAttributeValue(node, "value"), false);
}


void sim_mob::ParseConfigFile::ProcessSystemNetworkSourceNode(xercesc::DOMElement* node)
{
	cfg.system.networkSource = ParseNetSourceEnum(GetNamedAttributeValue(node, "value"), SystemParams::NETSRC_XML);
}


void sim_mob::ParseConfigFile::ProcessSystemNetworkXmlInputFileNode(xercesc::DOMElement* node)
{
	cfg.system.networkXmlInputFile = ParseNonemptyString(GetNamedAttributeValue(node, "value"), "private/SimMobilityInput.xml");
}

void sim_mob::ParseConfigFile::ProcessSystemNetworkXmlOutputFileNode(xercesc::DOMElement* node)
{
	cfg.system.networkXmlOutputFile = ParseNonemptyString(GetNamedAttributeValue(node, "value"), "");//or NetworkCopy.xml
}

void sim_mob::ParseConfigFile::ProcessSystemDatabaseNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}
	cfg.system.networkDatabase.database = ParseString(GetNamedAttributeValue(node, "database"), "");
	cfg.system.networkDatabase.credentials = ParseString(GetNamedAttributeValue(node, "credentials"), "");
	cfg.system.networkDatabase.procedures = ParseString(GetNamedAttributeValue(node, "proc_map"), "");
}

void sim_mob::ParseConfigFile::ProcessSystemXmlSchemaFilesNode(xercesc::DOMElement* node)
{
	//For now, only the Road Network has an XSD file (doing this for the config file from within it would be difficult).
	DOMElement* rn = GetSingleElementByName(node, "road_network");
	if (rn) {
		std::vector<DOMElement*> options = GetElementsByName(rn, "option");
		for (std::vector<DOMElement*>::const_iterator it=options.begin(); it!=options.end(); ++it) {
			std::string path = ParseString(GetNamedAttributeValue(*it, "value"), "");
			if (!path.empty()) {
				//See if the file exists.
				if (boost::filesystem::exists(path)) {
					//Convert it to an absolute path.
					boost::filesystem::path abs_path = boost::filesystem::absolute(path);
					cfg.system.roadNetworkXsdSchemaFile = abs_path.string();
					break;
				}
			}
		}

		//Did we try and find nothing?
		if (!options.empty() && cfg.system.roadNetworkXsdSchemaFile.empty()) {
			Warn() <<"Warning: No viable options for road_network schema file." <<std::endl;
		}
	}
}


void sim_mob::ParseConfigFile::ProcessSystemGenericPropsNode(xercesc::DOMElement* node)
{
	if (!node) {
		return;
	}

	std::vector<DOMElement*> properties = GetElementsByName(node, "property");
	for (std::vector<DOMElement*>::const_iterator it=properties.begin(); it!=properties.end(); ++it) {
		std::string key = ParseString(GetNamedAttributeValue(*it, "key"), "");
		std::string val = ParseString(GetNamedAttributeValue(*it, "value"), "");
		if (!(key.empty() && val.empty())) {
			cfg.system.genericProps[key] = val;
		}
	}
}


unsigned int sim_mob::ParseConfigFile::ProcessTimegranUnits(xercesc::DOMElement* node)
{
	return ParseTimegranAsMs(GetNamedAttributeValue(node, "value"), GetNamedAttributeValue(node, "units"));
}

bool sim_mob::ParseConfigFile::ProcessValueBoolean(xercesc::DOMElement* node)
{
	return ParseBoolean(GetNamedAttributeValue(node, "value"));
}

int sim_mob::ParseConfigFile::ProcessValueInteger(xercesc::DOMElement* node)
{
	return ParseInteger(GetNamedAttributeValue(node, "value"));
}

DailyTime sim_mob::ParseConfigFile::ProcessValueDailyTime(xercesc::DOMElement* node)
{
	return ParseDailyTime(GetNamedAttributeValue(node, "value"));
}

void sim_mob::ParseConfigFile::ProcessSystemAuraManagerImplNode(xercesc::DOMElement* node)
{
	cfg.system.simulation.auraManagerImplementation = ParseAuraMgrImplEnum(GetNamedAttributeValue(node, "value"), AuraManager::IMPL_RSTAR);
}

void sim_mob::ParseConfigFile::ProcessSystemWorkgroupAssignmentNode(xercesc::DOMElement* node)
{
	cfg.system.simulation.workGroupAssigmentStrategy = ParseWrkGrpAssignEnum(GetNamedAttributeValue(node, "value"), WorkGroup::ASSIGN_SMALLEST);
}

void sim_mob::ParseConfigFile::ProcessSystemLoadAgentsOrderNode(xercesc::DOMElement* node)
{
	//Separate into a string array.
	std::string value = ParseString(GetNamedAttributeValue(node, "order"), "");
	std::vector<std::string> valArray;
	boost::split(valArray, value, boost::is_any_of(", "), boost::token_compress_on);

	//Now, turn into an enum array.
	for (std::vector<std::string>::const_iterator it=valArray.begin(); it!=valArray.end(); ++it) {
		SimulationParams::LoadAgentsOrderOption opt(SimulationParams::LoadAg_Database);
		if ((*it) == "database") {
			opt = SimulationParams::LoadAg_Database;
		} else if ((*it) == "drivers") {
			opt = SimulationParams::LoadAg_Drivers;
		} else if ((*it) == "pedestrians") {
			opt = SimulationParams::LoadAg_Pedestrians;
		} else if ((*it) == "passengers") {
			opt = SimulationParams::LoadAg_Passengers;
		} else if ((*it) == "xml-tripchains") {
			opt = SimulationParams::LoadAg_XmlTripChains;
		} else {
			std::stringstream out;
			out.str("");
			out << "Unexpected load_agents order param." << "[" << *it << "]";
			throw std::runtime_error(out.str());
		}
		cfg.system.simulation.loadAgentsOrder.push_back(opt);
	}
}

void sim_mob::ParseConfigFile::ProcessSystemMutexEnforcementNode(xercesc::DOMElement* node)
{
	cfg.system.simulation.mutexStategy = ParseMutexStrategyEnum(GetNamedAttributeValue(node, "strategy"), MtxStrat_Buffered);
}

void sim_mob::ParseConfigFile::ProcessSystemCommsimNode(xercesc::DOMElement* node)
{
	if (!node) { return; }

	//Enabled?
	cfg.system.simulation.commsim.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), false);

	//Number of threads assigned to the boost I/O service that reads from Android clients.
	cfg.system.simulation.commsim.numIoThreads = ProcessValueInteger(GetSingleElementByName(node, "io_threads", true));

	//Minimum clients
	cfg.system.simulation.commsim.minClients = ProcessValueInteger(GetSingleElementByName(node, "min_clients", true));

	//Hold tick
	cfg.system.simulation.commsim.holdTick = ProcessValueInteger(GetSingleElementByName(node, "hold_tick", true));

	//Use ns-3 for routing?
	cfg.system.simulation.commsim.useNs3 = ProcessValueBoolean(GetSingleElementByName(node, "use_ns3", true));
}

void sim_mob::ParseConfigFile::ProcessWorkerPersonNode(xercesc::DOMElement* node)
{
	cfg.system.workers.person.count = ParseInteger(GetNamedAttributeValue(node, "count"));
	cfg.system.workers.person.granularityMs = ParseGranularitySingle(GetNamedAttributeValue(node, "granularity"));
}


void sim_mob::ParseConfigFile::ProcessWorkerSignalNode(xercesc::DOMElement* node)
{
	cfg.system.workers.signal.count = ParseInteger(GetNamedAttributeValue(node, "count"));
	cfg.system.workers.signal.granularityMs = ParseGranularitySingle(GetNamedAttributeValue(node, "granularity"));
}

void sim_mob::ParseConfigFile::ProcessWorkerIntMgrNode(xercesc::DOMElement* node)
{
	cfg.system.workers.intersectionMgr.count = ParseInteger(GetNamedAttributeValue(node, "count"));
	cfg.system.workers.intersectionMgr.granularityMs = ParseGranularitySingle(GetNamedAttributeValue(node, "granularity"));
}

void sim_mob::ParseConfigFile::ProcessWorkerCommunicationNode(xercesc::DOMElement* node)
{
	cfg.system.workers.communication.count = ParseInteger(GetNamedAttributeValue(node, "count"));
	cfg.system.workers.communication.granularityMs = ParseGranularitySingle(GetNamedAttributeValue(node, "granularity"));
}

/*void sim_mob::ParseConfigFile::ProcessGeomDbConnection(xercesc::DOMElement* node)
{
	std::set<std::string> passed; //Kind of messy; format needs a redesign.
	std::vector<DOMElement*> params = GetElementsByName(node, "param");
	for (std::vector<DOMElement*>::const_iterator it=params.begin(); it!=params.end(); it++) {
		std::string pKey = ParseString(GetNamedAttributeValue(*it, "name", true));
		std::transform(pKey.begin(), pKey.end(), pKey.begin(), ::tolower);
		std::string pVal = ParseString(GetNamedAttributeValue(*it, "value", true));

		//Make sure we tag each property.
		if (pKey=="host") {
			cfg.geometry.connection.host = pVal;
		} else if (pKey=="port") {
			cfg.geometry.connection.port = pVal;
		} else if (pKey=="dbname") {
			cfg.geometry.connection.dbName = pVal;
		} else if (pKey=="user") {
			cfg.geometry.connection.user = pVal;
		} else if (pKey=="password") {
			cfg.geometry.connection.password = Password(pVal);
		} else {
			//Skip unknown properties (can also do an error).
			continue;
		}

		//Tag it.
		passed.insert(pKey);
	}

	//Did we get everything?
	if (passed.size()<5) {
		Warn() <<"Database connection has too few parameters; connection attempts may fail.\n";
	}
}*/


/*void sim_mob::ParseConfigFile::ProcessGeomDbMappings(xercesc::DOMElement* node)
{
	//These are so backwards...
	for (DOMElement* prop=node->getFirstElementChild(); prop; prop=prop->getNextElementSibling()) {
		//Save its name/procedure type in the procedures map.
		cfg.geometry.procedures.procedureMappings[TranscodeString(prop->getNodeName())] = TranscodeString(GetNamedAttributeValue(prop, "procedure", true));
	}
}*/


void sim_mob::ParseConfigFile::ProcessFutureAgentList(xercesc::DOMElement* node, const std::string& itemName, std::vector<EntityTemplate>& res, bool originReq, bool destReq, bool timeReq, bool laneReq)
{
	//We use the existing "element child" functions, it's significantly slower to use "getElementsByTagName()"
	for (DOMElement* item=node->getFirstElementChild(); item; item=item->getNextElementSibling()) {
		if (TranscodeString(item->getNodeName())==itemName) {
			EntityTemplate ent;
			ent.originPos = ParsePoint2D(GetNamedAttributeValue(item, "originPos", originReq),Point2D());
			ent.destPos = ParsePoint2D(GetNamedAttributeValue(item, "destPos", destReq), Point2D());
			ent.startTimeMs = ParseUnsignedInt(GetNamedAttributeValue(item, "time", timeReq), static_cast<unsigned int>(0));
			ent.laneIndex = ParseUnsignedInt(GetNamedAttributeValue(item, "lane", laneReq), static_cast<unsigned int>(0));
			ent.angentId = ParseUnsignedInt(GetNamedAttributeValue(item, "id", false), static_cast<unsigned int>(0));
			ent.initSegId = ParseUnsignedInt(GetNamedAttributeValue(item, "initSegId", false), static_cast<unsigned int>(0));
			ent.initDis = ParseUnsignedInt(GetNamedAttributeValue(item, "initDis", false), static_cast<unsigned int>(0));
			ent.initSpeed = ParseUnsignedInt(GetNamedAttributeValue(item, "initSpeed", false), static_cast<double>(0));
			ent.originNode = ParseUnsignedInt(GetNamedAttributeValue(item, "originNode", false), static_cast<double>(0));
			ent.destNode = ParseUnsignedInt(GetNamedAttributeValue(item, "destNode", false), static_cast<double>(0));
			res.push_back(ent);
		}
	}
}

void sim_mob::ParseConfigFile::ProcessIncidentsNode(xercesc::DOMElement* node)
{
	if(!node) {
		return;
	}

	bool enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), false);
	if(!enabled){
		return;
	}

	for(DOMElement* item=node->getFirstElementChild(); item; item=item->getNextElementSibling()) {
		IncidentParams incident;
		incident.incidentId = ParseUnsignedInt(GetNamedAttributeValue(item, "id"));
		incident.visibilityDistance = ParseFloat(GetNamedAttributeValue(item, "visibility"));
		incident.segmentId = ParseUnsignedInt(GetNamedAttributeValue(item, "segment") );
		incident.position = ParseFloat(GetNamedAttributeValue(item, "position"));
		incident.capFactor = ParseFloat(GetNamedAttributeValue(item, "cap_factor") );
		incident.startTime = ParseDailyTime(GetNamedAttributeValue(item, "start_time") ).getValue();
		incident.duration = ParseDailyTime(GetNamedAttributeValue(item, "duration") ).getValue();
		incident.length = ParseFloat(GetNamedAttributeValue(item, "length") );
		incident.compliance = ParseFloat(GetNamedAttributeValue(item, "compliance") );
		incident.accessibility = ParseFloat(GetNamedAttributeValue(item, "accessibility") );

		for(DOMElement* child=item->getFirstElementChild(); child; child=child->getNextElementSibling()){
			IncidentParams::LaneParams lane;
			lane.laneId = ParseUnsignedInt(GetNamedAttributeValue(child, "laneId"));
			lane.speedLimit = ParseFloat(GetNamedAttributeValue(child, "speedLimitFactor") );
			incident.laneParams.push_back(lane);
		}

		cfg.incidents.push_back(incident);
	}
}

void sim_mob::ParseConfigFile::ProcessModelScriptsNode(xercesc::DOMElement* node)
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
	cfg.luaScriptsMap = scriptsMap;
}

