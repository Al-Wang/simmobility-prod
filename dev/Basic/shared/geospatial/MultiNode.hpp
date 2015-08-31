//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <vector>
#include <map>
#include <set>

#include "geospatial/Node.hpp"
#include "TurningSection.hpp"
/*namespace geo {
class intersection_t_pimpl;
class GeoSpatial_t_pimpl;
}*/

namespace sim_mob
{


//Forward declarations
class Point2D;
class Link;
class Lane;
class LaneConnector;
class TurningSection;
class RoadSegment;
class RoadNetwork;

namespace aimsun
{
//Forward declarations
class Loader;
}



class MultiNode : public sim_mob::Node {
public:
	MultiNode(int x, int y, bool hasSignal=false) : Node(x, y), hasTrafficSignal(hasSignal) {}

	///Query the list of connectors at the current node, restricting the results to
	///   those which originate at the "from" segment.
	///Fails if no outgoing Lanes exist.
	///The reference to this vector may be invalidated when a new connector is added
	///   to this Node which links from a Lane* that is not currently linked.
	const std::set<sim_mob::LaneConnector*>& getOutgoingLanes(const sim_mob::RoadSegment* from) const;

	///Test if connetors exist at this node.
	bool hasOutgoingLanes(const sim_mob::RoadSegment* from) const;

	///Retrieve a list of all RoadSegments at this node.
	const std::set<sim_mob::RoadSegment*>& getRoadSegments() const { return roadSegmentsAt; }
	bool canFindRoadSegment(sim_mob::RoadSegment* rs) const;
	//Determine which RoadSegments (and in which direction) you have to cross as a pedestrian approaching this
	//  MultiNode
	//TODO: The return value will _definitely_ be a class.
	std::pair< std::vector< std::pair<sim_mob::RoadSegment*, bool> >, std::vector< std::pair<sim_mob::RoadSegment*, bool> > >
		getPedestrianPaths(const sim_mob::Node* const nodeBefore, const sim_mob::Node* const nodeAfter) const;

	//seth suggested its removal for good reasons-vahid
//	static void BuildClockwiseLinks(const sim_mob::RoadNetwork& rn, sim_mob::MultiNode* node);
	const std::map<const sim_mob::RoadSegment*, std::set<sim_mob::LaneConnector*> > & getConnectors() const {return connectors;}
	const std::map<const sim_mob::RoadSegment*, std::set<sim_mob::LaneConnector*> > & getConnectors()  {return connectors;}

	//TEMP: Added for XML loading.
	void setConnectorAt(const sim_mob::RoadSegment* key, std::set<sim_mob::LaneConnector*>& val) { this->connectors[key] = val; }
	// increase the laneconnectors if the roadsegment already is key in connectors
	void setConnectorAt2(const sim_mob::RoadSegment* key, std::set<sim_mob::LaneConnector*>& val);
	void addRoadSegmentAt(sim_mob::RoadSegment* rs) { roadSegmentsAt.insert(rs); }

	/// Set the Turnings for the multi-node
	void setTurnings(const sim_mob::RoadSegment *key, TurningSection *val);
 
	bool isSignalized() const;

	/// Inserts the turning section to the map mapFromToLanesVsTurning
	void updateMapLaneVsTurning(const Lane *fromLane, const Lane *toLane, TurningSection *turning);
        
	/// Finds and returns the TurningSection object that connects the given 'from' and 'to' lanes
	const TurningSection* getTurningSection(const Lane *currentLane, const Lane *nextLane) const;
    
	/// Returns the turnings from the given road segment
	const std::set<TurningSection*>& getTurnings(const RoadSegment *) const;
        
	//Returns the turning with the given id
	const TurningSection* getTurning(int id) const;

protected:
	///Mapping from RoadSegment* -> set<LaneConnector*> representing lane connectors.
	///Currently allows one to make quick requests upon arriving at a Node of which
	//  LaneConnectors are available _from_ the segment you are arriving on.
	//NOTE: A multimap is another option here, but a map of vectors is probably easier on
	//      developers (who might not be familiar with multimaps).
	//      In addition, the multimap wouldn't be able to handle a uniqueness qualifier (which
	//      is why we use "set").
	std::map<const sim_mob::RoadSegment*, std::set<sim_mob::LaneConnector*> > connectors;

	/// Mapping from RoadSegment* to set<TurningSection> representing the Turnings
	std::map<const sim_mob::RoadSegment*, std::set<sim_mob::TurningSection*> > turnings;
    
	//Mapping of turnings with their ids
	std::map<int, sim_mob::TurningSection *> mapOfIdsVsTurnings;    
        
	/// This is a 2-level map of TurningSection with key as the the from lane and to lane
    std::map<const Lane *, std::map<const Lane *, TurningSection *> > mapFromToLanesVsTurning;

	///Bookkeeping: which RoadSegments meet at this Node?
	std::set<sim_mob::RoadSegment*> roadSegmentsAt;

	///flag to track whether this multinode has a Traffic Signal
	bool hasTrafficSignal;

friend class sim_mob::aimsun::Loader;

};





}
