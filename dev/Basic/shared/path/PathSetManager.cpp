/*
 * PathSetManager.cpp
 *
 *  Created on: May 6, 2013
 *      Author: Max
 */

#include "PathSetManager.hpp"
#include "Path.hpp"
#include "entities/PersonLoader.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/Lane.hpp"
#include "geospatial/Node.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/MultiNode.hpp"
#include "geospatial/LaneConnector.hpp"
#include "path/PathSetThreadPool.hpp"
#include "geospatial/streetdir/KShortestPathImpl.hpp"
#include "util/threadpool/Threadpool.hpp"
#include "workers/Worker.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "message/MessageBus.hpp"
#include <cmath>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <sstream>

using std::vector;
using std::string;

using namespace sim_mob;

namespace{
sim_mob::BasicLogger & logger = sim_mob::Logger::log("path_set");
}
double getPathTravelCost(sim_mob::SinglePath *sp,const std::string & travelMode, const sim_mob::DailyTime & startTime_);
sim_mob::SinglePath* findShortestPath_LinkBased(const std::set<sim_mob::SinglePath*, sim_mob::SinglePath> &pathChoices, const sim_mob::RoadSegment *rs);
sim_mob::SinglePath* findShortestPath_LinkBased(const std::set<sim_mob::SinglePath*, sim_mob::SinglePath> &pathChoices, const sim_mob::Link *ln);


std::string getFromToString(const sim_mob::Node* fromNode,const sim_mob::Node* toNode ){
	std::stringstream out("");
	std::string idStrTo = toNode->originalDB_ID.getLogItem();
	std::string idStrFrom = fromNode->originalDB_ID.getLogItem();
	out << Utils::getNumberFromAimsunId(idStrFrom) << "," << Utils::getNumberFromAimsunId(idStrTo);
	return out.str();
}

PathSetManager *sim_mob::PathSetManager::instance_;
std::map<boost::thread::id, boost::shared_ptr<soci::session> > sim_mob::PathSetManager::cnnRepo;
boost::shared_ptr<sim_mob::batched::ThreadPool> sim_mob::PathSetManager::threadpool_(new sim_mob::batched::ThreadPool(10));


uint32_t sim_mob::PathSetManager::getSize(){
	uint32_t sum = 0;
	//first get the pathset param here
	sum += pathSetParam->getSize();
	sum += sizeof(StreetDirectory&);
	sum += sizeof(PathSetParam *);
//	sum += sizeof(bool); // bool isUseCache;
	sum += sizeof(const sim_mob::RoadSegment*) * partialExclusions.size(); // std::set<const sim_mob::RoadSegment*> currIncidents;
	sum += sizeof(boost::shared_mutex); // boost::shared_mutex mutexPartial;
	sum += sizeof(std::string &);  // const std::string &pathSetTableName;
	sum += sizeof(const std::string &); // const std::string &pathSetTableName;
	sum += sizeof(const std::string &); // const std::string &dbFunction;
	// static std::map<boost::thread::id, boost::shared_ptr<soci::session > > cnnRepo;
	sum += (sizeof(boost::thread::id)) + sizeof(boost::shared_ptr<soci::session >) * cnnRepo.size();
	sum += sizeof(sim_mob::batched::ThreadPool *); // sim_mob::batched::ThreadPool *threadpool_;
	sum += sizeof(boost::shared_mutex); // boost::shared_mutex cachedPathSetMutex;
//	//map<const std::string,sim_mob::PathSet > cachedPathSet;
//	for(std::map<const std::string,boost::shared_ptr<sim_mob::PathSet> >::iterator it = cachedPathSet.begin(); it !=cachedPathSet.end(); it++)
////			BOOST_FOREACH(cachedPathSet_pair,cachedPathSet)
//	{
//		sum += it->first.length();
//		sum += it->second->getSize();
//	}
	sum += scenarioName.length(); // std::string scenarioName;
	// std::map<std::string ,std::vector<WayPoint> > fromto_bestPath;
//	sum += sizeof(WayPoint) * fromto_bestPath.size();
//	for(std::map<std::string ,std::vector<WayPoint> >::iterator it = fromto_bestPath.begin(); it != fromto_bestPath.end(); sum += it->first.length(), it++);
	for(std::set<std::string>::iterator it = tempNoPath.begin(); it != tempNoPath.end(); sum += (*it).length(), it++); // std::set<std::string> tempNoPath;
	sum += sizeof(SGPER); // SGPER pathSegments;
	sum += pathSetParam->RTTT.length(); // std::string RTTT;
	sum += sizeof(sim_mob::K_ShortestPathImpl *); // sim_mob::K_ShortestPathImpl *kshortestImpl;
	sum += sizeof(double); // double bTTVOT;
	sum += sizeof(double); // double bCommonFactor;
	sum += sizeof(double); // double bLength;
	sum += sizeof(double); // double bHighway;
	sum += sizeof(double); // double bCost;
	sum += sizeof(double); // double bSigInter;
	sum += sizeof(double); // double bLeftTurns;
	sum += sizeof(double); // sum += sizeof(); // double bWork;
	sum += sizeof(double); // double bLeisure;
	sum += sizeof(double); // double highway_bias;
	sum += sizeof(double); // double minTravelTimeParam;
	sum += sizeof(double); // double minDistanceParam;
	sum += sizeof(double); // double minSignalParam;
	sum += sizeof(double);  // double maxHighwayParam;

	return sum;
}

unsigned int sim_mob::PathSetManager::curIntervalMS = 0;
unsigned int sim_mob::PathSetManager::intervalMS = 0;

sim_mob::PathSetManager::PathSetManager():stdir(StreetDirectory::instance()),
//		pathSetTableName(sim_mob::ConfigManager::GetInstance().FullConfig().pathSet().pathSetTableName),
		pathSetTableName(sim_mob::ConfigManager::GetInstance().FullConfig().pathSet().pathSetTableName),
		dbFunction(sim_mob::ConfigManager::GetInstance().FullConfig().pathSet().dbFunction),cacheLRU(2500),
		processTT(intervalMS, curIntervalMS),
		blacklistSegments((sim_mob::ConfigManager::GetInstance().FullConfig().CBD() ?
				RestrictedRegion::getInstance().getZoneSegments(): std::set<const sim_mob::RoadSegment*>()))//todo placeholder
{
	pathSetParam = PathSetParam::getInstance();
	std::string dbStr(ConfigManager::GetInstance().FullConfig().getDatabaseConnectionString(false));
//	// 1.2 get all segs
	cnnRepo[boost::this_thread::get_id()].reset(new soci::session(soci::postgresql,dbStr));
}

void HandleMessage(messaging::Message::MessageType type, const messaging::Message& message){

}

sim_mob::PathSetManager::~PathSetManager()
{
}


//void sim_mob::PathSetManager::initTimeInterval()
//{
//	TravelTimeManager::initTimeInterval();
//}
//
//void sim_mob::PathSetManager::updateCurrTimeInterval()
//{
//	TravelTimeManager::updateCurrTimeInterval();
//}

namespace {
int pathsCnt = 0;
int spCnt = 0;
}

bool sim_mob::PathSetManager::pathInBlackList(const std::vector<WayPoint> path, const std::set<const sim_mob::RoadSegment*> & blkLst)
{
	BOOST_FOREACH(WayPoint wp, path)
	{
		if(blkLst.find(wp.roadSegment_) != blkLst.end())
		{
			return true;
		}
	}
	return false;
}

#if 0
//most of thease methods with '2' suffix are prone to memory leak due to inserting duplicate singlepaths into a std::set
bool sim_mob::PathSetManager::generateAllPathSetWithTripChain2()
{
	const std::map<std::string, std::vector<sim_mob::TripChainItem*> > *tripChainPool =
			&ConfigManager::GetInstance().FullConfig().getTripChains();
	logger<<"generateAllPathSetWithTripChain: trip chain pool size "<<  tripChainPool->size() <<  "\n";
	int poolsize = tripChainPool->size();
	bool res=true;
	// 1. get from and to node
	std::map<std::string, std::vector<sim_mob::TripChainItem*> >::const_iterator it;
	int i=1;
	for(it = tripChainPool->begin();it!=tripChainPool->end();++it)
	{
		std::string personId = (*it).first;
		std::vector<sim_mob::TripChainItem*> tci = (*it).second;
		for(std::vector<sim_mob::TripChainItem*>::iterator it=tci.begin(); it!=tci.end(); ++it)
		{
			if((*it)->itemType == sim_mob::TripChainItem::IT_TRIP)
			{
				sim_mob::Trip *trip = dynamic_cast<sim_mob::Trip*> ((*it));
				if(!trip)
				{
					throw std::runtime_error("generateAllPathSetWithTripChainPool: trip error");
				}
				const std::vector<sim_mob::SubTrip> subTripPool = trip->getSubTrips();
				for(int i=0; i<subTripPool.size(); ++i)
				{
					const sim_mob::SubTrip *st = &subTripPool.at(i);
					std::string subTripId = st->tripID;
					if(st->mode == "Car") //only driver need path set
					{
						std::vector<WayPoint> res =  generateBestPathChoice2(st);
					}
				}
			}
		}
		i++;
	}
	return res;
}
#endif

//bool sim_mob::PathSetManager::storeRTT2DB()
//{
//	//1. prepare the csv file to be copied into DB
////	sim_mob::Logger::log("real_time_travel_time");
//	bool res=false;
//	//2.truncate/empty out the realtime travel time table
//	res = sim_mob::aimsun::Loader::truncateTable(*getSession(),	pathSetParam->RTTT);
//	if(!res)
//	{
//		return false;
//	}
//	//3.write into DB table
//	sim_mob::Logger::log("real_time_travel_time").flush();
//	sim_mob::aimsun::Loader::insertCSV2Table(*getSession(),	pathSetParam->RTTT, boost::filesystem::canonical("real_time_travel_time.txt").string());
//	return res;
//}

