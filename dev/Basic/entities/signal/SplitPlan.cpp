#include "SplitPlan.hpp"
#include<stdio.h>
#include<sstream>

using namespace boost::multi_index;
using std::vector;

namespace sim_mob
{


void SplitPlan::setCycleLength(std::size_t c = 96) {cycleLength = c;}
void SplitPlan::setcurrSplitPlanID(std::size_t index) { currSplitPlanID = index; }
void SplitPlan::setCoiceSet(std::vector< vector<double> > choiceset){choiceSet = choiceset;}

std::size_t SplitPlan::CurrSplitPlanID() { return currSplitPlanID; }
double SplitPlan::getCycleLength() const {return cycleLength;}



std::size_t SplitPlan::getOffset() {return offset;}

void SplitPlan::setOffset(std::size_t o) {offset = o;}



//const std::vector<sim_mob::Phase> & SplitPlan::getPhases() const { return phases_; }
//std::vector<sim_mob::Phase> & SplitPlan::getPhases() { return phases_; }

void SplitPlan::addPhase(sim_mob::Phase phase) { phases_.push_back(phase); }

std::size_t & SplitPlan::CurrPhaseID() { return currPhaseID; }

const  sim_mob::Phase & SplitPlan::CurrPhase() const { return phases_[currPhaseID]; }

/*
 * This function has 2 duties
 * 1- Update the Votes data structure
 * 2- Return the index having the highest value(vote) with the help of another function(getMaxVote())
 */

std::size_t SplitPlan::Vote(std::vector<double> maxproDS) {
	std::vector<int> vote(NOF_Plans,0);//choiceSet.size()=no of plans
	vote[fmin_ID(maxproDS)]++;//the corresponding split plan's vote is incremented by one(the rests are zero)
	votes.push_back(vote);
	if(votes.size() > NUMBER_OF_VOTING_CYCLES) votes.erase(votes.begin()); //removing the oldest vote if necessary(wee keep only NUMBER_OF_VOTING_CYCLES records)
	/*
	 * step 3: The split plan with the highest vote in the last certain number of cycles will win the vote
	 *         no need to eat up your brain, in the following chunk of code I will get the id of the maximum
	 *         value in vote vector and that id will actually be my nextSplitPlanID
	*/
	return getMaxVote();
}
//calculate the projected DS and max Projected DS for each split plan (for internal use only, refer to section 4.3 table-4)
void SplitPlan::calMaxProDS(std::vector<double>  &maxproDS,std::vector<double> DS)
{
	double maax=0.0;
	vector<double> proDS(NOF_Phases, 0);
	for(int i=0; i < NOF_Plans; i++)//traversing the columns of Phase::choiceSet matrix
	{
		maax = 0.0;
		for(int j=0; j < NOF_Phases; j++)
		{
			//calculate the projected DS for this plan
			proDS[j] = DS[j] * choiceSet[currSplitPlanID][j]/choiceSet[i][j];
			//then find the Maximum Projected DS for each Split plan(used in the next step)
			if(maax < proDS[j]) maax = proDS[j];
		}
		maxproDS[i] = maax;
	}
}


//4.3 Split Plan Selection (use DS to choose SplitPlan for next cycle)(section 4.3 of the Li Qu's manual)
std::size_t SplitPlan::findNextPlanIndex(std::vector<double> DS) {

	std::vector<double>  maxproDS(NOF_Plans,0);// max projected DS of each SplitPlan

	int i,j;
	//step 1:Calculate the Max projected DS for each approach (and find the maximum projected DS)
	calMaxProDS(maxproDS,DS);
	//step2 2 & 3 in one function to save your brain.
	//Step 2: The split plan with the ' lowest "Maximum Projected DS" ' will get a vote
	//step 3: The split plan with the highest vote in the last certain number of cycles will win the vote
	nextSplitPlanID = Vote(maxproDS);
	return nextSplitPlanID;
}

void SplitPlan::updatecurrSplitPlan() {
	currSplitPlanID = nextSplitPlanID;
}

void SplitPlan::initialize()
{
	/*
	 * Now you know the each phase percentage from the choice set,
		so you may set the phase percntage and phase offset of each phase,
		then initialize phases(calculate its phase length, green time ...)
	 */
	std::vector<double> choice = CurrSplitPlan();
	if(choice.size() != find_NOF_Phases())
		throw std::runtime_error("Mismatch on number of phases");
	int i = 0 ; double percentage_sum =0;
	//setting percentage and phaseoffset for each phase
	for(sim_mob::SplitPlan::phases_iterator ph_it = getPhases().begin();ph_it != getPhases().end(); ph_it++, i++)
	{
		//this ugly line of code is due to the fact that multi index renders constant versions of its elements
		sim_mob::Phase & target_phase = const_cast<sim_mob::Phase &>(*ph_it);
		if( i > 0) percentage_sum += choice[i - 1]; // i > 0 : the first phase has phase offset equal to zero,
		(target_phase).setPercentage(choice[i]);
		(target_phase).setPhaseOffset(percentage_sum * cycleLength / 100);
	}
	//Now Initialize the phases(later you  may put this back to the above phase iteration loop
	for(phases_iterator it = phases_.begin(); it != phases_.end()  ; it++)
		const_cast<sim_mob::Phase &>(*it).initialize();
//	getchar();
}

///////////////////////////////Not so Important //////////////////////////////////////////////////////////////////////

//find the minimum among the max projected DS
double SplitPlan::fmin_ID(std::vector<double> maxproDS) {
	int min = 0;
	for (int i = 1; i < maxproDS.size(); i++) {
		if (maxproDS[i] < maxproDS[min]) {
			min = i;
		}
	}
	return min;//(Note: not minimum value ! but minimum value's "index")
}

//find the split plan Id which currently has the maximum vote
//remember : in votes, columns represent split plan vote
std::size_t SplitPlan::getMaxVote()
{
	int PlanId_w_MaxVote = -1;
	int SplitPlanID;
	int max_vote_value = -1;
	int vote_sum = 0;
	for(SplitPlanID = 0 ; SplitPlanID < NOF_Plans; SplitPlanID++)//column iterator(plans)
	{
		//calculating sum of votes in each column
		vote_sum = 0;
		for(int i=0; i < votes.size() ; i++)//row iterator(cycles)
			vote_sum += votes[i][SplitPlanID];
		if(max_vote_value < vote_sum)
		{
			max_vote_value = vote_sum;
			PlanId_w_MaxVote = SplitPlanID;// SplitPlanID with max vote so far
		}
	}
	return PlanId_w_MaxVote;
}

std::vector< double >  SplitPlan::CurrSplitPlan()
{
	if(choiceSet.size() == 0)
	{
		throw std::runtime_error( "Choice Set is empty the program can crash without it");
	}
	return choiceSet[currSplitPlanID];
}

std::size_t SplitPlan::nofPhases()
{
	return NOF_Phases;
}

std::size_t SplitPlan::find_NOF_Phases()
{
	return NOF_Phases=phases_.size();
}
std::size_t SplitPlan::nofPlans()
{
	return NOF_Plans;
}
//find the max DS
double SplitPlan::fmax(std::vector<double> &DS) {
	double max = DS[0];
	for (int i = 0; i < DS.size(); i++) {
		if (DS[i] > max) {
			max = DS[i];
		}
	}
	return max;
}
void SplitPlan::Update(std::vector<double> &DS)
{
	double DS_all = fmax(DS);
	cycle_.Update(DS_all);
	cycleLength = cycle_.getcurrCL();
	std::cout << "currplan index changed from " << currSplitPlanID  << " to " ;
		findNextPlanIndex(DS);
		updatecurrSplitPlan();
		std::cout << currSplitPlanID << std::endl;
		initialize();
}
/*
 * find out which phase we are in the current plan
 * based on the currCycleTimer(= cycle-time lapse so far)
 * at the moment, this function just returns what phase it is going to be(does not set any thing)
 */
std::size_t SplitPlan::computeCurrPhase(double currCycleTimer)
{
	std::vector< double > currSplitPlan = CurrSplitPlan();

	double sum = 0;
	int i;
	for(i = 0; i < NOF_Phases; i++)
	{
		//expanded the single line loop, for better understanding of future readers
		sum += cycleLength * currSplitPlan[i] / 100;
		if(sum > currCycleTimer) break;
	}

	if(i >= NOF_Phases) throw std::runtime_error("CouldNot computeCurrPhase for the given currCycleTimer");
	currPhaseID = (std::size_t)(i);
	return currPhaseID;
}

SplitPlan::SplitPlan(double cycleLength_,double offset_):cycleLength(cycleLength_),offset(offset_)
{
	currPhaseID = 0;
	nextSplitPlanID = 0;
	currSplitPlanID = 0;
	NOF_Phases = 0;
	NOF_Plans = 0;
	cycle_.setCurrCL(cycleLength_);
}
void SplitPlan::fill(double defaultChoiceSet[5][10], int approaches)
{
	for(int i = 0; i < 5; i++)
		for(int j = 0; j < approaches; j++)
		{
			choiceSet[i][j] = defaultChoiceSet[i][j];
		}
}

void SplitPlan::setDefaultSplitPlan(int approaches)
{
	NOF_Plans = 5;
	int ii=5,jj=0;
	double defaultChoiceSet_1[5][10] = {
			{100},
			{100},
			{100},
			{100},
			{100}
	};
	double defaultChoiceSet_2[5][10] = {
			{50,50},
			{30,70},
			{75,25},
			{60,40},
			{40,60}
	};
	double defaultChoiceSet_3[5][10] = {
			{40,40,20},
			{40,30,30},
			{30,30,40},
			{25,25,50},
			{50,20,30}
	};
	double defaultChoiceSet_4[5][10] = {
			{30,30,20,20},
			{20,35,20,25},
			{35,35,20,10},
			{35,30,10,25},
			{20,35,25,20}
	};
	double defaultChoiceSet_5[5][10] = {
			{20,20,20,20,20},
			{15,15,25,25,20},
			{30,30,20,10,10},
			{25,25,20,15,15},
			{10,15,20,25,30}
	};
	choiceSet.resize(5, vector<double>(approaches));
	switch(approaches)
	{
	case 1:
		fill(defaultChoiceSet_1,1);
		break;
	case 2:
		fill(defaultChoiceSet_2,2);
		break;
	case 3:
		fill(defaultChoiceSet_3,3);
		break;
	case 4:
		fill(defaultChoiceSet_4,4);
		break;
	case 5:
		fill(defaultChoiceSet_5,5);
		break;

	}

	currSplitPlanID = 0;
	currPhaseID = 0; //what the hell :)
}

std::string SplitPlan::createStringRepresentation()
{
	if(phases_.size() == 0)
		{
			return 0;
		}
	std::ostringstream output;
	output << "\"phases\":\n[";
	phases_iterator it = phases_.begin();
	while(it !=phases_.end())
	{
		output << (*it).createStringRepresentation();
		it++;
		if(it !=phases_.end())
			output << ",";
	}
	output << "\n]";
	return output.str();
}

void
SplitPlan::printColors(double currCycleTimer)
{
	phases_iterator it = phases_.begin();
	while(it !=phases_.end())
	{
		(*it).printPhaseColors(currCycleTimer);
		it++;
	}
}

std::string SplitPlan::outputTrafficLights(int phaseId) const
{
//	if (phaseId > phases_.size())
//	{
//		std::ostringstream err;
//		err << "phaseId out of range.. phaseId:" << phaseId << " phases_.size():"<<  phases_.size() << std::endl;
//		throw std::runtime_error(err.str());
//	}


	if(phases_.size() == 0)
		{
			return 0;
		}
	std::ostringstream output;
	output << "\"phases\":\n[";
	phases_iterator it = phases_.begin();
	while(it !=phases_.end())
	{
		output << (*it).outputPhaseTrafficLight();
		it++;
		if(it !=phases_.end())
			output << ",";
	}
	output << "\n]";
	return output.str();

}


};//namespace

