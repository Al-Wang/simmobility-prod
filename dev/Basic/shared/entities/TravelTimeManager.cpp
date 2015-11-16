#include "TravelTimeManager.hpp"
#include "path/PathSetManager.hpp"
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "path/PathSetManager.hpp"
#include "util/LangHelpers.hpp"

sim_mob::TravelTimeManager* sim_mob::TravelTimeManager::instance = nullptr;

sim_mob::TravelTimeManager::TravelTimeManager()
	: intervalMS(sim_mob::ConfigManager::GetInstance().FullConfig().getPathSetConf().interval * 1000), //conversion from seconds to milliseconds
	  enRouteTT(new sim_mob::TravelTimeManager::EnRouteTT(*this))
{}

sim_mob::TravelTimeManager::~TravelTimeManager()
{
	safe_delete_item(enRouteTT);
}

void sim_mob::TravelTimeManager::addTravelTime(const RdSegTravelStat & stats) {
	TT::TimeInterval timeInterval = TravelTimeManager::getTimeInterval(stats.entryTime * 1000, intervalMS);//milliseconds
	{
		boost::upgrade_lock<boost::shared_mutex> lock(ttMapMutex);
		boost::upgrade_to_unique_lock<boost::shared_mutex> uniquelock(lock);
		TT::TimeAndCount &tc = ttMap[timeInterval][stats.travelMode][stats.rs];
		tc.totalTravelTime += stats.travelTime; //add to total travel time
		tc.travelTimeCnt += 1; //increment the total contribution
	}
}

sim_mob::TT::TimeInterval sim_mob::TravelTimeManager::getTimeInterval(const unsigned long time, const unsigned int interval)
{
	return time / interval ;/*milliseconds*/
}

double sim_mob::TravelTimeManager::getInSimulationLinkTT(const std::string mode, const sim_mob::Link *lnk) const
{
	return enRouteTT->getInSimulationLinkTT(mode,lnk);
}

double sim_mob::TravelTimeManager::EnRouteTT::getInSimulationLinkTT(const std::string mode, const sim_mob::Link *lnk) const
{
	boost::shared_lock<boost::shared_mutex> lock(parent.ttMapMutex);
	//[time interval][travel mode][road segment][average travel time]
	//<-----TI-----><-------------------MRTC----------------------->
	//start from the last recorded time interval (before the current time interval) and proceed to find a travel time for the given section.
	//if no records found, check the previous time interval and so on.
	sim_mob::TravelTime::reverse_iterator itTI = parent.ttMap.rbegin();
	//like I said, not the current interval
	if(itTI != parent.ttMap.rend() && itTI->first == sim_mob::PathSetManager::curIntervalMS) { itTI++; }
	sim_mob::TT::MRTC::iterator itMode;
	sim_mob::TT::RSTC::iterator itSeg;
	//search backwards. try to find a matching road segment in any of the previous time intervals
	while(itTI != parent.ttMap.rend())
	{
		itMode = itTI->second.find(mode);
		if(itMode != itTI->second.end())
		{
			itSeg = itMode->second.find(lnk);
			if(itSeg != itMode->second.end())
			{
				return itSeg->second.totalTravelTime / itSeg->second.travelTimeCnt;
			}
		}
		++itTI;
	}
	return 0.0;
}

void sim_mob::TravelTimeManager::dumpTravelTimesToFile(const std::string fileName)
{
	//	easy reading down the line
	typedef std::map<const sim_mob::RoadSegment*,sim_mob::TT::TimeAndCount >::value_type STC;//SegmentTimeCount
	typedef TT::MRTC::value_type TravelModes;
	typedef sim_mob::TravelTime::value_type TRPs;
	//	destination file
	sim_mob::BasicLogger & TTLogger  = sim_mob::Logger::log(fileName);
	// config interval(in seconds)
	int intervalSec = sim_mob::ConfigManager::GetInstance().FullConfig().getPathSetConf().interval;
	//time range
	BOOST_FOREACH(TRPs &TT_Pair, ttMap)
	{
		const TT::TimeInterval & timeInterval = TT_Pair.first;
		TT::MRTC & travelModes = TT_Pair.second;
		//travel mode
		BOOST_FOREACH(TravelModes &travelMode, travelModes)
		{
			const std::string & travelModeStr = travelMode.first;
			std::map<const sim_mob::RoadSegment*,TT::TimeAndCount > & travelTimes = travelMode.second;
			//road segment
			BOOST_FOREACH(STC &RS_Pair, travelTimes)
			{

				//easy reading
				const unsigned long &segmentId = RS_Pair.first->getRoadSegmentId();
				TT::TimeAndCount & timeAndCount = RS_Pair.second;
				double& totalTT = timeAndCount.totalTravelTime;
				unsigned int& totalCount = timeAndCount.travelTimeCnt;
				// calculate the average travel time
				double avgTravelTime = 0.0;
				if(totalCount) { avgTravelTime = totalTT / totalCount; }

				//start and end time:
				const DailyTime &simStartTime = sim_mob::ConfigManager::GetInstance().FullConfig().simStartTime();
				DailyTime startTime(simStartTime.getValue() +  (timeInterval* intervalMS) );
				DailyTime endTime(simStartTime.getValue() + ((timeInterval + 1) * intervalMS - 1000) );
				//now simply write it to the file
				TTLogger << segmentId << ";" << startTime.getStrRepr() << ";" << endTime.getStrRepr() << ";" << avgTravelTime << ";"  << intervalSec << ";"  << travelModeStr <<  "\n";
			}
		}
	}
}

bool sim_mob::TravelTimeManager::storeRTT2DB()
{
	typedef std::map<boost::thread::id, boost::shared_ptr<TravelTimeManager> >::value_type TTs;
	std::string tempFileName = sim_mob::PathSetParam::getInstance()->RTTT;
	dumpTravelTimesToFile(tempFileName);
	sim_mob::Logger::log(tempFileName).flush();
	tempFileName += ".txt";
	/*The below fucntion only works when SimMobility is executed on the same machine as the location of the db
	 i.e. SimMobility on user machine + local db 
	 OR	  SimMobility on remote machine + remote db
	sim_mob::aimsun::Loader::upsertTravelTime(*sim_mob::PathSetManager::getInstance()->getSession(), boost::filesystem::canonical(tempFileName).string(), sim_mob::PathSetParam::getInstance()->RTTT, sim_mob::ConfigManager::GetInstance().PathSetConfig().alpha);
	*/
	return true;
}

sim_mob::TravelTimeManager* sim_mob::TravelTimeManager::getInstance()
{
	if(!instance)
	{
		instance = new TravelTimeManager();
	}
	return instance;
}
