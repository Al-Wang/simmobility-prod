/* Copyright Singapore-MIT Alliance for Research and Technology */
#pragma once
#include "A_StarShortestPathImpl.hpp"
#include "AStarShortestTravelTimePathImpl.hpp"
#include "StreetDirectory.hpp"

#include <map>
#include <vector>
#include <string>

namespace sim_mob {

class PathSet;
class SinglePath;

class K_ShortestPathImpl {
public:
	K_ShortestPathImpl();
	virtual ~K_ShortestPathImpl();
	static boost::shared_ptr<K_ShortestPathImpl> instance;
public:
	static boost::shared_ptr<K_ShortestPathImpl> getInstance();
	/**
	 * Main Operation of this Class: Find K-Shortest Paths
	 * \param from Oigin
	 * \param to Destination
	 * \param res generated paths
	 * \return number of paths found
	 * Note: naming conventions follows the Huang HE's documented algorithm.
	 */
	int getKShortestPaths(const sim_mob::Node *from, const sim_mob::Node *to, std::vector< std::vector<sim_mob::WayPoint> > &res);

	void setK(int value) { k = value; }
	int getK() { return k; }
private:
	int k;
	/**
	 * store all segments in A, key=id, value=road segment
	 */
	std::map< std::string,const sim_mob::RoadSegment* > A_Segments;
	StreetDirectory* stdir;
	/**
	 * initialization
	 */
	void init();
	/**
	 * Obtain the end segments of a given node, as per requirement of Yen's shortest path
	 * (mentioned in He's Algorithm)
	 * \param SpurNode input
	 * \paream endSegments output
	 */
	void getEndSegments(const Node *SpurNode,std::set<const RoadSegment*> &endSegments);
	/**
	 * store intermediary result into A_Segments(get segments list of the path)
	 */
	void storeSegments(std::vector<sim_mob::WayPoint> path);
	/**
	 * Validates the intermediary results
	 * \param RootPath root path of the k-shortest path
	 * \param spurPath spur path of the k-shortest path
	 * \return true if all the validations are valid, false otherwise
	 */
	bool validatePath(const std::vector<sim_mob::WayPoint> &RootPath, const std::vector<sim_mob::WayPoint>&SpurPath);
};

} //end namespace sim_mob
