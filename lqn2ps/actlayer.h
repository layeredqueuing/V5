/* -*- c++ -*-
 * actlayer.h	-- Greg Franks
 *
 * $Id: actlayer.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef _ACTLAYER_H
#define _ACTLAYER_H

#include "point.h"
#include "cltn.h"

class Activity;
class ActivityLayer;

ostream& operator<<( ostream&, const ActivityLayer& );

class ActivityLayer
{
private:
    ActivityLayer( const ActivityLayer& );

public:
    ActivityLayer() : origin(0,0), extent(0,0) {}

    ActivityLayer& operator<<( Activity * );
    ActivityLayer& operator+=( Activity * );
    ActivityLayer const & sort( compare_func_ptr compare ) const;
    const Cltn<Activity *>& activities() const { return myActivities; }
    unsigned size() const { return myActivities.size(); }
    bool canMoveBy( const unsigned, const double ) const;
    ActivityLayer& clearContents();

    ActivityLayer const& format( const double ) const;
    ActivityLayer const& reformat( const double ) const;
    ActivityLayer const& label() const;
    ActivityLayer const& scaleBy( const double, const double ) const;
    ActivityLayer const& moveBy( const double, const double ) const;
    ActivityLayer const& moveTo( const double, const double ) const;
    ActivityLayer const& translateY( const double ) const;
    ActivityLayer const& depth( const unsigned ) const;
    ActivityLayer const& justify( const double, const justification_type ) const;
    ActivityLayer const& alignActivities() const;

    double x() const { return origin.x(); }
    double y() const { return origin.y(); }
    double width() const { return extent.x(); }
    double height() const { return extent.y(); }
    ActivityLayer& height( double y ) { extent.y( y ); return *this; }
    ostream& print( ostream& ) const;

private:
    ActivityLayer const& shift( unsigned index, double amount ) const;
    ActivityLayer const& crop() const;

private:
    Cltn<Activity *> myActivities;
    mutable Point origin;
    mutable Point extent;
};

#endif
