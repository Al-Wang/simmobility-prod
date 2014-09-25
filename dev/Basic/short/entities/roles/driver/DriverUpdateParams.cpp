//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "DriverUpdateParams.hpp"
#include "../short/entities/roles/driver/DriverFacets.hpp"

namespace sim_mob
{
DriverUpdateParams::DriverUpdateParams()
: UpdateParams() ,status(0),flags(0),yieldTime(0,0),lcTimeTag(200),speedOnSign(0),newFwdAcc(0),cftimer(0.0),newLatVelM(0.0),
  utilityCurrent(0),utilityRight(0),utilityLeft(0),rnd(0),perceivedDistToTrafficSignal(500),disAlongPolyline(0),dorigPosx(0),dorigPosy(0),
  movementVectx(0),movementVecty(0),headway(999),currLane(NULL){

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
	//	double ct=cftimer;
	//	if(abs(cftimer)<0.001)
	//		ct=0;

	//
	//	size_t currLaneIdx = currLaneIndex;
	//	if(currLaneIdx<0.1) currLaneIdx = 0;
	//




#if 0
		//debug car jump;
		char dl[20] = "\0";
		sprintf(dl,"dl%3.2f",disAlongPolyline/100.0);
		char dox[20] = "\0";
		sprintf(dox,"dox%3.2f",dorigPosx/100.0);
		char doy[20] = "\0";
		sprintf(doy,"doy%3.2f",dorigPosy/100.0);
		char latx[20] = "\0";
		sprintf(latx,"latx%3.2f",latMv_.getX()/100.0);
		char laty[20] = "\0";
		sprintf(laty,"laty%3.2f",latMv_.getY()/100.0);
		char mx[20] = "\0";
		sprintf(mx,"mx%12f",movementVectx);
		char my[20] = "\0";
		sprintf(my,"my%12f",movementVecty);
		s<<"            "<<parentId
				<<":"<<latx<<":"<<laty<<":"<<dl<<":"<<dox<<":"<<doy<<":"<<mx<<":"<<my;
#endif

#if 0
		//debug car following
		char newFwdAccChar[20] = "\0";
		sprintf(newFwdAccChar,"acc%03.1f",newFwdAcc);

		s<<"            "<<parentId
		<<":"<<newFwdAccChar
		<<":"<<accSelect;

#endif

#if 0
		//debug lane changing
	//
	////	s<<ct
	//			s<<":"<<newFwdAccChar

//						<<":"<<accSelect
//						<<":acc"<<newFwdAccChar
//				<<":"<<sp
//				<<":"<<lc;
	//			<<":"<<ds;


	//	// utility
		char ul[20] = "\0";
		sprintf(ul,"ul%3.2f",utilityLeft);
		char ur[20] = "\0";
		sprintf(ur,"ur%3.2f",utilityRight);
		char uc[20] = "\0";
		sprintf(uc,"uc%3.2f",utilityCurrent);
	//
		char rnd_[20] = "\0";
		sprintf(rnd_,"rd%3.2f",rnd);


		char sp[20] = "\0";
		sprintf(sp,"sp%3.2f",perceivedFwdVelocity);

		char ds[200] = "\0";
		sprintf(ds,"ds%3.2f",perceivedDistToTrafficSignal);
		// lc
		string lc="lc-s";
		if(getStatus(STATUS_LC_LEFT)) {
			lc = "lc-l";
		}
		else if(getStatus(STATUS_LC_RIGHT)){
			lc = "lc-r";
		}
		s<<"            "<<parentId;
		s<<":"<<ul;
		s<<":"<<uc;
		s<<":"<<ur;
		s<<":"<<rnd_;
		s<<":"<<lcd;
		s<<":"<<lc;
		s<<":"<<sp;
		s<<"=="<<lcDebugStr.str();


#endif

	// debug aura mgr
	#if 0
		int fwdcarid=-1;
		char fwdnvdis[20] = "\0";
				if(this->nvFwd.exists())
				{
					Driver* fwd_driver_ = const_cast<Driver*>(nvFwd.driver);
					fwdcarid = fwd_driver_->getParent()->getId();
					sprintf(fwdnvdis,"fwdnvdis%03.1f",nvFwd.distance);
				}
				//
				int backcarid=-1;
				if(this->nvBack.exists())
				{
					Driver* back_driver_ = const_cast<Driver*>(nvBack.driver);
					backcarid = back_driver_->getParent()->getId();
				}
				//
				int leftFwdcarid=-1;
				if(this->nvLeftFwd.exists())
				{
					Driver* driver_ = const_cast<Driver*>(nvLeftFwd.driver);
					leftFwdcarid = driver_->getParent()->getId();
				}
				int leftBackcarid=-1;
				if(this->nvLeftBack.exists())
				{
					Driver* driver_ = const_cast<Driver*>(nvLeftBack.driver);
					leftBackcarid = driver_->getParent()->getId();
				}
				//
				int rightFwdcarid=-1;
				if(this->nvRightFwd.exists())
				{
					Driver* driver_ = const_cast<Driver*>(nvRightFwd.driver);
					rightFwdcarid = driver_->getParent()->getId();
				}
				int rightBackcarid=-1;
				if(this->nvRightBack.exists())
				{
					Driver* driver_ = const_cast<Driver*>(nvRightBack.driver);
					rightBackcarid = driver_->getParent()->getId();
				}

				double dis = perceivedDistToFwdCar / 100.0;
				char disChar[20] = "\0";
				sprintf(disChar,"fwdis%03.1f",dis);

				char az[20] = "\0";
				sprintf(az,"az%03.1f",aZ);

				char pfwdv[20] = "\0";
				sprintf(pfwdv,"pfwdv%3.2f",perceivedFwdVelocity/100.0);

				if(headway>100) {
					headway = 100;
				}
				if(headway<-100){
					headway = 100;
				}
				char headwaystr[20] = "\0";
				sprintf(headwaystr,"headway%03.1f",headway);

				if(emergHeadway>100) {
					emergHeadway = 100;
				}
				if(emergHeadway<-100){
					emergHeadway = 100;
				}
				char emergHeadwaystr[20] = "\0";
				sprintf(emergHeadwaystr,"emergHeadway%03.1f",emergHeadway);

				s<<":fwd"<<fwdcarid;
				s<<":"<<fwdnvdis;
				s<<":"<<disChar;
				s<<":"<<headwaystr;
				s<<":"<<emergHeadwaystr;
				s<<":"<<az;
				s<<":"<<pfwdv;
				s<<":"<<cfDebugStr;
				//<<":back"<<backcarid
				//<<":lfwd"<<leftFwdcarid
				//<<":lback"<<leftBackcarid
				//<<":rfwd"<<rightFwdcarid
				//<<":rback"<<rightBackcarid;
	#endif

	//			//<<":"<<ct
	//			<<":"<<ul
				//<<":"<<uc
	//			//<<":"<<ur
	//		//<<":"<<rnd_;
	//
	////			<<":"<<accSelect
	////			<<":"<<nvFwd.exists()
	////			<<":"<<disChar
	////	<<":"<<currLaneIndex;
	////			<<":"<<perceivedTrafficColor
	////			<<":"<<perceivedDistToTrafficSignal/100.0;
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
