/*
 * LC_Model.cpp
 *
 *  Created on: 2011-8-15
 *      Author: mavswinwxy
 */

#include "Driver.hpp"
using namespace sim_mob;
const double sim_mob::Driver::GA_parameters[4][9] = {
//	    scale alpha lambda beta0  beta1  beta2  beta3  beta4  stddev
		{ 1.00, 0.0, 0.000, 0.508, 0.000, 0.000,-0.420, 0.000, 0.488},	//Discretionary,lead
		{ 1.00, 0.0, 0.000, 2.020, 0.000, 0.000, 0.153, 0.188, 0.526},	//Discretionary,lag
		{ 1.00, 0.0, 0.000, 0.384, 0.000, 0.000, 0.000, 0.000, 0.859},	//Mandatory,lead
		{ 1.00, 0.0, 0.000, 0.587, 0.000, 0.000, 0.048, 0.356, 1.073}	//Mandatory,lag
};
const double sim_mob::Driver::MLC_parameters[5] = {
		1320.0,		//feet, lower bound
	    5280.0,		//feet, delta
		   0.5,		//coef for number of lanes
		   1.0,		//coef for congestion level
		   1.0		//minimum time in lane
};


double sim_mob::Driver::lcCriticalGap(int type,	double dis_, double spd_, double dv_)
{
	double k=( type < 2 ) ? 1 : 5;
	return k*-dv_*timeStep;

	//The code below is from MITSIMLab. While the calculation result not suit for current unit.
	//So now, I just put them here.
	/*double dis=unit2Feet(dis_);
	double spd=unit2Feet(spd_);
	double dv=unit2Feet(dv_);
	double rem_dist_impact = (type < 3) ?
		0.0 : (1.0 - 1.0 / (1 + exp(GA_parameters[type][2] * dis)));
	double dvNegative = (dv < 0) ? dv : 0.0;
	double dvPositive = (dv > 0) ? dv : 0.0;
	double gap = GA_parameters[type][3] + GA_parameters[type][4] * rem_dist_impact +
			GA_parameters[type][5] * dv + GA_parameters[type][6] * dvNegative + GA_parameters[type][7] *  dvPositive;
	double u = gap + nRandom(0, GA_parameters[type][8]);
	double cri_gap ;

	if (u < -4.0) {
		cri_gap = 0.0183 * GA_parameters[type][0] ;		// exp(-4)=0.0183
	}
	else if (u > 6.0) {
		cri_gap = 403.4 * GA_parameters[type][0] ;   	// exp(6)=403.4
	}
	else cri_gap = GA_parameters[type][0] * exp(u) ;

	if (cri_gap < GA_parameters[type][1]) return feet2Unit(GA_parameters[type][1]) ;
	else return feet2Unit(cri_gap) ;*/
}


