//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once
#include "behavioral/params/PersonParams.hpp"
#include "behavioral/params/LogsumTourModeDestinationParams.hpp"
#include "conf/ConfigParams.hpp"
#include "lua/LuaModel.hpp"

namespace sim_mob {

/**
 * Class to interface with the preday logit models specified in Lua scripts.
 * This class contains only logsum computation functions which are exposed to long-term code.
 *
 * \author Harish Loganathan
 */
class PredayLogsumLuaModel : public lua::LuaModel {
public:
	PredayLogsumLuaModel();
	virtual ~PredayLogsumLuaModel();

	/**
	 * Computes logsums for all tour types by invoking corresponding functions in tour mode-destination model
	 *
	 * @param personParams object containing person and household related variables. logsums will be updated in this object
	 * @param tourModeDestinationParams parameters specific to tour mode-destination models
	 * @param numZones total number of zones (possible destinations in the mode destination choice model)
	 */
    void computeTourModeDestinationLogsum(PersonParams& personParams, const std::unordered_map<int, ActivityTypeConfig> &activityTypes,
                                          LogsumTourModeDestinationParams& tourModeDestinationParams, int numZones) const;

	/**
	 * Computes logsums for work tours with fixed work location by invoking corresponding functions in tour mode (for work) model
	 *
	 * @param personParams object containing person and household related variables. logsums will be updated in this object
	 * @param tourModeDestinationParams parameters specific to tour mode model
	 */
    void computeTourModeLogsum(PersonParams& personParams, const std::unordered_map<int, ActivityTypeConfig> &activityTypes,
                               LogsumTourModeParams& tourModeParams) const;

	/**
	 * Computes logsums from day-pattern tours and day-pattern stops models
	 *
	 * @param personParams object containing person and household related variables. logsums will be updated in this object
	 */
	void computeDayPatternLogsums(PersonParams& personParams) const;

	/**
	 * Computes logsums from day pattern binary model. These logsums will be passed to long-term
	 *
	 * @param personParams object containing person and household related variables. logsums will be updated in this object
	 */
	void computeDayPatternBinaryLogsums(PersonParams& personParams) const;

private:
    /**
     * Inherited from LuaModel
     */
    void mapClasses();
};
}//end namespace sim_mob
