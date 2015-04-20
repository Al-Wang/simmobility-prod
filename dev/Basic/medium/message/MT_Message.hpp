//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * \file MT_Message.hpp
 *
 * \author Zhang huai Peng
 */

#pragma once

#include "entities/Agent.hpp"
#include "event/SystemEvents.hpp"
#include "event/args/EventArgs.hpp"
#include "message/MessageBus.hpp"
#include "event/EventPublisher.hpp"

namespace sim_mob {
class BusStop;

namespace medium {
class BusDriver;

enum PublicTransitMessage {
	BOARD_BUS = 6000000,
	ALIGHT_BUS,
	BUS_ARRIVAL,
	BUS_DEPARTURE,
	STORE_BUS_ARRIVAL,
	STORE_PERSON_WAITING,
	STORE_WAITING_AMOUNT
};

/**
 * Message holding a pointer to BusStop
 */
class BusStopMessage : public messaging::Message {
public:
	BusStopMessage(const BusStop* stop):nextStop(stop){}
	virtual ~BusStopMessage() {}
	const BusStop* nextStop;
};

/**
 * Message to transfer bus arrival time at bus stop
 */
class BusArrivalTimeMessage: public messaging::Message {
public:
	BusArrivalTimeMessage(const std::string& stopNo, const std::string& line,
			const std::string& trip, const std::string& arrival,
			const std::string& dwellTime, unsigned int sequence, double pctOccupancy) :
			busStopNo(stopNo), busLine(line), busTrip(trip), arrivalTime(arrival),
			dwellTime(dwellTime), sequenceNo(sequence), pctOccupancy(pctOccupancy) {}
	virtual ~BusArrivalTimeMessage() {
	}
	std::string busStopNo;
	std::string busLine;
	std::string busTrip;
	std::string arrivalTime;
	std::string dwellTime;
	unsigned int sequenceNo;
	double pctOccupancy; //percentage
};

class WaitingAmountMessage: public messaging::Message {
public:
	WaitingAmountMessage(const std::string& stopNo, const std::string& timeSlice, unsigned int waitingNum) :
			busStopNo(stopNo), timeSlice(timeSlice), waitingNum(waitingNum) {
	}
	virtual ~WaitingAmountMessage() {
	}
	std::string timeSlice;
	std::string busStopNo;
	unsigned int waitingNum;
};

/**
 * Message to transfer person waiting time at bus stop
 */
class PersonWaitingTimeMessage: public messaging::Message {
public:
	PersonWaitingTimeMessage(const std::string& stopNo,
			const std::string& personId, const std::string& waitingTime, const unsigned int failedBoardingTimes) :
			busStopNo(stopNo), personId(personId), waitingTime(waitingTime), failedBoardingTimes(failedBoardingTimes) {
	}
	virtual ~PersonWaitingTimeMessage() {
	}
	std::string busStopNo;
	std::string personId;
	std::string waitingTime;
	unsigned int failedBoardingTimes;
};

/**
 * Message holding a pointer to busDriver
 */
class BusDriverMessage : public messaging::Message {
public:
	BusDriverMessage(BusDriver* busDriver):busDriver(busDriver) {}
	virtual ~BusDriverMessage() {}
	BusDriver* busDriver;
};
}
}