unsigned int sim_mob::Driver::gapAcceptance(int type)
{
	int border[2]={0,2};
	int offset[2]={-1,1};
	bool badarea[2]={false,false};
	//check if there is bad area on the left/right side
	for(int i=0;i<numOfBadAreas;i++){
		for(int j=0;j<2;j++){
			if(getLane()+j*2-1==badareas[i].lane
				&& xPos_-length/2 < badareas[i].endX
				&& xPos_+length/2 > badareas[i].startX
				)badarea[j]=true;
		}
	}

	//[0:left,1:right][0:lead,1:lag]
	double otherSpeed[2][2];		//the speed of the closest vehicle in adjacent lane
	double otherDistance[2][2];		//the distance to the closest vehicle in adjacent lane

	double other_xOffset;
	double other_yOffset;
	double other_xPos_;
	double other_xVel_;

	LF=getNextForBDriver(true,true);
	LB=getNextForBDriver(true,false);
	RF=getNextForBDriver(false,true);
	RB=getNextForBDriver(false,false);
	Agent* F;
	Agent* B;
	for(int i=0;i<2;i++){
		if(i==0){
			F=LF;
			B=LB;
		} else {
			F=RF;
			B=RB;
		}
		if(getLane()!=border[i] && badarea[i]!=true){	//the left/right side exists
		int BA=findClosestBadAreaAhead(getLane()+offset[i]);
			if(!F) {		//no vehicle ahead
				if(BA==-1) {		//also no bad area ahead
					otherSpeed[i][0]=MAX_NUM;
					otherDistance[i][0]=MAX_NUM;
				} else {			//bad area ahead, check the gap between subject vehicle and bad area
					otherSpeed[i][0]=0;
					otherDistance[i][0]=badareas[BA].startX-xPos_-length/2;
				}
			} else {				//has vehicle ahead
				other_xOffset	= F->xPos.get()	- testLinks[currentLink].startX;
				other_yOffset	= F->yPos.get()	- testLinks[currentLink].startY;
				other_xPos_		= other_xOffset	* xDirection	+ other_yOffset	* yDirection;
				other_xVel_		= F->xVel.get()	* xDirection	+ F->yVel.get()	* yDirection;

				double gna1=other_xPos_-xPos_-length;
				if(BA==-1) {		//no bad area, just check the gap
					otherSpeed[i][0]=other_xVel_;
					otherDistance[i][0]=gna1;
				} else {			//bad area ahead, use the smallest gap to compute
					double gna2=badareas[BA].startX-xPos_-length/2;
					otherSpeed[i][0]=( gna1 < gna2 )?other_xVel_:0;
					otherDistance[i][0]=( gna1 < gna2 )?gna1:gna2;
				}
			}

			if(B) {		//has vehicle behind, check the gap
				other_xOffset	= B->xPos.get()	- testLinks[currentLink].startX;
				other_yOffset	= B->yPos.get()	- testLinks[currentLink].startY;
				other_xPos_		= other_xOffset	* xDirection	+ other_yOffset	* yDirection;
				other_xVel_		= B->xVel.get()	* xDirection	+ B->yVel.get()	* yDirection;

				otherSpeed[i][1]=other_xVel_;
				otherDistance[i][1]=xPos_-other_xPos_-length;
			} else {		//no vehicle behind
				otherSpeed[i][1]=-MAX_NUM;
				otherDistance[i][1]=MAX_NUM;
			}
		} else {			// no left/right side exists
			otherSpeed[i][0]=-MAX_NUM;
			otherDistance[i][0]=0;
			otherSpeed[i][1]=MAX_NUM;
			otherDistance[i][1]=0;
		}
	}

	//[0:left,1:right][0:lead,1:lag]
	bool flags[2][2];
	for(int i=0;i<2;i++){	//i for left / right
		for(int j=0;j<2;j++){	//j for lead / lag
			double v 	= ( j == 0 ) ? xVel_ : otherSpeed[i][1];
			double dv 	= ( j == 0 ) ? otherSpeed[i][0] - xVel_ : xVel_-otherSpeed[i][1];
			flags[i][j]= otherDistance[i][j] > lcCriticalGap(j+type,dis2stop,v,dv);
		}
	}

	//Build up a return value.
	unsigned int returnVal = 0;
	if ( flags[0][0] && flags[0][1] ) {
		returnVal |= LSIDE_LEFT;
	}
	if ( flags[1][0] && flags[1][1] ) {
		returnVal |= LSIDE_RIGHT;
	}

	return returnVal;
}

double sim_mob::Driver::calcSideLaneUtility(bool isLeft){
	int border = (isLeft) ? 0 : 2;
	int offset = (isLeft) ? -1 : 1;
	Agent* F = (isLeft) ? LF : RF;
	double other_xOffset;
	double other_yOffset;
	double other_xPos_;
	if(getLane()==border) {
		return -MAX_NUM;	//has no left/right side
	} else {
		int BA=findClosestBadAreaAhead(getLane()+offset);
		if(BA==-1) {			//no bad area ahead
			if(F)	{	//has vehicle ahead
				other_xOffset	= F->xPos.get()	- testLinks[currentLink].startX;
				other_yOffset	= F->yPos.get()	- testLinks[currentLink].startY;
				other_xPos_		= other_xOffset	* xDirection	+ other_yOffset	* yDirection;
				//other_xPos_=F->xPos.get();
				return other_xPos_-xPos_-length;
			} else {			//no vehicle ahead neither
				return MAX_NUM;
			}
		} else if(badareas[BA].startX-xPos_-length < avoidBadAreaDistance) {
			return -MAX_NUM;	//bad area is to close, won't choose that side
		} else {				//has bad area but is not too close
			double slr1=badareas[BA].startX-xPos_-length/2;
			if(F) {	//has vehicle ahead
				other_xOffset	= F->xPos.get()	- testLinks[currentLink].startX;
				other_yOffset	= F->yPos.get()	- testLinks[currentLink].startY;
				other_xPos_		= other_xOffset	* xDirection	+ other_yOffset	* yDirection;
				//other_xPos_=F->xPos.get();
				double slr2=other_xPos_-xPos_-length;
				return ( slr1 < slr2 ) ? slr1 : slr2;		//check the smaller gap
			} else {
				return slr1;	//has no vehicle ahead,just check the gap to the bad area
			}
		}
	}
}

double sim_mob::Driver::makeDiscretionaryLaneChangingDecision()
{
	// for available gaps(including current gap between leading vehicle and itself), vehicle will choose the longest
	unsigned int freeLanes = gapAcceptance(DLC);
	bool freeLeft = ((freeLanes&LSIDE_LEFT)!=0);
	bool freeRight = ((freeLanes&LSIDE_RIGHT)!=0);

	if(!freeLeft && !freeRight)return 0;		//neither gap is available, stay in current lane

	double s=getDistance();
	if(s>satisfiedDistance)return 0;	// space ahead is satisfying, stay in current lane

	//calculate the utility of both sides
	double leftUtility = calcSideLaneUtility(true);
	double rightUtility = calcSideLaneUtility(false);

	//to check if their utilities are greater than current lane
	bool left = ( s < leftUtility );
	bool right = ( s < rightUtility );

	//decide
	if(freeRight && !freeLeft && right) {
		return 1;
	}
	if(freeLeft && !freeRight && left) {
		return -1;
	}
	if(freeLeft && freeRight){
		if(right && left){
			return (leftUtility < rightUtility) ? 1 : -1;	//both side is available, choose the better one
		}
		if(right && !left) {
			return 1;
		}
		if(!right && left) {
			return -1;
		}
		if(!right && !left) {
			return 0;
		}
	}
	return 0;
}

