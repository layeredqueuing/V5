/* -*- c++ -*-
 * point.h	-- Greg Franks
 *
 * $Id: point.h 13477 2020-02-08 23:14:37Z greg $
 */

#ifndef _POINT_H
#define _POINT_H

#include <iostream>

class Point
{
public:
    Point() : _x(0), _y(0) {}
    Point( double x, double y ) : _x(x), _y(y) {}
    Point( const Point &point ) : _x(point.x()), _y(point.y()) {}
    Point& operator=( const Point& );
    Point& operator+=( const Point& point ) { _x += point.x(); _y += point.y(); return *this; }
    Point& operator-=( const Point& point ) { _x -= point.x(); _y -= point.y(); return *this; }
    Point& operator*=( const double s ) { _x *= s; _y *= s; return *this; }
    bool operator==( const Point& p2 ) const { return x() == p2.x() && y() == p2.y(); }
    bool operator!=( const Point& p2 ) const { return x() != p2.x() || y() != p2.y(); }

    Point& moveBy( const Point& point ) { _x += point.x(); _y += point.y(); return *this; }
    Point& moveBy( const double dx, const double dy ) { _x += dx; _y += dy; return *this; }
    Point& moveTo( const double x, const double y ) { _x = x; _y = y; return *this; }
    Point& scaleBy( const double sx, const double sy ) { _x *= sx; _y *= sy; return *this; }
    Point& translateY( const double dy ) { _y = dy - _y; return *this;}
    Point& rotate( const Point& origin, const double theta );
    double x() const { return _x; }
    double y() const { return _y; }
    Point& x( const double x ) { _x = x; return *this; }
    Point& y( const double y  ) { _y = y;  return *this; }
    Point& min( const double, const double );
    Point& max( const double, const double );
    Point& min( const Point& point ) { return min( point.x(), point.y() ); }
    Point& max( const Point& point ) { return max( point.x(), point.y() ); }
    std::ostream& print( std::ostream& ) const;

private:
    double _x;
    double _y;
};

inline std::ostream& operator<<( std::ostream& output, const Point& self ) { return self.print( output ); }
#endif
