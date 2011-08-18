/* Copyright Singapore-MIT Alliance for Research and Technology */

#pragma once

#include <time.h>
#include <math.h>

#include "../Role.hpp"
#include "../../../geospatial/Point2D.hpp"

#include "../../../conf/simpleconf.hpp"

namespace sim_mob
{

/**
 * A Person in the Pedestrian role is navigating sidewalks and zebra crossings.
 */
class Pedestrian : public sim_mob::Role {
public:
	Pedestrian(Agent* parent);

	virtual void update(frame_t frameNumber);

private:
	//Movement-related variables
	double speed;
	double xVel;
	double yVel;
	Point2D goal;
	bool isGoalSet;
	unsigned int currPhase; //Current pedestrian signal phase: 0-green, 1-red
	unsigned int phaseCounter; //To be replaced by traffic management system

	//For collisions
	double xCollisionVector;
	double yCollisionVector;
	static double collisionForce;
	static double agentRadius;

	//The following methods are to be moved to agent's sub-systems in future
	bool isGoalReached();
	void setGoal();
	void updateVelocity();
	void updatePosition();
	void updatePedestrianSignal();
	void checkForCollisions();
	bool reachStartOfCrossing();
	bool checkGapAcceptance();

};



}