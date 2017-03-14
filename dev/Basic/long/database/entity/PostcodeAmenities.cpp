//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/* 
 * File:   PostcodeAmenities.cpp
 * Author: Pedro Gandola <pedrogandola@smart.mit.edu>
 * 
 * Created on Feb 11, 2014, 3:05 PM
 */

#include "PostcodeAmenities.hpp"

using namespace sim_mob;
using namespace sim_mob::long_term;

PostcodeAmenities::PostcodeAmenities(): postcode(EMPTY_STR), buildingName(EMPTY_STR), unitBlock(EMPTY_STR), roadName(EMPTY_STR), mtzNumber(EMPTY_STR),
										mrtStation(EMPTY_STR), distanceToMRT(0), distanceToBus(0), distanceToExpress(0), distanceToPMS30(0),
										distanceToCBD(0), distanceToMall(0), distanceToJob(0), mrt_200m(false), mrt_400m(false), express_200m(false),
										bus_200m(false), bus_400m(false), pms_1km(false),addressId(0),tazId(0) {}

PostcodeAmenities::~PostcodeAmenities() {
}

bool PostcodeAmenities::hasPms_1km() const {
    return pms_1km;
}

bool PostcodeAmenities::hasBus_400m() const {
    return bus_400m;
}

bool PostcodeAmenities::hasBus_200m() const {
    return bus_200m;
}

bool PostcodeAmenities::hasExpress_200m() const {
    return express_200m;
}

bool PostcodeAmenities::hasMRT_400m() const {
    return mrt_400m;
}

bool PostcodeAmenities::hasMRT_200m() const {
    return mrt_200m;
}

double PostcodeAmenities::getDistanceToJob() const {
    return distanceToJob;
}

double PostcodeAmenities::getDistanceToMall() const {
    return distanceToMall;
}

double PostcodeAmenities::getDistanceToCBD() const {
    return distanceToCBD;
}

double PostcodeAmenities::getDistanceToPMS30() const {
    return distanceToPMS30;
}

double PostcodeAmenities::getDistanceToExpress() const {
    return distanceToExpress;
}

double PostcodeAmenities::getDistanceToBus() const {
    return distanceToBus;
}

double PostcodeAmenities::getDistanceToMRT() const {
    return distanceToMRT;
}

const std::string& PostcodeAmenities::getMrtStation() const {
    return mrtStation;
}

const std::string& PostcodeAmenities::getMtzNumber() const {
    return mtzNumber;
}

const std::string& PostcodeAmenities::getRoadName() const {
    return roadName;
}

const std::string& PostcodeAmenities::getUnitBlock() const {
    return unitBlock;
}

const std::string& PostcodeAmenities::getBuildingName() const {
    return buildingName;
}

const std::string& PostcodeAmenities::getPostcode() const {
    return postcode;
}

BigSerial PostcodeAmenities::getTazId() const {
    return tazId;
}

BigSerial PostcodeAmenities::getAddressId() const {
    return addressId;
}

void PostcodeAmenities::setDistanceToJob(double val)
{
	distanceToJob = val;
}

void PostcodeAmenities::setDistanceToMall(double val)
{
	distanceToMall = val;
}

void PostcodeAmenities::setDistanceToCBD(double val)
{
	distanceToCBD = val;
}

void PostcodeAmenities::setDistanceToPMS30(double val)
{
	distanceToPMS30 = val;
}

void PostcodeAmenities::setDistanceToExpress(double val)
{
	distanceToExpress = val;
}

void PostcodeAmenities::setDistanceToBus(double val)
{
	distanceToBus = val;
}

void PostcodeAmenities::setDistanceToMRT(double val)
{
	distanceToMRT = val;
}


namespace sim_mob {
    namespace long_term {

        std::ostream& operator<<(std::ostream& strm, const PostcodeAmenities& data) {
            return strm << "{"
                    << "\"postcode\":\"" << data.postcode << "\","
                    << "\"buildingName\":\"" << data.buildingName << "\","
                    << "\"unitBlock\":\"" << data.unitBlock << "\","
                    << "\"roadName\":\"" << data.roadName << "\","
                    << "\"mtzNumber\":\"" << data.mtzNumber << "\","
                    << "\"mrtStation\":\"" << data.mrtStation << "\","
                    << "\"distanceMrt\":\"" << data.distanceToMRT << "\","
                    << "\"distanceBus\":\"" << data.distanceToBus << "\","
                    << "\"distanceExpress\":\"" << data.distanceToExpress << "\","
                    << "\"distancePms30\":\"" << data.distanceToPMS30 << "\","
                    << "\"distanceCBD\":\"" << data.distanceToCBD << "\","
                    << "\"distanceMall\":\"" << data.distanceToMall << "\","
                    << "\"distanceJob\":\"" << data.distanceToJob << "\","
                    << "\"mrt_200m\":\"" << data.mrt_200m << "\","
                    << "\"mrt_400m\":\"" << data.mrt_400m << "\","
                    << "\"express_200m\":\"" << data.express_200m << "\","
                    << "\"bus_200m\":\"" << data.bus_200m << "\","
                    << "\"bus_400m\":\"" << data.bus_400m << "\","
                    << "\"pms_1km\":\"" << data.pms_1km << "\""
                    << "}";
        }
    }
}
