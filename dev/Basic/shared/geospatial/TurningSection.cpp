//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "TurningSection.hpp"
#include <sstream>

using namespace sim_mob;

sim_mob::TurningSection::TurningSection():
		dbId(-1),from_xpos(-1),from_ypos(-1),
				to_xpos(-1),to_ypos(-1),
				from_road_section(""),
				to_road_section(""),
				from_lane_index(-1),
				to_lane_index(-1),
				fromSeg(nullptr),toSeg(nullptr),
	laneFrom(nullptr),laneTo(nullptr){

}

sim_mob::TurningSection::TurningSection(const TurningSection & ts):
		dbId(ts.dbId),from_xpos(ts.from_xpos),from_ypos(ts.from_ypos),
		to_xpos(ts.to_xpos),to_ypos(ts.to_ypos),
		from_road_section(ts.from_road_section),
		to_road_section(ts.to_road_section),
		from_lane_index(ts.from_lane_index),
		to_lane_index(ts.to_lane_index),
		fromSeg(nullptr),toSeg(nullptr),
		sectionId(ts.sectionId),laneFrom(nullptr),laneTo(nullptr){
	std::stringstream out("");
	out<<ts.dbId;
	sectionId = out.str();

	std::stringstream trimmer;
	trimmer << from_road_section;
	from_road_section.clear();
	trimmer >> from_road_section;

	trimmer.clear();
	trimmer << to_road_section;
	to_road_section.clear();
	trimmer >> to_road_section;
}

std::vector<TurningSection*>& sim_mob::TurningSection::getConflictTurnings() {
	return conflicts;
}
TurningConflict* sim_mob::TurningSection::getTurningConflict(const TurningSection* ts) {
	TurningConflict* res = nullptr;
	std::cout<<"getTurningConflict: size "<<turningConflicts.size()<<std::endl;
	for(int i=0;i<turningConflicts.size();++i) {
		TurningConflict* tc = turningConflicts[i];
		if(ts == tc->firstTurning || ts == tc->secondTurning) {
			res = tc;
			break;
		}
	}// end of for
	return res;
}

