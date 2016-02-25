//Copyright (c) 2016 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//license.txt   (http://opensource.org/licenses/MIT)


/*
 * AlternativeHedonicPrice.hpp
 *
 *  Created on: 24 Feb 2016
 *  Author: Chetan Rogbeer <chetan.rogbeer@smart.mit.edu>
 */

#pragma once
#include <string>
#include "Types.hpp"

namespace sim_mob
{
	namespace long_term
	{
		class AlternativeHedonicPrice
		{
		public:
			AlternativeHedonicPrice( int planning_area_id = 0, std::string planning_area ="", int dwelling_type = 0, double total_price =.0);
			virtual ~AlternativeHedonicPrice();

			int getPlanningAreaId();
			std::string getPlanningArea();
			int getDwellingType();
			double getTotalPrice();

		private:
			friend class AlternativeHedonicPriceDao;

			BigSerial planning_area_id;
			std::string planning_area;
			int dwelling_type;
			double total_price;
		};
	}

}
