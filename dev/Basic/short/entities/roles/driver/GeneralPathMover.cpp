//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "GeneralPathMover.hpp"

#include <limits>
#include <algorithm>
#include <stdexcept>

#include "conf/settings/DisableMPI.h"

#include "conf/ConfigManager.hpp"
#include "conf/CMakeConfigParams.hpp"

#include "geospatial/RoadSegment.hpp"
#include "geospatial/Link.hpp"
#include "geospatial/Node.hpp"
#include "geospatial/Lane.hpp"
#include "geospatial/Point2D.hpp"
#include "logging/Log.hpp"
#include "util/DynamicVector.hpp"
#include "util/GeomHelpers.hpp"
#include "util/DebugFlags.hpp"

#include "partitions/PartitionManager.hpp"

#ifndef SIMMOB_DISABLE_MPI
#include "partitions/PackageUtils.hpp"
#include "partitions/UnPackageUtils.hpp"
#include "partitions/ParitionDebugOutput.hpp"
#endif

using namespace sim_mob;
using std::vector;
using std::string;
using std::endl;

//Error message strings
const std::string sim_mob::GeneralPathMover::ErrorPathNotSet("GeneralPathMover path not set.");
const std::string sim_mob::GeneralPathMover::ErrorPolylineLength("Can't manage polylines of length 0/1");
const std::string sim_mob::GeneralPathMover::ErrorNotInIntersection("Not actually in an Intersection!");
const std::string sim_mob::GeneralPathMover::ErrorAdvancePathDone1("advance(double fwdDistance)::Entire path is already done.");
const std::string sim_mob::GeneralPathMover::ErrorAdvancePathDone2("advance(const RoadSegment* currSegment, ...)::Entire path is already done.");
const std::string sim_mob::GeneralPathMover::ErrorPolylineCantAdvance("Polyline can't advance");
const std::string sim_mob::GeneralPathMover::ErrorRoadSegmentAtEnd("Road segment at end");
const std::string sim_mob::GeneralPathMover::ErrorPathDoneActual("actualMoveToNextSegmentAndUpdateDir::Entire path is already done.");
const std::string sim_mob::GeneralPathMover::ErrorGeneralPathDone("Entire path is already done.");




sim_mob::GeneralPathMover::GeneralPathMover() :
	distAlongPolylineCM(0), /*currPolylineLengthCM(0),*/
	distMovedInCurrSegment(0), distOfThisSegment(0), distOfRestSegments(0), inIntersection(false), isMovingForwardsInLink(false), currLaneID(0), distToEndSegment(0.0),
	isMoveToNextSegment(false)
{
}

sim_mob::GeneralPathMover::GeneralPathMover(const GeneralPathMover& copyFrom) :
	fullPath(copyFrom.fullPath), polypointsList(copyFrom.polypointsList), distAlongPolylineCM(copyFrom.distAlongPolylineCM), distMovedInCurrSegment(copyFrom.distMovedInCurrSegment),
			distOfThisSegment(copyFrom.distOfThisSegment), distOfRestSegments(copyFrom.distOfRestSegments), inIntersection(copyFrom.inIntersection),
			isMovingForwardsInLink(copyFrom.isMovingForwardsInLink), currLaneID(copyFrom.currLaneID)

{
	//We don't really care about the debug stream, but we should probably inform the user
	if (Debug::Paths)
	{
		DebugStream << "<COPY_CONSTRUCTOR>";
	}

	//Need to align our iterators
	currSegmentIt = fullPath.begin() + (copyFrom.currSegmentIt - copyFrom.fullPath.begin());
	currPolypoint = polypointsList.begin() + (copyFrom.currPolypoint - copyFrom.polypointsList.begin());
	nextPolypoint = polypointsList.begin() + (copyFrom.nextPolypoint - copyFrom.polypointsList.begin());

//	//Lane-zero iterators are a little trickier
	const vector<Point2D>& myLaneZero = const_cast<RoadSegment*> (*currSegmentIt)->getLanes()[currLaneID]->getPolyline();
	const vector<Point2D>& otherLaneZero = const_cast<RoadSegment*> (*copyFrom.currSegmentIt)->getLanes()[currLaneID]->getPolyline();
//	currLaneZeroPolypoint = myLaneZero.begin() + (copyFrom.currLaneZeroPolypoint - otherLaneZero.begin());
//	nextLaneZeroPolypoint = myLaneZero.begin() + (copyFrom.nextLaneZeroPolypoint - otherLaneZero.begin());
//	const vector<Point2D>& otherLane = const_cast<RoadSegment*> (*copyFrom.currSegmentIt)->getLanes()[currLaneID]->getPolyline();
//	currLaneZeroPolypoint = otherLane.begin();
//	nextLaneZeroPolypoint = currLaneZeroPolypoint+1;
}

