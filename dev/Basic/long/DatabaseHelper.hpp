//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   DatabaseHelper.h
 * Author: gandola
 *         chetan rogbeer <chetan.rogbeer@smart.mit.edu>
 *
 * Created on April 24, 2013, 12:14 PM
 */

#pragma once
#include <string>

namespace sim_mob {

    namespace long_term {

#define APPLY_SCHEMA(schema, field) std::string(schema)+std::string(field)

        /**
         * Schemas
         */
        const std::string DB_EMPTY_QUERY = "";
        const std::string DB_SCHEMA_EMPTY = "";
        const std::string MAIN_SCHEMA = "main2012.";
        const std::string CALIBRATION_SCHEMA = "calibration2012.";
        const std::string OUTPUT_SCHEMA = "output2012.";
        const std::string LIMIT_10000 = " order by random() limit 10000";
        const std::string LIMIT_RAND0_DOT1 = "WHERE random() < 0.01";
        const std::string LIMIT_ALL = "";
        const std::string LIMIT = LIMIT_ALL;

        /**
         * Tables
         */
        const std::string DB_TABLE_HOUSEHOLD = APPLY_SCHEMA(MAIN_SCHEMA, "household");
        const std::string DB_TABLE_BUILDING = APPLY_SCHEMA(MAIN_SCHEMA, "fm_building");
        const std::string DB_TABLE_UNIT = APPLY_SCHEMA(MAIN_SCHEMA, "fm_unit_res");
        const std::string DB_TABLE_DEVELOPER = APPLY_SCHEMA(MAIN_SCHEMA, "developer");
        const std::string DB_TABLE_PARCEL = APPLY_SCHEMA(MAIN_SCHEMA, "fm_parcel");
        const std::string DB_TABLE_TEMPLATE = APPLY_SCHEMA(MAIN_SCHEMA, "template");
        const std::string DB_TABLE_POSTCODE = APPLY_SCHEMA(MAIN_SCHEMA, "sla_addresses");
        const std::string DB_TABLE_POSTCODE_AMENITIES = APPLY_SCHEMA(MAIN_SCHEMA, "postcode_amenities");
        const std::string DB_TABLE_LAND_USE_ZONE = APPLY_SCHEMA(MAIN_SCHEMA, "land_use_zone");
        const std::string DB_TABLE_DEVELOPMENT_TYPE_TEMPLATE = APPLY_SCHEMA(MAIN_SCHEMA, "development_type_template");
        const std::string DB_TABLE_TEMPLATE_UNIT_TYPE = APPLY_SCHEMA(MAIN_SCHEMA, "template_unit_type");
        const std::string DB_TABLE_PROJECT = APPLY_SCHEMA(MAIN_SCHEMA, "fm_project");
        const std::string DB_TABLE_PARCEL_MATCH = APPLY_SCHEMA(MAIN_SCHEMA, "parcel_match");
        const std::string DB_TABLE_SLA_PARCEL = APPLY_SCHEMA(MAIN_SCHEMA, "sla_parcel");
        const std::string DB_TABLE_INDIVIDUAL = APPLY_SCHEMA(MAIN_SCHEMA, "individual");
        const std::string DB_TABLE_RESIDENTIAL_STATUS = APPLY_SCHEMA( MAIN_SCHEMA, "residential_status");
        const std::string DB_TABLE_UNIT_TYPE = APPLY_SCHEMA( MAIN_SCHEMA, "unit_type");
        const std::string DB_TABLE_PARCEL_AMENITIES = APPLY_SCHEMA( MAIN_SCHEMA, "parcel_amenities");
        const std::string DB_TABLE_AWAKENING = APPLY_SCHEMA(CALIBRATION_SCHEMA, "hh_awakening_probability");
        const std::string DB_TABLE_MACRO_ECONOMICS = APPLY_SCHEMA(MAIN_SCHEMA, "macro_economics");
        const std::string DB_TABLE_VEHICLE_OWNERSHIP_COEFFICIENTS = APPLY_SCHEMA(CALIBRATION_SCHEMA, "vehicle_ownership_coefficients");
        const std::string DB_TABLE_TAXI_ACCESS_COEFFICIENTS = APPLY_SCHEMA(CALIBRATION_SCHEMA, "taxi_access_coefficients");
        const std::string DB_TABLE_ESTABLISHMENT= APPLY_SCHEMA(MAIN_SCHEMA, "establishment");
        const std::string DB_TABLE_JOB= APPLY_SCHEMA(MAIN_SCHEMA, "job");
        const std::string DB_TABLE_LOGSUM_FOR_DEVMODEL = APPLY_SCHEMA(CALIBRATION_SCHEMA, "logsum_for_dev_model");
        const std::string DB_TABLE_HIR= APPLY_SCHEMA(MAIN_SCHEMA, "housing_interest_rates");
        const std::string DB_TABLE_TAZ= APPLY_SCHEMA(MAIN_SCHEMA, "taz");
        const std::string DB_TABLE_TAZ_LOGUM_WEIGHT= APPLY_SCHEMA(CALIBRATION_SCHEMA, "taz_logsum_hedonic_price");
        const std::string DB_TABLE_HH_HITS_SAMPLE = APPLY_SCHEMA(MAIN_SCHEMA, "household_hits_sample");
        const std::string DB_TABLE_TAO = APPLY_SCHEMA(CALIBRATION_SCHEMA, "tao_hedonic_price");
        const std::string DB_TABLE_LOGSUMMTZV2 = APPLY_SCHEMA(CALIBRATION_SCHEMA, "logsum_mtz_v2");
        const std::string DB_TABLE_PLANNING_AREA = APPLY_SCHEMA(MAIN_SCHEMA, "planning_area");
        const std::string DB_TABLE_PLANNING_SUBZONE = APPLY_SCHEMA(MAIN_SCHEMA, "planning_subzone");
        const std::string DB_TABLE_MTZ = APPLY_SCHEMA(MAIN_SCHEMA, "mtz");
        const std::string DB_TABLE_MTZ_TAZ= APPLY_SCHEMA(MAIN_SCHEMA, "mtz_taz");
        const std::string DB_TABLE_ALTERNATIVE= APPLY_SCHEMA(CALIBRATION_SCHEMA, "alternative");
        const std::string DB_TABLE_ACCESSIBILITYFIXEDPZID= APPLY_SCHEMA(CALIBRATION_SCHEMA, "accessibility_fixed_pzid");
        const std::string DB_TABLE_HITS2008SCREENINGPROB= APPLY_SCHEMA(CALIBRATION_SCHEMA, "hits2008_screening_prob");
        const std::string DB_TABLE_ZONALLANDUSEVARIABLEVALUES= APPLY_SCHEMA(CALIBRATION_SCHEMA, "zonal_landuse_variable_values");
        const std::string DB_TABLE_HITSINDIVIDUALLOGSUM= APPLY_SCHEMA(MAIN_SCHEMA, "hits_individual_logsum");
        const std::string DB_TABLE_TAZ_LEVEL_LAND_PRICE = APPLY_SCHEMA(CALIBRATION_SCHEMA, "taz_level_land_price");
        const std::string DB_TABLE_SIM_VERSION = APPLY_SCHEMA( OUTPUT_SCHEMA, "simulation_version");
        const std::string DB_TABLE_ENCODED_PARAMS_BY_SIMULATION = APPLY_SCHEMA( OUTPUT_SCHEMA, "encoded_params_by_simulation");
        const std::string DB_TABLE_CREATE_OP_SCHEMA = APPLY_SCHEMA( MAIN_SCHEMA, "create_output_schema");
        const std::string DB_TABLE_SCREENINGCOSTTIME= APPLY_SCHEMA( CALIBRATION_SCHEMA, "cost_time");
        const std::string DB_TABLE_OWNERTENANTMOVINGRATE= APPLY_SCHEMA( CALIBRATION_SCHEMA, "owner_tenant_moving_rate");
        const std::string DB_TABLE_TENURETRANSITIONRATE= APPLY_SCHEMA( CALIBRATION_SCHEMA, "tenure_transition_rate");

