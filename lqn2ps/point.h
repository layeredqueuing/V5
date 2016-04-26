/* -*- c++ -*-
 * point.h	-- Greg Franks
 *
 * $Id: point.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef _POINT_H
#define _POINT_H

#include "lqn2ps.h"

class Point;

ostream& operator<<( ostream&, const Point& );

class Point
{
public:
    Point() : my_x(0), my_y(0) {}
    Point( double an_x, double a_y ) : my_x(an_x), my_y(a_y) {}
    Point( const Point &aPoint ) : my_x(aPoint.x()), my_y(aPoint.y()) {}
    Point& operator=( const Point& );
    Point& operator+=( const Point& aPoint ) { my_x += aPoint.x(); my_y += aPoint.y(); return *this; }
    Point& operator-=( const Point& aPoint ) { my_x -= aPoint.x(); my_y -= aPoint.y(); return *this; }
    Point& operator*=( const double s ) { my_x *= s; my_y *= s; return *this; }
    bool operator==( const Point& p2 ) const { return x() == p2.x() && y() == p2.y(); }
    bool operator!=( const Point& p2 ) const { return x() != p2.x() || y() != p2.y(); }

    Point& moveBy( const Point& aPoint ) { my_x += aPoint.x(); my_y += aPoint.y(); return *this; }
    Point& moveBy( const double dx, const double dy ) { my_x += dx; my_y += dy; return *this; }
    Point& moveTo( const double x, const double y ) { my_x = x; my_y = y; return *this; }
    Point& scaleBy( const double sx, const double sy ) { my_x *= sx; my_y *= sy; return *this; }
    Point& rotate( const Point& origin, const double theta );
    double x() const { return my_x; }
    double y() const { return my_y; }
    Point& x( const double an_x ) { my_x = an_x; return *this; }
    Point& y( const double a_y  ) { my_y = a_y;  return *this; }
    Point& min( const double, const double );
    Point& max( const double, const double );
    Point& min( const Point& aPoint ) { return min( aPoint.x(), aPoint.y() ); }
    Point& max( const Point& aPoint ) { return max( aPoint.x(), aPoint.y() ); }
    ostream& print( ostream& ) const;

private:
    double my_x;
    double my_y;
};
#endif
