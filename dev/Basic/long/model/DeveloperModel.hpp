/* 
 * Copyright Singapore-MIT Alliance for Research and Technology
 * 
 * File:   HM_Model.hpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 *       : Gishara Premarathne <gishara@smart.mit.edu>
 *
 * Created on March 11, 2014, 3:08 PM
 */
#pragma once
#include "boost/unordered_map.hpp"
#include "Model.hpp"
#include "database/entity/Developer.hpp"
#include "database/entity/Template.hpp"
#include "database/entity/Parcel.hpp"
#include "database/entity/LandUseZone.hpp"
#include "database/entity/DevelopmentTypeTemplate.hpp"
#include "database/entity/TemplateUnitType.hpp"
#include "database/entity/Project.hpp"
#include "database/entity/ParcelMatch.hpp"
#include "database/entity/SlaParcel.hpp"
#include "database/entity/UnitType.hpp"
#include "database/entity/Building.hpp"
#include "database/entity/ParcelAmenities.hpp"
#include "database/entity/MacroEconomics.hpp"
#include "database/entity/LogsumForDevModel.hpp"
#include "database/entity/ParcelsWithHDB.hpp"
#include "database/entity/TAO.hpp"
#include "database/entity/UnitPriceSum.hpp"
#include "database/entity/TazLevelLandPrice.hpp"
#include "database/entity/SimulationStoppedPoint.hpp"
#include "database/entity/DevelopmentPlan.hpp"
#include "database/entity/BuildingAvgAgePerParcel.hpp"
#include "database/entity/ROILimits.hpp"
#include "database/entity/HedonicCoeffs.hpp"
#include "database/entity/HedonicCoeffsByUnitType.hpp"
#include "database/entity/LagPrivateT.hpp"
#include "database/entity/HedonicLogsums.hpp"
#include "database/entity/TAOByUnitType.hpp"
#include "database/entity/LagPrivate_TByUnitType.hpp"
#include "agent/impl/DeveloperAgent.hpp"
#include "agent/impl/RealEstateAgent.hpp"
#include "model/HM_Model.hpp"
#include "conf/ConfigManager.hpp"
#include "conf/ConfigParams.hpp"


namespace sim_mob {
    namespace long_term {

        class DeveloperModel : public Model {
        public:

            typedef std::vector<DeveloperAgent*> DeveloperList;
            typedef std::vector<Template*> TemplateList;
            typedef std::vector<Parcel*> ParcelList;
            typedef std::vector<LandUseZone*> LandUseZonesList;
            typedef std::vector<DevelopmentTypeTemplate*> DevelopmentTypeTemplateList;
            typedef std::vector<TemplateUnitType*> TemplateUnitTypeList;
            typedef std::vector<UnitType*> UnitTypeList;
            typedef std::vector<Building*> BuildingList;
            typedef std::vector<Project*> ProjectList;
            typedef std::vector<ParcelAmenities*> AmenitiesList;
            typedef std::vector<MacroEconomics*> MacroEconomicsList;
            typedef std::vector<LogsumForDevModel*> AccessibilityLogsumList;
            typedef std::vector<ParcelsWithHDB*> ParcelsWithHDBList;
            typedef std::vector<TAO*> TAOList;
            typedef std::vector<TAOByUnitType*> TAOByUTList;
            typedef std::vector<UnitPriceSum*> UnitPriceSumList;
            typedef std::vector<TazLevelLandPrice*>TazLevelLandPriceList;
            typedef std::vector<SimulationStoppedPoint*>SimulationStoppedPointList;
            typedef std::vector<BuildingAvgAgePerParcel*>BuildingAvgAgePerParcelList;
            typedef std::vector<ROILimits*>ROILimitsList;
            typedef std::vector<Unit*>UnitList;
            typedef std::vector<HedonicCoeffs*>HedonicCoeffsList;
            typedef std::vector<HedonicCoeffsByUnitType*>HedonicCoeffsByUnitTypeList;
            typedef std::vector<LagPrivateT*>LagPrivateTList;
            typedef std::vector<HedonicLogsums*>HedonicLogsumsList;
            typedef std::vector<LagPrivate_TByUnitType*>LagPrivateTByUTList;