double sim_mob::GeneralPathMover::currPolylineLengthCM() const
{
	//TEMP: Just making sure.
	if (isInIntersection()) {
		return distAlongPolylineCM;
	}
	DynamicVector temp(currPolypoint->getX(), currPolypoint->getY(),nextPolypoint->getX(), nextPolypoint->getY());
	return temp.getMagnitude();
}

void sim_mob::GeneralPathMover::setPath(const vector<const RoadSegment*>& path, int startLaneID)
{
	if (Debug::Paths) {
		DebugStream << "New Path of length " << path.size() << endl;
		DebugStream << "Starting in Lane: " << startLaneID << endl;
	}

	fullPath.clear();
	if(path.empty()){
		return;
	}
	//Add RoadSegments to the path.
	Link* currLink = nullptr;
	for (vector<const RoadSegment*>::const_iterator it = path.begin(); it != path.end(); it++) {
		fullPath.push_back(*it);

		if (Debug::Paths) {
 			DebugStream << "  " << (*it)->getStart()->originalDB_ID.getLogItem() << "=>" << (*it)->getEnd()->originalDB_ID.getLogItem();
			if ((*it)->getLink() != currLink)
			{
				currLink = (*it)->getLink();
				if (it != path.begin()) {
					//fwd = sim_mob::dist((*it)->getLink()->getStart()->location, (*it)->getStart()->location)
					//	< sim_mob::dist((*it)->getLink()->getStart()->location, (*it)->getEnd()->location);
				}
				DebugStream << "  Link: " << currLink  << "  length: " << Fmt_M(currLink->getLength()) << "  poly-length: " << Fmt_M(CalcSegmentLaneZeroDist(it, path.end()));
			}
			DebugStream << endl;
			DebugStream << "    Euclidean length: " << Fmt_M(dist((*it)->getStart()->location, (*it)->getEnd()->location)) << "   reported length: " << Fmt_M((*it)->length) << endl;
		}
	}


	//Re-generate the polylines array, etc.
	currSegmentIt = fullPath.begin();
	isMovingForwardsInLink = true;

	//Ensure that the current lane is valid
	currLaneID = std::max(std::min(startLaneID, static_cast<int>((*currSegmentIt)->getLanes().size())-1), 0);

	//Generate a polyline array
	generateNewPolylineArray(getCurrSegment(), pathWithDirection.path, pathWithDirection.areFwds);
	distAlongPolylineCM = 0;

	inIntersection = false;
	//calcNewLaneDistances();

	distMovedInCurrSegment = 0;
	distOfThisSegment = CalcSegmentLaneZeroDist(currSegmentIt, fullPath.end());
	distOfRestSegments = CalcRestSegmentsLaneZeroDist(currSegmentIt, fullPath.end());

//	setStartPositionInSegment(); //for mid-term
}


void sim_mob::GeneralPathMover::setPath(const vector<const RoadSegment*>& path, vector<bool>& areFwds, int startLaneID)
{
	if (Debug::Paths)
	{
		DebugStream << "New Path of length " << path.size() << endl;
		DebugStream << "Starting in Lane: " << startLaneID << endl;
	}

	areFwds.insert(areFwds.begin(), true);
	//areFwds.front() = isFwd;

	//Add RoadSegments to the path.
	Link* currLink = nullptr;
	fullPath.clear();
	//bool fwd = isFwd;

	for (vector<const RoadSegment*>::const_iterator it = path.begin(); it != path.end(); it++) {
		fullPath.push_back(*it);

		if (it != path.begin()) {
			areFwds.insert(areFwds.end(), true);
		}

		if (Debug::Paths)
		{
			DebugStream << "  " << (*it)->getStart()->originalDB_ID.getLogItem() << "=>" << (*it)->getEnd()->originalDB_ID.getLogItem();
			if ((*it)->getLink() != currLink)
			{
				currLink = (*it)->getLink();
				DebugStream << "  Link: " << currLink << "  length: " << Fmt_M(currLink->getLength()) << "  poly-length: " << Fmt_M(
						CalcSegmentLaneZeroDist(it, path.end()));
			}
			DebugStream << endl;
			DebugStream << "    Euclidean length: " << Fmt_M(dist((*it)->getStart()->location, (*it)->getEnd()->location)) << "   reported length: " << Fmt_M((*it)->length) << endl;
		}
	}


	//Re-generate the polylines array, etc.
	currSegmentIt = fullPath.begin();
	isMovingForwardsInLink = true;

	//Ensure that the current lane is valid
	currLaneID = std::max(std::min(startLaneID, static_cast<int>((*currSegmentIt)->getLanes().size())-1), 0);

	//Generate a polyline array
	generateNewPolylineArray(getCurrSegment(), path, areFwds);
	distAlongPolylineCM = 0;
	inIntersection = false;
	//calcNewLaneDistances();

	distMovedInCurrSegment = 0;
	distOfThisSegment = CalcSegmentLaneZeroDist(currSegmentIt, fullPath.end());
	distOfRestSegments = CalcRestSegmentsLaneZeroDist(currSegmentIt, fullPath.end());

	setStartPositionInSegment(); //for mid-term
}