const std::pair <SGPER::const_iterator,SGPER::const_iterator > sim_mob::PathSetManager::getODbySegment(const sim_mob::RoadSegment* segment) const{
       logger << "pathSegments cache size =" <<  pathSegments.size() << "\n";
       const std::pair <SGPER::const_iterator,SGPER::const_iterator > range = pathSegments.equal_range(segment);
       return range;
}

void sim_mob::PathSetManager::inserIncidentList(const sim_mob::RoadSegment* rs) {
	boost::unique_lock<boost::shared_mutex> lock(mutexExclusion);
	partialExclusions.insert(rs);
}

const boost::shared_ptr<soci::session> & sim_mob::PathSetManager::getSession(){
	std::map<boost::thread::id, boost::shared_ptr<soci::session> >::iterator it = cnnRepo.find(boost::this_thread::get_id());
	if(it == cnnRepo.end())
	{
		std::string dbStr(ConfigManager::GetInstance().FullConfig().getDatabaseConnectionString(false));
		cnnRepo[boost::this_thread::get_id()].reset(new soci::session(soci::postgresql,dbStr));
		it = cnnRepo.find(boost::this_thread::get_id());
	}
	return it->second;
}

void sim_mob::PathSetManager::storeRTT()
{
	processTT.storeRTT2DB();
}

void sim_mob::PathSetManager::clearSinglePaths(boost::shared_ptr<sim_mob::PathSet>&ps){
	logger << "clearing " << ps->pathChoices.size() << " SinglePaths\n";
	BOOST_FOREACH(sim_mob::SinglePath* sp_, ps->pathChoices){
		if(sp_){
			safe_delete_item(sp_);
		}
	}
	ps->pathChoices.clear();
}

bool sim_mob::PathSetManager::cachePathSet(boost::shared_ptr<sim_mob::PathSet>&ps){
	return cachePathSet_LRU(ps);
}

bool sim_mob::PathSetManager::cachePathSet_LRU(boost::shared_ptr<sim_mob::PathSet>&ps){
	cacheLRU.insert(ps->id, ps);
}

//bool sim_mob::PathSetManager::cachePathSet_orig(boost::shared_ptr<sim_mob::PathSet>&ps){
//	logger << "caching [" << ps->id << "]\n";
//	//test
////	return false;
//	//first step caching policy:
//	// if cache size excedded 250 (upper threshold), reduce the size to 200 (lowe threshold)
//	if(cachedPathSet.size() > 2500)
//	{
//		logger << "clearing some of the cached PathSets\n";
//		int i = cachedPathSet.size() - 2000;
//		std::map<std::string, boost::shared_ptr<sim_mob::PathSet> >::iterator it(cachedPathSet.begin());
//		for(; i >0 && it != cachedPathSet.end(); --i )
//		{
//			cachedPathSet.erase(it++);
//		}
//	}
//	{
//		boost::unique_lock<boost::shared_mutex> lock(cachedPathSetMutex);
//		bool res = cachedPathSet.insert(std::make_pair(ps->id,ps)).second;
//		if(!res){
//			logger << "Failed to cache [" << ps->id << "]\n";
//		}
//		return res;
//	}
//}

bool sim_mob::PathSetManager::findCachedPathSet(std::string  key,
		boost::shared_ptr<sim_mob::PathSet> &value,
		const std::set<const sim_mob::RoadSegment*> & blcklist){
	bool res = findCachedPathSet_LRU(key,value);
	//expensive operation, use with cautions
	if(res && !blcklist.empty())
	{
		throw std::runtime_error("This demo shall NOT check cache for blacklist");
		//expensive operation, use with cautions
		std::set<sim_mob::SinglePath*, sim_mob::SinglePath>::iterator it(value->pathChoices.begin());
		for ( ; it != value->pathChoices.end(); ) {
		  if (pathInBlackList((*it)->path,blcklist)) {
		    value->pathChoices.erase(it++);
		  } else {
		    ++it;
		  }
		}
	}
	return res;
}

bool sim_mob::PathSetManager::findCachedPathSet(std::string  key, boost::shared_ptr<sim_mob::PathSet> &value){
	return findCachedPathSet_LRU(key,value);
}

bool sim_mob::PathSetManager::findCachedPathSet_LRU(std::string  key, boost::shared_ptr<sim_mob::PathSet> &value){
	return cacheLRU.find(key,value);
}


void sim_mob::PathSetManager::setPathSetTags(boost::shared_ptr<sim_mob::PathSet>&ps)
{

	// find MIN_DISTANCE
	double minDistance = std::numeric_limits<double>::max();
	SinglePath * minSP = *(ps->pathChoices.begin()); // record which is min
	BOOST_FOREACH(sim_mob::SinglePath* sp, ps->pathChoices)
	{
		if(sp->length < minDistance)
		{
			minDistance = sp->length;
			minSP = sp;
		}
	}
	minSP->isMinDistance = 1;

	// find MIN_SIGNAL
	int minSignal = std::numeric_limits<int>::max();
	minSP = *(ps->pathChoices.begin()); // record which is min
	BOOST_FOREACH(sim_mob::SinglePath* sp, ps->pathChoices)
	{
		if(sp->signalNumber < minSignal)
		{
			minSignal = sp->signalNumber;
			minSP = sp;
		}
	}
	minSP->isMinSignal = 1;

	// find MIN_RIGHT_TURN
	int minRightTurn = std::numeric_limits<int>::max();
	minSP = *(ps->pathChoices.begin()); // record which is min
	BOOST_FOREACH(sim_mob::SinglePath* sp, ps->pathChoices)
	{
		if(sp->rightTurnNumber < minRightTurn)
		{
			minRightTurn = sp->rightTurnNumber;
			minSP = sp;
		}
	}
	minSP->isMinRightTurn = 1;

	// find MAX_HIGH_WAY_USAGE
	double maxHighWayUsage=0.0;
	minSP = *(ps->pathChoices.begin()); // record which is min
	BOOST_FOREACH(sim_mob::SinglePath* sp, ps->pathChoices)
	{
		if(maxHighWayUsage < sp->highWayDistance / sp->length)
		{
			maxHighWayUsage = sp->highWayDistance / sp->length;
			minSP = sp;
		}
	}
	minSP->isMaxHighWayUsage = 1;

}

//bool sim_mob::PathSetManager::findCachedPathSet_orig(std::string  key, boost::shared_ptr<sim_mob::PathSet> &value){
////	//test
////	return false;
//	std::map<std::string, boost::shared_ptr<sim_mob::PathSet> >::iterator it ;
//	{
//		boost::unique_lock<boost::shared_mutex> lock(cachedPathSetMutex);
//		it = cachedPathSet.find(key);
//		if (it == cachedPathSet.end()) {
//			//debug
//			std::stringstream out("");
//			out << "Failed finding [" << key << "] in" << cachedPathSet.size() << " entries\n" ;
//			typedef std::map<std::string, boost::shared_ptr<sim_mob::PathSet> >::value_type ITEM;
//			BOOST_FOREACH(ITEM & item,cachedPathSet){
//				out << item.first << ",";
//			}
//			logger << out.str() << "\n";
//			return false;
//		}
//		value = it->second;
//	}
//	return true;
//}

std::string sim_mob::printWPpath(const std::vector<WayPoint> &wps , const sim_mob::Node* startingNode ){
	std::ostringstream out("wp path--");
	if(startingNode){
		out << startingNode->getID() << ":";
	}
	BOOST_FOREACH(WayPoint wp, wps){
		if(wp.type_ == sim_mob::WayPoint::ROAD_SEGMENT)
		{
			out << wp.roadSegment_->getSegmentAimsunId() << ",";
		}
		else if(wp.type_ == sim_mob::WayPoint::NODE)
		{
			out << wp.node_->getID() << ",";
		}
	}
	out << "\n";

	logger << out.str();
	return out.str();
}


vector<WayPoint> sim_mob::PathSetManager::getPath(const sim_mob::Person* per,const sim_mob::SubTrip &subTrip, bool enRoute)
{
	// get person id and current subtrip id
	std::stringstream str("");
	str << subTrip.fromLocation.node_->getID() << "," << subTrip.toLocation.node_->getID();
	std::string fromToID(str.str());
	//todo change the subtrip signature from pointer to referencer
	vector<WayPoint> res = vector<WayPoint>();
	//CBD area logic
	bool from = sim_mob::RestrictedRegion::getInstance().isInRestrictedZone(subTrip.fromLocation);
	bool to = sim_mob::RestrictedRegion::getInstance().isInRestrictedZone(subTrip.toLocation);
	str.str("");
	str << "[" << fromToID << "]";
	if (sim_mob::ConfigManager::GetInstance().FullConfig().CBD()) {
		if (to == false && from == false) {
			subTrip.cbdTraverseType = TravelMetric::CBD_PASS;
			str << "[BLCKLST]";
			std::stringstream outDbg("");
			getBestPath(res, &subTrip, &outDbg,
					std::set<const sim_mob::RoadSegment*>(), false, true,enRoute);//use/enforce blacklist
			if (sim_mob::RestrictedRegion::getInstance().isInRestrictedSegmentZone(res)) {
				logger << "[" << fromToID << "]" << " [PERSON: "	<< per->getId() << "] " << "[PATH : " << res.size() << "]" << std::endl;
				printWPpath(res);
				logger << outDbg.str() << std::endl;
				throw std::runtime_error("\npath inside cbd ");
			}
		} else {

			if (!(to && from)) {
				subTrip.cbdTraverseType =
						(from ? TravelMetric::CBD_EXIT : TravelMetric::CBD_ENTER);
				str << (from ? " [EXIT CBD]" : "[ENTER CBD]");
			} else {
				str << "[BOTH INSIDE CBD]";
			}
			getBestPath(res, &subTrip);
		}
	} else {
		getBestPath(res, &subTrip);
	}

	//subscribe person
	logger << fromToID  << ": Path chosen for person[" << per->getId() << "]"  << "\n";
	str <<  " [PERSON: " << per->getId() << "]"  ;
	if(!res.empty())
	{
		logger << "[" << fromToID << "]" <<  " : was assigned path of size " << res.size()  << "\n";
		str << "[PATH : " << res.size()  << "]\n";
	}
	else{
		logger << "[" << fromToID << "]" << " : NO PATH" << "\n";
		str << "[NO PATH]" << "\n";
	}
	logger << str.str();
	return res;
}

