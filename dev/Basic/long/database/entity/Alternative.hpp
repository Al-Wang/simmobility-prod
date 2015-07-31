//Copyright (c) 2015 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//license.txt   (http://opensource.org/licenses/MIT)

/*
 * Alternative.hpp
 *
 *  Created on: 31 Jul, 2015
 *  Author: Chetan Rogbeer <chetan.rogbeer@smart.mit.edu>
 */

#pragma once

#include "Common.hpp"
#include "Types.hpp"

namespace sim_mob
{
	namespace long_term
	{
		class Alternative
		{
		public:
			Alternative(BigSerial id = 0, BigSerial planAreaId =0, std::string planAreaName="", BigSerial dwellingTypeId = 0, std::string dwellingTypeName="");
			virtual ~Alternative();

			Alternative(const Alternative& source);
			Alternative& operator=(const Alternative& source);

			BigSerial getId() const;
			BigSerial getPlanAreaId() const;
			std::string getPlanAreaName() const;
			BigSerial getDwellingTypeId() const;
			std::string getDwellingTypeName() const;

			friend std::ostream& operator<<(std::ostream& strm, const Alternative& data);

		private:
			BigSerial id;
			BigSerial planAreaId;
			std::string planAreaName;
			BigSerial dwellingTypeId;
			std::string dwellingTypeName;
		};
	}
}
