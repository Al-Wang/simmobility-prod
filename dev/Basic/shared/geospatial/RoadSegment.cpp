//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "RoadSegment.hpp"

#include <stdexcept>

#include "conf/settings/DisableMPI.h"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"

//TEMP
#include "geospatial/aimsun/Loader.hpp"

#include "geospatial/Link.hpp"
#include "util/DynamicVector.hpp"
#include "util/GeomHelpers.hpp"
#include "util/Utils.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#endif

#include "Lane.hpp"
#include "entities/conflux/Conflux.hpp"

using std::pair;
using std::vector;
using std::set;

using namespace sim_mob;

std::map<unsigned long, const RoadSegment*> sim_mob::RoadSegment::allSegments;//map<segment id, segment pointer>

sim_mob::RoadSegment::RoadSegment(sim_mob::Link* parent, unsigned long id) :
	Pavement(),
	maxSpeed(0), capacity(0), busstop(nullptr), lanesLeftOfDivider(0), parentLink(parent),segmentID(id),
	parentConflux(nullptr), polylineLength(-1.0), type(LINK_TYPE_DEFAULT), CBD(false), defaultTravelTime(0), highway(false)
{
	allSegments[segmentID] = this;
}

const unsigned long sim_mob::RoadSegment::getId()const
{
	return segmentID;
}
void sim_mob::RoadSegment::setLanes(std::vector<sim_mob::Lane*> lanes)
{
	this->lanes = lanes;
}
unsigned int sim_mob::RoadSegment::getAdjustedLaneId(unsigned int laneId)
{
	unsigned int adjustedId  = lanes.size()-1 - laneId;
	if (adjustedId > lanes.size() -1) {
		adjustedId = 0;
	}
	return adjustedId;
}

void sim_mob::RoadSegment::setParentLink(Link* parent)
{
	this->parentLink = parent;
}

const sim_mob::Lane* sim_mob::RoadSegment::getLane(int laneID) const
{
	if (laneID<0 || laneID>=lanes.size()) {
		return nullptr;
	}
	return lanes[laneID];
}
size_t sim_mob::RoadSegment::getLanesSize(bool isIncludePedestrianLane) const
{
	if(isIncludePedestrianLane)
	{
		return getLanes().size();
	}
	else
	{
		size_t s = getLanes().size();
		if(getLanes().at(s-1)->is_pedestrian_lane()) // most left lane is ped?
		{
			s--;
		}
		if(getLanes().at(0)->is_pedestrian_lane() )// most right lane is ped?
		{
			s--;
		}
		return s;
	}
}

bool sim_mob::RoadSegment::isSingleDirectional()
{
	return lanesLeftOfDivider==0 || lanesLeftOfDivider==lanes.size()-1;
}


bool sim_mob::RoadSegment::isBiDirectional()
{
	return !isSingleDirectional();
}


pair<int, const Lane*> sim_mob::RoadSegment::translateRawLaneID(unsigned int rawID)
{
	//TODO: Need to convert the ID into an "effective" lane ID based on road direction
	//      (including bidirectional segments).
	throw std::runtime_error("Not yet defined.");
}

unsigned int sim_mob::RoadSegment::getSegmentAimsunId() const{

	unsigned int originId = 0;

	std::string aimsunId = originalDB_ID.getLogItem();
	std::string segId = sim_mob::Utils::getNumberFromAimsunId(aimsunId);
	try {
		originId = boost::lexical_cast<int>(segId);
	} catch( boost::bad_lexical_cast const& ) {
		Warn() << "Error: aimsun id string was not valid" << std::endl;
	}

	return originId;
}
std::string sim_mob::RoadSegment::getSegmentAimsunIdStr() const{
	std::string aimsunId = originalDB_ID.getLogItem();
	std::string segId = sim_mob::Utils::getNumberFromAimsunId(aimsunId);
	return segId;
}
void sim_mob::RoadSegment::specifyEdgePolylines(const vector< vector<Point2D> >& calcdPolylines)
{
	//Save the edge polylines.
	laneEdgePolylines_cached = calcdPolylines;

	//TODO: Optionally reset this Segment's own polyline to laneEdge[0].
}

const double sim_mob::RoadSegment::getLengthOfSegment() const
{
	std::vector<sim_mob::Point2D> polypointsList = (this)->getLanes().at(0)->getPolyline();
	double dis=0;
	std::vector<sim_mob::Point2D>::iterator ite;
	for ( std::vector<sim_mob::Point2D>::iterator it =  polypointsList.begin(); it != polypointsList.end(); ++it )
	{
		ite = it+1;
		if ( ite != polypointsList.end() )
		{
			DynamicVector temp(it->getX(), it->getY(),ite->getX(), ite->getY());
			dis += temp.getMagnitude();
		}
	}

	return dis;
}

