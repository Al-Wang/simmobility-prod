/* Copyright Singapore-MIT Alliance for Research and Technology */

/*
 * Passenger.cpp
 *
 *  Created on: 2012-12-20
 *      Author: Meenu
 */

#include "Passenger.hpp"

#include <vector>
#include <iostream>
#include <cmath>

#include <boost/unordered_map.hpp>

#include "entities/Person.hpp"
#include "entities/roles/driver/BusDriver.hpp"
#include "entities/vehicle/BusRoute.hpp"
#include "entities/vehicle/Bus.hpp"
#include "entities/AuraManager.hpp"
#include "entities/signal/Signal.hpp"
#include "entities/roles/driver/BusDriver.hpp"

#include "geospatial/Node.hpp"
#include "geospatial/Link.hpp"
#include "geospatial/RoadSegment.hpp"
#include "geospatial/Lane.hpp"
#include "geospatial/Node.hpp"
#include "geospatial/UniNode.hpp"
#include "geospatial/MultiNode.hpp"
#include "geospatial/LaneConnector.hpp"
#include "geospatial/Crossing.hpp"
#include "geospatial/BusStop.hpp"
#include "geospatial/Point2D.hpp"
#include "geospatial/streetdir/StreetDirectory.hpp"

#include "util/OutputUtil.hpp"
#include "util/GeomHelpers.hpp"
#include "util/GeomHelpers.hpp"
#include "util/DynamicVector.hpp"

#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"

using namespace sim_mob;
using std::vector;
using std::cout;
using std::map;
using std::string;


sim_mob::Passenger::Passenger(Agent* parent, MutexStrategy mtxStrat, std::string roleName) : Role(parent,roleName),
	waitingAtBusStop(mtxStrat, true), boardedBus(mtxStrat,false), alightedBus(mtxStrat,false),
	destReached(false), busdriver(mtxStrat,nullptr), randomX(0), randomY(0),
	WaitingTime(-1), TimeofReachingBusStop(0), params(parent->getGenerator())
{
}


void sim_mob::Passenger::setParentBufferedData()
{
	//if passenger inside bus,update position of the passenger agent(inside bus)every frame tick
	if((this->isAtBusStop()==false)and(this->busdriver.get()!=NULL))
	{
		//passenger x,y position equals the bus drivers x,y position as passenger is inside the bus
		parent->xPos.set(this->busdriver.get()->getPositionX());
		parent->yPos.set(this->busdriver.get()->getPositionY());
	}
}

