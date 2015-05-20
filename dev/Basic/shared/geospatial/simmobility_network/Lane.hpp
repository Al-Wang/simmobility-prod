//Copyright (c) 2013 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

namespace simmobility_network
{

  class Lane
  {
  private:
    
    //Unique identifier for the lane
    unsigned int laneId;
    
    //Indicates the id of the geometry information
    unsigned int geometryId;
    
    //Indicates the index of the lane
    unsigned int laneIndex;
    
    //Indicates the id of the road section to which the lane belongs
    unsigned int roadSectionId;
    
    //Holds additional information
    Tag *tag;
    
    //Indicates the id of the turning path to which the lane belongs
    unsigned int turningPathId;    
    
  public:
    
    Lane(unsigned int id, unsigned int geometryId, unsigned int index, unsigned int sectionId, Tag *tag, unsigned int pathId);
    
    Lane(const Lane& orig);
    
    virtual ~Lane();
    
    //Returns the lane id
    unsigned int getLaneId() const;
    
    //Sets the lane id
    void setLaneId(unsigned int laneId);
    
    //Returns the id of the geometry information
    unsigned int getGeometryId() const;
    
    //Sets the id of the geometry information
    void setGeometryId(unsigned int geometryId);
    
    //Returns the lane index
    unsigned int getLaneIndex() const;
    
    //Sets the lane index
    void setLaneIndex(unsigned int laneIndex);
    
    //Returns the id of the road section to which the lane belongs
    unsigned int getRoadSectionId() const;
    
    //Sets the id of the road section to which the lane belongs
    void setRoadSectionId(unsigned int roadSectionId);
    
    //Returns a pointer to the tag which holds the additional information
    Tag* getTag() const;
    
    //Setter for the tag field which holds the additional information
    void setTag(Tag* tag);
    
    //Returns the id of the turning path to which the lane belongs
    unsigned int getTurningPathId() const;
    
    //Sets the id of the turning path to which the lane belongs
    void setTurningPathId(unsigned int turningPathId);
    
  } ;
}



