//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once
#include "conf/settings/DisableMPI.h"
#include "Driver.hpp"
#include "DriverUpdateParams.hpp"
#include "entities/roles/RoleFacets.hpp"
#include "entities/conflux/SegmentStats.hpp"
#include "MesoPathMover.hpp"

namespace sim_mob {
namespace medium {

class Driver;

/**
 * Behaviour Facet class for mid-term Driver role
 *
 * \author Melani Jayasuriya
 * \author Harish Loganathan
 */
class DriverBehavior: public sim_mob::BehaviorFacet {
public:
	explicit DriverBehavior(sim_mob::Person* parentAgent = nullptr);
	virtual ~DriverBehavior();

	/**
	 * Virtual overrides
	 */
	virtual void frame_init();
	virtual void frame_tick();
	virtual void frame_tick_output();

	sim_mob::medium::Driver* getParentDriver() const {
		return parentDriver;
	}

	void setParentDriver(sim_mob::medium::Driver* parentDriver) {
		if(!parentDriver) {
			throw std::runtime_error("parentDriver cannot be NULL");
		}
		this->parentDriver = parentDriver;
	}

	sim_mob::medium::Driver* getParentDriver();

protected:
	/**
	 * Pointer to the parent Driver role.
	 */
	sim_mob::medium::Driver* parentDriver;

};

/**
 * Movement Facet class for mid-term Driver role
 *
 * \author Melani Jayasuriya
 * \author Harish Loganathan
 */
class DriverMovement: public sim_mob::MovementFacet {
public:
	explicit DriverMovement(sim_mob::Person* parentAgent = nullptr);
	virtual ~DriverMovement();

	//virtual overrides
	virtual void frame_init();
	virtual void frame_tick();
	virtual void frame_tick_output();

	sim_mob::medium::Driver* getParentDriver() const {
		return parentDriver;
	}

	void setParentDriver(sim_mob::medium::Driver* parentDriver) {
		if(!parentDriver) {
			throw std::runtime_error("parentDriver cannot be NULL");
		}
		this->parentDriver = parentDriver;
	}


protected:
	/**
	 * Pointer to the parent Driver role.
	 */
	sim_mob::medium::Driver* parentDriver;

	MesoPathMover pathMover;
	const Lane* currLane;
	const Lane* laneInNextSegment;
	bool isQueuing;

	mutable std::stringstream DebugStream;

	/**
	 * For moving into a new link after getting permission from the managing conflux
	 *
	 * @param params driver update params for current tick
	 */
	void flowIntoNextLinkIfPossible(DriverUpdateParams& params);

	/**
	 * sets fields in the parent person so that the conflux can manage the person
	 *
	 * @param params driver update params for current tick
	 */
	void setParentData(DriverUpdateParams& params);	///<set next data to parent buffer data

	/**
	 * adds to the time spent by driver in current tick
	 *
	 * @param params driver update params for current tick
	 */
	void stepFwdInTime(DriverUpdateParams& params, double time);

	/**
	 * moves the driver forward along its path
	 *
	 * @param params driver update params for current tick
	 * @return true if advance was successful; false otherwise
	 */
	bool advance(DriverUpdateParams& params);

	/**
	 * moves driver to the next segment
	 *
	 * @param params driver update params for current tick
	 * @return true if successfully moved to next segment; false otherwise
	 */
	bool moveToNextSegment(DriverUpdateParams& params);

	/**
	 * checks whether the driver can move into the next segment in path
	 *
	 * @param params driver update params for current tick
	 * @param nextSegStats next segment stats in path
	 * @return true if driver can move into the next segment; false otherwise
	 */
	bool canGoToNextRdSeg(DriverUpdateParams& params, const sim_mob::SegmentStats* nextSegStats);

	/**
	 * sets position of driver in queue
	 */
	void moveInQueue();

	/**
	 * move driver forward within the seg stats
	 *
	 * @param distance distance to move forward
	 * @return status of move (success = true, failure = false)
	 */
	bool moveInSegment(double distance);

	/**
	 * move driver forward in queue
	 *
	 * @param params driver update params for current tick
	 * @return status of move (success = true, failure = false)
	 */
	bool advanceQueuingVehicle(DriverUpdateParams& params);

	/**
	 * move driver forward in the moving part of seg stats
	 *
	 * @param params driver update params for current tick
	 * @return status of move (success = true, failure = false)
	 */
	bool advanceMovingVehicle(DriverUpdateParams& params);

