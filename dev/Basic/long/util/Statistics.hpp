//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   Statistics.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on June 14, 2013, 11:15 AM
 */
#pragma once

namespace sim_mob {
    
    namespace long_term {

        /**
         * Class for system statistics.
         */
        class Statistics {
        public:

            enum StatsParameter {
                N_BIDS = 0,
                N_BID_RESPONSES = 1,
                N_ACCEPTED_BIDS = 2,
				N_WAITING_TO_MOVE = 3,
				N_BIDDERS = 4,
				N_SELLERS = 5
            };

            /**
             * Increments the given parameter by 1.
             * @param param to increment.
             */
            static void increment(StatsParameter param);
            /**
             * Decrements the given parameter by 1.
             * @param param to decrement.
             */
            static void decrement(StatsParameter param);

            /**
             * Increments the given parameter by given value.
             * @param param to increment.
             * @param value to add.
             */
            static void increment(StatsParameter param, long value);

            /**
             * Decrements the given parameter by given value.
             * @param param to decrement.
             * @param value to subtract.
             */
            static void decrement(StatsParameter param, long value);
            
            static void reset(StatsParameter param);

            /**
             * Print out the current statistics.
             */
            static void print();

            static long getValue(StatsParameter param);
        };
    }
}

