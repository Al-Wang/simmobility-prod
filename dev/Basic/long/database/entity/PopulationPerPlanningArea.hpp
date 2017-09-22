/*
 * PopulationPerPlanningArea.hpp
 *
 *  Created on: 13 Aug, 2015
 *  Author: Chetan Rogbeer <chetan.rogbeer@smart.mit.edu>
 */

#pragma once

#include "Common.hpp"
#include "Types.hpp"

namespace sim_mob
{
	namespace long_term
	{
		class PopulationPerPlanningArea
		{
		public:
			PopulationPerPlanningArea(int planningAreaId = 0, int population = 0, int ethnicityId=0, int ageCategoryId=0, double avgIncome=0, int avgHhSize=0, int unitType = 0, int floorArea = 0 );
			virtual ~PopulationPerPlanningArea();

			PopulationPerPlanningArea( const PopulationPerPlanningArea & source);
			PopulationPerPlanningArea& operator=(const PopulationPerPlanningArea & source);

			int getPlanningAreaId() const;
			int getPopulation() const;
			int getEthnicityId() const;
			int getAgeCategoryId() const;
			double getAvgIncome() const;
			int getAvgHhSize() const;
			int getUnitType() const;
			double getFloorArea() const;

			template<class Archive>
		    void serialize(Archive & ar,const unsigned int version);
			void saveData(std::vector<PopulationPerPlanningArea*> &s);
			std::vector<PopulationPerPlanningArea*> loadSerializedData();


		private:
			friend class PopulationPerPlanningAreaDao;

			int population;
			int planningAreaId;
			int ethnicityId;
			int ageCategoryId;
			double avgIncome;
			int avgHhSize;
			int unitType;
			double floorArea;
			static constexpr auto filename = "populationPerPlanningArea";
		};
	}
}

