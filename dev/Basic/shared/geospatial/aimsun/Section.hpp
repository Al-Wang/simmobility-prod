//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <string>

#include "geospatial/network/Point.hpp"
#include "util/LangHelpers.hpp"
#include "util/DynamicVector.hpp"
#include "Base.hpp"

namespace sim_mob
{

//Forward declarations
class RoadSegment;

struct SegmentType
{
    std::string id;
    int type;
};
namespace aimsun
{

//Forward declarations
class Lane;
class Node;
class Turning;
class Polyline;

///An AIMSUN link or road segment
/// \author Seth N. Hetu
class Section : public Base {
public:
    std::string roadName;
    int numLanes;
    double speed;
    double capacity;
    double length;
    Node* fromNode;
    Node* toNode;
    std::string serviceCategory;

    Section() : Base(), fromNode(nullptr), toNode(nullptr), generatedSegment(nullptr),
            roadName(std::string()), serviceCategory(std::string()) {}

    //Placeholders
    int TMP_FromNodeID;
    int TMP_ToNodeID;

    //Decorated data
    std::vector<Turning*> connectedTurnings;
    std::vector<Polyline*> polylineEntries;
    std::map<int, std::vector<Lane*> > laneLinesAtNode; //Arranged by laneID
    std::vector< std::vector<sim_mob::Point> > lanePolylinesForGenNode;

    //Temporary fixings
    DynamicVector HACK_LaneLinesStartLineCut;
    DynamicVector HACK_LaneLinesEndLineCut;

    //Reference to saved object
    sim_mob::RoadSegment* generatedSegment;
};


}
}
