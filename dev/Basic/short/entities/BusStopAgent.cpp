//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "BusStopAgent.hpp"

#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"
#include "entities/AuraManager.hpp"
#include "entities/Person.hpp"
#include "entities/roles/waitBusActivityRole/WaitBusActivityRole.hpp"
#include "entities/roles/waitBusActivityRole/WaitBusActivityRoleFacets.hpp"
#include "geospatial/network/BusStop.hpp"
#include "workers/WorkGroup.hpp"

using std::vector;
using namespace sim_mob;

typedef Entity::UpdateStatus UpdateStatus;

BusStopAgent::BusStopAgentsMap BusStopAgent::allBusstopAgents;

size_t sim_mob::BusStopAgent::AllBusStopAgentsCount()
{
	return allBusstopAgents.size();
}

void sim_mob::BusStopAgent::AssignAllBusStopAgents(sim_mob::WorkGroup& wg)
{
	for (BusStopAgentsMap::iterator it=allBusstopAgents.begin();
			it!=allBusstopAgents.end(); ++it) {
		wg.assignAWorker(it->second);
	}
}

void sim_mob::BusStopAgent::RegisterNewBusStopAgent(BusStop& busstop, const MutexStrategy& mtxStrat)
{
	BusStopAgent* sig_ag = new BusStopAgent(busstop, mtxStrat);
	//sig_ag->setBusStopAgentNo(busstop.getRoadItemId());
	//allBusstopAgents[busstop.getRoadItemId()] = sig_ag;
}

bool sim_mob::BusStopAgent::HasBusStopAgents()
{
	return !allBusstopAgents.empty();
}

void sim_mob::BusStopAgent::createBusStopAgents(const std::set<BusStop*>& stopsList, const MutexStrategy& mtxStrat) {
	for(std::set<BusStop*>::const_iterator stopIt=stopsList.begin();
			stopIt!=stopsList.end(); ++stopIt) {
		RegisterNewBusStopAgent(**stopIt, mtxStrat);
	}
}

bool sim_mob::BusStopAgent::isNonspatial()
{
	return true;
}

void sim_mob::BusStopAgent::PlaceAllBusStopAgents(std::vector<sim_mob::Entity*>& agents_list)
{
	std::cout << "all_BusStopAgents size: " << allBusstopAgents.size() << std::endl;
	//Push every BusStopAgent on the list into the agents array as an active agent
	for (BusStopAgentsMap::iterator it=allBusstopAgents.begin();
			it!=allBusstopAgents.end(); ++it) {
		agents_list.push_back(it->second);
	}
}

BusStopAgent* sim_mob::BusStopAgent::findBusStopAgentByBusStop(const BusStop* busstop)
{
	try {
		return nullptr;// allBusstopAgents.at(busstop->getRoadItemId());
	}
	catch (const std::out_of_range& oor) {
		return nullptr;
	}
}

BusStopAgent* sim_mob::BusStopAgent::findBusStopAgentByBusStopNo(const std::string& busstopno)
{
	try {
		return allBusstopAgents.at(busstopno);
	}
	catch (const std::out_of_range& oor) {
		return nullptr;
	}
}

std::vector<BufferedBase*> sim_mob::BusStopAgent::buildSubscriptionList()
{
	return Agent::buildSubscriptionList();
}

bool sim_mob::BusStopAgent::frame_init(timeslice now)
{
	return true;
}

