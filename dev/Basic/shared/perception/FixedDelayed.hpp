//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <stdint.h>
#include <list>
#include <utility>
#include <stdexcept>

#include "conf/settings/DisableMPI.h"

#include "util/LangHelpers.hpp"
#include "partitions/Serialization.hpp"


namespace sim_mob
{

class PackageUtils;
class UnPackageUtils;

/**
 * Templatized wrapper for data values with a fixed delay.
 *
 * \author Seth N. Hetu
 * \author Xu Yan
 * \author Li Zhemin
 *
 * Imposes a fixed delay (in ms) on a given object. For example, a FixedDelayed<int> is a
 * "(fixed) delayed integer". Attempting to retrieve the value of a FixedDelayed item will
 * return its most recently observed value (after a delay has been accounted for).
 *
 * \note
 * The FixedDelayed class copies items frequently, so if you are storing a large class or
 * non-copyable item, you should pass a pointer as the template paramter. By default, this pointer
 * is deleted when it is no longer needed by the FixedDelayed class.
 *
 * \note
 * This class should be used for continuous values. It should NOT be used for things like
 * reactionary signals or threshold values, since a call to sense() may discard values which
 * must trigger an action but are not technically the latest values.
 *
 * \note
 * If a FixedDelayed<> is constructed with a maximum delay of 0, it will attempt to optimize away
 * most list<> calls and shouldn't be much more inefficient than just storing the value directly.
 */
template <typename T>
class FixedDelayed {
public:
	/**
	 * Construct a new FixedDelayed item with the given delay in ms.
	 * \param maxDelayMS The maximum time to hold on to each sensation value. The default "delay" time is equal to this, but it can be set larger to allow variable reaction times.
	 * \param reclaimPtrs If true, any item discarded by this history list is deleted. Does nothing if the template type is not a pointer.
	 */
	explicit FixedDelayed(uint32_t maxDelayMS=0, bool reclaimPtrs=true);

	~FixedDelayed();


	/**
	 * Remove all delayed perceptions.
	 *
	 * \todo
	 * It's not clear why we need this. Resetting a FixedDelayed object should also take the
	 * current time into account. Drivers should provide an override if needed anyway. ~Seth
	 */
	void clear();


	/**
	 * Update the current time for reading and writing values. All calls to delay() and sense()
	 *  will use this to store and retrieve their values. Causes all sensed values past the maximum
	 *  delay time to be discarded.
	 * \param currTimeMS The current time in ms. Must be monotonically increasing.
	 */
	void update(uint32_t currTimeMS);


	/**
	 * Set the current perception delay to the given value. All calls to sense() are affected by this value.
	 * \param currDelayMS The new delay value. Must be less than maxDelayMS
	 */
	void set_delay(uint32_t currDelayMS);


	/**
	 * Delay a value, to be returned after the delay time.
	 * \param value The value to delay.
	 *
	 * \todo
	 * If we need to, we can add an "observedTimeMS" parameter (instead of using currTimeMS).
	 * For now, this is not required, but it is technically not very difficult to do.
	 */
	void delay(const T& value);


	/**
	 * Retrieve the current perceived value of this item.
	 */
	const T& sense();


	/**
	 * Return true if the current time is sufficiently advanced for sensing to occur.
	 * Sensing will always suceed after the warmup period (of "delayMS") has elapsed.
	 * If the value of delayMS is constantly changing, then you might want to wait until
	 * "maxDelayMS" has elapsed before assuming that this function will always succeed.
	 */
	bool can_sense();


private:
	//Serialization code, if MPI is enabled.
#ifndef SIMMOB_DISABLE_MPI
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version) {
    	//NOTE: We are going for a "naive" implementation first. This should "just work" (the entire
    	//      STL has serialize() defined), but it wastes space. We should take care to ensure that
    	//      memory-managed items are deleted properly.
    	ar & history;
    	ar & maxDelayMS;
    	ar & currDelayMS;
    	ar & currTime;
    	ar & reclaimPtrs;

    	//Iterators don't serialize; fortunately, we can just cheat here.
    	//ar & percFront;
    	update_iterator(); //Un-necessary on serialization; necessary on deserialization.
    }
#endif


private:
	//Helper function: delete the first item in the history array. Return true if there's more to delete.
	bool del_history_front();

	//Helper function: ensure that our percFront iterator is set to the correct (sense-able) History Item.
	void update_iterator();

	//Helper function: is there no chance of delay, ever? (I.e., is the max delay zero?)
	bool zero_delay() const;

public:
	void printHistory();

private:
	//Internal class to store an item and its observed time.
	struct HistItem {
		T item;
		uint32_t observedTime;

		explicit HistItem(T item=T(), uint32_t observedTime=0) : item(item), observedTime(observedTime) {}

		bool canObserve(uint32_t currTimeMS, uint32_t delayMS){
			return observedTime + delayMS <= currTimeMS;
		}

		//Serialization code, if MPI is enabled.
	#ifndef SIMMOB_DISABLE_MPI
	    template<class Archive>
	    void serialize(Archive& ar, const unsigned int version) {
	    	ar & item;
	    	ar & observedTime;
	    }
	#endif
	};



private:
	//The list of history items
	std::list<HistItem> history;

	//The maximum delay allowed by the system.
	const uint32_t maxDelayMS;

