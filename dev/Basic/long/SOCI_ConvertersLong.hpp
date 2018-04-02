/*
 * SOCI_ConvertersLong.hpp
 *
 *  Created on: 29 Aug 2016
 *      Author: gishara
 */

//Copyright (c) 2016 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <database/entity/JobsByIndustryTypeByTaz.hpp>
#include <database/entity/WorkersGrpByLogsumParams.hpp>
#include <map>
#include <soci/soci.h>
#include <string>
#include "database/entity/HedonicCoeffs.hpp"
#include "database/entity/LagPrivateT.hpp"
#include "database/entity/LtVersion.hpp"
#include "database/entity/HedonicLogsums.hpp"
#include "database/entity/TAOByUnitType.hpp"
#include "database/entity/StudyArea.hpp"
#include "database/entity/JobsWithIndustryTypeAndTazId.hpp"
#include "database/entity/ResidentialWTP_Coefs.hpp"

using namespace sim_mob;
using namespace long_term;
namespace soci
{

template<>
struct type_conversion<sim_mob::long_term::HedonicCoeffs>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::HedonicCoeffs& hedonicCoeffs)
    {
    	hedonicCoeffs.setPropertyTypeId(values.get<BigSerial>("property_type_id", INVALID_ID));
    	hedonicCoeffs.setIntercept(values.get<double>("intercept", 0));
    	hedonicCoeffs.setLogSqrtArea(values.get<double>("log_area", 0));
    	hedonicCoeffs.setFreehold( values.get<double>("freehold", 0));
    	hedonicCoeffs.setLogsumWeighted(values.get<double>("logsum_weighted", 0));
    	hedonicCoeffs.setPms1km(values.get<double>("pms_1km", 0));
    	hedonicCoeffs.setDistanceMallKm(values.get<double>("distance_mall_km", 0));
    	hedonicCoeffs.setMrt200m(values.get<double>("mrt_200m", 0));
    	hedonicCoeffs.setMrt_2_400m(values.get<double>("mrt_2_400m", 0));
    	hedonicCoeffs.setExpress200m(values.get<double>("express_200m", 0));
    	hedonicCoeffs.setBus400m(values.get<double>("bus2_400m", 0));
    	hedonicCoeffs.setBusGt400m(values.get<double>("bus_gt400m", 0));
    	hedonicCoeffs.setAge(values.get<double>("age", 0));
    	hedonicCoeffs.setLogAgeSquared(values.get<double>("age_squared", 0));
    	hedonicCoeffs.setMisage(values.get<double>("misage", 0));
    	hedonicCoeffs.setStorey(values.get<double>("storey", 0));
    	hedonicCoeffs.setStoreySquared(values.get<double>("storey_squared", 0));

    }
};

template<>
struct type_conversion<sim_mob::long_term::HedonicCoeffsByUnitType>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::HedonicCoeffsByUnitType& hedonicCoeffsByUT)
    {
    	hedonicCoeffsByUT.setUnitTypeId(values.get<BigSerial>("unit_type_id", INVALID_ID));
    	hedonicCoeffsByUT.setIntercept(values.get<double>("intercept", 0));
    	hedonicCoeffsByUT.setLogArea(values.get<double>("log_area", 0));
    	hedonicCoeffsByUT.setFreehold( values.get<double>("freehold", 0));
    	hedonicCoeffsByUT.setLogsumWeighted(values.get<double>("logsum_weighted", 0));
    	hedonicCoeffsByUT.setPms1km(values.get<double>("pms_1km", 0));
    	hedonicCoeffsByUT.setDistanceMallKm(values.get<double>("distance_mall_km", 0));
    	hedonicCoeffsByUT.setMrt200m(values.get<double>("mrt_200m", 0));
    	hedonicCoeffsByUT.setMrt2400m(values.get<double>("mrt_2_400m", 0));
    	hedonicCoeffsByUT.setExpress200m(values.get<double>("express_200m", 0));
    	hedonicCoeffsByUT.setBus2400m(values.get<double>("bus2_400m", 0));
    	hedonicCoeffsByUT.setBusGt400m(values.get<double>("bus_gt400m", 0));
    	hedonicCoeffsByUT.setAge(values.get<double>("age", 0));
    	hedonicCoeffsByUT.setAgeSquared(values.get<double>("age_squared", 0));
    	hedonicCoeffsByUT.setMisage(values.get<double>("misage", 0));
    	hedonicCoeffsByUT.setNonMature(values.get<double>("non_mature", 0));
    	hedonicCoeffsByUT.setOtherMature(values.get<double>("other_mature", 0));
    	hedonicCoeffsByUT.setStorey(values.get<double>("storey", 0));
    	hedonicCoeffsByUT.setStoreySquared(values.get<double>("storey_squared", 0));

    }
};

