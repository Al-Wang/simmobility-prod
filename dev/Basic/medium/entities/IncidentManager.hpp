#pragma once
#include <sstream>
#include <vector>
#include <stdint.h>


#include "message/Message.hpp"
#include "entities/Agent.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>


namespace sim_mob {


/**
 * A class designed to manage the incidents.
 * It can read the load the incidents for preday/test planning as well as en-route incidents
 */
class IncidentManager : sim_mob::Agent {
	typedef boost::tuples::tuple<unsigned int, double, uint32_t> Incident; //<sectionId, flowrate, start_tick>
	std::multimap<uint32_t,Incident> incidents;
	typedef std::pair<std::multimap<uint32_t,Incident>::iterator, std::multimap<uint32_t,Incident>::iterator> TickIncidents;
	std::string fileName;
public:
	/**
	 * Set the source file name
	 * \param fileName_ the file containing the incident information
	 */
	void setFileSource(const std::string fileName_);

	/**
	 * read the incidents from a file
	 */
	void readFromFile(std::string fileName);
	/**
	 * insert incidents into simulation starting in a given tick.
	 * This can be done via defining a new flow rate to the lanes of the given segment.
	 * since it using the message bus, it will be available in the next tick
	 * \param tick starting tick
	 */
	void insertTickIncidents(uint32_t tick);

	/**
	 *	step-1: find those who used the target rs in their path
	 *	step-2: for each person, iterate through the path(meso path for now) to see if the agent's current segment is before, on or after the target path.
	 *	step-3: if agent's current segment is before the target path, then inform him.
	 * \param in targetRS the target road segment that is having an incident
	 * \param out filteredPersons persons to be notified of the incident in the target roadsegment
	 */
	void identifyAffectedDrivers(const sim_mob::RoadSegment * targetRS,std::vector <const sim_mob::Person*> filteredPersons);

	bool frame_init(timeslice now);

	virtual Entity::UpdateStatus frame_tick(timeslice now);




};

}
