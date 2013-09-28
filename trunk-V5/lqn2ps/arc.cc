/* arc.cc	-- Greg Franks Thu Jan 30 2003
 *
 * $Id$
 */

#include "lqn2ps.h"
#include <cmath>
#include <cstdlib>
#include "arc.h"
#include "model.h"

template <typename Type> Type square( Type a ) 
{
    return a * a;
}

/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Printing function.
 */

ostream&
operator<<( ostream& output, const Arc& self ) 
{
    return self.print( output );
}

Arc *
Arc::newArc( const unsigned size, const arrowhead_type arrow )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
	return new ArcTeX( size, arrow );	/* the graphical object		*/
    case FORMAT_PSTEX:
	return new ArcFig(size, arrow);		/* the graphical object		*/
    case FORMAT_POSTSCRIPT:
	return new ArcPostScript( size, arrow );/* the graphical object		*/
    case FORMAT_FIG:
	return new ArcFig( size, arrow );	/* the graphical object		*/
#if HAVE_GD_H && HAVE_LIBGD
#if HAVE_GDIMAGEGIFPTR
    case FORMAT_GIF:
#endif
#if HAVE_LIBJPEG 
    case FORMAT_JPEG:
#endif
#if HAVE_LIBPNG
    case FORMAT_PNG:
#endif
	return new ArcGD( size, arrow );
	break;
#endif
#if defined(SVG_OUTPUT)
    case FORMAT_SVG:
	return new ArcSVG( size, arrow );
#endif
#if defined(SXD_OUTPUT)
    case FORMAT_SXD:
	return new ArcSXD( size, arrow );
#endif
#if defined(EMF_OUTPUT)
    case FORMAT_EMF:
	return new ArcEMF( size, arrow );
#endif
#if defined(X11_OUTPUT)
    case FORMAT_X11:
	return new ArcX11( size, arrow );
#endif
	break;
    case FORMAT_NULL:
    case FORMAT_SRVN:
    case FORMAT_OUTPUT:
    case FORMAT_PARSEABLE:
    case FORMAT_RTF:
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
#endif
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
#endif
    case FORMAT_XML:
	return new ArcNull( size, arrow );
    default:
	abort();
    }
    return 0;
}


/*
 * Assign iA to receiver.
 */

Arc&
Arc::operator=( const Arc &anArc )
{
    if ( this == &anArc ) return *this;
    unsigned n = size();
    for ( unsigned i = 1; i <= n; ++i ) {
	(*this)[i] = anArc[i];
    }
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
    unsigned n = size();
    for ( unsigned i = 1; i <= n; ++i ) {
	(*this)[i].moveBy( dx, dy );
    }
    return *this;
}

Arc& 
Arc::moveSrc( const double x, const double y )
{
    if ( size() > 0 ) {
	(*this)[1].moveTo( x, y );
    }
    return *this;
}

Arc&
Arc::moveDst( const double x, const double y )
{
    const unsigned n = size();
    if ( n > 0 ) {
	(*this)[n].moveTo( x, y );
    }
    return *this;
}


Arc& 
Arc::moveSrcBy( const double dx, const double dy )
{
    if ( size() > 0 ) {
	(*this)[1].moveBy( dx, dy );
    }
    return *this;
}

Arc&
Arc::moveDstBy( const double dx, const double dy )
{
    const unsigned n = size();
    if ( n > 0 ) {
	(*this)[n].moveBy( dx, dy );
    }
    return *this;
}


Point& 
Arc::secondPoint() const 
{ 
    const unsigned n = size();
    for ( unsigned i = 2; i <= n; ++i ) {
	if ( (*this)[i] != (*this)[1] ) return (*this)[i];
    }
    return (*this)[n];
}

