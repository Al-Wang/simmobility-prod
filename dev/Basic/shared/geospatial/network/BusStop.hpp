//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <vector>
#include <stdexcept>
#include <map>

#include "RoadItem.hpp"
#include "RoadSegment.hpp"
#include "Point.hpp"

namespace sim_mob
{

/**Defines the various types of bus terminals*/
enum TerminusType
{
	NOT_A_TERMINUS = 0,
	SOURCE_TERMINUS,
	SINK_TERMINUS
};

class BusStop : public RoadItem
{
private:
	/**bus stop location*/
	Point location;

	/**The road segment that contains the bus stop*/
	RoadSegment *roadSegment;

	/**Control flag for bus terminus stops*/
	TerminusType terminusType;

	/**How many meters of "bus" can park in the bus lane/bay to pick up pedestrians*/
	unsigned int capacityAsLengthCM;

	/**store bus line info at each bus stop for passengers*/
	//std::vector<Busline> busLines;

	/**pointer to the twin bus stop. Contains value if this bus stop is a terminus*/
	const BusStop *twinStop;

	/**indicator to determine whether this stop is virtually created for terminus stops*/
	bool virtualStop;

	/**offset to its parent segment's obstacle list*/
	double offset;

	/**bus stop name*/
	std::string stopName;

public:	
	BusStop();
	virtual ~BusStop();

	const Point& getStopLocation() const;
	void setStopLocation(Point& location);

	const RoadSegment* getParentSegment() const;
	void setParentSegment(RoadSegment *roadSegment);

	const TerminusType& getTerminusType() const;
	void setTerminusType(TerminusType type);

	unsigned int getCapacityAsLength() const;
	void setCapacityAsLength(unsigned int len);

	//void addBusLine(Busline& line);
	//const std::vector<Busline>& getBusLine() const;

	const BusStop* getTwinStop() const;
	void setTwinStop(const BusStop* stop);

	bool isVirtualStop() const;
	void setVirtualStop(bool val);

	double getOffset() const;
	void setOffset(double val);

	const std::string& getStopName() const;
	void setStopName(std::string name);
};

}