template<>
struct type_conversion<sim_mob::long_term::LagPrivateT>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::LagPrivateT& lagPrivateT)
    {
    	lagPrivateT.setPropertyTypeId(values.get<BigSerial>("property_type_id", INVALID_ID));
    	lagPrivateT.setIntercept(values.get<double>("intercept", 0));
    	lagPrivateT.setT4(values.get<double>("t4", 0));

    }
};


template<>
struct type_conversion<sim_mob::long_term::LtVersion>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::LtVersion& ltVersion)
    {
    	ltVersion.setId(values.get<long long>("id", INVALID_ID));
    	ltVersion.setBase_version(values.get<string>("base_version", ""));
    	ltVersion.setChange_date(values.get<std::tm>("change_date", tm()));
    	ltVersion.setComments(values.get<string>("comments", ""));
    	ltVersion.setUser_id(values.get<string>("userid", ""));
    }
};

template<>
struct type_conversion<sim_mob::long_term::HedonicLogsums>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::HedonicLogsums& hedonicLogsums)
    {
    	hedonicLogsums.setTazId(values.get<long long>("taz_id", INVALID_ID));
    	hedonicLogsums.setLogsumWeighted(values.get<double>("logsum_weighted", 0));
    }
};



template<>
struct type_conversion<sim_mob::long_term::WorkersGrpByLogsumParams>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::WorkersGrpByLogsumParams& workersGrpByLogsumParams)
    {
    	workersGrpByLogsumParams.setIndividualId(values.get<long long>("id", INVALID_ID));
    	workersGrpByLogsumParams.setLogsumCharacteristicsGroupId(values.get<int>("rowid", 0));
    }
};

template<>
struct type_conversion<sim_mob::long_term::BuildingMatch>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::BuildingMatch& buildingMatch)
    {
    	buildingMatch.setFm_building(values.get<long long>("fm_building_id", INVALID_ID));
    	buildingMatch.setFm_building_id_2008(values.get<long long>("fm_building_id_2008", INVALID_ID));
    	buildingMatch.setSla_building_id(values.get<string>("sla_building_id", ""));
    	buildingMatch.setSla_inc_cnc(values.get<string>("sla_inc_cnc",""));
    	buildingMatch.setMatch_code(values.get<int>("match_code",0));
    	buildingMatch.setMatch_date(values.get<tm>("match_date",tm()));
    }
};


template<>
struct type_conversion<sim_mob::long_term::SlaBuilding>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::SlaBuilding& slaBuilding)
    {
    	slaBuilding.setSla_address_id(values.get<long long>("sla_address_id",0));
    	slaBuilding.setSla_building_id(values.get<string>("sla_building_id",""));
    	slaBuilding.setSla_inc_crc(values.get<string>("sla_inc_crc",""));
    }
};

template<>
struct type_conversion<sim_mob::long_term::TAOByUnitType>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::TAOByUnitType& taoByUT)
    {
    	taoByUT.setId(values.get<BigSerial>("id",0));
    	taoByUT.setQuarter(values.get<std::string>("quarter",std::string()));
    	taoByUT.setTreasuryBillYield1Year(values.get<double>("treasury_bill_yield_1year",0));
    	taoByUT.setGdpRate(values.get<double>("gdp_rate",0));
    	taoByUT.setInflation(values.get<double>("inflation",0));
    	taoByUT.setTApartment7(values.get<double>("tapt_7",0));
    	taoByUT.setTApartment8(values.get<double>("tapt_8",0));
    	taoByUT.setTApartment9(values.get<double>("tapt_9",0));
    	taoByUT.setTApartment10(values.get<double>("tapt_10",0));
    	taoByUT.setTApartment11(values.get<double>("tapt_11",0));
    	taoByUT.setTCondo12(values.get<double>("tcondo_12",0));
    	taoByUT.setTCondo13(values.get<double>("tcondo_13",0));
    	taoByUT.setTCondo14(values.get<double>("tcondo_14",0));
    	taoByUT.setTCondo15(values.get<double>("tcondo_15",0));
    	taoByUT.setTCondo16(values.get<double>("tcondo_16",0));
    }
};

