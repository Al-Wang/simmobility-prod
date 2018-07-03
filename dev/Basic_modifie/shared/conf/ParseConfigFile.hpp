//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <boost/noncopyable.hpp>
#include <string>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

#include "conf/ParseConfigXmlBase.hpp"
#include "conf/RawConfigParams.hpp"
#include "util/DailyTime.hpp"

using namespace sim_mob;
using namespace xercesc;

namespace sim_mob
{

/**
 * Class used to parse a config file into a RawConfigParams object.
 * Typically used like a verb:
 * RawConfigParams& cfg = ConfigParams::getInstanceRW();
 * ParseConfigFile print(cfg);
 */
class ParseConfigFile : public ParseConfigXmlBase, private boost::noncopyable
{
public:
	/**
	 * Parse a config file into RawConfigParams, performing all XML parsing and some trivial semantic processing.
	 *
	 * @param configFileName - Configuration filename which is to be parsed
	 * @param result - parsed values are stored here
	 * @param longTerm - flag for long term config file
	 */
	ParseConfigFile(const std::string &configFileName, RawConfigParams &result, bool longTerm = false);

private:
	/**
	 * Code for processing the xml
	 *
	 * @param configFileName is the filename of configuration
	 */
	virtual void processXmlFile(xercesc::XercesDOMParser &parser);

	//These functions are called by processXmlFile()
	//Process through recursive descent.

	/**
	 * Processes the constructs element in the config file
	 *
	 * @param node node corresponding to the construct element in xml file
	 */
	void processConstructsNode(xercesc::DOMElement *node);

	/**
	 * Processes the simulation element in the config file
	 *
	 * @param node node corresponding to the simulation element in xml file
	 */
	void processSimulationNode(xercesc::DOMElement *node);

	/*
	 * Process the schemas defined in the xml file
	 * */
	void processSchemasParamsNode(xercesc::DOMElement *node);

	/**
	 * Processes the longTerm element in the config file
	 * @param node node corresponding to the longTerm element in xml file
	 */
	void processLongTermParamsNode(xercesc::DOMElement *node);

	/**
	 * Processes the developerModel element in the config file
	 * @param node node corresponding to the developerModel element in the xml file
	 */
	void processDeveloperModelNode(xercesc::DOMElement *node);

	/**
	 * Processes the housingModel element in the config file
	 * @param node node corresponding to the housingModel element in the xml file
	 */
	void processHousingModelNode(DOMElement *node);

	/**
	 * Processes the outputHouseholdLogsums element in the config file
	 * @param node node corresponding to the outputHouseholdLogsums element in the xml file
	 */
	void processHouseHoldLogsumsNode(DOMElement *node);

	/**
	 * Processes the vehicleOwnershipModel element in the config file
	 * @param node node corresponding to the vehicleOwnershipModel element in the xml file
	 */
	void processVehicleOwnershipModelNode(DOMElement *vehOwnModel);

	/**
	 * Processes the schoolAssignmentModel element in the config file
	 * @param node node corresponding to the schoolAssignmentModel element in the xml file
	 */
	void processSchoolAssignmentModelNode(DOMElement *schoolAssignModel);

	/**
	 * Processes the jobAssignmentModel element in the config file
	 * @param node node corresponding to the jobAssignmentModel element in the xml file
	 */
	void processJobAssignmentModelNode(DOMElement *jobAssignModel);

	/**
	 * Processes the scenario element in the config file
	 * @param node node corresponding to the scenario element in the xml file
	 */
	void processScenarioNode(DOMElement *scenarioNode);

	/**
	 * Processes the outputFiles element in the config file
	 * @param node node corresponding to the outputFiles element in the xml file
	 */
	void processOutputFilesNode(DOMElement *output);

	//Descend through Constructs

	/**
	 * Processes the databases element in the config file
	 *
	 * @param node node corresponding to the databases element in the xml file
	 */
	void processConstructDatabaseNode(xercesc::DOMElement *node);

	/**
	 * Processes the credentials element in the config file
	 *
	 * @param node node corresponding to the credentials element in the xml file
	 */
	void processConstructCredentialNode(xercesc::DOMElement *node);

	//Descend through Simulation

	/**
	 * Processes the workgroup_assignment element in the config file
	 *
	 * @param node node correspoding to the workgroup_assignment element in the xml file
	 */
	void processWorkgroupAssignmentNode(xercesc::DOMElement *node);

	/**
	 * Processes the operational cost in the config file
	 *
	 * @param node node correspoding to the operational cost element in the simulation.xml file
	 */
	void processOperationalCostNode(xercesc::DOMElement *node);


	/**
	 * Processes the closed_loop element in the config file
	 *
	 * @param node node corresponding to the closed_loop element in the xml file
	 */
	void processClosedLoopPropertiesNode(xercesc::DOMElement *node);

	/**
	 * Processes the merge_log_files element in the config file
	 *
	 * @param node node corresponding to the merge_log_files element in the xml file
	 */
	void processMergeLogFilesNode(xercesc::DOMElement *node);

	/**
	 * Processes the generic_props element in the config file
	 *
	 * @param node node corresponding to the generic_props element in the xml file
	 */
	void processGenericPropsNode(xercesc::DOMElement *node);

	/**
	 * Helper function to process time from nodes represented with "value" attribute
	 *
	 * @param node xml element to be processed
	 *
	 * @return extracted time in ms
	 */
	unsigned int processTimeGranUnits(xercesc::DOMElement *node);

	/**
	 * Helper function to process DailyTime from nodes represented with "value" attribute
	 *
	 * @param node xml element to be processed
	 *
	 * @return extracted DailyTime value
	 */
	sim_mob::DailyTime processValueDailyTime(xercesc::DOMElement *node);

	/**
	 * Helper function to process the value for percentage of drivers using in-simulation travel times
	 *
	 * @param node xml element to be processed
	 *
	 * @return extracted percentage value
	 */
	unsigned int processInSimulationTTUsage(xercesc::DOMElement *node);

	/**
	 * Processes the mutex_enforcement element in the config file
	 *
	 * @param node node corresponding to the mutex_enforcement element in the xml file
	 */
	void processMutexEnforcementNode(xercesc::DOMElement *node);

	/**
	 * Processes the model_scripts element in the config file
	 *
	 * @param node node corresponding to the model_scripts element in the xml file
	 */
	void processModelScriptsNode(xercesc::DOMElement *node);

    /**
     * Processes the mobilityServiceController element in config xml
     *
     * @param node node corresponding to the mobilityServiceController element inside xml file
     */
    void processMobilityServiceControllerNode(xercesc::DOMElement *node);

private:
	//The config file we are currently loading
	RawConfigParams &cfg;

	// LongTerm configuration flag
	bool longTerm;
};

}