double sim_mob::Driver::checkIfMandatory()
{
	int BA=findClosestBadAreaAhead(getLane());
	if( BA==-1 ){
		dis2stop=MAX_NUM;
		return 0;	//no bad area ahead, do not have to MLC
	}
	else{
		dis2stop=badareas[BA].startX-xPos_-length/2;
		//we first use simple rule to decide lane changing mode
		return (dis2stop < satisfiedDistance) ? 1 : 0;  //This is equivalent to the if statement below. ~Seth
		/*if(dis2stop < satisfiedDistance) {
			return 1;
		} else {
			return 0;
		}*/

		//the code below is MITSIMLab model
		/*double num		=	1;		//now we just assume that MLC only need to change to the adjacent lane
		double y		=	0.5;		//segment density/jam density, now assume that it is 0.5
		double delta0	=	feet2Unit(MLC_parameters[0]);
		double dis		=	dis2stop - delta0;
		double delta	=	1.0 + MLC_parameters[2] * num + MLC_parameters[3] * y;
		delta *= MLC_parameters[1];
		return exp(-dis * dis / (delta * delta));*/
	}
}

double sim_mob::Driver::makeMandatoryLaneChangingDecision()
{
	unsigned int freeLanes = gapAcceptance(MLC);
	bool freeLeft = ((freeLanes&LSIDE_LEFT)!=0);
	bool freeRight = ((freeLanes&LSIDE_RIGHT)!=0);

	//calculate the utility of both sides
	double leftUtility = calcSideLaneUtility(true);
	double rightUtility = calcSideLaneUtility(false);

	/*
	 * In fact, in most cases, vehicle has its desired lane to achieve.
	 * This parts of the functions will be made in later updates.
	 * Now we just compare utilities of the 2 adjacent lanes and choose the bigger one.
	 * */

	//decide
	if(freeRight && !freeLeft) {
		isWaiting=false;
		return 1;
	} else if(freeLeft && !freeRight) {
		isWaiting=false;
		return -1;
	} else if(freeLeft && freeRight) {
		isWaiting=false;
		return (leftUtility < rightUtility) ? 1 : -1;
	} else {			//when neither side is available,vehicle will decelerate to wait a proper gap.
		isWaiting=true;
		return 0;
	}
}

/*
 * In MITSIMLab, vehicle change lane in 1 time step.
 * While in sim mobility, vehicle approach the target lane in limited speed.
 * So the behavior when changing lane is quite different from MITSIMLab.
 * New behavior and model need to be discussed.
 * Here is just a simple model, which now can function.
 *
 * -wangxy
 * */
void sim_mob::Driver::excuteLaneChanging()
{
	// when vehicle is on the lane, make decision
	if(!ischanging){

		if(getLinkLength()-xPos_ < 50){
			return;			//when close to link end, do not change lane
		}
		//check if MLC is needed(vehicle has probability=checkIfMandatory() to be tagged in to MLC mode)
		double p=(double)(rand()%1000)/1000;
		if(p<checkIfMandatory()){
			changeMode = MLC;
		} else {
			changeMode = DLC;
			dis2stop=MAX_NUM;		//no crucial point ahead
		}

		//make decision depending on current lane changing mode
		if(changeMode==DLC) {
			changeDecision=makeDiscretionaryLaneChangingDecision();
		} else {
			changeDecision=makeMandatoryLaneChangingDecision();
		}

		//set original lane and target lane
		fromLane=getLane();
		toLane=getLane()+changeDecision;
	}
	if(isWaiting){			//when waiting proper gap
		ischanging=true;		//vehicle is actually changing lane
		changeDecision=makeMandatoryLaneChangingDecision();		//to make decision again
		fromLane=getLane();
		toLane=getLane()+changeDecision;
		acc_=MAX_DECELERATION;	//decelerate in max rate to wait for acceptable gap
	} else{			//when changing lane
		if(changeDecision!=0) {
			double change=1;		//for MLC

			if(checkForCrash() ) {	//if get crashed
				if(changeMode==DLC && !isback){		// in DLC mode
					//exchange the leaving lane and target lane to get back
					int tmp;
					tmp=fromLane;
					fromLane=toLane;
					toLane=tmp;
					changeDecision=-changeDecision;
					isback=true;
				}
				if(changeMode==MLC){	//in MLC mode
					change=0;			//do not change in this time step
					acc_=MAX_DECELERATION;	//decelerate to let other vehicle go
				}
			}
			ischanging=true;
			yPos_ += changeDecision*VelOfLaneChanging*change;	//change y position according to decision
		}
	}
}