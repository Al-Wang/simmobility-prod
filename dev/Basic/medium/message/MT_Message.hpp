//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include "message/Message.hpp"
#include "entities/Person_MT.hpp"
#include "geospatial/network/RoadSegment.hpp"

namespace sim_mob
{
class BusStop;

namespace medium
{
class BusDriver;

enum ConfluxMessage
{
	MSG_PEDESTRIAN_TRANSFER_REQUEST = 5000000,
	MSG_INSERT_INCIDENT,
	MSG_WAITING_PERSON_ARRIVAL,
	MSG_WAKEUP_STASHED_PERSON,
	MSG_WAKEUP_MRT_PAX,
	MSG_WAKEUP_PEDESTRIAN,
	MSG_WARN_INCIDENT,
	MSG_PERSON_LOAD,
	MSG_PERSON_TRANSFER,
	MSG_TRAVELER_TRANSFER,
	// Vehicle Controller Messages
	MSG_VEHICLE_REQUEST,
	MSG_VEHICLE_ASSIGNMENT
};

enum PublicTransitMessage
{
	BOARD_BUS = 6000000,
	ALIGHT_BUS,
	BUS_ARRIVAL,
	BUS_DEPARTURE
};

/**
 * Message holding a pointer to BusStop
 */
class BusStopMessage: public messaging::Message
{
public:
	BusStopMessage(const BusStop* stop) :
			nextStop(stop)
	{
	}
	virtual ~BusStopMessage()
	{
	}
	const BusStop* nextStop;
	std::string busLines;
};

/**
 * Message holding a pointer to busDriver
 */
class BusDriverMessage: public messaging::Message
{
public:
	BusDriverMessage(BusDriver* busDriver) :
			busDriver(busDriver)
	{
	}
	virtual ~BusDriverMessage()
	{
	}
	BusDriver* busDriver;
};

/**
 * Message to wrap a Person
 */
class PersonMessage: public messaging::Message
{
public:
	PersonMessage(Person_MT* inPerson) :
			person(inPerson)
	{
	}

	virtual ~PersonMessage()
	{
	}

	Person_MT* person;
};

/**
 * Message to notify incidents
 */
class InsertIncidentMessage: public messaging::Message
{
public:
	InsertIncidentMessage(const RoadSegment* rs, double newFlowRate) :
			affectedSegment(rs), newFlowRate(newFlowRate)
	{
	}

	virtual ~InsertIncidentMessage()
	{
	}

	const RoadSegment* affectedSegment;
	double newFlowRate;
};

/**
 * Subclass wraps a bus stop into message so as to make alighting decision.
 * This is to allow it to function as an message callback parameter.
 */
class ArrivalAtStopMessage: public messaging::Message
{
public:
	ArrivalAtStopMessage(Person_MT* person) :
			waitingPerson(person)
	{
	}

	virtual ~ArrivalAtStopMessage()
	{
	}

	Person_MT* waitingPerson;
};

/**
 * Message to request a vehicle
 */
class VehicleRequestMessage: public messaging::Message
{
public:
	VehicleRequestMessage(Person_MT* p, const Node* sn, const Node* dn) :
			person(p), startNode(sn), destinationNode(dn)
	{
	}

	virtual ~VehicleRequestMessage()
	{
	}

	Person_MT* person;
	const Node* startNode;
	const Node* destinationNode;
};

/**
 * Message to assign a vehicle to a passenger
 */
class VehicleAssignmentMessage: public messaging::Message
{
public:
	VehicleAssignmentMessage(Person_MT* person) :
			driver(person)
	{
	}

	virtual ~VehicleAssignmentMessage()
	{
	}

	Person_MT* driver;
};

}
}

