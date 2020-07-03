/* -*- c++ -*-
 * element.h	-- Greg Franks
 *
 * $Id: key.h 13477 2020-02-08 23:14:37Z greg $
 */

#ifndef SRVN2EEPIC_KEY_H
#define SRVN2EEPIC_KEY_H

#include "lqn2ps.h"
#include <vector>
#include "point.h"

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
    std::map<Label *, Arc *> myLabels;
};

#endif
