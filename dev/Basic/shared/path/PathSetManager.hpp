/*
 * PathSetManager.hpp
 *
 *  Created on: May 6, 2013
 *      Author: Max
 *      Author: Vahid
 */

#pragma once

#include "PathSetParam.hpp"
#include "TravelTimeManager.hpp"
#include "geospatial/Link.hpp"
#include "entities/Person.hpp"
#include "util/Cache.hpp"
#include "util/Utils.hpp"

namespace sim_mob
{
namespace batched {
class ThreadPool;
}
class PathSetWorkerThread;

///	Debug Method to print WayPoint based paths
std::string printWPpath(const std::vector<WayPoint> &wps , const sim_mob::Node* startingNode = 0);

/**
 * Path set manager class
 *
 * \author Vahid
 */
class PathSetManager {

public:
	static PathSetManager* getInstance()
	{
		{
			boost::unique_lock<boost::mutex> lock(instanceMutex);
			if(!instance_)
			{
				instance_ = new PathSetManager();
			}
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
	/**
	 * generate shortest distance path
	 */
	sim_mob::SinglePath *  findShortestDrivingPath( const sim_mob::Node *fromNode, const sim_mob::Node *toNode,std::set<std::string> duplicateChecker,
			  const std::set<const sim_mob::RoadSegment*> & excludedSegs=std::set<const sim_mob::RoadSegment*>());

	///	generate a path based on shortest travel time
	sim_mob::SinglePath* generateShortestTravelTimePath(const sim_mob::Node *fromNode, const sim_mob::Node *toNode,
//			std::set<std::string>& duplicateChecker,
			sim_mob::TimeRange tr = sim_mob::MorningPeak,
			const sim_mob::RoadSegment* excludedSegs=NULL, int random_graph_idx=0);

	/**
	 * calculate those part of the utility function that are always fixed(like path length)
	 * and are not going to change(like travel time)
	 * @param sp the input path
	 */
	double generatePartialUtility(const sim_mob::SinglePath* sp) const;

	/**
	 * Generates a log file to validate the computation of partial utility
	 * @param sp the given singlepath
	 * @param pUtility the already computed utility
	 * @return the generated string
	 */
	std::string logPartialUtility(const sim_mob::SinglePath* sp, double pUtility) const;

	/**
	 * calculates utility of the given path those part of the utility function that are always fixed(like path length)
	 * and are not going to change(like travel time)
	 * @param sp the target path
	 */
	double generateUtility(const sim_mob::SinglePath* sp) const;

	//todo: remove this obsolete method later when all its features are provided by other routines
	std::vector<WayPoint> generateBestPathChoice2(const sim_mob::SubTrip* st);

	/**
	 * update pathset paramenters before selecting the best path
	 * @param ps the input pathset
	 * @param enRoute decides if travel time retrieval should included in simulation travel time or not
	 */
	void onPathSetRetrieval(boost::shared_ptr<PathSet> &ps, bool enRoute);

	/**
	 * post pathset generation processes
	 * @param ps the input pathset
	 */
	void onGeneratePathSet(boost::shared_ptr<PathSet> &ps);

	/**
	 * find/generate set of path choices for a given suntrip, and then return the best of them
	 * @param st input subtrip
	 * @param res output path generated
	 * @param partialExcludedSegs segments temporarily having different attributes
	 * @param blckLstSegs segments off the road network. This
	 * @param tempBlckLstSegs segments temporarily off the road network
	 * @param enRoute is this method called for an enroute path request
	 * @param approache if this is an entoute, from which segment is it permitted to enter the rerouting point to start a new path
	 * Note: PathsetManager object already has containers for partially excluded and blacklisted segments. They will be
	 * the default containers throughout the simulation. but partialExcludedSegs and blckLstSegs arguments are combined
	 * with their counterparts in PathSetmanager only during the scope of this method to serve temporary purposes.
	 */
	 bool getBestPath(std::vector<sim_mob::WayPoint>& res,
			 const sim_mob::SubTrip& st,bool useCache,
			 const std::set<const sim_mob::RoadSegment*> tempBlckLstSegs/*=std::set<const sim_mob::RoadSegment*>()*/,
			 bool usePartialExclusion ,
			 bool useBlackList ,
			 bool enRoute ,const sim_mob::RoadSegment* approache);

	 /**
	  * generate K-shortest path
	  * @param pathset general information
	  * @param KSP_Storage output
	  * @return the number of paths generated
	  *TODO: a more generic approach is required to cover any number of pathset generation types;
	  *TODO: because having a separate method and call-back for every type of pathset generation is not scalable
	  */
	 int genK_ShortestPath(boost::shared_ptr<sim_mob::PathSet> &ps, std::set<sim_mob::SinglePath*, sim_mob::SinglePath> &KSP_Storage);
	 int genSDLE(boost::shared_ptr<sim_mob::PathSet> &ps,std::vector<PathSetWorkerThread*> &SDLE_Storage);
	 int genSTTLE(boost::shared_ptr<sim_mob::PathSet> &ps,std::vector<PathSetWorkerThread*> &STTLE_Storage);
	 int genSTTHBLE(boost::shared_ptr<sim_mob::PathSet> &ps,std::vector<PathSetWorkerThread*> &STTHBLE_Storage);
	 int genRandPert(boost::shared_ptr<sim_mob::PathSet> &ps,std::vector<PathSetWorkerThread*> &RandPertStorage);

	/**
	 * generate all the paths for a person given its subtrip(OD)
	 * @param per input agent applying to get the path
	 * @param st input subtrip
	 * @param res output path generated
	 * @param excludedSegs input list segments to be excluded from the target set
	 * @param isUseCache is using the cache allowed
	 * @return number of paths generated
	 */
	int generateAllPathChoices(boost::shared_ptr<sim_mob::PathSet> ps, std::set<OD> &recursiveODs, const std::set<const sim_mob::RoadSegment*> & excludedSegs);

	/**
	 *	offline pathset generation method.
	 *	This method collects the distinct demands from database,
	 *	creates a set of distinct demands and supplies them one by one to generateAllPathChoices.
	 *	it creates recursive paths if proper settings are configured.
	 *	The out put will be a csv file ready to be inserted into database.
	 */
	void bulkPathSetGenerator();

	///	generate travel time required to complete a path represented by different singlepath objects
	void generateTravelTimeSinglePathes(const sim_mob::Node *fromNode, const sim_mob::Node *toNode, std::set<std::string>& duplicateChecker,boost::shared_ptr<sim_mob::PathSet> &ps_);

	void generatePathesByLinkElimination(std::vector<WayPoint>& path, std::set<std::string>& duplicateChecker,boost::shared_ptr<sim_mob::PathSet> &ps_,const sim_mob::Node* fromNode,const sim_mob::Node* toNode);

	void generatePathesByTravelTimeLinkElimination(std::vector<WayPoint>& path, std::set<std::string>& duplicateChecker, boost::shared_ptr<sim_mob::PathSet> &ps_,const sim_mob::Node* fromNode,const sim_mob::Node* toNode,	sim_mob::TimeRange tr);

	bool getBestPathChoiceFromPathSet(boost::shared_ptr<sim_mob::PathSet> &ps,
			const std::set<const sim_mob::RoadSegment *> & partialExclusion,
			const std::set<const sim_mob::RoadSegment*> &blckLstSegs, bool enRoute, const sim_mob::RoadSegment* rs);

	/**
	 * The main entry point to the pathset manager,
	 * returns a path for the requested subtrip
	 * @param per the requesting person (todo:for logging purpose only)
	 * @subTrip the subtrip information containing OD, start time etc
	 * @enRoute indication of whether this request was made in the beginning of the trip or enRoute
	 * @return a sequence of road segments wrapped in way point structure
	 */
	std::vector<WayPoint> getPath(const sim_mob::SubTrip &subTrip, bool enRoute , const sim_mob::RoadSegment* approach);

	/**
	 * 	calculates the travel time of a path
	 * 	@param sp the given path
	 * 	@param travelMode mode of travelling through the path
	 * 	@startTime when to start the path
	 *  @enRoute decided whether in simulation travel time should be searched or not
	 * 	@return path's travel time
	 */
	double getPathTravelTime(sim_mob::SinglePath *sp,const std::string & travelMode, const sim_mob::DailyTime & startTime, bool enRoute = false);

	/**
	 * record the travel time reported by agents
	 * @param stats road segment travel time information
	 */
	void addSegTT(const Agent::RdSegTravelStat & stats);

	/**
	 * gets the average travel time of a segment experienced during the current simulation.
	 * Whether the desired travel time is coming from the last time interval
	 * or from the average of all previous time intervals is implementation dependent.
	 * @param rs input road segment
	 * @param travelMode intended mode of traversing the segment
	 * @param startTime indicates when the segment is to be traversed.
	 * @return travel time in seconds
	 */
	double getInSimulationSegTT(const sim_mob::RoadSegment* rs, const std::string &travelMode, const sim_mob::DailyTime &startTime);

	void setScenarioName(std::string& name){ scenarioName = name; }

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

	static void initTimeInterval();
	static void updateCurrTimeInterval();

	PathSetManager();
	~PathSetManager();
	/**
	 * time interval value used for processing data.
	 * This value is based on its counterpart in pathset manager.
	 */

	static unsigned int intervalMS;

	/**
	* current time interval, with respect to simulation time
	* this is used to avoid continuous calculation of the current
	* time interval.
	* Note: Updating this happens once in one of the barriers, currently
	* Aura Manager barrier(void sim_mob::WorkGroupManager::waitAllGroups_AuraManager())
	*/
	static unsigned int curIntervalMS;

private:
	static PathSetManager *instance_;
	static boost::mutex instanceMutex;

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
	/// PathsetManager's instance upon creation of PathSetManager's instance.
	/// if any additional/temporary blacklist emerged during simulation, they can be supplied
	///	to getPath() with proper switches. The management of such blacklists is not in the scope
	///	of PathSetManager
	const std::set<const sim_mob::RoadSegment*> blacklistSegments;

	///	protect access to incidents list
	boost::shared_mutex mutexExclusion;

	///	stores the name of database's singlepath table//todo:doublecheck the usability
	const std::string &pathSetTableName;

	///	stores the name of database's function operating on the pathset and singlepath tables
	const std::string &psRetrieval;

	///every thread which invokes db related parts of pathset manages, should have its own connection to the database
	static std::map<boost::thread::id, boost::shared_ptr<soci::session > > cnnRepo;
	static boost::shared_mutex cnnRepoMutex;

	///	Travel time processing
	TravelTimeManager processTT;

	///	static sim_mob::Logger profiler;
	static boost::shared_ptr<sim_mob::batched::ThreadPool> threadpool_;

	///	Yet another cache
	sim_mob::LRU_Cache<std::string, boost::shared_ptr<PathSet> > cacheLRU;
	/**
	 * structure to help avoiding simultaneous pathset generation by multiple threads for identical OD
	 */
	struct SimpleCollector
	{
	private:
		boost::mutex mutex_;
		std::set<std::string> collection;
	public:
		bool tryCheck(const std::string &od)
		{
			boost::unique_lock<boost::mutex> lock(mutex_);
			if(collection.find(od) != collection.end())
			{
				return false;
			}
			collection.insert(od);
			return true;
		}

		bool insert(const std::string &od)
		{
			boost::unique_lock<boost::mutex> lock(mutex_);
			return collection.insert(od).second;
		}

		void erase(const std::string &od)
		{
			boost::unique_lock<boost::mutex> lock(mutex_);
			collection.erase(od);
		}

		bool find(const std::string &od)
		{
			boost::unique_lock<boost::mutex> lock(mutex_);
			return collection.find(od) != collection.end();
		}

	};

	/**
	 * simple structure to help avoiding simultaneous operations for pathset retrieval/Generation
	 * Any attempt to generate a path set for any OD is recorded here and will not be attempted again
	 */
	SimpleCollector pathRetrievalAttempt;

	///	contains arbitrary description usually to indicating which configuration file the generated data has originated from
	std::string scenarioName;

	///	used to avoid entering duplicate "HAS_PATH=-1" pathset entries into PathSet. It will be removed once the cache and/or proper DB functions are in place
	SimpleCollector tempNoPath;
};

/**
 * Generate pathsize of paths. PathSize values are stored in the corresponding SinglePath object
 * @param ps the given pathset
 */
void generatePathSize(boost::shared_ptr<sim_mob::PathSet> &ps);


static unsigned int seed = 0;
inline float genRandomFloat(float min, float max)
{
    boost::mt19937 rng;
    rng.seed(static_cast<unsigned int>(std::time(0) + (++seed)));
    boost::uniform_real<float> u(min, max);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<float> > gen(rng, u);
    return gen();
}

}//namespace
