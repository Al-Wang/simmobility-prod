#include "IncidentManager.hpp"

#include <entities/conflux/Conflux.hpp>
#include <entities/roles/driver/DriverFacets.hpp>
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "geospatial/PathSetManager.hpp"
#include "message/MessageBus.hpp"
//may needed when you move definition to cpp
//#include <boost/random.hpp>
//#include <boost/random/normal_distribution.hpp>

#include <boost/tokenizer.hpp>

void sim_mob::IncidentManager::setFileSource(const std::string fileName_){
	fileName = fileName_;
}

/**
 * read the incidents from a file
 */
void sim_mob::IncidentManager::readFromFile(std::string fileName){
	std::ifstream in(fileName.c_str());
	if (!in.is_open()){
		std::ostringstream out("");
		out << "File " << fileName << " not found";
		throw std::runtime_error(out.str());
		//return false;
	}
	sim_mob::StreetDirectory & stDir = sim_mob::StreetDirectory::instance();
	typedef boost::tokenizer< boost::escaped_list_separator<char> > Tokenizer;
	std::string line;

	while (getline(in,line))
	{
		Tokenizer record(line);
		Tokenizer::iterator it = record.begin();
		unsigned int sectionId = boost::lexical_cast<unsigned int>(*(it));//first element
		double newFlowRate = boost::lexical_cast<double>(*(++it));//second element
		uint32_t tick =  boost::lexical_cast<uint32_t>(*(++it));//second element
		incidents.insert(std::make_pair(tick,Incident(sectionId,newFlowRate,tick)));
	}
}

void sim_mob::IncidentManager::insertTickIncidents(uint32_t tick){
	/*find the incidents in this tick*/
	TickIncidents currIncidents = incidents.equal_range(tick);
	if(currIncidents.first == currIncidents.second){
		//no incidents for this tick
		return;
	}
	sim_mob::StreetDirectory & stDir = sim_mob::StreetDirectory::instance();
	std::pair<uint32_t,Incident> incident;
	/*inserting and informing*/
	BOOST_FOREACH(incident, currIncidents){
		//get the conflux
		const sim_mob::RoadSegment* rs = stDir.getRoadSegment(incident.second.get<0>());
		const std::vector<sim_mob::SegmentStats*>& stats = rs->getParentConflux()->findSegStats(rs);
		//send a message to conflux
		messaging::MessageBus::PostMessage(rs->getParentConflux(), MSG_INSERT_INCIDENT,
							messaging::MessageBus::MessagePtr(new InsertIncidentMessage(stats, incident.second.get<1>())));
		//find affected Drivers (only active agents for now)
		std::vector <const sim_mob::Person*> persons;
		identifyAffectedDrivers(rs,persons);
		//inform the drivers
		BOOST_FOREACH(const sim_mob::Person * person, persons) {
			//send the same type of message
			messaging::MessageBus::PostMessage(const_cast<MovementFacet*>(person->getRole()->Movement()), MSG_INSERT_INCIDENT,
								messaging::MessageBus::MessagePtr(new InsertIncidentMessage(stats, incident.second.get<1>())));

		}
	}
}

//step-1: find those who used the target rs in their path
//step-2: for each person, iterate through the path(meso path for now) to see if the agent's current segment is before, on or after the target path.
//step-3: if agent's current segment is before the target path, then inform him(if the probability function allows that).
void sim_mob::IncidentManager::identifyAffectedDrivers(const sim_mob::RoadSegment * targetRS,
		std::vector <const sim_mob::Person*> filteredPersons){

	//normal distribution
	  boost::mt19937 rng; // I don't seed it on purpouse (it's not relevant)

	  boost::normal_distribution<> nd(0.0, 1.0);

	  boost::variate_generator<boost::mt19937&,
	                           boost::normal_distribution<> > var_nor(rng, nd);


	//step-1: find those who used the target rs in their path
	const std::pair <RPOD::const_iterator,RPOD::const_iterator > range(sim_mob::PathSetManager::getInstance()->getODbySegment(targetRS));

	for(RPOD::const_iterator it(range.first); it != range.second; it++){
		const sim_mob::Person *per = it->second.per;
		//get his,meso, path...//todo: you need to dynamic_cast!
		const sim_mob::medium::DriverMovement *dm = dynamic_cast<sim_mob::medium::DriverMovement*>(per->getRole()->Movement());
		const std::vector<const sim_mob::SegmentStats*> path = dm->getMesoPathMover().getPath();
		const sim_mob::SegmentStats* curSS = dm->getMesoPathMover().getCurrSegStats();
		//In the following steps, we try to select only those who are before the current segment
		//todo, increase the criteria , add some min distance restriction
		std::vector<const sim_mob::SegmentStats*>::const_iterator itSS;//segStat iterator
		//a.is the incident before driver's current segment?
		bool res = false;
		for(itSS = path.begin(); (*itSS) != curSS ; itSS++){
			if(targetRS == (*itSS)->getRoadSegment()){
				res = true;
				break;
			}
		}
		//for (*itSS) == curSS , same check:
		if(itSS != path.end() && (targetRS == (*itSS)->getRoadSegment())){
			res = true;
		}

		if(res){
			//person passed, or currently on the target path. So, not interested in this person
			continue;
		}

		for(itSS = path.begin(); (*itSS) != curSS ; itSS++){
			if(targetRS == (*itSS)->getRoadSegment()){
				res = true;
				break;
			}
		}
		if(!res){
			//can't be! this means we have been serching for a target road segment that is not in the path!
			throw std::runtime_error("searching for a roadsegment which was not in the path!");
		}

		//probability function(for now, just behave like tossing a coin
		//todo, move to a method, probably inside a person
		if(var_nor() > 0){
			//this person willbe informed
			filteredPersons.push_back(per);
			Print() << "Person" << per->getId() << " selected" << std::endl;
		}
		else
		{
			Print() << "Person" << per->getId() << " discarded" << std::endl;
		}
	}//RPOD
}

bool sim_mob::IncidentManager::frame_init(timeslice now){
	if(fileName.size()){
		readFromFile(fileName);
	}
}

sim_mob::Entity::UpdateStatus sim_mob::IncidentManager::frame_tick(timeslice now){
	insertTickIncidents(now.frame());
}

