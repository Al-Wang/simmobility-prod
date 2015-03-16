//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "TurningSection.hpp"
#include <sstream>

using namespace sim_mob;

namespace sim_mob
{
	struct TComparator
	{
		TurningSection* turningSection;

		TComparator(TurningSection* turning)
		{
			turningSection = turning;
		}

		bool operator()(const TurningConflict* conflict1, const TurningConflict* conflict2)
		{
			//Distance to conflict point for the current turning
			double dstConflict1 = (conflict1->getFirstTurning() == turningSection) ? conflict1->getFirst_cd() : conflict1->getFirst_cd();

			//Distance to conflict point for the current turning
			double dstConflict2 = (conflict2->getFirstTurning() == turningSection) ? conflict2->getFirst_cd() : conflict2->getSecond_cd();

			return (dstConflict1 < dstConflict2);
		}
	} ;
};

sim_mob::TurningSection::TurningSection():
		dbId(-1),from_xpos(-1),from_ypos(-1),
				to_xpos(-1),to_ypos(-1),
				from_road_section(""),
				to_road_section(""),
				from_lane_index(-1),
				to_lane_index(-1),
				fromSeg(nullptr),toSeg(nullptr),
	laneFrom(nullptr),laneTo(nullptr),turningSpeed(0){

}

sim_mob::TurningSection::TurningSection(const TurningSection & ts):
		dbId(ts.dbId),from_xpos(ts.from_xpos),from_ypos(ts.from_ypos),
		to_xpos(ts.to_xpos),to_ypos(ts.to_ypos),
		from_road_section(ts.from_road_section),
		to_road_section(ts.to_road_section),
		from_lane_index(ts.from_lane_index),
		to_lane_index(ts.to_lane_index),
		fromSeg(nullptr),toSeg(nullptr),
		sectionId(ts.sectionId),laneFrom(nullptr),laneTo(nullptr),turningSpeed(ts.turningSpeed){
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

void TurningSection::setPolyline(TurningPolyline* src)
{
	polyline = src;
	polylinePoints.clear();
	for(int i=0;i<src->polypoints.size();++i){
		Polypoint* pp = src->polypoints.at(i);
		DPoint p;
		p.x = pp->x;
		p.y = pp->y;
		polylinePoints.push_back(p);
	}
}
void TurningSection::addTurningConflict(TurningConflict* turningConflict)
{
	this->turningConflicts.push_back(turningConflict);
	
	//Comparator struct for the sort function
	TComparator tComparator(this);
	
	//Sort the conflicts - the nearest conflict to the Turning comes first
	std::sort(this->turningConflicts.begin(), this->turningConflicts.end(), tComparator);
}

const std::vector<TurningConflict*>& TurningSection::getTurningConflicts() const
{
	return turningConflicts;
}

void TurningSection::addConflictingTurningSections(TurningSection* conflictingTurningSection)
{
	this->conflictingTurningSections.push_back(conflictingTurningSection);
}

std::vector<TurningSection*>& TurningSection::getConflictingTurningSections()
{
	return conflictingTurningSections;
}

void TurningSection::setLaneTo(const sim_mob::Lane* laneTo)
{
	this->laneTo = laneTo;
}

const sim_mob::Lane* TurningSection::getLaneTo() const
{
	return laneTo;
}

void TurningSection::setLaneFrom(const sim_mob::Lane* laneFrom)
{
	this->laneFrom = laneFrom;
}

void TurningSection::makePolylinePoint()
{
	if(!laneFrom || !laneTo) {
		throw std::runtime_error("no from/to lane");
	}
	// get last point of "from lane" polyline
	Point2D pfrom = laneFrom->polyline_.back();
	// get first point of to lane polyline
	from_xpos = pfrom.getX() / 100.0;
	from_ypos = pfrom.getY() / 100.0;

	// get first point of "to lane" polyline
	Point2D pto = laneTo->polyline_.front();
	// get first point of to lane polyline
	to_xpos = pto.getX() / 100.0;
	to_ypos = pto.getY() / 100.0;
}

const sim_mob::Lane* TurningSection::getLaneFrom() const
{
	return laneFrom;
}

void TurningSection::setPolylinePoints(std::vector<DPoint> polylinePoints)
{
	this->polylinePoints = polylinePoints;
}

std::vector<DPoint> TurningSection::getPolylinePoints() const
{
	return polylinePoints;
}

void TurningSection::setToSeg(sim_mob::RoadSegment* toSeg)
{
	this->toSeg = toSeg;
}

sim_mob::RoadSegment* TurningSection::getToSeg() const
{
	return toSeg;
}

void TurningSection::setFromSeg(sim_mob::RoadSegment* fromSeg)
{
	this->fromSeg = fromSeg;
}

sim_mob::RoadSegment* TurningSection::getFromSeg() const
{
	return fromSeg;
}

void TurningSection::setSectionId(std::string sectionId)
{
	this->sectionId = sectionId;
}

std::string TurningSection::getSectionId() const
{
	return sectionId;
}

void TurningSection::setTo_lane_index(int to_lane_index)
{
	this->to_lane_index = to_lane_index;
}

int TurningSection::getTo_lane_index() const
{
	return to_lane_index;
}

void TurningSection::setFrom_lane_index(int from_lane_index)
{
	this->from_lane_index = from_lane_index;
}

int TurningSection::getFrom_lane_index() const
{
	return from_lane_index;
}

void TurningSection::setTo_road_section(std::string to_road_section)
{
	this->to_road_section = to_road_section;
}

std::string TurningSection::getTo_road_section() const
{
	return to_road_section;
}

void TurningSection::setFrom_road_section(std::string from_road_section)
{
	this->from_road_section = from_road_section;
}

std::string TurningSection::getFrom_road_section() const
{
	return from_road_section;
}

void TurningSection::setTo_ypos(double to_ypos)
{
	this->to_ypos = to_ypos;
}

double TurningSection::getTo_ypos() const
{
	return to_ypos;
}

void TurningSection::setTo_xpos(double to_xpos)
{
	this->to_xpos = to_xpos;
}

double TurningSection::getTo_xpos() const
{
	return to_xpos;
}

void TurningSection::setFrom_ypos(double from_ypos)
{
	this->from_ypos = from_ypos;
}

double TurningSection::getFrom_ypos() const
{
	return from_ypos;
}

void TurningSection::setFrom_xpos(double from_xpos)
{
	this->from_xpos = from_xpos;
}

double TurningSection::getFrom_xpos() const
{
	return from_xpos;
}

void TurningSection::setDbId(int dbId)
{
	this->dbId = dbId;
}

int TurningSection::getDbId() const
{
	return dbId;
}

//Returns the TurningConflict between the given turnings (the current one (this) and the parameter)
TurningConflict* sim_mob::TurningSection::getTurningConflict(const TurningSection* turningSection) const
{
	TurningConflict* res = nullptr;
	
	for(int i = 0; i < turningConflicts.size(); ++i) 
	{
		TurningConflict* conflict = turningConflicts[i];
		TurningSection *firstTurning = conflict->getFirstTurning();
		TurningSection *secondTurning = conflict->getSecondTurning();
		
		if((turningSection == firstTurning && this == secondTurning) || 
		 (this == firstTurning && turningSection == secondTurning)) 
		{
			res = conflict;
			break;
		}
	}// end of for
	
	return res;
}

void TurningSection::setTurningSpeed(int turningSpeed)
{
	this->turningSpeed = turningSpeed;
}

int TurningSection::getTurningSpeed() const
{
	return turningSpeed;
}

