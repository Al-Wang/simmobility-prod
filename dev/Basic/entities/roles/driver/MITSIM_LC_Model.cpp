/* Copyright Singapore-MIT Alliance for Research and Technology */

/*
 * LC_Model.cpp
 *
 *  Created on: 2011-8-15
 *      Author: mavswinwxy & Li Zhemin
 */

#include <boost/random.hpp>

#include <limits>

#include "entities/vehicle/Vehicle.hpp"
#include "LaneChangeModel.hpp"
#include "Driver.hpp"

using std::numeric_limits;
using namespace sim_mob;


namespace {
//Random number generator
//TODO: We need a policy on who can get a generator and why.
//boost::mt19937 gen;

//Declare MAX_NUM as a private variable here to limit its scope.
const double MAX_NUM = numeric_limits<double>::max();

///Simple struct to hold Gap Acceptance model parameters
struct GapAcceptParam {
	double scale;
	double alpha;
	double lambda;
	double beta0;
	double beta1;
	double beta2;
	double beta3;
	double beta4;
	double stddev;
};

///Simple struct to hold mandator lane changing parameters
struct MandLaneChgParam {
	double feet_lowbound;
	double feet_delta;
	double lane_coeff;
	double congest_coeff;
	double lane_mintime;
};

const GapAcceptParam GA_parameters[4] = {
//	      scale alpha lambda beta0  beta1  beta2  beta3  beta4  stddev
		{ 1.00, 0.0, 0.000, 0.508, 0.000, 0.000,-0.420, 0.000, 0.488},	//Discretionary,lead
		{ 1.00, 0.0, 0.000, 2.020, 0.000, 0.000, 0.153, 0.188, 0.526},	//Discretionary,lag
		{ 1.00, 0.0, 0.000, 0.384, 0.000, 0.000, 0.000, 0.000, 0.859},	//Mandatory,lead
		{ 1.00, 0.0, 0.000, 0.587, 0.000, 0.000, 0.048, 0.356, 1.073}	//Mandatory,lag
};

const MandLaneChgParam MLC_parameters = {
		1320.0,		//feet, lower bound
	    5280.0,		//feet, delta
		   0.5,		//coef for number of lanes
		   1.0,		//coef for congestion level
		   1.0		//minimum time in lane
};

//Helper struct
template <class T>
struct LeadLag {
	T lead;
	T lag;
};

//Unit conversion
double meter2Feet(double meter) {
	return meter*3.2808399;
}
double feet2Meter(double feet) { //Note: This function is now in two locations.
	return feet*0.3048;
}



} //End anon namespace



