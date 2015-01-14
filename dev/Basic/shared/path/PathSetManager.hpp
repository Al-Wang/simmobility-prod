/*
 * PathSetManager.hpp
 *
 *  Created on: May 6, 2013
 *      Author: Max
 *      Author: Vahid
 */

#pragma once

#include "Common.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/MultiNode.hpp"
#include "geospatial/aimsun/Loader.hpp"
#include "geospatial/Link.hpp"
#include "entities/Person.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "util/Profiler.hpp"
#include "util/Cache.hpp"
#include "util/Utils.hpp"

#include <boost/iterator/filter_iterator.hpp>

namespace sim_mob
{
namespace batched {
class ThreadPool;
}

/**
 * A structure to stor Origin and Destination in one pair
 * additional operator overload for assignment and comparisons
 */
class OD
{
private:
	std::string key;
public:
	OD(const sim_mob::WayPoint &origin, const sim_mob::WayPoint &destination):
		origin(origin), destination(destination)
	{
		std::stringstream str("");
		str << origin.node_->getID() << "," << destination.node_->getID();
		key = str.str();
	}
	OD(const sim_mob::Node * origin, const sim_mob::Node * destination):
		origin(sim_mob::WayPoint(origin)), destination(sim_mob::WayPoint(destination))
	{
		std::stringstream str("");
		str << origin->getID() << "," << destination->getID();
		key = str.str();
	}
	sim_mob::WayPoint origin;
	sim_mob::WayPoint destination;
	bool operator==(const OD & rhs) const
	{
		return (origin == rhs.origin && destination == rhs.destination);
	}

	bool operator!=(const OD & rhs) const
	{
		return !(*this == rhs);
	}

	OD & operator=(const OD & rhs)
	{
		origin = rhs.origin;
		destination = rhs.destination;
		return *this;
	}
	bool operator<(const OD & rhs) const
	{
		// just an almost dummy operator< to preserve uniquness
		return key < rhs.key;
	}
};
class PathSetManager;
/**
 * ProcessTT is a small helper class to process Real Time Travel Time at RoadSegment Level.
 * PathSetManager receives Real Time Travel Time and delegates
 * the processing task to this class.
 * This class aggregates the data received within different
 * time ranges and writes them to a file.
 */
class ProcessTT
{
	///	indicates the current time interval of the simulation
	const sim_mob::DailyTime simStartTime;

	/**
	 * time interval value used for processing data.
	 * This value is based on its counterpart in pathset manager.
	 */

	const unsigned int intervalMS;
	/**
	 * current time interval, with respect to simulation time
	 * this is used to avoid continuous calculation of the current
	 * time interval.
	 * Note: Updating this happens once in one of the barriers, currently
	 * Aura Manager barrier(void sim_mob::WorkGroupManager::waitAllGroups_AuraManager())
	 */

	unsigned int curInterval;
	/**
	 *	container to stor road segment travel times at different time intervals
	 */

	sim_mob::TravelTime rdSegTravelTimesMap;

	/**
	 * an iterator pointing to part of travel time container that maintains
	 * the latest processed travel time information.
	 * this is naturally the time interval before the current time interval.
	 */
	sim_mob::TravelTime::iterator latestTT;

public:
	static int dbg_ProcessTT_cnt;
	ProcessTT();
	~ProcessTT();
	/*
	 * accumulates Travel Time data
	 * @param stats travel time record
	 * @person the recording person
	 */
	void addTravelTime(const Agent::RdSegTravelStat & stats);

	/**
	 * Writes the aggregated data into the file
	 */
	void insertTravelTime2TmpTable(const std::string fileName);

	/**
	 * save Realtime Travel Time into Database
	 */
	bool storeRTT2DB();

	/**
	 * get corresponding TI (Time interval) give a time of day
	 * @param time a time within the day (usually segment entry time)
	 * @param interval travel time intreval specified for this simulation
	 * @return Time interval corresponding the give time
	 * Note: for uniformity purposes this methods works with milliseconds values
	 */
	static sim_mob::TT::TI getTimeInterval(const unsigned long timeMS, const unsigned int intervalMS);
	friend class sim_mob::PathSetManager;
};

///	Debug Method to print WayPoint based paths
std::string printWPpath(const std::vector<WayPoint> &wps , const sim_mob::Node* startingNode = 0);

/**
 * This class is used to store, retrieve, cache different parameters used in the pathset generation
 */
class PathSetParam {
private:
	PathSetParam();
	static PathSetParam *instance_;

public:
	static PathSetParam *getInstance();