///This function forces a rebuild of all Lane and LaneEdge polylines.
///There are two ways to calculate the polyline. First, if the parent RoadSegment's "laneEdgePolylines_cached"
/// is non-empty, we can simply take the left lane line and continuously project it onto the right lane line
/// (then scale it back halfway). If these data points are not available, we have to compute it based on the
/// RoadSegment's polyline, which might be less accurate.
///We compute all points at once, since calling getLanePolyline() and then getLaneEdgePolyline() might
/// leave the system in a questionable state.
void sim_mob::RoadSegment::syncLanePolylines() /*const*/
{
	//Check our width (and all lane widths) are up-to-date:
	int totalWidth = 0;
    for (vector<Lane*>::const_iterator it=lanes.begin(); it!=lanes.end(); it++) {
    	if ((*it)->getWidth()==0) {
    		(*it)->width_ = 300; //TEMP: Hardcoded. TODO: Put in DB somewhere.
    	}
    	totalWidth += (*it)->getWidth();
    }

    if (width == 0) { width = totalWidth; }

	//First, rebuild the Lane polylines; these will never be specified in advance.
	bool edgesExist = !laneEdgePolylines_cached.empty();
	for (size_t i=0; i<lanes.size(); i++) {
		if (edgesExist) {
			//check lane polyline already exist
			if(lanes[i]->polyline_.size() == 0)
			{
				makeLanePolylineFromEdges(lanes[i], laneEdgePolylines_cached[i], laneEdgePolylines_cached[i+1]);
			}
		} else {
			lanes[i]->makePolylineFromParentSegment();
		}
	}

	//Next, if our edges array doesn't exist, re-generate it from the computed lanes.
	if (!edgesExist) {
		for (size_t i=0; i<=lanes.size(); i++) {
			bool edgeIsRight = i<lanes.size();
			laneEdgePolylines_cached.push_back(makeLaneEdgeFromPolyline(lanes[edgeIsRight?i:i-1], edgeIsRight));
		}
	}
}

void sim_mob::RoadSegment::computePolylineLength()
{
	polylineLength = 0.0;
	for (vector<Point2D>::const_iterator it2 = polyline.begin(); (it2 + 1) != polyline.end(); it2++)
	{
		polylineLength += dist(it2->getX(), it2->getY(), (it2 + 1)->getX(), (it2 + 1)->getY());
	}
}

void sim_mob::RoadSegment::setCapacity() {
	capacity = lanes.size()*2000.0;
}

double sim_mob::RoadSegment::getCapacityPerInterval() const {
	return capacity * ConfigManager::GetInstance().FullConfig().baseGranSecond() / 3600;
}

vector<Point2D> sim_mob::RoadSegment::makeLaneEdgeFromPolyline(Lane* refLane, bool edgeIsRight) const
{
	//Sanity check
	if (refLane->polyline_.size()<=1) {
		throw std::runtime_error("Can't manage with a Lane polyline of 0 or 1 points.");
	}
	else
	{
//		std::cout << refLane->getLaneID() << " : refLane->polyline_.size() = " << refLane->polyline_.size() << std::endl;
	}
	if (refLane->width_==0) {
		throw std::runtime_error("Can't manage with a Segment/Lane with zero width.");
	}

	//Create a vector from start to end
	DynamicVector fullLine(refLane->polyline_.front().getX(), refLane->polyline_.front().getY(), refLane->polyline_.back().getX(), refLane->polyline_.back().getY());

	//Iterate over every point on the midline
	vector<Point2D> res;
	const Point2D* lastPt = nullptr;
	for (vector<Point2D>::const_iterator it=refLane->polyline_.begin(); it!=refLane->polyline_.end(); it++) {
		//Scale and translate the primary vector?
		if (lastPt) {
			double segDist = sim_mob::dist(*lastPt, *it);
			fullLine.scaleVectTo(segDist).translateVect();
		}

		//Make another vector, rotate right/left, scale half the width and add it to our result.
		DynamicVector currLine(fullLine);
		currLine.flipNormal(edgeIsRight).scaleVectTo(refLane->width_/2.0).translateVect();
		res.push_back(Point2D(currLine.getX(), currLine.getY()));

		//Save for the next round
		lastPt = &(*it);
	}

	//TEMP:
//	std::cout <<"Line: " <<edgeIsRight <<"\n";
//	std::cout <<"  Median:" <<refLane->polyline_.front().getX() <<"," <<refLane->polyline_.front().getY() <<" => " <<refLane->polyline_.back().getX() <<"," <<refLane->polyline_.back().getY()  <<"\n";
//	std::cout <<"  Edge:" <<res.front().getX() <<"," <<res.front().getY() <<" => " <<res.back().getX() <<"," <<res.back().getY()  <<"\n";
	return res;
}