///	discard those entries which have segments whith their CBD flag set to true
///	return the final number of path choices
unsigned short purgeBlacklist(sim_mob::PathSet &ps)
{
	std::set<sim_mob::SinglePath*>::iterator it = ps.pathChoices.begin();
	for(;it != ps.pathChoices.end();)
	{
		bool erase = false;
		std::vector<sim_mob::WayPoint>::iterator itWp = (*it)->path.begin();
		for(; itWp != (*it)->path.end(); itWp++)
		{
			if(itWp->roadSegment_->CBD)
			{
				erase = true;
				break;
			}
		}
		if(erase)
		{
			if(ps.oriPath == *it)
			{
				ps.oriPath = nullptr;
			}
			SinglePath * sp = *it;
			delete sp;
			ps.pathChoices.erase(it++);
		}
		else
		{
			it++;
		}
	}
	return ps.pathChoices.size();
}

void sim_mob::PathSetManager::onPathSetRetrieval(boost::shared_ptr<PathSet> &ps, bool enRoute)
{
	//step-1 time dependent calculations
	int i = 0;
	double minTravelTime= std::numeric_limits<double>::max();
	sim_mob::SinglePath *minSP = *(ps->pathChoices.begin());
	BOOST_FOREACH(SinglePath *sp, ps->pathChoices)
	{
		sp->travleTime = getPathTravelTime(sp,ps->subTrip->mode,ps->subTrip->startTime, enRoute);
		sp->travelCost = getPathTravelCost(sp,ps->subTrip->mode,ps->subTrip->startTime );
		//MIN_TRAVEL_TIME
		if(sp->travleTime < minTravelTime)
		{
			minTravelTime = sp->travleTime;
			minSP = sp;
		}
	}
	if(!ps->pathChoices.empty() && minSP)
	{
		minSP->isMinTravelTime = 1;
	}

	//step-2 utility calculation
	BOOST_FOREACH(SinglePath *sp, ps->pathChoices)
	{
		sp->utility = generateUtility(sp);
	}
}
void sim_mob::PathSetManager::onGeneratePathSet(boost::shared_ptr<PathSet> &ps)
{
	setPathSetTags(ps);
	generatePathSize(ps);
	//partial utility calculation to save some time
	BOOST_FOREACH(SinglePath *sp, ps->pathChoices)
	{
		sp->partialUtility = generatePartialUtility(sp);
	}
	//store in into the database
	logger << "[STORE PATH: " << ps->id << "]\n";
	pathSetParam->storeSinglePath(*getSession(), ps->pathChoices,pathSetTableName);
}
//Operations:
//step-0: Initial preparations
//step-1: Check the cache
//step-2: If not found in cache, check DB
//Step-3: If not found in DB, generate all 4 types of path
//step-5: Choose the best path using utility function
bool sim_mob::PathSetManager::getBestPath(
		std::vector<sim_mob::WayPoint> &res,
		const sim_mob::SubTrip* st,std::stringstream *outDbg,
		std::set<const sim_mob::RoadSegment*> tempBlckLstSegs,
		 bool usePartialExclusion,
		 bool useBlackList,
		bool enRoute)
{
	res.clear();
//	if(st->mode == "Car" || st->mode == "Taxi" || st->mode == "Motorcycle") //only driver need path set
//	{
//		return false;
//	}
	//take care of partially excluded and blacklisted segments here
	std::set<const sim_mob::RoadSegment*> blckLstSegs(tempBlckLstSegs);
	if(useBlackList && this->blacklistSegments.size())
	{
		blckLstSegs.insert(this->blacklistSegments.begin(), this->blacklistSegments.end()); //temporary + permanent
	}
	const std::set<const sim_mob::RoadSegment*> &partial = (usePartialExclusion ? this->partialExclusions : std::set<const sim_mob::RoadSegment*>());

	const sim_mob::Node* fromNode = st->fromLocation.node_;
	const sim_mob::Node* toNode = st->toLocation.node_;
	if(!(toNode && fromNode)){
		logger << "Error, OD null\n" ;
		return false;
	}
	if(toNode->getID() == fromNode->getID()){
		logger << "Error: same OD id from different objects discarded:" << toNode->getID() << "\n" ;
		return false;
	}
	std::stringstream out("");
	std::string idStrTo = toNode->originalDB_ID.getLogItem();
	std::string idStrFrom = fromNode->originalDB_ID.getLogItem();
	out << Utils::getNumberFromAimsunId(idStrFrom) << "," << Utils::getNumberFromAimsunId(idStrTo);
	std::string fromToID(out.str());
	if(tempNoPath.find(fromToID) != tempNoPath.end())
	{
		logger <<  fromToID   << "[RECORERD OF FAILURE.BYPASSING : " << fromToID << "]\n";
		return false;
	}
	logger << "[THREAD " << boost::this_thread::get_id() << "][SEARCHING FOR : " << fromToID << "]\n" ;
	boost::shared_ptr<sim_mob::PathSet> ps_;

	//Step-1 Check Cache
	sim_mob::Logger::log("ODs") << "[" << fromToID << "]" <<  "\n";
	/*
	 * supply only the temporary blacklist, because with the current implementation,
	 * cache should never be filled with paths containing permanent black listed segments
	 */
	std::set<const sim_mob::RoadSegment*> emptyBlkLst = std::set<const sim_mob::RoadSegment*>();//and sometime you dont need a black list at all!
	if(findCachedPathSet(fromToID,ps_,(useBlackList && tempBlckLstSegs.size() ? tempBlckLstSegs : std::set<const sim_mob::RoadSegment*>())))
	{
		logger <<  fromToID  << " : Cache Hit\n";
		onPathSetRetrieval(ps_,enRoute);
		//no need to supply permanent blacklist
		bool r = getBestPathChoiceFromPathSet(ps_,partial,emptyBlkLst);
		logger <<  fromToID << " : getBestPathChoiceFromPathSet returned best path of size : " << ps_->bestPath->size() << "\n";
		if(r)
		{
			res = *(ps_->bestPath);
			logger <<  fromToID << " : returning a path " << res.size() << "\n";
			return true;

		}
		else{
				logger <<  fromToID  << "UNUSED Cache Hit" <<  "\n";
		}
	}
	else
	{
		logger <<  fromToID << " : Cache Miss " << "\n";
	}
	//step-2:check  DB
	sim_mob::HasPath hasPath = PSM_UNKNOWN;
	ps_.reset(new sim_mob::PathSet());
	ps_->subTrip = st;
	ps_->id = fromToID;
	ps_->scenario = scenarioName;
	hasPath = sim_mob::aimsun::Loader::loadSinglePathFromDB(*getSession(),fromToID,ps_->pathChoices, dbFunction,outDbg,blckLstSegs);
	logger  <<  fromToID << " : " << (hasPath == PSM_HASPATH ? "" : "Don't " ) << "have SinglePaths in DB \n" ;
	switch (hasPath) {
	case PSM_HASPATH: {
		logger << "[" << fromToID << "]" <<  " : DB Hit\n";
		bool r = false;
		ps_->oriPath = 0;
		BOOST_FOREACH(sim_mob::SinglePath* sp, ps_->pathChoices) {
			if (sp->isShortestPath) {
				ps_->oriPath = sp;
				break;
			}
		}
		if (ps_->oriPath == 0) {
			std::string str = "Warning => SP: oriPath(shortest path) for "
					+ ps_->id + " not valid anymore\n";
			logger << str;
		}
		//	no need of processing and storing blacklisted paths
		short psCnt = ps_->pathChoices.size();
		onPathSetRetrieval(ps_);
		r = getBestPathChoiceFromPathSet(ps_, partial,emptyBlkLst);
		logger << "[" << fromToID << "]" <<  " :  number of paths before blcklist: " << psCnt << " after blacklist:" << ps_->pathChoices.size() << "\n";
		if (r) {
			res = *(ps_->bestPath);
			//cache
			cachePathSet(ps_);
			logger << "returning a path " << res.size() << "\n";
			return true;
		} else {
			logger << "UNUSED DB hit\n";
		}
		break;
	}
	case PSM_NOTFOUND: {
		logger << "[" << fromToID << "]" <<  " : DB Miss\n";
		// Step-3 : If not found in DB, generate all 4 types of path
		logger << "[GENERATING PATHSET : " << fromToID << "]\n";
		// 1. generate shortest path with all segs
		// 1.2 get all segs
		// 1.3 generate shortest path with full segs

		//just to avoid
		ps_.reset(new PathSet(fromNode, toNode));
		ps_->id = fromToID;
		ps_->scenario = scenarioName;
		ps_->subTrip = st;
		std::set<OD> recursiveOrigins;
		bool r = generateAllPathChoices(ps_, recursiveOrigins, blckLstSegs);
		if (!r)
		{
			logger << "[PATHSET GENERATION FAILURE : " << fromToID << "]\n";
			tempNoPath.insert(fromToID);
			return false;
		}
		//this hack conforms to the CBD property added to segment and node
		if(useBlackList)
		{
			if(!purgeBlacklist(*ps_))
			{
				logger << "[ALL PATHS IN CBD" << fromToID << "]\n" ;
				tempNoPath.insert(fromToID);
				return false;
			}
		}
		logger << "[PATHSET GENERATED : " << fromToID << "]\n" ;
		onPathSetRetrieval(ps_, enRoute);
		r = getBestPathChoiceFromPathSet(ps_, partial,emptyBlkLst);
		if (r) {
			res = *(ps_->bestPath);
			//cache
			cachePathSet(ps_);
			logger << ps_->id	<< "WARNING not cached, apparently, already in cache. this is NOT and expected behavior!!\n";
			logger << "[RETURN PATH OF SIZE : " << res.size() << " : " << fromToID << "]\n";
			return true;
		}
		else
		{
			logger << "[NO PATH RETURNED EVEN AFTER GENERATING A PATHSET : " << fromToID << "]\n";
			return false;
		}
		break;
	}
	case PSM_NOGOODPATH:
	default: {
		tempNoPath.insert(fromToID);
		break;
	}
	};

	logger << "[FINALLY NO RESULT :  " << fromToID << "]\n";
	return false;
}

