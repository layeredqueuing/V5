/* -*- c++ -*-
 * element.h	-- Greg Franks
 *
 * $Id: key.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef SRVN2EEPIC_KEY_H
#define SRVN2EEPIC_KEY_H

#include "lqn2ps.h"
#include "point.h"
#include "cltn.h"

class Arc;
class Label;
class Key;

ostream& operator<<( ostream&, const Key& );

class Key 
{
private:
    Key( const Key& );
    Key& operator=( const Key& );

public:
    Key();
    ~Key();
    Key& init( Point&, Point& ); 

    Key& justification( const justification_type justify ) { myJustification = justify; return *this; }
    double width() const { return extent.x(); }
    double height() const { return extent.y(); }
    Key& moveTo( const double x, const double y );
    Key& moveBy( const double, const double );
    Key& scaleBy( const double, const double );
    Key& translateY( const double );

    Key& label();

    ostream& print( ostream& ) const;

private:
    Point origin;
    Point extent;
    justification_type myJustification;
    Cltn<Arc *> myArcs;
    Cltn<Label *> myLabels;
};

#endif
