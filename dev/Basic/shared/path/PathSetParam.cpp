#include "PathSetParam.hpp"
#include "PathSetManager.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "util/Profiler.hpp"
#include <boost/thread.hpp>

namespace{
sim_mob::BasicLogger & logger = sim_mob::Logger::log("path_set");
}

sim_mob::PathSetParam *sim_mob::PathSetParam::instance_ = NULL;

sim_mob::PathSetParam* sim_mob::PathSetParam::getInstance()
{
	if(!instance_)
	{
		instance_ = new PathSetParam();
	}
	return instance_;
}

void sim_mob::PathSetParam::getDataFromDB()
{
	setRTTT(ConfigManager::GetInstance().FullConfig().getRTTT());
	logger << "[RTT TABLE NAME : " << RTTT << "]\n";
	std::cout << "[RTT TABLE NAME : " << RTTT << "]\n";
	sim_mob::aimsun::Loader::LoadERPData(ConfigManager::GetInstance().FullConfig().getDatabaseConnectionString(false),
			ERP_SurchargePool,	ERP_Gantry_ZonePool,ERP_SectionPool);
	std::cout << "[ERP RETRIEVED " <<	ERP_SurchargePool.size() << " "  << ERP_Gantry_ZonePool.size() << " " << ERP_SectionPool.size() << "]\n";

	sim_mob::aimsun::Loader::LoadDefaultTravelTimeData(*(PathSetManager::getSession()), segDefTT);
	std::cout << segDefTT.size() << " records for Link_default_travel_time found\n";

	bool res = sim_mob::aimsun::Loader::LoadRealTimeTravelTimeData(*(PathSetManager::getSession()),
			RTTT, sim_mob::ConfigManager::GetInstance().FullConfig().pathSet().interval, segHistoryTT);
	std::cout << segHistoryTT.size() << " records for Link_realtime_travel_time found " << segHistoryTT.begin()->first << "  " << segHistoryTT.end()->first << "\n";
	if(!res) // no realtime travel time table
	{
		//create
		if(!createTravelTimeRealtimeTable() )
		{
			throw std::runtime_error("can not create travel time table");
		}
	}
//	getchar();
}
void sim_mob::PathSetParam::storeSinglePath(soci::session& sql,std::set<sim_mob::SinglePath*, sim_mob::SinglePath>& spPool,const std::string pathSetTableName)
{
	sim_mob::aimsun::Loader::storeSinglePath(sql,spPool,pathSetTableName);
}

bool sim_mob::PathSetParam::createTravelTimeRealtimeTable()
{
	bool res=false;
	std::string createTableStr = "create table " + RTTT +
			"("
			"link_id integer NOT NULL,"
			"start_time time without time zone NOT NULL,"
			"end_time time without time zone NOT NULL,"
			"travel_time double precision NOT NULL,"
	        "interval_time integer NOT NULL,"
			"travel_mode character varying NOT NULL,"
			"history integer NOT NULL DEFAULT 1"
			")"
			"WITH ("
			"  OIDS=FALSE"
			");"
			"ALTER TABLE " + RTTT +
			"  OWNER TO postgres;";
	res = sim_mob::aimsun::Loader::excuString(*(PathSetManager::getSession()),createTableStr);
	return res;
}

void sim_mob::PathSetParam::setRTTT(const std::string& value)
{
	if(!value.size())
	{
		throw std::runtime_error("Missing Travel Time Table Name.\n "
				"It is either missing in the XML configuration file,\n"
				"or you are trying to access the file name before reading the Configuration file");
	}
	RTTT = value;
	logger << "[REALTIME TABLE NAME : " << RTTT << "]\n";
}

double sim_mob::PathSetParam::getSegRangeTT(const sim_mob::RoadSegment* rs,const std::string travelMode, sim_mob::DailyTime startTime,sim_mob::DailyTime endTime)
{
	//1. check realtime table
	double res=0.0;
	double totalTravelTime=0.0;
	int count = 0;
	sim_mob::DailyTime dailyTime(startTime);
	TT::TI endTimeRange = TravelTimeManager::getTimeInterval(endTime.getValue(), intervalMS);
	while(dailyTime.isBeforeEqual(endTime))
	{
		double tt = getSegTT(rs,travelMode, dailyTime);
		totalTravelTime += tt;
		if(tt > 0.0)
		{
			count ++;
		}
		dailyTime = sim_mob::DailyTime(dailyTime.getValue() + intervalMS) ;
	}
	return (count ? totalTravelTime / count : 0.0);
}

