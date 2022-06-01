/* -*- c++ -*- 
 * arc.h	-- Greg Franks
 *
 * $Id: arc.h 15614 2022-06-01 12:17:43Z greg $
 */

#ifndef _ARC_H
#define _ARC_H

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "point.h"
#include "graphic.h"

class Arc : public Graphic, protected std::vector<Point>
{
    typedef Arc * (*create_func)( const unsigned , const Arrowhead );

private:
    Arc( const Arc& ) = delete;

public:
    Arc( const unsigned size, const Arrowhead arrowhead ) 
	: Graphic(), std::vector<Point>(size), _arrowhead(arrowhead)
	{}
    virtual ~Arc() {}
    Arc& operator=( const Arc& );

    static Arc * newArc( const unsigned size = 2 , const Arrowhead arrowhead = Graphic::Arrowhead::CLOSED );
    unsigned int nPoints() const { return size(); }
    Arc& resize( unsigned int size ) { std::vector<Point>::resize( size ); return *this; }
    const Point& srcPoint() const { return front(); }
    const Point& secondPoint() const;
    Point& pointAt( unsigned i ) { return std::vector<Point>::at(i); }
    const Point& pointAt( unsigned int i ) const { return std::vector<Point>::at(i); }
    const Point& penultimatePoint() const;
    const Point& dstPoint() const { return back(); }
    
    /* Computation */

    Arc& moveBy( const double dx, const double dy );
    Arc& moveBy( const Point& aPoint ) { moveBy( aPoint.x(), aPoint.y() ); return *this; } 
    Arc& moveDst( const double dx, const double dy );
    Arc& moveDst( const Point& aPoint ) { moveDst( aPoint.x(), aPoint.y() ); return *this; } 
    Arc& moveDstBy( const double dx, const double dy );
    Arc& moveSrc( const double dx, const double dy );
    Arc& moveSrc( const Point& aPoint ) { moveSrc( aPoint.x(), aPoint.y() ); return *this; } 
    Arc& moveSrcBy( const double dx, const double dy );
    Arc& moveSecond( const Point& aPoint );
    Arc& movePenultimate( const Point& aPoint );
    Arc& scaleBy( const double, const double );
    Arc& translateY( const double );
    virtual int direction() const { return 1; }

    Point pointFromSrc( const double offset ) const;
    Point pointFromDst( const double offset ) const;
    Point srcIntersectsCircle( const Point& aPoint, const double r ) const;
    Point dstIntersectsCircle( const Point& aPoint, const double r ) const;

    Arc& arrowhead( Arrowhead arrowhead ) { _arrowhead = arrowhead; return *this; }
    Arrowhead arrowhead() const { return _arrowhead; }
    double arrowScaling() const;

    virtual const Arc& draw( std::ostream& ) const = 0;

protected:
    std::vector<Point> removeDuplicates() const;

private:
    Point intersectsCircle( const Point&, const Point&, const Point&, const double r ) const;

protected:
    Arrowhead _arrowhead;
};

inline std::ostream& operator<<( std::ostream& output, const Arc& self ) { self.draw( output ); return output; }

#if EMF_OUTPUT
class ArcEMF : public Arc, private EMF
{
protected:
    ArcEMF( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcEMF(size,arrowhead); }
    virtual const ArcEMF& draw( std::ostream& ) const;
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
};
#endif

class ArcFig : public Arc, private Fig
{
protected:
    ArcFig( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}    
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcFig(size,arrowhead); }
    virtual int direction() const { return -1; }

    virtual const ArcFig& draw( std::ostream& ) const;
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
};

#if HAVE_GD_H && HAVE_LIBGD
class ArcGD : public Arc, private GD
{
protected:
    ArcGD( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}    
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcGD(size,arrowhead); }
    virtual int direction() const { return -1; }

    virtual const ArcGD& draw( std::ostream& ) const;
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
};
#endif

class ArcNull : public Arc
{
protected:
    ArcNull( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}    
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcNull(size,arrowhead); }
    virtual int direction() const { return 1; }

    virtual const ArcNull& draw( std::ostream& output ) const { return *this; }
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const { return output; }
};

class ArcPostScript : public Arc, private PostScript
{
protected:
    ArcPostScript( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}    
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcPostScript(size,arrowhead); }
    virtual const ArcPostScript& draw( std::ostream& ) const;
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
};

#if SVG_OUTPUT
class ArcSVG : public Arc, private SVG
{
protected:
    ArcSVG( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}    
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcSVG(size,arrowhead); }
    virtual const ArcSVG& draw( std::ostream& ) const;
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
};
#endif

#if SXD_OUTPUT
class ArcSXD : public Arc, private SXD
{
protected:
    ArcSXD( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}    
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcSXD(size,arrowhead); }
    virtual const ArcSXD& draw( std::ostream& ) const;
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
};
#endif

class ArcTeX : public Arc, private TeX
{
protected:
    ArcTeX( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}    
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcTeX(size,arrowhead); }
    virtual const ArcTeX& draw( std::ostream& ) const;
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
};

#if X11_OUTPUT
class ArcX11 : public Arc, private X11
{
protected:
    ArcX11( const unsigned size, const Arrowhead arrowhead ) : Arc(size,arrowhead) {}    
public:
    static Arc * create( const unsigned size, const Arrowhead arrowhead ) { return new ArcX11(size,arrowhead); }
    virtual std::ostream& print( std::ostream& output ) const { return output; }
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
};
#endif
#endif
