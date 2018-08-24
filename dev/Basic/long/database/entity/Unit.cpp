//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   Property.cpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 * 		   Chetan Rogbeer <rogbeer@mit.edu>
 * 
 * Created on March 11, 2013, 3:05 PM
 */

#include "Unit.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

using namespace sim_mob;
using namespace sim_mob::long_term;

Unit::Unit( BigSerial id, BigSerial building_id, int unit_type, int storey_range, int constructionStatus, double floor_area, int storey,
			double monthlyRent, std::tm sale_from_date, std::tm occupancyFromDate, int sale_status, int occupancyStatus, std::tm lastChangedDate,
			double totalPrice,std::tm valueDate,int tenureStatus,int biddingMarketEntryDay, int timeOnMarket, int timeOffMarket, double askingPrice,double lagCoefficient, int zoneHousingType,
			int dwellingType, bool existInDB, bool isBTO, double _btoPrice,int remainingTimeOnMarket, int remainingTimeOffMarket,bool unitByDevModel,BigSerial tazIdByDevModel)
		   : id(id), building_id(building_id), unit_type(unit_type), storey_range(storey_range), constructionStatus(constructionStatus),
		     floor_area(floor_area), storey(storey), monthlyRent(monthlyRent), sale_from_date(sale_from_date), occupancyFromDate(occupancyFromDate),
			 sale_status(sale_status), occupancyStatus(occupancyStatus), lastChangedDate(lastChangedDate),totalPrice(totalPrice),valueDate(valueDate),tenureStatus(tenureStatus),
			 biddingMarketEntryDay(biddingMarketEntryDay),timeOnMarket(timeOnMarket), timeOffMarket(timeOffMarket), askingPrice(0),lagCoefficient(lagCoefficient),
			 zoneHousingType(zoneHousingType), dwellingType(dwellingType), existInDB(existInDB),isBTO(isBTO),btoPrice(_btoPrice),remainingTimeOnMarket(remainingTimeOnMarket),remainingTimeOffMarket(remainingTimeOffMarket),
			 unitByDevModel(false),tazIdByDevModel(INVALID_ID){}


Unit::Unit(const Unit& source)
{
    this->id  = source.id;
    this->building_id  = source.building_id;
    this->unit_type  = source.unit_type;
    this->storey_range  = source.storey_range;
    this->constructionStatus  = source.constructionStatus;
    this->floor_area  = source.floor_area;
    this->storey  = source.storey;
    this->monthlyRent  = source.monthlyRent;
    this->sale_from_date  = source.sale_from_date;
    this->occupancyFromDate  = source.occupancyFromDate;
    this->sale_status  = source.sale_status;
    this->occupancyStatus = source.occupancyStatus;
    this->lastChangedDate = source.lastChangedDate;
    this->totalPrice = source.totalPrice;
    this->valueDate = source.valueDate;
    this->tenureStatus = source.tenureStatus;
    this->biddingMarketEntryDay = source.biddingMarketEntryDay;
    this->timeOnMarket = source.timeOnMarket;
    this->timeOffMarket = source.timeOffMarket;
    this->lagCoefficient = source.lagCoefficient;
    this->zoneHousingType = source.zoneHousingType;
    this->dwellingType = source.dwellingType;
    this->existInDB = source.existInDB;
    this->isBTO = source.isBTO;
    this->btoPrice = source.btoPrice;
    this->askingPrice = source.askingPrice;
    this->remainingTimeOnMarket = source.remainingTimeOnMarket;
    this->remainingTimeOffMarket = source.remainingTimeOffMarket;
    this->unitByDevModel = source.unitByDevModel;
    this->tazIdByDevModel = source.tazIdByDevModel;
}

Unit::~Unit() {}