Point& 
Arc::penultimatePoint() const 
{ 
    const unsigned n = size();
    for ( unsigned i = n - 1; i >= 1; --i ) {
	if ( (*this)[i] != (*this)[n] ) return (*this)[i];
    }
    return (*this)[1]; 
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
    if ( temp < 0 ) throw DoesNotIntersect();
    
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



Arc& 
Arc::scaleBy( const double sx, const double sy ) 
{
    int n = size();
    for ( int i = 1; i <= n; ++i ) {
	(*this)[i].x( sx * (*this)[i].x() );
	(*this)[i].y( sy * (*this)[i].y() );
    }
    return *this;
}



Arc& 
Arc::translateY( const double dy ) 
{
    int n = size();
    for ( int i = 1; i <= n; ++i ) {
	(*this)[i].y( dy - (*this)[i].y() );
    }
    return *this;
}


/*
 * Remove all duplicate points.  Shrink the arc as necessary.
 */

unsigned
Arc::removeDuplicates() const
{
    const unsigned n = size();
    unsigned int j = 1;
    for ( unsigned int i = 2; i <= n; ++i ) {
	if ( (*this)[i] != (*this)[j] ) {
	    j += 1;
	    if ( i != j ) {
		(*this)[j] = (*this)[i];
	    }
	}
    }
    const_cast<Arc *>(this)->resize( j );
    return j;
}

#if defined(EMF_OUTPUT)
ostream&
ArcEMF::print( ostream& output ) const
{
    const unsigned j = removeDuplicates();
    if ( j < 2 ) return output;

    EMF::polyline( output, j, &(*this)[1], penColour(), Arc::linestyle(), arrowhead(), arrowScaling() );
    return output;
}


ostream& 
ArcEMF::comment( ostream& output, const string& aString ) const
{
    /* Binary file format.  No operation. */
    return output;
}
#endif

ostream&
ArcFig::print( ostream& output ) const
{
    const unsigned j = removeDuplicates();
    if ( j < 2 ) return output;

    Fig::polyline( output, j, &(*this)[1], Fig::POLYLINE, penColour(), Graphic::TRANSPARENT, depth(), linestyle(), arrowhead(), arrowScaling() );
    return output;
}



ostream& 
ArcFig::comment( ostream& output, const string& aString ) const
{
    output << "# " << aString << endl;
    return output;
}

#if HAVE_GD_H && HAVE_LIBGD
ostream&
ArcGD::print( ostream& output ) const
{
    const unsigned j = removeDuplicates();
    if ( j < 2 ) return output;

    for ( unsigned i = 1; i < j; ++i ) {    
	GD::drawline( (*this)[i], (*this)[i+1], penColour(), linestyle() );
    }
    /* Now draw the arrowhead */
    
    switch ( arrowhead() ) {
    case CLOSED_ARROW:
	arrowHead( (*this)[j-1], (*this)[j], arrowScaling(), penColour(), fillColour() );
	break;
    case OPEN_ARROW:
	arrowHead( (*this)[j-1], (*this)[j], arrowScaling(), penColour(), Graphic::WHITE );
	break;
    }
    return output;
}



/*
 * One can't put comments into PNG/JPEG output. :-)
 */

ostream& 
ArcGD::comment( ostream& output, const string& aString ) const
{
    return output;
}
#endif

ostream&
ArcPostScript::print( ostream& output ) const
{
    const unsigned j = removeDuplicates();
    if ( j < 2 ) return output;

    PostScript::polyline( output, j, &(*this)[1], penColour(), Arc::linestyle(), arrowhead(), arrowScaling() );
    return output;
}


ostream& 
ArcPostScript::comment( ostream& output, const string& aString ) const
{
    output << "% " << aString << endl;
    return output;
}

#if defined(SVG_OUTPUT)
ostream&
ArcSVG::print( ostream& output ) const
{
    const unsigned j = removeDuplicates();
    if ( j < 2 ) return output;

    SVG::polyline( output, j, &(*this)[1], penColour(), Arc::linestyle(), arrowhead(), arrowScaling() );
    return output;
}


ostream& 
ArcSVG::comment( ostream& output, const string& aString ) const
{
    output << "<!-- " << aString << " -->" << endl;
    return output;
}
#endif

#if defined(SXD_OUTPUT)
ostream&
ArcSXD::print( ostream& output ) const
{
    const unsigned j = removeDuplicates();
    if ( j < 2 ) return output;

    SXD::polyline( output, j, &(*this)[1], penColour(), Arc::linestyle(), arrowhead(), arrowScaling() );
    return output;
}


ostream& 
ArcSXD::comment( ostream& output, const string& aString ) const
{
    output << "<!-- " << aString << " -->" << endl;
    return output;
}
#endif

ostream&
ArcTeX::print( ostream& output ) const
{
    const unsigned j = removeDuplicates();
    if ( j < 2 ) return output;

    TeX::polyline( output, j, &(*this)[1], penColour(), linestyle(), arrowhead(), arrowScaling() );
    return output;
}


ostream& 
ArcTeX::comment( ostream& output, const string& aString ) const
{
    output << "% " << aString << endl;
    return output;
}
