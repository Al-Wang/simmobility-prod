//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <string>

#include <stdint.h>  //NOTE: There's a bug in GCC whereby <cstdint> is not the same as <stdint.h>

#include <boost/serialization/access.hpp>

/*namespace geo {
class TripChainItem_t_pimpl;
}*/

namespace sim_mob
{

/**
 * Simple class to represent any point in time during a single day.
 *
 * \author Seth N. Hetu
 * \author Xu Yan
 *
 * This class is based on the ISO 8601 standard, with the following restrictions:
 *   \li No date may be specified, only times.
 *   \li Times must be of the format HH:MM:SS  --the colon is not optional.
 *   \li Only the seconds component may have a fractional component: HH:MM:SS.ffff..fff
 *   \li Hours and minutes are mandatory. Seconds (and second fractions) are optional.
 *
 * \note
 * Many of these constraints are not enforced, but may be in the future. ~Seth
 *
 *
 * All times are constant once created. (If we need modifiers later, we should probably use
 *    functions like "addSeconds", which return a different DailyTime object.)
 */
class DailyTime {
public:
	///Construct a new DailyTime from a given value. Subtract the "base" time (i.e., the day's start time).
	explicit DailyTime(uint32_t value=0, uint32_t base=0);

	///Construct a new DailyTime from a string formatted to ISO 8601 format.
	explicit DailyTime(const std::string& value);

	inline DailyTime(const DailyTime& dailytime) : time_(dailytime.getValue()), repr_(dailytime.getRepr_()) {}

	//Various comparison functions
	bool isBefore(const DailyTime& other) const;
	bool isBeforeEqual(const DailyTime& other) const;
	bool isAfter(const DailyTime& other) const;
	bool isAfterEqual(const DailyTime& other) const;
	bool isEqual(const DailyTime& other) const;

	///Retrieve the distance in MS between another DailyTime and this.
	uint32_t offsetMS_From(const DailyTime& other) const;

	//Accessors
	std::string toString() const;

	DailyTime& operator=(const DailyTime& dailytime);

	bool operator==(const DailyTime& dailytime);
	bool operator !=(const DailyTime& dailytime);

	inline uint32_t getValue() const { return time_; }
	inline std::string getRepr_() const { return repr_; }

    const DailyTime& operator+=(const DailyTime& dailytime);
    const DailyTime& operator-=(const DailyTime& dailytime);

    friend const DailyTime operator+(DailyTime lhs, const DailyTime& rhs)
    {
        return lhs += rhs;
    }

    friend const DailyTime operator-(DailyTime lhs, const DailyTime& rhs)
    {
        return lhs -= rhs;
    }

private:
	///Helper method: create a string representation from a given time value in miliseconds.
	///
	///\note
	///The maxFractionDigits parameter is currently ignored. ~Seth
	static std::string BuildStringRepr(uint32_t timeVal, size_t maxFractionDigits=4);

	///Helper method: generate a time from a formatted string.
	static uint32_t ParseStringRepr(std::string timeRepr);

	uint32_t time_;  //MS from 0, which corresponds to 00:00:00.00
	std::string repr_;

	//add by xuyan
public:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & time_;
		ar & repr_;
	}
};

bool operator==(const DailyTime& lhs, const DailyTime& rhs);
bool operator!=(const DailyTime& lhs, const DailyTime& rhs);
}