bool sim_mob::PathSetManager::generateAllPathChoices(boost::shared_ptr<sim_mob::PathSet> &ps, std::set<OD> &recursiveODs, const std::set<const sim_mob::RoadSegment*> & excludedSegs)
{
	std::string fromToID(getFromToString(ps->fromNode, ps->toNode));
	logger << "generateAllPathChoices" << std::endl;
	/**
	 * step-1: find the shortest path. if not found: create an entry in the "PathSet" table and return(without adding any entry into SinglePath table)
	 * step-2: K-SHORTEST PATH
	 * step-3: SHORTEST DISTANCE LINK ELIMINATION
	 * step-4: shortest travel time link elimination
	 * step-5: TRAVEL TIME HIGHWAY BIAS
	 * step-6: Random Pertubation
	 * step-7: Some caching/bookkeeping
	 * step-8: RECURSION!!!
	 */
	std::set<std::string> duplicateChecker;//for extra optimization only(creating singlepath and discarding it later can be expensive)
	sim_mob::SinglePath *s = findShortestDrivingPath(ps->fromNode,ps->toNode,duplicateChecker/*,excludedSegs*/);
	if(!s)
	{
		// no path
		if(tempNoPath.find(ps->id) == tempNoPath.end())
		{
			ps->hasPath = false;
			ps->isNeedSave2DB = true;
			tempNoPath.insert(ps->id);
		}
		return false;
	}
	s->pathSetId = fromToID;
	s->scenario += "SP";
	//some additional settings
	ps->hasPath = true;
	ps->isNeedSave2DB = true;
	ps->oriPath = s;
	ps->id = fromToID;

	//K-SHORTEST PATH
	//TODO:CONSIDER MERGING THE PREVIOUS OPERATION(findShortestDrivingPath) IN THE FOLLOWING OPERATION
	std::vector< std::vector<sim_mob::WayPoint> > ksp;
	std::set<sim_mob::SinglePath*, sim_mob::SinglePath> KSP_Storage;//main storage for k-shortest path
	int kspn = sim_mob::K_ShortestPathImpl::getInstance()->getKShortestPaths(ps->fromNode,ps->toNode,ksp);

	logger << "[" << fromToID << "][K-SHORTEST-PATH]\n";
	for(int i=0;i<ksp.size();++i)
	{
		std::vector<sim_mob::WayPoint> path_ = ksp[i];
		std::string id = sim_mob::makeWaypointsetString(path_);
		std::set<std::string>::iterator it_id =  duplicateChecker.find(id);
		if(it_id == duplicateChecker.end())
		{
			sim_mob::SinglePath *s = new sim_mob::SinglePath();
			// fill data
			s->isNeedSave2DB = true;
			s->id = id;
			s->pathSetId = fromToID;
			s->init(path_);
			s->scenario = ps->scenario + "KSP";
			s->pathSize=0;
			duplicateChecker.insert(id);
			KSP_Storage.insert(s);
			logger << "[KSP:" << i << "] " << s->id << "[length: " << s->length << "]\n";
		}
	}

	std::set<const RoadSegment*> blackList = std::set<const RoadSegment*>();
	sim_mob::Link *curLink = NULL;

	logger << "[" << fromToID << "][SHORTEST DISTANCE LINK ELIMINATION]\n";
	// SHORTEST DISTANCE LINK ELIMINATION
	//declare the profiler  but don't start profiling. it will just accumulate the elapsed time of the profilers who are associated with the workers
	std::vector<PathSetWorkerThread*> workPool;
	A_StarShortestPathImpl * impl = (A_StarShortestPathImpl*)stdir.getDistanceImpl();
	StreetDirectory::VertexDesc from = impl->DrivingVertex(*ps->fromNode);
	StreetDirectory::VertexDesc to = impl->DrivingVertex(*ps->toNode);
	StreetDirectory::Vertex* fromV = &from.source;
	StreetDirectory::Vertex* toV = &to.sink;
	if(ps->oriPath && !ps->oriPath->path.empty())
	{
		curLink = ps->oriPath->path.begin()->roadSegment_->getLink();
	}
	for(std::vector<sim_mob::WayPoint>::iterator it(ps->oriPath->path.begin());	it != ps->oriPath->path.end() ;++it)
	{
		const sim_mob::RoadSegment* currSeg = it->roadSegment_;
		if(currSeg->getLink() == curLink)
		{
			blackList.insert(currSeg);
		}
		else
		{
			curLink = currSeg->getLink();
			PathSetWorkerThread * work = new PathSetWorkerThread();
			//introducing the profiling time accumulator
			//the above declared profiler will become a profiling time accumulator of ALL workeres in this loop
			work->graph = &impl->drivingMap_;
			work->segmentLookup = &impl->drivingSegmentLookup_;
			work->fromVertex = fromV;
			work->toVertex = toV;
			work->fromNode = ps->fromNode;
			work->toNode = ps->toNode;
			work->excludeSeg = blackList;
			blackList.clear();
			work->ps = ps;
			std::stringstream out("");
			out << "sdle," << fromToID ;
			work->dbgStr = out.str();
			threadpool_->enqueue(boost::bind(&PathSetWorkerThread::executeThis,work));
			workPool.push_back(work);
		} //ROAD_SEGMENT
	}

	threadpool_->wait();
	logger << "[" << fromToID << "][SHORTEST TRAVEL TIME LINK ELIMINATION]\n";
	//step-3: SHORTEST TRAVEL TIME LINK ELIMINATION
	blackList.clear();
	curLink=NULL;
	A_StarShortestTravelTimePathImpl * sttpImpl = (A_StarShortestTravelTimePathImpl*)stdir.getTravelTimeImpl();
	from = sttpImpl->DrivingVertexNormalTime(*ps->fromNode);
	to = sttpImpl->DrivingVertexNormalTime(*ps->toNode);
	fromV = &from.source;dsnkfjsdkffile:///home/fm-simmobility/vahid/simmobility/dev/Basic/tempIncident/private/simrun_basic-1.xml
	toV = &to.sink;
	SinglePath *pathTT = generateShortestTravelTimePath(ps->fromNode,ps->toNode,duplicateChecker,sim_mob::Default);
	if(pathTT)
	{
		if(!pathTT->path.empty())
		{
			curLink = pathTT->path.begin()->roadSegment_->getLink();
		}
		for(std::vector<sim_mob::WayPoint>::iterator it(pathTT->path.begin()); it != pathTT->path.end() ;++it)
		{
			const sim_mob::RoadSegment* currSeg = it->roadSegment_;
			if(currSeg->getLink() == curLink)
			{
				blackList.insert(currSeg);
			}
			else
			{
				curLink = currSeg->getLink();
				PathSetWorkerThread *work = new PathSetWorkerThread();
				work->graph = &sttpImpl->drivingMap_Default;
				work->segmentLookup = &sttpImpl->drivingSegmentLookup_Default_;
				work->fromVertex = fromV;
				work->toVertex = toV;
				work->fromNode = ps->fromNode;
				work->toNode = ps->toNode;
				work->excludeSeg = blackList;
				blackList.clear();
				work->ps = ps;
				std::stringstream out("");
				out << "ttle," << fromToID ;
				work->dbgStr = out.str();
				threadpool_->enqueue(boost::bind(&PathSetWorkerThread::executeThis,work));
				workPool.push_back(work);
			} //ROAD_SEGMENT
		}//for
	}//if sinPathTravelTimeDefault

	threadpool_->wait();

	logger << "[" << fromToID << "][SHORTEST TRAVEL TIME LINK ELIMINATION HIGHWAY BIAS]\n";
	// TRAVEL TIME HIGHWAY BIAS
	//declare the profiler  but dont start profiling. it will just accumulate the elapsed time of the profilers who are associated with the workers
	blackList.clear();
	curLink = NULL;
	SinglePath *sinPathHightwayBias = generateShortestTravelTimePath(ps->fromNode,ps->toNode,duplicateChecker,sim_mob::HighwayBias_Distance);
	from = sttpImpl->DrivingVertexHighwayBiasDistance(*ps->fromNode);
	to = sttpImpl->DrivingVertexHighwayBiasDistance(*ps->toNode);
	fromV = &from.source;
	toV = &to.sink;
	if(sinPathHightwayBias)
	{
		if(!sinPathHightwayBias->path.empty())
		{
			curLink = sinPathHightwayBias->path.begin()->roadSegment_->getLink();
		}
		for(std::vector<sim_mob::WayPoint>::iterator it(sinPathHightwayBias->path.begin()); it != sinPathHightwayBias->path.end() ;++it)
		{
			const sim_mob::RoadSegment* currSeg = it->roadSegment_;
			if(currSeg->getLink() == curLink)
			{
				blackList.insert(currSeg);
			}
			else
			{
				curLink = currSeg->getLink();
				PathSetWorkerThread *work = new PathSetWorkerThread();
				//the above declared profiler will become a profiling time accumulator of ALL workeres in this loop
				//introducing the profiling time accumulator
				work->graph = &sttpImpl->drivingMap_HighwayBias_Distance;
				work->segmentLookup = &sttpImpl->drivingSegmentLookup_HighwayBias_Distance_;
				work->fromVertex = fromV;
				work->toVertex = toV;
				work->fromNode = ps->fromNode;
				work->toNode = ps->toNode;
				work->excludeSeg = blackList;
				blackList.clear();
				work->ps = ps;
				std::stringstream out("");
				out << "highway," << fromToID ;
				work->dbgStr = out.str();
				threadpool_->enqueue(boost::bind(&PathSetWorkerThread::executeThis,work));
				workPool.push_back(work);
			} //ROAD_SEGMENT
		}//for
	}//if sinPathTravelTimeDefault
	logger  << "waiting for TRAVEL TIME HIGHWAY BIAS" << "\n";
	threadpool_->wait();

	// generate random path
	for(int i=0;i<20;++i)//todo flexible number
	{
		from = sttpImpl->DrivingVertexRandom(*ps->fromNode,i);
		to = sttpImpl->DrivingVertexRandom(*ps->toNode,i);
		fromV = &from.source;
		toV = &to.sink;
		PathSetWorkerThread *work = new PathSetWorkerThread();
		//introducing the profiling time accumulator
		//the above declared profiler will become a profiling time accumulator of ALL workeres in this loop
		work->graph = &sttpImpl->drivingMap_Random_pool[i];
		work->segmentLookup = &sttpImpl->drivingSegmentLookup_Random_pool[i];
		work->fromVertex = fromV;
		work->toVertex = toV;
		work->fromNode = ps->fromNode;
		work->toNode = ps->toNode;
		work->ps = ps;
		std::stringstream out("");
		out << "random pertubation[" << i << "] ," << fromToID ;
		work->dbgStr = out.str();
		logger << work->dbgStr;
		threadpool_->enqueue(boost::bind(&PathSetWorkerThread::executeThis,work));
		workPool.push_back(work);
	}
	threadpool_->wait();
	//now that all the threads have concluded, get the total times
	//record
	//a.record the shortest path with all segments
	if(!ps->oriPath){
		std::string str = "path set " + ps->id + " has no shortest path\n" ;
		throw std::runtime_error(str);
	}
	if(!ps->oriPath->isShortestPath){
		std::string str = "path set " + ps->id + " is supposed to be the shortest path but it is not!\n" ;
		throw std::runtime_error(str);
	}
	ps->addOrDeleteSinglePath(ps->oriPath);
	//b. record k-shortest paths
	BOOST_FOREACH(sim_mob::SinglePath* sp, KSP_Storage)
	{
		ps->addOrDeleteSinglePath(sp);
	}
	//c. record the rest of the paths (link eliminations and random perturbation)
	BOOST_FOREACH(PathSetWorkerThread* p, workPool){
		if(p->hasPath){
			if(p->s->isShortestPath){
				std::string str = "Single path from pathset " + ps->id + " is not supposed to be marked as a shortest path but it is!\n" ;
				throw std::runtime_error(str);
			}
			ps->addOrDeleteSinglePath(p->s);
		}
	}
	//cleanupworkPool
	for(std::vector<PathSetWorkerThread*>::iterator wrkPoolIt=workPool.begin(); wrkPoolIt!=workPool.end(); wrkPoolIt++) {
		safe_delete_item(*wrkPoolIt);
	}
	workPool.clear();
	//step-7
	onGeneratePathSet(ps);
//#if 0
	//step -8 :
	boost::shared_ptr<sim_mob::PathSet> recursionPs;
	/*
	 * a) iterate through each path to first find ALL the multinodes to destination to make a set.
	 * c) then iterate through this set to choose each of them as a new Origin(Destination is same).
	 * call generateAllPathChoices on the new OD pair
	 */
	//a)
	std::set<Node*> newOrigins = std::set<Node*>();
	BOOST_FOREACH(sim_mob::SinglePath *sp, ps->pathChoices)
	{
		if(sp->path.size() <=1)
		{
			continue;
		}
		sim_mob::Node * linkEnd = nullptr;
		//skip the origin and destination node(first and last one)
		std::vector<WayPoint>::iterator it = sp->path.begin();
		it++;
		std::vector<WayPoint>::iterator itEnd = sp->path.end();
		itEnd--;
		for(; it != itEnd; it++)
		{
			//skip uninodes
			sim_mob::Node * newFrom = it->roadSegment_->getLink()->getEnd();
			// All segments of the link have the same link end node. Skip if already chosen
			if(linkEnd == newFrom)
			{
				continue;
			}
			else
			{
				linkEnd = newFrom;
			}
			//check if the new OD you want to process is not already scheduled for processing by previous iterations(todo: or even by other threads!)
			if(recursiveODs.insert(OD(newFrom,ps->toNode)).second == false)
			{
				continue;
			}
			//Now we have a new qualified Origin. note it down for further processing
			newOrigins.insert(newFrom);
		}
	}
	//b)
	BOOST_FOREACH(sim_mob::Node *from, newOrigins)
	{
		boost::shared_ptr<sim_mob::PathSet> recursionPs(new sim_mob::PathSet(from,ps->toNode));
		recursionPs->scenario = ps->scenario;
		generateAllPathChoices(recursionPs,recursiveODs);
	}
//#endif

	return true;
}

