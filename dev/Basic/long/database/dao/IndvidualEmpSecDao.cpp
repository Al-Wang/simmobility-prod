/*
 * IndvidualEmpSecDao.cpp
 *
 *  Created on: 11 Jul 2016
 *      Author: gishara
 */

#include "IndvidualEmpSecDao.hpp"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;

IndvidualEmpSecDao::IndvidualEmpSecDao(DB_Connection& connection)
: SqlAbstractDao<IndvidualEmpSec>(connection, EMPTY_STR, EMPTY_STR, EMPTY_STR, EMPTY_STR, "SELECT * FROM " + connection.getSchema()+"getIndividualIndustryTypeIds()", EMPTY_STR)
{}

IndvidualEmpSecDao::~IndvidualEmpSecDao() {
}

void IndvidualEmpSecDao::fromRow(Row& result, IndvidualEmpSec& outObj)
{
    outObj.indvidualId = result.get<BigSerial>("id", INVALID_ID);
    outObj.empSecId = result.get<int>("sector_id",INVALID_ID);
}

void IndvidualEmpSecDao::toRow(IndvidualEmpSec& data, Parameters& outParams, bool update) {}




