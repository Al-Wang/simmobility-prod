//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include "conf/params/ParameterManager.hpp"
#include "entities/vehicle/VehicleBase.hpp"
#include "entities/roles/driver/Driver.hpp"

#include <string>
#include <set>
#include <map>
#include <boost/random.hpp>

using namespace std;

namespace sim_mob {

//Forward declaration
class DriverUpdateParams;
class NearestVehicle;
class Driver;


/*
 * \file CarFollowModel.hpp
 *
 * \author Wang Xinyuan
 * \author Li Zhemin
 * \author Seth N. Hetu
 */

//Abstract class which describes car following.
class CarFollowModel {
public:
	//Allow propagation of delete
	virtual ~CarFollowModel() {}

	virtual double makeAcceleratingDecision(sim_mob::DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed) = 0;  ///<Decide acceleration

public:
	string modelName;
	double maxAcceleration;
	/// grade factor
	double accGradeFactor;
	double normalDeceleration;
	double maxDeceleration;
};

/**
 *
 * Simple version of the car following model
 * The purpose of this model is to demonstrate a very simple (yet reasonably accurate) model
 * which generates somewhat plausible visuals. This model should NOT be considered valid, but
 * it can be used for demonstrations and for learning how to write your own *Model subclasses.
 *
 * \author Seth N. Hetu
 */
class SimpleCarFollowModel : public CarFollowModel {
public:
	///Decide acceleration. Simply attempts to reach the target speed.
	virtual double makeAcceleratingDecision(sim_mob::DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed) = 0;
};

//MITSIM version of car following model
class MITSIM_CF_Model : public CarFollowModel {
public:
	//Simple struct to hold Car Following model parameters
	struct CarFollowParam {
		double alpha;
		double beta;
		double gama;
		double lambda;
		double rho;
		double stddev;
	};

	MITSIM_CF_Model();
	void initParam();
	/** \brief make index base on speed scaler
	 *  \param speedScalerStr speed scaler
	 *  \param cstr index value
	 *  \param idx  index container
	 *  \param upperBound store upper bound of index
	 **/
	void makeSpeedIndex(VehicleBase::VehicleType vhType,
						string& speedScalerStr,
						string& cstr,
						map< VehicleBase::VehicleType,map<int,double> >& idx,
						int& upperBound);
	/** \brief create scale index base on string data ,like "0.6 0.7 0.8 0.9 1.0 1.1 1.2 1.3 1.4 1.5"
	 *  \param s data string
	 *  \param c container to store data
	 **/
	void makeScaleIdx(string& s,vector<double>& c);
	double getMaxAcceleration(sim_mob::DriverUpdateParams& p,VehicleBase::VehicleType vhType=VehicleBase::CAR);
	double getNormalDeceleration(sim_mob::DriverUpdateParams& p,VehicleBase::VehicleType vhType=VehicleBase::CAR);
	double getMaxDeceleration(sim_mob::DriverUpdateParams& p,VehicleBase::VehicleType vhType=VehicleBase::CAR);
	/** \brief get max acc scaler
	 *  \return scaler value
	 **/
	double getMaxAccScale();
	/** \brief normal deceleration scaler
	 *  \return scaler value
	 **/
	double getNormalDecScale();
	/** \brief max deceleration scaler
	 *  \return scaler value
	 **/
	double getMaxDecScale();
	virtual double makeAcceleratingDecision(sim_mob::DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed);

private:
	double carFollowingRate(sim_mob::DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed,NearestVehicle& nv);
	/**
	 *  /brief Calculate the acceleration rate by merging constraint.
	 *  \param p driver's parameters
	 *  \return acceleration rate
	 */
	double calcMergingRate(sim_mob::DriverUpdateParams& p);
	/** \brief this function calculates the acceleration rate when the car is stopped at a traffic light
	 *  \param p driver's parameters
	 *  \return acceleration rate
	 **/
	double calcSignalRate(sim_mob::DriverUpdateParams& p);
	double calcYieldingRate(sim_mob::DriverUpdateParams& p,double targetSpeed, double maxLaneSpeed);
	/** \brief this function calculates the acceleration rate before exiting a specific lane
	 *  \param p driver's parameters
	 *  \return acceleration rate
	 **/
	double waitExitLaneRate(sim_mob::DriverUpdateParams& p);
	/** \brief this function calculates the acceleration rate when
	 *   current lane is incorrect and the distance is close to the an incurrent lane of the segment
	 *  \param p driver's parameters
	 *  \return acceleration rate
	 **/
	double waitAllowedLaneRate(sim_mob::DriverUpdateParams& p);
	double calcForwardRate(sim_mob::DriverUpdateParams& p);
	double calcBackwardRate(sim_mob::DriverUpdateParams& p);
	double calcAdjacentRate(sim_mob::DriverUpdateParams& p);

	/** \brief return the acc to a target speed within a specific distance
	 *  \param p vehicle state value
	 *  \param s distance (meter)
	 *  \param v velocity (m/s)
	 *  \return acceleration (m/s^2)
	 **/
	double brakeToTargetSpeed(sim_mob::DriverUpdateParams& p,double s,double v);
	double brakeToStop(DriverUpdateParams& p, double dis);
	double accOfEmergencyDecelerating(sim_mob::DriverUpdateParams& p);  ///<when headway < lower threshold, use this function
	double accOfCarFollowing(sim_mob::DriverUpdateParams& p);  ///<when lower threshold < headway < upper threshold, use this function
	double accOfFreeFlowing(sim_mob::DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed);  ///<when upper threshold < headway, use this funcion
	double accOfMixOfCFandFF(sim_mob::DriverUpdateParams& p, double targetSpeed, double maxLaneSpeed); 	///<mix of car following and free flowing
	void distanceToNormalStop(sim_mob::DriverUpdateParams& p);
	/** \brief This function update the variables that depend only on the speed of
	 *         the vehicle and type.
	 *  \param p vehicle state value
	 **/
	void calcStateBasedVariables(DriverUpdateParams& p);
	/** \brief calculate the step size of update state variables
	 *         the vehicle and type.
	 *  \return step size
	 **/
	double calcNextStepSize();

private:
	// split delimiter in xml param file
	string splitDelimiter;
	/// key=vehicle type
	/// submap key=speed, value=max acc
	map< VehicleBase::VehicleType,map<int,double> > maxAccIndex;
	int maxAccUpperBound;
	vector<double> maxAccScale;

	map< VehicleBase::VehicleType,map<int,double> > normalDecelerationIndex;
	int normalDecelerationUpperBound;
	vector<double> normalDecelerationScale;

	map< VehicleBase::VehicleType,map<int,double> > maxDecelerationIndex;
	int maxDecelerationUpperBound;
	vector<double> maxDecelerationScale;

	// param of calcSignalRate()
	double yellowStopHeadway;
	double minSpeedYellow;

	/// time step to calculate state variables
	double timeStep;

	/// grade is the road slope
	double tmpGrade;

	double minSpeed;
	double minResponseDistance;

	// param of carFollowingRate()
	double hBufferUpper;
	vector<double> hBufferUpperScale;
	/** \brief calculate hBufferUpper value uniform distribution
	 *  \return hBufferUpper
	 **/
	double getBufferUppder();
	double hBufferLower;

	//Car following parameters
	CarFollowParam CF_parameters[2];
	/** \brief convert string to CarFollowParam
	 *  \param s string data
	 *  \param cfParam CarFollowParam to store converted double value
	 **/
	void makeCFParam(string& s,CarFollowParam& cfParam);

	// target gap parameters
	vector<double> targetGapAccParm;
};


}
