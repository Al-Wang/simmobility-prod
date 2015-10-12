//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "Color.hpp"
#include "geospatial/network/Node.hpp"
#include "logging/Log.hpp"
#include "util/LangHelpers.hpp"

namespace sim_mob {
namespace xml {
class Phase_t_pimpl;
}
}

namespace sim_mob
{
//Forward declarations
class SplitPlan;
class RoadSegment;
class Signal;


struct linkToLink
{
	linkToLink(sim_mob::Link *linkto = 0):LinkTo(linkto) {
			colorSequence.addColorDuration(Green,0);
			colorSequence.addColorDuration(Amber,3);//a portion of the total time of the phase length is taken by amber
			colorSequence.addColorDuration(Red,1);//All red moment ususally takes 1 second

		currColor = sim_mob::Red;
		RS_From = RS_To = 0;
	}
	linkToLink(sim_mob::Link *linkto,sim_mob::RoadSegment *RS_From_, sim_mob::RoadSegment *RS_To_):LinkTo(linkto) {
			colorSequence.addColorDuration(Green,0);
			colorSequence.addColorDuration(Amber,3);//a portion of the total time of the phase length is taken by amber
			colorSequence.addColorDuration(Red,1);//All red moment ususally takes 1 second
		RS_To = RS_To_;
		RS_From = RS_From_;
		currColor = sim_mob::Red;
	}

	sim_mob::Link *LinkTo;
	mutable ColorSequence colorSequence;
	mutable TrafficColor currColor;
	//just in case you need to use segment instead of link
	sim_mob::RoadSegment *RS_From;
	sim_mob::RoadSegment *RS_To;
};
//typedef ll linkToLink;

typedef std::multimap<const Link *, sim_mob::linkToLink> links_map; //mapping of LinkFrom to linkToLink{which is LinkTo,colorSequence,currColor}

////////////////////crossings////////////////////////////////////////////////////////////////////////////////////
/*struct crossings
{
	crossings(sim_mob::Link *link_,sim_mob::Crossing *crossig_):link(link_),crossig(crossig_), colorSequence(Pedestrian_Light){
		colorSequence.addColorDuration(Green,0);
		colorSequence.addColorDuration(Amber,0);
		colorSequence.addColorDuration(Red,1);//All red moment ususally takes 1 second
		currColor = sim_mob::Red;
	}
	crossings()
	{
		currColor = sim_mob::Red;
		colorSequence.addColorDuration(Green,0);
		colorSequence.addColorDuration(Amber,0);
		colorSequence.addColorDuration(Red,1);//All red moment ususally takes 1 second
		link = 0;
		crossig = 0;
	}
	sim_mob::Link * link;//this is extra but keep it until you are sure!
	sim_mob::Crossing *crossig;//same as the key in the corresponding multimap(yes yes: it is redundant)
	ColorSequence colorSequence;//color and duration
	TrafficColor currColor;
} ;

typedef struct crossings Crossings;
typedef std::map<sim_mob::Crossing *, sim_mob::Crossings> crossings_map;
typedef crossings_map::const_iterator crossings_map_const_iterator;
typedef crossings_map::iterator crossings_map_iterator;
typedef std::pair<crossings_map_const_iterator, crossings_map_const_iterator> crossings_map_equal_range;*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * in adaptive signal control, the cycle length, phase percentage and offset are likely to change
 * but the color sequence of a traffic signal, amber time etc are assumed to be intact for many years.
 */
class Phase
{
	friend class sim_mob::xml::Phase_t_pimpl;
public:
	typedef links_map::iterator links_map_iterator;
	typedef links_map::const_iterator links_map_const_iterator;
	typedef std::pair<links_map_const_iterator, links_map_const_iterator> links_map_equal_range;

	/*typedef crossings_map::iterator crossings_map_iterator;
	typedef crossings_map::const_iterator crossings_map_const_iterator;*/

	Phase():parentSignal(NULL), percentage(0), phaseOffset(0), phaseLength(0), parentPlan(NULL){}
	Phase(std::string name_, sim_mob::SplitPlan *parent = nullptr):name(name_), parentPlan(parent),
			parentSignal(NULL), percentage(0), phaseOffset(0), phaseLength(0){}

	void setPercentage(double p)
	{
		percentage = p;
	}
	void setPhaseOffset(double p)
	{
		phaseOffset = p;
	}

	double getPhaseOffset()
	{
		return phaseOffset ;
	}

	links_map & getlinksMap()const{
		return linksMap;
	}

	links_map_equal_range LinkFrom_Range(sim_mob::Link *LinkFrom)
	{
		return linksMap.equal_range(LinkFrom);
	}

	links_map_iterator LinkFrom_begin() const
	{
		return linksMap.begin();
	}

	links_map_iterator LinkFrom_end() const
	{
		return linksMap.end();
	}

	links_map_equal_range getLinkTos(const Link *linkFrom) const
	{
		return linksMap.equal_range(linkFrom);
	}

	void addLinkMapping(sim_mob::Link * lf, sim_mob::linkToLink &ll, Node *node)const ;
	//void addCrossingMapping(sim_mob::Link *,sim_mob::Crossing *, ColorSequence);
	//void addCrossingMapping(sim_mob::Link *,sim_mob::Crossing *);

	//add crossing to any link of this node which is not involved in this phase
	//void addDefaultCrossings(sim_mob::LinkAndCrossingC const & ,Node *node)const;
	const links_map & getLinkMaps() const { return linksMap;}//apparently not needed, getLinkTos is good enough for getdriverlight()...except for the xmlwrite :)
	//const crossings_map & getCrossingMaps() const;

	void updatePhaseParams(double phaseOffset_, double percentage_);
	/* Used in computing DS for split plan selection
	 * the argument is the output
	 * */
	void update(double lapse) const;
	double computeTotalG() const;//total green time
	const std::string & getPhaseName()const { return name;}
	std::string createStringRepresentation(std::string newLine) const;
	void initialize(sim_mob::SplitPlan&);
	void calculateGreen();
	void calculateGreen_Crossings();
	void calculateGreen_Links();
	void calculatePhaseLength();//one day this function needs to beome vritual coz its calculations may be dependent on parameters that may or may not exist in all traffic signal algorithms
	void printColorDuration() ;
	void printPhaseColors(double currCycleTimer) const;
	const std::string & getName() const;
	std::string outputPhaseTrafficLight(std::string newLine) const;
	sim_mob::RoadSegment * findRoadSegment(sim_mob::Link *, Node *) const;
	void setParent(sim_mob::Signal*);
	Signal * getParent()const;

	std::string name; //we can assign a name to a phase for ease of identification
private:
	Signal * parentSignal;
	mutable std::size_t percentage;
	mutable double phaseOffset; //the amount of time from cycle start until this phase start
	double phaseLength;

	//The links that will get a green light at this phase
	mutable sim_mob::links_map linksMap;
	//The crossings that will get a green light at this phase
	//mutable sim_mob::crossings_map crossings_map_;

	sim_mob::SplitPlan *parentPlan;

	friend class SplitPlan;
	friend class Signal_SCATS;
};

}//namespace