void sim_mob::GeneralPathMover::resetPath(const vector<const RoadSegment*>& path)
{

	//Determine whether or not the first one is fwd.
	if (path.empty())
	{
		throw std::runtime_error("Attempting to reset a path with 0 road segments");
	}
	//Add RoadSegments to the path.
	Link* currLink = nullptr;
	fullPath.clear();
	for (vector<const RoadSegment*>::const_iterator it = path.begin(); it != path.end(); it++)
	{
		fullPath.push_back(*it);
	}
	//Re-generate the polylines array, etc.
	currSegmentIt = fullPath.begin();
}

void sim_mob::GeneralPathMover::calcNewLaneDistances()
{
//	DynamicVector zeroPoly(currLaneZeroPolypoint->getX(), currLaneZeroPolypoint->getY(), nextLaneZeroPolypoint->getX(), nextLaneZeroPolypoint->getY());
	DynamicVector zeroPoly(currPolypoint->getX(), currPolypoint->getY(), nextPolypoint->getX(), nextPolypoint->getY());
	distMovedInCurrSegment += zeroPoly.getMagnitude();
}

string sim_mob::GeneralPathMover::Fmt_M(centimeter_t dist)
{
	std::stringstream res;
	res << static_cast<int> (dist / 100) << " m";
	return res.str();
}

double sim_mob::GeneralPathMover::CalcSegmentLaneZeroDist(vector<const RoadSegment*>::const_iterator it, vector<const RoadSegment*>::const_iterator end)
{
	double res = 0.0;
	if (it != end)
	{
		const vector<Point2D>& polyLine = const_cast<RoadSegment*> (*it)->getLanes()[0]->getPolyline();
		for (vector<Point2D>::const_iterator it2 = polyLine.begin(); (it2 + 1) != polyLine.end(); it2++)
		{
			res += dist(it2->getX(), it2->getY(), (it2 + 1)->getX(), (it2 + 1)->getY());
		}
	}
	return res;
}

double sim_mob::GeneralPathMover::CalcRestSegmentsLaneZeroDist(vector<const RoadSegment*>::const_iterator start, vector<const RoadSegment*>::const_iterator end)
{
	double res = 0.0;
	for (vector<const RoadSegment*>::const_iterator it = start; it != end; it++)
	{
		//Add all polylines in this Segment
		const vector<Point2D>& polyLine = const_cast<RoadSegment*> (*it)->getLanes()[0]->getPolyline();
		for (vector<Point2D>::const_iterator it2 = polyLine.begin(); (it2 + 1) != polyLine.end(); it2++)
		{
			res += dist(it2->getX(), it2->getY(), (it2 + 1)->getX(), (it2 + 1)->getY());
		}

		//Break if the next Segment isn't in this link.
		if ((it + 1 == end) || ((*it)->getLink() != (*(it + 1))->getLink()))
		{
			break;
		}
	}
	return res;
}

void sim_mob::GeneralPathMover::generateNewPolylineArray()
{
	//Simple; just make sure to take the forward direction into account.
	//TODO: Take the current lane into account.
	polypointsList = (*currSegmentIt)->getLanes().at(currLaneID)->getPolyline();

	//Check
	throwIf(polypointsList.size() < 2, GeneralPathMover::ErrorPolylineLength);

	/*if (!isMovingForwards) { //NOTE: I don't think this makes sense.
	 std::reverse(polypointsList.begin(), polypointsList.end());
	 }*/
	currPolypoint = polypointsList.begin();
	nextPolypoint = polypointsList.begin() + 1;
	//currPolylineLength = sim_mob::dist(&(*currPolypoint), &(*nextPolypoint));

	//Set our lane zero polypoint-ers.
	const vector<Point2D>& tempLaneZero = const_cast<RoadSegment*> (*currSegmentIt)->getLanes()[0]->getPolyline();
//	currLaneZeroPolypoint = tempLaneZero.begin();
//	nextLaneZeroPolypoint = tempLaneZero.begin() + 1;

	//Debug output
	if (Debug::Paths)
	{
		DebugStream << "\nOn new polyline (1 of " << polypointsList.size() - 1 << ") of length: " << Fmt_M(currPolylineLengthCM()) << "  length of lane zero: " << Fmt_M(
				dist(*nextLaneZeroPolypoint, *currLaneZeroPolypoint)) << ", starting at: " << Fmt_M(distAlongPolylineCM) << ", starting at: " << Fmt_M(distAlongPolylineCM) << endl;
		DebugStream << "Distance of polyline end from end of Road Segment: " << Fmt_M(dist(polypointsList.back(), (*currSegmentIt)->getEnd()->location)) << endl;
	}
}

