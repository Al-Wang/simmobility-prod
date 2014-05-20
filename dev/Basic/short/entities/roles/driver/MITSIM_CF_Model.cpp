//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

/*
 * CF_Model.cpp
 *
 *  Created on: 2011-8-15
 *      Author: wangxy & Li Zhemin
 */

#include <boost/random.hpp>

#include <limits>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "entities/vehicle/Vehicle.hpp"
#include "entities/models/CarFollowModel.hpp"
#include "util/Math.hpp"
#include "util/Utils.hpp"
#include "Driver.hpp"

using std::numeric_limits;
using namespace sim_mob;
using namespace std;


namespace {
//Random number generator
//TODO: We need a policy on who can get a generator and why.
//boost::mt19937 gen;

//Threshold defaults
//const double hBufferUpper			=	  1.6;	 ///< upper threshold of headway
//const double hBufferLower			=	  0.8;	 ///< lower threshold of headway

//Set default data for acceleration
//const double maxAcceleration = 5.0;   ///< 5m/s*s, might be tunable later
//const double normalDeceleration = -maxAcceleration*0.6;
//const double maxDeceleration = -maxAcceleration;

//Simple conversion
double feet2Unit(double feet) {
	return feet*0.158;
}

////Simple struct to hold Car Following model parameters
//struct CarFollowParam {
//	double alpha;
//	double beta;
//	double gama;
//	double lambda;
//	double rho;
//	double stddev;
//};

////Car following parameters for this model.
//const CarFollowParam CF_parameters[2] = {
////    alpha   beta    gama    lambda  rho     stddev
//	{ 0.0400, 0.7220, 0.2420, 0.6820, 0.6000, 0.8250},
//	{-0.0418, 0.0000, 0.1510, 0.6840, 0.6800, 0.8020}
//};

//const double targetGapAccParm[] = {0.604, 0.385, 0.323, 0.0678, 0.217,
//		0.583, -0.596, -0.219, 0.0832, -0.170, 1.478, 0.131, 0.300};

////Acceleration mode// CLA@04/2014 this enum can be deleted
//enum ACCEL_MODE {
//	AM_VEHICLE = 0,
//	AM_PEDESTRIAN = 1,
//	AM_TRAFF_LIGHT = 2,
//	AM_NONE = 3
//};

double uRandom(boost::mt19937& gen)
{
	boost::uniform_int<> dist(0, RAND_MAX);
	long int seed_ = dist(gen);

	const long int M = 2147483647;  // M = modulus (2^31)
	const long int A = 48271;       // A = multiplier (was 16807)
	const long int Q = M / A;
	const long int R = M % A;
	seed_ = A * (seed_ % Q) - R * (seed_ / Q);
	seed_ = (seed_ > 0) ? (seed_) : (seed_ + M);
	return (double)seed_ / (double)M;
}

double nRandom(boost::mt19937& gen, double mean,double stddev)
{
	   double r1 = uRandom(gen), r2 = uRandom(gen);
	   double r = - 2.0 * log(r1);
	   if (r > 0.0) return (mean + stddev * sqrt(r) * sin(2 * 3.1415926 * r2));
	   else return (mean);
}


double CalcHeadway(double space, double speed, double elapsedSeconds, double maxAcceleration)
{
	if (speed == 0) {
		return 2 * space * 100000;
	} else {
		return 2 * space / (speed+speed+elapsedSeconds*maxAcceleration);
	}
}

} //End anon namespace

/*
 *--------------------------------------------------------------------
 * The acceleration model calculates the acceleration rate based on
 * interaction with other vehicles. The function returns a the
 * most restrictive acceleration (deceleration if negative) rate
 * among the rates given by several constraints.
 *--------------------------------------------------------------------
 */