template<>
struct type_conversion<sim_mob::long_term::LagPrivate_TByUnitType>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::LagPrivate_TByUnitType& lagPvtTByUT)
    {
    	lagPvtTByUT.setUnitTypeId(values.get<BigSerial>("unit_type_id",0));
    	lagPvtTByUT.setIntercept(values.get<double>("intercept",0));
    	lagPvtTByUT.setT4(values.get<double>("t4",0));
    	lagPvtTByUT.setT5(values.get<double>("t5",0));
    	lagPvtTByUT.setT6(values.get<double>("t6",0));
    	lagPvtTByUT.setT7(values.get<double>("t7",0));
    	lagPvtTByUT.setGdpRate(values.get<double>("gdp_rate",0));
    }
};

template<>
struct type_conversion<sim_mob::long_term::StudyArea>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::StudyArea& studyArea)
    {
    	studyArea.setId(values.get<BigSerial>("id",0));
    	studyArea.setFmTazId(values.get<BigSerial>("fm_taz_id",0));
    	studyArea.setStudyCode(values.get<std::string>("study_code",std::string()));
    }
};

template<>
struct type_conversion<sim_mob::long_term::JobAssignmentCoeffs>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::JobAssignmentCoeffs& jobAssignmentCoeff)
    {
    	jobAssignmentCoeff.setId(values.get<int>("id",0));
    	jobAssignmentCoeff.setBetaInc1(values.get<double>("beta_inc1",0));
    	jobAssignmentCoeff.setBetaInc2(values.get<double>("beta_inc2",0));
    	jobAssignmentCoeff.setBetaInc3(values.get<double>("beta_inc3",0));
    	jobAssignmentCoeff.setBetaLgs(values.get<double>("beta_lgs",0));
    	jobAssignmentCoeff.setBetaS1(values.get<double>("beta_s1",0));
    	jobAssignmentCoeff.setBetaS2(values.get<double>("beta_s2",0));
    	jobAssignmentCoeff.setBetaS3(values.get<double>("beta_s3",0));
    	jobAssignmentCoeff.setBetaS4(values.get<double>("beta_s4",0));
    	jobAssignmentCoeff.setBetaS5(values.get<double>("beta_s5",0));
    	jobAssignmentCoeff.setBetaS6(values.get<double>("beta_s6",0));
    	jobAssignmentCoeff.setBetaS7(values.get<double>("beta_s7",0));
    	jobAssignmentCoeff.setBetaS8(values.get<double>("beta_s8",0));
    	jobAssignmentCoeff.setBetaS9(values.get<double>("beta_s9",0));
    	jobAssignmentCoeff.setBetaS10(values.get<double>("beta_s10",0));
    	jobAssignmentCoeff.setBetaS11(values.get<double>("beta_s11",0));
    	jobAssignmentCoeff.setBetaS98(values.get<double>("beta_s98",0));
    	jobAssignmentCoeff.setBetaLnJob(values.get<double>("beta_lnjob",0));
    }
};

template<>
struct type_conversion<sim_mob::long_term::JobsByIndustryTypeByTaz>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::JobsByIndustryTypeByTaz& jobsByIndustryByTaz)
    {
    	jobsByIndustryByTaz.setTazId(values.get<BigSerial>("taz_id",0));
    	jobsByIndustryByTaz.setIndustryType1(values.get<int>("industry1",0));
    	jobsByIndustryByTaz.setIndustryType2(values.get<int>("industry2",0));
    	jobsByIndustryByTaz.setIndustryType3(values.get<int>("industry3",0));
    	jobsByIndustryByTaz.setIndustryType4(values.get<int>("industry4",0));
    	jobsByIndustryByTaz.setIndustryType5(values.get<int>("industry5",0));
    	jobsByIndustryByTaz.setIndustryType6(values.get<int>("industry6",0));
    	jobsByIndustryByTaz.setIndustryType7(values.get<int>("industry7",0));
    	jobsByIndustryByTaz.setIndustryType8(values.get<int>("industry8",0));
    	jobsByIndustryByTaz.setIndustryType9(values.get<int>("industry9",0));
    	jobsByIndustryByTaz.setIndustryType10(values.get<int>("industry10",0));
    	jobsByIndustryByTaz.setIndustryType11(values.get<int>("industry11",0));
    	jobsByIndustryByTaz.setIndustryType98(values.get<int>("industry98",0));
    }
};

template<>
struct type_conversion<sim_mob::long_term::IndLogsumJobAssignment>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::IndLogsumJobAssignment& indLogsumJobAssignment)
    {
    	indLogsumJobAssignment.setIndividualId(values.get<BigSerial>("individual_id",0));
    	indLogsumJobAssignment.setTazId(values.get<std::string>("taz_id",0));
    	indLogsumJobAssignment.setLogsum(values.get<float>("logsum",.0));

    }
};

