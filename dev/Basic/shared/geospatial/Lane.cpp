//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Lane.hpp"

#include <cmath>
#include <cassert>
#include <limits>
#include <sstream>

#include "conf/settings/DisableMPI.h"

#include "geospatial/RoadSegment.hpp"
#include "util/DynamicVector.hpp"
#include "util/GeomHelpers.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#endif

using std::vector;
using namespace sim_mob;

namespace {
    // Return the distance between the middle of the lane specified by <thisLane> and the middle
    // of the road-segment specified by <segment>; <thisLane> is one of the lanes in <segment>.
    double mid_of_lane_and_seg(const Lane& thisLane, const RoadSegment& segment)
    {
        //Retrieve half the segment width
        //double w = segment.width / 2.0;
        if (segment.width==0) {
        	//We can use default values here, but I've already hardcoded 300cm into too many places. ~Seth
        	throw std::runtime_error("Both the segment and all its lanes have a width of zero.");
        }
        double w = 0; //Note: We should be incrementing, right? ~Seth

        //Maintain a default lane width
        double defaultLaneWidth = static_cast<double>(segment.width) / segment.getLanes().size();

        //Iterate through each lane, reducing the return value by each lane's width until you reach the current lane.
        // At that point, reduce by half the lane's width and return.
        for (vector<Lane*>::const_iterator it=segment.getLanes().begin(); it!=segment.getLanes().end(); it++) {
        	double thisLaneWidth = (*it)->getWidth()>0 ? (*it)->getWidth() : defaultLaneWidth;
            if (*it != &thisLane) {
                w += thisLaneWidth;
            } else {
                w += (thisLaneWidth / 2.0);
                return w;
            }
        }

        //Exceptions don't require a fake return.
        //But we still need to figure out whether we're using assert() or exceptions for errors! ~Seth
        throw std::runtime_error("mid_of_lane_and_seg() called on a Lane not in this Segment.");
    }


}  //End anonymous namespace.



sim_mob::Lane::Lane(sim_mob::RoadSegment* segment, unsigned long laneID, const std::string& bit_pattern)
	: parentSegment_(segment), rules_(bit_pattern), width_(0), laneID_(0)
{
	if (segment) {
		//10 lanes per segment
		//TODO: This is not a reasonable restriction. ~Seth
		laneID_ = segment->getSegmentID()*10 + laneID;/*10 lanes per segment*/
	}
}


sim_mob::RoadSegment* sim_mob::Lane::getRoadSegment() const
{
    return parentSegment_;
}

unsigned int sim_mob::Lane::getWidth() const
{
	if (width_==0) {
		unsigned int width = parentSegment_->width / parentSegment_->getLanes().size();
		if(width <= 0) {
			return 50; //0.5 m should be visible; better than throwing an error.
		}
		return width;
	}
	return width_;
}

unsigned int sim_mob::Lane::getWidth_real() const
{
	return width_;
}

unsigned int sim_mob::Lane::getLaneID() const
{
	return laneID_;
}



//This contains most of the functionality from getPolyline(). It is called if there
// is no way to determine the polyline from the lane edges (i.e., they don't exist).
void Lane::makePolylineFromParentSegment()
{
	double distToMidline = mid_of_lane_and_seg(*this, *parentSegment_);
	if (distToMidline==0) {
		throw std::runtime_error("No side point for line with zero magnitude.");
	}

	//Set the width if it hasn't been set
	if (width_==0) {
		width_ = parentSegment_->width/parentSegment_->getLanes().size();
	}

	//Save
	const vector<Point2D>& poly = parentSegment_->polyline;
	polyline_ = sim_mob::ShiftPolyline(poly, distToMidline);
}


/*TODO: Some one has to fix this item. it is not an ordinarry getter function! this function doesn't
 * return polylines as it should. it 'creates' the polylines if the container is empty!!!
 * so if the function is used in any place other than what was originally ment to be used in,
 * it wn't show any flexibility ands messes up iterators, creates seg faults etc.
 * vahid
 * */
const std::vector<sim_mob::Point2D>& sim_mob::Lane::getPolyline(bool sync) const
{
    //Recompute the polyline if needed
    if (polyline_.empty() && sync) {
    	parentSegment_->syncLanePolylines();
    }
    return polyline_;
}

void sim_mob::Lane::insertNewPolylinePoint(Point2D p, bool isPre)
{
	if(isPre)
		polyline_.push_back(p);
	else
		polyline_.insert(polyline_.begin(), p);
}

void sim_mob::Lane::setParentSegment(sim_mob::RoadSegment* segment)
{
	parentSegment_ = segment;
}

void sim_mob::Lane::set(const std::string& bit_pattern)
{
	std::istringstream stream(bit_pattern);
	stream >> rules_;
}


std::string sim_mob::Lane::to_string() const
{
	return rules_.to_string();
}

