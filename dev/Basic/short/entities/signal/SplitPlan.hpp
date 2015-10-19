//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <map>
#include <vector>

#include "Cycle.hpp"
#include "Phase.hpp"
#include "defaults.hpp"

namespace geo
{
class SplitPlan_t_pimpl;
class Signal_t_pimpl;
}
namespace sim_mob
{
namespace xml
{
class Signal_t_pimpl;
class SplitPlan_t_pimpl;
}
//Forward dseclaration
class Signal_SCATS;

enum TrafficControlMode
{
	FIXED_TIME,
	ADAPTIVE
};

class SplitPlan
{

	friend class sim_mob::xml::SplitPlan_t_pimpl;
//	friend class geo::Signal_t_pimpl;
public:
	unsigned int TMP_PlanID;//to identify "this" object(totally different from choice set related terms like currSplitPlanID,nextSplitPlanID....)
private:

    //int signalTimingMode;//Fixed plan or adaptive control
	double cycleLength,offset;
	std::size_t NOF_Plans; //NOF_Plans= number of split plans = choiceSet.size()

	std::size_t currSplitPlanID;
	std::size_t nextSplitPlanID;

	sim_mob::Cycle cycle_;
	sim_mob::Signal_SCATS *parentSignal;

	/*
	 * the following variable will specify the various choiceSet combinations that
	 * can be assigned to phases.
	 * therefore we can note that in this matrix the outer vector denote columns and inner vector denotes rows(reverse to common sense):
	 * 1- the size of the inner vector = the number of phases(= the size of the above phases_ vector)
	 * 2- currPlanIndex is actually one of the index values of the outer vector.
	 * everytime a voting procedure is performed, one of the sets of choiceSet are orderly assigned to phases.
	 */

	std::vector< std::vector<double> > choiceSet; //choiceSet[Plan][phase]

	/* the following variable keeps track of the votes obtained by each splitplan(I mean phase choiceSet combination)
	 * ususally a history of the last 5 votings are kept
	 */
	/*
	 * 			plan1	plan2	plan3	plan4	plan5
	 * 	iter1	1		0		0		0		0
	 * 	iter2	0		1		0		0		0
	 * 	iter3	0		1		0		0		0
	 * 	iter5	1		0		0		0		0
	 * 	iter5	0		0		0		1		0
	 */
	std::vector< std::vector<int> > votes;  //votes[cycle][plan]

public:
	/*plan methods*/
	SplitPlan(double cycleLength_ = 90,double offset_ = 0/*, int signalTimingMode_= ConfigParams::GetInstance().signalTimingMode*/, unsigned int TMP_PlanID_ = 1);
	//the current plan id being used
	std::size_t CurrSplitPlanID();
	//the current plan being used
	std::vector< double >  CurrSplitPlan();
	void setCurrPlanIndex(std::size_t);
	//find/calculate the appropriate split plan for the next cycle based on the loop detector counts
	std::size_t findNextPlanIndex(std::vector<double> DS);
	//update currSplitPlanID by nextSplitPlanID
	void updatecurrSplitPlan();
	//number of SplitPlans available to choose from
	std::size_t nofPlans();
	void setcurrSplitPlanID(std::size_t index);
	//setting the container of split plans(known as choice set)
	void setChoiceSet(std::vector< std::vector<double> >);
	const std::vector< std::vector<double> > &getChoiceSet()const ;
	//resort to default split plans when there is no input
	void setDefaultSplitPlan(int);
	void initialize();

	/*cycle length related methods*/
	double getCycleLength() const ;
	void setCycleLength(std::size_t);

	/*offset related methods*/
	std::size_t getOffset() const;
	void setOffset(std::size_t);

	/*main update mehod*/
	void Update(std::vector<double> &DS);

	/*General Methods*/
	void calMaxProDS(std::vector<double>  &maxproDS,std::vector<double> DS);
	std::size_t Vote(std::vector<double> maxproDS);
	int getPlanId_w_MaxVote();
	double fmin_ID(std::vector<double> maxproDS);
	std::size_t getMaxVote();
	void fill(double defaultChoiceSet[5][10], int approaches);
	void setParentSignal(sim_mob::Signal_SCATS * signal) { parentSignal = signal;}
	sim_mob::Signal_SCATS * getParentSignal() { return parentSignal;}
	void printColors(double printColors);
	double fmax(std::vector<double> &DS);

	/*friends*/
	friend class Signal_SCATS;
};
}
