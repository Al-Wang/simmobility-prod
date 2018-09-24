//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <vector>
#include <string>
#include <cmath>

#include "util/LangHelpers.hpp"

#include "Base.hpp"

namespace sim_mob
{

//Forward declarations


namespace aimsun
{

//Forward declarations
class Section;


///An AIMSUN lane that is not a Crossing.
///   Lanes don't have an ID, but they still extend Base() to get access to the write flag.
///   Lanes share the same basic data types as Crossings, but they have different derived types.
/// \author Seth N. Hetu
/// \author LIM Fung Chai
class Lane : public Base {
public:
    int laneID;
    int rowNo;
    std::string laneType;
    Section* atSection;

    double xPos;
    double yPos;

    Lane() : Base(), atSection(nullptr) {}

    //Placeholders
    int TMP_AtSectionID;

    //Decorated data

};


}
}
