//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * ParkingCoordinator.cpp
 *
 *  Created on: Jul 3, 2013
 *      Author: zhang
 */

#include "ParkingCoordinator.hpp"
#include "algorithm"
#include "entities/Agent.hpp"
#include "entities/Person.hpp"
#include "entities/Person_ST.hpp"

namespace sim_mob {

namespace FMOD
{

ParkingCoordinator::ParkingCoordinator() {
    // TODO Auto-generated constructor stub

}

ParkingCoordinator::~ParkingCoordinator() {
    // TODO Auto-generated destructor stub
}

bool ParkingCoordinator::enterTo(const Node* node, const Agent* agent)
{
    bool ret = isExisted(node, agent);
    if(ret == true){
        return ret;
    }

    std::map<const Node*, ParkingLot>::iterator it = vehicleParking.find(node);
    if( it == vehicleParking.end() ){
        ParkingLot lot;
        vehicleParking.insert( std::make_pair(node, lot));
        it = vehicleParking.find(node);
    }

    ParkingLot& lot = (*it).second;
    std::vector<const Agent*>::iterator itAg = std::find(lot.vehicles.begin(), lot.vehicles.end(), agent);
    if( itAg != lot.vehicles.end() ){
        ret = false;
    }
    else {
        ret = true;
        lot.vehicles.push_back(agent);
        lot.currentOccupancy++;
        lot.node = node;
    }

    return ret;
}

const Agent* ParkingCoordinator::leaveFrom(const Node* node, const Agent* agent)
{
    const Agent* ret = nullptr;

    if( !isExisted(node, agent) ){
        return ret;
    }
    else{
        std::map<const Node*, ParkingLot>::iterator it = vehicleParking.find(node);
        if( it == vehicleParking.end() )
            return ret;

        ParkingLot& lot = (*it).second;
        std::vector<const Agent*>::iterator itAg = std::find(lot.vehicles.begin(), lot.vehicles.end(), agent );
        if(itAg != lot.vehicles.end() ){
            lot.vehicles.erase(itAg);
            ret = agent;
        }
    }
    return ret;
}

const Agent* ParkingCoordinator::remove(const int clientid)
{
    const Agent* ret = nullptr;

    for( std::map<const Node*, ParkingLot>::iterator it = vehicleParking.begin(); it!=vehicleParking.end(); it++ ){

        ParkingLot& lot = (*it).second;
        for( std::vector<const Agent*>::iterator itAg = lot.vehicles.begin(); itAg!=lot.vehicles.end(); itAg++ ){
            const Agent* agent = (*itAg);
            const Person_ST* person = dynamic_cast<const Person_ST*>( agent );

            if( (person!=nullptr) && (person->client_id == clientid) ){
                lot.vehicles.erase(itAg);
                ret = (*itAg);
                return ret;
            }
        }
    }

    return ret;
}

int  ParkingCoordinator::getCapacity(const Node* node)
{
    std::map<const Node*, ParkingLot>::iterator it = vehicleParking.find(node);
    if( it == vehicleParking.end() ){
        return 0;
    }

    ParkingLot& lot = (*it).second;

    return lot.maxCapacity;
}

int  ParkingCoordinator::getOccupancy(const Node* node)
{
    std::map<const Node*, ParkingLot>::iterator it = vehicleParking.find(node);
    if( it == vehicleParking.end() ){
        return 0;
    }

    ParkingLot& lot = (*it).second;

    return lot.currentOccupancy;
}

bool ParkingCoordinator::isExisted(const Node* node, const Agent* agent)
{
    bool ret = true;

    if(vehicleParking.size()==0){
        ret = false;
    }
    else {
        std::map<const Node*, ParkingLot>::iterator it = vehicleParking.find(node);
        if( it == vehicleParking.end() ){
            ret = false;
        }

        ParkingLot& lot = (*it).second;
        std::vector<const Agent*>::iterator itAg = std::find(lot.vehicles.begin(), lot.vehicles.end(), agent);
        if( itAg == lot.vehicles.end() ){
            ret = false;
        }
    }
    return ret;
}


} /* namespace sim_mob */

}
