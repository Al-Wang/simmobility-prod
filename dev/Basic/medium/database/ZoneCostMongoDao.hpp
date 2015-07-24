//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * ZoneMongoDao.hpp
 *
 *  Created on: Nov 30, 2013
 *      Author: Harish Loganathan
 */

#pragma once
#include <boost/unordered_map.hpp>
#include <vector>
#include "behavioral/params/ZoneCostParams.hpp"
#include "database/dao/MongoDao.hpp"
#include "database/DB_Config.hpp"
#include "logging/Log.hpp"

namespace sim_mob {
namespace medium {
class ZoneMongoDao : db::MongoDao {
public:
	ZoneMongoDao(db::DB_Config& dbConfig, const std::string& database, const std::string& collection);
	virtual ~ZoneMongoDao();

    /**
     * Gets all values from the source and put them on the given list.
     * @param outList to put the retrieved values.
     * @return true if some values were returned, false otherwise.
     */
    bool getAllZones(boost::unordered_map<int, ZoneParams*>& outList) {
        unsigned long long count = connection.getSession<mongo::DBClientConnection>().count(collectionName, mongo::BSONObj());
    	//boost 1.49
        outList.rehash(ceil(count / outList.max_load_factor()));
    	//boost >= 1.50
        //outList.reserve(count);
        std::auto_ptr<mongo::DBClientCursor> cursor = connection.getSession<mongo::DBClientConnection>().query(collectionName, mongo::BSONObj());
    	while(cursor->more()) {
    		ZoneParams* zoneParams = new ZoneParams();
    		fromRow(cursor->next(), *zoneParams);
    		outList[zoneParams->getZoneId()] = zoneParams;
    	}
    	return true;
    }

    /**
     * Converts a given row into a PersonParams type.
     * @param result result row.
     * @param outParam (Out parameter) to receive data from row.
     */
    void fromRow(mongo::BSONObj document, ZoneParams& outParam);
};

class CostMongoDao : db::MongoDao {
public:
	CostMongoDao(db::DB_Config& dbConfig, const std::string& database, const std::string& collection);
	virtual ~CostMongoDao();

    /**
     * Gets all values from the source and put them on the given list.
     * @param outList to put the retrieved values.
     * @return true if some values were returned, false otherwise.
     */
    bool getAll(boost::unordered_map<int, boost::unordered_map<int, CostParams*> >& outList) {
    	std::auto_ptr<mongo::DBClientCursor> cursor = connection.getSession<mongo::DBClientConnection>().query(collectionName, mongo::BSONObj());
    	while(cursor->more()) {
    		CostParams* costParams = new CostParams();
    		fromRow(cursor->next(), *costParams);
    		outList[costParams->getOriginZone()][costParams->getDestinationZone()] = costParams;
    	}
    	return true;
    }

    /**
     * Converts a given row into a PersonParams type.
     * @param result result row.
     * @param outParam (Out parameter) to receive data from row.
     */
    void fromRow(mongo::BSONObj document, CostParams& outParam);
};

class ZoneNodeMappingDao : db::MongoDao {
public:
	ZoneNodeMappingDao(db::DB_Config& dbConfig, const std::string& database, const std::string& collection);
	virtual ~ZoneNodeMappingDao();

    /**
     * Gets all values from the source and put them on the given list.
     * @param outList to put the retrieved values.
     * @return true if some values were returned, false otherwise.
     */
    bool getAll(boost::unordered_map<int, std::vector<ZoneNodeParams*> >& outList);

    /**
     * Converts a given row into a ZoneNodeParams type.
     * @param result result row.
     * @param outParam (Out parameter) to receive data from row.
     */
    void fromRow(mongo::BSONObj document, ZoneNodeParams& outParam);
};

class MTZ12_MTZ08_MappingDao : db::MongoDao {
public:
	MTZ12_MTZ08_MappingDao(db::DB_Config& dbConfig, const std::string& database, const std::string& collection);
	virtual ~MTZ12_MTZ08_MappingDao();

    /**
     * Gets all values from the source and put them on the given list.
     * @param outList to put the retrieved values.
     * @return true if some values were returned, false otherwise.
     */
    bool getAll(std::map<int,int>& outList);
};
} // end namespace medium
} // end namespace sim_mob
