/*
 * HouseHoldHitsSampleDao.cpp
 *
 *  Created on: Jun 24, 2015
 *      Author: gishara
 */
#include "HouseHoldHitsSampleDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

HouseHoldHitsSampleDao::HouseHoldHitsSampleDao(DB_Connection& connection)
: SqlAbstractDao<HouseHoldHitsSample>(connection, "", "", "", "", "SELECT * FROM " + connection.getSchema()+"household_hits_sample", "")
{}

HouseHoldHitsSampleDao::~HouseHoldHitsSampleDao() {
}

void HouseHoldHitsSampleDao::fromRow(Row& result, HouseHoldHitsSample& outObj)
{
    outObj.houseHoldHitsId = result.get<std::string>("H1_HHID", "");
    outObj.houseHoldId = result.get<BigSerial>("household_id", INVALID_ID);
    outObj.groupId = result.get<BigSerial>("group_id",INVALID_ID);
}

void HouseHoldHitsSampleDao::toRow(HouseHoldHitsSample& data, Parameters& outParams, bool update) {}





