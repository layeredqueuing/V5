/* point.cc	-- Greg Franks Wed Jan 29 2003
 *
 * $Id$
 */

#include "lqn2ps.h"
#include "point.h"
#include <cmath>

/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Printing function.
 */

ostream&
operator<<( ostream& output, const Point& self ) 
{
    return self.print( output );
}

/*
 * Assign iA to receiver.
 */

Point&
Point::operator=( const Point &aPoint )
{
    if ( this == &aPoint ) return *this;

    my_x = aPoint.x();
    my_y = aPoint.y();
    return *this;
}


ostream&
Point::print( ostream& output ) const
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
    my_x = ::min( my_x, x ); 
    my_y = ::min( my_y, y ); 
    return *this;
}


Point&
Point::max( const double x, const double y )
{
    my_x = ::max( my_x, x ); 
    my_y = ::max( my_y, y ); 
    return *this;
}
