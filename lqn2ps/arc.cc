/* arc.cc	-- Greg Franks Thu Jan 30 2003
 *
 * $Id: arc.cc 15256 2021-12-25 01:47:40Z greg $
 */

#include "lqn2ps.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "arc.h"
#include "model.h"

Arc *
Arc::newArc( const unsigned size, const arrowhead_type arrow )
{
    static const std::map<const File_Format,Arc::create_func> new_arc = {
	{ File_Format::EEPIC,	    ArcTeX::create },
	{ File_Format::PSTEX,	    ArcFig::create },
	{ File_Format::POSTSCRIPT,  ArcPostScript::create },
	{ File_Format::FIG,	    ArcFig::create },
#if HAVE_GD_H && HAVE_LIBGD
#if HAVE_GDIMAGEGIFPTR
	{ File_Format::GIF,	    ArcGD::create },
#endif
#if HAVE_LIBJPEG 
	{ File_Format::JPEG,	    ArcGD::create },
#endif
#if HAVE_LIBPNG
	{ File_Format::PNG,	    ArcGD::create },
#endif
#endif
#if SVG_OUTPUT
	{ File_Format::SVG,	    ArcSVG::create },
#endif
#if SXD_OUTPUT
	{ File_Format::SXD,	    ArcSXD::create },
#endif
#if EMF_OUTPUT
	{ File_Format::EMF,	    ArcEMF::create },
#endif
#if X11_OUTPUT
	{ File_Format::X11,	    ArcX11::create },
#endif
    };

    std::map<const File_Format,Arc::create_func>::const_iterator f = new_arc.find( Flags::output_format() );
    if ( f != new_arc.end() ) {
	return (*(f->second))(size, arrow);
    } else {
	return ArcNull::create(size, arrow);
    }
}


/*
 * Assign iA to receiver.
 */

Arc&
Arc::operator=( const Arc &anArc )
{
    if ( this == &anArc ) return *this;
    myArrowhead = anArc.myArrowhead;
    std::vector<Point>::operator=( anArc );
    return *this;
}


double
Arc::arrowScaling() const
{
    return Flags::arrow_scaling * Model::scaling();
}



Arc& 
Arc::moveBy( const double dx, const double dy )
{
    for_each( begin(), end(), ExecXY<Point>( &Point::moveBy, dx, dy ) );
    return *this;
}

Arc& 
Arc::moveSrc( const double x, const double y )
{
    if ( size() > 0 ) {
	front().moveTo( x, y );
    }
    return *this;
}

Arc&
Arc::moveDst( const double x, const double y )
{
    if ( size() > 0 ) {
	back().moveTo( x, y );
    }
    return *this;
}


Arc& 
Arc::moveSrcBy( const double dx, const double dy )
{
    if ( size() > 0 ) {
	front().moveBy( dx, dy );
    }
    return *this;
}

Arc&
Arc::moveDstBy( const double dx, const double dy )
{
    if ( size() > 0 ) {
	back().moveBy( dx, dy );
    }
    return *this;
}


const Point& 
Arc::secondPoint() const 
{ 
    const unsigned n = size();
    const Point& p = front();
    for ( unsigned i = 1; i < n; ++i ) {
	if ( at(i) != p ) return at(i);
    }
    return back();
}

const Point& 
Arc::penultimatePoint() const 
{ 
    const unsigned n = size();
    const Point& p = back();
    for ( unsigned i = n - 2; i > 0; --i ) {
	if ( at(i) != p ) return at(i);
    }
    return front(); 
}

Arc&
Arc::moveSecond( const Point& dst )
{
    Point& src = const_cast<Point&>(secondPoint());
    if ( src != back() ) {
	src = dst;
    }
    return *this;
}

Arc&
Arc::movePenultimate( const Point& dst )
{
    Point& src = const_cast<Point&>(penultimatePoint());
    if ( src != front() ) {
	src = dst;
    }
    return *this;
}