double sim_mob::PathSetParam::getDefSegTT(const sim_mob::RoadSegment* rs)const
{
	/*Note:
	 * this method doesn't look at the intended time range.
	 * Instead, it searches for all occurances of the given road segment in the
	 * default travel time container, and returns an average.
	 */
	double res = 0.0;
	double totalTravelTime = 0.0;
	int count = 0;
	std::map<unsigned long,std::vector<sim_mob::LinkTravelTime> >::const_iterator it;
	it = segDefTT.find(rs->getId());

	if(it == segDefTT.end() || it->second.empty())
	{
		logger << "[NO DTT FOR : " <<  rs->getId() << "]\n";
		return 0.0;
	}

	const std::vector<sim_mob::LinkTravelTime> &e = (*it).second;
	BOOST_FOREACH(const sim_mob::LinkTravelTime& l, e)
	{
		//discard the invalid values, if any
		if(l.travelTime)
		{
			totalTravelTime += l.travelTime;
			count++;
		}
	}

	res = totalTravelTime / count;
	return res;
}

double sim_mob::PathSetParam::getDefSegTT(const sim_mob::RoadSegment* rs, const sim_mob::DailyTime &startTime)
{
	/*
	 *	Note:
	 *	This method searches for the target segment,
	 *	if found, it returns the first occurrence of travel time
	 *	which includes the given time
	 */

	std::ostringstream dbg("");
	std::map<unsigned long,std::vector<sim_mob::LinkTravelTime> >::iterator it = segDefTT.find(rs->getId());

	if(it ==segDefTT.end())
	{
		logger <<  "[NOTT] " << rs->getId() << "\n";
		return 0.0;
	}

	std::vector<sim_mob::LinkTravelTime> &e = (*it).second;
	for(std::vector<sim_mob::LinkTravelTime>::iterator itL(e.begin());itL != e.end();++itL)
	{
		sim_mob::LinkTravelTime& l = *itL;
		if( l.startTime_DT.isBeforeEqual(startTime) && l.endTime_DT.isAfter(startTime) )
		{
			logger << rs->getId() << "  " << startTime.getRepr_() << " [DEFTT] " <<  "  " << dbg.str() << "\n";
			return l.travelTime;
		}
	}

	return 0.0;
}

double sim_mob::PathSetParam::getHistorySegTT(const sim_mob::RoadSegment* rs, const std::string &travelMode, const sim_mob::DailyTime &startTime)
{
	std::ostringstream dbg("");
	//1. check realtime table
	double res = 0.0;
	TT::TI timeInterval = TravelTimeManager::getTimeInterval(startTime.getValue(),intervalMS);
	AverageTravelTime::iterator itRange = segHistoryTT.find(timeInterval);
	if(itRange != segHistoryTT.end())
	{
		sim_mob::TT::MST & mst = itRange->second;
		sim_mob::TT::MST::iterator itMode = mst.find(travelMode);
		if(itMode != mst.end())
		{
			std::map<const sim_mob::RoadSegment*,double >::iterator itRS = itMode->second.find(rs);
			if(itRS != itMode->second.end())
			{
				logger << startTime.getRepr_() << " [REALTT] " <<  "  " << dbg.str() << "\n";
				return itRS->second;
			}
		}
	}
	return 0.0;
}

double sim_mob::PathSetParam::getSegTT(const sim_mob::RoadSegment* rs, const std::string &travelMode, const sim_mob::DailyTime &startTime)
{
	std::ostringstream out("");
	//1. check realtime table
	double res = 0.0;
	if((res = getHistorySegTT(rs, travelMode, startTime)) > 0.0)
	{
		return res;
	}
//	else
//	{
//		out << "couldn't find timeInterval " << timeInterval  << " [" << startTime.getRepr_() << " ### " <<  startTime.getValue() << "] minus ["
//				<< sim_mob::ConfigManager::GetInstance().FullConfig().simStartTime().getRepr_() << " ### " << sim_mob::ConfigManager::GetInstance().FullConfig().simStartTime().getValue() << "] = " <<
//				(startTime - sim_mob::ConfigManager::GetInstance().FullConfig().simStartTime()).getRepr_() << " ### " <<
//				(startTime - sim_mob::ConfigManager::GetInstance().FullConfig().simStartTime()).getValue() << "]"
//				 << " / [intervalMS:"<< intervalMS << "] =>" << timeInterval << "\n";
//		//logger << "[NO REALTT] " << id << "\n";
//	}
	//2. if not , check default
	if((res = getDefSegTT(rs, startTime)) > 0.0)
	{
		return res;
	}
	return res;
}