Unit& Unit::operator=(const Unit& source)
{
    this->id  = source.id;
    this->building_id  = source.building_id;
    this->unit_type  = source.unit_type;
    this->storey_range  = source.storey_range;
    this->constructionStatus  = source.constructionStatus;
    this->floor_area  = source.floor_area;
    this->storey  = source.storey;
    this->monthlyRent  = source.monthlyRent;
    this->sale_from_date  = source.sale_from_date;
    this->occupancyFromDate  = source.occupancyFromDate;
    this->sale_status  = source.sale_status;
    this->occupancyStatus = source.occupancyStatus;
    this->lastChangedDate = source.lastChangedDate;
    this->totalPrice = source.totalPrice;
    this->valueDate = source.valueDate;
    this->tenureStatus = source.tenureStatus;
    this->biddingMarketEntryDay = source.biddingMarketEntryDay;
    this->timeOnMarket = source.timeOnMarket;
    this->timeOffMarket = source.timeOffMarket;
    this->lagCoefficient = source.lagCoefficient;
    this->zoneHousingType = source.zoneHousingType;
    this->dwellingType = source.dwellingType;
    this->existInDB = source.existInDB;
    this->isBTO = source.isBTO;
    this->btoPrice = source.btoPrice;
    this->askingPrice = source.askingPrice;
    this->remainingTimeOnMarket = source.remainingTimeOnMarket;
    this->remainingTimeOffMarket = source.remainingTimeOffMarket;
    this->unitByDevModel = source.unitByDevModel;
    this->tazIdByDevModel = source.tazIdByDevModel;

    return *this;
}

template<class Archive>
void Unit::serialize(Archive & ar,const unsigned int version)
{
	ar & id;
	ar & building_id;
	ar & unit_type;
	ar & storey_range;
	ar & constructionStatus;
	ar & floor_area;
	ar & storey;
	ar & monthlyRent;

	ar & BOOST_SERIALIZATION_NVP(sale_from_date.tm_year);
	ar & BOOST_SERIALIZATION_NVP(sale_from_date.tm_mon);
	ar & BOOST_SERIALIZATION_NVP(sale_from_date.tm_mday);
	sale_from_date.tm_year = sale_from_date.tm_year+1900;

	ar & BOOST_SERIALIZATION_NVP(occupancyFromDate.tm_year);
	ar & BOOST_SERIALIZATION_NVP(occupancyFromDate.tm_mon);
	ar & BOOST_SERIALIZATION_NVP(occupancyFromDate.tm_mday);
	occupancyFromDate.tm_year = occupancyFromDate.tm_year+1900;

	ar & sale_status;
	ar & occupancyStatus;

	ar & BOOST_SERIALIZATION_NVP(lastChangedDate.tm_year);
	ar & BOOST_SERIALIZATION_NVP(lastChangedDate.tm_mon);
	ar & BOOST_SERIALIZATION_NVP(lastChangedDate.tm_mday);
	lastChangedDate.tm_year = lastChangedDate.tm_year+1900;

	ar & totalPrice;
	ar & btoPrice;

	ar & BOOST_SERIALIZATION_NVP(valueDate.tm_year);
	ar & BOOST_SERIALIZATION_NVP(valueDate.tm_mon);
	ar & BOOST_SERIALIZATION_NVP(valueDate.tm_mday);
	valueDate.tm_year = valueDate.tm_year+1900;

	ar & tenureStatus;
	ar & biddingMarketEntryDay;
	ar & timeOnMarket;
	ar & timeOffMarket;
	ar & lagCoefficient;
	ar & zoneHousingType;
	ar & dwellingType;
	ar & existInDB;
	ar & isBTO;
	ar & askingPrice;
	ar & remainingTimeOnMarket;
	ar & remainingTimeOffMarket;


}

void Unit::saveData(std::vector<Unit*> &units)
{
	// make an archive
	std::ofstream ofs(filename);
	boost::archive::binary_oarchive oa(ofs);
	oa & units;

}

