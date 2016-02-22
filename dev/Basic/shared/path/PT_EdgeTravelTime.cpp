/*
 * PT_EdgeTravelTime.cpp
 *
 *  Created on: Jan 28, 2016
 *      Author: zhang huai peng
 */

#include "PT_EdgeTravelTime.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "util/Profiler.hpp"
#include "logging/Log.hpp"

namespace
{
	/**time interval value used for processing data.*/
	const unsigned int INTERVAL_MS = 5*60*1000;
	/** millisecs conversion unit from seconds*/
	const double MS_IN_SECONDS = 1000.0;
}

namespace sim_mob
{
PT_EdgeTravelTime* PT_EdgeTravelTime::instance=nullptr;

PT_EdgeTravelTime::PT_EdgeTravelTime() {
	// TODO Auto-generated constructor stub

}

PT_EdgeTravelTime::~PT_EdgeTravelTime() {
	if(instance){
		delete instance;
	}
}


PT_EdgeTravelTime* PT_EdgeTravelTime::getInstance()
{
	if (!instance)
	{
		instance = new PT_EdgeTravelTime();
	}
	return instance;
}

void PT_EdgeTravelTime::updateEdgeTravelTime(const unsigned int edgeId, const unsigned int startTime, const unsigned int endTime, const std::string& travelMode)
{
	const sim_mob::ConfigParams& cfg = sim_mob::ConfigManager::GetInstance().FullConfig();
	if (!cfg.isEnabledEdgeTravelTime() || edgeId == 0)
	{
		return;
	}

	boost::unique_lock<boost::mutex> lock(instanceMutex);

	EdgeTimeSlotMap& edgeTime = storeEdgeTimes[edgeId];
	unsigned int index = startTime / INTERVAL_MS;
	EdgeTimeSlotMap::iterator itSlot = edgeTime.find(index);
	if (itSlot == edgeTime.end())
	{
		EdgeTimeSlot slot;
		slot.edgeId = edgeId;
		slot.timeInterval = index;
		edgeTime[index] = slot;
	}

	EdgeTimeSlot& slot = edgeTime[index];
	double timeInSecs = ((double)(endTime - startTime)) / MS_IN_SECONDS;
	if (travelMode == "WaitingBusActivity")
	{
		slot.waitTime +=
		slot.countforWaitTime++;
	}
	else
	{
		slot.countforLinkTime++;
		slot.linkTravelTime += timeInSecs;
		if (travelMode == "Walk")
		{
			slot.walkTime += timeInSecs;
		}
		else
		{
			slot.dayTransitTime += timeInSecs;
		}
	}
}


void PT_EdgeTravelTime::exportEdgeTravelTime() const
{
	const sim_mob::ConfigParams& cfg = sim_mob::ConfigManager::GetInstance().FullConfig();
	if(!cfg.isEnabledEdgeTravelTime()){
		return;
	}
    const std::string& fileName("pt_edge_time.csv");
    sim_mob::BasicLogger& ptEdgeTimeLogger  = sim_mob::Logger::log(fileName);
    std::map<int, EdgeTimeSlotMap>::const_iterator it;
    for( it = storeEdgeTimes.begin(); it!=storeEdgeTimes.end(); it++){
    	const EdgeTimeSlotMap& edgeTime=it->second;
    	for(EdgeTimeSlotMap::const_iterator itSlot = edgeTime.begin(); itSlot != edgeTime.end(); itSlot++){
    		const EdgeTimeSlot& slot = itSlot->second;
    		ptEdgeTimeLogger << slot.edgeId << ",";
    		DailyTime startTime = ConfigManager::GetInstance().FullConfig().simStartTime();
    		unsigned int start = (startTime.getValue()/INTERVAL_MS)*INTERVAL_MS;
    		ptEdgeTimeLogger << DailyTime(start+slot.timeInterval*INTERVAL_MS).getStrRepr()<<",";
    		ptEdgeTimeLogger << DailyTime(start+(slot.timeInterval+1)*INTERVAL_MS-MS_IN_SECONDS).getStrRepr() <<",";
    		ptEdgeTimeLogger << (slot.waitTime>0.0?slot.waitTime/slot.countforWaitTime:0) << ",";
    		ptEdgeTimeLogger << (slot.walkTime>0.0?slot.walkTime/slot.countforLinkTime:0) << ",";
    		ptEdgeTimeLogger << (slot.dayTransitTime>0.0?slot.dayTransitTime/slot.countforLinkTime:0) <<",";
    		ptEdgeTimeLogger << (slot.linkTravelTime>0.0?slot.linkTravelTime/slot.countforLinkTime:0) << std::endl;
    	}
    }
    sim_mob::Logger::log(fileName).flush();
}

void PT_EdgeTravelTime::loadPT_EdgeTravelTime()
{
    const ConfigParams& config = ConfigManager::GetInstance().FullConfig();
    const std::map<std::string, std::string>& storedProcMap = config.getDatabaseProcMappings().procedureMappings;
    std::map<std::string, std::string>::const_iterator storedProcIter = storedProcMap.find("pt_edges_time");
    if(storedProcIter == storedProcMap.end())
    {
        Print()<<"PT_EdgeTravelTime: Stored Procedure not specified"<<std::endl;
        return;
    }

    soci::session sql_(soci::postgresql, config.getDatabaseConnectionString(false));
    soci::rowset<soci::row> rs = (sql_.prepare << "select * from " + storedProcIter->second);
	for (soci::rowset<soci::row>::const_iterator it=rs.begin(); it!=rs.end(); ++it)
	{
		const soci::row& r = (*it);
		int edgeId = r.get<int>(0);
		std::string startTime = r.get<std::string>(1);
		std::string endTime = r.get<std::string>(2);
		double waitTime = r.get<double>(3);
		double walkTime = r.get<double>(4);
		double dayTransitTime = r.get<double>(5);
		double linkTravelTime = r.get<double>(6);
		loadOneEdgeTravelTime(edgeId, startTime, endTime, waitTime, walkTime, dayTransitTime, linkTravelTime);
	}
}

void PT_EdgeTravelTime::loadOneEdgeTravelTime(const unsigned int edgeId,
		const std::string& startTime, const std::string endTime,
		const double waitTime, const double walkTime,
		const double dayTransitTime, const double linkTravelTime)
{
	std::map<int, EdgeTimeSlotMap>::iterator it = loadEdgeTimes.find(edgeId);
	if(it==loadEdgeTimes.end()){
		EdgeTimeSlotMap edgeTime;
		loadEdgeTimes[edgeId]= edgeTime;
	}

	EdgeTimeSlotMap& edgeTime = loadEdgeTimes[edgeId];
	DailyTime start = DailyTime(startTime);
	unsigned int index = start.getValue()/INTERVAL_MS;
	EdgeTimeSlotMap::iterator itSlot = edgeTime.find(index);
	if(itSlot==edgeTime.end()){
		EdgeTimeSlot slot;
		edgeTime[index]=slot;
		edgeTime[index].edgeId = edgeId;
		edgeTime[index].timeInterval = index;
	}

	EdgeTimeSlot& slot = edgeTime[index];
	edgeTime[index].countforLinkTime = 1.0;
	edgeTime[index].linkTravelTime = linkTravelTime;
	edgeTime[index].waitTime = waitTime;
	edgeTime[index].walkTime = walkTime;
	edgeTime[index].dayTransitTime = dayTransitTime;
}

bool PT_EdgeTravelTime::getEdgeTravelTime(int edgeId,
		unsigned int currentTime,
		double& waitTime,
		double& walkTime,
		double& dayTransitTime,
		double& linkTravelTime) const
{
	bool res = false;
	std::map<int, EdgeTimeSlotMap>::const_iterator it = loadEdgeTimes.find(edgeId);
	if (it == loadEdgeTimes.end())
	{
		return res;
	}

	const EdgeTimeSlotMap& edgeTime = it->second;
	DailyTime start = DailyTime(currentTime);
	unsigned int index = start.getValue() / INTERVAL_MS;
	EdgeTimeSlotMap::const_iterator itSlot = edgeTime.find(index);
	if (itSlot == edgeTime.end())
	{
		return res;
	}

	const EdgeTimeSlot& slot = itSlot->second;
	waitTime = slot.waitTime;
	walkTime = slot.walkTime;
	dayTransitTime = slot.dayTransitTime;
	linkTravelTime = slot.linkTravelTime;
	res = true;
	return res;
}
}