            //maps
            typedef boost::unordered_map<BigSerial,Parcel*> ParcelMap;
            typedef boost::unordered_map<BigSerial,UnitType*> UnitTypeMap;
            typedef boost::unordered_map<BigSerial,ParcelAmenities*> AmenitiesMap;
            typedef boost::unordered_map<BigSerial,MacroEconomics*> MacroEconomicsMap;
            typedef boost::unordered_map<BigSerial,LogsumForDevModel*> AccessibilityLogsumMap;
            typedef boost::unordered_map<BigSerial,ParcelsWithHDB*> ParcelsWithHDBMap;
            typedef boost::unordered_map<std::string,TAO*> TAOMap;
            typedef boost::unordered_map<std::string,TAOByUnitType*> TAOByUTMap;
            typedef boost::unordered_map<int,TAOByUnitType*> TAOByIdMap;
            typedef boost::unordered_map<BigSerial,UnitPriceSum*> UnitPriceSumMap;
            typedef boost::unordered_map<BigSerial,TazLevelLandPrice*> TazLevelLandPriceMap;
            typedef boost::unordered_map<BigSerial,Project*> ProjectMap;
            typedef boost::unordered_map<BigSerial,BuildingAvgAgePerParcel*> BuildingAvgAgePerParcelMap;
            typedef boost::unordered_map<BigSerial,ROILimits*> ROILimitsMap;
            typedef boost::unordered_map<BigSerial,HedonicCoeffs*>HedonicCoeffsMap;
            typedef boost::unordered_map<BigSerial,HedonicCoeffsByUnitType*>HedonicCoeffsByUnitTypeMap;
            typedef boost::unordered_map<BigSerial,LagPrivateT*>LagPrivateTMap;
            typedef boost::unordered_map<BigSerial,HedonicLogsums*>HedonicLogsumsMap;
            typedef boost::unordered_map<BigSerial,LagPrivate_TByUnitType*>LagPrivateTByUTMap;

        public:
            DeveloperModel(WorkGroup& workGroup);
            DeveloperModel(WorkGroup& workGroup, unsigned int timeIntervalDevModel );
            virtual ~DeveloperModel();
            
            /*
             * create developer agents for each parcel in the given ParcelList
             */
            void createDeveloperAgents(ParcelList initParcelList, bool onGoingProject, bool day0Project);

            /*
             * create developer agent for BTO launching
             */
            void createBTODeveloperAgents();

            void wakeUpDeveloperAgents(DeveloperList devAgentList);

            /*
             * process the initial parcel list e to create sub parcel lists as follow:
             * 1. parcelsWithProjectsList 2. developmentCandidateParcelList 3. nonEligibleParcelList
             *
             */
            void processParcels();

            /*
             * check whether a project is older than 90 days or more.
             * if so, add it to the developmentCandidateParcelList if the parcel is not in the list yet.
             */
            void processProjects();

            /**
             * Getters 
             */
            unsigned int getTimeInterval() const;
            Parcel* getParcelById(BigSerial id) const;
            SlaParcel* getSlaParcelById(std::string id) const;
            const LandUseZone* getZoneById(BigSerial id) const;
            const DevelopmentTypeTemplateList& getDevelopmentTypeTemplates() const;
            const TemplateUnitTypeList& getTemplateUnitType() const;
            const UnitType* getUnitTypeById(BigSerial id) const;
            const ParcelAmenities* getAmenitiesById(BigSerial fmParcelId) const;
            const MacroEconomics* getMacroEconById(BigSerial id) const;
            ParcelList getDevelopmentCandidateParcels(bool isInitial);

            /*
             * these logsums are only used in 2008.
             */
            const LogsumForDevModel* getAccessibilityLogsumsByTAZId(BigSerial fmParcelId) const;

            const ParcelsWithHDB* getParcelsWithHDB_ByParcelId(BigSerial fmParcelId) const;
            DeveloperList getDeveloperAgents();
            const TAO* getTaoByQuarter(std::string& quarterStr);


            /*
             * @param days number of days the simulation runs
             */
            void setDays(int days);

            /*
             * reload the zones from the db if a zoning rule change event occurs
             */
            void reLoadZonesOnRuleChangeEvent();

            //calculate the actual gpr of a given parcel
            //float getActualGpr(Parcel &parcel);

            //get the allowed gpr of zoning.
            float getAllowedGpr(Parcel &parcel);

            /**
            * check whether a given parcel is empty or not
            * @param id of the given parcel
            */
            const bool isEmptyParcel(BigSerial id) const;

            /**
             * check whether a given parcel is empty or not
             * @param id of the given parcel
             */
            const int isFreeholdParcel(BigSerial id) const;

            /*
             * increment the id of the last project in db
             * @return next projectId
             */
            BigSerial getProjectIdForDeveloperAgent();
            /*
             * increment the id of the last building in db
             * @return next buildingId.
             */
            BigSerial getBuildingIdForDeveloperAgent();
            /*
             * increment the id of the last building in db
             * @return next unitId.
             */
            boost::mutex unitIdLock;
            BigSerial getUnitIdForDeveloperAgent();

