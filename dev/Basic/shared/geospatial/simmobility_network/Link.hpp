//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <string>
#include <vector>

#include "PolyLine.hpp"
#include "RoadSegment.hpp"
#include "Tag.hpp"
#include "Node.hpp"

namespace simmobility_network
{
  //Defines the various categories of links supported by SimMobility
  enum LinkCategory
  {
    //The default category
    LINK_CATEGORY_DEFAULT = 0
  };
  
  //Defines the various types of links supported by SimMobility
  enum LinkType
  {
    //Default road segment
    LINK_TYPE_DEFAULT = 0,

    //Expressway
    LINK_TYPE_EXPRESSWAY = 1,

    //Urban road
    LINK_TYPE_URBAN = 2,
    
    //Ramp
    LINK_TYPE_RAMP = 3,
    
    //Roundabout
    LINK_TYPE_ROUNDABOUT = 4,
    
    //Access
    LINK_TYPE_ACCESS = 5
  } ;

  class RoadSegment;
  class Node;

  class Link
  {
  private:
    
    //Unique identifier for the link
    unsigned int linkId;
    
    //Pointer to the node from which this link begins
    Node* fromNode;
    
    //Indicates the node from which this link begins
    unsigned int fromNodeId;
    
    //The length of the link
    double length;
    
    //Indicates the link category
    LinkCategory linkCategory;
    
    //Indicates the link type
    LinkType linkType;
    
    //The name of the road this link represents
    std::string roadName;
    
    //The road segments making up the link
    std::vector<RoadSegment *> roadSegments;
    
    //Holds the additional information
    std::vector<Tag> *tags;
    
    //Pointer to the node at which this link ends
    Node* toNode;
    
    //Indicates the node at which this link ends
    unsigned int toNodeId;    

  public:
    
    Link();
    
    Link(const Link& orig);
    
    virtual ~Link();

    //Returns the length of the link
    double getLength();

    //Returns the link id
    unsigned int getLinkId() const;
    
    //Setter for the link id
    void setLinkId(unsigned int linkId);
    
    //Returns a pointer to the node from where the link begins
    Node* getFromNode() const; 
    
    //Setter for the from node
    void setFromNode(Node *fromNode);
    
    //Returns the id of the node from where the link begins
    unsigned int getFromNodeId() const;
    
    //Setter for the from node id
    void setFromNodeId(unsigned int fromNodeId);
    
    //Returns the link category
    LinkCategory getLinkCategory() const;
    
    //Sets the link category
    void setLinkCategory(LinkCategory linkCategory);
    
    //Returns the link type
    LinkType getLinkType() const;
    
    //Sets the link type
    void setLinkType(LinkType linkType);
    
    //Returns the road name
    std::string getRoadName() const;
    
    //Setter for the road name
    void setRoadName(std::string roadName);
    
    //Returns the vector of road segments that make up the link
    const std::vector<RoadSegment*>& getRoadSegments() const;
    
    //Returns a pointer to the road segment at the given index in the vector
    const RoadSegment* getRoadSegment(int idx);
    
    //Returns a vector of tags which holds the additional information
    const std::vector<Tag>* getTags() const;
    
    //Setter for the tags field which holds the additional information
    void setTags(std::vector<Tag> *tags);
    
    //Returns a pointer to the node at which the link terminates
    Node* getToNode() const;
    
    //Setter for the node at which the link terminates
    void setToNode(Node *toNode);
    
    //Returns the id of the node at which the link terminates
    unsigned int getToNodeId() const;
    
    //Setter for id of the node at which the link terminates
    void setToNodeId(unsigned int toNodeId);
    
    //Adds a road segment to the vector of road segments that make up the link
    void addRoadSegment(RoadSegment *roadSegment);    
  } ;
}

