/* 
 * Copyright Singapore-MIT Alliance for Research and Technology
 * 
 * File:   HM_Model.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *
 * Created on October 21, 2013, 3:08 PM
 */
#pragma once
#include "Model.hpp"
#include "database/entity/Household.hpp"
#include "database/entity/Unit.hpp"
#include "database/entity/Individual.hpp"
#include "database/entity/Awakening.hpp"
#include "database/entity/VehicleOwnershipCoefficients.hpp"
#include "database/entity/TaxiAccessCoefficients.hpp"
#include "database/entity/HousingInterestRate.hpp"
#include "database/entity/Postcode.hpp"
#include "database/entity/Establishment.hpp"
#include "database/entity/Job.hpp"
#include "database/entity/LogSumVehicleOwnership.hpp"
#include "database/entity/DistanceMRT.hpp"
#include "core/HousingMarket.hpp"
#include "boost/unordered_map.hpp"

namespace sim_mob
{
    namespace long_term
    {
        /**
         * Class that contains Housing market model logic.
         */
        class HM_Model : public Model
        {
        public:
            typedef std::vector<Unit*> UnitList;
            typedef boost::unordered_map<BigSerial, Unit*> UnitMap;

            typedef std::vector<Household*> HouseholdList;
            typedef boost::unordered_map<BigSerial, Household*> HouseholdMap;

            typedef std::vector<Individual*> IndividualList;
            typedef boost::unordered_map<BigSerial, Individual*> IndividualMap;

            typedef std::vector<Postcode*> PostcodeList;
            typedef boost::unordered_map<BigSerial, Postcode*> PostcodeMap;

            typedef std::vector<Awakening*> AwakeningList;
            typedef boost::unordered_map<BigSerial, Awakening*> AwakeningMap;

            typedef std::vector<VehicleOwnershipCoefficients*> VehicleOwnershipCoeffList;
            typedef boost::unordered_map<BigSerial, VehicleOwnershipCoefficients*> VehicleOwnershipCoeffMap;

            typedef std::vector<TaxiAccessCoefficients*> TaxiAccessCoeffList;
            typedef boost::unordered_map<BigSerial, TaxiAccessCoefficients*> TaxiAccessCoeffMap;

            typedef std::vector<Establishment*> EstablishmentList;
            typedef boost::unordered_map<BigSerial, Establishment*> EstablishmentMap;

            typedef std::vector<Job*> JobList;
            typedef boost::unordered_map<BigSerial, Job*> JobMap;

            typedef std::vector<HousingInterestRate*> HousingInterestRateList;
            typedef boost::unordered_map<BigSerial, HousingInterestRate*> HousingInterestRateMap;

            typedef std::vector<LogSumVehicleOwnership*> VehicleOwnershipLogsumList;
            typedef boost::unordered_map<BigSerial, LogSumVehicleOwnership*> VehicleOwnershipLogsumMap;

            typedef std::vector<DistanceMRT*> DistMRTList;
            typedef boost::unordered_map<BigSerial, DistanceMRT*> DistMRTMap;

            /**
             * Taz statistics
             */
            class TazStats
            {
            public:
                TazStats(BigSerial tazId = INVALID_ID);
                virtual ~TazStats();
                
                /**
                 * Getters 
                 */
                BigSerial getTazId() const;
                long int getHH_Num() const;
                double getHH_TotalIncome() const;
                double getHH_AvgIncome() const;
            private:
                friend class HM_Model;
                void updateStats(const Household& household);

                BigSerial tazId;
                long int hhNum;
                double hhTotalIncome;
            };
            
            typedef boost::unordered_map<BigSerial, HM_Model::TazStats*> StatsMap;
            

            HM_Model(WorkGroup& workGroup);
            virtual ~HM_Model();
            
            /**
             * Getters & Setters 
             */
            const Unit* getUnitById(BigSerial id) const;
            BigSerial getUnitTazId(BigSerial unitId) const;
            const TazStats* getTazStats(BigSerial tazId) const;
            const TazStats* getTazStatsByUnitId(BigSerial unitId) const;


            Household* getHouseholdById( BigSerial id) const;
			Individual* getIndividualById( BigSerial id) const;
            Awakening* getAwakeningById( BigSerial id) const;
            Postcode* getPostcodeById(BigSerial id) const;

            void hdbEligibilityTest(int );
            void unitsFiltering();
            void incrementAwakeningCounter();
            int getAwakeningCounter() const;

            HousingMarket* getMarket();

            HouseholdList* getHouseholdList();

            HousingInterestRateList* getHousingInterestRateList();

            void incrementBidders();
            void decrementBidders();
            int	 getNumberOfBidders();

            void incrementLifestyle1HHs();
            void incrementLifestyle2HHs();
            void incrementLifestyle3HHs();
            int getLifestyle1HHs() const;
            int getLifestyle2HHs() const;
            int getLifestyle3HHs() const;
            VehicleOwnershipCoeffList getVehicleOwnershipCoeffs()const;
            VehicleOwnershipCoefficients* getVehicleOwnershipCoeffsById( BigSerial id) const;
            TaxiAccessCoeffList getTaxiAccessCoeffs()const;
            TaxiAccessCoefficients* getTaxiAccessCoeffsById( BigSerial id) const;
            void addUnit(Unit* unit);
            std::vector<BigSerial> getRealEstateAgentIds();
            VehicleOwnershipLogsumList getVehicleOwnershipLosums()const;
            LogSumVehicleOwnership* getVehicleOwnershipLogsumsById( BigSerial id) const;
            void setTaxiAccess(const Household *household);
            DistMRTList getDistanceMRT()const;
            DistanceMRT* getDistanceMRTById( BigSerial id) const;


        protected:
            /**
             * Inherited from Model.
             */
            void startImpl();
            void stopImpl();
            void update(int day);

        private:
            // Data
            HousingMarket market;

            HouseholdList households;
            HouseholdMap householdsById;

            UnitList units; //residential only.
            UnitMap unitsById;

            StatsMap stats;

            IndividualList individuals;
            IndividualMap individualsById;

            PostcodeList postcodes;
            PostcodeMap postcodesById;

            EstablishmentList establishments;
            EstablishmentMap establishmentsById;

            HousingInterestRateList housingInterestRates;
            HousingInterestRateMap housingInterestRatesById;

            JobList jobs;
            JobMap jobsById;

            boost::unordered_map<BigSerial, BigSerial> assignedUnits;
            VehicleOwnershipCoeffList vehicleOwnershipCoeffs;
            VehicleOwnershipCoeffMap vehicleOwnershipCoeffsById;
            TaxiAccessCoeffList taxiAccessCoeffs;
            TaxiAccessCoeffMap taxiAccessCoeffsById;
            AwakeningList awakening;
            AwakeningMap awakeningById;
            HouseholdStatistics household_stats;
            VehicleOwnershipLogsumList vehicleOwnershipLogsums;
            VehicleOwnershipLogsumMap vehicleOwnershipLogsumById;
            DistMRTList mrtDistances;
            DistMRTMap mrtDistancesById;

            int	initialHHAwakeningCounter;
            int numberOfBidders;
            int numLifestyle1HHs;
            int numLifestyle2HHs;
            int numLifestyle3HHs;
            std::vector<BigSerial> realEstateAgentIds;
            bool hasTaxiAccess;

        };
    }
}