//sim_mob::Node* sim_mob::PathSetParam::getCachedNode(std::string id)
//{
//	std::map<std::string,sim_mob::Node*>::iterator it = nodePool.find(id);
//	if(it != nodePool.end())
//	{
//		sim_mob::Node* node = (*it).second;
//		return node;
//	}
//	return NULL;
//}

void sim_mob::PathSetParam::initParameters()
{
	bTTVOT = -0.01373;//-0.0108879;
	bCommonFactor = 1.0;
	bLength = -0.001025;//0.0; //negative sign proposed by milan
	bHighway = 0.00052;//0.0;
	bCost = 0.0;
	bSigInter = -0.13;//0.0;
	bLeftTurns = 0.0;
	bWork = 0.0;
	bLeisure = 0.0;
	highway_bias = 0.5;

	minTravelTimeParam = 0.879;
	minDistanceParam = 0.325;
	minSignalParam = 0.256;
	maxHighwayParam = 0.422;
}

//todo:obsolete
uint32_t sim_mob::PathSetParam::getSize()
{
	uint32_t sum = 0;
	sum += sizeof(double); //double bTTVOT;
	sum += sizeof(double); //double bCommonFactor;
	sum += sizeof(double); //double bLength;
	sum += sizeof(double); //double bHighway;
	sum += sizeof(double); //double bCost;
	sum += sizeof(double); //double bSigInter;
	sum += sizeof(double); //double bLeftTurns;
	sum += sizeof(double); //double bWork;
	sum += sizeof(double); //double bLeisure;
	sum += sizeof(double); //double highway_bias;
	sum += sizeof(double); //double minTravelTimeParam;
	sum += sizeof(double); //double minDistanceParam;
	sum += sizeof(double); //double minSignalParam;
	sum += sizeof(double); //double maxHighwayParam;

	//std::map<std::string,sim_mob::RoadSegment*> segPool;
	typedef std::map<std::string,sim_mob::RoadSegment*>::value_type SPP;
//	BOOST_FOREACH(SPP& segPool_pair,segPool)
//	{
//		sum += segPool_pair.first.length();
//	}
//	sum += sizeof(sim_mob::RoadSegment*) * segPool.size();

//		std::map<const sim_mob::RoadSegment*,sim_mob::WayPoint*> wpPool;//unused for now
	//std::map<std::string,sim_mob::Node*> nodePool;
	typedef std::map<std::string,sim_mob::Node*>::value_type NPP;
	logger << "nodePool.size() " << nodePool.size() << "\n";
	BOOST_FOREACH(NPP& nodePool_pair,nodePool)
	{
		sum += nodePool_pair.first.length();
	}
	sum += sizeof(sim_mob::Node*) * nodePool.size();

//		const std::vector<sim_mob::MultiNode*>  &multiNodesPool;
	sum += sizeof(sim_mob::MultiNode*) * multiNodesPool.size();

//		const std::set<sim_mob::UniNode*> & uniNodesPool;
	sum += sizeof(sim_mob::UniNode*) * uniNodesPool.size();

//		std::map<std::string,std::vector<sim_mob::ERP_Surcharge*> > ERP_SurchargePool;
	typedef std::map<std::string,std::vector<sim_mob::ERP_Surcharge*> >::value_type ERPSCP;
	BOOST_FOREACH(ERPSCP & ERP_Surcharge_pool_pair,ERP_SurchargePool)
	{
		sum += ERP_Surcharge_pool_pair.first.length();
		sum += sizeof(sim_mob::ERP_Surcharge*) * ERP_Surcharge_pool_pair.second.size();
	}

//		std::map<std::string,sim_mob::ERP_Gantry_Zone*> ERP_Gantry_ZonePool;
	typedef std::map<std::string,sim_mob::ERP_Gantry_Zone*>::value_type ERPGZP;
	BOOST_FOREACH(ERPGZP & ERP_Gantry_Zone_pool_pair,ERP_Gantry_ZonePool)
	{
		sum += ERP_Gantry_Zone_pool_pair.first.length();
	}
	sum += sizeof(sim_mob::ERP_Gantry_Zone*) * ERP_Gantry_ZonePool.size();

//		std::map<std::string,sim_mob::ERP_Section*> ERP_Section_pool;
	typedef std::map<int,sim_mob::ERP_Section*>::value_type  ERPSP;
	BOOST_FOREACH(ERPSP&ERP_Section_pair,ERP_SectionPool)
	{
		sum += sizeof(int);
	}
	sum += sizeof(sim_mob::ERP_Section*) * ERP_SectionPool.size();

//		std::map<std::string,std::vector<sim_mob::LinkTravelTime*> > segDefTT;
	typedef std::map<unsigned long,std::vector<sim_mob::LinkTravelTime> >::value_type LDTTPP;
	BOOST_FOREACH(LDTTPP & ldttpp,segDefTT)
	{
		sum += sizeof(unsigned long);
		sum += sizeof(sim_mob::LinkTravelTime) * ldttpp.second.size();
	}
	//todo historical avg travel time
//		const roadnetwork;
	sum += sizeof(sim_mob::RoadNetwork&);

//		std::string RTTT;
	sum += RTTT.length();
	return sum;
}