	/**
	 * move driver forward in seg stats which had a queue at the start of the current tick
	 * but the queue has dissipated within this tick
	 *
	 * @param params driver update params for current tick
	 * @return status of move (success = true, failure = false)
	 */
	bool advanceMovingVehicleWithInitialQ(DriverUpdateParams& params);

	/**
	 * get the number of vehicles that can move out of a lane in this tick
	 *
	 * @param l lane in segment
	 * @param segStats segment stats corresponding to lane l's segment
	 * @return num. of vehicles that can move out
	 */
	int getOutputCounter(const Lane* lane, const sim_mob::SegmentStats* segStats);

	/**
	 * set number of vehicles that can move out of a lane in this tick
	 *
	 * @param l lane in segment
	 * @param count new value of outpur counter
	 * @param segStats segStats segment stats corresponding to lane l's segment
	 */
	void setOutputCounter(const Lane* lane, int count, const sim_mob::SegmentStats* segStats);

	double getOutputFlowRate(const Lane* lane);
	double getAcceptRate(const Lane* lane, const sim_mob::SegmentStats* segStats);
	double getQueueLength(const Lane* lane);
	double getLastAccept(const Lane* lane, const sim_mob::SegmentStats* segStats);
	void setLastAccept(const Lane* lane, double lastAccept, const sim_mob::SegmentStats* segStats);

	/**
	 * update flow of segment segStats
	 * @param segStats segment stats whose flow is to be updated
	 * @param startPos position of driver at the start of the tick
	 * @param endPos final position of driver
	 */
	void updateFlow(const sim_mob::SegmentStats* segStats, double startPos, double endPos);

	/**
	 * constructs the path for this driver if required.
	 * converts the path into a list of SementStats*.
	 * sets the path in the path mover.
	 * @return true if the path has successfully been set; false otherwise.
	 */
	virtual bool initializePath();

	/**
	 * adds driver to his starting lane
	 * @param params driver update params for current tick
	 */
	void setOrigin(DriverUpdateParams& params);

	/**
	 * get the length of queue in lane at the start of current tick
	 * @param lane the lane for which queue length is required
	 * @return initial length of queue
	 */
	double getInitialQueueLength(const Lane* lane);

	/**
	 * checks if lane is connected to the next segment stats
	 *
	 * @param lane current lane
	 * @param nextSegStats next segment stats
	 * @return true if lane is connected to nextSegStats; false otherwise
	 */
	bool isConnectedToNextSeg(const Lane* lane, const SegmentStats* nextSegStats);

	/**
	 * add driver to queue
	 *
	 * @param lane lane in which the driver is added to queue
	 */
	void addToQueue(const Lane* lane);

	/**
	 * remove driver from queue
	 */
	void removeFromQueue();

	/**
	 * get the best lane in next segment for driver to move into
	 *
	 * @param nextSegStats next segment stats
	 * @param nextToNextSegStats second segment stats ahead from the current
	 * @return best lane in nextSegStats
	 */
	virtual const sim_mob::Lane* getBestTargetLane(const sim_mob::SegmentStats* nextSegStats, const SegmentStats* nextToNextSegStats);

	//Note: insert and remove incident functions should probably be in Confluxes. To be updated when actual incident functionality is implemented.
	/**
	 * Inserts an Incident by updating the flow rate for all lanes of a road segment to a new value.
	 *
	 * @param rdSeg roadSegment to insert incident
	 * @param newFlowRate new flow rate to be updated
	 */
	void insertIncident(sim_mob::SegmentStats* segStats, double newFlowRate);

	/**
	 * Removes a previously inserted incident by restoring the flow rate of each lane of a road segment to normal values
	 *
	 * @param segStats road segment stats to remove incident
	 */
	void removeIncident(sim_mob::SegmentStats* segStats);

	/**
	 * Updates travel time for this driver for the link which he has just exited from.
	 *
	 * @param prevSeg the last segment in the link from which the driver has just exited
	 * @param linkExitTimeSec time at which the link was exited
	 */
	void updateLinkTravelTimes(const sim_mob::SegmentStats* prevSegStat, double linkExitTimeSec);

	/**
	 * Updates travel time for this driver for the road segment which he has just exited from.
	 *
	 * @param prevSeg the segment from which the driver has just exited
	 * @param linkExitTimeSec time at which the segment was exited
	 */
	void updateRdSegTravelTimes(const sim_mob::SegmentStats* prevSegStat, double linkExitTimeSec);
};

} /* namespace medium */
} /* namespace sim_mob */
