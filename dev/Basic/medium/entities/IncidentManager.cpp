#include "IncidentManager.hpp"

#include <entities/conflux/Conflux.hpp>
#include <entities/roles/driver/DriverFacets.hpp>
#include "geospatial/streetdir/StreetDirectory.hpp"
#include "geospatial/PathSetManager.hpp"
#include "message/MessageBus.hpp"

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
	TickIncidents res = incidents.equal_range(tick);
	if(res.first == res.second){
		//no incidents for this tick
		return;
	}
	sim_mob::StreetDirectory & stDir = sim_mob::StreetDirectory::instance();
	std::pair<uint32_t,Incident> element;
	BOOST_FOREACH(element, res){
		//get the conflux
		const sim_mob::RoadSegment* rs = stDir.getRoadSegment(element.second.get<0>());
		const std::vector<sim_mob::SegmentStats*>& stats = rs->getParentConflux()->findSegStats(rs);
		//send a message to conflux to change its flow rate for the next tick
		messaging::MessageBus::PostMessage(rs->getParentConflux(), MSG_INSERT_INCIDENT,
							messaging::MessageBus::MessagePtr(new InsertIncidentMessage(stats, element.second.get<1>())));
		//Identify the to-be-affected Drivers (at present, only the active agents are considered)
		//contact the path set manager(via your own method). he should already have the paths in its cache
		identifyAffectedDrivers(rs);
	}
}

//step-1: find those who used the target rs in their path
//step-2: for each person, iterate through the path(meso path for now) to see if the agent's current segment is before, on or after the target path.
//step-3: if agent's current segment is before the target path, then inform him.
void sim_mob::IncidentManager::identifyAffectedDrivers(const sim_mob::RoadSegment * rs){

	//make life easy, create a final list of eligible persons
	std::vector <const sim_mob::Person*> persons;

	//step-1: find those who used the target rs in their path
	const std::pair <RPOD::const_iterator,RPOD::const_iterator > range(sim_mob::PathSetManager::getInstance()->getODbySegment(rs));
	//At this stage, we have information about persons who had the given segment in their path
	//No filter out those who have already passed the given segment
	for(RPOD::const_iterator it(range.first); it != range.second; it++){
		//get the persons subtrip see if the current trip is...blah blah blah
		//target(aka Mr. sim_mob::Person) :
		const sim_mob::Person *per = it->second.per;
		//get his,meso, path...//todo: you need to dynamic_cast!
		const sim_mob::medium::DriverMovement *dm = dynamic_cast<sim_mob::medium::DriverMovement*>(per->getRole()->Movement());
		const std::vector<const sim_mob::SegmentStats*> path = dm->getMesoPathMover().getPath();

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