std::vector<Unit*> Unit::loadSerializedData()
{
	std::vector<Unit*> units;
	// Restore from saved data and print to verify contents
	std::vector<Unit*> restored_info;
	{
		// Create and input archive
		std::ifstream ifs(filename);
		boost::archive::binary_iarchive ar( ifs );

		// Load the data
		ar & restored_info;
	}

	for (auto *itr :restored_info)
	{
		Unit *unit = itr;
		units.push_back(unit);
	}

	return units;
}

BigSerial Unit::getId() const
{
    return id;
}

BigSerial Unit::getBuildingId() const
{
    return building_id;
}


int Unit::getUnitType() const
{
    return unit_type;
}

int Unit::getStoreyRange() const
{
	return storey_range;
}

int Unit::getConstructionStatus() const
{
	return constructionStatus;
}

double Unit::getFloorArea() const
{
    return floor_area;
}

int Unit::getStorey() const
{
    return storey;
}

double Unit::getMonthlyRent() const
{
		return monthlyRent;
}

std::tm Unit::getSaleFromDate() const
{
	return sale_from_date;
}

std::tm Unit::getOccupancyFromDate() const
{
	return occupancyFromDate;
}

int Unit::getOccupancyFromYear() const
{
	return occupancyFromDate.tm_year;
}


int Unit::getSaleStatus() const
{
	return sale_status;
}

std::tm Unit::getLastChangedDate() const
{
	return lastChangedDate;
}

int Unit::getDwellingType() const
{
	return dwellingType;
}

void Unit::setBuildingId(BigSerial buildingId) {
	this->building_id = buildingId;
}

void Unit::setFloorArea(double floorArea) {
	this->floor_area = floorArea;
}

void Unit::setId(BigSerial id) {
	this->id = id;
}

void Unit::setOccupancyFromDate(const std::tm& occupancyFromDate) {
	this->occupancyFromDate = occupancyFromDate;
}

void Unit::setMonthlyRent(double monthlyRent)
{
		this->monthlyRent = monthlyRent;
}

void Unit::setSaleFromDate(const std::tm& saleFromDate) {
	this->sale_from_date = saleFromDate;
}

void Unit::setSaleStatus(int saleStatus) {
	this->sale_status = saleStatus;
}

void Unit::setStorey(int storey) {
	this->storey = storey;
}

void Unit::setStoreyRange(int storeyRange) {
	this->storey_range = storeyRange;
}

void Unit::setConstructionStatus(int unitStatus) {
	this->constructionStatus = unitStatus;
}

void Unit::setUnitType(int unitType) {
	this->unit_type = unitType;
}

void Unit::setLastChangedDate(std::tm date)
{
	this->lastChangedDate = date;
}

int Unit::getOccupancyStatus() const
{
		return occupancyStatus;
}

void Unit::setOccupancyStatus(int occStatus)
{
		this->occupancyStatus = occStatus;
}

int Unit::getTenureStatus() const
{
		return tenureStatus;
}

void Unit::setTenureStatus(int tenureStatus)
{
		this->tenureStatus = tenureStatus;
}

double Unit::getTotalPrice() const
{
		return totalPrice;
}

void Unit::setTotalPrice(double totalPrice)
{
		this->totalPrice = totalPrice;
}

const std::tm& Unit::getValueDate() const
{
		return valueDate;
}

void Unit::setValueDate(const std::tm& valueDate)
{
		this->valueDate = valueDate;
}

int Unit::getbiddingMarketEntryDay() const
{
	return biddingMarketEntryDay;
}

void Unit::setbiddingMarketEntryDay( int day )
{
	biddingMarketEntryDay = day;
}

int  Unit::getTimeOnMarket() const
{
	return timeOnMarket;
}

void Unit::setTimeOnMarket(int day )
{
	timeOnMarket = day;
}

int  Unit::getTimeOffMarket() const
{
	return timeOffMarket;
}

void Unit::setTimeOffMarket(int day )
{
	timeOffMarket = day;
}