sim_mob::PathSetParam::PathSetParam() :
		roadNetwork(ConfigManager::GetInstance().FullConfig().getNetwork()),
		multiNodesPool(ConfigManager::GetInstance().FullConfig().getNetwork().getNodes()), uniNodesPool(ConfigManager::GetInstance().FullConfig().getNetwork().getUniNodes()),
		RTTT(""),intervalMS(sim_mob::ConfigManager::GetInstance().FullConfig().pathSet().interval* 1000 /*milliseconds*/)
{
	initParameters();
//	for (std::vector<sim_mob::Link *>::const_iterator it =	ConfigManager::GetInstance().FullConfig().getNetwork().getLinks().begin(), it_end( ConfigManager::GetInstance().FullConfig().getNetwork().getLinks().end()); it != it_end; it++) {
//		for (std::set<sim_mob::RoadSegment *>::iterator seg_it = (*it)->getUniqueSegments().begin(), it_end((*it)->getUniqueSegments().end()); seg_it != it_end; seg_it++) {
//			if (!(*seg_it)->originalDB_ID.getLogItem().empty()) {
//				string aimsun_id = (*seg_it)->originalDB_ID.getLogItem();
//				string segId = Utils::getNumberFromAimsunId(aimsun_id);
//				segPool.insert(std::make_pair(segId, *seg_it));
//			}
//		}
//	}
	//we are still in constructor , so const refs like roadNetwork and multiNodesPool are not ready yet.
	BOOST_FOREACH(sim_mob::Node* n, ConfigManager::GetInstance().FullConfig().getNetwork().getNodes()){
		if (!n->originalDB_ID.getLogItem().empty()) {
			std::string t = n->originalDB_ID.getLogItem();
			std::string id = sim_mob::Utils::getNumberFromAimsunId(t);
			nodePool.insert(std::make_pair(id , n));
		}
	}

	BOOST_FOREACH(sim_mob::UniNode* n, ConfigManager::GetInstance().FullConfig().getNetwork().getUniNodes()){
		if (!n->originalDB_ID.getLogItem().empty()) {
			std::string t = n->originalDB_ID.getLogItem();
			std::string id = sim_mob::Utils::getNumberFromAimsunId(t);
			nodePool.insert(std::make_pair(id, n));
		}
	}

	logger << "PathSetParam: nodes amount " <<
			ConfigManager::GetInstance().FullConfig().getNetwork().getNodes().size() +
			ConfigManager::GetInstance().FullConfig().getNetwork().getNodes().size() << "\n";
//	logger << "PathSetParam: segments amount "	<<
//			segPool.size() << "\n";
	getDataFromDB();
}

sim_mob::ERP_Section::ERP_Section(ERP_Section &src)
	: section_id(src.section_id),ERP_Gantry_No(src.ERP_Gantry_No)
{
	ERP_Gantry_No_str = boost::lexical_cast<std::string>(src.ERP_Gantry_No);
}

sim_mob::LinkTravelTime::LinkTravelTime(const LinkTravelTime& src)
	: linkId(src.linkId),
			startTime(src.startTime),endTime(src.endTime),travelTime(src.travelTime),interval(src.interval)
			,startTime_DT(sim_mob::DailyTime(src.startTime)),endTime_DT(sim_mob::DailyTime(src.endTime))
{
}
sim_mob::LinkTravelTime::LinkTravelTime()
	: linkId(0),
			startTime(""),endTime(""),travelTime(0.0),
			startTime_DT(0),endTime_DT(0)
{
}
