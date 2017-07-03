/*
 * FrazzoliController.h
 *
 *  Created on: 21 Jun 2017
 *      Author: araldo
 */

#ifndef SHARED_ENTITIES_CONTROLLERS_FRAZZOLICONTROLLER_HPP_
#define SHARED_ENTITIES_CONTROLLERS_FRAZZOLICONTROLLER_HPP_

#include "OnCallController.hpp"

namespace sim_mob {

/**
 * This edge exists in the RV graph if the two requests can share the same ride
 */
typedef std::pair<TripRequestMessage, Person*> RR_Edge;

/**
 * This edge exists in the RV graph if the driver can serve the request
 * (D stands for Driver)
 */
typedef std::pair<TripRequestMessage, Person*> RD_Edge;

/**
 * This edge exists in the RTV graph if the request belongs to the request group
 * (G stands for request Group )
 */
typedef std::pair<TripRequestMessage, Group<TripRequestMessage>> RG_Edge;

/**
 * This edge exists in the RTV graph if the request request group can be served by the driver
 * (G stands for request Group, D stands for Driver )
 */
class GD_Edge
{
public:
	GD_Edge(const Group<TripRequestMessage>& requestGroup_, const Person* driver_, double cost_, const Schedule& schedule):
		requestGroup(requestGroup_), driver(driver_),cost(cost_){};

	const Group<TripRequestMessage> requestGroup;
	const Person* driver;
	const double cost;
	const Schedule schedule;
};

/**
 * In [Frazzoli2017] this graph is called RV graph
 */
class RD_Graph
{
public:
	/**
	 * Returns the Request-to-Driver edges having that specific driver
	 */
	const std::vector< RD_Edge>& getRD_Edges(const Person* driver) const;

	virtual void addEdge(const TripRequestMessage& r1, const TripRequestMessage& r2) ;
	virtual void addEdge(const TripRequestMessage& request, const Person* mobilityServiceDriver);
	virtual bool doesEdgeExists(const TripRequestMessage& r1, const TripRequestMessage& r2) const;

protected:
	/**
	 * Request-to-request association
	 */
	std::vector< RR_Edge > rrEdges;

	/**
	 * Request-to-vehicle association
	 */
	std::vector< RD_Edge> rdEdges;

};

/**
 * In [Frazzoli2017] this graph is callerd RTV graph
 */
class RGD_Graph
{
public:
	virtual void addEdge(const TripRequestMessage& request, const Group<TripRequestMessage>& requestGroup);
	virtual void addEdge(const Group<TripRequestMessage>& requestGroup, const Person* mobilityServiceDriver,
			double cost, const Schedule& schedule);
	virtual void sortGD_Edges();
	virtual GD_Edge popGD_Edge();
	virtual bool hasGD_Edges() const;


protected:
	/**
	 * Request-to-RequestGroup association
	 */
	std::vector<RG_Edge> rgEdges;

	/**
	 * RequestGroup-to-Driver association
	 */
	std::vector<GD_Edge> gdEdges;
};

class FrazzoliController: public OnCallController {
public:
	explicit FrazzoliController
		(const MutexStrategy& mtxStrat = sim_mob::MtxStrat_Buffered,
		unsigned int computationPeriod = 0) :
		OnCallController(mtxStrat, computationPeriod, MobilityServiceControllerType::SERVICE_CONTROLLER_FRAZZOLI),
		requestGroupsPerOccupancy(std::vector< Group< Group<TripRequestMessage> > >(maxVehicleOccupancy) )
	{
	}

	virtual ~FrazzoliController();


	virtual void computeSchedules();


protected:
	/**
	 * This mimicks Appendix II.C of [Frazzoli2017]
	 */
	virtual RD_Graph generateRD_Graph();

	/**
	 * Performs the controller algorithm to assign vehicles to requests. This mimicks Alg.1 of Appendix [Frazzoli2017]. In the paper, the RGD graph is called RTV.
	 * [Frazzoli2017] Alonso-mora, J., Samaranayake, S., Wallar, A., Frazzoli, E., & Rus, D. (2017). On-demand high-capacity ride-sharing via dynamic trip-vehicle assignment - Supplemental Material. Proceedings of the National Academy of Sciences of the United States of America, 114(3).
	 */
	virtual RGD_Graph generateRGD_Graph(const RD_Graph& rdGraph);

	/**
	 * This mimicks Algorithm 2 of Suppl.Material of [Frazzoli2017]
	 */
	virtual void greedyAssignment(RD_Graph& rdGraph, RGD_Graph& rgdGraph);

	// requestGroupsPerOccupancy[i] will contain all the request groups of i+1 requests
	std::vector< Group< Group<TripRequestMessage> > > requestGroupsPerOccupancy;




};

} /* namespace sim_mob */

#endif /* SHARED_ENTITIES_CONTROLLERS_FRAZZOLICONTROLLER_HPP_ */