sim_mob::MITSIM_CF_Model::MITSIM_CF_Model()
	:timeStep(0.0)
{
	modelName = "general_driver_model";
	splitDelimiter = " ,";
	initParam();
}
void sim_mob::MITSIM_CF_Model::initParam()
{
	// speed scaler
	string speedScalerStr,maxAccStr,decelerationStr,maxAccScaleStr,normalDecScaleStr,maxDecScaleStr;
	ParameterManager::Instance()->param(modelName,"speed_scaler",speedScalerStr,string("5 20 20"));
	// max acceleration
	ParameterManager::Instance()->param(modelName,"max_acc_car1",maxAccStr,string("10.00  7.90  5.60  4.00  4.00"));
	makeSpeedIndex(Vehicle::CAR,speedScalerStr,maxAccStr,maxAccIndex,maxAccUpperBound);
	ParameterManager::Instance()->param(modelName,"max_acceleration_scale",maxAccScaleStr,string("0.6 0.7 0.8 0.9 1.0 1.1 1.2 1.3 1.4 1.5"));
	makeScaleIdx(maxAccScaleStr,maxAccScale);
	// normal deceleration
	ParameterManager::Instance()->param(modelName,"normal_deceleration_car1",decelerationStr,string("7.8 	6.7 	4.8 	4.8 	4.8"));
	makeSpeedIndex(Vehicle::CAR,speedScalerStr,decelerationStr,normalDecelerationIndex,normalDecelerationUpperBound);
	ParameterManager::Instance()->param(modelName,"normal_deceleration_scale",normalDecScaleStr,string("1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0"));
	makeScaleIdx(maxAccScaleStr,normalDecelerationScale);
	// max deceleration
	ParameterManager::Instance()->param(modelName,"Max_deceleration_car1",decelerationStr,string("16.0   14.5   13.0   11.0   10.0"));
	makeSpeedIndex(Vehicle::CAR,speedScalerStr,decelerationStr,maxDecelerationIndex,maxDecelerationUpperBound);
	ParameterManager::Instance()->param(modelName,"max_deceleration_scale",maxDecScaleStr,string("1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.0"));
	makeScaleIdx(maxDecScaleStr,maxDecelerationScale);
	// acceleration grade factor
	ParameterManager::Instance()->param(modelName,"acceleration_grade_factor",accGradeFactor,0.305);
	ParameterManager::Instance()->param(modelName,"tmp_all_grades",tmpGrade,0.0);
	// param for distanceToNormalStop()
	ParameterManager::Instance()->param(modelName,"min_speed",minSpeed,0.1);
	ParameterManager::Instance()->param(modelName,"min_response_distance",minResponseDistance,5.0);
	// param of calcSignalRate()
	ParameterManager::Instance()->param(modelName,"yellow_stop_headway",yellowStopHeadway,1.0);
	ParameterManager::Instance()->param(modelName,"min_speed_yellow",minSpeedYellow,2.2352);
	// param of carFollowingRate()
	ParameterManager::Instance()->param(modelName,"hbuffer_lower",hBufferLower,0.8);
	string hBufferUpperStr;
	ParameterManager::Instance()->param(modelName,"hbuffer_Upper",hBufferUpperStr,string("1.7498 2.2737 2.5871 2.8379 3.0633 3.2814 3.5068 3.7578 4.0718 4.5979"));
	makeScaleIdx(hBufferUpperStr,hBufferUpperScale);
	hBufferUpper = getBufferUppder();
	// Car following parameters
	string cfParamStr;
	ParameterManager::Instance()->param(modelName,"CF_parameters_1",cfParamStr,string("0.0400, 0.7220, 0.2420, 0.6820, 0.6000, 0.8250"));
	makeCFParam(cfParamStr,CF_parameters[0]);
	ParameterManager::Instance()->param(modelName,"CF_parameters_2",cfParamStr,string("-0.0418 0.0000 0.1510 0.6840 0.6800 0.8020"));
	makeCFParam(cfParamStr,CF_parameters[1]);
	//
	string targetGapAccParmStr;
	ParameterManager::Instance()->param(modelName,"target_gap_acc_parm",targetGapAccParmStr,string("0.604, 0.385, 0.323, 0.0678, 0.217,0.583, -0.596, -0.219, 0.0832, -0.170, 1.478, 0.131, 0.300"));
	makeScaleIdx(targetGapAccParmStr,targetGapAccParm);
}
void sim_mob::MITSIM_CF_Model::makeCFParam(string& s,CarFollowParam& cfParam)
{
	std::vector<std::string> arrayStr;
	vector<double> c;
	boost::trim(s);
	boost::split(arrayStr, s, boost::is_any_of(splitDelimiter),boost::token_compress_on);
	for(int i=0;i<arrayStr.size();++i)
	{
		double res;
		try {
				res = boost::lexical_cast<double>(arrayStr[i].c_str());
			}catch(boost::bad_lexical_cast&) {
				std::string str = "can not covert <" +s+"> to double.";
				throw std::runtime_error(str);
			}
			c.push_back(res);
	}
	cfParam.alpha  = c[0];
	cfParam.beta   = c[1];
	cfParam.gama   = c[2];
	cfParam.lambda = c[3];
	cfParam.rho    = c[4];
	cfParam.stddev = c[5];
}
void sim_mob::MITSIM_CF_Model::makeScaleIdx(string& s,vector<double>& c)
{
	std::vector<std::string> arrayStr;
	boost::trim(s);
	boost::split(arrayStr, s, boost::is_any_of(splitDelimiter),boost::token_compress_on);
	for(int i=0;i<arrayStr.size();++i)
	{
		double res;
		try {
				res = boost::lexical_cast<double>(arrayStr[i].c_str());
			}catch(boost::bad_lexical_cast&) {
				std::string str = "can not covert <" +s+"> to double.";
				throw std::runtime_error(str);
			}
		c.push_back(res);
	}
}
void sim_mob::MITSIM_CF_Model::makeSpeedIndex(VehicleBase::VehicleType vhType,
						string& speedScalerStr,
						string& cstr,
						map< VehicleBase::VehicleType,map<int,double> >& idx,
						int& upperBound)
{
	std::cout<<"makeSpeedIndex: vh type "<<vhType<<std::endl;
	// for example
	// speedScalerStr "5 20 20" ft/sec
	// maxAccStr      "10.00  7.90  5.60  4.00  4.00" ft/(s^2)
	std::vector<std::string> arrayStr;
	boost::trim(speedScalerStr);
	boost::split(arrayStr, speedScalerStr, boost::is_any_of(splitDelimiter),boost::token_compress_on);
	std::vector<double> speedScalerArrayDouble;
	for(int i=0;i<arrayStr.size();++i)
	{
		double res;
		try {
				res = boost::lexical_cast<double>(arrayStr[i].c_str());
			}catch(boost::bad_lexical_cast&) {
				std::string str = "can not covert <" +speedScalerStr+"> to double.";
				throw std::runtime_error(str);
			}
			speedScalerArrayDouble.push_back(res);
	}
	arrayStr.clear();
	//
	boost::algorithm::trim(cstr);
//	std::vector<std::string> maxAccArrayStr;
	boost::split(arrayStr, cstr, boost::is_any_of(splitDelimiter),boost::token_compress_on);
	std::vector<double> cArrayDouble;
	for(int i=0;i<arrayStr.size();++i)
	{
		double res;
		try {
				res = boost::lexical_cast<double>(arrayStr[i].c_str());
			}catch(boost::bad_lexical_cast&) {
				std::string str = "can not covert <" +cstr+"> to double.";
				throw std::runtime_error(str);
			}
			cArrayDouble.push_back(res);
	}
	//
	upperBound = round(speedScalerArrayDouble[1] * (speedScalerArrayDouble[0]-1));
	map<int,double> cIdx;
	for(int speed=0;speed<=upperBound;++speed)
	{
		double maxAcc;
		// Convert speed value to a table index.
		int j = speed/speedScalerArrayDouble[1];
		if(j>=(speedScalerArrayDouble[0]-1))
		{
			maxAcc = cArrayDouble[speedScalerArrayDouble[0]-1];
		}
		else
		{
			maxAcc = cArrayDouble[j];
		}
		cIdx.insert(std::make_pair(speed, maxAcc ));

//		std::cout<<"speed: "<<speed<<" max acc: "<<maxAcc<<std::endl;
	}

	idx[vhType] = cIdx;
}
double sim_mob::MITSIM_CF_Model::getMaxAcceleration(sim_mob::DriverUpdateParams& p,VehicleBase::VehicleType vhType)
{
	if(!p.driver)
	{
		throw std::runtime_error("no driver");
	}

	// convert speed to int
	int speed  = round(p.perceivedFwdVelocity/100);
	if(speed <0) speed=0;
	if(speed >maxAccUpperBound) speed=maxAccUpperBound;

	double maxTableAcc = maxAccIndex[vhType][speed];

	// TODO: get random multiplier from data file and normal distribution

	double maxAcc = ( maxTableAcc - tmpGrade*accGradeFactor) * getMaxAccScale();

	return maxAcc;
}
double sim_mob::MITSIM_CF_Model::getNormalDeceleration(sim_mob::DriverUpdateParams& p,VehicleBase::VehicleType vhType)
{
	if(!p.driver)
	{
		throw std::runtime_error("no driver");
	}

	// convert speed to int
	int speed  = round(p.perceivedFwdVelocity/100);
	if(speed <0) speed=0;
	if(speed >normalDecelerationUpperBound) speed=normalDecelerationUpperBound;

	double normalDec = normalDecelerationIndex[vhType][speed];

	double dec = ( normalDec - tmpGrade*accGradeFactor) * getNormalDecScale();

	return dec;
}
double sim_mob::MITSIM_CF_Model::getMaxDeceleration(sim_mob::DriverUpdateParams& p,VehicleBase::VehicleType vhType)
{
	if(!p.driver)
	{
		throw std::runtime_error("no driver");
	}

	// convert speed to int
	int speed  = round(p.perceivedFwdVelocity/100);
	if(speed <0) speed=0;
	if(speed >maxDecelerationUpperBound) speed=maxDecelerationUpperBound;

	double maxDec = maxDecelerationIndex[vhType][speed];

	double dec = ( maxDec - tmpGrade*accGradeFactor) * getMaxDecScale();

	return dec;
}
double sim_mob::MITSIM_CF_Model::getMaxAccScale()
{
	// get random number (uniform distribution), as Random::urandom(int n) in MITSIM Random.cc
	int scaleNo = Utils::generateInt(0,maxAccScale.size()-1);
	// return max acc scale,as maxAccScale() in MITSIM TS_Parameter.h
	return maxAccScale[scaleNo];
}
double sim_mob::MITSIM_CF_Model::getNormalDecScale()
{
	// get random number (uniform distribution), as Random::urandom(int n) in MITSIM Random.cc
	int scaleNo = Utils::generateInt(0,normalDecelerationScale.size()-1);
	// return normal dec scale,as maxAccScale() in MITSIM TS_Parameter.h
	return normalDecelerationScale[scaleNo];
}
double sim_mob::MITSIM_CF_Model::getMaxDecScale()
{
	// get random number (uniform distribution), as Random::urandom(int n) in MITSIM Random.cc
	int scaleNo = Utils::generateInt(0,maxDecelerationScale.size()-1);
	// return max dec scale,as maxAccScale() in MITSIM TS_Parameter.h
	return maxDecelerationScale[scaleNo];
}
double sim_mob::MITSIM_CF_Model::getBufferUppder()
{
	// get random number (uniform distribution), as Random::urandom(int n) in MITSIM Random.cc
	int scaleNo = Utils::generateInt(0,hBufferUpperScale.size()-1);
	// return max acc scale,as maxAccScale() in MITSIM TS_Parameter.h
	return hBufferUpperScale[scaleNo];
}
double sim_mob::MITSIM_CF_Model::makeAcceleratingDecision(DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed)
{
	//initiate
//	distanceToNormalStop(p);

	// VARIABLE || FUNCTION ||				REGIME
	calcStateBasedVariables(p);

	double acc = maxAcceleration;
	double aB = calcMergingRate(p);
	double aC = calcSignalRate(p);			// near signal or incidents
	double aD = calcYieldingRate(p, targetSpeed, maxLaneSpeed); // when yielding
	double aE = waitExitLaneRate(p);		//
	double  aF = waitAllowedLaneRate(p);
//	double  aG = calcLaneDropRate(p);		// MISSING! > NOT YET IMPLEMENTED (@CLA_04/14)
	double aH1 = calcAdjacentRate(p);		// to reach adjacent gap
	double aH2 = calcBackwardRate(p);		// to reach backward gap
	double aH3 = calcForwardRate(p);		// to reach forward gap
	// The target gap acceleration should be based on the target gap status and not on the min
	// MISSING! > NOT YET IMPLEMENTED (@CLA_04/14)
	/*
	if (status(STATUS_ADJACENT)) {
	    double aH = calcAdjacentRate(p);	// to reach adjacent gap
	  }
	  else if (status(STATUS_BACKWARD)) {
	    double aH = calcBackwardRate(p);	// to reach backward gap
	  }
	  else if (status(STATUS_FORWARD)) {
		double aH = calcForwardRate(p);		// to reach forward gap
	  } else {
	    double aH = desiredSpeedRate(p); // FUNCTION desiredSpeedRate MISSING! > NOT YET IMPLEMENTED (@CLA_04/14)
	  }
	*/

	// if (intersection){
	// double aI = approachInter(p); // when approaching intersection to achieve the turn speed
	// if(acc > aI) acc = aI;
	// }
	// FUNCTION approachInter MISSING! > NOT YET IMPLEMENTED (@CLA_04/14)

	double aZ1 = carFollowingRate(p, targetSpeed, maxLaneSpeed, p.nvFwd);
	double aZ2 = carFollowingRate(p, targetSpeed, maxLaneSpeed, p.nvFwdNextLink);

	// Make decision
	// Use the smallest
	//	if(acc > aB) acc = aB;
	if(acc > aD) acc = aD;
	//if(acc > aF) acc = aF;
	if(acc > aH1) acc = aH1;
	if(acc > aH2) acc = aH2;
	if(acc > aH3) acc = aH3;
	//if(acc > aG) acc = aG;
	if(acc > aC) acc = aC;
	if(acc > aE) acc = aE;
	if(acc > aZ1) acc = aZ1;
	if(acc > aZ2) acc = aZ2;

	// SEVERAL CONDITONS MISSING! > NOT YET IMPLEMENTED (@CLA_04/14)

	return acc;
}