void sim_mob::GeneralPathMover::generateNewPolylineArray(const RoadSegment* currSegment, vector<const RoadSegment*> path, vector<bool> areFwds)
{
	int i = 0;
	bool isFwd = true;
	for(i = 0; i < path.size(); i++){
		if( currSegment == path[i] ){
			isFwd = areFwds[i];
			break;
		}
	}

	//Simple; just make sure to take the forward direction into account.
	//TODO: Take the current lane into account.
	polypointsList = (*currSegmentIt)->getLanes().at(currLaneID)->getPolyline();

	//Check
	throwIf(polypointsList.size() < 2, GeneralPathMover::ErrorPolylineLength);

	if (!isFwd) { //NOTE: I don't think this makes sense.
	 std::reverse(polypointsList.begin(), polypointsList.end());
	}
	currPolypoint = polypointsList.begin();
	nextPolypoint = polypointsList.begin() + 1;
	//currPolylineLength = sim_mob::dist(&(*currPolypoint), &(*nextPolypoint));

	//Set our lane zero polypoint-ers.
//	vector<Point2D> tempLaneZero =(*currSegmentIt)->getLanes()[0]->getPolyline();
	laneZeroPolypointsList = (*currSegmentIt)->getLanes()[currLaneID]->getPolyline();
	if (!isFwd) { //NOTE: I don't think this makes sense.
		 std::reverse(laneZeroPolypointsList.begin(), laneZeroPolypointsList.end());
	}
//	currLaneZeroPolypoint = laneZeroPolypointsList.begin();
//	nextLaneZeroPolypoint = laneZeroPolypointsList.begin() + 1;

	//Debug output
	if (Debug::Paths)
	{
		DebugStream << "\nOn new polyline (1 of " << polypointsList.size() - 1 << ") of length: " << Fmt_M(currPolylineLengthCM()) << "  length of lane zero: " << Fmt_M(
				dist(*nextLaneZeroPolypoint, *currLaneZeroPolypoint)) << ", starting at: " << Fmt_M(distAlongPolylineCM) << ", starting at: " << Fmt_M(distAlongPolylineCM) << endl;
		DebugStream << "Distance of polyline end from end of Road Segment: " << Fmt_M(dist(polypointsList.back(), (*currSegmentIt)->getEnd()->location)) << endl;
	}
}

bool sim_mob::GeneralPathMover::isPathSet() const
{
	return !fullPath.empty();
}

bool sim_mob::GeneralPathMover::isDoneWithEntireRoute() const
{
	bool res = currSegmentIt == fullPath.end();

	if (Debug::Paths && res)
	{
		if (ConfigManager::GetInstance().CMakeConfig().OutputEnabled()) {
			if (!DebugStream.str().empty())
			{
				//TEMP: Re-enable later.
				DebugStream << "Path is DONE." << endl;
				PrintOut(DebugStream.str());
				DebugStream.str("");
			}
		}
	}

	return res;
}

const Lane* sim_mob::GeneralPathMover::leaveIntersection()
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(!inIntersection, GeneralPathMover::ErrorNotInIntersection);

	if (Debug::Paths)
	{
		DebugStream << "User left intersection." << endl;
	}

	//Unset flag; move to the next segment.
	inIntersection = false;
	return actualMoveToNextSegmentAndUpdateDir();
}

void sim_mob::GeneralPathMover::throwIf(bool conditional, const std::string& msg) const
{
	if (conditional)
	{
		//Debug
		if (Debug::Paths)
		{
			if (ConfigManager::GetInstance().CMakeConfig().OutputEnabled()) {
				if (!DebugStream.str().empty())
				{
					DebugStream << "EXCEPTION: " << msg << endl;
					PrintOut(DebugStream.str());
					DebugStream.str("");
				}
			}
		}

		throw std::runtime_error(msg.c_str());
	}
}

//This is where it gets a little complex.
double sim_mob::GeneralPathMover::advance(double fwdDistance)
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);

	isMoveToNextSegment = false;

	//Taking precedence above everything else is the intersection model. If we are in an intersection,
	//  simply update the total distance and return (let the user deal with it). Also udpate the
	//  current polyline length to always be the same as the forward distance.
	if (inIntersection)
	{
		throw std::runtime_error("Calling \"advance\" within an Intersection currently doesn't work right; use the Intersection model.");

		distAlongPolylineCM += fwdDistance;
		//currPolylineLength = distAlongPolyline;
		return 0;
	}

	//Debug output
	if (Debug::Paths)
	{
		//DebugStream <<"  +" <<fwdDistance <<"cm" <<", (" <<Fmt_M(distAlongPolyline+fwdDistance) <<")";
		//Print the distance from the next Node
		Point2D myPos(getPosition().x, getPosition().y);
		DebugStream << "  " << Fmt_M(dist(myPos, (*currSegmentIt)->getEnd()->location)) << ",";
	}

	//Next, if we are truly at the end of the path, we should probably throw an error for trying to advance.
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorAdvancePathDone1);

	//Move down the current polyline. If this brings us to the end point, go to the next polyline
	double res = 0.0;
	distAlongPolylineCM += fwdDistance;
	//distMovedInSegment += fwdDistance;

	while (distAlongPolylineCM >= currPolylineLengthCM() && !inIntersection)
	{
		if (Debug::Paths)
		{
			Point2D myPos(getPosition().x, getPosition().y);
			DebugStream << endl << "Now at polyline end. Distance from actual end: " << Fmt_M(dist(*nextPolypoint, myPos)) << endl;
		}

		//Subtract distance moved thus far
		distAlongPolylineCM -= currPolylineLengthCM();

		//Advance pointers, etc.
		res = advanceToNextPolyline(true);
	}

	return res;
}

