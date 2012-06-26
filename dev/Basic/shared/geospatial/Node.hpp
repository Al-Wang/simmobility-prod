/* Copyright Singapore-MIT Alliance for Research and Technology */

#pragma once

#include <vector>
#include <map>
#include <set>
#include <string>

#include "util/LangHelpers.hpp"
#include "util/OpaqueProperty.hpp"
#include "Point2D.hpp"
namespace sim_mob
{
//Forward declarations
class Link;
class Lane;
class LaneConnector;
class RoadSegment;

#ifndef SIMMOB_DISABLE_MPI
class PackageUtils;
class UnPackageUtils;
#endif

/**
 * A location on a map where other elements interact. Nodes contain a Point2D representing their
 * location. Additional information (such as lane connectors) are located in other classes (e.g.,
 * Intersections, Roundabouts, and SegmentNodes.
 *
 * \author Seth N. Hetu
 * \author LIM Fung Chai
 *
 * Nodes should not be constructed directly. Instead, they exist to provide a uniform interface
 * to define locations in a RoadNetwork. UniNodes and MultiNodes (and their subclasses) provide
 * more comprehensive functionality, and their sub-classes provide even more.
 */
class Node {
	unsigned int nodeId;//read from DB
public:
	virtual ~Node() {} //A virtual destructor allows dynamic casting

	///The location of this Node.
	Point2D const location;


	//Nodes may have hidden properties useful only in for the visualizer.
	OpaqueProperty<int> originalDB_ID;

#ifndef SIMMOB_DISABLE_MPI
	///The identification of Node is packed using PackageUtils;
	static void pack(PackageUtils& package, const Node* one_node);

	///UnPackageUtils use the identification of Node to find the Node Object
	static Node* unpack(UnPackageUtils& unpackage);
#endif

protected:
    Node(int x, int y) : location(x, y) {}

private:
    sim_mob::Link* linkLoc;

public:
    void setID(unsigned int);
    unsigned int getID()const;
    void setLinkLoc(sim_mob::Link* link);
    sim_mob::Link* getLinkLoc();
};



}