Arc& 
Arc::scaleBy( const double sx, const double sy ) 
{
    for_each( begin(), end(), ExecXY<Point>( &Point::scaleBy, sx, sy ) );
    return *this;
}



Arc& 
Arc::translateY( const double dy ) 
{
    for_each( begin(), end(), Exec1<Point,double>( &Point::translateY, dy ) );
    return *this;
}


Point
Arc::pointFromSrc( const double offset ) const
{
    const Point & p1 = srcPoint();
    const Point & p2 = secondPoint();
    const double theta = atan2( p2.y() - p1.y(), p2.x() - p1.x() );
    if ( p2.y() > p1.y() ) {
	return Point( p1.x() + offset / tan( theta ), p1.y() + offset );
    } else {
	return Point( p1.x() - offset / tan( theta ), p1.y() - offset );
    }
}



Point
Arc::pointFromDst( const double offset ) const
{
    const Point& p1 = dstPoint();
    const Point& p2 = penultimatePoint();
    const double theta = atan2( p2.y() - p1.y(), p2.x() - p1.x() );
    if ( p2.y() > p1.y() ) {
	return Point( p1.x() + offset / tan( theta ), p1.y() + offset );
    } else {
	return Point( p1.x() - offset / tan( theta ), p1.y() - offset );
    }
}


Point 
Arc::srcIntersectsCircle( const Point& p3, const double r ) const
{
    const Point& p1 = srcPoint();
    const Point& p2 = secondPoint();
    return intersectsCircle( p1, p2, p3, r );
}


Point
Arc::dstIntersectsCircle( const Point& p3, const double r ) const
{
    const Point& p1 = penultimatePoint();
    const Point& p2 = dstPoint();
    return intersectsCircle( p1, p2, p3, r );
}


/*
 * http://astronomy.swin.edu.au/~pbourke/geometry/sphereline/ 
 * p1 and p2 define the line.
 * p3 is the center of the circle.
 */

Point
Arc::intersectsCircle( const Point& p1, const Point& p2, const Point& p3, const double r ) const
{
    const double dx = p2.x() - p1.x();
    const double dy = p2.y() - p1.y();

    const double a = square( dx ) + square( dy );
    const double b = 2.0 * ( ( dx * (p1.x() - p3.x()))
			   + ( dy * (p1.y() - p3.y())) );
    const double c = square( p3.x() ) + square( p3.y() )
	+ square( p1.x() ) + square( p1.y() ) 
	- 2.0 * ( p3.x() * p1.x() + p3.y() * p1.y() )
	- square( r );
    
    const double temp = square( b ) - 4.0 * a * c;
    if ( temp < 0 ) throw std::domain_error( "Does not intersect" );
    
    double mu1 = (-b + sqrt( temp )) / (2.0 * a);
    double mu2 = (-b - sqrt( temp )) / (2.0 * a);
    double ty1 = p1.y() + mu1 * dy;
    double ty2 = p1.y() + mu2 * dy;
    if ( (p1.y() >= ty1 && ty1 >= p2.y())
	|| (p2.y() >= ty1 && ty1 >= p1.y()) ) {
	return Point( p1.x() + mu1 * dx, ty1 );
    } else {
	return Point( p1.x() + mu2 * dx, ty2 );
    }
}



/*
 * Remove all duplicate points.  Shrink the arc as necessary.
 */

std::vector<Point>
Arc::removeDuplicates() const
{
    std::vector<Point> new_arc;
    Point last_point;
    for ( std::vector<Point>::const_iterator point = begin(); point != end(); ++point ) {
	if ( point == begin() || *point != last_point ) {
	    new_arc.push_back(*point);
	    last_point = *point;
	}
    }
    return new_arc;
}

#if EMF_OUTPUT
const ArcEMF&
ArcEMF::draw( std::ostream& output ) const
{
    std::vector<Point> points = removeDuplicates();
    if ( points.size() > 0 ) {
	EMF::polyline( output, points, penColour(), Arc::linestyle(), arrowhead(), arrowScaling() );
    }
    return *this;
}


