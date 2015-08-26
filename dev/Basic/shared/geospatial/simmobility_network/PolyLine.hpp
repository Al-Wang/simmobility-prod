//Copyright (c) 2015 Singapore-MIT Alliance for Research and Technology
//Licensed under the terms of the MIT License, as described in the file:
//   license.txt   (http://opensource.org/licenses/MIT)

#pragma once

#include <string>
#include <vector>

#include "Point.hpp"
#include "util/GeomHelpers.hpp"

namespace simmobility_network
{

  class PolyLine
  {
  protected:

    //Unique identifier for the Poly-line
    int polyLineId;
    
    //Defines the length of the poly-line
    double length;
    
    //Defines the points in the poly-line
    std::vector<Point> points;    

  public:
    
    PolyLine();
    
    PolyLine(const PolyLine& orig);
    
    virtual ~PolyLine();
    
    //Sets the id of the poly-line
    void setPolyLineId(int polyLineId);
    
    //Returns the id of the poly-line
    int getPolyLineId() const;
    
    //Sets the length of the poly-line
    void setLength(double length);
    
    //Returns the length of the poly-line
    double getLength() const;
    
    //Returns the vector of points within the poly-line
    const std::vector<Point>& getPoints() const;
    
    //Returns the first point in the poly-line
    Point getFirstPoint() const;

    //Returns the last point in the poly-line
    Point getLastPoint() const;

    //Adds a point to the poly-line
    void addPoint(Point point);    

  } ;
}