void Unit::setLagCoefficient(double lag)
{
	lagCoefficient = lag;
}
double Unit::getLagCoefficient() const
{
	return lagCoefficient;
}

int Unit::getZoneHousingType() const
{
	return zoneHousingType;
}

void Unit::setZoneHousingType(int value)
{
	zoneHousingType = value;
}

void Unit::setDwellingType( int value)
{
	dwellingType = value;
}

bool  Unit::isBto() const
{
	return isBTO;
}

void  Unit::setBto(bool bto)

{
	isBTO = bto;

}

bool Unit::isExistInDb() const
{
	return existInDB;
}

void Unit::setExistInDb(bool existInDb)
{
	existInDB = existInDb;
}

double Unit::getBTOPrice() const
{
	return btoPrice;
}

void Unit::setBTOPrice(double p)
{
	btoPrice = p;
}

double Unit::getAskingPrice() const
{
	return askingPrice;
}

void Unit::setAskingPrice(double askingPrice)
{
	this->askingPrice = askingPrice;
}

int Unit::getRemainingTimeOffMarket() const
{
	return remainingTimeOffMarket;
}

void Unit::setRemainingTimeOffMarket(int remainingTimeOffMarket)
{
	this->remainingTimeOffMarket = remainingTimeOffMarket;
}

int Unit::getRemainingTimeOnMarket() const
{
	return remainingTimeOnMarket;
}

void Unit::setRemainingTimeOnMarket(int remainingTimeOnMarket)
{
	this->remainingTimeOnMarket = remainingTimeOnMarket;
}

BigSerial Unit::getTazIdByDevModel() const {
	return tazIdByDevModel;
}

void Unit::setTazIdByDevModel(BigSerial tazIdByDevModel) {
	this->tazIdByDevModel = tazIdByDevModel;
}

bool Unit::isUnitByDevModel() const {
	return unitByDevModel;
}

void Unit::setUnitByDevModel(bool unitByDevModel) {
	this->unitByDevModel = unitByDevModel;
}

namespace sim_mob
{
    namespace long_term
    {
        std::ostream& operator<<(std::ostream& strm, const Unit& data)
        {
            return strm << "{"
						<< "\"id \":\"" << data.id << "\","
						<< "\"building_id \":\"" << data.building_id << "\","
						<< "\"unit_type \":\"" << data.unit_type << "\","
						<< "\"storey_range \":\"" << data.storey_range << "\","
						<< "\"unit_status \":\"" << data.constructionStatus << "\","
						<< "\"floor_area \":\"" << data.floor_area << "\","
						<< "\"storey \":\"" << data.storey << "\","
						<< "\"monthlyRent \":\"" << data.monthlyRent<< "\","
						<< "\"sale_from_date \":\"" << data.sale_from_date.tm_year << data.sale_from_date.tm_mon << data.sale_from_date.tm_wday << "\","
						<< "\"occupancy_from_date \":\"" << data.occupancyFromDate.tm_year << data.occupancyFromDate.tm_mon << data.occupancyFromDate.tm_wday << "\","
						<< "\"sale_status \":\"" << data.sale_status << "\","
						<< "\"occupancy_status \":\"" << data.occupancyStatus << "\","
						<< "\"biddingMarketEntryDay\":\"" << data.biddingMarketEntryDay << "\""
						<< "\"timeOnMarket\":\"" << data.timeOnMarket << "\""
            			<< "\"timeOffMarket\":\"" << data.timeOffMarket << "\""
						<< "\"lastChangedDate\":\"" << data.lastChangedDate.tm_year << data.lastChangedDate.tm_mon << data.lastChangedDate.tm_wday << "\","
						<< "\"lagCoefficient\":\"" << data.lagCoefficient << "\","
						<< "\"zoneHousingType\":\"" << data.zoneHousingType << "\","
						<< "\"dwellingType\":\"" << data.dwellingType << "\""
						<< "}";
        }
    }
}
