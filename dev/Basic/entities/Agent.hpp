#pragma once

#include <vector>

#include <boost/thread.hpp>

#include "../constants.h"
#include "Entity.hpp"
#include "../buffering/Buffered.hpp"
#include "../buffering/BufferedDataManager.hpp"
#include "../simple_classes.h"


namespace sim_mob
{


/**
 * Possible agent behaviors.
 *
 * \todo
 * Represent agent behavior using inheritance, instead.
 */
enum AGENT_MODES {
	DRIVER,
	PEDESTRIAN,
	CYCLIST,
	PASSENGER
};



/**
 * Basic Agent class. Agents maintain an x and a y position. They may have different
 * behavioral models.
 */
class Agent : public sim_mob::Entity {
public:
	Agent(unsigned int id=0);

	virtual void update(frame_t frameNumber);  ///<Update agent behvaior

	///Subscribe this agent to a data manager.
	virtual void subscribe(sim_mob::BufferedDataManager* mgr, bool isNew);

	///Update the agent's shortest path. (Currently does nothing; might not even belong here)
	void updateShortestPath();

public:
//	sim_mob::Buffered<unsigned int> xPos;  ///<The agent's position, X
//	sim_mob::Buffered<unsigned int> yPos;  ///<The agent's position, Y
	sim_mob::Buffered<double> xPos;  ///<The agent's position, X
	sim_mob::Buffered<double> yPos;  ///<The agent's position, Y

	//Agents can access all other agents (although they usually do not access by ID)
	static std::vector<Agent*> all_agents;

	//TEMP; we can't link to the config file directly or we get a circular dependency.
	Point topLeft;
	Point lowerRight;
	Point topLeftCrossing;
	Point lowerRightCrossing;

private:
	unsigned int currMode;
	double speed;
	double xVel;
	double yVel;
	Point goal;
	bool isGoalSet;
	bool toRemoved;
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

public:
	//TEMP
	static boost::mutex global_mutex;


//TODO: Move these into the proper location (inheritance, for most of them)
public:
	static void pathChoice(Agent& a);
	static void updateDriverBehavior(Agent& a);
	static void updatePedestrianBehavior(Agent& a);
	static void updatePassengerBehavior(Agent& a);
};

}

