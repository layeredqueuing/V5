/* -*- c++ -*-  
 * node.h	-- Greg Franks
 *
 * $Id$
 */

#ifndef _NODE_H
#define _NODE_H

#include <lqn2ps.h>
#include "point.h"
#include "graphic.h"

class Node : public Graphic
{
public:
    Node() : Graphic(), origin(0,0), extent(0,0) {}
    Node( const Point& aPoint ) : Graphic(), origin(aPoint), extent(0,0) {}
    Node(double x1, double y1, double x2, double y2 ) : Graphic(), origin(x1,y1), extent(x2,y2) {}

    static Node * newNode( double x, double y );

    Point topLeft() const;
    Point topRight() const;
    Point bottomLeft() const;
    Point bottomRight() const;
    Point center() const;
    Point topCenter() const;
    Point bottomCenter() const;
    double left() const { return origin.x(); }
    double right() const { return origin.x() + extent.x(); }
    double top() const { return origin.y() + extent.y(); }
    double bottom() const { return origin.y(); }

    /* Computation */

    virtual Node& moveTo( const double x, const double y ) { origin.moveTo( x, y ); return *this; }
    virtual Node& moveTo( const Point& aPoint ) { origin.moveTo( aPoint.x(), aPoint.y() ); return *this; }
    virtual Node& moveBy( const double x, const double y ) { origin.moveBy( x, y ); return *this; }
    virtual Node& moveBy( const Point& aPoint ) { origin.moveBy( aPoint.x(), aPoint.y() ); return *this; }
    virtual Node& scaleBy( const double, const double );
    virtual Node& translateY( const double );

    virtual int direction() const { return 1; }
    virtual ostream& polygon( ostream&, unsigned nPoints, Point points[] ) const = 0;
    virtual ostream& polyline( ostream&, unsigned nPoints, Point points[] ) const = 0;
    virtual ostream& circle( ostream& output, const Point&, const double radius ) const = 0;
    virtual ostream& rectangle( ostream& output ) const = 0;
    virtual ostream& roundedRectangle( ostream& output ) const = 0;
    virtual ostream& text( ostream& output, const Point&, const char * ) const = 0;

    ostream& multi_server( ostream& output, const Point&, const double radius ) const;
    ostream& open_source( ostream& output, const Point&, const double ) const;
    ostream& open_sink( ostream& output, const Point&, const double ) const;
    ostream& draw_queue( ostream& output, const Point&, const double ) const;

public:
    Point origin;
    Point extent;
};

