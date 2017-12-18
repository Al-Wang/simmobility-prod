//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include "DriverFacets.hpp"

namespace sim_mob
{

class OnCallDriver;

class OnCallDriverMovement : public DriverMovement
{
private:
	/**The on call driver to which this movement object belongs*/
	OnCallDriver *onCallDriver;

	/**Indicates the current node of the vehicle.*/
	const Node *currNode;


protected:
	/**
	 * This method looks up the path for driving to the node from the current
	 * position and begins the drive towards it
	 * @param node the chosen node to cruise to
	 */
	void beginCruising(const Node *node);
	/**
	 * This method enables the driver to continue cruising, one link at a time
	 * @param fromNode the node the driver has reached when the defined path has been traversed
	 * and wants to continue cruising
	 */
	void continueCruising(const Node *fromNode);

public:
	OnCallDriverMovement();
	virtual ~OnCallDriverMovement();

	/**
	 * This method performs the initialisation required by the movement
	 */
	virtual void frame_init();

	/**
	 * Performs the movement related updates for the agent in the OnHailDriver role
	 * for the current tick
	 */
	virtual void frame_tick();

	/**
	 * Collects the movement related output for the agent in the OnHailDriver role
	 * for the current tick
	 * @return the output of the current tick
	 */
	virtual std::string frame_tick_output();

	/**
	 * This method allows us to perform the task in the assigned schedule
	 */
	virtual void performScheduleItem();
    /**
     * Handles the movement into the next segment
     * @param params the driver update parameters
     * @return true if successfully moved to next segment, false otherwise
     */
  //  virtual bool moveToNextSegment(DriverUpdateParams &params);

	void setOnCallDriver(OnCallDriver *driver)
	{
		onCallDriver = driver;
	}

	const Node* getCurrentNode() const
	{
		return currNode;
	}

        Vehicle* initialisePath(bool createVehicle);

    void beginDriveToPickUpPoint(const Node *pickupNode);
    /**
 * Stores the stopping points for the driver
 * Key = segment id, value= vector of stopping points (one segment may have more than one stop point)
 */
    std::map<unsigned int, std::vector<StopPoint> > stopPointPool;

    void insertStopPoint(StopPoint stopPt);
};

class OnCallDriverBehaviour : public DriverBehavior
{
private:
	/**The on call driver to which this behaviour object belongs*/
	OnCallDriver *onCallDriver;

public:
	OnCallDriverBehaviour()
	{
	}

	~OnCallDriverBehaviour()
	{
	}

	/**
	 * This method chooses a random downstream node to cruise to. This method is intended to be
	 * used when the driver is supposed to continue cruising (after arriving at the node specified
	 * by the controller) or from frame_init, where the driver has no initial path
	 * @param fromNode the node from which we want to move/continue moving
	 * @return the chosen Node
	 */
	const Node *chooseDownstreamNode(const Node *fromNode) const;

	/**
	 * Checks if the driver's shift has ended
	 * @return true if the driver's shift has ended, false otherwise
	 */
	bool hasDriverShiftEnded() const;

	void setOnCallDriver(OnCallDriver *driver)
	{
		onCallDriver = driver;
	}
};

}