/*
 *--------------------------------------------------------------------
 * Calculate acceleration rate by car-following constraint. This
 * function may also be used in the lane changing algorithm to find
 * the potential acceleration rate in neighbor lanes.
 *
 * CAUTION: The two vehicles concerned in this function may not
 * necessarily be in the same lane or even the same segment.
 *
 * A modified GM model is used in this implementation.
 *--------------------------------------------------------------------
 */
double sim_mob::MITSIM_CF_Model::carFollowingRate(DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed,NearestVehicle& nv)
{
	p.space = p.perceivedDistToFwdCar/100;

	double res = 0;
	//If we have no space left to move, immediately cut off acceleration.
//	if ( p.space < 2.0 && p.isAlreadyStart )
//		return maxDeceleration;
	if(p.space<2.0 && p.isAlreadyStart && p.isBeforIntersecton && p.perceivedFwdVelocityOfFwdCar/100 < 1.0)
	{
		return maxDeceleration*4.0;
	}
	if(p.space > 0) {
		if(!nv.exists()) {
			return accOfFreeFlowing(p, targetSpeed, maxLaneSpeed);
		}
		// when nv is left/right vh , can not use perceivedxxx!
//		p.v_lead = p.perceivedFwdVelocityOfFwdCar/100;
//		p.a_lead = p.perceivedAccelerationOfFwdCar/100;

		p.v_lead = nv.driver->fwdVelocity/100;
		p.a_lead = nv.driver->fwdAccel/100;

		double dt	=	p.elapsedSeconds;
		double headway = CalcHeadway(p.space, p.perceivedFwdVelocity/100, p.elapsedSeconds, maxAcceleration);
//		std::cout<<"carFollowingRate: headway1: "<<headway<<std::endl;

		//Emergency deceleration overrides the perceived distance; check for it.
		{
//			double emergSpace = p.perceivedDistToFwdCar/100;
			double emergSpace = nv.distance/100;
			double emergHeadway = CalcHeadway(emergSpace, p.perceivedFwdVelocity/100, p.elapsedSeconds, maxAcceleration);
			if (emergHeadway < hBufferLower) {
				//We need to brake. Override.
				p.space = emergSpace;
				headway = emergHeadway;
			}
		}

		p.space_star	=	p.space + p.v_lead * dt + 0.5 * p.a_lead * dt * dt;
//		std::cout<<"carFollowingRate: headway2: "<<headway<<std::endl;
		if(headway < hBufferLower) {
			res = accOfEmergencyDecelerating(p);
//			std::cout<<"carFollowingRate: EmergencyDecelerating: "<<res<<std::endl;
		}
		hBufferUpper = getBufferUppder();
		if(headway > hBufferUpper) {
			res = accOfMixOfCFandFF(p, targetSpeed, maxLaneSpeed);
		}
		if(headway <= hBufferUpper && headway >= hBufferLower) {
			res = accOfCarFollowing(p);
		}

//		if(p.isWaiting && p.dis2stop<5000 && res > 0)
//		{
//			res=res*(5000.0-p.dis2stop)/5000.0;
//			if(p.dis2stop<2000)
//			{
//				res=0;
//			}
//		}
	}
	return res;
}

