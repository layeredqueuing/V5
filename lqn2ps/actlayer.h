/* -*- c++ -*-
 * actlayer.h	-- Greg Franks
 *
 * $Id: actlayer.h 13477 2020-02-08 23:14:37Z greg $
 */

#ifndef _ACTLAYER_H
#define _ACTLAYER_H

#include "point.h"
#include "actlist.h"
#include "element.h"

class Activity;
class ActivityLayer;

class ActivityLayer
{
public:
    ActivityLayer() : origin(0,0), extent(0,0) {}
    ActivityLayer( const ActivityLayer& );

    int operator!() const { return myActivities.size() == 0; }	/* Layer is empty! */
    ActivityLayer& operator+=( Activity * );
    ActivityLayer& sort( compare_func_ptr compare );
    const std::vector<Activity *>& activities() const { return myActivities; }
    unsigned size() const { return myActivities.size(); }
    bool canMoveBy( const unsigned, const double ) const;
    ActivityLayer& clearContents();

    ActivityLayer& format( const double );
    ActivityLayer& reformat( const double );
    ActivityLayer& label();
    ActivityLayer& scaleBy( const double, const double );
    ActivityLayer& moveBy( const double, const double );
    ActivityLayer& moveTo( const double, const double );
    ActivityLayer& translateY( const double );
    ActivityLayer& depth( const unsigned );
    ActivityLayer& justify( const double, const justification_type );
    ActivityLayer& alignActivities();

    double x() const { return origin.x(); }
    double y() const { return origin.y(); }
    double width() const { return extent.x(); }
    double height() const { return extent.y(); }
    ActivityLayer& height( double y ) { extent.y( y ); return *this; }
    ostream& print( ostream& ) const;

private:
    ActivityLayer& shift( unsigned index, double amount );
    ActivityLayer& crop();

private:
    std::vector<Activity *> myActivities;
    Point origin;
    Point extent;
};

inline ostream& operator<<( ostream& output, const ActivityLayer& self ) { return self.print( output ); }
#endif
