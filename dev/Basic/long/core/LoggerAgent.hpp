/* 
 * Copyright Singapore-MIT Alliance for Research and Technology
 * 
 * File:   LoggerAgent.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on Feb 21, 2014, 1:32 PM
 */
#pragma once

#include "entities/Entity.hpp"
#include <iostream>
#include <fstream>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>

namespace sim_mob
{
    namespace long_term
    {
        /**
         * Entity responsible log messages in a thread-safe way without logs.
         * 
         * This special agent uses the MessageBus to receive messages to log. 
         * 
         * The output is depending on the given configuration.
         * 
         * Attention: This agent is designed to work with SimMobility Workers and 
         * MessageBus system. Do not use it within external threads.
         * 
         */
        class LoggerAgent : public Entity
        {
        public:
            // STATIC for now this will change in the future with new output mechanisms
            enum LogFile
            {
                BIDS,
                EXPECTATIONS,
                STDOUT,
                PARCELS,
                UNITS,
                PROJECTS,
                HH_PC,
                UNITS_IN_MARKET,
                LOG_TAXI_AVAILABILITY,
                LOG_VEHICLE_OWNERSIP,
                LOG_TAZ_LEVEL_LOGSUM,
                LOG_HOUSEHOLDGROUPLOGSUM,
                LOG_INDIVIDUAL_HITS_LOGSUM,
                LOG_HOUSEHOLDBIDLIST,
                LOG_INDIVIDUAL_LOGSUM_VO,
				LOG_SCREENINGPROBABILITIES,
				LOG_HHCHOICESET,
				LOG_ERROR,
				LOG_PRIMARY_SCHOOL_ASSIGNMENT,
				LOG_SECONDARY_SCHOOL_ASSIGNMENT,
				LOG_PRE_SCHOOL_ASSIGNMENT,
				LOG_UNIVERSITY_ASSIGNMENT,
				LOG_POLYTECH_ASSIGNMENT,
				LOG_SCHOOL_DESK,
				LOG_HH_AWAKENING,
				LOG_HH_EXIT,
				LOG_RANDOM_NUMS,
				LOG_DEV_ROI,
				LOG_HOUSEHOLD_STATISTICS,
				LOG_NON_ELIGIBLE_PARCELS,
				LOG_ELIGIBLE_PARCELS,
				LOG_GPR,
				LOG_JOB_ASIGN_PROBS,
				LOG_INDIVIDUAL_JOB_ASSIGN,
				LOG_DAILY_HOUSING_MARKET_UNITS,
				LOG_DAILY_HOUSING_MARKET_UNIT_TIMES,
				LOG_NEARSET_UNI_EZ_LINK,
				LOG_NEARSET_POLYTECH_EZ_LINK
            };

            LoggerAgent();
            virtual ~LoggerAgent();
            
            /**
             * Inherited from Entity
             */
            virtual UpdateStatus update(timeslice now);
            
            /**
             * Log the given message. 
             * @param logMsg to print.
             */
            void log(LogFile outputType, const std::string& logMsg);
            
        protected:
            /**
             * Inherited from Entity
             */
            virtual bool isNonspatial();
            virtual std::vector<sim_mob::BufferedBase*> buildSubscriptionList();
            virtual void HandleMessage(messaging::Message::MessageType type, const messaging::Message& message);

        private:
            /**
             * Inherited from Entity
             */
            void onWorkerEnter();
            void onWorkerExit();

        private:
            typedef boost::unordered_map<LogFile, std::ofstream*> Files;
            boost::unordered_map<LogFile, std::ofstream*> streams; 

            boost::mutex mtx;

        };
    }
}