double sim_mob::MITSIM_CF_Model::calcMergingRate(sim_mob::DriverUpdateParams& p)
{
	// TS_Vehicles from freeways and on ramps have different
	// priority.  Seperate procedures are applied. (MITSIM TS_CFModels.cc)
	//  if (lane_->linkType() == LINK_TYPE_FREEWAY) {
	DriverMovement *driverMvt = (DriverMovement*)p.driver->Movement();
	if(driverMvt->fwdDriverMovement.getCurrSegment()->type == sim_mob::LINK_TYPE_FREEWAY)
	{

	}
	//	first = vehicleAheadInTypedLanes(LINK_TYPE_FREEWAY);

}
double sim_mob::MITSIM_CF_Model::calcSignalRate(DriverUpdateParams& p)
{
	double minacc = maxAcceleration;
//	double yellowStopHeadway = 1; //1 second
//	double minSpeedYellow = 2.2352;//5 mph = 2.2352 m / s

	sim_mob::TrafficColor color;

#if 0
	Signal::TrafficColor color;
#endif
    double distanceToTrafficSignal;
    distanceToTrafficSignal = p.perceivedDistToTrafficSignal;
    color = p.perceivedTrafficColor;
    double dis = p.perceivedDistToFwdCar;
	if(distanceToTrafficSignal<500)
	{
	double dis = distanceToTrafficSignal/100;

#if 0
		if(p.perceivedTrafficColor == sim_mob::Red)
				{
					double a = brakeToStop(p, dis);
					if(a < minacc)
						minacc = a;
				}
				else if(p.perceivedTrafficColor == sim_mob::Amber)
				{
					double maxSpeed = (speed>minSpeedYellow)?speed:minSpeedYellow;
					if(dis/maxSpeed > yellowStopHeadway)
					{
						double a = brakeToStop(p, dis);
						if(a < minacc)
							minacc = a;
					}
				}
				else if(p.perceivedTrafficColor == sim_mob::Green)
				{
					minacc = maxAcceleration;
				}
#else
		if(color == sim_mob::Red)
#if 0
		if(color == Signal::Red)
#endif
		{
			double a = brakeToStop(p, dis);
			if(a < minacc)
				minacc = a;
		}
			else if(color == sim_mob::Amber)
#if 0
		else if(color == Signal::Amber)
#endif
		{
			double maxSpeed = (p.perceivedFwdVelocity/100>minSpeedYellow)?p.perceivedFwdVelocity/100:minSpeedYellow;
			if(dis/maxSpeed > yellowStopHeadway)
			{
				double a = brakeToStop(p, dis);
				if(a < minacc)
					minacc = a;
			}
		}
		else if(color == sim_mob::Green)
#if 0
		else if(color == Signal::Green)
#endif
		{
			minacc = maxAcceleration;
		}

#endif

	}
	return minacc;
}

