//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   HousingMarket.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on March 11, 2013, 4:13 PM
 */
#pragma once

#include <boost/unordered_map.hpp>
#include "entities/Entity.hpp"
#include "database/entity/Unit.hpp"

namespace sim_mob
{
    namespace long_term
    {
        class LT_Agent;

        /**
         * Represents the housing market.
         * 
         * This guarantees thread safety without locks using 
         * the following assumptions:
         *  1- All calls to update, add and delete entry will produce 
         * an internal message that will be processed in the MessageBus 
         * main thread. This means that all actions will be 
         * available only in the very beginning of the next tick.
         * 
         *  2- It is guaranteed that the list is not changed during the 
         * simulation process all values are updated, added or removed 
         * on the very beginning of the next tick.
         * If an agent wants to sell his unit the we should create 
         * a new entry on Housing Market entity.
         * All entries on the market are available to be visited for 
         * other agents.
         * 
         * Bidders should use **getAvailableEntries** method to get 
         * the current list of available units.
         * 
         * Th main responsibility is the management of: 
         *  - avaliable units to sell
         *  - Notify agents which are looking for home. 
         */
        class HousingMarket : public sim_mob::Entity
        {
        public:

            /**
             * Represents a Market entry on Housing Market model.
             * If a unit has an entry on the market it means
             * that this unit is available to sell.
             * 
             * @param owner of the Unit.
             * @param unit to be that is available on the market.
             * @param askingPrice of the unit.
             * @param hedonicPrice of the unit.
             */
            class Entry
            {
            public:
                Entry(LT_Agent* owner, BigSerial unitId, BigSerial postcodeId, BigSerial tazId, double askingPrice, double hedonicPrice);
                Entry( const Entry& source );

                virtual ~Entry();

                Entry&  operator=(const Entry& source);

                BigSerial getUnitId() const;
                BigSerial getPostcodeId() const;
                BigSerial getTazId() const;
                double getAskingPrice() const;
                double getHedonicPrice() const;
                LT_Agent* getOwner() const;

                void setAskingPrice(double askingPrice);
                void setHedonicPrice(double hedonicPrice);
                void setOwner(LT_Agent* owner);

            private:
                BigSerial tazId;
                BigSerial postcodeId;
                BigSerial unitId;
                double askingPrice;
                double hedonicPrice;
                LT_Agent* owner;
            };

            typedef std::vector<Entry*> EntryList;
            typedef std::vector<const Entry*> ConstEntryList;
            typedef boost::unordered_map<BigSerial, Entry*> EntryMap;
            typedef boost::unordered_map<BigSerial, EntryMap> EntryMapById;

        public:
            HousingMarket();
            virtual ~HousingMarket();

            /**
             * Adds new entry.
             * **Attention** changes are not visible after this call.
             * a message will be generated and processed on the next 
             * simulation day.
             * @param entry to add.
             */
            void addEntry(const Entry& entry);

            /**
             * Updates entry.
             * **Attention** changes are not visible after this call.
             * a message will be generated and processed on the next 
             * simulation day.
             * @param entry to update.
             */
            void updateEntry(const Entry& entry);

            /**
             * Removes entry by given unit id.
             * **Attention** changes are not visible after this call.
             * a message will be generated and processed on the next 
             * simulation day.
             * @param unitId to remove.
             */
            void removeEntry(const BigSerial& unitId);

            /**
             * Get available entries map filtered by given Taz ids.
             * @param tazIds to filter the options.
             * @param outList list to receive available units filtered by ids.
             */
            void getAvailableEntries(const IdVector& tazIds, ConstEntryList& outList);
            
            /**
             * Get all available entries map.
             * @param outList list to receive available units.
             */
            void getAvailableEntries(ConstEntryList& outList);

            /**
             * Get a pointer of the entry by given unit identifier.
             * You should not change the returned values.
             * @param unitId to get the entry.
             * @return Entry pointer or nullptr. 
             */
            const Entry* getEntryById(const BigSerial& unitId);

            /**
             * Inherited from Entity
             */
            virtual UpdateStatus update(timeslice now);

            size_t getEntrySize();

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
            EntryMap entriesById; // original copies
            EntryMapById entriesByTazId; // only lookup.
        };
    }
}

