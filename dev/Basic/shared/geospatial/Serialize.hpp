/* Copyright Singapore-MIT Alliance for Research and Technology */

/**
 * \file Serialize.hpp

 * This file contains a set of serialization functions (boost::serialize) for the Geospatial road
 *   network classes which are compatible with boost::xml.
 * Be careful when including this file, as it has a large number of dependencies coupled with template
 *   expansion.
 *
 * \note
 * In terms of justification for why we put all of these functions outside of their respective classes,
 * there are three. First, I don't want tiny classes like Point2D and Node having boost::xml as a
 * dependency. Second, no "friends" are needed if the proper accessors are provided. Third, a friend
 * class would have been needed anyway in boost::serialization::access, so we might as well remove
 * these functions. The downside is that changes to the RoadNetwork classes will force Serialize.hpp to
 * recompile, but that is mostly unavoidable (and the classes are mostly stable now anyway). If performance
 * really becomes a problem, we can force the template into a separate cpp file (although it's messy).
 *
 * \note
 * These serialization functions are used for *writing* only; they are untested and unintended for reading.
 *
 * \author Seth N. Hetu
 */


#pragma once

#include <set>
#include <vector>
#include <iostream>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>

#include "util/XmlWriter.hpp"

#include "geospatial/RoadNetwork.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/MultiNode.hpp"
#include "geospatial/Link.hpp"
#include "geospatial/Lane.hpp"
#include "geospatial/Point2D.hpp"


namespace sim_mob {
namespace xml {

/////////////////////////////////////////////////////////////////////
// get_id()
/////////////////////////////////////////////////////////////////////

template <>
std::string get_id(const sim_mob::RoadSegment& rs)
{
	return boost::lexical_cast<std::string>(rs.getSegmentID());
}

template <>
std::string get_id(const sim_mob::Lane& ln)
{
	return boost::lexical_cast<std::string>(ln.getLaneID());
}

/////////////////////////////////////////////////////////////////////
// write_xml()
/////////////////////////////////////////////////////////////////////

template <>
void write_xml(XmlWriter& write, const sim_mob::Link& lnk)
{
	//TEMP
    write.prop("TODO", 2);
}

template <>
void write_xml(XmlWriter& write, const sim_mob::MultiNode& mnd)
{
	//TEMP
    write.prop("TODO", 2);
}

template <>
void write_xml(XmlWriter& write, const sim_mob::RoadSegment& rs)
{
	//TEMP
    write.prop("TODO", 2);
}

template <>
void write_xml(XmlWriter& write, const sim_mob::LaneConnector& lc)
{
	//TEMP
    write.prop("TODO", 2);
}

template <>
void write_xml(XmlWriter& write, const sim_mob::Lane& lc)
{
	//TEMP
    write.prop("TODO", 2);
}

//Force pairs of Lanes (connectors) to have a specific format (laneFrom/ID, etc.)
template <>
void write_xml(XmlWriter& write, const std::pair<const sim_mob::Lane*, sim_mob::Lane* >& connectors)
{
	write.ident("laneFrom", *connectors.first);
	write.ident("laneTo", *connectors.second);
}

template <>
void write_xml(XmlWriter& write, const sim_mob::Point2D& pt)
{
	write.prop("xPos", pt.getX());
	write.prop("yPos", pt.getY());
}

template <>
void write_xml(XmlWriter& write, const sim_mob::UniNode& und)
{
	write.prop("nodeID", und.nodeId);
	write.prop("location", und.location);
	write.prop("originalDB_ID", und.originalDB_ID.getLogItem());
	write.ident("firstPair", *und.firstPair.first, *und.firstPair.second);
	if (und.secondPair.first && und.secondPair.second) {
		write.ident("secondPair", *und.secondPair.first, *und.secondPair.second);
	}
	write.list("Connectors", "Connector", flatten_map(und.getConnectors()));
}

template <>
void write_xml(XmlWriter& write, const sim_mob::RoadNetwork& rn)
{
    write.prop_begin("RoadNetwork");

    //Nodes are also wrapped
    write.prop_begin("Nodes");
    write.list("UniNodes", "UniNode", rn.getUniNodes());
    write.list("Intersections", "Intersection", rn.getNodes());
    write.prop_end(); //Nodes

    write.list("Links", "Link", rn.getLinks());
	write.prop_end(); //RoadNetwork
}



}}
