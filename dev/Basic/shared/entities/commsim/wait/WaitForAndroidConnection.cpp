//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * WaitForAndroidConnection.cpp
 *
 *  Created on: Jul 15, 2013
 *      Author: vahid
 */

#include "WaitForAndroidConnection.hpp"
#include "entities/commsim/Broker.hpp"
#include <boost/unordered_map.hpp>

namespace sim_mob {

WaitForAndroidConnection::WaitForAndroidConnection(sim_mob::Broker & broker_,int min_nof_clients_ ):
		BrokerBlocker(broker_),
		min_nof_clients(min_nof_clients_) {
	// TODO Auto-generated constructor stub

}

short WaitForAndroidConnection::get_MIN_NOF_Clients() {
	return min_nof_clients;
}

void WaitForAndroidConnection::set_MIN_NOF_Clients(int value) {
	min_nof_clients = value;
}

bool WaitForAndroidConnection::calculateWaitStatus() {
	ClientList::Type & clients = getBroker().getClientList();
	int cnt = clients[comm::ANDROID_EMULATOR].size();
//	Print() << "getBroker().getClientList().size() = " << cnt << " vs " << min_nof_clients << std::endl;
	if(cnt >= min_nof_clients)
	{

		setWaitStatus(false);
	}
	else{
		setWaitStatus(true);
	}
	return isWaiting();
}

WaitForAndroidConnection::~WaitForAndroidConnection() {
	// TODO Auto-generated destructor stub
}

} /* namespace sim_mob */