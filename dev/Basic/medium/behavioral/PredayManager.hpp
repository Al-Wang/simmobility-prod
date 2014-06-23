//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * PredayManager.hpp
 *
 *  Created on: Nov 18, 2013
 *      Author: Harish Loganathan
 */

#pragma once
#include <boost/unordered_map.hpp>
#include <vector>
#include "behavioral/PredaySystem.hpp"
#include "CalibrationStatistics.hpp"
#include "params/PersonParams.hpp"
#include "params/ZoneCostParams.hpp"
#include "database/DB_Connection.hpp"

namespace sim_mob {
namespace medium {

/**
 * structure to hold a calibration variable and its pertinent details.
 */
struct CalibrationVariable
{
public:
	CalibrationVariable() : variableName(std::string()), scriptFileName(std::string()), initialValue(0), currentValue(0), lowerLimit(0), upperLimit(0)
	{}

	const double getInitialValue() const
	{
		return initialValue;
	}

	const double getLowerLimit() const
	{
		return lowerLimit;
	}

	const std::string& getScriptFileName() const
	{
		return scriptFileName;
	}

	const double getUpperLimit() const
	{
		return upperLimit;
	}

	const std::string& getVariableName() const
	{
		return variableName;
	}

	void setInitialValue(double initialValue)
	{
		this->initialValue = initialValue;
	}

	void setLowerLimit(double lowerLimit)
	{
		this->lowerLimit = lowerLimit;
	}

	void setScriptFileName(const std::string& scriptFileName)
	{
		this->scriptFileName = scriptFileName;
	}

	void setUpperLimit(double upperLimit)
	{
		this->upperLimit = upperLimit;
	}

	void setVariableName(const std::string& variableName)
	{
		this->variableName = variableName;
	}

	double getCurrentValue() const
	{
		return currentValue;
	}

	void setCurrentValue(double currentValue)
	{
		this->currentValue = currentValue;
	}

private:
	std::string variableName;
	std::string scriptFileName;
	double initialValue;
	double currentValue;
	double lowerLimit;
	double upperLimit;
};

class PredayManager {
public:

	PredayManager();
	virtual ~PredayManager();

	/**
	 * Gets person data from the database and stores corresponding PersonParam pointers in personList.
	 *
	 * @param dbType type of backend where the population data is available
	 */
	void loadPersons(db::BackendType dbType);

	/**
	 * Gets details of all mtz zones
	 *
	 * @param dbType type of backend where the zone data is available
	 */
	void loadZones(db::BackendType dbType);

	/**
	 * Gets the list of nodes within each zone and stores them in a map
	 *
	 * @param dbType type of backend where the zone node mapping data is available
	 */
	void loadZoneNodes(db::BackendType dbType);

	/**
	 * loads the AM, PM and off peak costs data
	 *
	 * @param dbType type of backend where the cost data is available
	 */
	void loadCosts(db::BackendType dbType);

	/**
	 * Distributes persons to different threads and starts the threads which process the persons
	 */
	void distributeAndProcessPersons();

	/**
	 * preday calibration function
	 * Runs the spsa calibration algorithm
	 * TODO: implement w-spsa when weight matrix is ready
	 */
	void calibratePreday();

private:
	typedef std::vector<PersonParams*> PersonList;
	typedef boost::unordered_map<int, ZoneParams*> ZoneMap;
	typedef boost::unordered_map<int, boost::unordered_map<int, CostParams*> > CostMap;
	typedef boost::unordered_map<int, std::vector<long> > ZoneNodeMap;

	/**
	 * Threaded function loop for simulation.
	 * Loops through all elements in personList within the specified range and
	 * invokes the Preday system of models for each of them.
	 *
	 * @param first personList iterator corresponding to the first person to be
	 * 				processed
	 * @param last personList iterator corresponding to the person after the
	 * 				last person to be processed
	 */
	void processPersons(PersonList::iterator first, PersonList::iterator last);

	/**
	 * Threaded function loop for calibration.
	 * Loops through all elements in personList within the specified range and
	 * invokes the Preday system of models for each of them.
	 *
	 * @param first personList iterator corresponding to the first person to be
	 * 				processed
	 * @param last personList iterator corresponding to the person after the
	 * 				last person to be processed
	 * @param simStats the object to collect statistics into
	 */
	void processPersonsForCalibration(PersonList::iterator first, PersonList::iterator last, CalibrationStatistics& simStats);

	/**
	 * Threaded logsum computation
	 * Loops through all elements in personList within the specified range and
	 * invokes logsum computations for each of them.
	 *
	 * @param first personList iterator corresponding to the first person to be
	 * 				processed
	 * @param last personList iterator corresponding to the person after the
	 * 				last person to be processed
	 */
	void computeLogsums(PersonList::iterator first, PersonList::iterator last);

	/**
	 * loads csv containing calibration variables for preday
	 */
	void loadCalibrationVariables();

	/**
	 * computes gradients using SPSA technique
	 *
	 * @param randomVector symmetric random vector of +1s and -1s
	 * @param perturbationStepSize perturbation step size
	 */
	void computeGradientsUsingSPSA(const std::vector<short>& randomVector, double perturbationStepSize, std::vector<double>& gradientVector);

	/**
	 * runs preday and computes the value for objective function
	 */
	double computeObjectiveFunction(const std::vector<CalibrationVariable>& calibrationVariableList);

	PersonList personList;

    /**
     * map of zone_id -> ZoneParams
     * \note this map has 1092 elements
     */
    ZoneMap zoneMap;
    ZoneNodeMap zoneNodeMap;
    boost::unordered_map<int,int> zoneIdLookup;

    /**
     * Map of AM, PM and Off peak Costs [origin zone, destination zone] -> CostParams*
     * \note these maps have (1092 zones * 1092 zones - 1092 (entries with same origin and destination is not available)) 1191372 elements
     */
    CostMap amCostMap;
    CostMap pmCostMap;
    CostMap opCostMap;

    /**
     * list of values computed for objective function
     * objectiveFunctionValue[i] is the objective function value for iteration i
     */
    std::vector<double> objectiveFunctionValues;

    /**
     * Calibration statistics collector for each thread
     * simulatedStats[4] is the statistics from the 5th thread which pr
     */
    std::vector<CalibrationStatistics> simulatedStatsVector;
};
} //end namespace medium
} //end namespace sim_mob