double sim_mob::MITSIM_CF_Model::calcYieldingRate(DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed)
{
	if(p.turningDirection == LCS_LEFT)
	{
		return carFollowingRate(p, targetSpeed, maxLaneSpeed, p.nvLeftFwd);
	}
	else if(p.turningDirection == LCS_RIGHT)
	{
		return carFollowingRate(p, targetSpeed, maxLaneSpeed, p.nvRightFwd);
	}
	return maxAcceleration;
}

double sim_mob::MITSIM_CF_Model::waitExitLaneRate(DriverUpdateParams& p)
{
	double dx = p.perceivedDistToFwdCar/100-5;
	if(p.turningDirection == LCS_SAME || dx > p.distanceToNormalStop)
		return maxAcceleration;
	else
		return brakeToStop(p, dx);
}
double sim_mob::MITSIM_CF_Model::waitAllowedLaneRate(sim_mob::DriverUpdateParams& p)
{
	if(p.dis2stop < p.distanceToNormalStop && !p.isTargetLane)
	{
		double acc = brakeToStop(p,p.distanceToNormalStop);
		return acc;
	}
	return maxAcceleration;
}
double sim_mob::MITSIM_CF_Model::calcForwardRate(DriverUpdateParams& p)
{
	/*
	if(p.turningDirection == LCS_SAME)
		return maxAcceleration;
	NearestVehicle& nv = (p.turningDirection == LCS_LEFT)?p.nvLeftFwd:p.nvRightFwd;
	*/

	if(p.targetGap != TG_Left_Fwd || p.targetGap!= TG_Right_Fwd)
		return maxAcceleration;
	NearestVehicle& nv = (p.targetGap == TG_Left_Fwd)?p.nvLeftFwd:p.nvRightFwd;

	if(!nv.exists())
		return maxAcceleration;
	double dis = nv.distance/100 + targetGapAccParm[0];
	double dv = nv.driver->fwdVelocity.get()/100 - p.perceivedFwdVelocity/100;
	double acc = targetGapAccParm[1] * pow(dis, targetGapAccParm[2]);

	if (dv > 0) {
		acc *= pow(dv, targetGapAccParm[3]);
	} else if (dv < 0) {
		acc *= pow (-dv, targetGapAccParm[4]);
	}
	acc += targetGapAccParm[5] /0.824 ;
	return acc;
}