	void initParameters();

	/// Retrieve 'ERP' and 'link travel time' information from Database
	void getDataFromDB();

	///	insert an entry into singlepath table in the database
	void storeSinglePath(soci::session& sql,std::set<sim_mob::SinglePath*, sim_mob::SinglePath>& spPool,const std::string pathSetTableName);

	///	insert an entry into pathset table in the database
//	void storePathSet(soci::session& sql,std::map<std::string,boost::shared_ptr<sim_mob::PathSet> >& psPool,const std::string pathSetTableName);

	///	set the table name used to store temporary travel time information
	void setRTTT(const std::string& value);

	/// create the table used to store realtime travel time information
	bool createTravelTimeRealtimeTable();

	///	get the average travel time of a segment within a time range from 'real time' or 'default' source
	//todo: merge it with getTravelTimeBySegId, they look similar
	double getAverageTravelTimeBySegIdStartEndTime(const sim_mob::RoadSegment* rs,sim_mob::DailyTime startTime,sim_mob::DailyTime endTime);

	///	get the travel time of a segment from 'default' source
	double getDefaultTravelTimeBySegId(unsigned long id);

	///	get travel time of a segment in a specific time from 'real time' or 'default' source
	//todo: merge it with getAverageTravelTimeBySegIdStartEndTime, they look similar
	double getTravelTimeBySegId(const sim_mob::RoadSegment* rs, const std::string &travelMode, const sim_mob::DailyTime &startTime);

	///	return cached node given its id
	sim_mob::Node* getCachedNode(std::string id);

	double getHighwayBias() { return highway_bias; }

	///	return the current rough size of the class
	uint32_t getSize();
	///	pathset parameters
	double bTTVOT;
	double bCommonFactor;
	double bLength;
	double bHighway;
	double bCost;
	double bSigInter;
	double bLeftTurns;
	double bWork;
	double bLeisure;
	double highway_bias;
	double minTravelTimeParam;
	double minDistanceParam;
	double minSignalParam;
	double maxHighwayParam;

	///	store all segs <aimsun id ,seg>
	std::map<std::string,sim_mob::RoadSegment*> segPool;

	///	<seg , value>
	std::map<const sim_mob::RoadSegment*,sim_mob::WayPoint*> wpPool;

	///	store all nodes <id ,node>
	std::map<std::string,sim_mob::Node*> nodePool;

	///	store all multi nodes in the map
	const std::vector<sim_mob::MultiNode*>  &multiNodesPool;

	///	store all uni nodes
	const std::set<sim_mob::UniNode*> & uniNodesPool;

	///	ERP surcharge  information <gantryNo , value=ERP_Surcharge with same No diff time stamp>
	std::map<std::string,std::vector<sim_mob::ERP_Surcharge*> > ERP_SurchargePool;

	///	ERP Zone information <gantryNo, ERP_Gantry_Zone>
	std::map<std::string,sim_mob::ERP_Gantry_Zone*> ERP_Gantry_ZonePool;

	///	ERP section <aim-sun id , ERP_Section>
	std::map<int,sim_mob::ERP_Section*> ERP_SectionPool;

	///	information of "Segment" default travel time <segment aim-sun id ,Link_default_travel_time with diff time stamp>
	std::map<unsigned long,std::vector<sim_mob::LinkTravelTime> > segmentDefaultTravelTimePool;

	///	a structure to keep history of previous average travel time records from previous simulations
	///	[time interval][travel mode][road segment][average travel time]
	AverageTravelTime historicalAvgTravelTime;

	///	current real time collection/retrieval interval (in milliseconds)
	const int intervalMS;

	///	simmobility's road network
	const sim_mob::RoadNetwork& roadNetwork;

