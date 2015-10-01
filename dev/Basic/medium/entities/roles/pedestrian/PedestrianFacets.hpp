/*
 * PedestrainFacets.h
 *
 *  Created on: Mar 13, 2014
 *      Author: zhang huai peng
 */

#pragma once

#include "entities/roles/RoleFacets.hpp"
#include "entities/Person.hpp"
#include "geospatial/Link.hpp"

namespace sim_mob
{

class MRT_Stop;

namespace medium
{

class Pedestrian;

class PedestrianBehavior : public BehaviorFacet
{
public:
	explicit PedestrianBehavior();
	virtual ~PedestrianBehavior();

	//Virtual overrides

	virtual void frame_init()
	{
	}

	virtual void frame_tick()
	{
	}

	virtual void frame_tick_output()
	{
	}

	/**
	 * set parent reference to pedestrian.
	 * @param parentPedestrian is pointer to parent pedestrian
	 */
	void setParentPedestrian(sim_mob::medium::Pedestrian* parentPedestrian);

protected:
	sim_mob::medium::Pedestrian* parentPedestrian;

};

class PedestrianMovement : public MovementFacet
{
public:
	explicit PedestrianMovement(double speed);
	virtual ~PedestrianMovement();

	//Virtual overrides
	virtual void frame_init();
	virtual void frame_tick();
	virtual void frame_tick_output();
	virtual sim_mob::Conflux* getStartingConflux() const;

	/**
	 * set parent reference to pedestrian.
	 * @param parentPedestrian is pointer to parent pedestrian
	 */
	void setParentPedestrian(sim_mob::medium::Pedestrian* parentPedestrian);

	TravelMetric & startTravelTimeMetric();
	TravelMetric & finalizeTravelTimeMetric();
protected:

	/**
	 * initialize the path at the beginning
	 * @param path include aPathSetParams list of road segments
	 * */
	void initializePath(std::vector<const RoadSegment*>& path);

	/**
	 * choice shortest distance from node to mrt stop
	 */
	const sim_mob::RoadSegment* choiceNearestSegmentToMRT(const sim_mob::Node* src, const sim_mob::MRT_Stop* stop);

	/**parent pedestrian*/
	sim_mob::medium::Pedestrian* parentPedestrian;
	/**record the current remaining time to the destination*/
	double remainingTimeToComplete;
	/**pedestrian's walking speed*/
	const double walkSpeed;
	/**store the pair of link and walking time*/
	std::vector<std::pair<Link*, double> > trajectory;
	/**starting link*/
	sim_mob::Link* startLink;
	double totalTimeToCompleteSec;
};

}
}