            void setUnitId(BigSerial unitId);
            /*
             * @return the loaded building list
             */
            BuildingList getBuildings();
            /*
             * add the building id's of the demolished buildings to preserve and add to new buildings later
             * @param building Id of the demolished building
             */
            void addNewBuildingId(BigSerial buildingId);

            /*
             * get the year of the simulation
             * @return simulation year
             */
            int getSimYearForDevAgent();

            const RealEstateAgent* getRealEstateAgentForDeveloper();

            void setRealEstateAgentIds(std::vector<BigSerial> realEstateAgentIdVec);

            void setHousingMarketModel(HM_Model *housingModel);

            /*
            * @return newly generated 7digit postcode for new units in each parcel.
            */
            int getPostcodeForDeveloperAgent();

            /*
             * @return unit price by id
             */
            const UnitPriceSum* getUnitPriceSumByParcelId(BigSerial fmParcelId) const;

            /*
             * @return land price by taz id
             */
            const TazLevelLandPrice* getTazLevelLandPriceByTazId(BigSerial tazId) const;

            /*
             * return the average age of all the buildings in a given parcel for the current simulation date
             */
            const BuildingAvgAgePerParcel* getBuildingAvgAgeByParcelId(const BigSerial fmParcelId) const;

            /*
             * @return StatusOfWorld object to be inserted to DB at the end of the simulation
             */
            const boost::shared_ptr<SimulationStoppedPoint> getSimStoppedPointObj(BigSerial simVersionId);

            Parcel* getParcelWithOngoingProjectById(BigSerial parcelId) const;

            void addNewBuildings(boost::shared_ptr<Building> &newBuilding);

            void addNewProjects(boost::shared_ptr<Project> &newProject);

            void addNewUnits(boost::shared_ptr<Unit> &newUnit);

            void addProfitableParcels(boost::shared_ptr<Parcel> &profitableParcel);

            void addPotentialProjects(boost::shared_ptr<PotentialProject> &potentialProject);

            std::vector<boost::shared_ptr<Building> > getBuildingsVec();

            std::vector<boost::shared_ptr<Unit> > getUnitsVec();

            std::vector<boost::shared_ptr<Project> > getProjectsVec();

            std::vector<boost::shared_ptr<Parcel> > getProfitableParcelsVec();

            const int getOpSchemaloadingInterval();

            void setOpSchemaloadingInterval(int opSchemaLoadingInt);

            void addDevelopmentPlans(boost::shared_ptr<DevelopmentPlan> &devPlan);

            std::vector<boost::shared_ptr<DevelopmentPlan> > getDevelopmentPlansVec();

            Project* getProjectByParcelId(BigSerial parcelId) const;

            void setStartDay(int day);

            int getStartDay() const;

            ROILimitsList getROILimits() const;

            const ROILimits* getROILimitsByDevelopmentTypeId(BigSerial devTypeId) const;

            std::vector<BigSerial> getBTOUnits(std::tm currentDate);

            void loadHedonicCoeffs(DB_Connection &conn);

            const HedonicCoeffs* getHedonicCoeffsByPropertyTypeId(BigSerial propertyId) const;

            void loadHedonicCoeffsByUnitType(DB_Connection &conn);

            const HedonicCoeffsByUnitType* getHedonicCoeffsByUnitTypeId(BigSerial unitTypeId) const;

            void loadPrivateLagT(DB_Connection &conn);

            const LagPrivateT* getLagPrivateTByPropertyTypeId(BigSerial propertyId) const;

            void loadHedonicLogsums(DB_Connection &conn);

            const HedonicLogsums* getHedonicLogsumsByTazId(BigSerial tazId) const;

            void loadTaoByUnitType(DB_Connection &conn);

            const TAOByUnitType* getTaoUTByQuarter(std::string& quarterStr);

            const TAOByUnitType* getTaoUTById(int& id);

            void loadPrivateLagTByUT(DB_Connection &conn);

            const LagPrivate_TByUnitType* getLagPrivateTByUnitTypeId(BigSerial unitTypeId);

            const std::string &getScenario();

            void loadTAO(DB_Connection &conn);

            bool isToaPayohTaz(BigSerial tazId);


        protected:
            /**
             * Inherited from Model.
             */
            void startImpl();
            void stopImpl();