//This is where it gets a little complex.
double sim_mob::GeneralPathMover::advance(const RoadSegment* currSegment, vector<const RoadSegment*> path,vector<bool> areFwds, double fwdDistance)
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);

	//Taking precedence above everything else is the intersection model. If we are in an intersection,
	//  simply update the total distance and return (let the user deal with it). Also udpate the
	//  current polyline length to always be the same as the forward distance.
	if (inIntersection)
	{
		throw std::runtime_error("Calling \"advance\" within an Intersection currently doesn't work right; use the Intersection model.");

		distAlongPolylineCM += fwdDistance;

		//currPolylineLength = distAlongPolyline;
		return 0;
	}

	//Debug output
	if (Debug::Paths)
	{
		//DebugStream <<"  +" <<fwdDistance <<"cm" <<", (" <<Fmt_M(distAlongPolyline+fwdDistance) <<")";
		//Print the distance from the next Node
		Point2D myPos(getPosition().x, getPosition().y);
		DebugStream << "  " << Fmt_M(dist(myPos, (*currSegmentIt)->getEnd()->location)) << ",";
	}

	//Next, if we are truly at the end of the path, we should probably throw an error for trying to advance.
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorAdvancePathDone2);

	int i = 0;
	bool isFwd = true;
	for(i = 0; i < path.size(); i++){
		if( currSegment == path[i] ){
			isFwd = areFwds[i];
			break;
		}
	}


	double res = 0.0;
	if(isFwd){
		PrintOut("noteForDirection 1"<<std::endl);

		//Move down the current polyline. If this brings us to the end point, go to the next polyline
		distAlongPolylineCM += fwdDistance;
		//distMovedInSegment += fwdDistance;

		while (distAlongPolylineCM >= currPolylineLengthCM() && !inIntersection)
		{
			if (Debug::Paths)
			{
				Point2D myPos(getPosition().x, getPosition().y);
				DebugStream << endl << "Now at polyline end. Distance from actual end: " << Fmt_M(dist(*nextPolypoint, myPos)) << endl;
			}

			//Subtract distance moved thus far
			distAlongPolylineCM -= currPolylineLengthCM();

			//Advance pointers, etc.
			res = advanceToNextPolyline(true);
		}
	}
	else{
		PrintOut("noteForDirection 2"<<std::endl);

		//Move down the current polyline. If this brings us to the end point, go to the next polyline
		distAlongPolylineCM = currPolylineLengthCM() - fwdDistance - distAlongPolylineCM;

		while (distAlongPolylineCM <= 0 && !inIntersection)
		{
			if (Debug::Paths)
			{
				Point2D myPos(getPosition().x, getPosition().y);
				DebugStream << endl << "Now at polyline end. Distance from actual end: " << Fmt_M(dist(*nextPolypoint, myPos)) << endl;
			}

			//Subtract distance moved thus far
			distAlongPolylineCM = -distAlongPolylineCM;

			//Advance pointers, etc.
			res = advanceToNextPolyline(false);
		}
	}
	return res;
}

double sim_mob::GeneralPathMover::advanceToNextPolyline(bool isFwd)
{
	//An error if we're still at the end of this polyline
	throwIf(nextPolypoint == polypointsList.end(), GeneralPathMover::ErrorPolylineCantAdvance);

	//We can safely update our total distance here.
	calcNewLaneDistances();

	//Advance pointers

	if(isFwd){
		currPolypoint++;
		nextPolypoint++;

		//Advance lane zero pointers
//		currLaneZeroPolypoint++;
//		nextLaneZeroPolypoint++;
	}
	else{
		currPolypoint++;
		nextPolypoint++;

		//Advance lane zero pointers
//		currLaneZeroPolypoint++;
//		nextLaneZeroPolypoint++;
	}

	//Update length, OR move to a new Segment
	if (nextPolypoint == polypointsList.end())
	{
		if (Debug::Paths)
		{
			DebugStream << "On new Road Segment" << endl;
		}

		return advanceToNextRoadSegment();
	}
	else
	{
		isMoveToNextSegment = false;
		if (Debug::Paths)
		{
			DebugStream << "\nOn new polyline (" << (currPolypoint - polypointsList.begin() + 1) << " of " << polypointsList.size() - 1 << ") of length: " << Fmt_M(currPolylineLengthCM())
					<< "  length of lane zero: " << Fmt_M(dist(*nextLaneZeroPolypoint, *currLaneZeroPolypoint)) << ", starting at: " << Fmt_M(distAlongPolylineCM) << endl;
		}
	}

	return 0;
}

