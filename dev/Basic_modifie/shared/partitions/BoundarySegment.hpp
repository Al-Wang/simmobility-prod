//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * BoudarySegment.hpp
 * Target:
 * 1. has one road segment
 * 2. has attributes about the partition issues
 */

#pragma once

#include <vector>

#include "geospatial/network/Point.hpp"

namespace sim_mob
{

class RoadSegment;

/**
 * \author Xu Yan
 */
class BoundarySegment
{
public:
	const RoadSegment* boundarySegment;

	double cutLineOffset;
	int responsible_side; //-1 for upper stream; 1 for down stream
	int connected_partition_id; //the partition id of the other side

	//temp setting, need to change to id to make logic clear
	double start_node_x;
	double start_node_y;
	double end_node_x;
	double end_node_y;

	//boundary box
	Point* cut_line_start;
	Point* cut_line_to;

	std::vector<Point> bounary_box;

public:
	void buildBoundaryBox(double width, double height);
	void output();
};

}


