//Copyright (c) 2014 Singapore-MIT Alliance for Research and Technology
//Licenced under of the terms of the MIT licence, as described in the file:
//licence.txt (www.opensource.org\licences\MIT)

/*
 * IndividualDao.cpp
 *
 *  Created on: 3 Sep, 2014
 *  Author: Chetan Rogbeer <chetan.rogbeer@smart.mit.edU>
 */

#include "IndividualDao.h"
#include "DatabaseHelper.hpp"

using namespace sim_mob::db;
using namespace sim_mob::long_term;


IndividualDao::IndividualDao(DB_Connection& connection): SqlAbstractDao<Individual>(connection, DB_TABLE_INDIVIDUAL, DB_INSERT_INDIVIDUAL, DB_UPDATE_INDIVIDUAL,
																					DB_DELETE_INDIVIDUAL, DB_GETALL_INDIVIDUAL, DB_GETBYID_INDIVIDUAL) {}

IndividualDao::~IndividualDao() {}

void IndividualDao::fromRow(Row& result, Individual& outObj)
{
	outObj.id  					= result.get<BigSerial>(	"id", 					INVALID_ID);
	outObj.individualTypeId		= result.get<BigSerial>(	"individual_type_id", 	INVALID_ID);
	outObj.householdId			= result.get<BigSerial>(	"household_id", 		INVALID_ID);
	outObj.jobId				= result.get<BigSerial>(	"job_id", 				INVALID_ID);
	outObj.ethnicityId			= result.get<BigSerial>(	"ethnicity_id", 		INVALID_ID);
	outObj.employmentStatusId	= result.get<BigSerial>(	"employment_status_id", INVALID_ID);
	outObj.genderId				= result.get<BigSerial>(	"gender_id", 			INVALID_ID);
	outObj.educationId			= result.get<BigSerial>(	"education_id", 		INVALID_ID);
	outObj.occupationId			= result.get<BigSerial>(	"occupation_id", 		INVALID_ID);
	outObj.vehiculeCategoryId	= result.get<BigSerial>(	"vehicule_category_id", INVALID_ID);
	outObj.transitCategoryId	= result.get<BigSerial>(	"transit_category_id", 	INVALID_ID);
	outObj.ageCategoryId		= result.get<BigSerial>(	"age_category_id", 		INVALID_ID);
	outObj.residentialStatusId	= result.get<BigSerial>(	"residential_status_id",INVALID_ID);
	outObj.householdHead		= result.get<int>(			"household_head", 		0);
	outObj.income				= result.get<double>(		"income", 				0.0);
	outObj.memberId				= result.get<int>(			"member_id", 			0);
	outObj.workAtHome			= result.get<int>(			"work_at_home", 		0);
	outObj.driversLicence		= result.get<int>(			"driver_licence", 		0);
	outObj.dateOfBirth			= result.get<std::tm>(		"date_of_birth", 		std::tm());
}

void IndividualDao::toRow(Individual& data, Parameters& outParams, bool update) {}


