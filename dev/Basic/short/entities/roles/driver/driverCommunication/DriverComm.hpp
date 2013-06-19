#pragma once
#include "entities/roles/driver/Driver.hpp"
#include "entities/commsim/comm_support/JCommunicationSupport.hpp"
namespace sim_mob
{
class Broker;
class DriverCommMovement;
class DriverCommBehavior;

class DriverComm : public Driver, public JCommunicationSupport<std::string>
{
	static int totalSendCnt;
	static int totalReceiveCnt;
	int sendCnt,receiveCnt;
public:

	DriverComm(Person* parent, Broker* managingBroker, sim_mob::MutexStrategy mtxStrat, sim_mob::DriverCommBehavior* behavior = nullptr, sim_mob::DriverCommMovement* movement = nullptr);
	virtual ~DriverComm();

	virtual sim_mob::Role* clone(sim_mob::Person* parent) const;

	void receiveModule(timeslice now);
	void sendModule(timeslice now);
	sim_mob::Agent * getParentAgent();
	sim_mob::Broker& getBroker();
};

}//namspace sim_mob
