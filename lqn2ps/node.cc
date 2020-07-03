/* node.cc	-- Greg Franks Wed Jan 29 2003
 *
 * $Id: node.cc 13477 2020-02-08 23:14:37Z greg $
 */

#include "lqn2ps.h"
#include <stdarg.h>
#include <cstdlib>
#include <cstring>
#include "node.h"
#include "arc.h"

static string tex_string( const char * s );
static string xml_comment( const string& );
static string unicode_string( const char * s );

Node *
Node::newNode( double x, double y )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
	return new NodeTeX( 0, 0, x, y );
#if defined(EMF_OUTPUT)
    case FORMAT_EMF:
	return new NodeEMF( 0, 0, x, y );
#endif
    case FORMAT_FIG:
	return new NodeFig( 0, 0, x, y );
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
	return new NodeGD( 0, 0, x, y );
#endif	/* HAVE_LIBGD */
    case FORMAT_POSTSCRIPT:
	return new NodePostScript( 0, 0, x, y );	/* the graphical object		*/
    case FORMAT_PSTEX:
	return new NodePsTeX( 0, 0, x, y );
#if defined(SVG_OUTPUT)
    case FORMAT_SVG:
	return new NodeSVG( 0, 0, x, y );
#endif
#if defined(SXD_OUTPUT)
    case FORMAT_SXD:
	return new NodeSXD( 0, 0, x, y );
#endif
#if defined(X11_OUTPUT)
    case FORMAT_X11:
	return new NodeX11( 0, 0, x, y );
#endif
    default:
	return new NodeNull( 0, 0, x, y );
    }
    abort();
    return 0;
}

Node&
Node::scaleBy( const double sx, const double sy )
{
    origin.scaleBy( sx, sy );
    extent.scaleBy( sx, sy );
    return *this;
}


Node&
Node::translateY( const double dy )
{
    origin.y( dy - origin.y() );
    extent.y( -extent.y() );
    return *this;
}

Node&
Node::resizeBox( const double x, const double y, const double w, const double h )
{
    origin.moveBy( x, y );
    extent.moveBy( w, h );
    return *this;
}


ostream& 
Node::draw_queue( ostream& output, const Point& aPoint, const double radius ) const
{
    std::vector<Point> points(4);
    points[0] = aPoint;
    points[1] = aPoint;
    points[2] = aPoint;
    points[3] = aPoint;
    points[0].moveBy( -radius, 4 * radius * direction() );
    points[1].moveBy( -radius, 2 * radius * direction() );
    points[2].moveBy( radius,  2 * radius * direction() );
    points[3].moveBy( radius,  4 * radius * direction() );
    polyline( output, points );
    return output;
}

ostream&
Node::multi_server( ostream& output, const Point& centerBottom, const double radius ) const
{
    Arc * anArc = Arc::newArc( 3, Graphic::NO_ARROW );
    const double offset = radius * 14.0 / 9.0;

    anArc->penColour( penColour() );
    anArc->pointAt(0) = centerBottom;
    anArc->pointAt(1) = centerBottom;
    anArc->pointAt(2) = centerBottom;
    anArc->pointAt(0).moveBy( -offset, radius  * direction());
    anArc->pointAt(1).moveBy( 0, 2 * radius  * direction());
    anArc->pointAt(2).moveBy( offset, radius  * direction());

    circle( output, anArc->pointAt(0), radius );
    circle( output, anArc->pointAt(2), radius );
    anArc->pointAt(0) = anArc->srcIntersectsCircle( anArc->pointAt(0), radius );
    anArc->pointAt(2) = anArc->dstIntersectsCircle( anArc->pointAt(2), radius );
    output << *anArc;

    anArc->pointAt(0) = centerBottom;
    anArc->pointAt(1) = centerBottom;
    anArc->pointAt(2) = centerBottom;
    anArc->pointAt(0).moveBy( -offset, radius * direction());
    anArc->pointAt(2).moveBy( offset, radius * direction());
    anArc->pointAt(0) = anArc->srcIntersectsCircle( anArc->pointAt(0), radius );
    anArc->pointAt(2) = anArc->dstIntersectsCircle( anArc->pointAt(2), radius );
    output << *anArc;

    delete anArc;
    return output;
}


