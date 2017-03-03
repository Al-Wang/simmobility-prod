//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include <math.h>
#include "SurveillanceStation.hpp"

#include "conf/ConfigManager.hpp"
#include "RoadSegment.hpp"
#include "util/LangHelpers.hpp"

using namespace sim_mob;

const unsigned int SENSOR_LINKWIDE = 0x00000100;

SurveillanceStation::SurveillanceStation(unsigned int id, unsigned int type, unsigned int code, double zone, double offset,
										 unsigned int segId, unsigned int trafficLight) :
	stationId(id), stationType(type), taskCode(code), zoneLength(zone), offsetDistance(offset), segmentId(segId), trafficLightId(trafficLight)
{
	if(isLinkWide())
	{
		trafficSensors.push_back(new TrafficSensor(this));
	}
}

SurveillanceStation::~SurveillanceStation()
{
	clear_delete_vector(trafficSensors);
}

unsigned int SurveillanceStation::getSurveillanceStnId() const
{
	return stationId;
}

void SurveillanceStation::setSurveillanceStnId(unsigned int value)
{
	stationId = value;
}

unsigned int SurveillanceStation::isLinkWide() const
{
	return stationType & SENSOR_LINKWIDE;
}

void SurveillanceStation::setType(const unsigned int value)
{
	stationType = value;
}

unsigned int SurveillanceStation::getTaskCode() const
{
	return taskCode;
}

void SurveillanceStation::setTaskCode(unsigned int value)
{
	taskCode = value;
}

unsigned int SurveillanceStation::getZoneLength() const
{
	return zoneLength;
}

void SurveillanceStation::setZoneLength(unsigned int value)
{
	zoneLength = value;
}

unsigned int SurveillanceStation::getOffsetDistance() const
{
	return offsetDistance;
}

void SurveillanceStation::setOffsetDistance(unsigned int value)
{
	offsetDistance = value;
}

unsigned int SurveillanceStation::getSegmentId() const
{
	return segmentId;
}

void SurveillanceStation::setSegmentId(unsigned int value)
{
	segmentId = value;
}

const RoadSegment *SurveillanceStation::getSegment() const
{
	return segment;
}

void SurveillanceStation::setSegment(const RoadSegment *value)
{
	segment = value;

	//Delete the old sensors if any and associate new sensors witht the station
	clear_delete_vector(trafficSensors);

	unsigned int size = segment->getNoOfLanes();
	trafficSensors.reserve(size);

	while(size)
	{
		size--;
		trafficSensors[size] = new TrafficSensor(this, segment->getLane(size), size);
	}
}

unsigned int SurveillanceStation::getTrafficLightId() const
{
	return trafficLightId;
}

void SurveillanceStation::setTrafficLightId(unsigned int value)
{
	trafficLightId = value;
}

void SurveillanceStation::addSensor(TrafficSensor *sensor, int index)
{
	if(isLinkWide())
	{
		index = 0;
	}

	trafficSensors[index] = sensor;
}

TrafficSensor *SurveillanceStation::getTrafficSensor(int index)
{
	if(isLinkWide())
	{
		index = 0;
	}

	return trafficSensors[index];
}

TrafficSensor::TrafficSensor(SurveillanceStation *station) : index(0), lane(nullptr), surveillanceStn(station),
	count(0), occupancy(0), speed(0)
{
	id = (station->getSurveillanceStnId() * 100) + index;
}

TrafficSensor::TrafficSensor(SurveillanceStation *station, const Lane *lane, unsigned int index) : index(index),
	lane(lane), surveillanceStn(station), count(0), occupancy(0), speed(0)
{
	id = (station->getSurveillanceStnId() * 100) + index;
}

TrafficSensor::~TrafficSensor()
{
}

unsigned int TrafficSensor::getId() const
{
	return id;
}

unsigned int TrafficSensor::getIndex() const
{
	return index;
}

void TrafficSensor::setIndex(unsigned int value)
{
	index = value;
	id = (surveillanceStn->getSurveillanceStnId() * 100) + index;
}

const Lane *TrafficSensor::getLane() const
{
	return lane;
}

void TrafficSensor::setLane(const Lane *value)
{
	lane = value;
}

SurveillanceStation *TrafficSensor::getSurveillanceStn() const
{
	return surveillanceStn;
}

void TrafficSensor::setSurveillanceStn(SurveillanceStation *value)
{
	surveillanceStn = value;
}

unsigned int TrafficSensor::getCount() const
{
	return count;
}

double TrafficSensor::getOccupancy() const
{
	return occupancy;
}

double TrafficSensor::getSpeed() const
{
	return speed;
}

