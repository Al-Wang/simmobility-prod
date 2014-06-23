//Copyright (c) 2014 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <boost/shared_ptr.hpp>

namespace sim_mob {

class ConnectionHandler;
class MessageConglomerate;
class BrokerBase;

///A base class for anything that can "handle" a message. Note that the message is passed in 
/// as a Json object, so handlers should proceed to parse the type of message they expect in the 
/// handle() method. (The serialize.h functions can help with this.)
class Handler {
public:
	virtual ~Handler() {}

	/**
	 * Called by the Broker when a message of this type is encountered to handle it.
	 * \param handler The ConnectionHandler which sent the message that triggered this callback.
	 * \param messages The current bundle of (all) messages.
	 * \param msgNumber The id number (from 0) of the message which triggered this callback.
	 * \param broker The Broker that processed the message that triggered this callback.
	 */
	virtual void handle(boost::shared_ptr<ConnectionHandler> handler, const MessageConglomerate& messages, int msgNumber, BrokerBase* broker) const = 0;
};

}