        /**
         * Views
         */
        const std::string DB_VIEW_UNIT = APPLY_SCHEMA(MAIN_SCHEMA, "view_unit");

        /**
         * Functions API
         */
        const std::string DB_FUNC_DEL_HOUSEHOLD_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "deleteHouseholdById(:id)");
        const std::string DB_FUNC_GET_HOUSEHOLDS	  = APPLY_SCHEMA(MAIN_SCHEMA, "getHouseholds()");
        const std::string DB_FUNC_GET_HOUSEHOLD_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getHouseholdById(:id)");

        const std::string DB_FUNC_DEL_UNIT_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "deleteUnitById(:id)");
        const std::string DB_FUNC_GET_UNITS 	 = APPLY_SCHEMA(MAIN_SCHEMA, "getUnits()");
        const std::string DB_FUNC_GET_UNIT_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getUnitById(:id)");

        const std::string DB_FUNC_DEL_BUILDING_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "deleteBuildingById(:id)");
        const std::string DB_FUNC_GET_BUILDINGS 	 = APPLY_SCHEMA(MAIN_SCHEMA, "getBuildings()");
        const std::string DB_FUNC_GET_BUILDING_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getBuildingById(:id)");

        const std::string DB_FUNC_GET_DEVELPERS 	  = APPLY_SCHEMA(MAIN_SCHEMA, "getDevelopers()");
        const std::string DB_FUNC_GET_DEVELOPER_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getDeveloperById(:id)");

        const std::string DB_FUNC_GET_PARCELS 	   = APPLY_SCHEMA(MAIN_SCHEMA, "getParcels()");
        const std::string DB_FUNC_GET_PARCEL_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getParcelById(:id)");

        const std::string DB_FUNC_GET_TEMPLATES 	 = APPLY_SCHEMA(MAIN_SCHEMA, "getTemplates()");
        const std::string DB_FUNC_GET_TEMPLATE_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getTemplateById(:id)");

        const std::string DB_FUNC_DEL_POSTCODE_BY_ID = DB_EMPTY_QUERY;
        const std::string DB_FUNC_GET_POSTCODES 	 = APPLY_SCHEMA(MAIN_SCHEMA, "getPostcodes()");
        const std::string DB_FUNC_GET_POSTCODE_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getPostcodeById(:id)");

        const std::string DB_FUNC_DEL_POSTCODE_AMENITIES_BY_ID = DB_EMPTY_QUERY;
        const std::string DB_FUNC_GET_POSTCODES_AMENITIES 	   = APPLY_SCHEMA(MAIN_SCHEMA, "getPostcodeAmenities()");
        const std::string DB_FUNC_GET_POSTCODE_AMENITIES_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getPostcodeAmenitiesById(:id)");

        const std::string DB_FUNC_GET_LAND_USE_ZONES 	  = APPLY_SCHEMA(MAIN_SCHEMA, "getLandUseZones()");
        const std::string DB_FUNC_GET_LAND_USE_ZONE_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getLandUseZoneById(:id)");

        const std::string DB_FUNC_GET_DEVELOPMENT_TYPE_TEMPLATES 	  = APPLY_SCHEMA(MAIN_SCHEMA, "getDevelopmentTypeTemplates()");
        const std::string DB_FUNC_GET_DEVELOPMENT_TYPE_TEMPLATE_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getDevelopmentTypeTemplateById(:devId, :templateId)");

        const std::string DB_FUNC_GET_TEMPLATE_UNIT_TYPES 	   = APPLY_SCHEMA(MAIN_SCHEMA, "getTemplateUnitTypes()");
        const std::string DB_FUNC_GET_TEMPLATE_UNIT_TYPE_BY_ID = APPLY_SCHEMA(MAIN_SCHEMA, "getTemplateUnitTypeById(:templateId, :unitTypeId)");

        const std::string DB_FUNC_GET_PROJECTS = APPLY_SCHEMA(MAIN_SCHEMA, "getProjects()");
        const std::string DB_FUNC_GET_PARCEL_MATCHES = APPLY_SCHEMA(MAIN_SCHEMA, "getParcelMatches()");
        const std::string DB_FUNC_GET_SLA_PARCELS = APPLY_SCHEMA(MAIN_SCHEMA, "getSlaParcels()");
        const std::string DB_FUNC_GET_AWAKENING = APPLY_SCHEMA( MAIN_SCHEMA, "getHouseholdAwakeningProbability()");
        const std::string DB_FUNC_GET_MACRO_ECONOMICS = APPLY_SCHEMA( MAIN_SCHEMA, "getMacroEconomics()");
        const std::string DB_FUNC_GET_VEHICLE_OWNERSHIP_COEFFICIENTS = APPLY_SCHEMA( MAIN_SCHEMA, "getVehicleOwnershipCoefficients()");
        const std::string DB_FUNC_GET_TAXI_ACCESS_COEFFICIENTS = APPLY_SCHEMA( MAIN_SCHEMA, "getTaxiAccessCoefficients()");
        const std::string DB_FUNC_GET_CAR_OWNERSHIP_LOGSUMS_PER_HH = APPLY_SCHEMA( MAIN_SCHEMA, "getCarOwnershipLogsumsPerHH()");
        const std::string DB_FUNC_GET_DEV_LOGSUMS = APPLY_SCHEMA( MAIN_SCHEMA, "getDevLogsums()");

        const std::string DB_FUNC_DEL_INDIVIDUAL_BY_ID  = APPLY_SCHEMA(MAIN_SCHEMA, "deleteIndividualById(:id)");
        const std::string DB_FUNC_GET_INDIVIDUALS 		= APPLY_SCHEMA( MAIN_SCHEMA, "getIndividuals()");
        const std::string DB_FUNC_GET_INDIVIDUAL_BY_ID = APPLY_SCHEMA( MAIN_SCHEMA, "getIndividualsById(:id)");

        const std::string DB_FUNC_DEL_RESIDENTIAL_STATUS_BY_ID = DB_EMPTY_QUERY;
        const std::string DB_FUNC_GET_RESIDENTIAL_STATUS_BY_ID = APPLY_SCHEMA( MAIN_SCHEMA, "getResidentialStatusById(:id)");
        const std::string DB_FUNC_GET_RESIDENTIAL_STATUS 	   = APPLY_SCHEMA( MAIN_SCHEMA, "getResidentialStatus()");
        const std::string DB_FUNC_GET_AWAKENING_BY_ID = APPLY_SCHEMA( MAIN_SCHEMA, "getAwakeningById(:id)");

        const std::string DB_FUNC_GET_ESTABLISHMENT = APPLY_SCHEMA( MAIN_SCHEMA, "getEstablishments()");
        const std::string DB_FUNC_GET_ESTABLISHMENT_BY_ID = APPLY_SCHEMA( MAIN_SCHEMA, "getEstablishmentById(:id)");

        const std::string DB_FUNC_GET_JOB = APPLY_SCHEMA( MAIN_SCHEMA, "getJobs()");
        const std::string DB_FUNC_GET_JOB_BY_ID = APPLY_SCHEMA( MAIN_SCHEMA, "getJobById(:id)");

        const std::string DB_FUNC_GET_HIR = APPLY_SCHEMA( MAIN_SCHEMA, "getHousingInterestRates()");
        const std::string DB_FUNC_GET_HIR_BY_ID = APPLY_SCHEMA( MAIN_SCHEMA, "getHousingInterestRateById(:id)");

        const std::string DB_FUNC_GET_UNIT_TYPES 	   = APPLY_SCHEMA( MAIN_SCHEMA, "getUnitTypes()");
        const std::string DB_FUNC_GET_EMPTY_PARCELS = APPLY_SCHEMA( MAIN_SCHEMA, "getEmptyParcels()");
        const std::string DB_FUNC_GET_TOTAL_BUILDING_SPACE = APPLY_SCHEMA( MAIN_SCHEMA, "getTotalBuildingSpacePerParcel()");
        //const std::string DB_FUNC_GET_BUILDINGS_OF_PARCEL = APPLY_SCHEMA( MAIN_SCHEMA, "getBuildingsOfParcel(_parcelId BIGINT)");
        const std::string DB_FUNC_GET_PARCEL_AMENITIES 	   = APPLY_SCHEMA( MAIN_SCHEMA, "getParcelAmenities()");
        const std::string DB_FUNC_GET_UNIT_WITH_MAX_ID = APPLY_SCHEMA( OUTPUT_SCHEMA, "getUnitWithMaxId()");

        const std::string DB_FUNC_GET_DIST_MRT = APPLY_SCHEMA( MAIN_SCHEMA, "getdistMrt()");
        const std::string DB_FUNC_GET_PARCELS_WITH_HDB = APPLY_SCHEMA( MAIN_SCHEMA, "getParcelsWithHDB()");
        const std::string DB_FUNC_GET_TAO = APPLY_SCHEMA( MAIN_SCHEMA, "getTAO()");
        const std::string DB_FUNC_GET_UNIT_PRICE_SUM_PER_PARCLE = APPLY_SCHEMA( MAIN_SCHEMA, "getUnitPriceSumPerParcel()");

        const std::string DB_FUNC_GET_POPULATION_PER_PLANNING_AREA = APPLY_SCHEMA( MAIN_SCHEMA, "getPopulationPerPlanningArea()");


        /**
         * Fields
         */

        //NEW DATABASE
        const std::string DB_FIELD_ID = "id";
        const std::string DB_FIELD_UNIT_ID = "fm_unit_id";
        const std::string DB_FIELD_HOUSEHOLD_ID = "household_id";
        const std::string DB_FIELD_PROJECT_ID = "project_id";
        const std::string DB_FIELD_PARCEL_ID = "parcel_id";
        const std::string DB_FIELD_BUILDING_ID = "building_id";
        const std::string DB_FIELD_ESTABLISMENT_ID = "establishment_id";
        const std::string DB_FIELD_TYPE_ID = "type_id";
        const std::string DB_FIELD_POSTCODE_ID = "postcode_id";
        const std::string DB_FIELD_TAZ_ID = "taz_id";
        const std::string DB_FIELD_LIFESTYLE_ID = "lifestyle_id";
        const std::string DB_FIELD_VEHICLE_CATEGORY_ID = "vehicle_category_id";
        const std::string DB_FIELD_ETHNICITY_ID = "ethnicity_id";
        const std::string DB_FIELD_TENURE_ID = "tenure_id";
        const std::string DB_FIELD_INCOME = "income";
        const std::string DB_FIELD_FLOOR_AREA = "floor_area";
        const std::string DB_FIELD_YEAR = "year";
        const std::string DB_FIELD_STOREY = "storey";
        const std::string DB_FIELD_RENT = "rent";
        const std::string DB_FIELD_SIZE = "size";
        const std::string DB_FIELD_CHILDUNDER4 = "child_under4";
        const std::string DB_FIELD_CHILDUNDER15 = "child_under15";
        const std::string DB_FIELD_WORKERS = "workers";
        const std::string DB_FIELD_AGE_OF_HEAD = "age_of_head";
        const std::string DB_FIELD_TAXI_AVAILABILITY = "taxi_availability";
        const std::string DB_FIELD_HOUSING_DURATION = "housing_duration";
        const std::string DB_FIELD_BUILT_YEAR = "built_year";
        const std::string DB_FIELD_STOREYS = "storeys";
        const std::string DB_FIELD_PARKING_SPACES = "parking_spaces";
        const std::string DB_FIELD_RESIDENTIAL_UNITS = "residential_units";
        const std::string DB_FIELD_LANDED_AREA = "landed_area";
        const std::string DB_FIELD_IMPROVEMENT_VALUE = "improvement_value";
        const std::string DB_FIELD_TAX_EXEMPT = "tax_exempt";
        const std::string DB_FIELD_NON_RESIDENTIAL_SQFT = "non_residential_sqft";
        const std::string DB_FIELD_SQFT_PER_UNIT = "sqft_per_unit";
        const std::string DB_FIELD_LATITUDE = "latitude";
        const std::string DB_FIELD_LONGITUDE = "longitude";
        const std::string DB_FIELD_NAME = "name";
        const std::string DB_FIELD_TYPE = "type";
        const std::string DB_FIELD_CODE = "code";
        const std::string DB_FIELD_POSTCODE = "sla_postcode";
        const std::string DB_FIELD_BUILDING_NAME = "building_name";
        const std::string DB_FIELD_ROAD_NAME = "road_name";
        const std::string DB_FIELD_UNIT_BLOCK = "unit_block";
        const std::string DB_FIELD_MTZ_NUMBER = "mtz_number";
        const std::string DB_FIELD_MRT_STATION = "mrt_station";
        const std::string DB_FIELD_DISTANCE_TO_MRT = "distance_mrt";
        const std::string DB_FIELD_DISTANCE_TO_BUS = "distance_bus";
        const std::string DB_FIELD_DISTANCE_TO_PMS30 = "distance_pms30";
        const std::string DB_FIELD_DISTANCE_TO_CBD = "distance_cbd";
        const std::string DB_FIELD_DISTANCE_TO_MALL = "distance_mall";
        const std::string DB_FIELD_DISTANCE_TO_JOB = "accessibility_job";
        const std::string DB_FIELD_DISTANCE_TO_EXPRESS = "distance_express";
        const std::string DB_FIELD_MRT_200M = "mrt_200m";
        const std::string DB_FIELD_MRT_400M = "mrt_400m";
        const std::string DB_FIELD_EXPRESS_200M = "express_200m";
        const std::string DB_FIELD_BUS_200M = "bus_200m";
        const std::string DB_FIELD_BUS_400M = "bus_400m";
        const std::string DB_FIELD_PMS_1KM = "pms_1km";
        const std::string DB_FIELD_LAND_USE_ZONE_ID = "land_use_zone_id";
        const std::string DB_FIELD_AREA = "area";
        const std::string DB_FIELD_LENGTH = "length";
        const std::string DB_FIELD_MIN_X = "min_x";
        const std::string DB_FIELD_MAX_X = "max_x";
        const std::string DB_FIELD_MIN_Y = "min_y";
        const std::string DB_FIELD_MAX_Y = "max_y";
        const std::string DB_FIELD_GPR = "gpr";
        const std::string DB_FIELD_LAND_USE_ID = "land_use_type_id";
        const std::string DB_FIELD_DEVELOPMENT_TYPE_ID = "development_type_id";
        const std::string DB_FIELD_TEMPLATE_ID = "template_id";
        const std::string DB_FIELD_UNIT_TYPE_ID = "unit_type_id";
        const std::string DB_FIELD_PROPORTION = "proportion";
        const std::string DB_FIELD_TYPICAL_AREA = "typical_area";
        const std::string DB_FIELD_CONSTRUCTION_COST_PER_UNIT = "construction_cost_per_unit";


        /**
         * INSERT
         */
        const std::string DB_INSERT_HOUSEHOLD = "INSERT INTO "
												+ DB_TABLE_HOUSEHOLD + " ("
												+ DB_FIELD_ID + ", "
												+ DB_FIELD_UNIT_ID + ", "
												+ DB_FIELD_SIZE + ", "
												+ DB_FIELD_CHILDUNDER4 + ", "
												+ DB_FIELD_CHILDUNDER15 + ", "
												+ DB_FIELD_INCOME + ", "
												+ DB_FIELD_HOUSING_DURATION
												+ ") VALUES (:v1, :v2, :v3, :v4, :v5, :v6, :v7)";

        const std::string DB_INSERT_BUILDING = "INSERT INTO " + APPLY_SCHEMA(OUTPUT_SCHEMA, "fm_building")
        		+ " (" + "fm_building_id" + ", " + "fm_project_id" + ", " + "fm_parcel_id"
        		+ ", " + "storeys_above_ground" + ", " + "storeys_below_ground" + ", "
        		+ "from_date" + ", " + "to_date"  + ", "+ "building_status" + ", " + "gross_sq_m_res" + ", "
        		+ "gross_sq_m_office" + ", " + "gross_sq_m_retail" + ", " + "gross_sq_m_other"  + ", " + "last_changed_date"
        		+ ") VALUES (:v1, :v2, :v3, :v4, :v5, :v6, :v7, :v8, :v9, :v10, :v11, :v12, :v13)";

        const std::string DB_INSERT_UNIT = "INSERT INTO " + APPLY_SCHEMA(OUTPUT_SCHEMA, "fm_unit_res")
        		+ " (" + "fm_unit_id" + ", " + "fm_building_id" + ", " + "sla_address_id"
        		+ ", " + "unit_type" + ", " + "storey_range" + ", "
        		+ "unit_status" + ", " + "floor_area"  + ", "+ "storey" + ", " + "rent" + ", "
        		+ "ownership" + ", " + "sale_from_date" + ", " + "physical_from_date"  + ", " + "sale_status"
        		+ ", "+ "physical_status"  + ", " + "last_changed_date"
        		+ ") VALUES (:v1, :v2, :v3, :v4, :v5, :v6, :v7, :v8, :v9, :v10, :v11, :v12, :v13, :v14, :v15)";

        const std::string DB_INSERT_POSTCODE = DB_EMPTY_QUERY;
        const std::string DB_INSERT_POSTCODE_AMENITIES = DB_EMPTY_QUERY;
        const std::string DB_INSERT_PROJECT = "INSERT INTO " + APPLY_SCHEMA(OUTPUT_SCHEMA, "fm_project")
                		+ " (" + "fm_project_id" + ", " + "fm_parcel_id" + ", " + "developer_id"
                		+ ", " + "template_id" + ", " + "project_name" + ", "
                		+ "construction_date" + ", " + "completion_date"  + ", "+ "construction_cost" + ", " + "demolition_cost" + ", "
                		+ "total_cost" + ", " + "fm_lot_size" + ", " + "gross_ratio"  + ", " + "gross_area"
                		+ ", "+ "planned_date"  + ", " + "project_status"
                		+ ") VALUES (:v1, :v2, :v3, :v4, :v5, :v6, :v7, :v8, :v9, :v10, :v11, :v12, :v13, :v14, :v15)";
        const std::string DB_INSERT_INDIVIDUAL = DB_EMPTY_QUERY;
        const std::string DB_INSERT_RESIDENTIAL_STATUS = DB_EMPTY_QUERY;
        const std::string DB_INSERT_AWAKENING = DB_EMPTY_QUERY;

        const std::string DB_INSERT_PARCEL = "INSERT INTO " + APPLY_SCHEMA(OUTPUT_SCHEMA, "fm_parcel")
								+ " (" + "fm_parcel_id" + ", " + "taz_id" + ", " + "lot_size"
		                		+ ", " + "gpr" + ", " + "land_use_type_id" + ", "
		                		+ "owner_name" + ", " + "owner_category"  + ", "+ "last_transaction_date" + ", " + "last_transaction_type_total" + ", "
		                		+ "psm_per_gps" + ", " + "lease_type" + ", " + "lease_start_date"  + ", " + "centroid_x"
		                		+ ", "+ "centroid_y"  + ", " + "award_date" + ", " + "award_status" + ", " + "use_restriction" + ", " + "development_type_code"  + ", " + "successful_tender_id"
		                		+ ", "+ "successful_tender_price"  + ", " + "tender_closing_date" + ", " + "lease" + ", " + "development_status" + ", " + "development_allowed" + ", " + "next_available_date"  + ", " + "last_changed_date"
		                		+ ") VALUES (:v1, :v2, :v3, :v4, :v5, :v6, :v7, :v8, :v9, :v10, :v11, :v12, :v13, :v14, :v15, :v16, :v17, :v18, :v19, :v20, :v21, :v22, :v23, :v24, :v25, :v26)";

        /**
         * UPDATE
         */
        const std::string DB_UPDATE_HOUSEHOLD = "UPDATE "	+ DB_TABLE_HOUSEHOLD + " SET "
															+ DB_FIELD_UNIT_ID + "= :v1, "
															+ DB_FIELD_SIZE + "= :v2, "
															+ DB_FIELD_CHILDUNDER4 + "= :v3, "
															+ DB_FIELD_CHILDUNDER15 + "= :v4, "
															+ DB_FIELD_INCOME + "= :v5, "
															+ DB_FIELD_HOUSING_DURATION
															+ "= :v6 WHERE "
															+ DB_FIELD_ID + "=:v7";

        const std::string DB_UPDATE_BUILDING = DB_EMPTY_QUERY;
        const std::string DB_UPDATE_UNIT = DB_EMPTY_QUERY;
        const std::string DB_UPDATE_POSTCODE = DB_EMPTY_QUERY;
        const std::string DB_UPDATE_POSTCODE_AMENITIES = DB_EMPTY_QUERY;
        const std::string DB_UPDATE_PROJECT = DB_EMPTY_QUERY;
        const std::string DB_UPDATE_INDIVIDUAL = DB_EMPTY_QUERY;
        const std::string DB_UPDATE_RESIDENTIAL_STATUS = DB_EMPTY_QUERY;
        const std::string DB_UPDATE_AWAKENING = DB_EMPTY_QUERY;

        /**
         * DELETE
         */
        const std::string DB_DELETE_HOUSEHOLD 	= "SELECT * FROM " + DB_FUNC_DEL_HOUSEHOLD_BY_ID;
        const std::string DB_DELETE_BUILDING 	= "SELECT * FROM " + DB_FUNC_DEL_BUILDING_BY_ID;
        const std::string DB_DELETE_UNIT 		= "SELECT * FROM " + DB_FUNC_DEL_UNIT_BY_ID;
        const std::string DB_DELETE_POSTCODE 	= DB_EMPTY_QUERY;
        const std::string DB_DELETE_POSTCODE_AMENITIES = DB_EMPTY_QUERY;
        const std::string DB_DELETE_INDIVIDUAL = "SELECT * FROM " + DB_FUNC_DEL_INDIVIDUAL_BY_ID;
        const std::string DB_DELETE_RESIDENTIAL_STATUS = DB_EMPTY_QUERY;
        const std::string DB_DELETE_PROJECT = DB_EMPTY_QUERY;
        const std::string DB_DELETE_AWAKENING = DB_EMPTY_QUERY;



        /**
         * GET ALL
         */
        const std::string DB_GETALL_HOUSEHOLD 	= "SELECT * FROM " + DB_FUNC_GET_HOUSEHOLDS + LIMIT;
        const std::string DB_GETALL_BUILDING 	= "SELECT * FROM " + DB_FUNC_GET_BUILDINGS  + LIMIT;
        const std::string DB_GETALL_UNIT 		= "SELECT * FROM " + DB_FUNC_GET_UNITS + LIMIT;
        const std::string DB_GETALL_DEVELOPERS 	= "SELECT * FROM " + DB_FUNC_GET_DEVELPERS + LIMIT;
        const std::string DB_GETALL_PARCELS 	= "SELECT * FROM " + DB_FUNC_GET_PARCELS + LIMIT;
        const std::string DB_GETALL_TEMPLATES 	= "SELECT * FROM " + DB_FUNC_GET_TEMPLATES + LIMIT;
        const std::string DB_GETALL_POSTCODE 	= "SELECT * FROM " + DB_FUNC_GET_POSTCODES + LIMIT;
        const std::string DB_GETALL_POSTCODE_AMENITIES 	= "SELECT * FROM " + DB_FUNC_GET_POSTCODES_AMENITIES + LIMIT;
        const std::string DB_GETALL_LAND_USE_ZONES 		= "SELECT * FROM " + DB_FUNC_GET_LAND_USE_ZONES + LIMIT;
        const std::string DB_GETALL_TEMPLATE_UNIT_TYPE 	= "SELECT * FROM " + DB_FUNC_GET_TEMPLATE_UNIT_TYPES + LIMIT;
        const std::string DB_GETALL_DEVELOPMENT_TYPE_TEMPLATES = "SELECT * FROM " + DB_FUNC_GET_DEVELOPMENT_TYPE_TEMPLATES + LIMIT;
        const std::string DB_GETALL_INDIVIDUAL = "SELECT * FROM " + DB_FUNC_GET_INDIVIDUALS + LIMIT;
        const std::string DB_GETALL_RESIDENTIAL_STATUS = "SELECT * FROM " + DB_FUNC_GET_RESIDENTIAL_STATUS + LIMIT;
        const std::string DB_GETALL_PROJECTS = "SELECT * FROM "+ DB_FUNC_GET_PROJECTS + LIMIT;
        const std::string DB_GETALL_PARCEL_MATCHES = "SELECT * FROM "+ DB_FUNC_GET_PARCEL_MATCHES + LIMIT;
        const std::string DB_GETALL_SLA_PARCELS = "SELECT * FROM "+ DB_FUNC_GET_SLA_PARCELS+ LIMIT;
        const std::string DB_GETALL_UNIT_TYPES = "SELECT * FROM "+ DB_FUNC_GET_UNIT_TYPES+ LIMIT;
        const std::string DB_GETALL_EMPTY_PARCELS = "SELECT * FROM "+ DB_FUNC_GET_EMPTY_PARCELS + LIMIT;
        const std::string DB_GETALL_TOTAL_BUILDING_SPACE = "SELECT * FROM "+ DB_FUNC_GET_TOTAL_BUILDING_SPACE + LIMIT;
        const std::string DB_GETALL_PARCEL_AMENITIES = "SELECT * FROM "+ DB_FUNC_GET_PARCEL_AMENITIES + LIMIT;
        const std::string DB_GETALL_AWAKENING = "SELECT * FROM " + DB_FUNC_GET_AWAKENING + LIMIT;
        const std::string DB_GETALL_MACRO_ECONOMICS = "SELECT * FROM " + DB_FUNC_GET_MACRO_ECONOMICS + LIMIT;
        const std::string DB_GETALL_VEHCILE_OWNERSHIP_COEFFICIENTS = "SELECT * FROM " + DB_FUNC_GET_VEHICLE_OWNERSHIP_COEFFICIENTS + LIMIT;
        const std::string DB_FUNC_GETALL_UNIT_WITH_MAX_ID = "SELECT * FROM " + DB_FUNC_GET_UNIT_WITH_MAX_ID + LIMIT;
        const std::string DB_GETALL_TAXI_ACCESS_COEFFICIENTS = "SELECT * FROM " + DB_FUNC_GET_TAXI_ACCESS_COEFFICIENTS + LIMIT;
        const std::string DB_FUNC_GET_ALL_CAR_OWNERSHIP_LOGSUMS_PER_HH = "SELECT * FROM " + DB_FUNC_GET_CAR_OWNERSHIP_LOGSUMS_PER_HH + LIMIT;
        const std::string DB_GETALL_ESTABLISHMENT = "SELECT * FROM " + DB_FUNC_GET_ESTABLISHMENT + LIMIT;
        const std::string DB_GETALL_JOB = "SELECT * FROM " + DB_FUNC_GET_JOB + LIMIT;
        const std::string DB_GETALL_DEV_LOGSUMS = "SELECT * FROM " + DB_FUNC_GET_DEV_LOGSUMS + LIMIT;
        const std::string DB_GETALL_HIR = "SELECT * FROM " + DB_FUNC_GET_HIR + LIMIT;
        const std::string DB_GETALL_DIST_MRT = "SELECT * FROM " + DB_FUNC_GET_DIST_MRT + LIMIT;
        const std::string DB_GETALL_TAZS = "SELECT * FROM " + DB_TABLE_TAZ + LIMIT;
        const std::string DB_GETALL_TAZ_LOGSUM_WEIGHTS = "SELECT * FROM " + DB_TABLE_TAZ_LOGUM_WEIGHT + LIMIT;
        const std::string DB_GETALL_HH_HITS_SAMPLE = "SELECT * FROM " + DB_TABLE_HH_HITS_SAMPLE + LIMIT;
        const std::string DB_GETALL_PARCELS_WITH_HDB = "SELECT * FROM " + DB_FUNC_GET_PARCELS_WITH_HDB + LIMIT;
        const std::string DB_GETALL_TAO = "SELECT * FROM " + DB_FUNC_GET_TAO + LIMIT;
        const std::string DB_GETALL_UNIT_PRICE_SUM_PER_PARCEL = "SELECT * FROM " + DB_FUNC_GET_UNIT_PRICE_SUM_PER_PARCLE + LIMIT;
        const std::string DB_FUNC_GETALL_PARCELS_WITH_HDB = "SELECT * FROM " + DB_FUNC_GET_PARCELS_WITH_HDB + LIMIT;
        const std::string DB_GETALL_LOGSUMMTZV2 = "SELECT * FROM " + DB_TABLE_LOGSUMMTZV2 + LIMIT;
        const std::string DB_GETALL_PLANNING_AREA = "SELECT * FROM " + DB_TABLE_PLANNING_AREA + LIMIT;
        const std::string DB_GETALL_PLANNING_SUBZONE = "SELECT * FROM " + DB_TABLE_PLANNING_SUBZONE + LIMIT;
        const std::string DB_GETALL_MTZ = "SELECT * FROM " + DB_TABLE_MTZ + LIMIT;
        const std::string DB_GETALL_MTZ_TAZ = "SELECT * FROM " + DB_TABLE_MTZ_TAZ + LIMIT;
        const std::string DB_GETALL_ALTERNATIVE = "SELECT * FROM " + DB_TABLE_ALTERNATIVE + LIMIT;
        const std::string DB_GETALL_HITS2008SCREENINGPROB = "SELECT * FROM " + DB_TABLE_HITS2008SCREENINGPROB + LIMIT;
        const std::string DB_GETALL_ZONALLANDUSEVARIABLEVALUES = "SELECT * FROM " + DB_TABLE_ZONALLANDUSEVARIABLEVALUES + LIMIT;
        const std::string DB_GETALL_POPULATION_PER_PLANNING_AREA = "SELECT * FROM " + DB_FUNC_GET_POPULATION_PER_PLANNING_AREA + LIMIT;
        const std::string DB_GETALL_HITSINDIVIDUALLOGSUM = "SELECT * FROM " + DB_TABLE_HITSINDIVIDUALLOGSUM + LIMIT;
        const std::string DB_GETALL_TAZ_LEVEL_LAND_PRICES = "SELECT * FROM " + DB_TABLE_TAZ_LEVEL_LAND_PRICE + LIMIT;
        const std::string DB_GETALL_SIMVERSION = "SELECT * FROM " + DB_TABLE_SIM_VERSION + LIMIT;
      //  const std::string DB_GETALL_STAUS_OF_WORLD = "SELECT * FROM " + DB_TABLE_STATUS_OF_WORLD + LIMIT;
        const std::string DB_GETALL_PARCELS_WITH_ONGOING_PROJECTS = "SELECT * FROM " + APPLY_SCHEMA(OUTPUT_SCHEMA, "fm_parcel") + LIMIT;
        const std::string DB_GETALL_CREATEOPSCHEMA = "SELECT * FROM " + APPLY_SCHEMA(MAIN_SCHEMA, "create_output_schema") + LIMIT;
        const std::string DB_GETALL_ACCESSIBILITYFIXEDPZID = "SELECT * FROM "+ DB_TABLE_ACCESSIBILITYFIXEDPZID + LIMIT;
        const std::string DB_GETALL_SCREENINGCOSTTIME = "SELECT * FROM "+ DB_TABLE_SCREENINGCOSTTIME + LIMIT;
        const std::string DB_GETALL_OWNERTENANTMOVINGRATE = "SELECT * FROM "+ DB_TABLE_OWNERTENANTMOVINGRATE+ LIMIT;
        const std::string DB_GETALL_TENURETRANSITIONRATE = "SELECT * FROM "+ DB_TABLE_TENURETRANSITIONRATE+ LIMIT;

        /**
         * GET BY ID
         */
        const std::string DB_GETBYID_HOUSEHOLD = "SELECT * FROM " + DB_FUNC_GET_HOUSEHOLD_BY_ID;
        const std::string DB_GETBYID_BUILDING  = "SELECT * FROM " + DB_FUNC_GET_BUILDING_BY_ID;
        const std::string DB_GETBYID_UNIT      = "SELECT * FROM " + DB_FUNC_GET_UNIT_BY_ID;
        const std::string DB_GETBYID_DEVELOPER = "SELECT * FROM " + DB_FUNC_GET_DEVELOPER_BY_ID;
        const std::string DB_GETBYID_PARCEL    = "SELECT * FROM " + DB_FUNC_GET_PARCEL_BY_ID;
        const std::string DB_GETBYID_TEMPLATE  = "SELECT * FROM " + DB_FUNC_GET_TEMPLATE_BY_ID;
        const std::string DB_GETBYID_POSTCODE  = "SELECT * FROM " + DB_FUNC_GET_POSTCODE_BY_ID;
        const std::string DB_GETBYID_POSTCODE_AMENITIES = "SELECT * FROM " + DB_FUNC_GET_POSTCODE_AMENITIES_BY_ID;
        const std::string DB_GETBYID_LAND_USE_ZONE = "SELECT * FROM " 	   + DB_FUNC_GET_LAND_USE_ZONE_BY_ID;
        const std::string DB_GETBYID_TEMPLATE_UNIT_TYPE = "SELECT * FROM " + DB_FUNC_GET_TEMPLATE_UNIT_TYPE_BY_ID;
        const std::string DB_GETBYID_DEVELOPMENT_TYPE_TEMPLATE = "SELECT * FROM " + DB_FUNC_GET_DEVELOPMENT_TYPE_TEMPLATE_BY_ID;
        const std::string DB_GETBYID_INDIVIDUAL = "SELECT * FROM " + DB_FUNC_GET_INDIVIDUAL_BY_ID;
        const std::string DB_GETBYID_RESIDENTIAL_STATUS = "SELECT * FROM " + DB_FUNC_GET_RESIDENTIAL_STATUS_BY_ID;
        const std::string DB_GETBYID_AWAKENING = "SELECT * FROM " + DB_FUNC_GET_AWAKENING_BY_ID;
        const std::string DB_GETBYID_ESTABLISHMENT = "SELECT * FROM " + DB_FUNC_GET_ESTABLISHMENT_BY_ID;
        const std::string DB_GETBYID_JOB = "SELECT * FROM " + DB_FUNC_GET_JOB_BY_ID;
        const std::string DB_GETBYID_HIR = "SELECT * FROM " + DB_FUNC_GET_HIR_BY_ID;
        const std::string DB_GETBYID_TAZ = "SELECT * FROM " + DB_TABLE_TAZ + " WHERE id = :v1;";
        const std::string DB_GETBYID_TAZ_LOGSUM_WEIGHT = "SELECT * FROM " + DB_TABLE_TAZ_LOGUM_WEIGHT + " WHERE id = :v1;";
        const std::string DB_GETBYID_HH_HITS_SAMPLE = "SELECT * FROM " + DB_TABLE_HH_HITS_SAMPLE + " WHERE household_id = :v1;";
        const std::string DB_GETBYID_LOGSUMMTZV2 = "SELECT * FROM " + DB_TABLE_LOGSUMMTZV2 + " WHERE taz = :v1;";
        const std::string DB_GETBYID_PLANNING_AREA = "SELECT * FROM " + DB_TABLE_PLANNING_AREA + " WHERE id = :v1;";
        const std::string DB_GETBYID_PLANNING_SUBZONE = "SELECT * FROM " + DB_TABLE_PLANNING_SUBZONE + " WHERE id = :v1;";
        const std::string DB_GETBYID_MTZ = "SELECT * FROM " + DB_TABLE_MTZ + " WHERE id = :v1;";
        const std::string DB_GETBYID_MTZ_TAZ = "SELECT * FROM " + DB_TABLE_MTZ_TAZ + " WHERE id = :v1;";
        const std::string DB_GETBYID_ALTERNATIVE = "SELECT * FROM " + DB_TABLE_ALTERNATIVE + " WHERE id = :v1;";
        const std::string DB_GETBYID_HITS2008SCREENINGPROB = "SELECT * FROM " + DB_TABLE_HITS2008SCREENINGPROB + " WHERE id = :v1;";
        const std::string DB_GETBYID_ZONALLANDUSEVARIABLEVALUES = "SELECT * FROM " + DB_TABLE_ZONALLANDUSEVARIABLEVALUES + " WHERE id = :v1;";
        const std::string DB_GETBYID_POPULATION_PER_PLANNING_AREA = "SELECT * FROM " + DB_FUNC_GET_POPULATION_PER_PLANNING_AREA + " WHERE planning_area_id = :v1;";
        const std::string DB_GETBYID_HITSINDIVIDUALLOGSUM = "SELECT * FROM " + DB_TABLE_HITSINDIVIDUALLOGSUM + " WHERE id = :v1;";
        const std::string DB_GETBUILDINGS_BY_PARCELID      = "SELECT * FROM " + DB_TABLE_BUILDING + " WHERE fm_parcel_id = :v1;";
        //const std::string DB_GETBYID_AWAKENING      = "SELECT * FROM " + DB_TABLE_AWAKENING + " WHERE h1_hhid = :v1;";
        const std::string DB_GETBYID_ACCESSIBILITYFIXEDPZID = "SELECT * FROM "+ DB_TABLE_ACCESSIBILITYFIXEDPZID + " WHERE planning_area_id = :v1;";
        const std::string DB_GETBYID_SCREENINGCOSTTIME = "SELECT * FROM "+ DB_TABLE_SCREENINGCOSTTIME + " WHERE id = :v1;";
        const std::string DB_GETBYID_OWNERTENANTMOVINGRATE = "SELECT * FROM "+ DB_TABLE_OWNERTENANTMOVINGRATE + " WHERE id = :v1;";
        const std::string DB_GETBYID_TENURETRANSITIONRATE = "SELECT * FROM "+ DB_TABLE_TENURETRANSITIONRATE + " WHERE id = :v1;";

    }
}