void sim_mob::RoadSegment::makeLanePolylineFromEdges(Lane* lane, const vector<Point2D>& inner, const vector<Point2D>& outer) const
{
//	std::cout << "inner.size() = " << inner.size() << "      outer.size()" << outer.size() << std::endl << std::endl;
//	for(int i= 0; i < inner.size(); i++)
//	{
//		std::cout << "inner(" << inner.at(i).getX() << "," << inner.at(i).getY() << ")" << std::endl;
//	}
//	std::cout << "\n";
//	for(int i= 0; i < outer.size(); i++)
//	{
//		std::cout << "outer(" << outer.at(i).getX() << "," << outer.at(i).getY() << ")" << std::endl;
//	}
//	std::cout << "\n";
//	for (size_t i=0; i<lanes.size(); i++) {
//		std::cout << "+Again Lane " << lanes.at(i)->getLaneID() << " width: " << lanes.at(i)->getWidth_real() << std::endl;
//	}
//	std::cout << "\n";
	//Sanity check
	if(originalDB_ID.getLogItem().find("3440") != std::string::npos){
		int i=0;
	}
	if (outer.size()<=1 || inner.size()<=1) {
		throw std::runtime_error("Can't manage with a Lane Edge polyline of 0 or 1 points.");
	}

	//Get the offset of inner[0] to outer[0]
	double magX = outer.front().getX() - inner.front().getX(); //std::cout << "outer.front().getX() - inner.front().getX() = " << magX << std::endl;
	double magY = outer.front().getY() - inner.front().getY(); //std::cout << "outer.front().getY() - inner.front().getY() = " << magY << std::endl;
	double magTotal = sqrt(magX*magX + magY*magY);
	//std::cout << "magTotal =" << magTotal << std::endl;

//	for (size_t i=0; i<lanes.size(); i++) {
//		std::cout << "1+Again Lane " << lanes.at(i)->getLaneID() << " width: " << lanes.at(i)->getWidth_real() << std::endl;
//	}
//	std::cout << "\n";
	//Update the lane's width
	lane->width_ = magTotal;

//	for (size_t i=0; i<lanes.size(); i++) {
//		std::cout << "2+Again Lane " << lanes.at(i)->getLaneID() << " width: " << lanes.at(i)->getWidth_real() << std::endl;
//	}
//	std::cout << "\n";
	//Travel along the inner path. Essentially, the inner and outer paths should line up, but if there's an extra point
	// or two, we don't want our algorithm to go crazy.
	lane->polyline_.clear();
//	for (size_t i=0; i<lanes.size(); i++) {
//		std::cout << "3+Again Lane " << lanes.at(i)->getLaneID() << " width: " << lanes.at(i)->getWidth_real() << std::endl;
//	}
//	std::cout << "\n";
//	for (size_t i=0; i<lanes.size(); i++) {
//		std::cout << "++Again Lane " << lanes.at(i)->getLaneID() << " width: " << lanes.at(i)->getWidth_real() << std::endl;
//	}
	for (vector<Point2D>::const_iterator it=inner.begin(); it!=inner.end(); it++) {
		DynamicVector line(it->getX(), it->getY(), it->getX()+magX, it->getY()+magY);
		line.scaleVectTo(magTotal/2.0).translateVect();
		lane->polyline_.push_back(Point2D(line.getX(), line.getY()));
	}
//	for (size_t i=0; i<lanes.size(); i++) {
//		std::cout << "+++Again Lane " << lanes.at(i)->getLaneID() << " width: " << lanes.at(i)->getWidth_real() << std::endl;
//	}
}


//TODO: Restore const-correctness after cleaning up sidewalks.
const vector<Point2D>& sim_mob::RoadSegment::getLaneEdgePolyline(unsigned int laneID) /*const*/
{
	//TEMP: Due to the way we manually insert sidewalks, this is needed for now.
	bool syncNeeded = false;
	for (size_t i=0; i<lanes.size(); i++) {
		if (lanes.at(i)->polyline_.empty()) {
			syncNeeded = true;
		}
	}

	//Rebuild if needed
	if (laneEdgePolylines_cached.empty() || syncNeeded) {
		//try {
			syncLanePolylines();
		//} catch (std::exception& ex) {
			//std::cout <<"ERROR_2905" <<std::endl;
			//laneEdgePolylines_cached.push_back(std::vector<Point2D>());
		//}
	}
	return laneEdgePolylines_cached[laneID];
}


std::string sim_mob::RoadSegment::getStartEnd() const {
	std::stringstream startEndIDs;
	startEndIDs << "[" << getStart()->getID() << "," << getEnd()->getID() << "]";
	return startEndIDs.str();
}
