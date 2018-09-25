//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <map>
#include <string>
#include <vector>

#include "Lane.hpp"
#include "Link.hpp"
#include "PolyLine.hpp"
#include "RoadItem.hpp"

namespace sim_mob
{

class Link;
class Lane;
class RoadItem;
class SurveillanceStation;

/**
 * This class define the structure of a road segment
 * \author Neeraj D
 * \author Harish L
 */
class RoadSegment
{
private:

    /**Unique identifier for the road segment*/
    unsigned int roadSegmentId;

    /**Indicates the maximum number of vehicles that the road segment allow to exit this segment per second*/
    double capacity;

    /**The lanes that make up th road segment*/
    std::vector<const Lane *> lanes;

    /**The id of the link to which the segment belongs*/
    unsigned int linkId;

    /**Indicates the maximum speed the vehicles should adhere to when traveling on the road segment (m/s)*/
    double maxSpeed;

    /**The link to which this segment belongs*/
    Link *parentLink;

    /**Represents the poly-line for the road segment*/
    PolyLine *polyLine;

    /**The name of the road to which this segment belongs*/
    std::string roadName;

    /**Indicates the position of the road segment within the link*/
    unsigned int sequenceNumber;

    /**Obstacles generally include things like bus stops and crossing and incidents*/
    std::map<double, RoadItem *> obstacles;

    /**The traffic sensors on the road segment ordered by offset in metre.*/
    std::vector<SurveillanceStation *> surveillanceStations;

    /**Flag to indicate whether this segment exists only to hold a bus interchange*/
    bool busTerminusSegment;

public:
    RoadSegment();
    virtual ~RoadSegment();

    unsigned int getRoadSegmentId() const;
    void setRoadSegmentId(unsigned int roadSegmentId);

    double getCapacity() const;
    /**
     * sets the output capacity of this segment in vehicles/s
     * @param capacity capacity value to set in vehicles/hour
     */
    void setCapacity(unsigned int capacityVph);

    const std::vector<const Lane *>& getLanes() const;
    const Lane* getLane(int index) const;

    unsigned int getLinkId() const;
    void setLinkId(unsigned int linkId);

    double getMaxSpeed() const;
    void setMaxSpeed(double maxSpeedKmph);

    const Link* getParentLink() const;
    void setParentLink(Link *parentLink);

    PolyLine* getPolyLine() const;
    void setPolyLine(PolyLine *polyLine);

    unsigned int getSequenceNumber() const;
    void setSequenceNumber(unsigned int sequenceNumber);

    const std::map<double, RoadItem *>& getObstacles() const;
    unsigned int getNoOfLanes() const;

    bool isBusTerminusSegment() const;
    void setBusTerminusSegment();

    const std::vector<SurveillanceStation *>& getSurveillanceStations() const;

    /**
     * Gets the length of the road segment poly-line. This is equal to the length of the segment
     * @return length of the road segment
     */
    double getLength() const;

    /**
     * Adds a lane to the vector of lanes that make up the segment
     * @param lane - lane to be added
     */
    void addLane(const Lane *lane);

    /**
     * Adds an obstacle in the road segment. It can be a bus stop, a crossing or an incident
     * @param offset - the offset from the start of the segment at which the obstacle is to be added
     * @param item - the obstacle
     */
    void addObstacle(double offset, RoadItem *item);

    /**
     * Adds a traffic sensor to the road segment.
     * @param surveillanceStn - the traffic sensor
     */
    void addSurveillanceStation(SurveillanceStation *surveillanceStn);
};
}