void sim_mob::BusStopAgent::frame_output(timeslice now)
{
	if(now.ms() % busStopUpdateFrequencyMS == 0) {// every 5000ms output
		LogOut("(\"BusStopAgent\""
			<<","<<now.frame()
			<<","<<getId()
			<<",{" <<"\"BusStopAgent no\":\""<<busstopAgentno_
			<<"\"})"<<std::endl);
	}
	const ConfigParams& config = ConfigManager::GetInstance().FullConfig();
	uint32_t currMS = (config.simStartTime() + DailyTime(now.ms())).offsetMS_From(DailyTime("00:00:00"));// transfered to ms based on midnight
	// record 3 hours' information
	if(now.frame() == 108000) {
		std::stringstream currReachedMSOut;
		std::map<std::string, std::vector<uint32_t> >::const_iterator it;
		buslineIdCurrReachedMSs.clear();
		for (it = buslineIdCurrReachedMSs.begin(); it != buslineIdCurrReachedMSs.end(); ++it) {
			// #print 857_1 information
			if((it->first) == "857_1") {
				currReachedMSOut << "currReachedMsInformation for buslineId " << (it->first) << std::endl;
				for(int i = 0; i < it->second.size(); i++) {
					currReachedMSOut << now.frame() << " "
									 << "at stop " << this->busstop_.getRoadItemId() << " "
									 << (it->first) << " "
									 << (it->second)[i] << std::endl;
				}
				currReachedMSOut << std::endl;
			}
		}

		buslineIdAlightingNum.clear();
		for (it = buslineIdAlightingNum.begin(); it != buslineIdAlightingNum.end(); ++it) {
			// #print 857_1 information
			if((it->first) == "857_1") {
				currReachedMSOut << "AlightingInformation for buslineId " << (it->first) << std::endl;
				for(int j = 0; j < it->second.size(); j++) {
					currReachedMSOut << now.frame() << " "
									 << "alightingNum " << this->busstop_.getRoadItemId() << " "
									 << (it->first) << " "
									 << (it->second)[j] << std::endl;
				}
				currReachedMSOut << std::endl;
			}
		}

		buslineIdBoardingNum.clear();
		for (it = buslineIdBoardingNum.begin(); it != buslineIdBoardingNum.end(); ++it) {
			// #print 857_1 information
			if((it->first) == "857_1") {
				currReachedMSOut << "BoardingInformation for buslineId " << (it->first) << std::endl;
				for(int k = 0; k < it->second.size(); k++) {
					currReachedMSOut << now.frame() << " "
									 << "boardingNum " << this->busstop_.getRoadItemId() << " "
									 << (it->first) << " "
									 << (it->second)[k] << std::endl;
				}
				currReachedMSOut << std::endl;
			}
		}

		std::map<std::string, std::vector<double> >::const_iterator it1;
		buslineIdBoardingAlightingSecs.clear();
		for (it1 = buslineIdBoardingAlightingSecs.begin(); it1 != buslineIdBoardingAlightingSecs.end(); ++it1) {
			// #print 857_1 information
			if((it1->first) == "857_1") {
				currReachedMSOut << "BoardingAlightingSecs_Information for buslineId " << (it1->first) << std::endl;
				for(int m = 0; m < it1->second.size(); m++) {
					currReachedMSOut << now.frame() << " "
									 << "boardingAlightingSecs " << this->busstop_.getRoadItemId() << " "
									 << (it1->first) << " "
									 << (it1->second)[m] << std::endl;
				}
				currReachedMSOut << std::endl;
			}
		}

		std::map<std::string, std::vector<int> >::const_iterator it2;
		buslineIdBusTripRunSequenceNums.clear();
		for (it2 = buslineIdBusTripRunSequenceNums.begin(); it2 != buslineIdBusTripRunSequenceNums.end(); ++it2) {
			// #print 857_1 information
			if((it2->first) == "857_1") {
				currReachedMSOut << "BusTripRunSequenceNumInformation for buslineId " << (it2->first) << std::endl;
				for(int n = 0; n < it2->second.size(); n++) {
					currReachedMSOut << now.frame() << " "
									 << "bustripRunSequenceNum " << this->busstop_.getRoadItemId() << " "
									 << (it2->first) << " "
									 << (it2->second)[n] << std::endl;
				}
				currReachedMSOut << std::endl;
			}
		}

		buslineIdPassengerCounts.clear();
		for (it2 = buslineIdPassengerCounts.begin(); it2 != buslineIdPassengerCounts.end(); ++it2) {
			// #print 857_1 information
			if((it2->first) == "857_1") {
				currReachedMSOut << "PassengerCountsInformation for buslineId " << (it2->first) << std::endl;
				for(int s = 0; s < it2->second.size(); s++) {
					currReachedMSOut << now.frame() << " "
									 << "passengerCounts " << this->busstop_.getRoadItemId() << " "
									 << (it2->first) << " "
									 << (it2->second)[s] << std::endl;
				}
				currReachedMSOut << std::endl;
				HeadwayAtBusStopInfoPrint() << currReachedMSOut.str();
			}
		}

// #print all busline information
//		for (it = buslineId_CurrReachedMSs.begin(); it != buslineId_CurrReachedMSs.end(); ++it) {
//			currReachedMSOut << "(\"BusStopAgent\""
//							<<" frame no:" << now.frame()
//							<< " bus stop no:" << this->busstop_.busstopno_ << std::endl;
//			currReachedMSOut << " (\"buslineID\""
//							<< ": " << (it->first)
//							<< " currReachedMS size: " << (it->second).size()
//							<< std::endl;
//			for(int i = 0; i < it->second.size(); i++) {
////				currReachedMSOut << " (\"currReachedMS\""
////								<< ": " << (it->second)[i];
////				currReachedMSOut << "\"})" << std::endl;
//				currReachedMSOut << (it->second)[i];
//				currReachedMSOut << std::endl;
//			}
//			currReachedMSOut << std::endl;
//		}
//		HeadwayAtBusStopInfoPrint() << currReachedMSOut.str();
	}

}

