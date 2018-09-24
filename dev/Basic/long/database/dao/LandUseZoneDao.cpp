//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   LandUseZoneDao.cpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 * 
 * Created on Mar 21, 2014, 5:17 PM
 */

#include "LandUseZoneDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

LandUseZoneDao::LandUseZoneDao(DB_Connection& connection): SqlAbstractDao<LandUseZone>( connection, "","", "", "",
                                                                                        "SELECT * FROM " + connection.getSchema()+"land_use_zone", "")
{}

LandUseZoneDao::~LandUseZoneDao() {}

void LandUseZoneDao::fromRow(Row& result, LandUseZone& outObj)
{
    outObj.id = result.get<BigSerial>(DB_FIELD_ID, INVALID_ID);
    outObj.typeId = result.get<BigSerial>(DB_FIELD_TYPE_ID, INVALID_ID);
    outObj.gpr = result.get<std::string>(DB_FIELD_GPR, EMPTY_STR);
}

void LandUseZoneDao::toRow(LandUseZone& data, Parameters& outParams, bool update) {}
