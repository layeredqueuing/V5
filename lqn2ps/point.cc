/* point.cc	-- Greg Franks Wed Jan 29 2003
 *
 * $Id: point.cc 13477 2020-02-08 23:14:37Z greg $
 */

#include "point.h"
#include <cmath>
#include <algorithm>

/*
 * Assign iA to receiver.
 */

Point&
Point::operator=( const Point &point )
{
    if ( this == &point ) return *this;

    _x = point.x();
    _y = point.y();
    return *this;
}


std::ostream&
Point::print( std::ostream& output ) const
{
    output << "(" << x() << "," << y() << ")";
    return output;
}


/*
 * Rotate the receiver about origin.
 */

Point&
Point::rotate( const Point& origin, const double theta ) 
{
    const double dx = x() - origin.x();
    const double dy = y() - origin.y();
    double sinTheta = sin( theta );
    double cosTheta = cos( theta );

    moveTo( dx * cosTheta - dy * sinTheta + origin.x(),
	    dx * sinTheta + dy * cosTheta + origin.y() );
    return *this;
}


Point&
Point::min( const double x, const double y )
{
    _x = std::min( _x, x ); 
    _y = std::min( _y, y ); 
    return *this;
}


Point&
Point::max( const double x, const double y )
{
    _x = std::max( _x, x ); 
    _y = std::max( _y, y ); 
    return *this;
}
