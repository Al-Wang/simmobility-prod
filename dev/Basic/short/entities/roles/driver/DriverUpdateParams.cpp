//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "DriverUpdateParams.hpp"
#include "../short/entities/roles/driver/DriverFacets.hpp"

namespace sim_mob
{
DriverUpdateParams::DriverUpdateParams()
: UpdateParams() ,status(0),flags(0),yieldTime(0,0),lcTimeTag(200),speedOnSign(0),newFwdAcc(0),cftimer(0.0),newLatVelM(0.0),
  utilityCurrent(0),utilityRight(0),utilityLeft(0){

}
void DriverUpdateParams::setStatus(unsigned int s)
{
	status |= s;
}
void DriverUpdateParams::setStatus(string name,StatusValue v,string whoSet) {
	statusMgr.setStatus(name,v,whoSet);
}
StatusValue DriverUpdateParams::getStatus(string name) {
	return statusMgr.getStatus(name);
}
void DriverUpdateParams::setStatusDoingLC(LANE_CHANGE_SIDE& lcs)
{
	if(lcs == LCS_RIGHT)
	{
		setStatus(STATUS_LC_RIGHT);
	}
	else if(lcs == LCS_LEFT)
	{
		setStatus(STATUS_LC_LEFT);
	}

	// else do nothing
}
void DriverUpdateParams::buildDebugInfo()
{
	std::stringstream s;
	double ct=cftimer;
	if(abs(cftimer)<0.001)
		ct=0;
	char newFwdAccChar[20] = "\0";
	sprintf(newFwdAccChar,"%03.1f",newFwdAcc);
	// utility
	char ul[20] = "\0";
	sprintf(ul,"ul%2.1f",utilityLeft);
	char ur[20] = "\0";
	sprintf(ur,"ur%2.1f",utilityRight);
	char uc[20] = "\0";
	sprintf(uc,"uc%2.1f",utilityCurrent);

	size_t currLaneIdx = currLaneIndex;
	if(currLaneIdx<0.1) currLaneIdx = 0;

	double dis = perceivedDistToFwdCar / 100.0;
	char disChar[20] = "\0";
	sprintf(disChar,"%03.1f",dis);

	int fwdcarid=0;
	if(this->nvFwd.exists())
	{
		Driver* fwd_driver_ = const_cast<Driver*>(nvFwd.driver);
		fwdcarid = fwd_driver_->getParent()->getId();
	}

//	s<<ct
//			<<":"<<newFwdAccChar
			s<<ul
			<<":"<<uc
			<<":"<<ur;
//		<<":"<<rnd;

//			<<":"<<accSelect
//			<<":"<<nvFwd.exists()
//			<<":"<<disChar
//	<<":"<<currLaneIndex;
//			<<":"<<perceivedTrafficColor
//			<<":"<<perceivedDistToTrafficSignal/100.0;
	debugInfo = s.str();

//	std::cout<<debugInfo<<std::endl;
}
void DriverUpdateParams::addTargetLanes(set<const Lane*> tl)
{
	set<const Lane*> newTargetLanes;
	set<const Lane*>::iterator it;

	// find Lane* in both tl and targetLanes
	for(it=tl.begin();it!=tl.end();++it)
	{
		const Lane* l = *it;
		set<const Lane*>::iterator itFind = targetLanes.find(l);
		if(itFind != targetLanes.end())
		{
			newTargetLanes.insert(l);
		}
	}

	targetLanes = newTargetLanes;
}
void DriverUpdateParams::unsetStatus(unsigned int s)
{
	status &= ~s;
}

const RoadSegment* DriverUpdateParams::nextLink()
{
	DriverMovement *driverMvt = (DriverMovement*)driver->Movement();
	return driverMvt->fwdDriverMovement.getNextSegment(false);
}
bool DriverUpdateParams::willYield(unsigned int reason)
{
	//TODO willYield
	return true;
}
double DriverUpdateParams::lcMinGap(int type)
{
	std::vector<double> b = LC_GAP_MODELS[type];
	return b[2] * b[0];
}

}
