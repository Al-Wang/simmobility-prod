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
#include "params/PersonParams.hpp"
#include "params/ZoneCostParams.hpp"
#include "database/DB_Connection.hpp"

namespace sim_mob {
namespace medium {
typedef std::vector<PersonParams> PersonList;
typedef boost::unordered_map<int, ZoneParams> ZoneMap;

class PredayManager {
public:
	/**
	 * Gets person data from the database and stores corresponding PersonParam pointers in personList.
	 */
	void loadPersons(db::BackendType dbType);

	/**
	 * Gets details of all mtz zones
	 */
	void loadZones(db::BackendType dbType);

	/**
	 * Distributes persons to different threads and starts the threads which process the persons
	 */
	void distributeAndProcessPersons(uint16_t numWorkers = 1);

private:
	/**
	 * Threaded function loop.
	 * Loops through all elements in personList and invokes the Preday system of models for each of them.
	 *
	 */
	void processPersons(PersonList& persons);

	PersonList personList;

	ZoneMap zoneMap;

};
} //end namespace medium
} //end namespace sim_mob