double sim_mob::MITSIM_CF_Model::calcBackwardRate(DriverUpdateParams& p)
{
	/*
	if(p.turningDirection == LCS_SAME)
		return maxAcceleration;
	//NearestVehicle& nv = (p.turningDirection == LCS_LEFT)?p.nvLeftFwd:p.nvRightFwd;
	NearestVehicle& nv = (p.turningDirection == LCS_LEFT)?p.nvLeftBack:p.nvRightBack;//change a mistake!!!
	*/

	if(p.targetGap != TG_Left_Back || p.targetGap!= TG_Right_Back)
		return maxAcceleration;
	NearestVehicle& nv = (p.targetGap == TG_Left_Back)?p.nvLeftBack:p.nvRightBack;

	if(!nv.exists())
		return maxAcceleration;

	double dis = nv.distance/100 + targetGapAccParm[0];
	double dv = nv.driver->fwdVelocity.get()/100 - p.perceivedFwdVelocity/100;

	double acc = targetGapAccParm[6] * pow(dis, targetGapAccParm[7]);

	if (dv > 0) {
		acc *= pow(dv, targetGapAccParm[8]);
	} else if (dv < 0) {
		acc *= pow (-dv, targetGapAccParm[9]);
	}
	acc += targetGapAccParm[10] / 0.824 ;
	return acc;
}

