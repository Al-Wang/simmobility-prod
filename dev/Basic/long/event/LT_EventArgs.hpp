//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * File:   LT_EventArgs.h
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on April 4, 2013, 5:42 PM
 */
#pragma once
#include "event/args/EventArgs.hpp"
#include "event/EventListener.hpp"
#include "database/entity/Bid.hpp"
#include "database/entity/ExternalEvent.hpp"
#include "Types.hpp"

namespace sim_mob {

    namespace long_term {

        class HM_ActionEventArgs : public sim_mob::event::EventArgs {
        public:
            HM_ActionEventArgs(BigSerial unitId);
            HM_ActionEventArgs(const HM_ActionEventArgs& orig);
            virtual ~HM_ActionEventArgs();

            /**
             * Gets the unit affected by the action.
             * @return 
             */
            BigSerial getUnitId()const;
        private:
            BigSerial unitId;
        };
        
        class ExternalEventArgs : public sim_mob::event::EventArgs {
        public:
            ExternalEventArgs(const ExternalEvent& event);
            ExternalEventArgs(const ExternalEventArgs& orig);
            virtual ~ExternalEventArgs();

            /**
             * Getters 
             */
            const ExternalEvent& getEvent() const;
        private:
            ExternalEvent event;
        };
    }
}

