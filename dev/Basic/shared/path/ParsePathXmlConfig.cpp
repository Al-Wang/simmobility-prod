#include "ParsePathXmlConfig.hpp"
#include "util/XmlParseHelper.hpp"
#include <xercesc/dom/DOM.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>
#include "logging/Log.hpp"

using namespace xercesc;
using namespace std;
//Helper: turn a Xerces error message into a string.
std::string TranscodeString(const XMLCh* str) {
	char* raw = XMLString::transcode(str);
	std::string res(raw);
	XMLString::release(&raw);
	return res;
}

sim_mob::ParsePathXmlConfig::ParsePathXmlConfig(const std::string& configFileName, PathSetConf& result) : cfg(result), ParseConfigXmlBase(configFileName)
{
	if(configFileName.size() == 0)
	{
		throw std::runtime_error("pathset config file not specified");
	}
	parseXmlAndProcess();
}

void sim_mob::ParsePathXmlConfig::processXmlFile(XercesDOMParser& parser)
{
	//Verify that the root node is "config"
	DOMElement* rootNode = parser.getDocument()->getDocumentElement();
	if (TranscodeString(rootNode->getTagName()) != "pathset") {
		throw std::runtime_error("xml parse error: root node of path set configuration file must be \"pathset\"");
	}

	ProcessPathSetNode(rootNode);
}

void sim_mob::ParsePathXmlConfig::processXmlFileForServiceControler(xercesc::XercesDOMParser& parser)
{

}
void sim_mob::ParsePathXmlConfig::ProcessPathSetNode(xercesc::DOMElement* node){

	if (!node)
	{
		std::cerr << "Pathset Configuration Not Found\n" ;
		return;
	}

	if(!(cfg.enabled = ParseBoolean(GetNamedAttributeValue(node, "enabled"), "false"))) { return; }

	//pathset generation threadpool size
	xercesc::DOMElement* poolSize = GetSingleElementByName(node, "thread_pool");
	if(!poolSize)
	{
		std::cerr << "Pathset generation thread pool size not specified, defaulting to 5\n";
	}
	else
	{
		cfg.threadPoolSize = ParseInteger(GetNamedAttributeValue(poolSize, "size"), 5);
	}

	xercesc::DOMElement* pvtConfNode = GetSingleElementByName(node, "private_pathset");
    if((cfg.privatePathSetEnabled = ParseBoolean(GetNamedAttributeValue(pvtConfNode, "enabled"), "false")))
    {
    	processPrivatePathsetNode(pvtConfNode);
    }

    xercesc::DOMElement* publicConfNode = GetSingleElementByName(node, "public_pathset");
    if((cfg.publicPathSetEnabled = ParseBoolean(GetNamedAttributeValue(publicConfNode, "enabled"), "false")))
    {
    	processPublicPathsetNode(publicConfNode);
    }

}


ModelScriptsMap sim_mob::ParsePathXmlConfig::processModelScriptsNode(xercesc::DOMElement* node)
{
	std::string format = ParseString(GetNamedAttributeValue(node, "format"), "");
	std::string filename="";
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
		filename=val;
	}

	/*temporirily will be removed later */
	if(boost::iequals(filename , "serv.lua"))
	{
		cfg.ServiceControllerScriptsMap = scriptsMap;
		return  scriptsMap;
	}

	/* will be removed later */


	//cfg.ptRouteChoiceScriptsMap = scriptsMap;
	return scriptsMap;


}