double sim_mob::MITSIM_CF_Model::calcAdjacentRate(DriverUpdateParams& p)
{
	if(p.nextLaneIndex == p.currLaneIndex)
		return maxAcceleration;
	NearestVehicle& av = (p.nextLaneIndex > p.currLaneIndex)?p.nvLeftFwd:p.nvRightFwd;
	NearestVehicle& bv = (p.nextLaneIndex > p.currLaneIndex)?p.nvLeftBack:p.nvRightBack;
	if(!av.exists())
		return maxAcceleration;
	if(!bv.exists())
		return normalDeceleration;
	double gap = bv.distance/100 + av.distance/100;
	double position = bv.distance/100;
	double acc = targetGapAccParm[11] * (targetGapAccParm[0] * gap - position);

	acc += targetGapAccParm[12] / 0.824 ;
	return acc;
}
/*
 *-------------------------------------------------------------------
 * This function returns the acceleration rate required to
 * accelerate / decelerate from current speed to a full
 * stop within a given distance.
 *-------------------------------------------------------------------
 */
double sim_mob::MITSIM_CF_Model::brakeToStop(DriverUpdateParams& p, double dis)
{
//	double DIS_EPSILON =	0.001;
	if (dis > sim_mob::Math::DOUBLE_EPSILON) {
		double u2 = (p.perceivedFwdVelocity/100)*(p.perceivedFwdVelocity/100);
		double acc = - u2 / dis * 0.5;
		if (acc <= normalDeceleration)
			return acc;
		double dt = p.elapsedSeconds;
		double vt = p.perceivedFwdVelocity/100 * dt;
		double a = dt * dt;
		double b = 2.0 * vt - normalDeceleration * a;
		double c = u2 + 2.0 * normalDeceleration * (dis - vt);
		double d = b * b - 4.0 * a * c;

		if (d < 0 || a <= 0.0) return acc;

		return (sqrt(d) - b) / a * 0.5;
	}
	else {

		double dt = p.elapsedSeconds;
		return (dt > 0.0) ? -(p.perceivedFwdVelocity/100) / dt : maxDeceleration;
	}
}


/*
 *-------------------------------------------------------------------
 * This function returns the acceleration rate required to
 * accelerate / decelerate from current speed to a target
 * speed within a given distance.
 *-------------------------------------------------------------------
 */
double sim_mob::MITSIM_CF_Model::brakeToTargetSpeed(DriverUpdateParams& p,double s,double v)
{
//	double v 			=	p.perceivedFwdVelocity/100;
	double dt			=	p.elapsedSeconds;

//	//NOTE: This is the only use of epsilon(), so I just copied the value directly.
//	//      See LC_Model for how to declare a private temporary variable. ~Seth
//	if(p.space_star > sim_mob::Math::DOUBLE_EPSILON) {
//		return  ((p.v_lead + p.a_lead * dt ) * ( p.v_lead + p.a_lead * dt) - v * v) / 2 / p.space_star;
//	} else if ( dt <= 0 ) {
//		return maxAcceleration;
//	} else {
//		return ( p.v_lead + p.a_lead * dt - v ) / dt;
//	}

	double currentSpeed_ 	=	p.perceivedFwdVelocity/100;
	if (s > sim_mob::Math::DOUBLE_EPSILON) {
		float v2 = v * v;
		float u2 = currentSpeed_ * currentSpeed_;
		float acc = (v2 - u2) / s * 0.5;

		return acc;
	}
	else {
	//	float dt = nextStepSize();
//		if (dt <= 0.0) return maxAcceleration ;
		return (v - currentSpeed_) / dt;
	}
}

