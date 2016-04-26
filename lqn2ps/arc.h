/* -*- c++ -*- 
 * arc.h	-- Greg Franks
 *
 * $Id: arc.h 11963 2014-04-10 14:36:42Z greg $
 */

#ifndef _ARC_H
#define _ARC_H

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "point.h"
#include "graphic.h"
#include "vector.h"

class Arc;
ostream& operator<<( ostream&, const Arc& );

class DoesNotIntersect
{
public:
    DoesNotIntersect() {}
};


class Arc : public Graphic, public Vector2<Point>
{
private:
    Arc( const Arc& );

public:
    Arc( const unsigned size, const arrowhead_type anArrowhead ) 
	: Graphic(), myArrowhead(anArrowhead)
	{ grow(size); }
    virtual ~Arc() {}
    Arc& operator=( const Arc& );

    static Arc * newArc( const unsigned size = 2 , const arrowhead_type anArrowhead = CLOSED_ARROW );

    /* Computation */

    Arc& moveBy( const double dx, const double dy );
    Arc& moveBy( const Point& aPoint ) { moveBy( aPoint.x(), aPoint.y() ); return *this; } 
    Point& srcPoint() const { return (*this)[1]; }
    Point& secondPoint() const;
    Point& penultimatePoint() const;
    Point& dstPoint() const { return (*this)[size()]; }
    Arc& moveDst( const double dx, const double dy );
    Arc& moveDst( const Point& aPoint ) { moveDst( aPoint.x(), aPoint.y() ); return *this; } 
    Arc& moveDstBy( const double dx, const double dy );
    Arc& moveSrc( const double dx, const double dy );
    Arc& moveSrc( const Point& aPoint ) { moveSrc( aPoint.x(), aPoint.y() ); return *this; } 
    Arc& moveSrcBy( const double dx, const double dy );
    Arc& scaleBy( const double, const double );
    Arc& translateY( const double );
    virtual int direction() const { return 1; }

    Point pointFromSrc( const double offset ) const;
    Point pointFromDst( const double offset ) const;
    Point srcIntersectsCircle( const Point& aPoint, const double r ) const;
    Point dstIntersectsCircle( const Point& aPoint, const double r ) const;

    Arc& arrowhead( arrowhead_type anArrowhead ) { myArrowhead = anArrowhead; return *this; }
    arrowhead_type arrowhead() const { return myArrowhead; }
    double arrowScaling() const;

    virtual ostream& print( ostream& ) const = 0;

protected:
    unsigned removeDuplicates() const;

private:
    Point intersectsCircle( const Point&, const Point&, const Point&, const double r ) const;

protected:
    arrowhead_type myArrowhead;
};

#if defined(EMF_OUTPUT)
class ArcEMF : public Arc, private EMF
{
public:
    ArcEMF( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual ostream& print( ostream& ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};
#endif

class ArcFig : public Arc, private Fig
{
public:
    ArcFig( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual int direction() const { return -1; }

    virtual ostream& print( ostream& ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};

#if HAVE_GD_H && HAVE_LIBGD
class ArcGD : public Arc, private GD
{
public:
    ArcGD( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual int direction() const { return -1; }

    virtual ostream& print( ostream& output ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};
#endif

class ArcNull : public Arc
{
public:
    ArcNull( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual int direction() const { return 1; }

    virtual ostream& print( ostream& output ) const { return output; }
    virtual ostream& comment( ostream& output, const string& ) const { return output; }
};

class ArcPostScript : public Arc, private PostScript
{
public:
    ArcPostScript( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual ostream& print( ostream& ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};

#if defined(SVG_OUTPUT)
class ArcSVG : public Arc, private SVG
{
public:
    ArcSVG( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual ostream& print( ostream& ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};
#endif

#if defined(SXD_OUTPUT)
class ArcSXD : public Arc, private SXD
{
public:
    ArcSXD( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual ostream& print( ostream& ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};
#endif

class ArcTeX : public Arc, private TeX
{
public:
    ArcTeX( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual ostream& print( ostream& ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};

#if defined(X11_OUTPUT)
class ArcX11 : public Arc, private X11
{
public:
    ArcX11( const unsigned size, const arrowhead_type anArrowHead ) : Arc(size,anArrowHead) {}
    virtual ostream& print( ostream& output ) const { return output; }
    virtual ostream& comment( ostream& output, const string& ) const;
};
#endif
#endif
