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

UnitDao::UnitDao(DB_Connection& connection): SqlAbstractDao<Unit>(connection, "","", "", "","SELECT * FROM " + connection.getSchema()+"fm_unit_res", "") {}

UnitDao::~UnitDao() {}

void UnitDao::fromRow(Row& result, Unit& outObj)
{
    outObj.id  = result.get<BigSerial>("fm_unit_id", INVALID_ID);
    outObj.building_id  = result.get<BigSerial>("fm_building_id", INVALID_ID);
    outObj.unit_type  = result.get<int>("unit_type", INVALID_ID);
    outObj.storey_range  = result.get<int>("storey_range", 0);
    outObj.constructionStatus  = result.get<int>("construction_status", 0);
    outObj.floor_area  = result.get<double>("floor_area", .0);
    outObj.storey  = result.get<int>("storey", 0);
    outObj.monthlyRent  = result.get<double>("monthly_rent", .0);
    outObj.sale_from_date  = result.get<std::tm>("sale_from_date", std::tm());
    outObj.occupancyFromDate  = result.get<std::tm>("occupancy_from_date", std::tm());
    outObj.sale_status  = result.get<int>("sale_status", 0);
    outObj.occupancyStatus  = result.get<int>("occupancy_status", 0);
    outObj.lastChangedDate = result.get<std::tm>("last_changed_date", std::tm());
    outObj.totalPrice = result.get<double>("total_price", .0);
    outObj.valueDate = result.get<std::tm>("value_date", std::tm());
    outObj.tenureStatus = result.get<int>("tenure_status", 0);
    outObj.isBTO = false;
    outObj.btoPrice = result.get<double>("bto_price", 0);
    outObj.timeOnMarket = result.get<int>("time_on_market", 0);
    outObj.timeOffMarket = result.get<int>("time_off_market", 0);
    outObj.biddingMarketEntryDay = result.get<int>("bidding_market_entry_day", 0);
    outObj.askingPrice = result.get<double>("asking_price", 0);
}

void UnitDao::toRow(Unit& data, Parameters& outParams, bool update)
{
	outParams.push_back(data.getId());
	outParams.push_back(data.getBuildingId());
	outParams.push_back(data.getUnitType());
	outParams.push_back(data.getStoreyRange());
	outParams.push_back(data.getConstructionStatus());
	outParams.push_back(data.getFloorArea());
	outParams.push_back(data.getStorey());
	outParams.push_back(data.getMonthlyRent());
	outParams.push_back(data.getSaleFromDate());
	outParams.push_back(data.getOccupancyFromDate());
	outParams.push_back(data.getSaleStatus());
	outParams.push_back(data.getOccupancyStatus());
	outParams.push_back(data.getLastChangedDate());
	outParams.push_back(data.getTotalPrice());
	outParams.push_back(data.getBTOPrice());
	outParams.push_back(data.getValueDate());
	outParams.push_back(data.getTenureStatus());
	outParams.push_back(data.getTimeOnMarket());
	outParams.push_back(data.getTimeOffMarket());
	outParams.push_back(data.getbiddingMarketEntryDay());
	outParams.push_back(data.getAskingPrice());
}

