//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   HouseholdDao.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on April 23, 2013, 5:17 PM
 */
#pragma once
#include "database/dao/SqlAbstractDao.hpp"
#include "database/entity/Household.hpp"

using namespace boost;
namespace sim_mob {

    namespace long_term {
        /**
         * Data Access Object to Household table on datasource.
         */
        class HouseholdDao : public db::SqlAbstractDao<Household> {
        public:
            HouseholdDao(db::DB_Connection& connection);
            virtual ~HouseholdDao();

        private:
            
            /**
             * Fills the given outObj with all values contained on Row. 
             * @param result row with data to fill the out object.
             * @param outObj to fill.
             */
            void fromRow(db::Row& result, Household& outObj);
            
            /**
             * Fills the outParam with all values to insert or update on datasource.
             * @param data to get values.
             * @param outParams to put the data parameters.
             * @param update tells if operation is an Update or Insert.
             */
            void toRow(Household& data, db::Parameters& outParams, bool update);

        public:
            void insertHousehold(Household& houseHold,std::string schema);
            std::vector<Household*> getPendingHouseholds(std::tm currentSimYear,std::tm lastDayOfCurrentSimYear);
        };
    }
}



