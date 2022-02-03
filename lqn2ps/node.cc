/* node.cc	-- Greg Franks Wed Jan 29 2003
 *
 * $Id: node.cc 15423 2022-02-03 02:10:02Z greg $
 */

#include "lqn2ps.h"
#include <stdarg.h>
#include <cstdlib>
#include <cstring>
#include "node.h"
#include "arc.h"

static std::string tex_string( const char * s );
static std::string xml_comment( const std::string& );
static std::string unicode_string( const char * s );

/*
 * Create a new node.
 */

Node *
Node::newNode( double x, double y )
{
    static const std::map<const File_Format,Node::create_func> new_node = {
	{ File_Format::EEPIC,       NodeTeX::create },
#if EMF_OUTPUT
	{ File_Format::EMF,         NodeEMF::create },
#endif
	{ File_Format::FIG,         NodeFig::create },
#if HAVE_GD_H && HAVE_LIBGD
#if HAVE_GDIMAGEGIFPTR
	{ File_Format::GIF,         NodeGD::create },
#endif
#if HAVE_LIBJPEG 
	{ File_Format::JPEG,        NodeGD::create },
#endif
#if HAVE_LIBPNG
	{ File_Format::PNG,         NodeGD::create },
#endif
#endif  /* HAVE_LIBGD */
	{ File_Format::POSTSCRIPT,  NodePostScript::create }, 
	{ File_Format::PSTEX,       NodePsTeX::create }, 		/* the graphical object         */ 
#if SVG_OUTPUT
	{ File_Format::SVG,         NodeSVG::create },
#endif
#if SXD_OUTPUT
	{ File_Format::SXD,         NodeSXD::create },
#endif
#if X11_OUTPUT
	{ File_Format::X11,         NodeX11::create },
#endif
    };

    std::map<const File_Format,Node::create_func>::const_iterator f = new_node.find( Flags::output_format() );
    if ( f != new_node.end() ) {
	return (*(f->second))( 0, 0, x, y );
    } else {
	return NodeNull::create( 0, 0, x, y );
    }
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


std::ostream& 
Node::draw_queue( std::ostream& output, const Point& aPoint, const double radius ) const
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

std::ostream&
Node::multi_server( std::ostream& output, const Point& centerBottom, const double radius ) const
{
    Arc * anArc = Arc::newArc( 3, Graphic::ArrowHead::NONE );
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


std::ostream&
Node::open_source( std::ostream& output, const Point& centerBottom, const double radius ) const
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

std::ostream&
Node::open_sink( std::ostream& output, const Point& centerTop, const double radius ) const
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

#if EMF_OUTPUT
/* -------------------------------------------------------------------- */
/* Windows Enhanced Meta File output					*/
/* -------------------------------------------------------------------- */

std::ostream&
NodeEMF::polygon( std::ostream& output, const std::vector<Point>& points ) const
{
    EMF::polygon( output, points, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeEMF::polyline( std::ostream& output, const std::vector<Point>& points ) const
{
    EMF::polyline( output, points, penColour() );
    return output;
}


std::ostream&
NodeEMF::circle( std::ostream& output, const Point& c, const double r ) const
{
    EMF::circle( output, c, r, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeEMF::rectangle( std::ostream& output ) const
{
    EMF::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

std::ostream&
NodeEMF::roundedRectangle( std::ostream& output ) const
{
    EMF::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

std::ostream&
NodeEMF::text( std::ostream& output, const Point& c, const char * s ) const
{
    std::string aStr = unicode_string( s );
    EMF::text( output, c, aStr, Graphic::Font::NORMAL, Flags::font_size(), Justification::CENTER, penColour() );
    return output;
}

std::ostream& 
NodeEMF::comment( std::ostream& output, const std::string& aString ) const
{
    /* Binary file format.  No operation. */
    return output;
}
#endif

/* -------------------------------------------------------------------- */
/* XFIG output								*/
/* -------------------------------------------------------------------- */

std::ostream&
NodeFig::polygon( std::ostream& output, const std::vector<Point>& points ) const
{
    Fig::polyline( output, points, Fig::POLYGON, penColour(), fillColour(), depth() );
    return output;
}


std::ostream&
NodeFig::polyline( std::ostream& output, const std::vector<Point>& points ) const
{
    Fig::polyline( output, points, Fig::POLYLINE, penColour(), Graphic::Colour::TRANSPARENT, depth(), linestyle() );
    return output;
}


std::ostream&
NodeFig::circle( std::ostream& output, const Point& c, const double r ) const
{
    Fig::circle( output, c, r, 3, penColour(), fillColour(), depth(), fillStyle() );
    return output;
}


std::ostream&
NodeFig::rectangle( std::ostream& output ) const
{
    Fig::rectangle( output, origin, extent, penColour(), fillColour(), depth(), linestyle() );
    return output;
}

std::ostream&
NodeFig::roundedRectangle( std::ostream& output ) const
{
    Fig::roundedRectangle( output, origin, extent, penColour(), Graphic::Colour::TRANSPARENT, depth(), linestyle() );
    return output;
}

std::ostream&
NodeFig::text( std::ostream& output, const Point& c, const char * s ) const
{
    Fig::text( output, c, s, Graphic::Font::NORMAL, Flags::font_size(), Justification::CENTER, penColour(), Fig::POSTSCRIPT );
    return output;
}


std::ostream& 
NodeFig::comment( std::ostream& output, const std::string& aString ) const
{
    output << "# " << aString << std::endl;
    return output;
}

#if HAVE_GD_H && HAVE_LIBGD
/* -------------------------------------------------------------------- */
/* GD (Jpeg, PNG, GIF ) output						*/
/* -------------------------------------------------------------------- */

std::ostream&
NodeGD::polygon( std::ostream& output, const std::vector<Point>& points ) const
{
    GD::polygon( points, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeGD::polyline( std::ostream& output, const std::vector<Point>& points ) const
{
    for ( unsigned i = 1; i < points.size(); ++i ) {    
	GD::drawline( points[i-1], points[i], penColour(), linestyle() );
    }
    return output;
}


std::ostream&
NodeGD::circle( std::ostream& output, const Point& c, const double r ) const
{
    GD::circle( c, r, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeGD::rectangle( std::ostream& output ) const
{
    GD::rectangle( origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

std::ostream&
NodeGD::roundedRectangle( std::ostream& output ) const
{
    GD::rectangle( origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

std::ostream&
NodeGD::text( std::ostream& output, const Point& c, const char * s ) const
{
    Point aPoint( c );
    gdFont * font = GD::getfont();
    aPoint.moveBy( 0, -font->h );
    GD::text( aPoint, s, Graphic::Font::NORMAL, Flags::font_size(), Justification::CENTER, penColour() );
    return output;
}


/*
 * One can't put comments into PNG/JPEG output. :-)
 */

std::ostream& 
NodeGD::comment( std::ostream& output, const std::string& aString ) const
{
    return output;
}
#endif

/* -------------------------------------------------------------------- */
/* PostScript output							*/
/* -------------------------------------------------------------------- */

std::ostream&
NodePostScript::polygon( std::ostream& output, const std::vector<Point>& points ) const
{
    PostScript::polygon( output, points, penColour(), fillColour() );
    return output;
}


std::ostream&
NodePostScript::polyline( std::ostream& output, const std::vector<Point>& points ) const
{
    PostScript::polyline( output, points, penColour(), Graphic::linestyle() );
    return output;
}


std::ostream&
NodePostScript::circle( std::ostream& output, const Point& c, const double r ) const
{
    PostScript::circle( output, c, r, penColour(), fillColour() );
    return output;
}


std::ostream&
NodePostScript::rectangle( std::ostream& output ) const
{
    PostScript::rectangle( output, origin, extent, penColour(), fillColour(), Graphic::linestyle() );
    return output;
}

std::ostream&
NodePostScript::roundedRectangle( std::ostream& output ) const
{
    PostScript::roundedRectangle( output, origin, extent, penColour(), Graphic::Colour::TRANSPARENT, Graphic::linestyle() );
    return output;
}

std::ostream&
NodePostScript::text( std::ostream& output, const Point& c, const char * s ) const
{
    PostScript::text( output, c, s, Graphic::Font::NORMAL, Flags::font_size(), Justification::CENTER, penColour() );
    return output;
}


std::ostream& 
NodePostScript::comment( std::ostream& output, const std::string& aString ) const
{
    output << "% " << aString << std::endl;
    return output;
}

std::ostream&
NodePsTeX::text( std::ostream& output, const Point& c, const char * s ) const
{
    std::string aStr = tex_string( s );
    Fig::text( output, c, aStr, Graphic::Font::NORMAL, Flags::font_size(), Justification::CENTER, penColour(), 
	       Fig::SPECIAL );
    return output;
}

#if defined(SVG_OUTPUT)
/* -------------------------------------------------------------------- */
/* Scalable Vector Grahpics Ouptut					*/
/* -------------------------------------------------------------------- */

std::ostream&
NodeSVG::polygon( std::ostream& output, const std::vector<Point>& points ) const
{
    SVG::polygon( output, points, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeSVG::polyline( std::ostream& output, const std::vector<Point>& points ) const
{
    SVG::polyline( output, points, penColour(), Graphic::linestyle() );
    return output;
}


std::ostream&
NodeSVG::circle( std::ostream& output, const Point& c, const double r ) const
{
    SVG::circle( output, c, r, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeSVG::rectangle( std::ostream& output ) const
{
    SVG::rectangle( output, origin, extent, penColour(), fillColour(), Graphic::linestyle() );
    return output;
}

std::ostream&
NodeSVG::roundedRectangle( std::ostream& output ) const
{
    SVG::rectangle( output, origin, extent, penColour(), fillColour(), Graphic::linestyle() );
    return output;
}

std::ostream&
NodeSVG::text( std::ostream& output, const Point& c, const char * s ) const
{
    SVG::text( output, c, s, Graphic::Font::NORMAL, Flags::font_size(), Justification::CENTER, penColour() );
    return output;
}

std::ostream& 
NodeSVG::comment( std::ostream& output, const std::string& aString ) const
{
    output << "<!-- " << xml_comment( aString ) << " -->" << std::endl;
    return output;
}
#endif

#if defined(SXD_OUTPUT)
/* -------------------------------------------------------------------- */
/* Open(Star)Office output						*/
/* -------------------------------------------------------------------- */

std::ostream&
NodeSXD::polygon( std::ostream& output, const std::vector<Point>& points ) const
{
    SXD::polygon( output, points, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeSXD::polyline( std::ostream& output, const std::vector<Point>& points ) const
{
    SXD::polyline( output, points, penColour(), linestyle() );
    return output;
}


std::ostream&
NodeSXD::circle( std::ostream& output, const Point& c, const double r ) const
{
    SXD::circle( output, c, r, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeSXD::rectangle( std::ostream& output ) const
{
    SXD::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

std::ostream&
NodeSXD::roundedRectangle( std::ostream& output ) const
{
    SXD::rectangle( output, origin, extent, penColour(), fillColour(), Graphic::linestyle() );
    return output;
}

std::ostream&
NodeSXD::text( std::ostream& output, const Point& c, const char * s ) const
{
    Point boxOrigin = c;
    Point boxExtent( strlen(s) / 2.4, Flags::font_size() );		/* A guess... width is half of height */
    boxExtent *= SXD_SCALING;
    boxOrigin.moveBy( 0, -boxExtent.y() / 2.0 );
    SXD::begin_paragraph( output, boxOrigin, boxExtent, Justification::CENTER );
    SXD::text( output, c, s, Graphic::Font::NORMAL, Flags::font_size(), Justification::CENTER, penColour() );
    SXD::end_paragraph( output );
    return output;
}

std::ostream& 
NodeSXD::comment( std::ostream& output, const std::string& aString ) const
{
    output << "<!-- " << xml_comment( aString ) << " -->" << std::endl;
    return output;
}
#endif

/* -------------------------------------------------------------------- */
/* EEPIC output								*/
/* -------------------------------------------------------------------- */

std::ostream&
NodeTeX::polygon( std::ostream& output, const std::vector<Point>& points ) const
{
    TeX::polygon( output, points, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeTeX::polyline( std::ostream& output, const std::vector<Point>& points ) const
{
    TeX::polyline( output, points, penColour(), linestyle() );
    return output;
}


std::ostream&
NodeTeX::circle( std::ostream& output, const Point& c, const double r ) const
{
    TeX::circle( output, c, r, penColour(), fillColour() );
    return output;
}


std::ostream&
NodeTeX::rectangle( std::ostream& output ) const
{
    TeX::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

std::ostream&
NodeTeX::roundedRectangle( std::ostream& output ) const
{
    TeX::rectangle( output, origin, extent, penColour(), fillColour(), linestyle() );
    return output;
}

std::ostream&
NodeTeX::text( std::ostream& output, const Point& c, const char * s ) const
{
    std::string aStr = tex_string( s );
    TeX::text( output, c, aStr, Graphic::Font::NORMAL, Flags::font_size(), Justification::CENTER, penColour() );
    return output;
}

std::ostream& 
NodeTeX::comment( std::ostream& output, const std::string& aString ) const
{
    output << "% " << aString << std::endl;
    return output;
}

/*
 * Convert to safe string.
 */

static std::string
tex_string( const char * s )
{
    std::string aStr;

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


static std::string
unicode_string( const char * s )
{
    std::string aStr;

    for ( ; *s; ++s ) {
	aStr += '\0';	/* High Byte */
	aStr += *s;	/* Low byte */
    }
    return aStr;
}



/*
 * XML doesn't like the std::string -- in comments. 
 */

static std::string
xml_comment( const std::string& src )
{
    std::string dst = src;
    for ( unsigned i = 1; i < dst.length(); ++i ) {
	if ( dst[i-1] == dst[i] && dst[i-1] == '-' ) {
	    dst[i-1] = '~';				/* BUG 588 */
	}
    }
    return dst;
}
