//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   DevelopmentTypeTemplate.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on Mar 25, 2014, 5:54 PM
 */
#pragma once

#include "Common.hpp"
#include "Types.hpp"

namespace sim_mob {

    namespace long_term {

        class DevelopmentTypeTemplate {
        public:
            DevelopmentTypeTemplate(BigSerial developmentTypeId = INVALID_ID,
                    BigSerial templateId = INVALID_ID,
                    int landUseTypeId = 0);

            virtual ~DevelopmentTypeTemplate();

            /**
             * Getters and Setters
             */
            BigSerial getDevelopmentTypeId() const;
            BigSerial getTemplateId() const;
            int getLandUseTypeId() const;

            /**
             * Operator to print the DevelopmentTypeTemplate data.  
             */
            friend std::ostream& operator<<(std::ostream& strm,
                    const DevelopmentTypeTemplate& data);
        private:
            friend class DevelopmentTypeTemplateDao;
        private:
            BigSerial developmentTypeId;
            BigSerial templateId;
            int landUseTypeId;
        };
    }
}