void sim_mob::ParsePathXmlConfig::processPublicPathsetNode(xercesc::DOMElement* publicConfNode)
{

	cfg.publicPathSetMode = ParseString(GetNamedAttributeValue(publicConfNode, "mode"), "");
	if (cfg.publicPathSetMode.empty() || !(cfg.publicPathSetMode == "normal" || cfg.publicPathSetMode == "generation"))
	{
		throw std::runtime_error("No pathset mode specified, \"normal\" and \"generation\" modes are supported supported\n");
	}
	if(cfg.publicPathSetMode == "generation" && cfg.privatePathSetMode == "generation")
	{
		std::string err = std::string("Error: Both Private and Public transit cannot run in generation mode together");
		throw std::runtime_error(err);
	}
	if (cfg.publicPathSetMode == "generation")
	{
		xercesc::DOMElement* PT_odSource = GetSingleElementByName(publicConfNode, "od_source");
		if (!PT_odSource)
		{
			throw std::runtime_error("public transit pathset is in \"generation\" mode but od_source is not found in pathset xml config\n");
		}
		else
		{
			cfg.publicPathSetOdSource = ParseString(GetNamedAttributeValue(PT_odSource, "table"), "");
		}

        xercesc::DOMElement* bulk = GetSingleElementByName(publicConfNode, "bulk_generation_output_file");
		if (!bulk)
		{
			throw std::runtime_error("Public transit pathset is in \"generation\" mode but bulk_generation_output_file_name is not found in pathset xml config\n");
		}
		else
		{
				cfg.publicPathSetOutputFile = ParseString(GetNamedAttributeValue(bulk, "name"), "");
		}
	}
	xercesc::DOMElement* publicPathSetAlgoConf = GetSingleElementByName(publicConfNode, "pathset_generation_algorithms");
	if (publicPathSetAlgoConf)
	{
				cfg.publickShortestPathLevel = ParseInteger(GetNamedAttributeValue(
								GetSingleElementByName(publicPathSetAlgoConf, "k_shortest_path"), "level"), 10);
				cfg.simulationApproachIterations = ParseInteger(GetNamedAttributeValue(
								GetSingleElementByName(publicPathSetAlgoConf, "simulation_approach"), "iterations"), 10);
	}
}