#if defined(EMF_OUTPUT)
class NodeEMF : public Node, private EMF
{
public:
    NodeEMF(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual ostream& polygon( ostream& output, unsigned nPoints, Point points[] ) const;
    virtual ostream& polyline( ostream& output, unsigned nPoints, Point points[] ) const;
    virtual ostream& circle( ostream& output, const Point& center, const double radius ) const;
    virtual ostream& rectangle( ostream& output ) const;
    virtual ostream& roundedRectangle( ostream& output ) const;
    virtual ostream& text( ostream& output, const Point&, const char * ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
    virtual int direction() const { return -1; }
};
#endif

class NodeFig : public Node, protected Fig
{
public:
    NodeFig(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual ostream& polygon( ostream&, unsigned nPoints, Point points[] ) const;
    virtual ostream& polyline( ostream&, unsigned nPoints, Point points[] ) const;
    virtual ostream& circle( ostream& output, const Point& center, const double radius  ) const;
    virtual ostream& rectangle( ostream& output ) const;
    virtual ostream& roundedRectangle( ostream& output ) const;
    virtual ostream& text( ostream& output, const Point&, const char * ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
    virtual int direction() const { return -1; }
};

class NodePostScript : public Node, private PostScript
{
public:
    NodePostScript(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual ostream& polygon( ostream&, unsigned nPoints, Point points[] ) const;
    virtual ostream& polyline( ostream&, unsigned nPoints, Point points[] ) const;
    virtual ostream& circle( ostream& output, const Point& center, const double radius  ) const;
    virtual ostream& rectangle( ostream& output ) const;
    virtual ostream& roundedRectangle( ostream& output ) const;
    virtual ostream& text( ostream& output, const Point&, const char * ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};

class NodeNull : public Node 
{
public:
    NodeNull(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual ostream& polygon( ostream& output, unsigned nPoints, Point points[] ) const { return output; }
    virtual ostream& polyline( ostream& output, unsigned nPoints, Point points[] ) const { return output; }
    virtual ostream& circle( ostream& output, const Point& center, const double radius  ) const { return output; }
    virtual ostream& rectangle( ostream& output ) const { return output; }
    virtual ostream& roundedRectangle( ostream& output ) const  { return output; }
    virtual ostream& text( ostream& output, const Point&, const char * ) const { return output; }
    virtual ostream& comment( ostream& output, const string& ) const { return output; }
};

class NodePsTeX : public NodeFig
{
public:
    NodePsTeX(double x1, double y1, double x2, double y2 ) : NodeFig(x1,y1,x2,y2) {}
    virtual ostream& text( ostream& output, const Point&, const char * ) const;
};

#if defined(SVG_OUTPUT)
class NodeSVG : public Node, private SVG
{
public:
    NodeSVG(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual ostream& polygon( ostream& output, unsigned nPoints, Point points[] ) const;
    virtual ostream& polyline( ostream& output, unsigned nPoints, Point points[] ) const;
    virtual ostream& circle( ostream& output, const Point& center, const double radius ) const;
    virtual ostream& rectangle( ostream& output ) const;
    virtual ostream& roundedRectangle( ostream& output ) const;
    virtual ostream& text( ostream& output, const Point&, const char * ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
    virtual int direction() const { return -1; }
};
#endif

#if defined(SXD_OUTPUT)
class NodeSXD : public Node, private SXD
{
public:
    NodeSXD(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual int direction() const { return -1; }
    virtual ostream& polygon( ostream& output, unsigned nPoints, Point points[] ) const;
    virtual ostream& polyline( ostream& output, unsigned nPoints, Point points[] ) const;
    virtual ostream& circle( ostream& output, const Point& center, const double radius ) const;
    virtual ostream& rectangle( ostream& output ) const;
    virtual ostream& roundedRectangle( ostream& output ) const;
    virtual ostream& text( ostream& output, const Point&, const char * ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};
#endif

class NodeTeX : public Node, private TeX
{
public:
    NodeTeX(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual ostream& polygon( ostream&, unsigned nPoints, Point points[] ) const;
    virtual ostream& polyline( ostream&, unsigned nPoints, Point points[] ) const;
    virtual ostream& circle( ostream& output, const Point& center, const double radius  ) const;
    virtual ostream& rectangle( ostream& output ) const;
    virtual ostream& roundedRectangle( ostream& output ) const;
    virtual ostream& text( ostream& output, const Point&, const char * ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
};

#if defined(X11_OUTPUT)
class NodeX11 : public Node, private X11
{
public:
    NodeX11(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual ostream& polygon( ostream& output, unsigned nPoints, Point points[] ) const { return output; }
    virtual ostream& polyline( ostream& output, unsigned nPoints, Point points[] ) const { return output; }
    virtual ostream& circle( ostream& output, const Point& center, const double radius  ) const{ return output; }
    virtual ostream& rectangle( ostream& output ) const;
    virtual ostream& roundedRectangle( ostream& output ) const;
    virtual ostream& text( ostream& output, const Point&, const char * ) const { return output; }
    virtual ostream& comment( ostream& output, const string& ) const;
};
#endif

#if HAVE_GD_H && HAVE_LIBGD
class NodeGD : public Node, private GD
{
public:
    NodeGD(double x1, double y1, double x2, double y2 ) : Node(x1,y1,x2,y2) {}
    virtual ostream& polygon( ostream& output, unsigned nPoints, Point points[] ) const;
    virtual ostream& polyline( ostream& output, unsigned nPoints, Point points[] ) const;
    virtual ostream& circle( ostream& output, const Point& center, const double radius ) const;
    virtual ostream& rectangle( ostream& output ) const;
    virtual ostream& roundedRectangle( ostream& output ) const;
    virtual ostream& text( ostream& output, const Point&, const char * ) const;
    virtual ostream& comment( ostream& output, const string& ) const;
    virtual int direction() const { return -1; }
};
#endif
#endif
