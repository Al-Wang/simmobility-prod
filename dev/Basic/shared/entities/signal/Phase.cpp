//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#include "Phase.hpp"
#include "SplitPlan.hpp"

#include <vector>
#include <sstream>

#include "geospatial/Link.hpp"
#include "geospatial/RoadSegment.hpp"

namespace sim_mob
{
namespace {
std::string getColor(size_t id)
{
	std::ostringstream o;
	switch(id)
	{
	case sim_mob::Red:
		return "Red";
	case sim_mob::Amber:
		return "Amber";
	case sim_mob::Green:
		return "Green";
	case sim_mob::FlashingRed:
		return "FlashingRed";
	case sim_mob::FlashingAmber:
		return "FlashingAmber";
	case sim_mob::FlashingGreen:
		return "FlashingGreen";
	default:
		o << id << " Unknown";
		return o.str();
	}
	o << id << " Unknown__";
	return o.str();
}
} //End un-named namespace


	void Phase::updatePhaseParams(double phaseOffset_, double percentage_)
	{
		phaseOffset = phaseOffset_;
		phaseLength = percentage_;
	}
	const crossings_map & Phase::getCrossingMaps() const { return crossings_map_ ; }
	/*
	 * Functionalities of this function will be listed here as they emerge:
	 * 1- update the color of the link_maps
	 */
	void Phase::update(double currentCycleTimer) const
	{
		std::ostringstream o;

		double lapse = currentCycleTimer - phaseOffset;
		//todo: avoid color updation if already end of cycle--update the following line just did that:
		//when time lapse i= zero it means either
		//1-signal update was called at time zero(which is not likely)
		//or
		//2-you are at the beginning of this phase so the rest(computeColor) will be ok
		if(lapse < 0)
			{
			Print() << "Phase " << name << " update: " << "RETURNING lapse( " << lapse << ")=currentCycleTimer(" << currentCycleTimer << ") - phaseOffset(" << phaseOffset << ")" << std::endl;
				return;
			}
		//update each link-link signal's color
		links_map_iterator linkIterator = linksMap.begin();
		for(;linkIterator != linksMap.end() ; linkIterator++)
		{
			/*
			 * if no time lapse, don't change the color
			 * this is especialy important when a full cycle has passed and current cycle timer is zero!
			 * then instead of moving to the first phase , again the last phase may reset to green
			 */
			//
//			Print() << "Phase " << name << " update: " << "lapse( " << lapse << ")=currentCycleTimer(" << currentCycleTimer << ") - phaseOffset(" << phaseOffset << ")" << std::endl;
			(*linkIterator).second.currColor = (*linkIterator).second.colorSequence.computeColor(lapse);
			if(((*linkIterator).second.currColor > sim_mob::FlashingGreen) || ((*linkIterator).second.currColor < sim_mob::Red))
			{
				o << "currentCycleTimer :" << currentCycleTimer << "  phaseOffset :" << phaseOffset  << "--->lapse :" << lapse << "\n creates out of range color";
				throw std::runtime_error(o.str());
			}
//			Print() << " phase " << name << " --phaseOffset "<<phaseOffset <<  " --  timer: " << currentCycleTimer  << " -- current color = " << (*linkIterator).second.currColor << std::endl;
		}

//		Print() << "calling compute for crossings" << std::endl;
		//update each crossing signal's color
		//common sense says there is only one crossing per link, but I kept a container for it just in case
		crossings_map_iterator crossing_it = crossings_map_.begin();
		for(;crossing_it != crossings_map_.end() ; crossing_it++)
		{
			if(lapse > 0)
				{

				TrafficColor t = (*crossing_it).second.colorSequence.computeColor(lapse);
					(*crossing_it).second.currColor = t;//(*crossing_it).second.colorSequence.computeColor(lapse);
				}
		}

}
//	assumption : total green time = the whole duration in the color sequence except red!
//	formula : for a given phase, total_g is maximum (G+FG+...+A - All_red, red...) of the linkFrom(s)
	/*todo a container should be created(probabely at split Plan level and mapped to "choiceSet" container)
	to store total_g for all phases once the phase is met, so that this function is not called for the second time
	if any plan is selected for the second time.*/
	double Phase::computeTotalG ()const
	{
		double green, maxGreen;
		links_map_const_iterator linkIterator = linksMap.begin();
		for(maxGreen = 0; linkIterator != linksMap.end() ; linkIterator++)
		{
			std::vector< std::pair<TrafficColor,int> >::const_iterator  colorIterator = (*linkIterator).second.colorSequence.ColorDuration.begin();
		for (green = 0;	colorIterator != (*linkIterator).second.colorSequence.ColorDuration.end(); colorIterator++) {
			if ((*colorIterator).first != Red) {
				green += (*colorIterator).second;
			}
		}
			if(maxGreen < green) maxGreen = green;//formula :)
		}
		return maxGreen * 1000;
	}
	void Phase::addCrossingMapping(sim_mob::Link * link,sim_mob::Crossing * crossing, ColorSequence cs)
	{
		sim_mob::Crossings crossing_(link,crossing);
		crossing_.colorSequence.clear();
		crossing_.colorSequence = cs;
		crossings_map_.insert(std::make_pair(crossing,crossing_));
	}
	void Phase::addCrossingMapping(sim_mob::Link * link,sim_mob::Crossing * crossing)
	{
		Crossings crossing_(link,crossing);
		crossings_map_.insert(std::make_pair(crossing,crossing_));
	}
/*this function will find those crossings in the intersection
 * which are not CFG (conflict green) with the rest of the phase movements
 * Developer note: the difficult point is that I don't have node information
 * in this class(Signal class has it) and I am not intending to spaghetti mix everything.
 * So I guess I will call this from outside(perhapse from loader.cpp, just like
 * addLinkMapping).
 * todo: update this part
 */
void Phase::addDefaultCrossings(LinkAndCrossingC const & LAC,sim_mob::MultiNode *node) const {
	bool flag = false;
	if (linksMap.size() == 0)
		throw std::runtime_error("Link maps empty, crossing mapping can not continue\n");

	LinkAndCrossingC::iterator it = LAC.begin();
	if(it == LAC.end())
	{
		Print() << "Link and crossing container for this node is empty, crossing mapping can not continue\n";
		return;
	}
	//filter the crossings which are in the links_maps_ container(both link from s and link To s)
	//the crossings passing this filter are our winners
	for (LinkAndCrossingC::iterator it = LAC.begin(), it_end(LAC.end()); it != it_end; it++) {
		flag = false;
		sim_mob::Link* link = const_cast<sim_mob::Link*>((*it).link);
		//link from
		links_map_const_iterator l_it = linksMap.find(link); //const_cast used coz multi index container elements are constant
		if (l_it != linksMap.end()){
			continue; //this linkFrom is involved, so we don't need to green light its crossing
		}
		//un-involved crossing- successful candidate to get a green light in this phase
		sim_mob::Crossing * crossing = const_cast<sim_mob::Crossing *>((*it).crossing);
//			for the line below,please look at the sim_mob::Crossings container and crossings_map for clearance
		if(crossing)
		{
			crossings_map_.insert(std::pair<sim_mob::Crossing *, sim_mob::Crossings>(crossing, sim_mob::Crossings(link, crossing)));
		}
	}
}

///obsolete
sim_mob::RoadSegment * Phase::findRoadSegment(sim_mob::Link * link,sim_mob::MultiNode * node) const {
	sim_mob::RoadSegment *rs = 0;
	std::set<sim_mob::RoadSegment*>::iterator itrs = (*link).getUniqueSegments().begin();
	for (; itrs != (*link).getUniqueSegments().end(); itrs++) {
		if((dynamic_cast<sim_mob::MultiNode *>(const_cast<sim_mob::Node*>((*itrs)->getEnd())) == node)||(dynamic_cast<sim_mob::MultiNode *>(const_cast<sim_mob::Node*>((*itrs)->getStart())) == node)){
			rs = *itrs;
			break;
		}
	}

	return rs;
}

void Phase::addLinkMapping(sim_mob::Link * lf, sim_mob::linkToLink & ll,sim_mob::MultiNode *node) const {
	linksMap.insert(std::pair<sim_mob::Link *, sim_mob::linkToLink>(lf, ll));
}

std::string Phase::createStringRepresentation(std::string newLine) const {
	std::ostringstream output;
	if (linksMap.size() == 0 && crossings_map_.size() == 0)
		return 0;
	output << newLine << "{" << newLine;
	output << "\"name\": \"" << name << "\"," << newLine;
	int i = 0;
	if (linksMap.size()) {
//		segment_based
		output << "\"segments\":" << newLine << "[" << newLine;
		//link_based
		links_map_iterator it = linksMap.begin();
		while(it != linksMap.end() )
		{
//			Print() << " linksMap.size() = "  << std::endl;
			output << "{";
			//link_based
//			output << "\"link_from\":\"" << (*it).first << "\" ,"; //linkFrom
//			output << "\"link_to\":\"" << (*it).second.LinkTo << "\"}";
//			//segment_based
			output << "\"segment_from\":\"" << (*it).second.RS_From << "\" ,"; //segmentFrom
			output << "\"segment_to\":\"" << (*it).second.RS_To << "\"}";
			it++;
			if(it != linksMap.end()) output << "," << newLine;
		}
		output << newLine << "]," << newLine;
	}
	output << "\"crossings\":" << newLine << "[" << newLine;
	if (crossings_map_.size()) {

		crossings_map_iterator it = crossings_map_.begin();
		while (it != crossings_map_.end()) {
			output << "{\"id\":\"" << (*it).first << "\"}"; //crossing *
			it++;
			if (it != crossings_map_.end())
				output << "," << newLine;

		}

	}
	output << newLine << "]";

	output << newLine << "}" << newLine;
	return output.str();
}
void Phase::initialize(sim_mob::SplitPlan& plan){
	parentPlan = &plan;
	calculatePhaseLength();
	calculateGreen();
//	printColorDuration();
//	Print() << "phase: " << name << "   PhaseLength: " << phaseLength << "   offset: " << phaseOffset << std::endl;
}
void Phase::printColorDuration()
{
	for(links_map_iterator it = linksMap.begin()  ; it != linksMap.end(); it++)
	{
		ColorSequence cs = it->second.colorSequence;
		const std::vector< std::pair<TrafficColor,int> > & cd = cs.getColorDuration();
		std::vector< std::pair<TrafficColor,int> >::const_iterator it_color = cd.begin();
		int greenIndex=-1, tempGreenIndex = -1;
		for(; it_color != cd.end(); it_color++)
		{
			Print() << "	color id(" <<  sim_mob::getColor(it_color->first) << ") : " << it_color->second <<  std::endl;
		}
		Print() << "----------------------------------------------------------------" << std::endl;
	}
}

void Phase::calculatePhaseLength(){
	phaseLength = parentPlan->getCycleLength() * percentage /100;
//	Print() << "phase " << name << " phaselength = " <<  phaseLength << "  (parentPlan->getCycleLength() * percentage /100):(" <<  parentPlan->getCycleLength() << "*" <<  percentage << ")\n";

}

//amber, red are fixed but green time is calculated by cycle length and percentage given to that phase
void Phase::calculateGreen_Links(){
	/*
	 * here is the drill:
	 * 1.what is the amount of time that is assigned to this phase,(phaseLength might be already calculated)
	 * 2.find out how long the colors other than green will take
	 * 3.subtract them
	 * this time is allocated to which color? yes, it is the green time.... yes yes, i know! you are a Genuis!
	 */

	for(links_map_iterator it = linksMap.begin()  ; it != linksMap.end(); it++)
	{
		//1.what is the amount of time that is assigned to this phase
//		phaseLength is a member
		//2.find out how long the colors other than green will take
		ColorSequence & cs = it->second.colorSequence;
		const std::vector< std::pair<TrafficColor,int> > & cd = cs.getColorDuration();
		std::vector< std::pair<TrafficColor,int> >::const_iterator it_color = cd.begin();
		size_t otherThanGreen = 0;
		int greenIndex=-1, tempGreenIndex = -1;
		for(int tempGreenIndex = 0; it_color != cd.end(); it_color++)
		{
			if(it_color->first != sim_mob::Green)
			{
				otherThanGreen += it_color->second;
			}
			else
				greenIndex = tempGreenIndex;//we need to know the location(index) of the green, right after this loop ends

			tempGreenIndex ++;
		}
		//3.subtract(the genius part)
		if(greenIndex > -1)
		{
			cs.changeColorDuration(cs.getColorDuration().at(greenIndex).first, phaseLength - otherThanGreen);
//			cs.getColorDuration().at(greenIndex).second = phaseLength - otherThanGreen;
//			Print() << "phase :" << name << " phaselength:"<< phaseLength << "percentage: " << percentage << "  Green time : " << cs.getColorDuration().at(greenIndex).second << " (phaseLength - otherThanGreen):(" << phaseLength << " - " << otherThanGreen << ")" << std::endl;
		}
	}
}

//red is fixed but green time is one third of flashing green and they are calculated by cycle length and percentage given to that phase
void Phase::calculateGreen_Crossings(){
	/*
	 * here is the drill:
	 * 1.what is the amount of time that is assigned to this phase,(phaseLength might be already calculated)
	 * 2.find out how long the colors other than green and flashing green will take
	 * 3.subtract them
	 * what is the output? yes, it is the green time and flashing green. yes yes, i know! you are a Genuis!
	 */

	for(crossings_map_iterator it = crossings_map_.begin()  ; it != crossings_map_.end(); it++)
	{
		//1.what is the amount of time that is assigned to this phase
//		phaseLength is a member
		//2.find out how long the colors other than green will take
		ColorSequence & cs = it->second.colorSequence;
		const std::vector< std::pair<TrafficColor,int> > & cd = cs.getColorDuration();
		std::vector< std::pair<TrafficColor,int> >::const_iterator it_color = cd.begin();
		size_t otherThanGreen = 0;
		int greenIndex=-1, flishingGreenIndex = -1;
		int tempGreenIndex = 0, tempFGreenIndex = 0;
		for(; it_color != cd.end(); it_color++)
		{
			if((it_color->first != sim_mob::Green) && (it_color->first != sim_mob::Amber))
			{
				otherThanGreen += it_color->second;
			}
			else
			{
				if(it_color->first == sim_mob::Green)
					greenIndex = tempGreenIndex;//we need to know the location of green, right after this loop ends
				else
					flishingGreenIndex = tempGreenIndex;//we also need to know the location of flashing green, right after this loop ends
			}

			tempGreenIndex ++;
		}
		//3.subtract(the genius part)
		if((greenIndex > -1)&&(flishingGreenIndex > -1))
		{
			cs.changeColorDuration(cs.getColorDuration().at(greenIndex).first, (phaseLength - otherThanGreen) / 3);
			cs.changeColorDuration(cs.getColorDuration().at(flishingGreenIndex).first, ((phaseLength - otherThanGreen) / 3) * 2);
//			cs.getColorDuration().at(greenIndex).second = (phaseLength - otherThanGreen) / 3; //green time is one third of flashing green
//			cs.getColorDuration().at(flishingGreenIndex).second = ((phaseLength - otherThanGreen) / 3) * 2 ;//f green time is two third of available time
		}
	}
}

//amber, red are fixed but green time is calculated by cycle length and percentage given to that phase
void Phase::calculateGreen(){
	calculateGreen_Links();
	calculateGreen_Crossings();
}

void Phase::printPhaseColors(double currCycleTimer) const
{
	for(links_map_iterator it = linksMap.begin()  ; it != linksMap.end(); it++)
	{
		Print() << name << "(currCycleTimer: " << currCycleTimer<< " , phaseLength: " <<  phaseLength << ") ::"<< sim_mob::getColor((*it).second.currColor)  << std::endl;
	}


}
const std::string & Phase::getName() const
{
	return name;
}

std::string Phase::outputPhaseTrafficLight(std::string newLine) const
{
	std::ostringstream output;
	output.str("");
	if (linksMap.size() == 0 && crossings_map_.size() == 0)
		return output.str();
	output << newLine << "{" << newLine;
	output << "\"name\": \"" << name << "\"," << newLine;
	int i = 0;
	if (linksMap.size()) {
		//link based
//		output << "\"links\":" << newLine << "[" << newLine;
		//segment based
		output << "\"segments\":" << newLine << "[" << newLine;
		links_map_iterator it = linksMap.begin();
		while (it != linksMap.end()) {
			output << "{";
			//link based
//			output << "\"link_from\":\"" << (*it).first << "\" ,"; //linkFrom
//			output << "\"link_to\":\"" << (*it).second.LinkTo << "\",";//linkTo
			//segment based
			output << "\"segment_from\":\"" << (*it).second.RS_From << "\" ,"; //linkFrom
			output << "\"segment_to\":\"" << (*it).second.RS_To << "\",";//linkTo
			output <<"\"current_color\":" << (*it).second.currColor << "}";//currColor
			it++;
			if (it != linksMap.end())
				output << "," << newLine;

		}
		output << newLine << "]," << newLine;
	}

	output << "\"crossings\":" << newLine << "[" << newLine;
	if (crossings_map_.size()) {

		crossings_map_iterator it = crossings_map_.begin();
		while (it != crossings_map_.end()) {
			output << "{\"id\":\"" << (*it).first << "\","; //crossing *
			output <<"\"current_color\":" << (*it).second.currColor << "}";//currColor
			it++;
			if (it != crossings_map_.end())
				output << "," << newLine;

		}
	}
	output << newLine << "]";

	output << newLine << "}" << newLine;
	return output.str();


//	std::ostringstream output;
//	if(linksMap.size() == 0 && crossings_map_.size() == 0) return 0;
//
//	output << "\"Phase_" << name << "\"{";
//	int i = 0;
//	if(linksMap.size())
//	{
//		output << "\"Link\"{";
//		links_map_iterator it = linksMap.begin();
//		while(it != linksMap.end())
//		{
//			output << (*it).first << ":";             //linkFrom
//			output << (*it).second.LinkTo << ":";     //linkTo
//			output << (*it).second.currColor;         //currentColor
//			it++;
//			if(it != linksMap.end())
//				output << ",";
//
//		}
//		output << "}";
//	}
//
//
//	if(crossings_map_.size())
//	{
//		output << "\"Crossing\"{";
//		crossings_map_iterator it = crossings_map_.begin();
//		while(it != crossings_map_.end())
//		{
//			output << (*it).first ;            //crossing *
//			output << (*it).second.currColor ; // current color
//			it++;
//			if(it != crossings_map_.end())
//				output << ",";
//
//		}
//		output << "}";
//	}
//
//	output << "}";
//	return output.str();
}
}//namespace