void sim_mob::Passenger::frame_init(UpdateParams& p)
{
	 // bus stop no 4179,x-372228.099782,143319.818791
	 destination.setX(parent->destNode->location.getX());
	 destination.setY(parent->destNode->location.getY());

	 if(parent->destNode->getID() == 75780)
	 {
		destination.setX(37222809);
		destination.setY(14331981);
	}
	else if(parent->destNode->getID()==75822)
	{
		destination.setX(37290070);//75822
		destination.setY(14390218);
	}
	else if(parent->destNode->getID() == 91144)
	{
		destination.setX(37285920);
		destination.setY(14375941);

	}
	else if(parent->destNode->getID() == 106946)
	{
		destination.setX(37267223);
		destination.setY(14352090);

	}
	else if(parent->destNode->getID() == 103046)
	{
		destination.setX(37234196);
		destination.setY(14337740);

	}
	else if(parent->destNode->getID() == 95374)
	{
		destination.setX(37241994);
		destination.setY(14347188);

	}
	else if(parent->destNode->getID() == 58950)
	{
		destination.setX(37263940);
		destination.setY(14373280);

	}
	else if(parent->destNode->getID() == 75808)
	{
		destination.setX(37274363);
		destination.setY(14385509);

	}
	else if(parent->destNode->getID() == 98852)
	{
		destination.setX(37254693);
		destination.setY(14335301);

	}

	if(parent->originNode->getID() == 75780)
	{
		parent->xPos.set(37222809);
		parent->yPos.set(14331981);

	}
	else if(parent->originNode->getID() == 91144)
	{
		parent->xPos.set(37285920);
		parent->yPos.set(14375941);

	}
	else if(parent->originNode->getID() == 106946)
	{
		parent->xPos.set(37267223);
		parent->yPos.set(14352090);

	}
	else if(parent->originNode->location.getX()==37236345)//103046
	 {
		 destination.setX(37222809);
		 destination.setY(14331981);
	 }
	 else if(parent->destNode->getID()==75822)
	 {
		 destination.setX(37290070);
		 destination.setY(14390218);
	 }
	 else if(parent->destNode->getID() == 91144)
	 {
		 destination.setX(37285920);
		 destination.setY(14375941);
	 }
	 else if(parent->destNode->getID() == 106946)
	 {
		 destination.setX(37267223);
		 destination.setY(14352090);
	 }
	 else if(parent->destNode->getID() == 103046)
	 {
		 destination.setX(37234196);
		 destination.setY(14337740);
	 }
	 else if(parent->destNode->getID() == 95374)
	 {
		 destination.setX(37241994);
		 destination.setY(14347188);
	 }
	 else if(parent->destNode->getID() == 58950)
	 {
		 destination.setX(37263940);
		 destination.setY(14373280);
	 }
	 else if(parent->destNode->getID() == 75808)
	 {
		 destination.setX(37274363);
		 destination.setY(14385509);
	 }
	 else if(parent->destNode->getID() == 98852)
	 {
		 destination.setX(37254693);
		 destination.setY(14335301);
	 }
	 if(parent->originNode->getID() == 75780)
	 {
		 parent->xPos.set(37222809);
		 parent->yPos.set(14331981);
	 }
	 else if(parent->originNode->getID() == 91144)
	 {
		 parent->xPos.set(37285920);
		 parent->yPos.set(14375941);
	 }
	 else if(parent->originNode->getID() == 106946)
	 {
		 parent->xPos.set(37267223);
		 parent->yPos.set(14352090);
	 }
	 else if(parent->originNode->getID()==103046)
	 {
		 parent->xPos.set(37234200);
		 parent->yPos.set(14337700);
	 }
	 else if(parent->originNode->getID()==95374)
	 {
		 parent->xPos.set(37242000);
	     parent->yPos.set(14347200);
	 }
	 else if(parent->originNode->getID()==58950)
	 {
		 parent->xPos.set(37263900);
		 parent->yPos.set(14373300);
	 }
	 else if(parent->originNode->getID()==75808)
	 {
		 parent->xPos.set(37274300);
		 parent->yPos.set(14385500);
	 }
	 else if(parent->originNode->getID()==75780)
	 {
		 parent->xPos.set(37222800);//4179
		 parent->yPos.set(14332000);
	 }
	 else if(parent->originNode->getID()==68014)
	 {
		 parent->xPos.set(37199539);//8069
		 parent->yPos.set(14351303);
	 }
	 else if(parent->originNode->getID()==75822)
	 {
		 parent->xPos.set(37290070);
		 parent->yPos.set(14390218);
	 }
	 else//default position
	 {
		 parent->xPos.set(parent->originNode->location.getX());
		 parent->yPos.set(parent->originNode->location.getY());
	 }

	//NOTE: These variables were already set, and setting them again in frame_init() risks
	//      clashing with the BusDriver's decision to set() them.
	/*busdriver.set(nullptr);
	 waitingAtBusStop.set(true);
	 boardedBus.set(false);
	 alightedBus.set(false);*/

	 randomX = 0;
	 randomY = 0;
	 destReached = false;

	 TimeofReachingBusStop = parent->getStartTime();
}


UpdateParams& sim_mob::Passenger::make_frame_tick_params(timeslice now)
{
	params.reset(now);
	return params;
}

//Main update method
void sim_mob::Passenger::frame_tick(UpdateParams& p)
{
	setParentBufferedData();//update passenger coordinates every frame tick
	if(destReached) {
		parent->setToBeRemoved();
	}
}


void sim_mob::Passenger::frame_tick_output(const UpdateParams& p)
{
	if (ConfigParams::GetInstance().is_run_on_many_computers) {
		return;
	}

	//Reset our offset if it's set to zero
	if (randomOffset.getX()==0 && randomOffset.getY()==0) {
		boost::uniform_int<> distX(0, 249);
		randomOffset.setX(distX(parent->getGenerator())+1);
		boost::uniform_int<> distY(0, 99);
		randomOffset.setY(distY(parent->getGenerator())+1);
	}

	//This code doesn't do what you think it does. ~Seth
	/*pthread_mutex_lock(&mu);
	srand(parent->getId()*parent->getId());
	int random_num1 = rand()%(250);
	int random_num2 = rand()%(100);
	pthread_mutex_unlock(&mu);*/

	if((this->waitingAtBusStop.get()==true) && (!(this->alightedBus)))
	{
		LogOut("("<<"\"passenger\","<<p.now.frame()<<","<<parent->getId()<<","<<"{\"xPos\":\""<<(this->parent->xPos.get()+randomOffset.getX())<<"\"," <<"\"yPos\":\""<<(this->parent->yPos.get()+randomOffset.getY())<<"\",})"<<std::endl);
	}
	else if(this->alightedBus)
	{
	   LogOut("("<<"\"passenger\","<<p.now.frame()<<","<<parent->getId()<<","<<"{\"xPos\":\""<<(randomX+randomOffset.getX())<<"\"," <<"\"yPos\":\""<<(randomY+randomOffset.getY())<<"\",})"<<std::endl);

	}
}