	/// Real Time Travel Time Table Name
	std::string RTTT;
	/// Default Travel Time Table Name
	std::string DTT;

};
///////////////////////////////////////////////////////////////////////////////////////////////////////
class ERP_Gantry_Zone
{
public:
	ERP_Gantry_Zone() {}
	ERP_Gantry_Zone(ERP_Gantry_Zone &src):gantryNo(src.gantryNo),zoneId(src.zoneId) {}
	std::string gantryNo;
	std::string zoneId;
};

class ERP_Section
{
public:
	ERP_Section() {}
	ERP_Section(ERP_Section &src);
	int section_id;
	int ERP_Gantry_No;
	std::string ERP_Gantry_No_str;
};

class ERP_Surcharge
{
public:
	ERP_Surcharge() {}
	ERP_Surcharge(ERP_Surcharge& src):gantryNo(src.gantryNo),startTime(src.startTime),endTime(src.endTime),rate(src.rate),
			vehicleTypeId(src.vehicleTypeId),vehicleTypeDesc(src.vehicleTypeDesc),day(src.day),
			startTime_DT(sim_mob::DailyTime(src.startTime)),endTime_DT(sim_mob::DailyTime(src.endTime)){}
	std::string gantryNo;
	std::string startTime;
	std::string endTime;
	sim_mob::DailyTime startTime_DT;
	sim_mob::DailyTime endTime_DT;
	double rate;
	int vehicleTypeId;
	std::string vehicleTypeDesc;
	std::string day;
};

class LinkTravelTime
{
public:
	LinkTravelTime();
	LinkTravelTime(const LinkTravelTime& src);
	/**
	 * Common Information
	 */
	unsigned long linkId;
	///	travel time in seconds
	double travelTime;
	std::string travelMode;
	int interval;
	/**
	 * Filled during data Retrieval From DB and information usage
	 */
	std::string startTime;
	std::string endTime;
	sim_mob::DailyTime startTime_DT;
	sim_mob::DailyTime endTime_DT;
};

enum TRIP_PURPOSE
{
	work = 1,
	leisure = 2
};

template<typename T>
std::string toString(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

/// Roadsegment-Person
typedef std::multimap<const sim_mob::RoadSegment*, const sim_mob::Person* > SGPER ;// Roadsegment-Person  :)

/*****************************************************
 * ****** Path Set Manager ***************************
 * ***************************************************
 */
class PathSetManager {

public:
	static PathSetManager* getInstance()
	{
		if(!instance_)
		{
			instance_ = new PathSetManager();
		}
		return instance_;
	}
public:
	const std::set<const sim_mob::RoadSegment*> & getPartialExclusions(){return partialExclusions;}
	void addPartialExclusion(const sim_mob::RoadSegment* value){ partialExclusions.insert(value);}
	const std::set<const sim_mob::RoadSegment*> & getBlkLstSegs(){return blacklistSegments;}
	bool pathInBlackList(const std::vector<WayPoint> path, const std::set<const sim_mob::RoadSegment*> & blkLst);
	//void addBlkLstSegs(const sim_mob::RoadSegment* value){ blacklistSegments.insert(value);}
	bool generateAllPathSetWithTripChain2();
	///	generate shortest path information
	sim_mob::SinglePath *  findShortestDrivingPath( const sim_mob::Node *fromNode, const sim_mob::Node *toNode,std::set<std::string> duplicateChecker,
			  const std::set<const sim_mob::RoadSegment*> & excludedSegs=std::set<const sim_mob::RoadSegment*>());

	///	generate a path based on shortest travel time
	sim_mob::SinglePath* generateShortestTravelTimePath(const sim_mob::Node *fromNode, const sim_mob::Node *toNode,
			std::set<std::string>& duplicateChecker, sim_mob::TimeRange tr = sim_mob::MorningPeak,
			const sim_mob::RoadSegment* excludedSegs=NULL, int random_graph_idx=0);

	bool isUseCacheMode() { return false;/*isUseCache;*/ }//todo: take care of this later
	///	calculate part of utility which is not time dependent
	double generatePartialUtility(sim_mob::SinglePath* sp);
	///	calculate utility
	double generateUtility(sim_mob::SinglePath* sp);
	std::vector<WayPoint> generateBestPathChoice2(const sim_mob::SubTrip* st);
	/**
	 * update pathset paramenters before selecting the best path
	 * @param travelTime decides if travel time retrieval should be included or not.(setPathSetTags can get TT during pathset generation)
	 */
	void onPathSetRetrieval(boost::shared_ptr<PathSet> &ps, bool travelTime = true);
	///	post pathset generation processes
	void onGeneratePathSet(boost::shared_ptr<PathSet> &ps);
	/**
	 * find/generate set of path choices for a given suntrip, and then return the best of them
	 * @param st input subtrip
	 * @param res output path generated
	 * @param partialExcludedSegs segments temporarily having different attributes
	 * @param blckLstSegs segments off the road network. This
	 * @param tempBlckLstSegs segments temporarily off the road network
	 * @param isUseCache is using the cache allowed
	 * Note: PathsetManager object already has containers for partially excluded and blacklisted segments. They will be
	 * the default containers throughout the simulation. but partialExcludedSegs and blckLstSegs arguments are combined
	 * with their counterparts in PathSetmanager only during the scope of this method to serve temporary purposes.
	 */
	 bool getBestPath(std::vector<sim_mob::WayPoint>& res,
			 const sim_mob::SubTrip* st,std::stringstream *outDbg=nullptr,
			 const std::set<const sim_mob::RoadSegment*> tempBlckLstSegs=std::set<const sim_mob::RoadSegment*>(),
			 bool usePartialExclusion = false,
			 bool useBlackList = false,
			 bool isUseCache = true);

	/**
	 * generate all the paths for a person given its subtrip(OD)
	 * @param per input agent applying to get the path
	 * @param st input subtrip
	 * @param res output path generated
	 * @param excludedSegs input list segments to be excluded from the target set
	 * @param isUseCache is using the cache allowed
	 */
	bool generateAllPathChoices(boost::shared_ptr<sim_mob::PathSet> &ps, std::set<OD> &recursiveODs, const std::set<const sim_mob::RoadSegment*> & excludedSegs=std::set<const sim_mob::RoadSegment*>());

	///	generate travel time required to complete a path represented by different singlepath objects
	void generateTravelTimeSinglePathes(const sim_mob::Node *fromNode, const sim_mob::Node *toNode, std::set<std::string>& duplicateChecker,boost::shared_ptr<sim_mob::PathSet> &ps_);

	void generatePathesByLinkElimination(std::vector<WayPoint>& path, std::set<std::string>& duplicateChecker,boost::shared_ptr<sim_mob::PathSet> &ps_,const sim_mob::Node* fromNode,const sim_mob::Node* toNode);

	void generatePathesByTravelTimeLinkElimination(std::vector<WayPoint>& path, std::set<std::string>& duplicateChecker, boost::shared_ptr<sim_mob::PathSet> &ps_,const sim_mob::Node* fromNode,const sim_mob::Node* toNode,	sim_mob::TimeRange tr);

	bool getBestPathChoiceFromPathSet(boost::shared_ptr<sim_mob::PathSet> &ps,
			const std::set<const sim_mob::RoadSegment *> & partialExclusion = std::set<const sim_mob::RoadSegment *>(),
			const std::set<const sim_mob::RoadSegment*> &blckLstSegs = std::set<const sim_mob::RoadSegment *>());

	/// one of the main PathSetManager interfaces used to return a path for the current OD of the given person.
	std::vector<WayPoint> getPath(const sim_mob::Person* per,const sim_mob::SubTrip &subTrip);

	///	calculate travel time of a path
	static double getTravelTime(sim_mob::SinglePath *sp,const std::string & travelMode, const sim_mob::DailyTime & startTime_);
	///	record the travel time reported by agents
	void addRdSegTravelTimes(const Agent::RdSegTravelStat & stats);

	void setScenarioName(std::string& name){ scenarioName = name; }

	const std::pair<SGPER::const_iterator,SGPER::const_iterator > getODbySegment(const sim_mob::RoadSegment* segment) const;

	///	handle messages sent to pathset manager using message bus
	void HandleMessage(messaging::Message::MessageType type, const messaging::Message& message);

	///insert into incident list
	void inserIncidentList(const sim_mob::RoadSegment*);

	/**
	 * get the database session used for this thread
	 */
	static const boost::shared_ptr<soci::session> & getSession();

	/**
	 * store the realtime travel time into permanent storage
	 */
	void storeRTT();

	///basically delete all the dynamically allocated memories, in addition to some more cleanups
	void clearSinglePaths(boost::shared_ptr<sim_mob::PathSet> &ps);

	///cache the generated pathset. returns true upon successful insertion
	bool cachePathSet(boost::shared_ptr<sim_mob::PathSet> &ps);

	/**
	 * searches for a pathset in the cache.
	 * @param key indicates the input key
	 * @param value the result of the search
	 * returns true/false to indicate if the search has been successful
	 */
	bool findCachedPathSet(std::string key, boost::shared_ptr<sim_mob::PathSet> &value);

	//either this or the above method will remain todo
	bool findCachedPathSet(std::string key,
			boost::shared_ptr<sim_mob::PathSet> &value,
			const std::set<const sim_mob::RoadSegment*> & blcklist);
//	///cache the generated pathset. returns true upon successful insertion
//	bool cachePathSet_orig(boost::shared_ptr<sim_mob::PathSet> &ps);
	/**
	 * searches for a pathset in the cache.
	 * @param key indicates the input key
	 * @param value the result of the search
	 * returns true/false to indicate if the search has been successful
	 */
//	bool findCachedPathSet_orig(std::string key, boost::shared_ptr<sim_mob::PathSet> &value);

	///cache the generated pathset. returns true upon successful insertion
	bool cachePathSet_LRU(boost::shared_ptr<sim_mob::PathSet> &ps);
	/**
	 * searches for a pathset in the cache.
	 * @param key indicates the input key
	 * @param value the result of the search
	 * returns true/false to indicate if the search has been successful
	 */
	bool findCachedPathSet_LRU(std::string key, boost::shared_ptr<sim_mob::PathSet> &value);
	///	set some tags as a result of comparing attributes among paths in a pathset
	void setPathSetTags(boost::shared_ptr<sim_mob::PathSet>&ps);

	///	returns the raugh size of object in Bytes
	uint32_t getSize();

	PathSetManager();
	~PathSetManager();

private:
	static PathSetManager *instance_;

	///	link to street directory
	StreetDirectory& stdir;

	///	link to pathset paramaters
	PathSetParam *pathSetParam;

	///	is caching on
	bool isUseCache;

	///	list of partially excluded segments
	///example:like segments with incidents which have to be assigned
	///a maximum travel time
	std::set<const sim_mob::RoadSegment*> partialExclusions;
	///	list of segments that must be considered off the network throughout simulation
	///	in all operations of this class. The list must be already ready and supplied to
	/// PathsetManager's instance upon creation og PathSetManager's instance.
	/// if any additional/temporary blacklist emerged during simulation, they can be supplied
	///	to getPath() with proper switches. The management of such blacklists is not in the scope
	///	of PathSetManager
	const std::set<const sim_mob::RoadSegment*> blacklistSegments;

	///	protect access to incidents list
	boost::shared_mutex mutexExclusion;

//	///	stores the name of database's pathset table
//	const std::string &pathSetTableName;

	///	stores the name of database's singlepath table//todo:doublecheck the usability
	const std::string &pathSetTableName;

	///	stores the name of database's function operating on the pathset and singlepath tables
	const std::string &dbFunction;

	///every thread which invokes db related parts of pathset manages, should have its own connection to the database
	static std::map<boost::thread::id, boost::shared_ptr<soci::session > > cnnRepo;

	///	Travel time processing
	ProcessTT processTT;

	///	static sim_mob::Logger profiler;
	static boost::shared_ptr<sim_mob::batched::ThreadPool> threadpool_;

	boost::shared_mutex cachedPathSetMutex;

//	std::map<std::string, boost::shared_ptr<sim_mob::PathSet> > cachedPathSet;//same as pathSetPool, used in a separate scenario //todo later use only one of the caches, cancel the other one

	///	Yet another cache
	sim_mob::LRU_Cache<std::string, boost::shared_ptr<PathSet> > cacheLRU;
	///	contains arbitrary description usually to indicating which configuration file the generated data has originated from
	std::string scenarioName;

	///	used to avoid entering duplicate "HAS_PATH=-1" pathset entries into PathSet. It will be removed once the cache and/or proper DB functions are in place
	std::set<std::string> tempNoPath;

	///a cache to help answer this question: a given road segment is within which path(s)
	SGPER pathSegments;
/////////////////////////////travel time todo cleanup
	//=======road segment travel time computation for current frame tick =================
	struct RdSegTravelTimes
	{
	public:
		double travelTimeSum;
		unsigned int agCnt;

		RdSegTravelTimes(double rdSegTravelTime, unsigned int agentCount)
		: travelTimeSum(rdSegTravelTime), agCnt(agentCount) {}
	};

	void resetRdSegTravelTimes();
	void reportRdSegTravelTimes(timeslice frameNumber);
	bool insertTravelTime2TmpTable(timeslice frameNumber,
			std::map<const RoadSegment*, RdSegTravelTimes>& rdSegTravelTimesMap);

};
/*****************************************************
 ******************* Single Path *********************
 *****************************************************
 */
class SinglePath
{
public:
	/// path representation
	std::vector<WayPoint> path;
	///	link representation of path
	std::vector<sim_mob::Link*> linkPath;
	///	segment collection of the path
	std::set<const sim_mob::RoadSegment*> segSet;

	bool isNeedSave2DB;
	std::string scenario;
	std::string id;   //id: seg1id_seg2id_seg3id
	std::string pathSetId;

	double travelCost;
	double travleTime;

	/// time independent part of utility(used for optimization purposes)
	double partialUtility;
	std::stringstream partialUtilityDbg;
	double utility;
	double pathSize;

	double highWayDistance;
	int signalNumber;
	int rightTurnNumber;
	double length; //length in meter
	sim_mob::TRIP_PURPOSE purpose;

	bool isMinTravelTime;
	bool isMinDistance;
	bool isMinSignal;
	bool isMinRightTurn;
	bool isMaxHighWayUsage;
	bool isShortestPath;

	bool valid_path;

	long long index;//unique serial number assigned by db

	SinglePath(const SinglePath &source);
	///	extract the segment waypoint from series og node-segments waypoints
	SinglePath();
	~SinglePath();
	void init(std::vector<WayPoint>& wpPools);
	void clear();

	 bool operator() (const SinglePath* lhs, const SinglePath* rhs) const
	 {
		 return lhs->id < rhs->id;
	 }
	///	returns the raugh size of object in Bytes
	uint32_t getSize();
	///does these SinglePath include the any of given RoadSegment(s)
	bool includesRoadSegment(const std::set<const sim_mob::RoadSegment*> &segs, bool dbg= false, std::stringstream *out = nullptr);

};

/*****************************************************
 ******************* Path Set ************************
 *****************************************************
 */
class PathSet
{
public:
	PathSet():hasPath(false) {};
	PathSet(const sim_mob::Node *fn,const sim_mob::Node *tn) : fromNode(fn),toNode(tn),logsum(0),hasPath(false) {}
//	PathSet(const boost::shared_ptr<sim_mob::PathSet> &ps);
	~PathSet();
	///	returns the rough size of object in Bytes
	uint32_t getSize();
	/**
	 * checks to see if any of the SinglePath s includes a segment from the given container
	 * returns true if there is any common segments between the two sets.
	 * Note: This can be a computationally very expensive operation, use it with caution
	 */
	bool includesRoadSegment(const std::set<const sim_mob::RoadSegment*> & segs);
	/**
	 * eliminate those SinglePaths which have a section in the given set
	 */
	void excludeRoadSegment(const std::set<const sim_mob::RoadSegment*> & segs);
	void addOrDeleteSinglePath(sim_mob::SinglePath* s);
	bool isInit;
	bool hasBestChoice;
	std::vector<WayPoint> *bestPath;  //best choice
	const sim_mob::Node *fromNode;
	const sim_mob::Node *toNode;
	SinglePath* oriPath;  // shortest path with all segments
	//std::map<std::string,sim_mob::SinglePath*> SinglePathPool;
	std::set<sim_mob::SinglePath*, sim_mob::SinglePath> pathChoices;
	bool isNeedSave2DB;
	double logsum;
	const sim_mob::SubTrip* subTrip; // pathset use info of subtrip to generate all things
	std::string id;
	std::string excludedPaths;
	std::string scenario;
	bool hasPath;

	PathSet(boost::shared_ptr<sim_mob::PathSet> &ps);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///given a path of alternative nodes and segments, keep segments, loose the nodes
struct segFilter{
		bool operator()(const WayPoint value){
			return value.type_ == WayPoint::ROAD_SEGMENT;
		}
};

inline void filterOutNodes(std::vector<WayPoint>& input, std::vector<WayPoint>& output)
{
	typedef boost::filter_iterator<segFilter,std::vector<WayPoint>::iterator> FilterIterator;
	std::copy(FilterIterator(input.begin(), input.end()),FilterIterator(input.end(), input.end()),std::back_inserter(output));
}

/**
 * Returns path length in meter
 * @param collection of road segments wrapped in waypoint
 * @return total length of the path in meter
 */
inline double generateSinglePathLength(const std::vector<WayPoint>& wp)// unit is meter
{
	double res = 0.0;
	for(std::vector<WayPoint>::const_iterator it = wp.begin(); it != wp.end(); it++)
	{
		const sim_mob::RoadSegment* seg = it->roadSegment_;
		res += seg->length;
	}
	return res/100.0; //meter
}

/**
 * TODO: remove after completely testing the new versions
 * find the shortest path by analyzing the length of segments
 * @param pathChoices given path set
 * @param rs consider only the paths having the given road segment
 * @return the singlepath object containing the shortest path
 */
//inline sim_mob::SinglePath* findShortestPath(std::set<sim_mob::SinglePath*, sim_mob::SinglePath> &pathChoices, const sim_mob::RoadSegment *rs)
//{
//	if(pathChoices.begin() == pathChoices.end())
//	{
//		return nullptr;
//	}
//	sim_mob::SinglePath* res = nullptr;
//	double min = std::numeric_limits<double>::max();
//	double tmp = 0.0;
//	BOOST_FOREACH(sim_mob::SinglePath*sp, pathChoices)
//	{
//		//search for target segment id
//		//note, be carefull you may be searching for id 123 and search a string id like ",1234"
//		std::stringstream begin(""),out("");
//		begin << rs->getId() << ","; //only for the begining of sp->id
//		out << "," << rs->getId() << ","; //for the rest of the string
//		if(! (sp->id.find(out.str()) !=  std::string::npos || sp->id.substr(0,begin.str().size()) == begin.str()) )
//		{
//			continue;
//		}
//		if(sp->length <= 0.0)
//		{
//			throw std::runtime_error("Invalid path length");//todo remove this after enough testing
//		}
//		//double tmp = generateSinglePathLength(sp->path);
//		if ((sp->length*1000000 - min*1000000  ) < 0.0) //easy way to check doubles
//		{
//			//min = tmp;
//			min = sp->length;
//			res = sp;
//		}
//	}
//	return res;
//}

/**
 * In the path set find the shortest path which includes the given segment.
 * The method uses linkPath container which covers all the links that the
 * path visits.
 * Note: A path visiting a link doesnt mean the enire link is within the path.
 * fist and last link might have segments that are not in the path.
 * @param pathChoices given path set
 * @param rs consider only the paths having the given road segment
 * @return the singlepath object containing the shortest path
 */
inline sim_mob::SinglePath* findShortestPath_LinkBased(const std::set<sim_mob::SinglePath*, sim_mob::SinglePath> &pathChoices, const sim_mob::Link *ln)
{
	if(pathChoices.begin() == pathChoices.end())
	{
		return nullptr;
	}
	sim_mob::SinglePath* res = nullptr;
	double min = std::numeric_limits<double>::max();
	double tmp = 0.0;
	BOOST_FOREACH(sim_mob::SinglePath*sp, pathChoices)
	{

		if(sp->linkPath.empty())
		{
			throw std::runtime_error("linkPath of singlepath object is Empty");
		}
		//filter paths not including the target link
		if(std::find(sp->linkPath.begin(),sp->linkPath.end(),ln) == sp->linkPath.end())
		{
			continue;
		}
		if(sp->length <= 0.0)
		{
			throw std::runtime_error("Invalid path length");//todo remove this after enough testing
		}
		//double tmp = generateSinglePathLength(sp->path);
		if ((sp->length*1000000 - min*1000000  ) < 0.0) //easy way to check doubles
		{
			//min = tmp;
			min = sp->length;
			res = sp;
		}
	}
	return res;
}

/**
 * Overload
 */
inline sim_mob::SinglePath* findShortestPath_LinkBased(const std::set<sim_mob::SinglePath*, sim_mob::SinglePath> &pathChoices, const sim_mob::RoadSegment *rs)
{
	const sim_mob::Link *ln = rs->getLink();
	return findShortestPath_LinkBased(pathChoices,ln);
}

inline double getTravelCost2(sim_mob::SinglePath *sp,const std::string & travelMode, const sim_mob::DailyTime & startTime_)
{
	sim_mob::DailyTime tripStartTime(startTime_);
	double res=0.0;
	double ts=0.0;
	if(!sp || !sp->path.empty()) {
		sim_mob::Logger::log("path_set") << "gTC: sp is empty" << std::endl;
	}
	int i = 0;
//	sim_mob::DailyTime trip_startTime = sp->pathSet->subTrip->startTime;
	for(std::vector<WayPoint>::iterator it1 = sp->path.begin(); it1 != sp->path.end(); it1++,i++)
	{
		unsigned long segId = (it1)->roadSegment_->getId();
		std::map<int,sim_mob::ERP_Section*>::iterator it = sim_mob::PathSetParam::getInstance()->ERP_SectionPool.find(segId);//todo type mismatch
		//get travel time to this segment
		double t = sim_mob::PathSetParam::getInstance()->getTravelTimeBySegId((it1)->roadSegment_,travelMode, tripStartTime);
		ts += t;
		tripStartTime = tripStartTime + sim_mob::DailyTime(t*1000);
		if(it!=sim_mob::PathSetParam::getInstance()->ERP_SectionPool.end())
		{
			sim_mob::ERP_Section* erp_section = (*it).second;
			std::map<std::string,std::vector<sim_mob::ERP_Surcharge*> >::iterator itt =
					sim_mob::PathSetParam::getInstance()->ERP_SurchargePool.find(erp_section->ERP_Gantry_No_str);
			if(itt!=sim_mob::PathSetParam::getInstance()->ERP_SurchargePool.end())
			{
				std::vector<sim_mob::ERP_Surcharge*> erp_surcharges = (*itt).second;
				for(int i=0;i<erp_surcharges.size();++i)
				{
					sim_mob::ERP_Surcharge* s = erp_surcharges[i];
					if( s->startTime_DT.isBeforeEqual(tripStartTime) && s->endTime_DT.isAfter(tripStartTime) &&
							s->vehicleTypeId == 1 && s->day == "Weekdays")
					{
						res += s->rate;
					}
				}
			}
			else
			{
			}
		}
		else
		{
		}
	}
	return res;
}

inline std::string makeWaypointsetString(std::vector<WayPoint>& wp)
{
	std::string str;
	if(wp.size()==0)
	{
		sim_mob::Logger::log("path_set")<<"warning: empty input for makeWaypointsetString"<<std::endl;
	}

	for(std::vector<WayPoint>::iterator it = wp.begin(); it != wp.end(); it++)
	{
		if (it->type_ == WayPoint::ROAD_SEGMENT)
		{
			std::string tmp = it->roadSegment_->originalDB_ID.getLogItem();
			str += sim_mob::Utils::getNumberFromAimsunId(tmp) + ",";
		} // if ROAD_SEGMENT
	}

	if(str.size()<1)
	{
		// when same f,t node, it happened
		sim_mob::Logger::log("path_set")<<"warning: empty output makeWaypointsetString id"<<std::endl;
	}
	return str;
}

/**
 * Generate pathsize of paths. PathSize values are stored in the corresponding SinglePath object
 * @param ps the given pathset
 */
void generatePathSize(boost::shared_ptr<sim_mob::PathSet> &ps);

inline size_t getLaneIndex2(const Lane* l){
	if (l) {
		const RoadSegment* r = l->getRoadSegment();
		std::vector<sim_mob::Lane*>::const_iterator it( r->getLanes().begin()), itEnd(r->getLanes().end());
		for (size_t i = 0; it != itEnd; it++, i++) {
			if (*it == l) {
				return i;
			}
		}
	}
	return -1; //NOTE: This might not do what you expect! ~Seth
}

void calculateRightTurnNumberAndSignalNumberByWaypoints(sim_mob::SinglePath *sp);

inline double calculateHighWayDistance(sim_mob::SinglePath *sp)
{
	double res=0;
	if(!sp) return 0.0;
	for(int i=0;i<sp->path.size();++i)
	{
		sim_mob::WayPoint& w = sp->path[i];
		if (w.type_ == WayPoint::ROAD_SEGMENT) {
			const sim_mob::RoadSegment* seg = w.roadSegment_;
			if(seg->maxSpeed >= 60)
			{
				res += seg->length;
			}
		}
	}
	return res/100.0; //meter
}

static unsigned int seed = 0;
inline float gen_random_float(float min, float max)
{
    boost::mt19937 rng;
    rng.seed(static_cast<unsigned int>(std::time(0) + (++seed)));
    boost::uniform_real<float> u(min, max);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<float> > gen(rng, u);
    return gen();
}




}//namespace
