//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

namespace sim_mob
{

/**
 * A general dwell time calculation.
 *
 * \refactor zhang huai peng
 */
double dwellTimeCalculation(int numOfPassengers, int A = 0, int B = 0,
		int deltaBay = 0, int deltaFull = 0, int P_Front = 0) {

	const double alpha1 = 2.1; //alighting passenger service time,assuming payment by smart card
	const double alpha2 = 3.5; //boarding passenger service time,assuming alighting through rear door
	const double alpha3 = 3.5; 	//door opening and closing times
	const double alpha4 = 1.0;
	const double beta1 = 0.7; 	//fixed parameters
	const double beta2 = 0.7;
	const double beta3 = 5.0;
	const int numOfSeats = 40;
	double busCrowdnessFactor = 0.0;
	double alpha = alpha1;
	if (numOfPassengers > numOfSeats) {	//standing people are present
		alpha += 0.5; 	//boarding time increase if standing people are present
	}
	if (numOfPassengers > numOfSeats) {
		busCrowdnessFactor = 1.0;
	} else {
		busCrowdnessFactor = 0.0;
	}
	double PT_Front = alpha * P_Front * A + alpha2 * B
			+ alpha3 * busCrowdnessFactor * B;
	double PT_Rear = alpha4 * (1.0 - P_Front) * A;
	double PT = std::max(PT_Front, PT_Rear);
	double DT = beta1 + PT + beta2 * deltaBay + beta3 * deltaFull;
	return DT;
}
}
