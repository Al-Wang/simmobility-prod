/*
 * IndvidualVehicleOwnershipLogsumDao.cpp
 *
 *  Created on: Jan 20, 2016
 *      Author: gishara
 */
#include "IndvidualVehicleOwnershipLogsumDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

IndvidualVehicleOwnershipLogsumDao::IndvidualVehicleOwnershipLogsumDao(DB_Connection& connection): SqlAbstractDao<IndvidualVehicleOwnershipLogsum>( connection, "", "", "", "","SELECT * FROM " + connection.getSchema()+"individual_level_vehicle_ownership_logsum", "") {}

IndvidualVehicleOwnershipLogsumDao::~IndvidualVehicleOwnershipLogsumDao() {}

void IndvidualVehicleOwnershipLogsumDao::fromRow(Row& result, IndvidualVehicleOwnershipLogsum& outObj)
{
    outObj.householdId 		= result.get<BigSerial>("household_id",INVALID_ID);
    outObj.individualId 		= result.get<BigSerial>("individual_id",0.0);
    outObj.logsumTransit = result.get<double>("logsum_transit",0.0);
    outObj.logsumCar = result.get<double>("logsum_car",0.0);
}

void IndvidualVehicleOwnershipLogsumDao::toRow(IndvidualVehicleOwnershipLogsum& data, Parameters& outParams, bool update) {}

std::vector<IndvidualVehicleOwnershipLogsum*> IndvidualVehicleOwnershipLogsumDao::getIndvidualVehicleOwnershipLogsumsByHHId(const long long householdId)
{
	const std::string queryStr = "SELECT * FROM individual_level_vehicle_ownership_logsum WHERE household_id = :v1;";
	db::Parameters params;
	params.push_back(householdId);
	std::vector<IndvidualVehicleOwnershipLogsum*> IndividualList;
	getByQueryId(queryStr,params,IndividualList);
	return IndividualList;
}