#if 0
//todo this method has already been reimplemented. delete this buggy function if you learnt how to use its sub functions
vector<WayPoint> sim_mob::PathSetManager::generateBestPathChoice2(const sim_mob::SubTrip* st)
{
	vector<WayPoint> res;
	std::set<std::string> duplicateChecker;
	std::string subTripId = st->tripID;
	if(st->mode != "Car") //only driver need path set
	{
		return res;
	}
	//find the node id
	const sim_mob::Node* fromNode = st->fromLocation.node_;
	const sim_mob::Node* toNode = st->toLocation.node_;
	std::string fromToID(getFromToString(fromNode,toNode));
	std::string pathSetID=fromToID;
	//check cache to save a trouble if the path already exists
	vector<WayPoint> cachedResult;
	//if(getCachedBestPath(fromToID,cachedResult))
	{
		return cachedResult;
	}
		//
		pathSetID = "'"+pathSetID+"'";
		boost::shared_ptr<PathSet> ps_;
#if 1
		bool hasPSinDB /*= sim_mob::aimsun::Loader::LoadOnePathSetDBwithId(
						ConfigManager::GetInstance().FullConfig().getDatabaseConnectionString(false),
						ps_,pathSetID)*/;
#else
		bool hasPSinDB = sim_mob::aimsun::Loader::LoadOnePathSetDBwithIdST(
						*getSession(),
						ConfigManager::GetInstance().FullConfig().getDatabaseConnectionString(false),
						ps_,pathSetID);
#endif
		if(ps_->hasPath == -1) //no path
		{
			return res;
		}
		if(hasPSinDB)
		{
			// init ps_
			if(!ps_->isInit)
			{
				ps_->subTrip = st;
				std::map<std::string,sim_mob::SinglePath*> id_sp;
#if 0
				//bool hasSPinDB = sim_mob::aimsun::Loader::LoadSinglePathDBwithId2(
#else
				bool hasSPinDB = sim_mob::aimsun::Loader::loadSinglePathFromDB(
						*getSession(),
#endif
						pathSetID,
						ps_->pathChoices,dbFunction);
				if(hasSPinDB)
				{
					std::map<std::string,sim_mob::SinglePath*>::iterator it ;//= id_sp.find(ps_->singlepath_id);
					if(it!=id_sp.end())
					{
//						ps_->oriPath = id_sp[ps_->singlepath_id];
						bool r = getBestPathChoiceFromPathSet(ps_);
						if(r)
						{
							res = *ps_->bestPath;// copy better than constant twisting
							// delete ps,sp
							BOOST_FOREACH(sim_mob::SinglePath* sp_, ps_->pathChoices)
							{
								if(sp_){
									delete sp_;
								}
							}
							ps_->pathChoices.clear();
							//insertFromTo_BestPath_Pool(fromToID,res);
							return res;
						}
						else
							return res;
					}
					else
					{
						std::string str = "gBestPC2: oriPath(shortest path) for "  + ps_->id + " not found in single path";
						logger << str <<  "\n";
						return res;
					}
				}// hasSPinDB
			}
		} // hasPSinDB
		else
		{
			logger<< "gBestPC2: create data for "<< "[" << fromToID << "]" <<  "\n";
			// 1. generate shortest path with all segs
			// 1.2 get all segs
			// 1.3 generate shortest path with full segs
			sim_mob::SinglePath *s = findShortestDrivingPath(fromNode,toNode,duplicateChecker);
			if(!s)
			{
				// no path
				ps_.reset(new PathSet(fromNode,toNode));
				ps_->hasPath = -1;
				ps_->isNeedSave2DB = true;
				ps_->id = fromToID;
//				ps_->fromNodeId = fromNode->originalDB_ID.getLogItem();
//				ps_->toNodeId = toNode->originalDB_ID.getLogItem();

//				std:string temp = fromNode->originalDB_ID.getLogItem();
//				ps_->fromNodeId = sim_mob::Utils::getNumberFromAimsunId(temp);
//				temp = toNode->originalDB_ID.getLogItem();
//				ps_->toNodeId = sim_mob::Utils::getNumberFromAimsunId(temp);

				ps_->scenario = scenarioName;
				std::map<std::string,boost::shared_ptr<sim_mob::PathSet> > tmp;
				tmp.insert(std::make_pair(fromToID,ps_));
//				sim_mob::aimsun::Loader::SaveOnePathSetData(*getSession(),tmp, pathSetTableName);
				return res;
			}
			//	// 1.31 check path pool
				// 1.4 create PathSet object
			ps_.reset(new PathSet(fromNode,toNode));
			ps_->hasPath = 1;
			ps_->subTrip = st;
			ps_->isNeedSave2DB = true;
			ps_->oriPath = s;
			ps_->id = fromToID;
			s->pathSetId = ps_->id;
			s->travelCost = getPathTravelCost(s,ps_->subTrip->mode, ps_->subTrip->startTime);
			s->travleTime = getPathTravelTime(s,ps_->subTrip->mode,ps_->subTrip->startTime);
			ps_->pathChoices.insert(s);
				// 2. exclude each seg in shortest path, then generate new shortest path
			generatePathesByLinkElimination(s->path,duplicateChecker,ps_,fromNode,toNode);
				// generate shortest travel time path (default,morning peak,evening peak, off time)
				generateTravelTimeSinglePathes(fromNode,toNode,duplicateChecker,ps_);

				// generate k-shortest paths
				std::vector< std::vector<sim_mob::WayPoint> > kshortestPaths;
				sim_mob::K_ShortestPathImpl::getInstance()->getKShortestPaths(ps_->fromNode,ps_->toNode,kshortestPaths);
//				ps_->fromNodeId = fromNode->originalDB_ID.getLogItem();
//				ps_->toNodeId = toNode->originalDB_ID.getLogItem();

//				std::string temp = fromNode->originalDB_ID.getLogItem();
//				ps_->fromNodeId = sim_mob::Utils::getNumberFromAimsunId(temp);
//				temp = toNode->originalDB_ID.getLogItem();
//				ps_->toNodeId = sim_mob::Utils::getNumberFromAimsunId(temp);

				ps_->scenario = scenarioName;
				// 3. store pathset
				sim_mob::generatePathSize(ps_);
				std::map<std::string,boost::shared_ptr<sim_mob::PathSet> > tmp;
				tmp.insert(std::make_pair(fromToID,ps_));
//				sim_mob::aimsun::Loader::SaveOnePathSetData(*getSession(),tmp, pathSetTableName);
				//
				bool r = getBestPathChoiceFromPathSet(ps_);
				if(r)
				{
					res = *ps_->bestPath;// copy better than constant twisting
					// delete ps,sp
					BOOST_FOREACH(sim_mob::SinglePath* sp_, ps_->pathChoices)
					{
						if(sp_){
							delete sp_;
						}
					}
					ps_->pathChoices.clear();
					return res;
				}
				else
					return res;
		}

	return res;
}
//todo ps_->pathChoices.insert(s) prone to memory leak
void sim_mob::PathSetManager::generatePathesByLinkElimination(std::vector<WayPoint>& path,
			std::set<std::string>& duplicateChecker,
			boost::shared_ptr<sim_mob::PathSet> &ps_,
			const sim_mob::Node* fromNode,
			const sim_mob::Node* toNode)
{
	for(int i=0;i<path.size();++i)
	{
		WayPoint &w = path[i];
		if (w.type_ == WayPoint::ROAD_SEGMENT) {
			std::set<const sim_mob::RoadSegment*> seg ;
			seg.insert(w.roadSegment_);
			SinglePath *sinPath = findShortestDrivingPath(fromNode,toNode,duplicateChecker,seg);
			if(!sinPath)
			{
				continue;
			}
			sinPath->travelCost = getPathTravelCost(sinPath,ps_->subTrip->startTime);
			sinPath->travleTime = getPathTravelTime(sinPath,ps_->subTrip->startTime);
			sinPath->pathSetId = ps_->id;
			ps_->pathChoices.insert(sinPath);
		}
	}//end for
}

