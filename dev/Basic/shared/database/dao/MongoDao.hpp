//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * MongoDao.hpp
 *
 *  Created on: Nov 20, 2013
 *      Author: Harish Loganathan
 */

#pragma once
#include "database/dao/I_Dao.h"
#include "database/DB_Connection.hpp"
#include "mongo/client/dbclient.h"

namespace sim_mob {
    namespace db {

        /**
         * Data access object for MongoDB - a NoSQL database
         *
         * \author Harish Loganathan
         */
        class MongoDao : public I_Dao<mongo::BSONObj> {
        public:

            MongoDao(DB_Config& dbConfig, const std::string& database, const std::string& collection)
            : connection(db::MONGO_DB, dbConfig) {
                connection.connect();
                std::stringstream ss;
                ss << database << "." << collection;
                collectionName = ss.str();
            }

            virtual ~MongoDao() {
            }

            /**
             * inserts a document into collection
             *
             * @param bsonObj a mongo::BSONObj containing object to insert
             * @return true if the transaction was committed with success; false otherwise.
             */
            mongo::BSONObj& insert(mongo::BSONObj& bsonObj, bool returning = false) {
                connection.getSession<mongo::DBClientConnection>().insert(collectionName, bsonObj);
                return bsonObj; // Unnecessary return just to comply with the base interface
            }

            /**
             * Updates the given entity into the data source.
             * @param bsonObj to update.
             * @return true if the transaction was committed with success,
             *         false otherwise.
             *
             * Note: This function is pure virtual in the base class. Dummy implementation provided to avoid compulsion to implement this function in sub classes
             */
            bool update(mongo::BSONObj& bsonObj) {
                throw std::runtime_error("MongoDao::update() - Not implemented");
            }

            /**
             * Updates the given entity into the data source.
             * @param query query for selecting document(s) to update
             * @param bsonObj obj to update
             * @param upsert flag to indicate whether to insert if no document was returned by query
             * @param multipleDocuments flag to indicate whether all documents (or just the first document ) returned by query need to be updated
             * @return true if the transaction was committed with success,
             *         false otherwise.
             */
            bool update(mongo::Query& query, mongo::BSONObj& bsonObj, bool upsert=false, bool multipleDocuments=false) {
            	connection.getSession<mongo::DBClientConnection>().update(collectionName, query, bsonObj, upsert, multipleDocuments);
            	return true;
            }

            /**
             * Deletes all objects filtered by given params.
             * @param params to filter.
             * @return true if the transaction was committed with success,
             *         false otherwise.
             *
             * Note: This function is pure virtual in the base class. Dummy implementation provided to avoid compulsion to implement this function in sub classes
             */
            bool erase(const Parameters& params) {
                throw std::runtime_error("MongoDao::erase() - Not implemented");
            }

            /**
             * Gets a single value filtered by given ids.
             * @param ids to filter.
             * @param outParam to put the value
             * @return true if a value was returned, false otherwise.
             *
             * Note: This function is pure virtual in the base class. Dummy implementation provided to avoid compulsion to implement this function in sub classes
             */
            bool getById(const Parameters& ids, mongo::BSONObj& outParam) {
                throw std::runtime_error("MongoDao::getById() - Not implemented");
            }

            /**
             * Gets all documents from the collection
             * @param outlist the container to be filled with objects retrieved from mongoDB
             * @return true if some values were returned, false otherwise.
             *
             * Note: This function is pure virtual in the base class. Dummy implementation provided to avoid compulsion to implement this function in sub classes
             */
            bool getAll(std::vector<mongo::BSONObj>& outList) {
                throw std::runtime_error("MongoDao::getAll() - Not implemented");
            }

            /**
             * Gets all documents from the collection
             * @param outlist the container to be filled with objects retrieved from mongoDB
             * @return true if some values were returned, false otherwise.
             *
             * Note: This function is pure virtual in the base class. Dummy implementation provided to avoid compulsion to implement this function in sub classes
             */
            bool getAll(std::vector<mongo::BSONObj*>& outList) {
                throw std::runtime_error("MongoDao::getAll() - Not implemented");
            }

            void getMultiple(mongo::Query& qry, std::auto_ptr<mongo::DBClientCursor>& outCursor) {
            	outCursor = connection.getSession<mongo::DBClientConnection>().query(collectionName, qry);
            	return;
            }

            /**
             * Overload. Fetches a cursor to the result of the query
             *
             * @param bsonObj a mongo::BSONObj object containing the constructed query
             * @return true if a value was returned, false otherwise.
             */
            bool getOne(mongo::BSONObj& bsonObj, mongo::BSONObj& outBsonObj) {
                mongo::Query qry(bsonObj);
                outBsonObj = connection.getSession<mongo::DBClientConnection>().findOne(collectionName, qry);
                return true;
            }

        protected:
            DB_Connection connection;
            std::string collectionName;
        };

    }//end namespace db
}//end namespace sim_mob