ostream&
Node::open_source( ostream& output, const Point& centerBottom, const double radius ) const
{
    std::vector<Point> points(5);
    const double top = radius * direction() * 2.0;
    const double y1  = radius * direction() * 0.5;
    points[0] = points[1] = points[2] = points[3] = points[4] = centerBottom;
    points[0].moveBy( -radius, top );
    points[1].moveBy( radius, top );
    points[2].moveBy( radius, y1 ); 
    points[4].moveBy( -radius, y1 );
    polygon( output, points );

    return output;
}

ostream&
Node::open_sink( ostream& output, const Point& centerTop, const double radius ) const
{
    std::vector<Point> points(5);
    const double bot = radius * -direction() * 2.0;
    const double y1  = radius * -direction() * 0.5;
    points[0] = points[1] = points[2] = points[3] = points[4] = centerTop;
    points[0].moveBy( -radius, 0 );
    points[1].moveBy( 0, y1 );
    points[2].moveBy( radius, 0 );
    points[3].moveBy( radius, bot );
    points[4].moveBy( -radius, bot );
    polygon( output, points );

    return output;
}

#if defined(EMF_OUTPUT)
/* -------------------------------------------------------------------- */
/* Windows Enhanced Meta File output					*/
/* -------------------------------------------------------------------- */

ostream&
NodeEMF::polygon( ostream& output, const std::vector<Point>& points ) const
{
    EMF::polygon( output, points, penColour(), fillColour() );
    return output;
}


ostream&
NodeEMF::polyline( ostream& output, const std::vector<Point>& points ) const
{
    EMF::polyline( output, points, penColour() );
    return output;
}


ostream&
NodeEMF::circle( ostream& output, const Point& c, const double r ) const
{
    EMF::circle( output, c, r, penColour(), fillColour() );
    return output;
}