template<>
struct type_conversion<sim_mob::long_term::JobsWithIndustryTypeAndTazId>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::JobsWithIndustryTypeAndTazId& jobsWithIndustryTypeAndTazId)
    {
    	jobsWithIndustryTypeAndTazId.setJobId(values.get<BigSerial>("job_id",0));
    	jobsWithIndustryTypeAndTazId.setIndustryTypeId(values.get<int>("industry_type_id",0));
    	jobsWithIndustryTypeAndTazId.setTazId(values.get<BigSerial>("taz_id",0));

    }
};

template<>
struct type_conversion<sim_mob::long_term::School>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::School& school)
    {
    	school.setId(values.get<BigSerial>("id",0));
    	school.setFmBuildingId(values.get<BigSerial>("fm_building_id",0));
    	school.setFloorArea(values.get<double>("floor_area",0));
    	school.setSchoolSlot(values.get<int>("school_slot",0));
    	school.setCentroidX(values.get<double>("centroid_x",0));
    	school.setCentroidY(values.get<double>("centroid_y",0));
    	school.setGiftedProgram(values.get<int>("gifted_program",0));
    	school.setSapProgram(values.get<int>("sap_program",0));
    	school.setPlanningArea(values.get<std::string>("planning_area",std::string()));
    	school.setTazName(values.get<BigSerial>("taz_name",0));
    	school.setSchoolType(values.get<std::string>("school_type",std::string()));
    }
};

template<>
struct type_conversion<sim_mob::long_term::ResidentialWTP_Coefs>
{
    typedef values base_type;

    static void
    from_base(soci::values const & values, soci::indicator & indicator, sim_mob::long_term::ResidentialWTP_Coefs& wtpCoeffs)
    {
    	wtpCoeffs.setId(values.get<int>("id",0));
    	wtpCoeffs.setPropertyType(values.get<std::string>("property_type",std::string()));
    	wtpCoeffs.setSde(values.get<double>("sde",0));
    	wtpCoeffs.setM2(values.get<double>("m2",0));
    	wtpCoeffs.setS2(values.get<double>("s2",0));
    	wtpCoeffs.setConstant(values.get<double>("constant",0));
    	wtpCoeffs.setLogArea(values.get<double>("log_area",0));
    	wtpCoeffs.setLogsumTaz(values.get<double>("logsum_taz",0));
    	wtpCoeffs.setAge(values.get<double>("age",0));
    	wtpCoeffs.setAgeSquared(values.get<double>("age_squared",0));
    	wtpCoeffs.setMissingAgeDummy(values.get<double>("missing_age_dummy",0));
    	wtpCoeffs.setCarDummy(values.get<double>("car_dummy",0));
    	wtpCoeffs.setCarIntoLogsumTaz(values.get<double>("car_into_logsum_taz",0));
    	wtpCoeffs.setDistanceMall(values.get<double>("distance_mall",0));
    	wtpCoeffs.setMrt200m400m(values.get<double>("mrt_200_400m_dummy",0));
    	wtpCoeffs.setMatureDummy(values.get<double>("mature_dummy",0));
    	wtpCoeffs.setMatureOtherDummy(values.get<double>("mature_other_dummy",0));
    	wtpCoeffs.setFloorNumber(values.get<double>("floor_number",0));
    	wtpCoeffs.setLogIncome(values.get<double>("log_income",0));
    	wtpCoeffs.setLogIncomeIntoLogArea(values.get<double>("log_income_into_log_area",0));
    	wtpCoeffs.setFreeholdApartment(values.get<double>("freehold_apartment",0));
    	wtpCoeffs.setFreeholdCondo(values.get<double>("freehold_condo",0));
    	wtpCoeffs.setFreeholdTerrace(values.get<double>("freehold_terrace",0));
    	wtpCoeffs.setFreeholdDetached(values.get<double>("freehold_detached",0));
    	wtpCoeffs.setBus200m400mDummy(values.get<double>("bus_200_400m_dummy",0));
    	wtpCoeffs.setOneTwoFullTimeWorkerDummy(values.get<double>("one_two_fulltime_worker_dummy",0));;
    	wtpCoeffs.setFullTimeWorkersTwoIntoLogArea(values.get<double>("fulltime_workers2_into_log_area",0));
    	wtpCoeffs.setHhSizeworkersDiff(values.get<double>("hh_size_wrokers_diff",0));


    }
};

} //namespace soci

