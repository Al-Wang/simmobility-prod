/* Copyright Singapore-MIT Alliance for Research and Technology */

#include "RoadSegment.hpp"

#include "util/DynamicVector.hpp"
#include "util/GeomHelpers.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#endif

#include "Lane.hpp"

using namespace sim_mob;

using std::pair;
using std::vector;
using std::set;

const unsigned long  &  sim_mob::RoadSegment::getSegmentID()const
{
	return segmentID;
}
void sim_mob::RoadSegment::setLanes(std::vector<sim_mob::Lane*> lanes)
{
	this->lanes = lanes;
}

sim_mob::RoadSegment::RoadSegment(Link* parent, unsigned long id)
	: Pavement(), parentLink(parent),segmentID(id)
{

}

sim_mob::RoadSegment::RoadSegment(Link* parent, const SupplyParams* sParams, unsigned long id)
	: Pavement(), parentLink(parent),segmentID(id), supplyParams(sParams)
{

}

void sim_mob::RoadSegment::setParentLink(Link* parent)
{
	this->parentLink = parent;
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



void sim_mob::RoadSegment::specifyEdgePolylines(const vector< vector<Point2D> >& calcdPolylines)
{
	//Save the edge polylines.
	laneEdgePolylines_cached = calcdPolylines;

	//TODO: Optionally reset this Segment's own polyline to laneEdge[0].
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
    if (width == 0) {
    	width = totalWidth;
    }
	//First, rebuild the Lane polylines; these will never be specified in advance.
	bool edgesExist = !laneEdgePolylines_cached.empty();
	/*if (!edgesExist) {
		//TODO: The segment width should be saved in the DB somehow? It shouldn't be stored here, that's for sure.
		width = 300 * lanes.size();
	}*/
	for (size_t i=0; i<lanes.size(); i++) {
		if (edgesExist) {
			makeLanePolylineFromEdges(lanes[i], laneEdgePolylines_cached[i], laneEdgePolylines_cached[i+1]);
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
	//TEMP FIX
	//Now, add one more edge and one more lane representing the sidewalk.
	//TODO: This requires our function (and several others) to be declared non-const.
	//      Re-enable const correctness when we remove this code.
	//TEMP: For now, we just add the outer lane as a sidewalk. This won't quite work for bi-directional
	//      segments or for one-way Links. But it should be sufficient for the demo.
	Lane* swLane = new Lane(this, lanes.size());
	swLane->is_pedestrian_lane(true);
	swLane->width_ = lanes.back()->width_/2;
	swLane->polyline_ = sim_mob::ShiftPolyline(lanes.back()->polyline_, lanes.back()->getWidth()/2+swLane->getWidth()/2);
	//Add it, update
	lanes.push_back(swLane);
	width += swLane->width_;

	vector<Point2D> res = makeLaneEdgeFromPolyline(lanes.back(), false);
//	std::cout << "Inside syncLanePolylines() :Before the crash point \n";
	laneEdgePolylines_cached.push_back(res);//crash -vahid
	//Add an extra sidewalk on the other side if it's a road segment on a one-way link.
	sim_mob::Link* parentLink = getLink();

	if(parentLink)
	{
		//Check whether the link is one-way
		if(parentLink->getPath(false).empty() || parentLink->getPath(true).empty())
		{
			for(size_t i = 0; i < lanes.size(); ++i)
			{
				lanes[i]->laneID_++;
			}
			//Add a sidewalk on the other side of the road segment
			Lane* swLane2 = new Lane(this, 0);
			swLane2->is_pedestrian_lane(true);
			swLane2->width_ = lanes.front()->width_/2;
			swLane2->polyline_ = sim_mob::ShiftPolyline(lanes.front()->polyline_, lanes.front()->getWidth()/2+swLane2->getWidth()/2, false);
			lanes.insert(lanes.begin(), swLane2);
			width += swLane2->width_;
			laneEdgePolylines_cached.insert(laneEdgePolylines_cached.begin(), makeLaneEdgeFromPolyline(lanes[0], true));
		}
	}
}

#ifndef SIMMOB_DISABLE_MPI


#endif

vector<Point2D> sim_mob::RoadSegment::makeLaneEdgeFromPolyline(Lane* refLane, bool edgeIsRight) const
{
	//Sanity check
	if (refLane->polyline_.size()<=1) {
		throw std::runtime_error("Can't manage with a Lane polyline of 0 or 1 points.");
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
	//Sanity check
	if (outer.size()<=1 || inner.size()<=1) {
		throw std::runtime_error("Can't manage with a Lane Edge polyline of 0 or 1 points.");
	}

	//Get the offset of inner[0] to outer[0]
	double magX = outer.front().getX() - inner.front().getX();
	double magY = outer.front().getY() - inner.front().getY();
	double magTotal = sqrt(magX*magX + magY*magY);

	//Update the lane's width
	lane->width_ = magTotal;

	//Travel along the inner path. Essentially, the inner and outer paths should line up, but if there's an extra point
	// or two, we don't want our algorithm to go crazy.
	lane->polyline_.clear();
	for (vector<Point2D>::const_iterator it=inner.begin(); it!=inner.end(); it++) {
		DynamicVector line(it->getX(), it->getY(), it->getX()+magX, it->getY()+magY);
		line.scaleVectTo(magTotal/2.0).translateVect();
		lane->polyline_.push_back(Point2D(line.getX(), line.getY()));
	}
}


//TODO: Restore const-correctness after cleaning up sidewalks.
const vector<Point2D>& sim_mob::RoadSegment::getLaneEdgePolyline(unsigned int laneID) /*const*/
{
	//TEMP: Due to the way we manually insert sidewalks, this is needed for now.
	bool syncNeeded = false;
	for (size_t i=0; i<lanes.size(); i++) {
		if (lanes.at(i)->polyline_.empty()) {
			syncNeeded = true;
			break;
		}
	}

	//Rebuild if needed
	if (laneEdgePolylines_cached.empty() || syncNeeded) {
		syncLanePolylines();
	}
	return laneEdgePolylines_cached[laneID];
}