	//The current delay value
	uint32_t currDelayMS;

	//The current clock time
	uint32_t currTime;

	//The current "front" of the history list, used to return the correct value via sense().
	//If equal to history.end(), we can't sense right now.
	typename std::list<HistItem>::iterator percFront;

	//Whether or not to reclaim memory once a sensed item is no longer needed.
	bool reclaimPtrs;

	//Optimization: if zero delay, just store the currently-observed value here.
	//If *not* zero delay, this value is undefined.
	//The boolean value is set to "true" (valid) once a single value is set.
	std::pair<T, bool> zeroDelayValue;

	//Serialization-related friends
	friend class PackageUtils;
	friend class UnPackageUtils;
};

} //End sim_mob namespace




///////////////////////////////////////////////////////////
// Template implementation
///////////////////////////////////////////////////////////


template <typename T>
sim_mob::FixedDelayed<T>::FixedDelayed(uint32_t maxDelayMS, bool reclaimPtrs)
	: maxDelayMS(maxDelayMS), currDelayMS(maxDelayMS), currTime(0), reclaimPtrs(reclaimPtrs)
{
	percFront = history.end();
	zeroDelayValue.second = false;
}


template <typename T>
sim_mob::FixedDelayed<T>::~FixedDelayed()
{
	//Reclaim memory
	while (del_history_front());
}


template <typename T>
bool sim_mob::FixedDelayed<T>::zero_delay() const
{
	return maxDelayMS==0;
}

template <typename T>
void sim_mob::FixedDelayed<T>::printHistory()
{
	std::cout<<std::endl;
	for (typename std::list<HistItem>::iterator it=history.begin(); it!=history.end(); it++) {
		HistItem h = it;
		std::cout<<"printHistory: "<<h.observedTime<<" "<<h.item<<std::endl;
	}
	std::cout<<std::endl;
}
template <typename T>
bool sim_mob::FixedDelayed<T>::del_history_front()
{
	//Failsafe; also for "zero-delay".
	if (history.empty()) { return false; }

	//Reclaim memory, pop the list
	if (reclaimPtrs) {
		safe_delete_item(history.front().item);
	}
	history.pop_front();

	return !history.empty();
}


template <typename T>
void sim_mob::FixedDelayed<T>::clear()
{
	//Clear the list and reclaim memory.
	while (del_history_front());
}


template <typename T>
void sim_mob::FixedDelayed<T>::update(uint32_t currTimeMS)
{
	//We don't even use the history list if there is zero delay.
	if (zero_delay()) {
		return;
	}

	//Check and save the current time
	if (currTimeMS<currTime) {
		throw std::runtime_error("Error: FixedDelayed<> can't move backwards in time.");
	}
	if (currTimeMS==currTime) {
		return;
	}
	currTime = currTimeMS;

	if (currTime >= maxDelayMS) {
		//Loop, discard items which are past the maximum sensing window.
		uint32_t minTime = currTimeMS - maxDelayMS;
		while (!history.empty()) {
			bool found = false;
			if (history.front().observedTime <= minTime) {
				//This value only needs to be kept if there's nothing to replace it
				if (history.size()>1 && (++history.begin())->observedTime <= minTime) {
					//Delete it and keep scanning.
					del_history_front();
					found = true;
				}
			}
			if (!found) {
				break;
			}
		}
	}

	//Now we need to update our pseudo-"front" pointer
	set_delay(currDelayMS);
}


template <typename T>
void sim_mob::FixedDelayed<T>::set_delay(uint32_t currDelayMS)
{
	//Check
//	if (currDelayMS > maxDelayMS) {
//		std::stringstream msg;
//		msg <<"FixedDelayed: Can't set delay to (" <<currDelayMS <<") since it is greater than the maximum ("
//				<<maxDelayMS <<") specified in the constructor.";
//		throw std::runtime_error(msg.str().c_str());
//	}

	//We ignore delay updates if the max delay is zero.
	if (zero_delay()) {
		return;
	}

	//Save
	this->currDelayMS = currDelayMS;

	//Update our "front" pointer, used by the "sense()" function.
	update_iterator();
}


template <typename T>
void sim_mob::FixedDelayed<T>::update_iterator()
{
	percFront = history.end();
	for (typename std::list<HistItem>::iterator it=history.begin(); it!=history.end(); it++) {
		if (!it->canObserve(currTime, currDelayMS)) {
			break;
		}
		percFront = it;
	}
}


template <typename T>
void sim_mob::FixedDelayed<T>::delay(const T& value)
{
	if (zero_delay()) {
		zeroDelayValue.first = value;
		zeroDelayValue.second = true;
	} else {
		history.push_back(HistItem(value, currTime));
		set_delay(currDelayMS);
	}
}

template <typename T>
const T& sim_mob::FixedDelayed<T>::sense()
{
	//Consistency check, done via update.
	if (!can_sense()) {
		throw std::runtime_error("Can't sense: not enough time has passed.");
	}

	//Return is cheap either way, but still depends on zero_delay().
	if (zero_delay()) {
		return zeroDelayValue.first;
	} else {
		return percFront->item;
	}
}

template <typename T>
bool sim_mob::FixedDelayed<T>::can_sense() {
	if (zero_delay()) {
		return zeroDelayValue.second;
	} else {
		return percFront != history.end();
	}
}









