/*
 * Project.hpp
 *
 *  Created on: Aug 20, 2014
 *      Author: gishara<gishara@smart.mit.edu>
 */
#pragma once

#include <ctime>
#include "Types.hpp"

namespace sim_mob
{

    namespace long_term
    {

    /*
     * Project plain object
     */
    	class Project
    	{
    	public:
    		Project(BigSerial projectId = INVALID_ID,
    		    				BigSerial parcelId = INVALID_ID,
    		    				BigSerial developerId = INVALID_ID,
    		    				BigSerial templateId = INVALID_ID,
    		    				std::string projectName = EMPTY_STR,
    		    				std::tm constructionDate = std::tm(),
    		    				std::tm completionDate = std::tm(),
    		    				double constructionCost = .0f,
    		    				double demolitionCost = .0f,
    		    				double totalCost = .0f,
    		    				double area = .0f,
    		    				std::string grossRatio = EMPTY_STR,
    		    				double grossArea = .0f)	;
    		virtual ~Project();


	double getLotSize();
	void setLotSize(double LotSize);
	std::tm getCompletionDate();
	void setCompletionDate(std::tm completionDate);
	double getConstructionCost();
	void setConstructionCost(double constructionCost);
	std::tm getConstructionDate();
	void setConstructionDate(std::tm constructionDate);
	double getDemolitionCost();
	void setDemolitionCost(double demolitionCost);
	BigSerial getDeveloperId();
	void setDeveloperId(BigSerial developerId);
	double getGrossArea();
	void setGrossArea(double grossArea);
	std::string getGrossRatio();
	void setGrossRatio(double grossRatio);
	BigSerial getParcelId();
	void setParcelId(BigSerial parcelId);
	BigSerial getProjectId();
	void setProjectId(BigSerial projectId);
	std::string getProjectName();
	BigSerial getTemplateId();
	void setTemplateId(BigSerial templateId);
	double getTotalCost();
	void setTotalCost(double totalCost);

	/* Operator to print the Parcel Match data.*/
	friend std::ostream& operator<<(std::ostream& strm, const Project& data);



    private:
	friend class ProjectDao;
    		BigSerial projectId;
    		BigSerial parcelId;
    		BigSerial developerId;
    		BigSerial templateId;
    		std::string projectName;
    		std::tm constructionDate;
    		std::tm completionDate;
    		double constructionCost;
    		double demolitionCost;
    		double totalCost;
    		std::string grossRatio;
    		double grossArea;
    		double lotSize;

    	};


    }

}
