#pragma once

#include <boost/ptr_container/ptr_vector.hpp>
#include <stdexcept>

#include "Message.hpp"

#include "entities/Person.hpp"
#include "geospatial/network/SMSVehicleParking.hpp"


namespace sim_mob
{

// Set to false if the barycenter is not needed. If false, the compiler will avoid all the barycenter update
// overhead
const bool doWeComputeBarycenter = true;

enum MobilityServiceControllerMessage
{
	MSG_DRIVER_SUBSCRIBE = 7100000,
	MSG_DRIVER_UNSUBSCRIBE,
	MSG_UNSUBSCRIBE_SUCCESSFUL,
	MSG_DRIVER_AVAILABLE,
	MSG_DRIVER_SHIFT_END,
	MSG_DRIVER_SCHEDULE_STATUS,
	MSG_TRIP_REQUEST,
	MSG_SCHEDULE_PROPOSITION,
	MSG_SCHEDULE_PROPOSITION_REPLY,
	MSG_SCHEDULE_UPDATE
};

/**Enumeration to indicate the type of trip requested by the passenger*/
enum class RequestType
{
	/**Specifies that the trip must be exclusive to the passenger - single rider request*/
	TRIP_REQUEST_SINGLE,

	/**Specifies that the trip may be shared with another passenger - shared trip request*/
	TRIP_REQUEST_SHARED,

	/**Does not specify any preference - the controller will assign arbitrarily*/
	TRIP_REQUEST_DEFAULT
};

/**
 * Message to subscribe a driver
 */
class DriverSubscribeMessage : public messaging::Message
{
public:
	DriverSubscribeMessage(Person *p) : person(p)
	{
	}

	virtual ~DriverSubscribeMessage()
	{
	}

	Person *person;
};

/**
 * Message to unsubscribe a driver
 */
class DriverUnsubscribeMessage : public messaging::Message
{
public:
	DriverUnsubscribeMessage(Person *p) : person(p)
	{
	}

	virtual ~DriverUnsubscribeMessage()
	{
	}

	Person *person;
};

/**
 * Message to state that a driver is available
 */
class DriverAvailableMessage : public messaging::Message
{
public:
	DriverAvailableMessage(Person *p) : person(p)
	{
		priority = 8;
	}

	virtual ~DriverAvailableMessage()
	{
	}

	Person *person;
};

/**
 * Message indicating that a driver has completed its shift
 */
class DriverShiftCompleted : public messaging::Message
{
public:
	DriverShiftCompleted(Person *p) : person(p)
	{
	}

	virtual ~DriverShiftCompleted()
	{
	}

	Person *person;
};

/**
 * Message to indicate that the driver has completed one item in the schedule
 */
class DriverScheduleStatusMsg : public messaging::Message
{
public:
	DriverScheduleStatusMsg(Person *p) : person(p)
	{
		priority = 10;
	}

	virtual ~DriverScheduleStatusMsg()
	{
	}

	Person *person;
};

/**
 * Message to request a trip
 */
class TripRequestMessage : public messaging::Message
{
public:
	TripRequestMessage() : timeOfRequest(timeslice(0, 0)), userId("no-id"), startNode(0),
	                       destinationNode(0), extraTripTimeThreshold(0), requestType(RequestType::TRIP_REQUEST_DEFAULT)
	{};

	TripRequestMessage(const TripRequestMessage &r) :
			timeOfRequest(r.timeOfRequest), userId(r.userId), startNode(r.startNode),
			destinationNode(r.destinationNode), extraTripTimeThreshold(r.extraTripTimeThreshold),
			requestType(r.requestType)
	{
	};


	TripRequestMessage(const timeslice &ct, const std::string &p, const Node *sn, const Node *dn,
	                   const unsigned int &threshold, const RequestType reqType = RequestType::TRIP_REQUEST_DEFAULT) :
			timeOfRequest(ct), userId(p), startNode(sn), destinationNode(dn), extraTripTimeThreshold(threshold),
			requestType(reqType)
	{
	};


	~TripRequestMessage()
	{
	}

	bool operator==(const TripRequestMessage &other) const;

	bool operator!=(const TripRequestMessage &other) const;

	bool operator<(const TripRequestMessage &other) const;

	bool operator>(const TripRequestMessage &other) const;

	timeslice timeOfRequest;
	std::string userId;
	const Node *startNode;
	const Node *destinationNode;
	RequestType requestType;

	/**
	 * The time the passenger can tolerate to spend more w.r.t. the fastest option in which
	 * she travels alone without any other passenger to share the trip with
	 */
	unsigned int extraTripTimeThreshold; // seconds
};

enum ScheduleItemType
{
	INVALID, PICKUP, DROPOFF, CRUISE, PARK
};

struct ScheduleItem
{
	ScheduleItem(const ScheduleItemType scheduleItemType_, const TripRequestMessage tripRequest_)
			: scheduleItemType(scheduleItemType_), tripRequest(tripRequest_), nodeToCruiseTo(NULL), parking(nullptr)
	{
#ifndef NDEBUG
		if (scheduleItemType != ScheduleItemType::PICKUP && scheduleItemType != ScheduleItemType::DROPOFF)
		{
			throw std::runtime_error("Only PICKUP or DROPOFF is admitted here");
		}
#endif

	};