//todo ps_->pathChoices.insert(s) prone to memory leak
void sim_mob::PathSetManager::generatePathesByTravelTimeLinkElimination(std::vector<WayPoint>& path,
		std::set<std::string>& duplicateChecker,
				boost::shared_ptr<sim_mob::PathSet> &ps_,
				const sim_mob::Node* fromNode,
				const sim_mob::Node* toNode,
				sim_mob::TimeRange tr)
{
	for(int i=0;i<path.size();++i)
	{
		WayPoint &w = path[i];
		if (w.type_ == WayPoint::ROAD_SEGMENT) {
			const sim_mob::RoadSegment* seg = w.roadSegment_;
			SinglePath *sinPath = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,tr,seg);
			if(!sinPath)
			{
				continue;
			}
			sinPath->travelCost = getPathTravelCost(sinPath,ps_->subTrip->startTime);
			sinPath->travleTime = getPathTravelTime(sinPath,ps_->subTrip->startTime);
			sinPath->pathSetId = ps_->id;
//			storePath(sinPath);
			ps_->pathChoices.insert(sinPath);
		}
	}//end for
}

//todo ps_->pathChoices.insert(s) prone to memory leak
void sim_mob::PathSetManager::generateTravelTimeSinglePathes(const sim_mob::Node *fromNode,
		   const sim_mob::Node *toNode,
		   std::set<std::string>& duplicateChecker,boost::shared_ptr<sim_mob::PathSet> &ps_)
{
	SinglePath *sinPath_morningPeak = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::MorningPeak);
	if(sinPath_morningPeak)
	{
		sinPath_morningPeak->travelCost = getPathTravelCost(sinPath_morningPeak,ps_->subTrip->startTime);
		sinPath_morningPeak->travleTime = getPathTravelTime(sinPath_morningPeak,ps_->subTrip->startTime);
		sinPath_morningPeak->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath_morningPeak);
		generatePathesByTravelTimeLinkElimination(sinPath_morningPeak->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::MorningPeak);
	}
	SinglePath *sinPath_eveningPeak = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::EveningPeak);
	if(sinPath_eveningPeak)
	{
		sinPath_eveningPeak->travelCost = getPathTravelCost(sinPath_eveningPeak,ps_->subTrip->startTime);
		sinPath_eveningPeak->travleTime = getPathTravelTime(sinPath_eveningPeak,ps_->subTrip->startTime);
		sinPath_eveningPeak->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath_eveningPeak);
		generatePathesByTravelTimeLinkElimination(sinPath_eveningPeak->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::EveningPeak);
	}
	SinglePath *sinPath_offPeak = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::OffPeak);
	if(sinPath_offPeak)
	{
		sinPath_offPeak->travelCost = getPathTravelCost(sinPath_offPeak,ps_->subTrip->startTime);
		sinPath_offPeak->travleTime = getPathTravelTime(sinPath_offPeak,ps_->subTrip->startTime);
		sinPath_offPeak->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath_offPeak);
		generatePathesByTravelTimeLinkElimination(sinPath_offPeak->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::OffPeak);
	}
	SinglePath *sinPath_default = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::Default);
	if(sinPath_default)
	{
		sinPath_default->travelCost = getPathTravelCost(sinPath_default,ps_->subTrip->startTime);
		sinPath_default->travleTime = getPathTravelTime(sinPath_default,ps_->subTrip->startTime);
		sinPath_default->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath_default);
		generatePathesByTravelTimeLinkElimination(sinPath_default->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::Default);
	}
	// generate high way bias path
	SinglePath *sinPath = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::HighwayBias_Distance);
	if(sinPath)
	{
		sinPath->travelCost = getPathTravelCost(sinPath,ps_->subTrip->startTime);
		sinPath->travleTime = getPathTravelTime(sinPath,ps_->subTrip->startTime);
		sinPath->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath);
		generatePathesByTravelTimeLinkElimination(sinPath->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::HighwayBias_Distance);
	}
	sinPath = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::HighwayBias_MorningPeak);
	if(sinPath)
	{
		sinPath->travelCost = getPathTravelCost(sinPath,ps_->subTrip->startTime);
		sinPath->travleTime = getPathTravelTime(sinPath,ps_->subTrip->startTime);
		sinPath->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath);
		generatePathesByTravelTimeLinkElimination(sinPath->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::HighwayBias_MorningPeak);
	}
	sinPath = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::HighwayBias_EveningPeak);
	if(sinPath)
	{
		sinPath->travelCost = getPathTravelCost(sinPath,ps_->subTrip->startTime);
		sinPath->travleTime = getPathTravelTime(sinPath,ps_->subTrip->startTime);
		sinPath->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath);
		generatePathesByTravelTimeLinkElimination(sinPath->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::HighwayBias_MorningPeak);
	}
	sinPath = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::HighwayBias_OffPeak);
	if(sinPath)
	{
		sinPath->travelCost = getPathTravelCost(sinPath,ps_->subTrip->startTime);
		sinPath->travleTime = getPathTravelTime(sinPath,ps_->subTrip->startTime);
		sinPath->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath);
		generatePathesByTravelTimeLinkElimination(sinPath->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::HighwayBias_MorningPeak);
	}
	sinPath = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::HighwayBias_Default);
	if(sinPath)
	{
		sinPath->travelCost = getPathTravelCost(sinPath,ps_->subTrip->startTime);
		sinPath->travleTime = getPathTravelTime(sinPath,ps_->subTrip->startTime);
		sinPath->pathSetId = ps_->id;
		ps_->pathChoices.insert(sinPath);
		//
		generatePathesByTravelTimeLinkElimination(sinPath->path,duplicateChecker,ps_,fromNode,toNode,sim_mob::HighwayBias_Default);
	}
	// generate random path
	for(int i=0;i<20;++i)
	{
		const sim_mob::RoadSegment *rs = NULL;
		sinPath = generateShortestTravelTimePath(fromNode,toNode,duplicateChecker,sim_mob::Random,rs,i);
		if(sinPath)
		{
			sinPath->travelCost = getPathTravelCost(sinPath,ps_->subTrip->startTime);
			sinPath->travleTime = getPathTravelTime(sinPath,ps_->subTrip->startTime);
			sinPath->pathSetId = ps_->id;
			ps_->pathChoices.insert(sinPath);
		}
	}
}
#endif