Entity::UpdateStatus sim_mob::BusStopAgent::frame_tick(timeslice now)
{
	if(now.ms() % busStopUpdateFrequencyMS == 0) {// every 5000ms check AuraManager
		vector<const Agent*> nearby_agents = 
		AuraManager::instance().agentsInRect(Point((busstop_.getStopLocation().getX() - 3500), 
												 (busstop_.getStopLocation().getY() - 3500)), 
											 Point((busstop_.getStopLocation().getX() + 3500),(busstop_.getStopLocation().getY() + 3500)), this);
		
		for (vector<const Agent*>::iterator it = nearby_agents.begin();it != nearby_agents.end(); ++it)
		{
			//Retrieve only agents with WaitBusActivityRoles.
		 	const Person* person = dynamic_cast<const Person *>(*it);
		 	Person* p = const_cast<Person *>(person);
		 	WaitBusActivityRole* waitbusactivityRole = p ? dynamic_cast<WaitBusActivityRole*>(p->getRole()) : nullptr;
		 	if(waitbusactivityRole) {
		 		WaitBusActivityRoleMovement* waitbusactivityRoleMovement = dynamic_cast<WaitBusActivityRoleMovement*> (waitbusactivityRole->Movement());
		 		if((!waitbusactivityRoleMovement->getRegisteredFlag()) && (waitbusactivityRoleMovement->getBusStopAgent() == this)) {// not registered and waiting in this BusStopAgent
		 			boardingWaitBusActivities.push_back(waitbusactivityRole);
		 			waitbusactivityRoleMovement->setRegisteredFlag(true);// set this person's role to be registered
		 		}
		 	}
		}
		if(!boardingWaitBusActivities.empty())
			sort(boardingWaitBusActivities.begin(),boardingWaitBusActivities.end(),less_than_TimeOfReachingBusStop());
	}
	for(int i = 0; i < boardingWaitBusActivities.size(); i++) {
		WaitBusActivityRoleMovement* waitbusactivityRoleMovement = dynamic_cast<WaitBusActivityRoleMovement*> (boardingWaitBusActivities[i]->Movement());
		if(waitbusactivityRoleMovement->isBoarded) {
			boardingWaitBusActivities.erase(boardingWaitBusActivities.begin() + i);
		}
	}

	return Entity::UpdateStatus::Continue;
}