double TrafficSensor::calculateSensorSpeed(double vehPosition, double vehLength, double vehSpeed, double acceleration)
{
	//Distance travelled from previous position to the end of the sensor
	double distCovered = (surveillanceStn->getOffsetDistance() + surveillanceStn->getZoneLength()) - vehPosition + (vehLength / 2);

	//Time required to travel such a distance
	double time = calcTimeRequiredToCoverDistance(distCovered, vehSpeed, acceleration);

	//Speed
	return (vehSpeed + time * acceleration);
}

double TrafficSensor::calcTimeRequiredToCoverDistance(double distCovered, double vehSpeed, double acceleration)
{
	const double accEpsilon = 1.0E-2;
	const double speedEpsilon = 1.0E-2;
	const double distEpsilon = 1.0E-3;
	const double infTime = 86400.0;

	if (distCovered < distEpsilon)
	{
		//No movement
		return 0.0;

	}
	else if (fabs(acceleration) > accEpsilon)
	{
		//Acceleration is NOT zero
		double finalSpeedSq = vehSpeed * vehSpeed + 2.0 * acceleration * distCovered;

		if (finalSpeedSq > 0.0)
		{
			return (sqrt(finalSpeedSq) - vehSpeed) / acceleration;
		}
		else
		{
			//Stopped before moving the given distance
			return infTime;
		}
	}
	else
	{
		//Acceleration is zero
		if (vehSpeed > speedEpsilon)
		{
			//NOT stopped
			return distCovered / vehSpeed;
		}
		else
		{
			//Stopped
			return infTime;
		}
	}
}

void TrafficSensor::calculateActivatingData(double vehPosition, double vehLength, double vehSpeed, double acceleration, unsigned int currTime)
{
	/*
	 * LOOP POSITION and DETECTION ZONE
	 * Detector location is represented by the distance from the downstream edge of the detection zone to the end of the
	 * link.  A detector is occupied if
	 *
	 * sensor_pos + zone_length > vehicle_pos >  sensor_pos.
	 *
	 * ACTIVATE and DEACTIVATE
	 *
	 * A vehicle will activate the detector when its front bumper touches the upstream edge of the detection zone, i.e.:
	 *
	 * vehicle_pos <= sensor_pos + zone
	 *
	 * and deactivate the detector when its back bumper leave the downstream edge of the detector, i.e.:
	 *
	 * vehicle_pos + vehicle_length < sensor_pos
	 */

	//Distance to the detector at the beginning of current interval
	//        |------dis------|
	//     PPPP             DDDCCCC
	//     PPPP             DDDCCCC

	double distanceToEndOfDetector = (surveillanceStn->getOffsetDistance() + surveillanceStn->getZoneLength()) - (vehPosition + (vehLength / 2));

	//Distance traveled before activate the detector
	//        |----dis_on---|
	//     PPPP             DDDCCCC
	//     PPPP             DDDCCCC

	double distToActivateDetector = distanceToEndOfDetector - surveillanceStn->getZoneLength();

	//Distance traveled before deactivate the detector
	//        |------dis_off------|
	//     PPPP             DDDCCCC
	//     PPPP             DDDCCCC

	double distToDeactivateDetector = distanceToEndOfDetector + vehLength;

	//Time to deactivate the detector
	double timeToDeactivateDetector = calcTimeRequiredToCoverDistance(distToDeactivateDetector, vehSpeed, acceleration);

	double stepSize = ConfigManager::GetInstance().FullConfig().baseGranSecond();

	if (timeToDeactivateDetector > stepSize)
	{
		timeToDeactivateDetector = stepSize;
	}

	//Time to activate the detector
	float timeToActivateDetector = calcTimeRequiredToCoverDistance(distToActivateDetector, vehSpeed, acceleration);

	if (timeToActivateDetector > stepSize)
	{
		timeToActivateDetector = stepSize;
	}

	//Time previous vehicle left the detector wrt the begining of the interval
	double prevTimeOff = prevTimeDetectorOff - currTime;

	//Calculate overlap time with repect to previous vehicle
	double overlapTime = 0;

	if (timeToActivateDetector < prevTimeOff)
	{
		//activate same detector simultaneously
		overlapTime = prevTimeOff - timeToActivateDetector;
	}

	//Record the time this vehicle "leaves" the detector
	prevTimeDetectorOff = currTime + timeToDeactivateDetector;

	double occupancyToBeAdded = timeToDeactivateDetector - timeToActivateDetector - overlapTime;

	if (occupancyToBeAdded > 0.0)
	{
		occupancy += occupancyToBeAdded;
	}
}

void TrafficSensor::calculatePassingData(double vehPosition, double vehLength, double vehSpeed, double acceleration)
{
	//Increment vehicle count
	count++;

	//Increment the speed
	speed += calculateSensorSpeed(vehLength, vehPosition, vehSpeed, acceleration);
}