std::map<long long,sim_mob::OneTimeFlag> utilityLogger;

double sim_mob::PathSetManager::generatePartialUtility(const sim_mob::SinglePath* sp) const
{
	double pUtility = 0;
	if(!sp)
	{
		return pUtility;
	}
	pUtility += sp->pathSize * pathSetParam->bCommonFactor;
	//3.0
	//Obtain the travel distance l and the highway distance w of the path.
	pUtility += sp->length * pathSetParam->bLength + sp->highWayDistance * pathSetParam->bHighway;
	//4.0
	//Obtain the travel cost c of the path.
//	pUtility += sp->travelCost * pathSetParam->bCost;
	//5.0
	//Obtain the number of signalized intersections s of the path.
	pUtility += sp->signalNumber * pathSetParam->bSigInter;
	//6.0
	//Obtain the number of right turns f of the path.
	pUtility += sp->rightTurnNumber * pathSetParam->bLeftTurns;
	//8.0
	//min distance param
	if(sp->isMinDistance == 1)
	{
		pUtility += pathSetParam->minDistanceParam;
	}
	//9.0
	//min signal param
	if(sp->isMinSignal == 1)
	{
		pUtility += pathSetParam->minSignalParam;
	}
	//10.0
	//min highway param
	if(sp->isMaxHighWayUsage == 1)
	{
		pUtility += pathSetParam->maxHighwayParam;
	}
	//Obtain the trip purpose.
	if(sp->purpose == sim_mob::work)
	{
		pUtility += sp->purpose * pathSetParam->bWork;
	}
	else if(sp->purpose == sim_mob::leisure)
	{
		pUtility += sp->purpose * pathSetParam->bLeisure;
	}

	if(utilityLogger[sp->index].check())
	{
		sp->partialUtilityDbg << sp->pathSetId << "," << "," << sp->index << sp->travleTime << "," << pathSetParam->bTTVOT <<  ", " << sp->travleTime * pathSetParam->bTTVOT
				<< sp->pathSize << "," << pathSetParam->bCommonFactor << "," <<  sp->pathSize * pathSetParam->bCommonFactor
				<< sp->length << "," << pathSetParam->bLength << ","   <<  sp->length * pathSetParam->bLength
				<< sp->highWayDistance << ","  <<  pathSetParam->bHighway << "," << sp->highWayDistance * pathSetParam->bHighway
				<< sp->travelCost << "," <<   pathSetParam->bCost << ","  << sp->travelCost * pathSetParam->bCost
				<< sp->signalNumber << ","  <<   pathSetParam->bSigInter << "," << sp->signalNumber * pathSetParam->bSigInter
				<< sp->rightTurnNumber << "," << pathSetParam->bLeftTurns << "," << sp->rightTurnNumber * pathSetParam->bLeftTurns
				<< pathSetParam->minTravelTimeParam << "," <<  (sp->isMinTravelTime == 1) << pathSetParam->minTravelTimeParam *  (sp->isMinTravelTime == 1 ? 1 : 0)
				<< pathSetParam->minDistanceParam << "," << (sp->isMinDistance == 1) << "," << pathSetParam->minDistanceParam * (sp->isMinDistance == 1 ? 1 : 0)
				<< pathSetParam->minSignalParam  << "," << (sp->isMinSignal == 1) << "," << pathSetParam->minSignalParam * (sp->isMinSignal == 1 ? 1 : 0)
				<< pathSetParam->maxHighwayParam  << (sp->isMaxHighWayUsage == 1)  << pathSetParam->maxHighwayParam * (sp->isMaxHighWayUsage == 1 ? 1 : 0)
				<< sp->purpose << ","  << (sp->purpose == sim_mob::work ? pathSetParam->bWork : pathSetParam->bLeisure) << "," << sp->purpose  * (sp->purpose == sim_mob::work ? pathSetParam->bWork : pathSetParam->bLeisure)
				 << "," << pUtility ;
	}

	return pUtility;

}

double sim_mob::PathSetManager::generateUtility(const sim_mob::SinglePath* sp) const
{
	double utility=0;
	if(!sp)
	{
		return utility;
	}

	if(sp->travleTime <= 0.0)
	{
		throw std::runtime_error("generateUtility: invalid single path travleTime :");
	}

	utility = (sp->partialUtility > 0.0 ? sp->partialUtility : generatePartialUtility(sp)) ;
	// calculate utility
	//Obtain value of time for the agent A: bTTlowVOT/bTTmedVOT/bTThiVOT.
	utility += sp->travleTime * pathSetParam->bTTVOT;
	//obtain travel cost part of utility
	utility += sp->travelCost * pathSetParam->bCost;
	std::stringstream out("");
	return utility;
}

bool sim_mob::PathSetManager::getBestPathChoiceFromPathSet(boost::shared_ptr<sim_mob::PathSet> &ps,
		const std::set<const sim_mob::RoadSegment *> & partialExclusion ,
		const std::set<const sim_mob::RoadSegment*> &blckLstSegs)
{
	std::stringstream out("");
	out << "path_selection_logger" << ps->id << ".csv";
	bool computeUtility = false;
	// step 1.1 : For each path i in the path choice:
	//1. set PathSet(O, D)
	//2. travle_time
	//3. utility
	//step 1.2 : accumulate the logsum
	double maxTravelTime = std::numeric_limits<double>::max();
	ps->logsum = 0.0;
	std:ostringstream utilityDbg("");
	utilityDbg << "***********\nPATH Selection for :" << ps->id << " : \n" ;
	int iteration = 0;
	BOOST_FOREACH(sim_mob::SinglePath* sp, ps->pathChoices)
	{
		if(blckLstSegs.size() && sp->includesRoadSegment(blckLstSegs))
		{
			continue;//do the same thing while measuring the probability in the loop below
		}
		if(sp->path.empty())
		{
			std::string str = iteration + " Singlepath empty";
			throw std::runtime_error (str);
		}
		//	state of the network changed
		//debug
		if(sp->travleTime <= 0.0 )
		{
			std::stringstream out("");
			out << "getBestPathChoiceFromPathSet=>invalid single path travleTime :" << sp->travleTime;
			throw std::runtime_error(out.str());
		}
		//debug..
		if (partialExclusion.size() && sp->includesRoadSegment(partialExclusion) ) {
			sp->travleTime = maxTravelTime;//some large value like infinity
			//	RE-calculate utility
			sp->utility = generateUtility(sp);
		}
		//this is done in onPathSetRetrieval so no need to repeat for now
//		sp->travleTime = getPathTravelTime(sp,ps->subTrip->startTime);
//		sp->travelCost = getPathTravelCost(sp,ps->subTrip->startTime);
		utilityDbg << "[" << sp->utility << "," << exp(sp->utility) << "]";
		ps->logsum += exp(sp->utility);
		iteration++;
	}

	// step 2: find the best waypoint path :
	// calculate a probability using path's utility and pathset's logsum,
	// compare the resultwith a  random number to decide whether pick the current path as the best path or not
	//if not, just chose the shortest path as the best path
	double upperProb=0;
	// 2.1 Draw a random number X between 0.0 and 1.0 for agent A.
	double x = sim_mob::gen_random_float(0,1);
	// 2.2 For each path i in the path choice set PathSet(O, D):
	int i = -1;
	utilityDbg << "\nlogsum : " << ps->logsum << "\nX : " << x << "\n";
	BOOST_FOREACH(sim_mob::SinglePath* sp, ps->pathChoices)
	{
		if(blckLstSegs.size() && sp->includesRoadSegment(blckLstSegs))
		{
			continue;//do the same thing while processing the single path in the loop above
		}
		i++;
		double prob = exp(sp->utility)/(ps->logsum);
		utilityDbg << prob << " , " ;
		upperProb += prob;
		if (x <= upperProb)
		{
			// 2.3 agent A chooses path i from the path choice set.
			ps->bestPath = &(sp->path);
			logger << "[LOGIT][" << sp->pathSetId <<  "] [" << i << " out of " << ps->pathChoices.size()  << " paths chosen] [UTIL: " <<  sp->utility << "] [LOGSUM: " << ps->logsum << "][exp(sp->utility)/(ps->logsum) : " << prob << "][X:" << x << "]\n";
			utilityDbg << "upperProb reached : " << upperProb << "\n";
			utilityDbg << "***********\n";
//			sim_mob::Logger::log(out.str()) << utilityDbg.str();
			return true;
		}
	}
	utilityDbg << "***********\n";
//	sim_mob::Logger::log(out.str()) << utilityDbg.str();

	// path choice algorithm
	if(!ps->oriPath)//return upon null oriPath only if the condition is normal(excludedSegs is empty)
	{
		logger<< "NO PATH , getBestPathChoiceFromPathSet, shortest path empty" << "\n";
		return false;
	}
	//the last step resorts to selecting and returning shortest path(aka oripath).
	logger << "NO BEST PATH. select to shortest path\n" ;
	ps->bestPath = &(ps->oriPath->path);
	return true;
}