std::ostream& 
ArcEMF::comment( std::ostream& output, const std::string& aString ) const
{
    /* Binary file format.  No operation. */
    return output;
}
#endif

const ArcFig&
ArcFig::draw( std::ostream& output ) const
{
    std::vector<Point> points = removeDuplicates();
    if ( points.size() > 0 ) {
	Fig::polyline( output, points, Fig::POLYLINE, penColour(), Graphic::Colour::TRANSPARENT, depth(), linestyle(), arrowhead(), arrowScaling() );
    }
    return *this;
}



std::ostream& 
ArcFig::comment( std::ostream& output, const std::string& aString ) const
{
    output << "# " << aString << std::endl;
    return output;
}

#if HAVE_GD_H && HAVE_LIBGD
const ArcGD&
ArcGD::draw( std::ostream& output ) const
{
    const unsigned int j = nPoints();
    if ( j < 2 ) return *this;
    std::vector<Point>::const_iterator p1 = begin();
    for ( std::vector<Point>::const_iterator p2 = p1 + 1; p2 != end(); ++p2 ) {
	if ( *p1 != *p2 ) {
	    GD::drawline( *p1, *p2, penColour(), linestyle() );
	    p1 = p2;
	}
    }
    /* Now draw the arrowhead */
    
    switch ( arrowhead() ) {
    case CLOSED_ARROW:
	arrowHead( penultimatePoint(), dstPoint(), arrowScaling(), penColour(), penColour() );
	break;
    case OPEN_ARROW:
	arrowHead( penultimatePoint(), dstPoint(), arrowScaling(), penColour(), Graphic::Colour::WHITE );
	break;
    }
    return *this;
}



/*
 * One can't put comments into PNG/JPEG output. :-)
 */

std::ostream& 
ArcGD::comment( std::ostream& output, const std::string& aString ) const
{
    return output;
}
#endif

const ArcPostScript&
ArcPostScript::draw( std::ostream& output ) const
{
    std::vector<Point> points = removeDuplicates();
    if ( points.size() > 0 ) {
	PostScript::polyline( output, points, penColour(), Arc::linestyle(), arrowhead(), arrowScaling() );
    }
    return *this;
}


std::ostream& 
ArcPostScript::comment( std::ostream& output, const std::string& aString ) const
{
    output << "% " << aString << std::endl;
    return output;
}

#if SVG_OUTPUT
const ArcSVG&
ArcSVG::draw( std::ostream& output ) const
{
    std::vector<Point> points = removeDuplicates();
    if ( points.size() > 0 ) {
	SVG::polyline( output, points, penColour(), Arc::linestyle(), arrowhead(), arrowScaling() );
    }
    return *this;
}


std::ostream& 
ArcSVG::comment( std::ostream& output, const std::string& aString ) const
{
    output << "<!-- " << aString << " -->" << std::endl;
    return output;
}
#endif

#if SXD_OUTPUT
const ArcSXD&
ArcSXD::draw( std::ostream& output ) const
{
    std::vector<Point> points = removeDuplicates();
    if ( points.size() > 0 ) {
	SXD::polyline( output, points, penColour(), Arc::linestyle(), arrowhead(), arrowScaling() );
    }
    return *this;
}


std::ostream& 
ArcSXD::comment( std::ostream& output, const std::string& aString ) const
{
    output << "<!-- " << aString << " -->" << std::endl;
    return output;
}
#endif

const ArcTeX&
ArcTeX::draw( std::ostream& output ) const
{
    std::vector<Point> points = removeDuplicates();
    if ( points.size() > 0 ) {
	TeX::polyline( output, points, penColour(), linestyle(), arrowhead(), arrowScaling() );
    }
    return *this;
}


std::ostream& 
ArcTeX::comment( std::ostream& output, const std::string& aString ) const
{
    output << "% " << aString << std::endl;
    return output;
}