double sim_mob::GeneralPathMover::advanceToNextRoadSegment()
{
	//std::cout << "AAA" << std::endl;
	//An error if we're already at the end of this road segment
	throwIf(currSegmentIt == fullPath.end(), GeneralPathMover::ErrorRoadSegmentAtEnd);
	//distMovedInSegment = distAlongPolyline;

	//If we are approaching a new Segment, the Intersection driving model takes precedence.
	// In addition, no further processing occurs. This means advancing a very large amount will
	// leave the user inside an intersection even if he/she would normally be beyond it.
	//Note that distAlongPolyline should still be valid.
	if (currSegmentIt + 1 != fullPath.end())
	{
		if ((*currSegmentIt)->getLink() != (*(currSegmentIt + 1))->getLink())
		{
			Point2D myPos(getPosition().x, getPosition().y);
			if (Debug::Paths)
			{
				DebugStream << "Now in Intersection. Distance from Node center: " << Fmt_M(dist((*currSegmentIt)->getEnd()->location, myPos)) << endl;
			}
			inIntersection = true;
			isMoveToNextSegment = false;
			return distAlongPolylineCM;
		}
	}

	//Move to the next Segment
	actualMoveToNextSegmentAndUpdateDir();
	return 0;
}

const Lane* sim_mob::GeneralPathMover::actualMoveToNextSegmentAndUpdateDir()
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorPathDoneActual);

	isMoveToNextSegment = true;

	//Record
	bool nextInNewLink = ((currSegmentIt + 1) != fullPath.end()) && ((*(currSegmentIt + 1))->getLink() != (*currSegmentIt)->getLink());

	//Move
	currSegmentIt++;

	//Reset our distance; calculate the total lane-zero distance of this segment.
	distMovedInCurrSegment = 0;
	distOfThisSegment = CalcSegmentLaneZeroDist(currSegmentIt, fullPath.end());
	distOfRestSegments = CalcRestSegmentsLaneZeroDist(currSegmentIt, fullPath.end());

	//In case we moved
	//distAlongPolyline = distMovedInSegment; //NOTE: Should probably factor this out into a sep. variable.

	if (currSegmentIt == fullPath.end())
	{
		return nullptr;
	}

	//Bound lanes
	currLaneID = std::min<int>(currLaneID, (*currSegmentIt)->getLanes().size() - 1);

	//Is this new segment part of a Link we're traversing in reverse?
	//const Node* prevNode = isMovingForwards ? (*(currSegmentIt-1))->getEnd() : (*(currSegmentIt-1))->getStart();
	if (nextInNewLink)
	{
		//calcNewLaneDistances();

		const Node* prevNode = (*currSegmentIt)->getStart(); //TEMP: Not sure about this.
		if ((*currSegmentIt)->getLink()->getStart() == prevNode)
		{
			isMovingForwardsInLink = true;
		}
		else if ((*currSegmentIt)->getLink()->getEnd() == prevNode)
		{
			isMovingForwardsInLink = false;
		}
		else
		{
			//Presumably, we could enable something like this later, but it would require advanced
			//  knowledge of which Segments face forwards.
			throw std::runtime_error("Can't jump around to arbitrary nodes with GeneralPathMover.");
		}
	}

	//Now generate a new polyline array.
	generateNewPolylineArray(getCurrSegment(), pathWithDirection.path, pathWithDirection.areFwds);

	//Done
	return (*currSegmentIt)->getLanes().at(currLaneID);
}

double sim_mob::GeneralPathMover::getCurrLinkReportedLength() const
{
	return getCurrLink()->getLength();
}

const RoadSegment* sim_mob::GeneralPathMover::getCurrSegment() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);
	return *currSegmentIt;
}
const RoadSegment* sim_mob::GeneralPathMover::getNextSegment(bool sameLink) const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);

	vector<const RoadSegment*>::iterator nextSegmentIt = currSegmentIt + 1;
	if (nextSegmentIt == fullPath.end())
	{
		return nullptr;
	}
	if (((*nextSegmentIt)->getLink() != (*currSegmentIt)->getLink()) && sameLink)
	{
		return nullptr;
	}

	return *nextSegmentIt;
}
const RoadSegment* sim_mob::GeneralPathMover::getNextToNextSegment() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);

	vector<const RoadSegment*>::iterator nextSegmentIt = currSegmentIt + 1;
	if (nextSegmentIt == fullPath.end())
	{
		return nullptr;
	}
	nextSegmentIt++; // currSegmentIt + 2
	if (nextSegmentIt == fullPath.end())
	{
		return nullptr;
	}

	return *nextSegmentIt;
}
const RoadSegment* sim_mob::GeneralPathMover::getPrevSegment(bool sameLink) const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone); //Not technically an error, but unlikely to be useful.

	if (currSegmentIt == fullPath.begin())
	{
		return nullptr;
	}

	vector<const RoadSegment*>::iterator nextSegmentIt = currSegmentIt - 1;
	if (((*nextSegmentIt)->getLink() != (*currSegmentIt)->getLink()) && sameLink)
	{
		return nullptr;
	}

	return *nextSegmentIt;
}
const Link* sim_mob::GeneralPathMover::getCurrLink() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);
	return getCurrSegment()->getLink();
}