        private:
            DeveloperList developers;
            TemplateList templates;
            ParcelList initParcelList;
            ParcelList parcelsWithProjectsList;
            ParcelList developmentCandidateParcelList;
            ParcelList nonEligibleParcelList;
            ParcelList emptyParcels;
            ParcelList freeholdParcels;
            ParcelList parcelsWithOngoingProjects; //this is loaded when the simulation is resumed from a previous run
            ParcelList parcelsWithDay0Projects;
            BuildingList buildings;
            DevelopmentTypeTemplateList developmentTypeTemplates;
            TemplateUnitTypeList templateUnitTypes;
            ProjectList projects;
            std::vector<boost::shared_ptr<Project> > newProjects;
            ParcelMap parcelsById;
            ParcelMap emptyParcelsById;
            ParcelMap devCandidateParcelsById;
            ParcelMap parcelsWithOngoingProjectsById;
            ParcelMap freeholdParcelsById;
            unsigned int timeInterval;
            std::vector<BigSerial> existingProjectIds;
            std::vector<BigSerial> newBuildingIdList;
            UnitTypeList unitTypes;
            UnitTypeMap unitTypeById;
            int dailyParcelCount;
            int dailyAgentCount;
            bool isParcelRemain;
            bool isDevAgentsRemain;
            int numSimulationDays;
            AmenitiesList amenities;
            AmenitiesMap amenitiesById;
            MacroEconomicsList macroEconomics;
            MacroEconomicsMap macroEconomicsById;
            std::vector<BigSerial> realEstateAgentIds;
            BigSerial realEstateAgentIdIndex;
            HM_Model *housingMarketModel;
            BigSerial postcodeForDevAgent;
            bool initPostcode;
            BigSerial unitIdForDevAgent;
            BigSerial buildingIdForDevAgent;
            BigSerial projectIdForDevAgent;
            BigSerial simYear;
            AccessibilityLogsumList accessibilityList;
            AccessibilityLogsumMap accessibilityByTazId;
            ParcelsWithHDBList parcelsWithHDB;
            ParcelsWithHDBMap parcelsWithHDB_ById;
            TAOList taoList;
            TAOMap taoByQuarterStr;
            int devAgentCount;
            double minLotSize;
            UnitPriceSumList unitPriceSumList;
            UnitPriceSumMap unitPriceSumByParcelId;
            TazLevelLandPriceList tazLevelLandPriceList;
            TazLevelLandPriceMap tazLevelLandPriceByTazId;
            boost::mutex buildingIdLock;
            mutable boost::mutex mtx1;
            boost::mutex addParcelLock;
            boost::mutex addBuildingLock;
            boost::mutex addUnitsLock;
            boost::mutex addProjectsLock;
            boost::mutex addPotentialProjectsLock;
            boost::mutex addDevPlansLock;
            bool isRestart;
            SimulationStoppedPointList simStoppedPointList;
            boost::mutex projectIdLock;
            boost::mutex postcodeLock;
            std::vector<boost::shared_ptr<Building> > newBuildings;
            std::vector<boost::shared_ptr<Unit> > newUnits;
            std::vector<boost::shared_ptr<Parcel> > profitableParcels;
            std::vector<boost::shared_ptr<PotentialProject> > potentialProjects;
            int OpSchemaLoadingInterval;
            std::vector<boost::shared_ptr<DevelopmentPlan> > developmentPlansVec;
            ProjectMap projectByParcelId;
            int startDay; //start tick of the simulation
            BuildingAvgAgePerParcelList buildingAvgAgePerParcel;
            BuildingAvgAgePerParcelMap BuildingAvgAgeByParceld;
            std::string  outputSchema;
            ROILimitsList roiLimits;
            ROILimitsMap roiLimitsByDevTypeId;
            UnitList btoUnits;
            UnitList ongoingBtoUnits;
            HedonicCoeffsList hedonicCoefficientsList;
            HedonicCoeffsByUnitTypeList hedonicCoefficientsByUnitTypeList;
            HedonicCoeffsMap hedonicCoefficientsByPropertyTypeId;
            HedonicCoeffsByUnitTypeMap hedonicCoefficientsByUnitTypeId;
            LagPrivateTList privateLagsList;
            LagPrivateTMap privateLagsByPropertyTypeId;
            HedonicLogsumsList hedonicLogsumsList;
            HedonicLogsumsMap hedonicLogsumsByTazId;
            TAOByUTList taoByUnitTypeList;
            TAOByUTMap taoUTByQuarterStr;
            TAOByIdMap taoUTById;
            LagPrivateTByUTList lagPrivateTByUTList;
            LagPrivateTByUTMap ptivateLagsByUnitTypeId;

        };
    }
}