sim_mob::SinglePath *  sim_mob::PathSetManager::findShortestDrivingPath(
		   const sim_mob::Node *fromNode,
		   const sim_mob::Node *toNode,std::set<std::string> duplicatePath,
		   const std::set<const sim_mob::RoadSegment*> & excludedSegs)
{
	/**
	 * step-1: find the shortest driving path between the given OD
	 * step-2: turn the outputted waypoint container into a string to be used as an id
	 * step-3: create a new Single path object
	 * step-4: return the resulting singlepath object as well as add it to the container supplied through args
	 */
	sim_mob::SinglePath *s=NULL;
	std::vector<WayPoint> wp = stdir.SearchShortestDrivingPath(stdir.DrivingVertex(*fromNode), stdir.DrivingVertex(*toNode));
	if(wp.empty())
	{
		// no path debug message
		std::stringstream out("");
		if(excludedSegs.size())
		{

			const sim_mob::RoadSegment* rs;
			out << "\nWith Excluded Segments Present: \n";
			BOOST_FOREACH(rs, excludedSegs)
			{
				out <<	rs->originalDB_ID.getLogItem() << "]" << ",";
			}
		}
		logger<< "No shortest driving path for nodes[" << fromNode->originalDB_ID.getLogItem() << "] and [" <<
		toNode->originalDB_ID.getLogItem() << "]" << out.str() << "\n";
		return s;
	}
	// make sp id
	std::string id = sim_mob::makeWaypointsetString(wp);
	if(!id.size()){
		logger << "Error: Empty shortest path for OD:" <<  fromNode->getID() << "," << toNode->getID() << "\n" ;
	}
	// 1.31 check path pool
	std::set<std::string>::iterator it =  duplicatePath.find(id);
	// no stored path found, generate new one
	if(it==duplicatePath.end())
	{
		s = new SinglePath();
		// fill data
		s->isNeedSave2DB = true;
		s->init(wp);
		s->id = id;
		s->scenario = scenarioName;
		s->isShortestPath = true;
		duplicatePath.insert(id);
	}
	else{
		logger<<"gSPByFTNodes3:duplicate pathset discarded\n";
	}

	return s;
}

sim_mob::SinglePath* sim_mob::PathSetManager::generateShortestTravelTimePath(const sim_mob::Node *fromNode,
			   const sim_mob::Node *toNode,
			   std::set<std::string>& duplicateChecker,
			   sim_mob::TimeRange tr,
			   const sim_mob::RoadSegment* excludedSegs,int random_graph_idx)
{
	sim_mob::SinglePath *s=NULL;
		std::vector<const sim_mob::RoadSegment*> blacklist;
		if(excludedSegs)
		{
			blacklist.push_back(excludedSegs);
		}
		std::vector<WayPoint> wp = stdir.SearchShortestDrivingTimePath(stdir.DrivingTimeVertex(*fromNode,tr,random_graph_idx),
				stdir.DrivingTimeVertex(*toNode,tr,random_graph_idx),
				blacklist,
				tr,
				random_graph_idx);
		if(wp.size()==0)
		{
			// no path
			logger<<"generateShortestTravelTimePath: no path for nodes"<<fromNode->originalDB_ID.getLogItem()<<
							toNode->originalDB_ID.getLogItem() << "\n";
			return s;
		}
		// make sp id
		std::string id = sim_mob::makeWaypointsetString(wp);
		// 1.31 check path pool
		std::set<std::string>::iterator it =  duplicateChecker.find(id);
		// no stored path found, generate new one
		if(it==duplicateChecker.end())
		{
			s = new SinglePath();
			// fill data
			s->isNeedSave2DB = true;
			s->init(wp);
			s->id = id;
			s->scenario = scenarioName;
			s->pathSize=0;
			duplicateChecker.insert(id);
		}
		else{
			logger<<"generateShortestTravelTimePath:duplicate pathset discarded\n";
		}

		return s;
}

void sim_mob::generatePathSize(boost::shared_ptr<sim_mob::PathSet>&ps)
{
	//sanity check
	if(ps->pathChoices.empty())
	{
		throw std::runtime_error("Cannot generate path size for an empty pathset");
	}
	double minL = 0;
	// Step 1: the length of each path in the path choice set

	bool uniquePath;
	//pathsize
	BOOST_FOREACH(sim_mob::SinglePath* sp, ps->pathChoices)
	{
		uniquePath = true; //this variable checks if a path has No common segments with the rest of the pathset
		double size=0.0;

		if(sp->path.empty())
		{
			throw std::runtime_error ("unexpected empty path in singlepath object");
		}
		// For each link a in the path:
		for(std::vector<WayPoint>::iterator it1=sp->path.begin(); it1!=sp->path.end(); ++it1)
		{
			const sim_mob::RoadSegment* seg = it1->roadSegment_;
			sim_mob::SinglePath* minSp = findShortestPath_LinkBased(ps->pathChoices, seg);
			if(minSp == nullptr)
			{
				std::stringstream out("");
				out << "couldn't find a min path for segment " << seg->getId();
				throw std::runtime_error(out.str());
			}
			minL = minSp->length;
			double l = seg->length / 100.0;
			double sum = 0.0;
			//For each path j in the path choice set PathSet(O, D):
			BOOST_FOREACH(sim_mob::SinglePath* spj, ps->pathChoices)
			{
				if(spj->segSet.empty())
				{
					throw std::runtime_error("segSet of singlepath object is Empty");
				}
				std::set<const sim_mob::RoadSegment*>::iterator itt2 = std::find(spj->segSet.begin(), spj->segSet.end(), seg);
				if(itt2 != spj->segSet.end())
				{
					sum += minL/(spj->length);
					if(sp->id != spj->id)
					{
						uniquePath = false;
					}
				}
			} // for j
			size += l / sp->length / sum;
		}
		//is this a unique path ?
		if(uniquePath)
		{
			sp->pathSize = 0;
		}
		else
		{
			//calculate path size
			sp->pathSize = log(size);
		}
	}// end for
}

double sim_mob::PathSetManager::getPathTravelTime(sim_mob::SinglePath *sp,const std::string & travelMode, const sim_mob::DailyTime & startTime_, bool enRoute)
{
	sim_mob::DailyTime startTime = startTime_;
	std::stringstream out("");
	out << "\n";
	double ts=0.0;
	for(int i=0;i<sp->path.size();++i)
	{
		out << i << " " ;
//		if(sp->path[i].type_ == WayPoint::ROAD_SEGMENT){
////			unsigned long segId = sp->path[i].roadSegment_->getId();
		double t = 0.0;
		if(enRoute)
		{
			t = getInSimulationSegTT(sp->path[i].roadSegment_,travelMode, startTime);
		}
		if(!enRoute || t == 0.0)
		{
			t = sim_mob::PathSetParam::getInstance()->getSegTT(sp->path[i].roadSegment_,travelMode, startTime);
		}
		if(t == 0.0)
		{
			Warn() << "No Travel Time for segment " << sp->path[i].roadSegment_->getId() << " Found, setting it to zero!\n";
		}
		ts += t;
		startTime = startTime + sim_mob::DailyTime(t*1000);
		out << t << " " << ts << startTime.getRepr_() ;
//		}
	}
	if (ts <=0.0)
	{
		throw std::runtime_error(out.str());
	}
	//logger << "Retrurn TT = " << ts << out.str() << "\n" ;
	return ts;
}


void sim_mob::PathSetManager::addSegTT(const Agent::RdSegTravelStat & stats) {
	processTT.addTravelTime(stats);
}

double sim_mob::PathSetManager::getInSimulationSegTT(const sim_mob::RoadSegment* rs, const std::string &travelMode, const sim_mob::DailyTime &startTime)
{
	return processTT.getInSimulationSegTT(travelMode,rs);
}

void sim_mob::PathSetManager::initTimeInterval()
{
	intervalMS = sim_mob::ConfigManager::GetInstance().FullConfig().pathSet().interval* 1000 /*milliseconds*/;
	uint32_t startTm = ConfigManager::GetInstance().FullConfig().simStartTime().getValue();
	curIntervalMS = TravelTimeManager::getTimeInterval(startTm, intervalMS);
}

void sim_mob::PathSetManager::updateCurrTimeInterval()
{
	curIntervalMS += intervalMS;
}


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
sim_mob::SinglePath* findShortestPath_LinkBased(const std::set<sim_mob::SinglePath*, sim_mob::SinglePath> &pathChoices, const sim_mob::Link *ln)
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
sim_mob::SinglePath* findShortestPath_LinkBased(const std::set<sim_mob::SinglePath*, sim_mob::SinglePath> &pathChoices, const sim_mob::RoadSegment *rs)
{
	const sim_mob::Link *ln = rs->getLink();
	return findShortestPath_LinkBased(pathChoices,ln);
}

double getPathTravelCost(sim_mob::SinglePath *sp,const std::string & travelMode, const sim_mob::DailyTime & startTime_)
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
		double t = sim_mob::PathSetParam::getInstance()->getSegTT((it1)->roadSegment_,travelMode, tripStartTime);
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

