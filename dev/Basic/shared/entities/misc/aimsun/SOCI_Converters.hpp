//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

/// \file SOCI_Converters.hpp
///This file contains several Type converters for SOCI object-mapping
/// \author Seth N. Hetu

#include "boost/algorithm/string.hpp"
#include "soci/soci.h"
#include "TripChain.hpp"


using namespace sim_mob::aimsun;
using std::string;


namespace soci
{


template<> struct type_conversion<sim_mob::aimsun::TripChainItem>
{
    typedef values base_type;
    static void from_base(const soci::values& vals, soci::indicator& ind, sim_mob::aimsun::TripChainItem &res)
    {
    	res.personID = vals.get<std::string>("entityid","");
    	res.sequenceNumber = vals.get<int>("trip_chain_sequence_number",0);
    	res.itemType = sim_mob::TripChainItem::getItemType(vals.get<std::string>("trip_chain_item_type",""));
    	if(res.itemType == sim_mob::TripChainItem::IT_TRIP) {
    		res.tripID = vals.get<std::string>("trip_id", "");
    		res.tmp_tripfromLocationNodeID = vals.get<int>("trip_from_location_id",0);
    		res.tripfromLocationType = sim_mob::TripChainItem::LT_NODE; //sim_mob::TripChainItem::getLocationType(vals.get<std::string>("trip_from_location_type",""));
    		res.tmp_triptoLocationNodeID = vals.get<int>("trip_to_location_id",0);
    		res.triptoLocationType = sim_mob::TripChainItem::LT_NODE; //sim_mob::TripChainItem::getLocationType(vals.get<std::string>("trip_to_location_type",""));
    		res.tmp_subTripID = vals.get<std::string>("sub_trip_id","");
    		res.tmp_fromLocationNodeID = vals.get<int>("from_location_id",0);
    		res.fromLocationType =  sim_mob::TripChainItem::LT_NODE; //sim_mob::TripChainItem::getLocationType(vals.get<std::string>("from_location_type",""));
    		res.tmp_toLocationNodeID = vals.get<int>("to_location_id",0);
    		res.toLocationType = sim_mob::TripChainItem::LT_NODE; //sim_mob::TripChainItem::getLocationType(vals.get<std::string>("to_location_type",""));
    		res.mode = vals.get<std::string>("mode","");
    		res.isPrimaryMode = vals.get<int>("primary_mode", 0);
    		res.ptLineId = vals.get<std::string>("pt_line_id","");
    		res.tmp_startTime = vals.get<std::string>("start_time","");
    	}
    	else if(res.itemType == sim_mob::TripChainItem::IT_ACTIVITY) {
    		res.tmp_activityID = vals.get<std::string>("activity_id", "");
    		res.description = vals.get<std::string>("activity_description", "");
    		res.isPrimary = vals.get<int>("primary_activity", 0);
    		res.isFlexible = vals.get<int>("flexible_activity", 0);
    		res.isMandatory = vals.get<int>("mandatory_activity", 0);
    		res.tmp_locationID = vals.get<int>("location_id", 0);
    		res.locationType = sim_mob::TripChainItem::getLocationType(vals.get<std::string>("location_type", ""));
    		res.tmp_startTime = vals.get<std::string>("activity_start_time", "");
    		res.tmp_endTime = vals.get<std::string>("activity_end_time", "");
    	}
    	else {
    		throw std::runtime_error("Couldn't load Trip Chain; unexpected type.");
    	}
    }

    static void to_base(const sim_mob::aimsun::TripChainItem& src, soci::values& vals, soci::indicator& ind)
    {
    	throw std::runtime_error("TripChainItem::to_base() not implemented");
    }
};
}