////find te split plan Id which currently has the maximum vote
//int SplitPlan::getPlanId_w_MaxVote()
//{
//	int PlanId_w_MaxVote = -1 , SplitPlanID , max_vote_value = -1, vote_sum = 0;
//	for(SplitPlanID = 0 ; SplitPlanID < votes.size(); SplitPlanID++)
//	{
//		for(int i=0, vote_sum = 0; i < NUMBER_OF_VOTING_CYCLES ; vote_sum += votes[SplitPlanID][i++]);//calculating sum of votes in each column
//		if(max_vote_value < vote_sum)
//		{
//			max_vote_value = vote_sum;
//			PlanId_w_MaxVote = SplitPlanID;// SplitPlanID with max vote so far
//		}
//	}
//	return PlanId_w_MaxVote;
//}
//
////4.3 Split Plan Selection (use DS to choose SplitPlan for next cycle)(section 4.3 of the Li Qu's manual)
//void SplitPlan::setnextSplitPlan(std::vector<double> DS) {
//	std::vector<int> vote(SplitPlan.size(),0);
//	std::vector<double> proDS(DS.size(),0);// projected DS
//	std::vector<double>  maxproDS(plan_.getPhases().size(),0);// max projected DS of each SplitPlan
//	int i,j;
//
//	//step 1:Calculate the projected DS for each approach (and find the maximum projected DS)
//	calProDS_MaxProDS(proDS,maxproDS);
//	//Step 2: The split plan with the ' lowest "Maximum Projected DS" ' will get a vote
//	vote[fmin_ID(maxproDS)]++;//the corresponding split plan's vote is incremented by one(the rests are zero)
//	votes.push_back(vote);
//	if(votes.size() > NUMBER_OF_VOTING_CYCLES) votes.erase(0); //removing the oldest vote if necessary(wee keep only NUMBER_OF_VOTING_CYCLES records)
//	/*
//	 * step 3: The split plan with the highest vote in the last certain number of cycles will win the vote
//	 *         no need to eat up your brain, in the following chunk of code I will get the id of the maximum
//	 *         value in vote vector and that id will actually be my nextSplitPlanID
//	*/
//	nextSplitPlanID = getPlanId_w_MaxVote();
//	//enjoy the result
//	nextSplitPlan = SplitPlan[nextSplitPlanID];
//}