const Lane* sim_mob::GeneralPathMover::getCurrLane() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	if (isDoneWithEntireRoute())
		return nullptr;
	return getCurrSegment()->getLanes().at(currLaneID);
}
const Point2D& sim_mob::GeneralPathMover::getCurrPolypoint() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone); //Not technically wrong, but probably an error.
	return *currPolypoint;
}
const Point2D& sim_mob::GeneralPathMover::getNextPolypoint() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);
	return *nextPolypoint;
}

const Point2D& sim_mob::GeneralPathMover::getNextPolypointNew() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);
	if(nextPolypoint == polypointsList.end())
	{
		return *currPolypoint;
	}
	return *nextPolypoint;
}

double sim_mob::GeneralPathMover::getCurrDistAlongPolyline() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);

	//If we're done, returning zero makes sense
	if (isDoneWithEntireRoute())
	{
		return 0;
	}

	//Limiting by the total distance makes sense.
	return std::min(distAlongPolylineCM, currPolylineLengthCM());
}

double sim_mob::GeneralPathMover::getCurrDistAlongRoadSegment() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	//throwIf(isInIntersection(), GeneralPathMover::ErrorNotInIntersection);
	if(isInIntersection()) {
		throw std::runtime_error("Can't get distance in Segment while in an intersection.");
	}

	//Get the current median polyline distance
	//DynamicVector zeroPoly(currPolypoint->getX(), currPolypoint->getY(), nextPolypoint->getX(), nextPolypoint->getY());
	//double totalPolyDist = zeroPoly.getMagnitude();

	//Get the ratio of distance moved over the current one.
	double distRatio = std::min(distAlongPolylineCM, currPolylineLengthCM()) / currPolylineLengthCM();

	//Add this to the distance moved so far.
	//return distMovedInCurrSegment + distRatio * totalPolyDist;
	return distMovedInCurrSegment + distAlongPolylineCM;
}
double sim_mob::GeneralPathMover::getX() const {
	return getPosition().x;
}

double sim_mob::GeneralPathMover::getY() const {
	return getPosition().y;
}
double sim_mob::GeneralPathMover::getDistanceMovedInSegment() const {
	return getCurrDistAlongRoadSegment();
}

double sim_mob::GeneralPathMover::getDistanceToSegmentStart() const {
	DynamicVector dis(this->getX(), this->getY(),
			this->getCurrSegment()->getStart()->location.getX(),
			this->getCurrSegment()->getStart()->location.getY());
	return dis.getMagnitude();
}

double sim_mob::GeneralPathMover::getDistanceToSegmentEnd() const {
	DynamicVector dis(this->getX(), this->getY(),
			this->getCurrSegment()->getEnd()->location.getX(),
			this->getCurrSegment()->getEnd()->location.getY());
	return dis.getMagnitude();
}
double sim_mob::GeneralPathMover::getTotalRoadSegmentLength() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isInIntersection(), GeneralPathMover::ErrorNotInIntersection);

	return distOfThisSegment;
}

double sim_mob::GeneralPathMover::getAllRestRoadSegmentsLength() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isInIntersection(), GeneralPathMover::ErrorNotInIntersection);

	return distOfRestSegments;
}

double sim_mob::GeneralPathMover::getCurrPolylineTotalDist() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);
	return currPolylineLengthCM();
}
double sim_mob::GeneralPathMover::getCurrentSegmentLength()
{
	double dis=0;
	std::vector<sim_mob::Point2D>::iterator ite;
	for ( std::vector<sim_mob::Point2D>::iterator it =  polypointsList.begin(); it != polypointsList.end(); ++it )
	{
		ite = it+1;
		if ( ite != polypointsList.end() )
		{
			DynamicVector temp(it->getX(), it->getY(),ite->getX(), ite->getY());
			dis += temp.getMagnitude();
		}
	}

	return dis;
}

void sim_mob::GeneralPathMover::shiftToNewPolyline(bool moveLeft)
{

	moveToNewPolyline(currLaneID + (moveLeft ? 1 : -1));
}