	ScheduleItem(const ScheduleItemType scheduleItemType_, const Node *nodeToCruiseTo_)
			: scheduleItemType(scheduleItemType_), nodeToCruiseTo(nodeToCruiseTo_), tripRequest(), parking(nullptr)
	{
#ifndef NDEBUG
		if (scheduleItemType != ScheduleItemType::CRUISE)
		{
			throw std::runtime_error("Only CRUISE is admitted here");
		}
#endif
	};

	ScheduleItem(const ScheduleItemType scheduleItemType_, const SMSVehicleParking *parkingLocation)
			: scheduleItemType(scheduleItemType_), nodeToCruiseTo(NULL), tripRequest(), parking(parkingLocation)
	{
#ifndef NDEBUG
		if (scheduleItemType != ScheduleItemType::PARK)
		{
			throw std::runtime_error("Only PARK is admitted here");
		}
#endif
	};

	bool operator<(const ScheduleItem &other) const;

	bool operator==(const ScheduleItem &rhs) const;

	ScheduleItemType scheduleItemType;

	TripRequestMessage tripRequest;

	const Node *nodeToCruiseTo;

	const SMSVehicleParking *parking;
};

class Schedule
{
private:
	//TODO: It would be more elegant using std::variant, available from c++17
	//aa!!: Given that we are inserting many elements now, it may be better to use a list instead of a vector
	//			(see http://john-ahlgren.blogspot.com/2013/10/stl-container-performance.html)
	std::vector<ScheduleItem> items;

	/**
	 * The barycenter of all the dropOff locations.
	 * Only needed for the ProximityBased controller
	 */
	Point dropOffBarycenter;

	short passengerCount;

public:
	Schedule() : passengerCount(0), dropOffBarycenter(Point())
	{};

	//{ EMULATE STANDARD CONTAINER FUNCTIONS
	typedef std::vector<ScheduleItem>::const_iterator const_iterator;
	typedef std::vector<ScheduleItem>::iterator iterator;

	const std::vector<ScheduleItem> &getItems() const;

	const size_t size() const;

	const bool empty() const;

	Schedule::iterator begin();

	Schedule::const_iterator begin() const;

	const_iterator end() const;

	iterator end();

	void insert(iterator position, const ScheduleItem scheduleItem);

	void insert(iterator position, iterator first, iterator last);

	const ScheduleItem &back() const;

	ScheduleItem &front();

	const ScheduleItem &front() const;

	void pop_back();

	void push_back(ScheduleItem item);

	iterator erase(iterator position);

	ScheduleItem &at(size_t n);

	const ScheduleItem &at(size_t n) const;
	//} EMULATE STANDARD CONTAINER FUNCTIONS

	/**
	 * Performs the appropriate internal data update when a pick up is added
	 */
	void onAddingScheduleItem(const ScheduleItem &item);

	void onRemovingScheduleItem(const ScheduleItem &item);

	const Point &getDropOffBarycenter() const;

	short getPassengerCount() const;
};

class SchedulePropositionMessage : public messaging::Message
{
public:
	SchedulePropositionMessage(const timeslice currTick_, Schedule schedule_, messaging::MessageHandler *msgSender) :
			currTick(currTick_), schedule(schedule_)
	{
		sender = msgSender;
	};

	const Schedule &getSchedule() const;

	const timeslice currTick;

private:
	Schedule schedule;
};

/**
 * Message to respond to a trip proposition
 */
class SchedulePropositionReplyMessage : public messaging::Message
{
public:
	SchedulePropositionReplyMessage(timeslice ct, const std::string &p,
	                                Person *t, const Node *sn, const Node *dn,
	                                const unsigned int threshold, const bool s) : currTick(ct),
	                                                                              personId(p), driver(t),
	                                                                              startNode(sn),
	                                                                              destinationNode(dn),
	                                                                              extraTripTimeThreshold(threshold),
	                                                                              success(s)
	{
	}

	virtual ~SchedulePropositionReplyMessage()
	{
	}

	const timeslice currTick;
	const bool success;
	const std::string personId;
	Person *driver;
	const Node *startNode;
	const Node *destinationNode;
	const unsigned int extraTripTimeThreshold;
};

class ScheduleException : public std::runtime_error
{
public:
	ScheduleException(const std::string &xmsg) : std::runtime_error(xmsg)
	{};

	virtual ~ScheduleException()
	{};
};

}


std::ostream &operator<<(std::ostream &strm, const sim_mob::TripRequestMessage &request);

std::ostream &operator<<(std::ostream &strm, const sim_mob::ScheduleItem &item);

std::ostream &operator<<(std::ostream &strm, const sim_mob::Schedule &schedule);