void sim_mob::Passenger::frame_tick_output_mpi(timeslice now)
{
	if (now.frame() < 1 || now.frame() < parent->getStartTime())
			return;
	if((this->waitingAtBusStop.get()==true) ||(this->alightedBus==true))
	{
		if (this->parent->isFake) {
			LogOut("("<<"\"passenger\","<<now.frame()<<","<<parent->getId()<<","<<"{\"xPos\":\""<<this->randomX<<"\"," <<"\"yPos\":\""<<this->randomY <<"\"," <<"\"fake\":\""<<"true" <<"\",})"<<std::endl);
		} else {
			LogOut("("<<"\"passenger\","<<now.frame()<<","<<parent->getId()<<","<<"{\"xPos\":\""<<this->randomX<<"\"," <<"\"yPos\":\""<<this->randomY<<"\"," <<"\"fake\":\""<<"false" <<"\",})"<<std::endl);
		}
	}
}

/*void sim_mob::Passenger::update(timeslice now)
{

}*/

bool sim_mob::Passenger::isAtBusStop()
{
	return this->waitingAtBusStop.get();
}

bool sim_mob::Passenger::isDestBusStopReached() {

	if(alightedBus==true)//passenger alights when destination is reached
		return true;
	else
	    return false;
}

Role* sim_mob::Passenger::clone(sim_mob::Person* parent) const {
	return new Passenger(parent, parent->getMutexStrategy());
}

Point2D sim_mob::Passenger::getXYPosition()
{
	return Point2D(parent->xPos.get(),parent->yPos.get());
}

Point2D sim_mob::Passenger::getDestPosition()
{
	return Point2D((destination.getX()),(destination.getY()));
}

bool sim_mob::Passenger::PassengerBoardBus(Bus* bus,BusDriver* busdriver,Person* p,std::vector<const BusStop*> busStops,int k)
{
	//chcking if destinaton node position of passenger is in the list of bus stops which the bus would stop
	for(;k < busStops.size();k++) {
		  Point2D busStop_Pos(busStops[k]->xPos,busStops[k]->yPos);
	 	  if((abs((busStop_Pos.getX() - this->getDestPosition().getX())/100)<= 3) and (abs((busStop_Pos.getY() - this->getDestPosition().getY())/100)<= 3))
	 	  {
	           if(bus->getPassengerCount()+1<=bus->getBusCapacity())//and(last_busstop==false))
	           {
	        	   //if passenger is to be boarded,add to passenger vector inside the bus
	        	   bus->passengers_inside_bus.push_back(p);
	        	   bus->setPassengerCount(bus->getPassengerCount()+1);
	        	   this->busdriver.set(busdriver);//passenger should store the bus driver
	        	   boardedBus.set(true);//to indicate passenger has boarded bus
	        	   alightedBus.set(false);//to indicate whether passenger has alighted bus
	        	   findWaitingTime(bus);
	        	   return true;
	           }
	 	  }
	 }
	 return false;
}

bool sim_mob::Passenger::PassengerAlightBus(Bus* bus,int xpos_approachingbusstop,int ypos_approachingbusstop,BusDriver* busdriver)
{
	 Point2D busStop_Pos(xpos_approachingbusstop,ypos_approachingbusstop);
     if((abs((busStop_Pos.getX()/100)-(this->getDestPosition().getX()/100))<=2) and (abs((busStop_Pos.getY()/100)-(this->getDestPosition().getY()/100))<=2))
	 {
    	 //alight-delete passenger agent from list
    	 bus->setPassengerCount(bus->getPassengerCount()-1);
    	 this->busdriver.set(nullptr);	//driver would be null as passenger has alighted
    	 alightedBus.set(true);
    	 boardedBus.set(false);
    	 parent->xPos.set(xpos_approachingbusstop);
    	 parent->yPos.set(ypos_approachingbusstop);
    	 randomX = xpos_approachingbusstop;
    	 randomY = ypos_approachingbusstop;
    	 return true;
	 }
     return false;
}

std::vector<sim_mob::BufferedBase*> sim_mob::Passenger::getSubscriptionParams()
{
 	std::vector<sim_mob::BufferedBase*> res;
 	res.push_back(&(waitingAtBusStop));
 	res.push_back(&(boardedBus));
 	res.push_back(&(alightedBus));
 	res.push_back(&(busdriver));

 	//res.push_back(&(DestReached));
 	//res.push_back(&(random_x));
 	//res.push_back(&(random_y));
 	return res;
}

bool sim_mob::Passenger::isBusBoarded()
{
	return (boardedBus==true);
}

void sim_mob::Passenger::findWaitingTime(Bus* bus)
{
	WaitingTime=(bus->TimeOfBusreachingBusstop)-(TimeofReachingBusStop);
}
