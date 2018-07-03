//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include "metrics/Frame.hpp"
#include <boost/random.hpp>

namespace sim_mob
{


///Simple struct to hold parameters which only exist for a single update tick.
/// \author Seth N. Hetu
///
///Passed into Agent::frame_tick() and Agent::frame_tick_output().
///This class should only contain general properties; each Role should define its own subclass and return that
/// in Agent::make_frame_tick_params().
///Note that the mt19937 generator cannot be changed once this object is constructed.
struct UpdateParams {
	UpdateParams() : now(0,0), gen(local_gen){}
	explicit UpdateParams(boost::mt19937& gen) : now(0,0), gen(gen) {}
	virtual ~UpdateParams(){

	}

	///Reset this struct for use with the next frame.
	virtual void reset(timeslice now) {
		this->now = now;
	}

	///The current timeslice.
	timeslice now;
	///The local random number generator if no generator specified
	boost::mt19937 local_gen;
	///The random number generator being used by this Agent.
	boost::mt19937& gen;
};


}
