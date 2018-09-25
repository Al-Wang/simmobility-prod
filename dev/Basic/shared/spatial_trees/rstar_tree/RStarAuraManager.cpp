//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "RStarAuraManager.hpp"

#include <cassert>

#include "spatial_trees/shared_funcs.hpp"
#include "entities/Person.hpp"
#include "entities/Entity.hpp"
#include "entities/Agent.hpp"

using namespace sim_mob;
using namespace sim_mob::spatial;

void RStarAuraManager::update(int time_step, const std::set<sim_mob::Entity *> &removedAgentPointers)
{
    tree_rstar.RemoveAll();
    assert(tree_rstar.GetSize() == 0);

    for (std::set<Entity *>::iterator itr = Agent::all_agents.begin(); itr != Agent::all_agents.end(); ++itr)
    {
        Agent *ag = dynamic_cast<Agent *> (*itr);
        if ((!ag) || ag->isNonspatial())
        {
            continue;
        }

        if (removedAgentPointers.find(ag) == removedAgentPointers.end())
        {
            tree_rstar.insert(ag);
        }
    }
}

std::vector<Agent const *> RStarAuraManager::agentsInRect(Point const &lowerLeft, Point const &upperRight, const sim_mob::Agent *refAgent) const
{
    R_tree::BoundingBox box;
    box.edges[0].first = lowerLeft.getX();
    box.edges[1].first = lowerLeft.getY();
    box.edges[0].second = upperRight.getX();
    box.edges[1].second = upperRight.getY();

    return tree_rstar.query(box);
}

std::vector<Agent const *> RStarAuraManager::nearbyAgents(Point const &position, WayPoint const &wayPoint, double distanceInFront, double distanceBehind, 
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

