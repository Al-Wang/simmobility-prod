/*
 * Polypoint.h
 *
 *  Created on: 16 Mar, 2015
 *      Author: max
 */

#ifndef GEOSPATIAL_POLYPOINT_H_
#define GEOSPATIAL_POLYPOINT_H_

#include <string>

namespace sim_mob {

class Polypoint {
public:
	Polypoint();
	Polypoint(const Polypoint& src);
	virtual ~Polypoint();

public:
	int id;
	int polylineId;
	int index;
	double x;
	double y;
	double z;
	std::string scenario;
};

} /* namespace sim_mob */

#endif /* GEOSPATIAL_POLYPOINT_H_ */
