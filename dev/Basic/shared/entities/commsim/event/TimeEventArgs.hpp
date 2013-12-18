//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * TimeEventArgs.hpp
 *
 *  Created on: May 28, 2013
 *      Author: vahid
 */

#pragma once

#include <iostream>

#include "entities/commsim/serialization/JsonParser.hpp"
#include "event/EventListener.hpp"
#include "metrics/Frame.hpp"
#include "entities/commsim/event/JsonSerializableEventArgs.hpp"

namespace sim_mob {

class TimeEventArgs: public sim_mob::comm::JsonSerializableEventArgs {
public:
	TimeEventArgs(timeslice time);
	virtual ~TimeEventArgs();
	//todo should be a virtual from a base class
	virtual Json::Value toJSON()const;

private:
	timeslice time;
};


} /* namespace sim_mob */