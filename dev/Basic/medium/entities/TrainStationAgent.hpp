/*
 * TrainStationAgent.hpp
 *
 *  Created on: Feb 24, 2016
 *      Author: zhang huai peng
 */

#ifndef TRAINSTATIONAGENT_HPP_
#define TRAINSTATIONAGENT_HPP_
#include "entities/Agent.hpp"
#include "entities/roles/passenger/Passenger.hpp"
#include "entities/roles/waitTrainActivity/WaitTrainActivity.hpp"
#include "geospatial/network/PT_Stop.hpp"
#include "entities/incident/IncidentManager.hpp"
namespace sim_mob {
namespace medium
{

struct ForceReleaseEntity
{
	int trainId;
	std::string lineId;
};
class TrainDriver;
class Conflux;
class TrainStationAgent : public sim_mob::Agent {
public:
	TrainStationAgent();
	virtual ~TrainStationAgent();
	void setStation(const Station* station);
	void setConflux(Conflux* conflux);
	void setStationName(const std::string& name);
	void setLines();
	void setLastDriver(std::string lineId,TrainDriver *driver);
	void addTrainDriverInToStationAgent(TrainDriver * driver);
	std::list<TrainDriver*>& getTrains();
	std::map<std::string, TrainDriver*> getLastDriver();

protected:
	virtual void HandleMessage(messaging::Message::MessageType type, const messaging::Message& message);
	//Virtual overrides
	virtual Entity::UpdateStatus frame_init(timeslice now);
	virtual Entity::UpdateStatus frame_tick(timeslice now);
	virtual void frame_output(timeslice now);
	virtual bool isNonspatial();
	/**
	 * calls frame_tick of the movement facet for the person's role
	 * @param now current time slice
	 * @param person person to tick
	 * @return update status
	 */
	Entity::UpdateStatus callMovementFrameTick(timeslice now, TrainDriver* person);
	/**
	 * Inherited from EventListener.
	 * @param eventId
	 * @param ctxId
	 * @param sender
	 * @param args
	 */
	virtual void onEvent(event::EventId eventId, sim_mob::event::Context ctxId, event::EventPublisher* sender, const event::EventArgs& args);

private:
	/**
	 * dispatch pending train
	 * @param now current time slice
	 */
	void dispathPendingTrains(timeslice now);
	/**
	 * remove ahead train
	 * @param aheadTrain is pointer to the ahead train
	 */
	void removeAheadTrain(TrainDriver* aheadDriver);
	/**
	 * alighting passenger leave platform
	 * @param now current time slice
	 */
	void passengerLeaving(timeslice now);
	/**
	 * update wait persons
	 */
	void updateWaitPersons();
	/**
	 * trigger reroute event
	 * @param args is event parameter
	 * @param now current time slice
	 */
	void triggerRerouting(const event::EventArgs& args, timeslice now);
	/**
	 * perform disruption processing
	 * @param now is current time
	 */
	void performDisruption(timeslice now);

	void checkAndInsertUnscheduledTrains();

	void prepareForUturn(TrainDriver *driver);

	void pushForceAlightedPassengersToWaitingQueue(const Platform *platform);


private:
	/**the reference to the station*/
	const Station* station;
	/**the reference */
	std::list<TrainDriver*> trainDriver;
	/**record last train in each line*/
	std::map<std::string, TrainDriver*> lastTrainDriver;
	/**record pending trains in each line*/
	std::map<std::string, std::list<TrainDriver*>> pendingTrainDriver;
	/**record pending unscheduled trains in each line*/
	std::map<std::string, std::list<TrainDriver*>> pendingUnscheduledTrainDriver;
	/**record usage in each line*/
	std::map<std::string, bool> lastUsage;
	/**waiting person for boarding*/
	std::map<const Platform*, std::list<WaitTrainActivity*>> waitingPersons;
	std::map<const Platform*, std::list<Passenger*>> forceAlightedPersons;
	std::list<Person_MT*> personsForcedAlighted;
	std::vector<std::string> unscheduledTrainLines;
	/**alighting person for next trip*/
	std::map<const Platform*, std::list<Passenger*>> leavingPersons;
	/** parent conflux */
	Conflux* parentConflux;
	/**recording disruption information*/
	boost::shared_ptr<DisruptionParams> disruptionParam;

	std::vector<ForceReleaseEntity> forceReleaseEntities;

	/**station name*/
	std::string stationName;
	std::map<std::string,bool> IsStartStation;
	bool arePassengersreRouted = false;
	static boost::mutex insertTrainOrUturnlock;
};
}
} /* namespace sim_mob */

#endif /* TRAINSTATIONAGENT_HPP_ */
