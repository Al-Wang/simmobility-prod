//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "PackingTreeAuraManager.hpp"
#include "spatial_trees/shared_funcs.hpp"

using namespace std;
using namespace sim_mob;
using namespace spatial;

PackingTreeAuraManager::PackingTreeAuraManager()
{
    tree = new RTree();
}

PackingTreeAuraManager::~PackingTreeAuraManager()
{
}

void PackingTreeAuraManager::update(int time_step, const std::set<sim_mob::Entity *> &removedAgentPointers)
{
    if(tree)
    {
        tree->clear();
        delete tree;
    }   

    std::vector<value> agentsToBeAdded;
    
    for (std::set<Entity *>::iterator itr = Agent::all_agents.begin(); itr != Agent::all_agents.end(); ++itr)
    {
        Agent *agent = dynamic_cast<Agent *> (*itr);
        if ((!agent) || agent->isNonspatial())
        {
            continue;
        }

        if (removedAgentPointers.find(agent) == removedAgentPointers.end())
        {
            point location(agent->xPos, agent->yPos);
            box agentPos(location, location);
            agentsToBeAdded.push_back(std::make_pair(agentPos, agent));
        }
    }
    
    tree = new RTree(agentsToBeAdded.begin(), agentsToBeAdded.end());
}

std::vector<const Agent*> PackingTreeAuraManager::agentsInRect(const Point &lowerLeft, const Point &upperRight, const sim_mob::Agent *refAgent) const
{
    box queryBox(point(lowerLeft.getX(), lowerLeft.getY()), point(upperRight.getX(), upperRight.getY()));
    vector<value> result;
    
    tree->query(bgi::intersects(queryBox), std::back_inserter(result));
    
    std::vector<const Agent *> agentsInRectangle;
    
    for(auto item : result)
    {
        agentsInRectangle.push_back(item.second);
    }
    
    return agentsInRectangle;
}

std::vector<const Agent*> PackingTreeAuraManager::nearbyAgents(const Point &position, const WayPoint &wayPoint, double distanceInFront, double distanceBehind, 
                                                               const sim_mob::Agent *refAgent) const
{
    // Find the stretch of the poly-line that <position> is in.
    std::vector<PolyPoint> points;
    
    if(wayPoint.type == WayPoint::LANE)
    {
        points = wayPoint.lane->getPolyLine()->getPoints();
    }
    else
    {
        points = wayPoint.turningPath->getPolyLine()->getPoints();
    }

    Point p1, p2;
    for (size_t index = 0; index < points.size() - 1; index++)
    {
        p1 = points[index];
        p2 = points[index + 1];
        if (isInBetween(position, p1, p2))
        {
            break;
        }
    }

    // Adjust <p1> and <p2>.  The current approach is simplistic.  <distanceInFront> and
    // <distanceBehind> may extend beyond the stretch marked out by <p1> and <p2>.
    adjust(p1, p2, position, distanceInFront, distanceBehind);

    // Calculate the search rectangle.  We use a quick and accurate method.  However the
    // inaccuracy only makes the search rectangle bigger.
    double left = 0, right = 0, bottom = 0, top = 0;
    if (p1.getX() > p2.getX())
    {
        left = p2.getX();
        right = p1.getX();
    }
    else
    {
        left = p1.getX();
        right = p2.getX();
    }
    if (p1.getY() > p2.getY())
    {
        top = p1.getY();
        bottom = p2.getY();
    }
    else
    {
        top = p2.getY();
        bottom = p1.getY();
    }

    double halfWidth = getAdjacentPathWidth(wayPoint) / 2;
    left -= halfWidth;
    right += halfWidth;
    top += halfWidth;
    bottom -= halfWidth;

    Point lowerLeft(left, bottom);
    Point upperRight(right, top);
    
    return agentsInRect(lowerLeft, upperRight, nullptr);
}