void sim_mob::ParsePathXmlConfig::processPrivatePathsetNode(xercesc::DOMElement* pvtConfNode)
{
	cfg.privatePathSetMode = ParseString(GetNamedAttributeValue(pvtConfNode, "mode"), "");
	if (cfg.privatePathSetMode.empty() || !(cfg.privatePathSetMode == "normal" || cfg.privatePathSetMode == "generation"))
	{
		throw std::runtime_error("No pathset mode specified, \"normal\" and \"generation\" modes are supported supported\n");
	}

	//bulk pathset generation
	if (cfg.privatePathSetMode == "generation") {
		xercesc::DOMElement* odSource = GetSingleElementByName(pvtConfNode, "od_source");
		if (!odSource)
		{
			throw std::runtime_error("pathset is in \"generation\" mode but od_source is not found in pathset xml config\n");
		}
		else
		{
			cfg.odSourceTableName = ParseString(GetNamedAttributeValue(odSource, "table"), "");
		}

		xercesc::DOMElement* bulk = GetSingleElementByName(pvtConfNode, "bulk_generation_output_file");
		if (!bulk)
		{
			throw std::runtime_error("pathset is in \"generation\" mode but bulk_generation_output_file_name is not found in pathset xml config\n");
		}
		else
		{
			cfg.bulkFile = ParseString(GetNamedAttributeValue(bulk, "name"), "");
		}
	}

	xercesc::DOMElement* tableNode = GetSingleElementByName(pvtConfNode, "tables");
	if (!tableNode)
	{
		throw std::runtime_error("Pathset Tables specification not found");
	}
	else
	{
		cfg.pathSetTableName = ParseString(GetNamedAttributeValue(tableNode, "pathset_table"), "");
		if (cfg.pathSetTableName.empty())
		{
			throw std::runtime_error("private pathset table name missing in configuration");
		}

		cfg.RTTT_Conf = ParseString(GetNamedAttributeValue(tableNode, "historical_traveltime"), "");
		if (cfg.RTTT_Conf.empty())
		{
			throw std::runtime_error("historical travel time table name missing in pathset configuration");
		}

		cfg.DTT_Conf = ParseString(GetNamedAttributeValue(tableNode, "default_traveltime"), "");
		if (cfg.DTT_Conf.empty())
		{
			throw std::runtime_error("default travel time table name missing in pathset configuration");
		}
	}
	//function
	xercesc::DOMElement* functionNode = GetSingleElementByName(pvtConfNode, "functions");
	if (!functionNode) {
		throw std::runtime_error("Pathset Stored Procedure Not Found\n");
	}
	else {
		cfg.psRetrieval = ParseString(GetNamedAttributeValue(functionNode, "pathset"), "");
		cfg.upsert = ParseString(GetNamedAttributeValue(functionNode, "travel_time"), "");
		cfg.psRetrievalWithoutBannedRegion = ParseString(GetNamedAttributeValue(functionNode, "pathset_without_banned_area"), "");
	}

	//recirsive pathset generation
	xercesc::DOMElement* recPS = GetSingleElementByName(pvtConfNode, "recursive_pathset_generation");
	if (!recPS)
	{
		std::cerr << "recursive_pathset_generation Not Found, setting to false\n";
		cfg.recPS = false;
	}
	else { cfg.recPS = ParseBoolean(GetNamedAttributeValue(recPS, "value"), false); }

	//reroute
	xercesc::DOMElement* reroute = GetSingleElementByName(pvtConfNode, "reroute");
	if (!reroute)
	{
		std::cerr << "reroute_enabled Not Found, setting to false\n";
		cfg.reroute = false;
	}
	else { cfg.reroute = ParseBoolean(GetNamedAttributeValue(reroute, "enabled"), false); }

	///	path generators configuration
	xercesc::DOMElement* gen = GetSingleElementByName(pvtConfNode, "path_generators");
	if (gen) {
		cfg.maxSegSpeed = ParseFloat(GetNamedAttributeValue(gen, "max_segment_speed"), 0.0);
		if (cfg.maxSegSpeed <= 0.0)
		{
			throw std::runtime_error("Invalid or missing max_segment_speed attribute in the pathset configuration file");
		}
		//random perturbation
		xercesc::DOMElement* random = GetSingleElementByName(gen, "random_perturbation");
		if (random)
		{
			cfg.perturbationIteration = ParseInteger(GetNamedAttributeValue(random, "iterations"), 0);
			std::vector<std::string> rangeStrVec;
			std::string rangerStr = ParseString(GetNamedAttributeValue(random, "uniform_range"), "0,0");
			boost::split(rangeStrVec, rangerStr, boost::is_any_of(","));
			cfg.perturbationRange.first = boost::lexical_cast<int>(rangeStrVec[0]);
			cfg.perturbationRange.second = boost::lexical_cast<int>(rangeStrVec[1]);
		}

		//K-shortest Path
		xercesc::DOMElement* ksp = GetSingleElementByName(gen, "k_shortest_path");
		if (ksp) {
			cfg.kspLevel = ParseInteger(GetNamedAttributeValue(ksp, "level"), 0);
		}

		//Link Elimination
		xercesc::DOMElement* LE = GetSingleElementByName(gen, "link_elimination");
		if (LE)
		{
			std::string types;
			types = ParseString(GetNamedAttributeValue(LE, "types"), "");
			boost::split(cfg.LE, types, boost::is_any_of(","));
		}
	}

	//utility parameters
	xercesc::DOMElement* utility = GetSingleElementByName(pvtConfNode, "utility_parameters");
	if (utility)
	{
		cfg.params.highwayBias = ParseFloat(GetNamedAttributeValue(GetSingleElementByName(utility, "highwayBias"), "value"), 0.5);
	}

	//sanity check
	std::stringstream out("");
	if (cfg.pathSetTableName == "") {
		out << "single path's table name, ";
	}
	if (cfg.RTTT_Conf == "") {
		out << "single path's realtime TT table name, ";
	}
	if (cfg.DTT_Conf == "") {
		out << "single path's default TT table name, ";
	}
	if (cfg.psRetrieval == "") {
		out << "path set retieval stored procedure, ";
	}
	if (cfg.upsert == "") {
		out << "travel time updation stored procedure, ";
	}
	if (out.str().size()) {
		std::string err = std::string("Missing:") + out.str();
		throw std::runtime_error(err);
	}
}