double sim_mob::MITSIM_CF_Model::accOfEmergencyDecelerating(DriverUpdateParams& p)
{
	double v 			=	p.perceivedFwdVelocity/100;
	double dv			=	v-p.v_lead;
	double epsilon_v	=	sim_mob::Math::DOUBLE_EPSILON;
	double aNormalDec	=	normalDeceleration;

	double a;
	if( dv < epsilon_v ) {
		a = p.a_lead + 0.25*aNormalDec;
	} else if (p.space > 0.01 ) {
		a = p.a_lead - dv * dv / 2 / p.space;
	} else {
		double dt	=	p.elapsedSeconds;
		//p.space_star	=	p.space + p.v_lead * dt + 0.5 * p.a_lead * dt * dt;
		double s = p.space_star;
		double v = p.v_lead + p.a_lead * dt ;
		a= brakeToTargetSpeed(p,s,v);
	}
	return min(normalDeceleration, a);
}



double sim_mob::MITSIM_CF_Model::accOfCarFollowing(DriverUpdateParams& p)
{
	const double density	=	1;		//represent the density of vehicles in front of the subject vehicle
										//now we ignore it, assuming that it is 1.
	double v				=	p.perceivedFwdVelocity/100;
	int i = (v > p.v_lead) ? 1 : 0;
	double dv =(v > p.v_lead)?(v-p.v_lead):(p.v_lead - v);

	double res = CF_parameters[i].alpha * pow(v , CF_parameters[i].beta) /pow(p.nvFwd.distance/100 , CF_parameters[i].gama);
	res *= pow(dv , CF_parameters[i].lambda)*pow(density,CF_parameters[i].rho);
	res += feet2Unit(nRandom(p.gen, 0, CF_parameters[i].stddev));

	return res;
}

double sim_mob::MITSIM_CF_Model::accOfFreeFlowing(DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed)
{
	double vn =	p.perceivedFwdVelocity/100;
	if (vn < targetSpeed) {
		return (vn<maxLaneSpeed) ? maxAcceleration : 0;
	} else if (vn > targetSpeed) {
		return 0;
	}

	//If equal:
	return (vn<maxLaneSpeed) ? maxAcceleration: 0;
}

double sim_mob::MITSIM_CF_Model::accOfMixOfCFandFF(DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed)
{
	if(p.space > p.distanceToNormalStop ) {
		return accOfFreeFlowing(p, targetSpeed, maxLaneSpeed);
	} else {
		double dt	=	p.elapsedSeconds;
		//p.space_star	=	p.space + p.v_lead * dt + 0.5 * p.a_lead * dt * dt;
		double s = p.space_star;
		double v = p.v_lead + p.a_lead * dt ;
		return brakeToTargetSpeed(p,s,v);
	}
}

void sim_mob::MITSIM_CF_Model::distanceToNormalStop(DriverUpdateParams& p)
{
//	double minSpeed = 0.1;
//	double minResponseDistance = 5;
//	double DIS_EPSILON = 0.001;
	if (p.perceivedFwdVelocity/100 > minSpeed) {
		p.distanceToNormalStop = sim_mob::Math::DOUBLE_EPSILON -
				0.5 * (p.perceivedFwdVelocity/100) * (p.perceivedFwdVelocity/100) / normalDeceleration;
		if (p.distanceToNormalStop < minResponseDistance) {
			p.distanceToNormalStop = minResponseDistance;
		}
	} else {
		p.distanceToNormalStop = minResponseDistance;
	}
}
void sim_mob::MITSIM_CF_Model::calcStateBasedVariables(DriverUpdateParams& p)
{
//	double dt	=	p.elapsedSeconds;
	timeStep -= p.elapsedSeconds;
	/// if time step >0 ,no need update variables
	if(timeStep>0) return;

	distanceToNormalStop(p);

	// Acceleration rate for a vehicle (a function of vehicle type,
	// facility type, segment grade, current speed).
	maxAcceleration    = getMaxAcceleration(p);
	// Deceleration rate for a vehicle (a function of vehicle type, and
	// segment grade).
	normalDeceleration = getNormalDeceleration(p);
	// Maximum deceleration is function of speed and vehicle class
	maxDeceleration    = getMaxDeceleration(p);

	timeStep = calcNextStepSize();
}
double sim_mob::MITSIM_CF_Model::calcNextStepSize()
{
	return 0.2;
}