ostream&
NodeEMF::rectangle( ostream& output ) const
{
    EMF::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

ostream&
NodeEMF::roundedRectangle( ostream& output ) const
{
    EMF::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

ostream&
NodeEMF::text( ostream& output, const Point& c, const char * s ) const
{
    string aStr = unicode_string( s );
    EMF::text( output, c, aStr, Graphic::NORMAL_FONT, Flags::print[FONT_SIZE].value.i, CENTER_JUSTIFY, penColour() );
    return output;
}

ostream& 
NodeEMF::comment( ostream& output, const string& aString ) const
{
    /* Binary file format.  No operation. */
    return output;
}
#endif

/* -------------------------------------------------------------------- */
/* XFIG output								*/
/* -------------------------------------------------------------------- */

ostream&
NodeFig::polygon( ostream& output, const std::vector<Point>& points ) const
{
    Fig::polyline( output, points, Fig::POLYGON, penColour(), fillColour(), depth() );
    return output;
}


ostream&
NodeFig::polyline( ostream& output, const std::vector<Point>& points ) const
{
    Fig::polyline( output, points, Fig::POLYLINE, penColour(), Graphic::TRANSPARENT, depth(), linestyle() );
    return output;
}


ostream&
NodeFig::circle( ostream& output, const Point& c, const double r ) const
{
    Fig::circle( output, c, r, 3, penColour(), fillColour(), depth(), fillStyle() );
    return output;
}


ostream&
NodeFig::rectangle( ostream& output ) const
{
    Fig::rectangle( output, origin, extent, penColour(), fillColour(), depth(), linestyle() );
    return output;
}

ostream&
NodeFig::roundedRectangle( ostream& output ) const
{
    Fig::roundedRectangle( output, origin, extent, penColour(), Graphic::TRANSPARENT, depth(), linestyle() );
    return output;
}

ostream&
NodeFig::text( ostream& output, const Point& c, const char * s ) const
{
    Fig::text( output, c, s, Graphic::NORMAL_FONT, Flags::print[FONT_SIZE].value.i, CENTER_JUSTIFY, penColour(), Fig::POSTSCRIPT );
    return output;
}


ostream& 
NodeFig::comment( ostream& output, const string& aString ) const
{
    output << "# " << aString << endl;
    return output;
}

#if HAVE_GD_H && HAVE_LIBGD
/* -------------------------------------------------------------------- */
/* GD (Jpeg, PNG, GIF ) output						*/
/* -------------------------------------------------------------------- */

ostream&
NodeGD::polygon( ostream& output, const std::vector<Point>& points ) const
{
    GD::polygon( points, penColour(), fillColour() );
    return output;
}


ostream&
NodeGD::polyline( ostream& output, const std::vector<Point>& points ) const
{
    for ( unsigned i = 1; i < points.size(); ++i ) {    
	GD::drawline( points[i-1], points[i], penColour(), linestyle() );
    }
    return output;
}


ostream&
NodeGD::circle( ostream& output, const Point& c, const double r ) const
{
    GD::circle( c, r, penColour(), fillColour() );
    return output;
}


ostream&
NodeGD::rectangle( ostream& output ) const
{
    GD::rectangle( origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

ostream&
NodeGD::roundedRectangle( ostream& output ) const
{
    GD::rectangle( origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

ostream&
NodeGD::text( ostream& output, const Point& c, const char * s ) const
{
    Point aPoint( c );
    gdFont * font = GD::getfont();
    aPoint.moveBy( 0, -font->h );
    GD::text( aPoint, s, Graphic::NORMAL_FONT, Flags::print[FONT_SIZE].value.i, CENTER_JUSTIFY, penColour() );
    return output;
}


/*
 * One can't put comments into PNG/JPEG output. :-)
 */

ostream& 
NodeGD::comment( ostream& output, const string& aString ) const
{
    return output;
}
#endif

/* -------------------------------------------------------------------- */
/* PostScript output							*/
/* -------------------------------------------------------------------- */

ostream&
NodePostScript::polygon( ostream& output, const std::vector<Point>& points ) const
{
    PostScript::polygon( output, points, penColour(), fillColour() );
    return output;
}


ostream&
NodePostScript::polyline( ostream& output, const std::vector<Point>& points ) const
{
    PostScript::polyline( output, points, penColour(), Graphic::linestyle() );
    return output;
}


ostream&
NodePostScript::circle( ostream& output, const Point& c, const double r ) const
{
    PostScript::circle( output, c, r, penColour(), fillColour() );
    return output;
}


ostream&
NodePostScript::rectangle( ostream& output ) const
{
    PostScript::rectangle( output, origin, extent, penColour(), fillColour(), Graphic::linestyle() );
    return output;
}

ostream&
NodePostScript::roundedRectangle( ostream& output ) const
{
    PostScript::roundedRectangle( output, origin, extent, penColour(), Graphic::TRANSPARENT, Graphic::linestyle() );
    return output;
}

ostream&
NodePostScript::text( ostream& output, const Point& c, const char * s ) const
{
    PostScript::text( output, c, s, Graphic::NORMAL_FONT, Flags::print[FONT_SIZE].value.i, CENTER_JUSTIFY, penColour() );
    return output;
}


ostream& 
NodePostScript::comment( ostream& output, const string& aString ) const
{
    output << "% " << aString << endl;
    return output;
}

ostream&
NodePsTeX::text( ostream& output, const Point& c, const char * s ) const
{
    string aStr = tex_string( s );
    Fig::text( output, c, aStr, Graphic::NORMAL_FONT, Flags::print[FONT_SIZE].value.i, CENTER_JUSTIFY, penColour(), 
	       Fig::SPECIAL );
    return output;
}

#if defined(SVG_OUTPUT)
/* -------------------------------------------------------------------- */
/* Scalable Vector Grahpics Ouptut					*/
/* -------------------------------------------------------------------- */

ostream&
NodeSVG::polygon( ostream& output, const std::vector<Point>& points ) const
{
    SVG::polygon( output, points, penColour(), fillColour() );
    return output;
}


ostream&
NodeSVG::polyline( ostream& output, const std::vector<Point>& points ) const
{
    SVG::polyline( output, points, penColour(), Graphic::linestyle() );
    return output;
}


ostream&
NodeSVG::circle( ostream& output, const Point& c, const double r ) const
{
    SVG::circle( output, c, r, penColour(), fillColour() );
    return output;
}


ostream&
NodeSVG::rectangle( ostream& output ) const
{
    SVG::rectangle( output, origin, extent, penColour(), fillColour(), Graphic::linestyle() );
    return output;
}

ostream&
NodeSVG::roundedRectangle( ostream& output ) const
{
    SVG::rectangle( output, origin, extent, penColour(), fillColour(), Graphic::linestyle() );
    return output;
}

ostream&
NodeSVG::text( ostream& output, const Point& c, const char * s ) const
{
    SVG::text( output, c, s, Graphic::NORMAL_FONT, Flags::print[FONT_SIZE].value.i, CENTER_JUSTIFY, penColour() );
    return output;
}

ostream& 
NodeSVG::comment( ostream& output, const string& aString ) const
{
    output << "<!-- " << xml_comment( aString ) << " -->" << endl;
    return output;
}
#endif

#if defined(SXD_OUTPUT)
/* -------------------------------------------------------------------- */
/* Open(Star)Office output						*/
/* -------------------------------------------------------------------- */

ostream&
NodeSXD::polygon( ostream& output, const std::vector<Point>& points ) const
{
    SXD::polygon( output, points, penColour(), fillColour() );
    return output;
}


ostream&
NodeSXD::polyline( ostream& output, const std::vector<Point>& points ) const
{
    SXD::polyline( output, points, penColour(), linestyle() );
    return output;
}


ostream&
NodeSXD::circle( ostream& output, const Point& c, const double r ) const
{
    SXD::circle( output, c, r, penColour(), fillColour() );
    return output;
}


ostream&
NodeSXD::rectangle( ostream& output ) const
{
    SXD::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

ostream&
NodeSXD::roundedRectangle( ostream& output ) const
{
    SXD::rectangle( output, origin, extent, penColour(), fillColour(), Graphic::linestyle() );
    return output;
}

ostream&
NodeSXD::text( ostream& output, const Point& c, const char * s ) const
{
    Point boxOrigin = c;
    Point boxExtent( strlen(s) / 2.4, Flags::print[FONT_SIZE].value.i );		/* A guess... width is half of height */
    boxExtent *= SXD_SCALING;
    boxOrigin.moveBy( 0, -boxExtent.y() / 2.0 );
    SXD::begin_paragraph( output, boxOrigin, boxExtent, CENTER_JUSTIFY );
    SXD::text( output, c, s, Graphic::NORMAL_FONT, Flags::print[FONT_SIZE].value.i, CENTER_JUSTIFY, penColour() );
    SXD::end_paragraph( output );
    return output;
}

ostream& 
NodeSXD::comment( ostream& output, const string& aString ) const
{
    output << "<!-- " << xml_comment( aString ) << " -->" << endl;
    return output;
}
#endif

/* -------------------------------------------------------------------- */
/* EEPIC output								*/
/* -------------------------------------------------------------------- */

ostream&
NodeTeX::polygon( ostream& output, const std::vector<Point>& points ) const
{
    TeX::polygon( output, points, penColour(), fillColour() );
    return output;
}


ostream&
NodeTeX::polyline( ostream& output, const std::vector<Point>& points ) const
{
    TeX::polyline( output, points, penColour(), linestyle() );
    return output;
}


ostream&
NodeTeX::circle( ostream& output, const Point& c, const double r ) const
{
    TeX::circle( output, c, r, penColour(), fillColour() );
    return output;
}


ostream&
NodeTeX::rectangle( ostream& output ) const
{
    TeX::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

ostream&
NodeTeX::roundedRectangle( ostream& output ) const
{
    TeX::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

ostream&
NodeTeX::text( ostream& output, const Point& c, const char * s ) const
{
    string aStr = tex_string( s );
    TeX::text( output, c, aStr, Graphic::NORMAL_FONT, Flags::print[FONT_SIZE].value.i, CENTER_JUSTIFY, penColour() );
    return output;
}

ostream& 
NodeTeX::comment( ostream& output, const string& aString ) const
{
    output << "% " << aString << endl;
    return output;
}

/*
 * Convert to safe string.
 */

static string
tex_string( const char * s )
{
    string aStr;

    for ( ; *s; ++s ) {
	switch ( *s ) {
	case '\\':
	case '$':
	case '&':
	case '_':
	    aStr += '\\';
	    break;
	case ' ':
	    aStr += '~';
	    continue;
	}
	aStr += *s;
    }
    return aStr;
}


static string
unicode_string( const char * s )
{
    string aStr;

    for ( ; *s; ++s ) {
	aStr += '\0';	/* High Byte */
	aStr += *s;	/* Low byte */
    }
    return aStr;
}



/*
 * XML doesn't like the string -- in comments. 
 */

static string
xml_comment( const string& src )
{
    string dst = src;
    for ( unsigned i = 1; i < dst.length(); ++i ) {
	if ( dst[i-1] == dst[i] && dst[i-1] == '-' ) {
	    dst[i-1] = '~';				/* BUG 588 */
	}
    }
    return dst;
}