double sim_mob::MITSIM_LC_Model::lcCriticalGap(DriverUpdateParams& p, int type,	double dis_, double spd_, double dv_)
{
	double k=( type < 2 ) ? 1 : 5;
	return k*-dv_ * p.elapsedSeconds;

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


LaneSide sim_mob::MITSIM_LC_Model::gapAcceptance(DriverUpdateParams& p, int type)
{
	//[0:left,1:right]
	LeadLag<double> otherSpeed[2];		//the speed of the closest vehicle in adjacent lane
	LeadLag<double> otherDistance[2];	//the distance to the closest vehicle in adjacent lane

	const Lane* adjacentLanes[2] = {p.leftLane, p.rightLane};
	const Driver* fwd;
	const Driver* back;
	for(int i=0;i<2;i++){
		fwd = (i==0) ? p.nvLeftFwd.driver : p.nvRightFwd.driver;
		back = (i==0) ? p.nvLeftBack.driver : p.nvRightBack.driver;

		if(adjacentLanes[i]){	//the left/right side exists

			if(!fwd||!fwd->getVehicle()) {		//no vehicle ahead on current lane
				otherSpeed[i].lead=MAX_NUM;
				otherDistance[i].lead=MAX_NUM;
			} else {				//has vehicle ahead
				otherSpeed[i].lead = fwd->getVehicle()->getVelocity();
				otherDistance[i].lead=(i==0)? p.nvLeftFwd.distance : p.nvRightFwd.distance;
			}

			if(!back||!back->getVehicle()){//no vehicle behind
				otherSpeed[i].lag=-MAX_NUM;
				otherDistance[i].lag=MAX_NUM;
			}
			else{		//has vehicle behind, check the gap
				otherSpeed[i].lag=back->getVehicle()->getVelocity();
				otherDistance[i].lag=(i==0)? p.nvLeftBack.distance : p.nvRightBack.distance;
			}
		} else {			// no left/right side exists
			otherSpeed[i].lead    = -MAX_NUM;
			otherDistance[i].lead = 0;
			otherSpeed[i].lag     = MAX_NUM;
			otherDistance[i].lag  = 0;
		}
	}

	//[0:left,1:right]
	LeadLag<bool> flags[2];
	for(int i=0;i<2;i++){	//i for left / right
		for(int j=0;j<2;j++){	//j for lead / lag
			if (j==0) {
				double v      = p.perceivedFwdVelocity;
				double dv     = otherSpeed[i].lead - v;
				flags[i].lead = (otherDistance[i].lead > lcCriticalGap(p, j+type,p.dis2stop,v,dv));
			} else {

				double v 	 = otherSpeed[i].lag;
				double dv 	 = p.perceivedFwdVelocity - otherSpeed[i].lag;
				flags[i].lag = (otherDistance[i].lag > lcCriticalGap(p, j+type,p.dis2stop,v,dv));
			}
		}
	}

	//Build up a return value.
	LaneSide returnVal = {false, false};
	if ( flags[0].lead && flags[0].lag ) {
		returnVal.left = true;
	}
	if ( flags[1].lead && flags[1].lag ) {
		returnVal.right = true;
	}

	return returnVal;
}

double sim_mob::MITSIM_LC_Model::calcSideLaneUtility(DriverUpdateParams& p, bool isLeft){
	if(isLeft && !p.leftLane) {
		return -MAX_NUM;	//has no left side
	} else if(!isLeft && !p.rightLane){
		return -MAX_NUM;    //has no right side
	}
	return (isLeft) ? p.nvLeftFwd.distance : p.nvRightFwd.distance;
}

LANE_CHANGE_SIDE sim_mob::MITSIM_LC_Model::makeDiscretionaryLaneChangingDecision(DriverUpdateParams& p)
{
	// for available gaps(including current gap between leading vehicle and itself), vehicle will choose the longest
	const LaneSide freeLanes = gapAcceptance(p, DLC);
	if(!freeLanes.left && !freeLanes.right) {
		return LCS_SAME;		//neither gap is available, stay in current lane
	}
	double s = p.nvFwd.distance;
	const double satisfiedDistance = 5000;
	if(s>satisfiedDistance) {
		return LCS_SAME;	// space ahead is satisfying, stay in current lane
	}

	//calculate the utility of both sides
	double leftUtility = calcSideLaneUtility(p, true);
	double rightUtility = calcSideLaneUtility(p, false);

	//to check if their utilities are greater than current lane
	bool left = ( s < leftUtility );
	bool right = ( s < rightUtility );

	//decide
	if(freeLanes.rightOnly() && right) {
		return LCS_RIGHT;
	}
	if(freeLanes.leftOnly() && left) {
		return LCS_LEFT;
	}
	if(freeLanes.both()){
		if(right && left){
			return (leftUtility < rightUtility) ? LCS_LEFT : LCS_RIGHT;	//both side is available, choose the better one
		}
		if(right && !left) {
			return LCS_LEFT;
		}
		if(!right && left) {
			return LCS_RIGHT;
		}
		if(!right && !left) {
			return LCS_SAME;
		}
	}
	return LCS_SAME;
}

double sim_mob::MITSIM_LC_Model::checkIfMandatory(DriverUpdateParams& p)
{
	if(p.fromLaneIndex == p.currLaneIndex)
		p.dis2stop = 1000;//defalut 1000m
	//The code below is MITSIMLab model
	double num		=	1;		//now we just assume that MLC only need to change to the adjacent lane
	double y		=	0.5;	//segment density/jam density, now assume that it is 0.5
	//double delta0	=	feet2Unit(MLC_parameters.feet_lowbound);
	double dis2stop_feet = meter2Feet(p.dis2stop);
	double dis		=	dis2stop_feet - MLC_parameters.feet_lowbound;
	double delta	=	1.0 + MLC_parameters.lane_coeff * num + MLC_parameters.congest_coeff * y;
	delta *= MLC_parameters.feet_delta;
	return exp(-dis * dis / (delta * delta));
}

LANE_CHANGE_SIDE sim_mob::MITSIM_LC_Model::makeMandatoryLaneChangingDecision(DriverUpdateParams& p)
{
	LaneSide freeLanes = gapAcceptance(p, MLC);

	//find which lane it should get to and choose which side to change
	//now manually set to 1, it should be replaced by target lane index
	//i am going to fix it.
	int direction = p.fromLaneIndex - p.currLaneIndex;
	//direction = 0; //Otherwise drivers always merge.

	//current lane is target lane
	if(direction==0) {
		return LCS_SAME;
	}

	//current lane isn't target lane
	if(freeLanes.right && direction<0) {		//target lane on the right and is accessable
		p.isWaiting=false;
		return LCS_RIGHT;
	} else if(freeLanes.left && direction>0) {	//target lane on the left and is accessable
		p.isWaiting=false;
		return LCS_LEFT;
	} else {			//when target side isn't available,vehicle will decelerate to wait a proper gap.
		p.isWaiting=true;
		return LCS_SAME;
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
double sim_mob::MITSIM_LC_Model::executeLaneChanging(DriverUpdateParams& p, double totalLinkDistance, double vehLen, LANE_CHANGE_SIDE currLaneChangeDir)
{
	//Behavior changes depending on whether or not we're actually changing lanes.
	if(currLaneChangeDir != LCS_SAME) { //Performing a lane change.
		//Set the lateral velocity of the vehicle; move it.
		int lcsSign = (currLaneChangeDir==LCS_RIGHT) ? -1 : 1;
		return lcsSign*p.laneChangingVelocity;
	} else {
		//If too close to node, don't do lane changing, distance should be larger than 3m
		if(p.currLaneLength - p.currLaneOffset - vehLen/2 <= 300) {
			return -1;
		}

		//Get a random number, use it to determine if we're making a discretionary or a mandatory lane change
		boost::uniform_int<> zero_to_max(0, RAND_MAX);
		double randNum = (double)(zero_to_max(p.gen)%1000)/1000;
		double mandCheck = checkIfMandatory(p);
		LANE_CHANGE_MODE changeMode;  //DLC or MLC
		if(randNum<mandCheck){
			changeMode = MLC;
		} else {
			changeMode = DLC;
			p.dis2stop = 1000;//MAX_NUM;		//no crucial point ahead
		}

		//make decision depending on current lane changing mode
		LANE_CHANGE_SIDE decision = LCS_SAME;
		if(changeMode==DLC) {
			decision = makeDiscretionaryLaneChangingDecision(p);
		} else {
			decision = makeMandatoryLaneChangingDecision(p);
		}


		//Finally, if we've decided to change lanes, set our intention.
		if(decision!=LCS_SAME) {
			const int lane_shift_velocity = 150;  //TODO: What is our lane changing velocity? Just entering this for now...
			return decision==LCS_LEFT?lane_shift_velocity:-lane_shift_velocity;
		}
	}
	return -1;
}
