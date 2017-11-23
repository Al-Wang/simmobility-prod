#ifndef WAITTAXIACTIVITY_HPP_
#define WAITTAXIACTIVITY_HPP_

#include "entities/roles/Role.hpp"
#include "entities/Person_ST.hpp"
namespace sim_mob
{
class TaxiStand;
class WaitTaxiActivityBehavior;
class WaitTaxiActivityMovement;
class WaitTaxiActivity: public sim_mob::Role<Person_ST>, public UpdateWrapper<UpdateParams>
{
public:
	explicit WaitTaxiActivity(Person_ST* parent, WaitTaxiActivityBehavior* behavior = nullptr,
			WaitTaxiActivityMovement* movement = nullptr, const TaxiStand* stand=nullptr, std::string roleName = std::string("WaitTaxiActivity_"),
			Role<Person_ST>::Type roleType = Role<Person_ST>::RL_WAITTAXIACTIVITY);

	virtual ~WaitTaxiActivity();

	/**
	 * override the virtual function inherited from base class Role
	 */
	virtual sim_mob::Role<Person_ST>* clone(Person_ST *parent) const;
	/**
	 * override the virtual function inherited from base class Role
	 */
	virtual void make_frame_tick_params(timeslice now);
	/**
	 * override the virtual function inherited from base class Role
	 */
	virtual std::vector<sim_mob::BufferedBase*> getSubscriptionParams();
	/**
	 * collect total travel time for current role, virtual function is inherited from base class
	 */
	virtual void collectTravelTime();
	/**
	 * increase waiting time every frame tick.
	 * @param timeMs is an accumulated time in milliseconds
	 */
	void increaseWaitingTime(unsigned int timeMs);
	/**
	 * message handler to handle message transfered from parent agent.
	 * @param type of the message.
	 * @param message is data associated to the message.
	 */
	virtual void HandleParentMessage(messaging::Message::MessageType type, const messaging::Message& message);
	/**
	 * get waiting time at current stand
	 * @return waiting time
	 */
	unsigned int getWaitingTime() const;
	/**
	 * get referred taxi-stand
	 * @return taxi-stand
	 */
	const TaxiStand* getTaxiStand() const;
private:
	friend class WaitTaxiActivityBehavior;
	friend class WaitTaxiActivityMovement;

	/**record waiting time (in milliseconds) in the taxi-stand*/
	unsigned int waitingTime;
	/**pointer to the taxi-stand*/
	const TaxiStand* stand;
};
}
#endif /* WAITTAXIACTIVITY_HPP_ */
