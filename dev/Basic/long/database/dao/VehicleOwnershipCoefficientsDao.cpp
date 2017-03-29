/*
 * VehicleOwnershipCoefficientsDao.cpp
 *
 *  Created on: Feb 24, 2015
 *      Author: gishara
 */
#include "VehicleOwnershipCoefficientsDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

VehicleOwnershipCoefficientsDao::VehicleOwnershipCoefficientsDao(DB_Connection& connection): SqlAbstractDao<VehicleOwnershipCoefficients>( connection, DB_TABLE_VEHICLE_OWNERSHIP_COEFFICIENTS, EMPTY_STR, EMPTY_STR, EMPTY_STR,DB_GETALL_VEHCILE_OWNERSHIP_COEFFICIENTS, EMPTY_STR ) {}

VehicleOwnershipCoefficientsDao::~VehicleOwnershipCoefficientsDao() {}

void VehicleOwnershipCoefficientsDao::fromRow(Row& result, VehicleOwnershipCoefficients& outObj)
{
    outObj.vehicleOwnershipOptionId 		= result.get<BigSerial>("vehicle_ownership_option_id",INVALID_ID);
    outObj.incomeAdj 		= result.get<double>("income_adj",0.0);
    outObj.malay		= result.get<double>("malay",0.0);
    outObj.indian		= result.get<double>("indian",0.0);
    outObj.otherRaces		= result.get<double>("other_races",0.0);
    outObj.aboveSixty		= result.get<double>("above_60",0.0);
    outObj.whiteCollar		= result.get<double>("white_collar",0.0);
    outObj.worker		= result.get<double>("worker",0.0);
    outObj.HHChild1		= result.get<double>("hh_child_1",0.0);
    outObj.HHChild2Plus		= result.get<double>("hh_child_2plus",0.0);
    outObj.taxi		= result.get<double>("taxi",0.0);
    outObj.mrt500m		= result.get<double>("mrt_500m",0.0);
    outObj.mrt1000m		= result.get<double>("mrt_1000m",0.0);
    outObj.privateProperty		= result.get<double>("private_property",0.0);
    outObj.logsum		= result.get<double>("logsum",0.0);
    outObj.constant		= result.get<double>("constant",0.0);
}

void VehicleOwnershipCoefficientsDao::toRow(VehicleOwnershipCoefficients& data, Parameters& outParams, bool update) {}



