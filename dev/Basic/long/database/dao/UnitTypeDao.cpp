/*
 * unitTypeDao.cpp
 *
 *  Created on: Nov 24, 2014
 *      Author: gishara
 */

#include "UnitTypeDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

UnitTypeDao::UnitTypeDao(DB_Connection& connection): SqlAbstractDao<UnitType>(connection, DB_TABLE_UNIT_TYPE,EMPTY_STR, EMPTY_STR, EMPTY_STR,DB_GETALL_UNIT_TYPES, EMPTY_STR) {}

UnitTypeDao::~UnitTypeDao() {}

void UnitTypeDao::fromRow(Row& result, UnitType& outObj)
{
    outObj.id  = result.get<BigSerial>("id", INVALID_ID);
    outObj.name  = result.get<std::string>("name", EMPTY_STR);
    outObj.typicalArea  = result.get<double>("typical_area", 0.0);
    outObj.constructionCostPerUnit  = result.get<double>("construction_cost_per_unit", INVALID_ID);
    outObj.demolitionCostPerUnit = result.get<double>("demolition_cost_per_unit", INVALID_ID);
    outObj.minLosize = result.get<double>("min_lot_size", INVALID_ID);

}

void UnitTypeDao::toRow(UnitType& data, Parameters& outParams, bool update) {}





