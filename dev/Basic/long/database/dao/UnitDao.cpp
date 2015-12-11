//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   BuildingDao.cpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 * 
 * Created on May 7, 2013, 3:59 PM
 */

#include "UnitDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

UnitDao::UnitDao(DB_Connection& connection): SqlAbstractDao<Unit>(connection, DB_TABLE_UNIT,DB_INSERT_UNIT, DB_UPDATE_UNIT, DB_DELETE_UNIT,DB_GETALL_UNIT, DB_GETBYID_UNIT) {}

UnitDao::~UnitDao() {}

void UnitDao::fromRow(Row& result, Unit& outObj)
{
    outObj.id  = result.get<BigSerial>("fm_unit_id", INVALID_ID);
    outObj.building_id  = result.get<BigSerial>("fm_building_id", INVALID_ID);
    outObj.sla_address_id  = result.get<BigSerial>("sla_address_id", INVALID_ID);
    outObj.unit_type  = result.get<int>("unit_type", INVALID_ID);
    outObj.storey_range  = result.get<int>("storey_range", 0);
    outObj.constructionStatus  = result.get<int>("construction_status", 0);
    outObj.floor_area  = result.get<double>("floor_area", .0);
    outObj.storey  = result.get<int>("storey", 0);
    outObj.monthlyRent  = result.get<double>("monthly_rent", .0);
    outObj.ownership = result.get<int>("ownership", 0);
    outObj.sale_from_date  = result.get<std::tm>("sale_from_date", std::tm());
    outObj.physical_from_date  = result.get<std::tm>("physical_from_date", std::tm());
    outObj.sale_status  = result.get<int>("sale_status", 0);
    outObj.occupancyStatus  = result.get<int>("occupancy_status", 0);
    outObj.lastChangedDate = result.get<std::tm>("last_changed_date", std::tm());
    outObj.totalPrice = result.get<double>("total_price", .0);
    outObj.valueDate = result.get<std::tm>("value_date", std::tm());
    outObj.tenureStatus = result.get<int>("tenure_status", 0);
}

void UnitDao::toRow(Unit& data, Parameters& outParams, bool update)
{
	outParams.push_back(data.getId());
	outParams.push_back(data.getBuildingId());
	outParams.push_back(data.getSlaAddressId());
	outParams.push_back(data.getUnitType());
	outParams.push_back(data.getStoreyRange());
	outParams.push_back(data.getConstructionStatus());
	outParams.push_back(data.getFloorArea());
	outParams.push_back(data.getStorey());
	outParams.push_back(data.getMonthlyRent());
	outParams.push_back(data.getOwnership());
	outParams.push_back(data.getSaleFromDate());
	outParams.push_back(data.getPhysicalFromDate());
	outParams.push_back(data.getSaleStatus());
	outParams.push_back(data.getOccupancyStatus());
	outParams.push_back(data.getLastChangedDate());
	outParams.push_back(data.getTotalPrice());
	outParams.push_back(data.getValueDate());
	outParams.push_back(data.getTenureStatus());

}

BigSerial UnitDao::getMaxUnitId()
{
	const std::string queryStr = DB_FUNC_GETALL_UNIT_WITH_MAX_ID;
	std::vector<Unit*> unitWithMaxId;
	getByQuery(queryStr,unitWithMaxId);
	return unitWithMaxId.at(0)->getId();
}

void UnitDao::insertUnit(Unit& unit,std::string schema)
{

	const std::string DB_INSERT_UNIT_OP = "INSERT INTO " + APPLY_SCHEMA(schema, ".fm_unit_res")
                		+ " (" + "fm_unit_id" + ", " + "fm_building_id" + ", " + "sla_address_id"
                		+ ", " + "unit_type" + ", " + "storey_range" + ", "
                		+ "construction_status" + ", " + "floor_area"  + ", "+ "storey" + ", " + "monthly_rent" + ", "
                		+ "ownership" + ", " + "sale_from_date" + ", " + "physical_from_date"  + ", " + "sale_status"
                		+ ", "+ "occupancy_status"  + ", " + "last_changed_date" + ", " + "total_price"+ ", " + "value_date" + ", " + "tenure_status"
                		+ ") VALUES (:v1, :v2, :v3, :v4, :v5, :v6, :v7, :v8, :v9, :v10, :v11, :v12, :v13, :v14, :v15, :v16, :v17, :v18)";
	insertViaQuery(unit,DB_INSERT_UNIT_OP);

}