void sim_mob::GeneralPathMover::moveToNewPolyline(int newLaneID)
{

	//Nothing to do?
	if (newLaneID == currLaneID) {
		//PrintOut("Nothing to do for next" << std::endl);
		return;
	}

	if (Debug::Paths) {
		DebugStream << "Switching to new lane: " << newLaneID << " from lane: " << currLaneID << endl;
	}

	//Invalid ID?
	if (newLaneID < 0 || newLaneID >= static_cast<int> ((*currSegmentIt)->getLanes().size()))
	{
		throw std::runtime_error("Switching to an invalid lane ID.");
	}

	//Save our progress
	int distTraveled = currPolypoint - polypointsList.begin();
	currLaneID = newLaneID;

	//Update our polyline array
	generateNewPolylineArray(getCurrSegment(), pathWithDirection.path, pathWithDirection.areFwds);
	if (distTraveled > 0)
	{
		currPolypoint += distTraveled;
		nextPolypoint += distTraveled;

		//And our lane zero pointers
//		currPolypoint += distTraveled;
//		nextLaneZeroPolypoint += distTraveled;
		//currPolylineLength = sim_mob::dist(&(*currPolypoint), &(*nextPolypoint));
	}

	//TODO: This is kind of a hack, but it's possible to have been on a shorter polyline than the
	//      one you just switched to. Might want to look at this later.
	advance(0);
}

DPoint sim_mob::GeneralPathMover::getPosition() const
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);

	//If we're done, return the position of the last poly-point
	if (isDoneWithEntireRoute())
	{
		return DPoint(currPolypoint->getX(), currPolypoint->getY());
	}

	//Else, scale a vector like normal
	DynamicVector movementVect(currPolypoint->getX(), currPolypoint->getY(), nextPolypoint->getX(), nextPolypoint->getY());
	movementVect.scaleVectTo(getCurrDistAlongPolyline()).translateVect();
	return DPoint(movementVect.getX(), movementVect.getY());
}

double sim_mob::GeneralPathMover::getPositionInSegmentCM()
{
	return distToEndSegment;
}

void sim_mob::GeneralPathMover::setPositionInSegmentCM(double newDistToEndCM)
{
	distToEndSegment = newDistToEndCM;
}

void sim_mob::GeneralPathMover::setStartPositionInSegment()
{
	std::vector<sim_mob::Point2D>::iterator ite;
	for ( std::vector<sim_mob::Point2D>::iterator it =  currPolypoint; it != polypointsList.end(); ++it )
	{
		ite = it+1;
		if ( ite != polypointsList.end() )
		{
			DynamicVector temp(it->getX(), it->getY(),ite->getX(), ite->getY());
			distToEndSegment += temp.getMagnitude();
		}
	}
}

double sim_mob::GeneralPathMover::getNextSegmentLength()
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);

	return CalcSegmentLaneZeroDist( (currSegmentIt+1), fullPath.end());
}

void sim_mob::GeneralPathMover::advance_med(double fwdDistance)
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);

	//Next, if we are truly at the end of the path, we should probably throw an error for trying to advance.
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorGeneralPathDone);

	distToEndSegment -= fwdDistance;

	distToEndSegment = std::max(distToEndSegment, 0.0);	//fwdDistance already takes this account. Just to make sure this doesn't fall below zero.
}

void sim_mob::GeneralPathMover::actualMoveToNextSegmentAndUpdateDir_med()
{
	throwIf(!isPathSet(), GeneralPathMover::ErrorPathNotSet);
	throwIf(isDoneWithEntireRoute(), GeneralPathMover::ErrorPathDoneActual);

	//Record
	bool nextInNewLink = ((currSegmentIt + 1) != fullPath.end()) && ((*(currSegmentIt + 1))->getLink() != (*currSegmentIt)->getLink());

	//Move
	currSegmentIt++;

	//Reset our distance; calculate the total lane-zero distance of this segment.
//	distMovedInCurrSegment = 0;
//	distOfThisSegment = CalcSegmentLaneZeroDist(currSegmentIt, fullPath.end());
//	distOfRestSegments = CalcRestSegmentsLaneZeroDist(currSegmentIt, fullPath.end());

	if (currSegmentIt == fullPath.end())
	{
		return;
	}

	//Is this new segment part of a Link we're traversing in reverse?
	//const Node* prevNode = isMovingForwards ? (*(currSegmentIt-1))->getEnd() : (*(currSegmentIt-1))->getStart();
	if (nextInNewLink)
	{
		//calcNewLaneDistances();

		const Node* prevNode = (*currSegmentIt)->getStart(); //TEMP: Not sure about this.
		if ((*currSegmentIt)->getLink()->getStart() == prevNode)
		{
			isMovingForwardsInLink = true;
		}
		else if ((*currSegmentIt)->getLink()->getEnd() == prevNode)
		{
			isMovingForwardsInLink = false;
		}
		else
		{
			//Presumably, we could enable something like this later, but it would require advanced
			//  knowledge of which Segments face forwards.
			throw std::runtime_error("Can't jump around to arbitrary nodes with GeneralPathMover.");
		}
	}

	//Now generate a new polyline array.
	generateNewPolylineArray(getCurrSegment(), pathWithDirection.path, pathWithDirection.areFwds);
}