void UnitDao::insertUnit(Unit& unit,std::string schema)
{
	if(unit.isExistInDb())
	{

		db::Parameters outParams;
		outParams.push_back(unit.getId());
		outParams.push_back(unit.getBuildingId());
		outParams.push_back(unit.getUnitType());
		outParams.push_back(unit.getStoreyRange());
		outParams.push_back(unit.getConstructionStatus());
		outParams.push_back(unit.getFloorArea());
		outParams.push_back(unit.getStorey());
		outParams.push_back(unit.getMonthlyRent());
		outParams.push_back(unit.getSaleFromDate());
		outParams.push_back(unit.getOccupancyFromDate());
		outParams.push_back(unit.getSaleStatus());
		outParams.push_back(unit.getOccupancyStatus());
		outParams.push_back(unit.getLastChangedDate());
		outParams.push_back(unit.getTotalPrice());
		outParams.push_back(unit.getBTOPrice());
		outParams.push_back(unit.getValueDate());
		outParams.push_back(unit.getTenureStatus());
		outParams.push_back(unit.getTimeOnMarket());
		outParams.push_back(unit.getTimeOffMarket());
		outParams.push_back(unit.getbiddingMarketEntryDay());
		outParams.push_back(unit.getAskingPrice());

		const std::string DB_UPDATE_UNIT = "UPDATE " + schema + "fm_unit_res" + " SET "
				+ "fm_building_id" + "= :v1, "
				+ "unit_type"  + "= :v2, "
				+ "storey_range" + "= :v3, "
				+ "construction_status" + "= :v45, "
				+ "floor_area" + "= :v5, "
				+ "storey" + "= :v6, "
				+ "monthly_rent" + "= :v7, "
				+ "sale_from_date" + "= :v8, "
				+ "occupancy_from_date" + "= :v9, "
				+ "sale_status" + "= :v10, "
				+ "occupancy_status" + "= :v11, "
				+ "last_changed_date" + "= :v12, "
				+ "total_price" + "= :v13, "
				+ "bto_price" + "= :v14, "
				+ "value_date" + "= :v15, "
				+ "tenure_status" + "= :v16, "
				+ "time_on_market" + "= :v17, "
				+ "time_off_market" + "= :v18, "
				+ "bidding_market_entry_day" + "= :v19, "
				+ "asking_price"
				+ "= :v20 WHERE "
				+ "fm_unit_id" + "=:v21";
		executeQueryWithParams(unit,DB_UPDATE_UNIT,outParams);
	}

	else
	{

		const std::string DB_INSERT_UNIT_OP = "INSERT INTO "  + schema + ".fm_unit_res"
                				+ " (" + "fm_unit_id" + ", " + "fm_building_id"
								+ ", " + "unit_type" + ", "+ "storey_range" + ", "
								+ "construction_status" + ", " + "floor_area"  + ", "+ "storey" + ", " + "monthly_rent" + ", "
								+ "sale_from_date" + ", " + "occupancy_from_date"  + ", " + "sale_status"
								+ ", "+ "occupancy_status"  + ", "  + "last_changed_date" + ", " + "total_price"+ ", " + "bto_price"+ ", " + "value_date" + ", " + "tenure_status"
								 + ", " + "time_on_market"  + ", " + "time_off_market"  + ", " + "bidding_market_entry_day"  + ", " + "asking_price"
								+ ") VALUES (:v1, :v2, :v3, :v4, :v5, :v6, :v7, :v8, :v9, :v10, :v11, :v12, :v13, :v14, :v15, :v16, :v17, :v18, :v19, :v20, :v21)";
		insertViaQuery(unit,DB_INSERT_UNIT_OP);
		}

}

std::vector<Unit*> UnitDao::getUnitsByBuildingId(const long long buildingId,std::string schema)
{
	const std::string DB_GET_UNITS_BY_BUILDINGID      = "SELECT * FROM " + schema + ".fm_unit_res" + " WHERE fm_buildingl_id = :v1;";
	db::Parameters params;
	params.push_back(buildingId);
	std::vector<Unit*> unitList;
	getByQueryId(DB_GET_UNITS_BY_BUILDINGID,params,unitList);
	return unitList;

}

std::vector<Unit*> UnitDao::getBTOUnits(std::tm currentSimYear)
{
	const std::string DB_GETALL_BTO_UNITS = "SELECT * FROM " + connection.getSchema() + "fm_unit_res" + " WHERE  sale_from_date > :v1";
	db::Parameters params;
	params.push_back(currentSimYear);
	std::vector<Unit*> BTOUnitList;
	getByQueryId(DB_GETALL_BTO_UNITS,params,BTOUnitList);
	return BTOUnitList;
}

std::vector<Unit*> UnitDao::getOngoingBTOUnits(std::tm currentSimYear)
{
	const std::string DB_GETALL_ONGOING_BTO_UNITS = "SELECT * FROM " + connection.getSchema() + "fm_unit_res" + " WHERE  sale_from_date < :v1  and occupancy_from_date > :v2;";
	db::Parameters params;
	params.push_back(currentSimYear);
	params.push_back(currentSimYear);
	std::vector<Unit*> OngoingBTOUnitList;
	getByQueryId(DB_GETALL_ONGOING_BTO_UNITS,params, OngoingBTOUnitList);
	return OngoingBTOUnitList;
}

std::vector<Unit*> UnitDao::loadUnitsToLaunchOnDay0(std::tm currentSimYear,std::tm lastDayOfCurrentSimYear,BigSerial fmParcelId)
{
	const std::string DB_GETALL_UNITS_TO_LOAD_ON_DAY0 = "SELECT U.* FROM " + connection.getSchema() + "fm_unit_res" + " U, " + connection.getSchema() + "fm_building" + " B, " +
			 connection.getSchema() + "fm_parcel" + " P" +
			" WHERE  U.construction_status = 2 and U.unit_type >=7 and U.unit_type <=36  and U.sale_from_date  >= :v1 and sale_from_date < :v2 and U.fm_building_id = B.fm_building_id and B.fm_parcel_id = P.fm_parcel_id and P.fm_parcel_id = :v3;";
	db::Parameters params;
	params.push_back(currentSimYear);
	params.push_back(lastDayOfCurrentSimYear);
	params.push_back(fmParcelId);
	std::vector<Unit*> day0UnitList;
	getByQueryId(DB_GETALL_UNITS_TO_LOAD_ON_DAY0,params,day0UnitList);
	return day0UnitList;
}
