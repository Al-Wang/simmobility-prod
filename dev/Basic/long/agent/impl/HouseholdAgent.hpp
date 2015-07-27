//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   HouseholdAgent.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on May 16, 2013, 6:36 PM
 */
#pragma once
#include "core/HousingMarket.hpp"
#include "agent/LT_Agent.hpp"
#include "database/entity/Household.hpp"
#include "event/LT_EventArgs.hpp"


namespace sim_mob
{
    namespace long_term
    {
        class HM_Model;
        class HouseholdBidderRole;
        class HouseholdSellerRole;
        /**
         * Represents an Long-Term household agent.
         * An household agent has the following capabilities:
         * - Sell units.
         * - Bid units. 
         */
        class HouseholdAgent : public LT_Agent
        {
        public:
            HouseholdAgent(BigSerial id, HM_Model* model, const Household* hh, HousingMarket* market, bool marketSeller = false, int day = 0);
            virtual ~HouseholdAgent();
            
            enum VehicleOwnershipOption
            {
            	NO_CAR, ONE_CAR, TWO_PLUS_CAR
            };
            VehicleOwnershipOption vehicleOwnershipOption;

            //not-thread safe
            void addUnitId (const BigSerial& unitId);
            void removeUnitId (const BigSerial& unitId);
            const IdVector& getUnitIds() const;
            const IdVector& getPreferableZones() const;
            HM_Model* getModel() const;
            HousingMarket* getMarket() const;
            const Household* getHousehold() const;
            void awakenHousehold();
            void setBuySellInterval( int value );
            int getBuySellInterval( ) const;
        
        protected:
            /**
             * Inherited from LT_Agent.
             */
            bool onFrameInit(timeslice now);
            sim_mob::Entity::UpdateStatus onFrameTick(timeslice now);
            void onFrameOutput(timeslice now);
            
            /**
             * Inherited from Entity. 
             */
            void onWorkerEnter();
            void onWorkerExit();
            virtual void HandleMessage(messaging::Message::MessageType type, const messaging::Message& message);
        private:
            /**
             * Method called to find some unit and bid it.
             * @param now current time.
             * @return true if some bid was sent, false otherwise.
             */
            bool bidUnit(timeslice now);
            
            /**
             * Events callbacks.
             */
            virtual void onEvent(event::EventId eventId, event::Context ctxId, event::EventPublisher* sender, const event::EventArgs& args);
            
            /**
             * Processes the given event.
             * @param eventId
             * @param ctxId
             * @param args
             */
            void processEvent(event::EventId eventId, event::Context ctxId, const event::EventArgs& args);
            
            /**
             * Processes external event.
             * @param args
             */
            void processExternalEvent(const ExternalEventArgs& args);
            
        private:
            HM_Model* model;
            HousingMarket* market;
            const Household* household;

            IdVector unitIds;
            IdVector preferableZones;

            HouseholdBidderRole* bidder;
            HouseholdSellerRole* seller;

            int buySellInterval;

            bool marketSeller; //tells if the agent is only a fake market seller
            int day;
        };
    }
}

