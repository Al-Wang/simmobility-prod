//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//license.txt   (http://opensource.org/licenses/MIT)

/*
 * AlternativeDao.cpp
 *
 *  Created on: 31 Jul, 2015
 *  Author: Chetan Rogbeer <chetan.rogbeer@smart.mit.edu>
 *
 */
#include "AlternativeDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

AlternativeDao::AlternativeDao(DB_Connection& connection): SqlAbstractDao<Alternative>( connection, "", "", "", "", "SELECT * FROM " + connection.getSchema()+"alternative", ""){}

AlternativeDao::~AlternativeDao(){}

void AlternativeDao::fromRow(Row& result, Alternative& outObj)
{
    outObj.id               = result.get<BigSerial>( "id", INVALID_ID);
    outObj.planAreaId       = result.get<BigSerial>( "plan_area_id", 0);
    outObj.planAreaName     = result.get<std::string>( "plan_area_name", "");
    outObj.dwellingTypeId   = result.get<BigSerial>( "dwelling_type_id", 0);
    outObj.dwellingTypeName = result.get<std::string>( "dwelling_type_name", "");
    outObj.medianHedonicPrice   = result.get<double>( "median_hedonic_price", .0);
    outObj.mapId            = result.get<BigSerial>("map_id", 0);
}

void AlternativeDao::toRow(Alternative& data, Parameters& outParams, bool update) {}


