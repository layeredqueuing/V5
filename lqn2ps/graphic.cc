/* graphic.cc	-- Greg Franks Wed Feb 12 2003
 *
 * $Id: graphic.cc 15170 2021-12-07 23:33:05Z greg $
 */

#include <cassert>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <lqio/input.h>
#include "graphic.h"
#include "point.h"
#if HAVE_GD_H
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontg.h>
#endif

static const int DASH_LENGTH	= 4;
static const int DOT_GAP	= 3;

static void box_to_points( const Point& origin, const Point& extent, std::vector<Point>& points )
{
    assert( points.size() > 4 );
    points[0] = origin;
    points[1].moveTo( origin.x(), origin.y() + extent.y() );
    points[2].moveTo( origin.x() + extent.x(), origin.y() + extent.y() );
    points[3].moveTo( origin.x() + extent.x(), origin.y());
    points[4] = origin;
}

const std::string XMLString::StringManip::nullStr( "" );

std::ostream&
XMLString::xml_escape_str( std::ostream& output, const std::string& s, const std::string& )
{
    for ( std::string::const_iterator c = s.begin(); c != s.end(); ++c ) {
	switch ( *c ) {
	case '&':
	    if ( *(c+1) != '#' ) output << "&amp;"; else output << "&"; 
	    break;	    
	case '<': output << "&lt;"; break;
	case '>': output << "&gt;"; break;
	default:  output << *c; break;
	}
    }
    return output;
}

/* See http://astronomy.swin.edu.au/~pbourke/colour/colourramp/ for a good way to generate colours */
/* See http://tex.loria.fr/graph-pack/grf/gr2.htm */

Colour::colour_defn Colour::colour_value[] =
{
    {0.000, 0.000, 0.000},		/* TRANSPARENT, */
    {0.000, 0.000, 0.000},		/* DEFAULT, 	*/
    {0.000, 0.000, 0.000},		/* BLACK,   	*/
    {1.000, 1.000, 1.000},		/* WHITE,   	*/
    {0.900, 0.900, 0.900},		/* GREY_10  	*/
    {1.000, 0.000, 1.000},		/* MAGENTA, 	*/
    {0.500, 0.000, 1.000},		/* VIOLET   	*/
    {0.000, 0.000, 1.000},		/* BLUE,    	*/
    {0.000, 0.434, 1.000},		/* INDIGO,	*/
    {0.000, 1.000, 1.000},		/* CYAN,    	*/
    {0.000, 1.000, 0.500},		/* TURQUOIS	*/
    {0.000, 1.000, 0.000},		/* GREEN,   	*/
    {0.500, 1.000, 0.000},		/* SPRINGGREEN	*/
    {1.000, 1.000, 0.000},		/* YELLOW,  	*/
    {1.000, 0.500, 0.000},		/* ORANGE   	*/
    {1.000, 0.000, 0.000},		/* RED,     	*/
    {1.000, 0.840, 0.000}		/* GOLD     	*/
};


const char * Colour::colour_name[] =
{
    "White",			/* TRANSPARENT */
    "black",			/* DEFAULT_COLOUR */
    "black",			/* BLACK */
    "White",			/* WHITE */
    "Grey",			/* GREY_10 */
    "Magenta",			/* MAGENTA */
    "Violet",			/* VIOLET */
    "Blue",			/* BLUE */
    "Indigo",			/* INDIGO */
    "Cyan",			/* CYAN */
    "Turquoise",		/* TURQUOISE */
    "Green",			/* GREEN */
    "SpringGreen",		/* SPRINGGREEN - Chartreuse */
    "Yellow",			/* YELLOW */
    "Orange",			/* ORANGE */
    "Red",			/* RED */
    "Goldenrod"			/* GOLD */
};

float
Colour::tint( const float x, const float tint )
{
    assert( 0 <= x && x <= 1.0 && 0 <= tint && tint <= 1.0 );
    return x * (1.0 - tint) + tint;
}

RGBManip
Colour::rgb( float red, float green, float blue )
{
    return RGBManip( Colour::rgb_str, red, green, blue );
}

std::ostream&
Colour::rgb_str( std::ostream& output, float red, float green, float blue )
{
    output << "#";
    output.setf( std::ios::hex, std::ios::basefield );
    char old_fill = output.fill( '0' );
    output << std::setw(2) << to_byte( red )
	   << std::setw(2) << to_byte( green )
	   << std::setw(2) << to_byte( blue );
    output.fill( old_fill );
    output.setf( std::ios::dec, std::ios::basefield );
    return output;
}

bool
Graphic::intersects( const Point& p1, const Point& p2, const Point& p3, const Point& p4 )
{
    const double uan = (p4.x() - p3.x()) * (p1.y() - p3.y()) - (p4.y() - p3.y()) * (p1.x() - p3.x());
    const double ubn = (p2.x() - p1.x()) * (p1.y() - p3.y()) - (p2.y() - p1.y()) * (p1.x() - p3.x());
    const double den = (p4.y() - p3.y()) * (p2.x() - p1.x()) - (p4.x() - p3.x()) * (p2.y() - p1.y());

    if ( den == 0.0 ) {
	return  uan == ubn && uan == 0.0;
    } else {
	const double ua = uan / den;
	const double ub = ubn / den;
	return 0 <= ua && ua <= 1 && 0 <= ub && ub <= 1;
    }
}



/*
 * Return true if the p1 is within the bounding box defined by p2 and p3.
 */

bool
Graphic::intersects( const Point& p1, const Point& p2, const Point& p3 )
{
    Point origin = p2;
    Point extent = p2;
    origin.min( p3 );
    extent.max( p3 );

    return origin.x() <= p1.x() && p1.x() <= extent.x()
	&& origin.y() <= p1.y() && p1.y() <= extent.y();
}

#if defined(EMF_OUTPUT)
/* -------------------------------------------------------------------- */
/* Windows Enhanced Meta File output					*/
/* -------------------------------------------------------------------- */

long EMF::record_count = 0;
Graphic::colour_type EMF::last_pen_colour    = Graphic::BLACK;
Graphic::colour_type EMF::last_fill_colour   = Graphic::BLACK;
Graphic::colour_type EMF::last_arrow_colour  = Graphic::BLACK;
Justification EMF::last_justification        = Justification::CENTER;
Graphic::font_type EMF::last_font	     = Graphic::NORMAL_FONT;
Graphic::linestyle_type EMF::last_line_style = Graphic::SOLID;
int EMF::last_font_size			     = 0;

std::ostream&
EMF::init( std::ostream& output, const double xmax, const double ymax, const std::string& command_line )
{
    const double twips_to_mm = .01763888;
    const double twips_to_pixels = twips_to_mm * 5.0;

    std::string aDescription = description( command_line );
    const unsigned n = aDescription.size();

    record_count = 0;

    /* Force redraw */

    last_pen_colour   	= static_cast<Graphic::colour_type>(-1);
    last_fill_colour  	= static_cast<Graphic::colour_type>(-1);
    last_arrow_colour 	= static_cast<Graphic::colour_type>(-1);
    last_justification  = Justification::CENTER;
    last_line_style     = Graphic::SOLID;
    last_font		= Graphic::NORMAL_FONT;
    last_font_size	= 0;

    output << start_record( EMR_HEADER, 100+((n+1)/2)*4 ) 		/* Record Type, size (bytes) */
	   << writep(0,0)		/* rclBounds */
	   << writep(static_cast<long>(xmax/EMF_SCALING+0.5),static_cast<long>(ymax/EMF_SCALING+0.5))
	   << writep(0,0)		/* rclFrame */
	   << writep(static_cast<long>(xmax),static_cast<long>(ymax))
	   << writel(0x464D4520)	/* signature */
	   << writel(0x00010000)	/* version */
	   << writel(0)			/* nBytes   -- updated later (ick) */
	   << writel(0)			/* nRecords -- updated later (ick) */
	   << writes(EMF_HANDLE_MAX)	/* nHandles */
	   << writes(0);		/* reserved */
    if ( n ) {
	output << writel(n)		/* descSize */
	       << writel(100)		/* descOff */;
    } else {
	output << writel(0)		/* descSize */
	       << writel(0);		/* descOff */;
    }
    output << writel(0)			/* nPalEntries */
	   << writel(static_cast<long>(xmax*twips_to_pixels+0.5))	/* ref dev pixwidth */
	   << writel(static_cast<long>(ymax*twips_to_pixels+0.5))	/* ref dev pixheight */
	   << writel(static_cast<long>(xmax*twips_to_mm+0.5))		/* ref dev width (mm) */
	   << writel(static_cast<long>(ymax*twips_to_mm+0.5))		/* ref dev height (mm) */
	   << writel(0)			/* cbPixelFormat  */
	   << writel(0)			/* offPixelFormat  */
	   << writel(0)			/* bOpenGL */
	   << writestr( aDescription );

    /* header end */

    output << start_record( EMR_SETMAPMODE, 12 )		/* forcing anisotropic mode */
	   << writel( 8 );
    output << start_record( EMR_SETWINDOWEXTEX, 16 )		/* setting logical (himetric) size      */
	   << writep(static_cast<long>(xmax+0.5),static_cast<long>(ymax+0.5));
    output << start_record( EMR_SETVIEWPORTEXTEX, 16 )		/* setting device (pixel) size */
	   << writep(static_cast<long>(xmax/EMF_SCALING+0.5), static_cast<long>(ymax/EMF_SCALING+0.5) );
    output << start_record( EMR_CREATEPEN, 28 )			/* init default pen */
	   << writel( EMF_HANDLE_PEN )
	   << writel( 0 )		/* type */
	   << writel( 1 )		/* width */
	   << writel( 0 )
	   << writel( 0x000000 );	/* color */
    output << start_record( EMR_SELECTOBJECT, 12 )
	   << writel( EMF_HANDLE_PEN );
    output << start_record( EMR_SETBKMODE, 12 )			/* transparent background for text */
	   << writel( 1 );
    output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )	/* transparent brush for polygons */
	   << writel( EMF_HANDLE_BRUSH )
//	   << writel( 1 )		/* type */		// transparent
	   << writel( 0 )		/* type */
	   << writel( 0 )		/* colour */
	   << writel( 0 );		/* hatch */
    output << start_record( EMR_SELECTOBJECT, 12 )
	   << writel( EMF_HANDLE_BRUSH );
    output << setfont( Graphic::NORMAL_FONT );
    return output;
}



/*
 * Generate the description string
 */

std::string
EMF::description( const std::string& command_line )
{
    std::string aString = LQIO::io_vars.lq_toolname;
    aString += " ";
    aString += VERSION;
    aString += '\0';		/* Record separator */
    aString += command_line;
    aString += '\0';
    aString += '\0';
    return aString;
}


/*
 * Generate the description string
 */

std::ostream&
EMF::writestr_str( std::ostream& output, const std::string& s )
{
    const unsigned int len_1 = s.size();
    const unsigned int len_2 = ((len_1+1) / 2) * 2;	/* Pad to word */

    for ( unsigned i = 0; i < len_1; ++i ) {
	output << s[i] << '\0';				/* Little endian */
    }
    /* Pad to long-word */
    for ( unsigned i = len_1; i < len_2; ++i ) {
	output << '\0' << '\0';
    }
    return output;
}


/*
 * Update header size and record count fields.
 */

std::ostream&
EMF::terminate( std::ostream& output )
{
    /* writing end of metafile */
    output << start_record( EMR_SELECTOBJECT, 12 )
	   << writel( EMF_STOCK_OBJECT_DEFAULT_FONT );
    output << start_record( EMR_DELETEOBJECT, 12 )
	   << writel( EMF_HANDLE_FONT );
    output << start_record( EMR_SELECTOBJECT, 12 )
	   << writel( EMF_STOCK_OBJECT_BLACK_PEN );
    output << start_record( EMR_DELETEOBJECT, 12 )
	   << writel( EMF_HANDLE_PEN );
    output << start_record( EMR_SELECTOBJECT, 12 )
	   << writel( EMF_STOCK_OBJECT_WHITE_BRUSH );
    output << start_record( EMR_DELETEOBJECT, 12 )
	   << writel( EMF_HANDLE_BRUSH );
    output << start_record( EMR_EOF, 20 )
	   << writel( 0 )
	   << writel( 0x10 )
	   << writel( 20 );

    /* Update the header */
    std::streampos pos = output.tellp();
    output.seekp( 48 );
    output << writel( pos )
	   << writel( record_count );
    return output;
}

/*
 */

std::ostream&
EMF::polyline( std::ostream& output, const std::vector<Point>& points,
	       Graphic::colour_type pen_colour, Graphic::linestyle_type line_style,
	       Graphic::arrowhead_type arrowhead, double scale ) const
{
    const unsigned int n_points = points.size();
    if ( n_points > 1 ) {
	output << setcolour( pen_colour );

	switch( line_style ) {
	case Graphic::DASHED:
	case Graphic::DOTTED:
	case Graphic::DASHED_DOTTED:
	    draw_dashed_line( output, line_style, points );
	    break;
	default:
	    draw_line( output, EMR_POLYLINE, points );
	    break;
	}

	/* Now draw the arrowhead */

	switch ( arrowhead ) {
	case Graphic::CLOSED_ARROW:
	    arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, pen_colour );
	    break;
	case Graphic::OPEN_ARROW:
	    arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, Graphic::WHITE );
	    break;
	}
    }

    return output;
}


std::ostream&
EMF::polygon( std::ostream& output, const std::vector<Point>& points,
	      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    output << setcolour( pen_colour ) << setfill( fill_colour );
    return draw_line( output, EMR_POLYGON, points );
}



/*
 * Draw a line (possibly with fill)
 */

std::ostream&
EMF::draw_line( std::ostream& output, extended_meta_record emr_type, const std::vector<Point>& points ) const
{
    const unsigned n_points = points.size();
    if ( n_points == 2 ) {
	output << moveto( points[0] ) << lineto( points[1] );
    } else {
	Point origin = points[0];
	Point extent = points[0];
	for ( unsigned i = 1; i < n_points; ++i ) {
	    origin.min( points[i] );
	    extent.max( points[i] );
	}
	output << start_record( emr_type, 12+(n_points+2)*8 )
	       << point( origin ) << point( extent ) << writel( n_points );
	for ( unsigned i = 0; i < n_points; ++i ) {
	    output << point( points[i] );
	}
    }
    return output;
}



std::ostream&
EMF::draw_dashed_line( std::ostream& output, Graphic::linestyle_type line_style, const std::vector<Point>& points ) const
{
    const unsigned n_points = points.size();
    double delta[2];
    double residual = 0.0;		/* Nothing left over */
    int stroke = 1;			/* Start with stroke */

    delta[0] = EMF_SCALING * 2;
    delta[1] = EMF_SCALING * 2;

    switch( line_style ) {
    case Graphic::DASHED:
	delta[1] = DOT_GAP * EMF_SCALING;
	break;
    case Graphic::DOTTED:
    case Graphic::DASHED_DOTTED:
	delta[0] = DASH_LENGTH * EMF_SCALING;
	break;
    }

    output << moveto( points[0] );
    Point deltaPoint[2];
    for ( unsigned i = 1; i < n_points; ++i ) {
	Point aPoint = points[i-1];
	const double theta = atan2( points[i].y() - points[i-1].y(), points[i].x() - points[i-1].x() );
	deltaPoint[0].moveTo( delta[0] * cos( theta ), delta[0] * sin( theta ) );
	deltaPoint[1].moveTo( delta[1] * cos( theta ), delta[1] * sin( theta ) );

	for ( ;; ) {
	    if ( residual == 0.0 ) {
		aPoint.moveBy( deltaPoint[stroke] );
	    } else {
		aPoint.moveBy( residual * cos( theta ), residual * sin( theta ) );
	    }
	    if ( stroke ) {
		if ( Graphic::intersects( aPoint, points[i-1], points[i] ) ) {
		    output << lineto( aPoint );
		} else {
		    output << lineto( points[i] );
		    break;
		}
	    } else {
		if ( Graphic::intersects( aPoint, points[i-1], points[i] ) ) {
		    output << moveto( aPoint );
		} else {
		    output << moveto( points[i] );
		    break;
		}
	    }
	    stroke = ~stroke & 0x01;
	    residual = 0.0;
	}

	/* Compute residual */
	aPoint -= points[i];
	residual = sqrt( square( aPoint.x() ) + square( aPoint.y() ) );
    }
    return output;
}



std::ostream&
EMF::circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour,
	     Graphic::colour_type fill_colour ) const
{
    Point origin( c );
    Point extent( c );
    origin.moveBy( -r, -r );
    extent.moveBy(  r,  r );

    output << setcolour( pen_colour ) << setfill( fill_colour );

    output << moveto( c );
    output << start_record( EMR_ELLIPSE, 24 )
	   << point( origin ) 	/* top left box */
	   << point( extent );	/* bottom right box */
    return output;
}

std::ostream&
EMF::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
		Graphic::colour_type fill_colour, Graphic::linestyle_type line_style ) const
{
    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( origin, extent, points );

    output << setcolour( pen_colour ) << setfill( fill_colour );
    switch( line_style ) {
    case Graphic::DASHED:
    case Graphic::DOTTED:
    case Graphic::DASHED_DOTTED:
	draw_dashed_line( output, line_style, points );
	break;
    default:
	draw_line( output, EMR_POLYGON, points );
	break;
    }
    return output;
}


/*
 * String should be UNICode.
 */


double
EMF::text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
	   Justification justification, Graphic::colour_type colour, unsigned ) const
{
    Point origin(0,0);
    Point extent(0,0);
    const unsigned int len_1 = s.size();    /* Pad to long-word */
    if ( len_1 % 2 != 0 ) {
	std::ostringstream msg; 
	msg << "EMF::text - Bogus string! Length is " << len_1 << ", string is ";
	for ( unsigned int i = 0; i < len_1; i += 1  ) {
	    msg.fill( '0' );
	    if ( isprint( s[i] ) ) {
		msg << s[i];
	    } else {
		msg << "0x" << std::setw(2) << static_cast<int>(s[i]);
	    }
	    msg << " ";
	}
	msg << ".";
	throw std::runtime_error( msg.str() );
    }
    const unsigned int len_2 = ( len_1 & 0x03 ) == 0 ? len_1 : (len_1 & 0xfffc) + 4;

    output << start_record( EMR_SETTEXTCOLOR, 12 ) << rgb( colour_value[colour].red, colour_value[colour].green, colour_value[colour].blue );
    output << setfont( font ) << justify( justification );
    output << start_record( EMR_EXTTEXTOUTW, 76 + len_2 )			/*  0 */
	   << point( origin ) 		/* Bounding -- never used */		/*  8 */
	   << point( extent )							/* 16 */
	   << writel( 1 )							/* 24 */
	   << writef( EMF_SCALING )	/* X Scale */				/* 28 */
	   << writef( EMF_SCALING )	/* Y Scale */				/* 32 */
	   << point( c )							/* 40 */
	   << writel( (len_1+1)/2 )						/* 44 */
	   << writel( 76 )		/* Offset to text */			/* 48 */
	   << writel( 0 )		/* No options */			/* 52 */
	   << point( origin ) 		/* Rectangle clipping -- not used */	/* 56 */
	   << point( extent )							/* 64 */
	   << writel( 0 );		/* Offset to DX */			/* 72 */
    for ( unsigned int i = 0; i < len_1; i += 2 ) {				/* 76 */
	output << s[i+1] << s[i];		/* Little endian */
    }
    for ( unsigned int i = len_1; i < len_2; ++i ) {
	output << '\0';
    }
    return fontsize * EMF_SCALING;
}



std::ostream&
EMF::arrowHead( std::ostream& output, const Point& head, const Point& tail, double scaling,
		Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    if ( last_arrow_colour != fill_colour ) {
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_STOCK_OBJECT_WHITE_BRUSH );
	output << start_record( EMR_DELETEOBJECT, 12 )
	       << writel( EMF_HANDLE_BRUSH );
	output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )
	       << writel( EMF_HANDLE_BRUSH )
	       << writel( 0 )		/* type */
	       << rgb( colour_value[fill_colour].red,
		       colour_value[fill_colour].green,
		       colour_value[fill_colour].blue )
	       << writel( 0 );		/* hatch */
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_HANDLE_BRUSH );
       	last_arrow_colour = fill_colour;
    }
    last_fill_colour = static_cast<Graphic::colour_type>(-1);		/* Force redraw */

    const double theta = atan2( tail.y() - head.y(), tail.x() - head.x() );
    const unsigned n_points = 4;

    std::vector<Point> arrow(n_points);
    arrow[0].moveTo( -6.0, -1.5 );
    arrow[1].moveTo( 0.0 ,  0.0 );
    arrow[2].moveTo( -6.0,  1.5 );
    arrow[3].moveTo( -5.0,  0.0 );
    Point origin( 0, 0 );

    for ( unsigned i = 0; i < n_points; ++i ) {
	arrow[i].scaleBy( scaling, scaling ).rotate( origin, theta ).moveBy( tail );
    }

    return draw_line( output, EMR_POLYGON, arrow );
}


/*
 * Print out a point in EMF Format: x y
 */

std::ostream&
EMF::lineto_str( std::ostream& output, const Point& aPoint )
{
    output << start_record( EMR_LINETO, 16 )
	   << point( aPoint );
    return output;
}


/*
 * Print out a point in EMF Format: x y
 */

std::ostream&
EMF::moveto_str( std::ostream& output, const Point& aPoint )
{
    output << start_record( EMR_MOVETOEX, 16 )
	   << point( aPoint );
    return output;
}


/*
 * Print out a point in EMF Format: x y
 */

std::ostream&
EMF::point_str( std::ostream& output, const Point& aPoint )
{
    output << writel( static_cast<long>(aPoint.x()) )
	   << writel( static_cast<long>(aPoint.y()) );
    return output;
}


std::ostream&
EMF::rgb_str( std::ostream& output, float red, float green, float blue )
{
    output << static_cast<unsigned char>(to_byte(red))
	   << static_cast<unsigned char>(to_byte(green))
	   << static_cast<unsigned char>(to_byte(blue))
	   << static_cast<unsigned char>(0x00);
    return output;
}


std::ostream&
EMF::setcolour_str( std::ostream& output, const Graphic::colour_type pen_colour )
{
    if ( pen_colour != last_pen_colour ) {
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_STOCK_OBJECT_BLACK_PEN );
	output << start_record( EMR_DELETEOBJECT, 12 )
	       << writel( EMF_HANDLE_PEN );
	output << start_record( EMR_CREATEPEN, 28 )			/* init default pen */
	       << writel( EMF_HANDLE_PEN )
	       << writel( PS_SOLID )
	       << writel( static_cast<unsigned long>(EMF_SCALING*0.5) )	/* width (1/2 point) */
	       << writel( 0 )
	       << rgb( colour_value[pen_colour].red, colour_value[pen_colour].green, colour_value[pen_colour].blue );
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_HANDLE_PEN );

	last_pen_colour = pen_colour;
    }
    return output;
}


/*
 * Tint all colours except black and white.   Default fill colour is white.  Default Pen colour is black.
 */

std::ostream&
EMF::setfill_str( std::ostream& output, const Graphic::colour_type colour )
{
    if ( colour != last_fill_colour ) {
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_STOCK_OBJECT_WHITE_BRUSH );
	output << start_record( EMR_DELETEOBJECT, 12 )
	       << writel( EMF_HANDLE_BRUSH );
	switch ( colour ) {
	case Graphic::TRANSPARENT:
	    output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )	/* transparent brush for polygons */
		   << writel( EMF_HANDLE_BRUSH )
		   << writel( 1 )		/* type */
		   << writel( 0 )
		   << writel( 0 );		/* hatch */
	    break;

	case Graphic::DEFAULT_COLOUR:
	case Graphic::WHITE:
	    output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )
		   << writel( EMF_HANDLE_BRUSH )
		   << writel( 0 )		/* type */
		   << writel( 0x00ffffff )
		   << writel( 0 );		/* hatch */
	    break;

	case Graphic::BLACK:
	    output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )
		   << writel( EMF_HANDLE_BRUSH )
		   << writel( 0 )		/* type */
		   << writel( 0x00000000 )
		   << writel( 0 );		/* hatch */
	    break;

	default:
	    output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )
		   << writel( EMF_HANDLE_BRUSH )
		   << writel( 0 )		/* type */
		   << rgb( tint( colour_value[colour].red, 0.9 ),
			   tint( colour_value[colour].green, 0.9 ),
			   tint( colour_value[colour].blue, 0.9 ) )
		   << writel( 0 );		/* hatch */
	    break;
	}

	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_HANDLE_BRUSH );

	last_fill_colour = colour;
    }

    last_arrow_colour = static_cast<Graphic::colour_type>(-1);		/* Force redraw */
    return output;
}

std::ostream&
EMF::justify_str( std::ostream& output, const Justification justification )
{
    int align = 8;
    switch( justification ) {
    case Justification::CENTER:  align |= 6; break;
    case Justification::RIGHT:   align |= 2; break;
    case Justification::LEFT:    align |= 0; break;
    }
    output << start_record( EMR_SETTEXTALIGN, 12 )
	   << writel( align );
    return output;
}

RGBManip
EMF::rgb( float red, float green, float blue )
{
    return RGBManip( EMF::rgb_str, red, green, blue );
}



std::ostream&
EMF::setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize )
{
    if ( aFont != last_font || fontSize != last_font_size ) {
	const double EMF_PT2HM = 35.28/2;
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_STOCK_OBJECT_DEFAULT_FONT );
	output << start_record( EMR_DELETEOBJECT, 12 )
	       << writel( EMF_HANDLE_FONT );
	output << start_record( EMR_EXTCREATEFONTINDIRECTW, 104 )		/* 0  */
	       << writel( EMF_HANDLE_FONT )					/* 8  */
	       << writel( static_cast<long>(fontSize * EMF_PT2HM) )		/* 12 */
	       << writel( 0 )	/* width */	 				/* 16 */
	       << writel( 0 )	/* escapement */ 				/* 20 */
	       << writel( 0 )	/* orientation */				/* 24 */
	       << writel( 400 )	/* weight */					/* 28 */
	       << static_cast<char>(aFont==Graphic::OBLIQUE_FONT) /* italic */  /* 32 */
	       << static_cast<char>(0)	/* underline */
	       << static_cast<char>(0)	/* strikeout */
	       << static_cast<char>(1)	/* charset */
	       << static_cast<char>(0)	/* out precision */   			/* 36 */
	       << static_cast<char>(0)	/* clip precision */
	       << static_cast<char>(0)	/* quality */
	       << static_cast<char>(0);	/* pitch and family */
	for ( unsigned i = 0; i < 32; i++ ) {	/* face name (max 32) */	/* 40 */
	    output << '\0' << '\0';		/* UNICode */
	}
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_HANDLE_FONT );

	last_font = aFont;
	last_font_size = fontSize;
    }
    return output;
}


std::ostream&
EMF::start_record_str( std::ostream& output, const int record_type, const int size )
{
    output << writel( record_type ) << writel( size );
    record_count += 1;
    return output;
}

/*
 * Output a long in Big Endian order (for X86)
 */

std::ostream&
EMF::writef_str( std::ostream& output, const double aDouble )
{
    union {
	long l;
	float f;
    } u;
    u.f = static_cast<float>(aDouble);
    return writel_str( output, u.l );
}

/*
 * Output a long in Little Endian order (for X86)
 */

std::ostream&
EMF::writel_str( std::ostream& output, const unsigned long aLong )
{
    output << static_cast<unsigned char>((aLong) & 0xff)
	   << static_cast<unsigned char>((aLong >> 8) & 0xff)
	   << static_cast<unsigned char>((aLong >> 16) & 0xff)
	   << static_cast<unsigned char>((aLong >> 24) & 0xff);
    return output;
}

std::ostream&
EMF::writep_str( std::ostream& output, const int x, const int y )
{
    output << writel( x ) << writel( y );
    return output;
}

/*
 * Output a short in Little Endian order (for X86)
 */

std::ostream&
EMF::writes_str( std::ostream& output, const unsigned short aShort )
{
    output << static_cast<unsigned char>((aShort) & 0xff)
	   << static_cast<unsigned char>((aShort >> 8) & 0xff);
    return output;
}

Colour::ColourManip
EMF::setfill( const Graphic::colour_type aColour )
{
    return Colour::ColourManip( EMF::setfill_str, aColour );
}

Colour::JustificationManip
EMF::justify( const Justification justification )
{
    return Colour::JustificationManip( justify_str, justification );
}

PointManip
EMF::lineto( const Point& aPoint )
{
    return PointManip( lineto_str, aPoint );
}


PointManip
EMF::moveto( const Point& aPoint )
{
    return PointManip( moveto_str, aPoint );
}


PointManip
EMF::point( const Point& aPoint )
{
    return PointManip( point_str, aPoint );
}


Colour::ColourManip
EMF::setcolour( const Graphic::colour_type aColour )
{
    return Colour::ColourManip( EMF::setcolour_str, aColour );
}

Colour::FontManip
EMF::setfont( const Graphic::font_type aFont )
{
    return Colour::FontManip( setfont_str, aFont, Flags::print[FONT_SIZE].opts.value.i );
}

DoubleManip
EMF::writef( const double aDouble )
{
    return DoubleManip( writef_str, aDouble );
}

LongManip
EMF::writel( const unsigned long aLong )
{
    return LongManip( writel_str, aLong );
}

Integer2Manip
EMF::writep( const int x, const int y )
{
    return Integer2Manip( writep_str, x, y );
}

ShortManip
EMF::writes( const unsigned short aShort )
{
    return ShortManip( writes_str, aShort );
}

StringManip2
EMF::writestr( const std::string& aString )
{
    return StringManip2( writestr_str, aString );
}

Integer2Manip
EMF::start_record( const int rec, const int size )
{
    return Integer2Manip( start_record_str, rec, size );
}

#endif

/* -------------------------------------------------------------------- */
/* XFIG output								*/
/* -------------------------------------------------------------------- */

int Fig::colour_index[] =
{
    -1,		/* TRANSPARENT */
    -1,		/* DEFAULT, */
    0,		/* BLACK,   */
    7,		/* WHITE,   */
    7,		/* GREY_10  */
    5,		/* MAGENTA, */
    32,
    1,		/* BLUE,    */
    33,
    3,		/* CYAN,    */
    34,
    2,		/* GREEN,   */
    35,
    6,		/* YELLOW,  */
    36,		/* ORANGE,  */
    4,		/* RED,     */
    37,		/* GOLD     */
};

int Fig::linestyle_value[] =
{
    0,		/* DEFAULT_LINESTYLE */
    0,		/* SOLID */
    1,		/* DASHED */
    2,		/* DOTTED */
    4		/* DASHED_DOTTED */
};

Fig::arrowhead_defn Fig::arrowhead_value[] =
{
    {0, 0},	/* NO_ARROW */
    {2, 1},	/* CLOSED_ARROW */
    {2, 0}	/* OPEN_ARROW */
};

int Fig::postscript_font_value[] =
{
    -1,		/* Default */
#if 1
    0,		/* Times-Roman */
    1,		/* Times-Italic */
    2,		/* Times-Bold */
#else
    20,		/* Helvetica-Narrow */
    21,		/* Helvetica-Narrow Oblique */
    22,		/* Helvetica-Narrow Bold */
#endif
    32		/* Symbol */
};

int Fig::tex_font_value[] =
{
    0,		/* Default */
    1,		/* Times-Roman */
    3,		/* Times-Italic */
    2,		/* Times-Bold */
    0		/* Symbol */
};

int Fig::fill_value[] = 
{
    -1, 	/* no fill */
    38,		/* Default (tint) */
    0,		/* Zero */
    18,		/* 90% */
    20,		/* 100% */
    38		/* Tint */
};

/*
 * Add any extra colours to the palette.
 */

std::ostream&
Fig::initColours( std::ostream& output )
{
    for ( int i = 3; i <= Graphic::GOLD; ++i ) {
	if ( colour_index[i] >= 32 ) {
	    output << "0 " << colour_index[i] << " " << rgb( colour_value[i].red, colour_value[i].green, colour_value[i].blue ) << std::endl;
	}
    }
    return output;
}



std::ostream&
Fig::init( std::ostream& output, int object_code, int sub_type,
	   Graphic::colour_type pen_colour, Graphic::colour_type fill_colour,
	   Graphic::linestyle_type line_style, Graphic::fill_type fill_style, int depth ) const
{
    output << object_code 				// int	object_code	(always 2)
	   << ' ' << sub_type 				// int	sub_type	(3: polygon)
	   << ' ' << linestyle_value[line_style]	// int	line_style	(enumeration type)
	   << ' ';

    /* Pen */
    if ( pen_colour == Graphic::TRANSPARENT ) {
	output << "0";					// int	thickness	(1/80 inch)
    } else {
	output << "1";					// int	thickness	(1/80 inch)
    }
    output << ' ' << colour_index[pen_colour];		// int	pen_color	(enumeration type, pen color)

    /* Fill colour */
    if ( fill_colour == Graphic::DEFAULT_COLOUR ) {
	output << ' ' << colour_index[Graphic::WHITE]; 	// int	fill_color	(enumeration type, fill color)
    } else {
	output << ' ' << colour_index[fill_colour]; 	// int	fill_color	(enumeration type, fill color)
    }
    output << ' ' << depth				// int	depth		(enumeration type)
	   << " 0 ";					// int	pen_style	(pen style, not used)

    /* Area fill */
    if ( fill_style == Graphic::DEFAULT_FILL ) {
	switch ( fill_colour ) {			// int	area_fill	(enumeration type, -1 = no fill)
	case Graphic::TRANSPARENT:
	    output << fill_value[Graphic::NO_FILL];	// No fill.
	    break;
	case Graphic::DEFAULT_COLOUR:
	case Graphic::BLACK:
	    output << fill_value[Graphic::FILL_SOLID];
	    break;
	case Graphic::GREY_10:
	    output << fill_value[Graphic::FILL_90];
	    break;
	default:
	    output << fill_value[Graphic::DEFAULT_FILL];				// TINT.
	    break;
	}
    } else {
	output << fill_value[fill_style];
    }

    switch ( line_style ) {
    case Graphic::DASHED:
	output << " " << static_cast<float>(DASH_LENGTH); // float style_val	(dash length/dot gap 1/80 inch)
	break;
    case Graphic::DASHED_DOTTED:
    case Graphic::DOTTED:
	output << " " << static_cast<float>(DOT_GAP);	// float style_val	(dash length/dot gap 1/80 inch)
	break;
    default:
	output << " 0.0000";				// float style_val	(dash length/dot gap 1/80 inch)
	break;
    }

    return output;
}


std::ostream&
Fig::startCompound( std::ostream& output, const Point & top_left, const Point & extent ) const
{
    Point bottom_right( top_left );
    bottom_right += extent;
    output << "6 "
	   << moveto( top_left )
	   << moveto( bottom_right )
	   << std::endl;
    return output;
}

std::ostream&
Fig::endCompound( std::ostream& output ) const
{
    output << "-6" << std::endl;
    return output;
}

std::ostream&
Fig::polyline( std::ostream& output, const std::vector<Point>& points,
	       int sub_type, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth,
	       Graphic::linestyle_type line_style, Graphic::arrowhead_type arrowhead, double scaling ) const
{
    const size_t n_points = points.size();
    init( output, 2, sub_type, pen_colour, fill_colour, line_style, Graphic::DEFAULT_FILL, depth );
    output << " 0"					// int	join_style	(enumeration type)
	   << " 0";					// int	cap_style	(enumeration type, only used for POLYLINE)

    if ( sub_type == ARC_BOX ) {
	output << " 12";				// int	radius		(1/80 inch, radius of arc-boxes)
    } else {
	output << " -1";
    }

    switch ( arrowhead ) {
    case Graphic::CLOSED_ARROW:
    case Graphic::OPEN_ARROW:
	output << " 1"	// int	forward_arrow		(0: off, 1: on)
	       << " 0 "	// int	backward_arrow		(0: off, 1: on)
	       << (sub_type == POLYGON ? n_points + 1 : n_points )
	       << std::endl;
	arrowHead( output, arrowhead, scaling );
	break;
    case Graphic::NO_ARROW:
	output << " 0"
	       << " 0 "
	       << (sub_type == POLYGON ? n_points + 1 : n_points )
	       << std::endl;
	break;
    }

    output << '\t';
    for ( unsigned i = 0; i < n_points; ++i ) {
	output << moveto( points[i] );
    }
    if ( sub_type == POLYGON ) {
	output << moveto( points[0] );
    }
    output << std::endl;
    return output;
}

std::ostream&
Fig::circle( std::ostream& output, const Point& c, const double r,
	     int sub_type, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth, Graphic::fill_type fill_style ) const
{
    Point radius( r, r );
    Point top( c );
    top.moveBy( 0, r );
    init( output, 1, sub_type, pen_colour, fill_colour, Graphic::DEFAULT_LINESTYLE, fill_style, depth );
    output << " 1"	// int	direction
	   << " 0 "	// float angle
	   << moveto( c )
	   << moveto( radius )
	   << moveto( top )
	   << moveto( top )
	   << std::endl;
    return output;
}


std::ostream&
Fig::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
		Graphic::colour_type fill_colour, int depth, Graphic::linestyle_type line_style ) const
{
    std::vector<Point> points(5);
    box_to_points( origin, extent, points );
    return polyline( output, points, BOX, pen_colour, fill_colour, depth, line_style );
}


std::ostream&
Fig::roundedRectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
		       Graphic::colour_type fill_colour, int depth, Graphic::linestyle_type line_style ) const
{
    std::vector<Point> points(5);
    box_to_points( origin, extent, points );
    return polyline( output, points, ARC_BOX, pen_colour, fill_colour, depth, line_style );
}


double
Fig::text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
	   Justification justification, Graphic::colour_type colour, unsigned flags ) const
{
    int sub_type = 1;
    switch ( justification ) {
    case Justification::LEFT:	 sub_type = 0; break;
    case Justification::RIGHT:	 sub_type = 2; break;
    default:  		 sub_type = 1; break;	/* Center */
    }
    output << "4 "		//int  	object 	     (always 4)
	   << sub_type 		//int	sub_type     (1: Center justified)
	   << " " << colour_index[colour]	//int	color	     (enumeration type)
	   << " 5"		//int	depth	     (enumeration type)
	   << " 0 ";		//int	pen_style    (enumeration , not used)
    if ( flags & POSTSCRIPT ) {	//int	font 	     (enumeration type)
	   output << postscript_font_value[font];
    } else {
	   output << tex_font_value[font];
    }
    output << " " << fontsize 	//float	font_size    (font size in points)
	   << " 0.0000 "	//float	angle	     (radians, the angle of the text)
	   << flags		//int	font_flags   (bit vector)
	   << " " << static_cast<unsigned int>(fontsize * FIG_SCALING + 0.5)		//float	height	     (Fig units - ignored)
	   << " " << static_cast<unsigned int>(s.length() * fontsize * FIG_SCALING + 0.5)	//float	length	     (Fig units - ignored)
	   << " " << moveto( c )
	   << s
	   << "\\001" << std::endl;

    return fontsize * FIG_SCALING;
}


std::ostream&
Fig::arrowHead( std::ostream& output, Graphic::arrowhead_type style, const double scale )
{
    output << '\t' << arrowhead_value[style].arrow_type	// int 	arrow_type	(enumeration type)
	   << ' '  << arrowhead_value[style].arrow_style// int	arrow_style	(enumeration type)
	   << " 1.00"					// float arrow_thickness(1/80 inch)
	   << " " << std::setprecision(2) << 3.0 * scale	// float arrow_width	(Fig units)
	   << " " << std::setprecision(2) << 6.0 * scale	// float arrow_height	(Fig units)
	   << std::endl;
    return output;
}


std::ostream&
Fig::clearBackground( std::ostream& output, const Point& anOrigin, const Point& anExtent, const Graphic::colour_type background_colour ) const
{
    if ( background_colour == Graphic::TRANSPARENT ) return output;

    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( anOrigin, anExtent, points );
    polyline( output, points, Fig::BOX,
	      Graphic::TRANSPARENT,		/* PEN */
	      background_colour,		/* FILL */
	      6 );
    return output;
}


/*
 * Print out a point in Fig Format: x y
 * Fig wants integers.
 */

std::ostream&
Fig::point( std::ostream& output, const Point& aPoint )
{
    output << static_cast<int>(aPoint.x() + 0.5) << ' '
	   << static_cast<int>(aPoint.y() + 0.5) << ' ';
    return output;
}

PointManip
Fig::moveto( const Point& aPoint )
{
    return PointManip( point, aPoint );
}

#if HAVE_GD_H && HAVE_LIBGD
/* -------------------------------------------------------------------- */
/* GD (Jpeg, PNG, GIF ) output						*/
/* -------------------------------------------------------------------- */

std::vector<int> GD::pen_value(sizeof(colour_value)/sizeof(colour_defn));
std::vector<int> GD::fill_value(sizeof(colour_value)/sizeof(colour_defn));

/* See http://fontconfig.org/fontconfig-user.html */
const char * GD::font_value[] =
{
    "times-12",			/* Default */
    "times-12",			/* Times-Roman */
    "times-12:italic",		/* Italic */
    "times-12:bold",		/* Bold */
    "times-12"			/* Symbol */
//    "symbol"
};


gdImagePtr GD::im = 0;
bool GD::haveTTF = false;


bool
operator==( const gdPoint& p1, const gdPoint& p2 )
{
    return p1.x == p2.x && p1.y == p2.y;
}

gdPoint
operator-( const gdPoint& p1, const gdPoint& p2 )
{
    gdPoint diff;
    diff.x = p1.x - p2.x;
    diff.y = p1.y - p2.y;
    return diff;
}

gdPoint
operator+( const gdPoint& p1, const gdPoint& p2 )
{
    gdPoint sum;
    sum.x = p1.x + p2.x;
    sum.y = p1.y + p2.y;
    return sum;
}

/*
 * Initialize the graphics library and allocate colours.
 */

void
GD::create( int x, int y )
{
    if ( im ) abort();		/* ONLY ONE can exist at a time. */
    im = gdImageCreate( x+1, y+1 );

    pen_value[Graphic::TRANSPARENT] = gdTransparent;
    fill_value[Graphic::TRANSPARENT] = gdTransparent;

    pen_value[Graphic::WHITE] = gdImageColorAllocate( im,
						      to_byte( colour_value[Graphic::WHITE].red ),
						      to_byte( colour_value[Graphic::WHITE].green ),
						      to_byte( colour_value[Graphic::WHITE].blue) );
    fill_value[Graphic::WHITE] = pen_value[Graphic::WHITE];
    pen_value[Graphic::BLACK] = gdImageColorAllocate( im,
						      to_byte( colour_value[Graphic::BLACK].red ),
						      to_byte( colour_value[Graphic::BLACK].green ),
						      to_byte( colour_value[Graphic::BLACK].blue) );
    fill_value[Graphic::BLACK] = pen_value[Graphic::BLACK];
    pen_value[Graphic::DEFAULT_COLOUR] = pen_value[Graphic::BLACK];
    fill_value[Graphic::DEFAULT_COLOUR] = pen_value[Graphic::WHITE];

    pen_value[Graphic::GREY_10] = gdImageColorAllocate( im,
						      to_byte( colour_value[Graphic::GREY_10].red ),
						      to_byte( colour_value[Graphic::GREY_10].green ),
						      to_byte( colour_value[Graphic::GREY_10].blue) );
    fill_value[Graphic::GREY_10] = pen_value[Graphic::GREY_10];

    for ( unsigned i = (unsigned)Graphic::MAGENTA; i <= (unsigned)Graphic::GOLD; ++i ) {
	pen_value.at(i) = gdImageColorAllocate( im,
						to_byte( colour_value[i].red ),
						to_byte( colour_value[i].green ),
						to_byte( colour_value[i].blue ) );
	fill_value.at(i) = gdImageColorAllocate( im,
						 to_byte( tint( colour_value[i].red, 0.9 ) ),
						 to_byte( tint( colour_value[i].green, 0.9 ) ),
						 to_byte( tint( colour_value[i].blue, 0.9 ) ) );
    }
}


void
GD::testForTTF()
{
#if HAVE_GDFTUSEFONTCONFIG
    haveTTF = (gdFTUseFontConfig(1) != 0);
#else
    haveTTF = false;
#endif
}



void
GD::destroy()
{
    gdImageDestroy( im );
    im = 0;
}

#if HAVE_GDIMAGEGIFPTR
std::ostream&
GD::outputGIF( std::ostream& output )
{
    int size;
    unsigned char * data = static_cast<unsigned char *>(gdImageGifPtr(GD::im, &size ));

    for ( int i = 0; i < size; ++i ) {
	output << data[i];
    }

    gdFree( data );
    return output;
}
#endif

#if HAVE_LIBJPEG
std::ostream&
GD::outputJPG( std::ostream& output )
{
    int size;
    unsigned char * data = static_cast<unsigned char *>(gdImageJpegPtr(GD::im, &size, -1 ));

    for ( int i = 0; i < size; ++i ) {
	output << data[i];
    }

    gdFree( data );
    return output;
}
#endif

#if HAVE_LIBPNG
std::ostream&
GD::outputPNG( std::ostream& output )
{
    int size;
    unsigned char * data = static_cast<unsigned char *>(gdImagePngPtr(GD::im, &size ));

    for ( int i = 0; i < size; ++i ) {
	output << data[i];
    }

    gdFree( data );
    return output;
}
#endif

GD const&
GD::polygon( const std::vector<Point>& points, Graphic::colour_type pen_colour,
	     Graphic::colour_type fill_colour ) const
{
    const unsigned int n_points = points.size();
    gdPoint * gd_points = new gdPoint[n_points];
    for ( unsigned i = 0; i < n_points; ++i ) {
	gd_points[i] = moveto( points[i] );
    }

    gdImageFilledPolygon( im, gd_points, n_points, fill_value.at(fill_colour) );
#if HAVE_GDIMAGESETANTIALIASED
    gdImageSetAntiAliased( im, pen_value.at(pen_colour) );
    gdImagePolygon( im, gd_points, n_points, gdAntiAliased );
#else
    gdImagePolygon( im, gd_points, n_points, pen_value.at(pen_colour) );
#endif
    delete [] gd_points;
    return *this;
}



GD const &
GD::drawline( const Point &p1, const Point &p2, Graphic::colour_type pen_colour, Graphic::linestyle_type line_style ) const
{
    static int styleDotted[2], styleDashed[6], styleDashedDotted[9];		/* Historical */

    switch ( line_style ) {
    case Graphic::DASHED:
	styleDashed[0] = pen_value[pen_colour];
	styleDashed[1] = pen_value[pen_colour];
	styleDashed[2] = pen_value[pen_colour];
	styleDashed[3] = pen_value[pen_colour];
	styleDashed[4] = gdTransparent;
	styleDashed[5] = gdTransparent;
	gdImageSetStyle(im, styleDashed, 6);
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), gdStyled );
	break;

    case Graphic::DASHED_DOTTED:
	styleDashedDotted[0] = pen_value[pen_colour];
	styleDashedDotted[1] = pen_value[pen_colour];
	styleDashedDotted[2] = pen_value[pen_colour];
	styleDashedDotted[3] = pen_value[pen_colour];
	styleDashedDotted[4] = gdTransparent;
	styleDashedDotted[5] = pen_value[pen_colour];
	styleDashedDotted[6] = gdTransparent;
	styleDashedDotted[7] = pen_value[pen_colour];
	styleDashedDotted[8] = gdTransparent;
	gdImageSetStyle(im, styleDashedDotted, 9);
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), gdStyled );

    case Graphic::DOTTED:
	styleDotted[0] = pen_value[pen_colour];
	styleDotted[1] = gdTransparent;
	gdImageSetStyle(im, styleDotted, 2);
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), gdStyled );
	break;

    default:
#if HAVE_GDIMAGESETANTIALIASED
	gdImageSetAntiAliased( im, pen_value[pen_colour]);
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), gdAntiAliased );
#else
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), pen_value[pen_colour] );
#endif
	break;
    }
    return *this;
}

GD const &
GD::circle( const Point& c, const double r, Graphic::colour_type pen_colour,
	    Graphic::colour_type fill_colour ) const
{
    const int d = static_cast<int>(r * 2.0);
#if HAVE_GDIMAGEFILLEDARC
    gdImageFilledArc( im, static_cast<int>(c.x()), static_cast<int>(c.y()), d, d, 0, 360,
		      fill_value.at(fill_colour), gdArc );
#endif
#if HAVE_GDIMAGESETANTIALIASED
    gdImageSetAntiAliased( im, pen_value[pen_colour] );
    gdImageArc( im, static_cast<int>(c.x()), static_cast<int>(c.y()), d, d, 0, 360, gdAntiAliased );
#else
    gdImageArc( im, static_cast<int>(c.x()), static_cast<int>(c.y()), d, d, 0, 360, pen_value[pen_colour] );
#endif

    return *this;
}


GD const&
GD::rectangle( const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
	       Graphic::colour_type fill_colour, Graphic::linestyle_type line_style ) const
{
    std::vector<Point> points(5);
    box_to_points( origin, extent, points );
    gdPoint p1 = moveto( points[0] );
    gdPoint p2 = moveto( points[2] );
    gdImageFilledRectangle( im, p1.x, p1.y, p2.x, p2.y, fill_value.at(fill_colour) );
    switch( line_style ) {
    case Graphic::DASHED:
    case Graphic::DASHED_DOTTED:
    case Graphic::DOTTED:
	for ( unsigned i = 1; i < points.size(); ++i ) {
	    drawline( points[i-1], points[i], pen_colour, line_style );
	}
	break;
    default:
#if HAVE_GDIMAGESETANTIALIASED
	gdImageSetAntiAliased( im, pen_value[pen_colour] );
	gdImageRectangle( im, p1.x, p1.y, p2.x, p2.y, gdAntiAliased );
#else
	gdImageRectangle( im, p1.x, p1.y, p2.x, p2.y, pen_value[pen_colour] );
#endif
	break;
    }
    return *this;
}


double
GD::text( const Point& p1, const std::string& aStr, Graphic::font_type font, int fontsize, Justification justification,
	  Graphic::colour_type pen_colour ) const
{
    int x = static_cast<int>(p1.x());

#if HAVE_GDFTUSEFONTCONFIG
    if ( haveTTF ) {
	int brect[8];

	const int len = width( aStr.c_str(), font, fontsize );		/* Compute bounding box */

	switch ( justification ) {
	case Justification::CENTER: x -= len / 2; break;
	case Justification::RIGHT:  x -= len; break;
	default:             break;
	}

	gdImageStringFT( im, brect,pen_value[pen_colour],
			 const_cast<char *>(font_value[font]),
			 fontsize,
			 0.0, 		/* Angle */
			 x, static_cast<int>(p1.y()),
			 const_cast<char *>(aStr.c_str()) );

	return brect[URy] - brect[LRy];

    } else {
#endif
	gdFont * f = getfont();
	const int len = f->w * aStr.length();

	switch ( justification ) {
	case Justification::CENTER: x -= len / 2; break;
	case Justification::RIGHT:  x -= len; break;
	default:             break;
	}

	gdImageString( im, f, x, static_cast<int>(p1.y()),
		       reinterpret_cast<unsigned char *>(const_cast<char *>(aStr.c_str())),
		       pen_value[pen_colour] );


	return -f->h;
#if HAVE_GDFTUSEFONTCONFIG
    }
#endif
}



gdFont *
GD::getfont() const
{
    if ( Flags::print[FONT_SIZE].opts.value.i <= 10 ) {
	return gdFontTiny;
    } else if ( Flags::print[FONT_SIZE].opts.value.i <= 14 ) {
	return gdFontSmall;
    } else if ( Flags::print[FONT_SIZE].opts.value.i <= 18 ) {
	return gdFontLarge;
    } else {
	return gdFontGiant;
    }
}



unsigned
GD::width( const std::string& aStr, Graphic::font_type font, int fontsize ) const
{
#if HAVE_GDFTUSEFONTCONFIG
    if ( haveTTF ) {
	int brect[8];

	/* Compute bounding box */

	gdImageStringFT( 0, brect, pen_value[Graphic::DEFAULT_COLOUR],
			 const_cast<char *>(font_value[font]),
			 fontsize,
			 0.0, 		/* Angle */
			 0, 0,
			 const_cast<char *>(aStr.c_str()) );

	return brect[LRx] - brect[LLx];
    } else {
#endif
	return aStr.length() * getfont()->w;
#if HAVE_GDFTUSEFONTCONFIG
    }
#endif
}


GD const &
GD::arrowHead( const Point& src, const Point& dst, const double scale,
	       const Graphic::colour_type pen_colour, const Graphic::colour_type fill_colour ) const
{
    const double theta = atan2( dst.y() - src.y(), dst.x() - src.x() );
    const unsigned n_points = 4;

    gdPoint arrow[n_points];
    arrow[0].x = 0;  arrow[0].y = 0;
    arrow[1].x = -6; arrow[1].y = -2;
    arrow[2].x = -5; arrow[2].y = 0;
    arrow[3].x = -6; arrow[3].y = 2;
    Point origin( 0, 0 );
    for ( unsigned i = 0; i < n_points; ++i ) {
	Point aPoint( arrow[i].x * scale, arrow[i].y * scale );
	aPoint.rotate( origin, theta ).moveBy( dst );
	arrow[i] = moveto( aPoint );
    }

    gdImageFilledPolygon( im, arrow, n_points, pen_value.at(fill_colour) );
#if HAVE_GDIMAGESETANTIALIASED
    gdImageSetAntiAliased( im, pen_value[pen_colour] );
    gdImagePolygon( im, arrow, n_points, gdAntiAliased );
#else
    gdImagePolygon( im, arrow, n_points, pen_value[pen_colour] );
#endif
    return *this;
}



gdPoint
GD::moveto( const Point& aPoint )
{
    gdPoint p1;
    p1.x = static_cast<int>(aPoint.x() + 0.5);
    p1.y = static_cast<int>(aPoint.y() + 0.5);
    return p1;
}
#endif

/* -------------------------------------------------------------------- */
/* PostScript output							*/
/* -------------------------------------------------------------------- */

const char * PostScript::font_value[] =
{
    "/Times-Roman",		/* Default */
    "/Times-Roman",		/* Times-Roman */
    "/Times-Italic",		/* Italic */
    "/Times-Bold",		/* Bold */
    "/Symbol"			/* Symbol */
};

/*
 * Initial PostScript goop.
 */

std::ostream&
PostScript::init( std::ostream& output )
{
    output << "/arrowdict 14 dict def" << std::endl
	   << "  arrowdict begin" << std::endl
	   << "/mtrx matrix def" << std::endl
	   << "  end" << std::endl
	   << "/arrow" << std::endl
	   << "{arrowdict begin" << std::endl
	   << "  /headlength exch def" << std::endl
	   << "  /halfheadthickness exch 2 div def" << std::endl
	   << "  /halfthickness exch 2 div def" << std::endl
	   << "  /tipy exch def /tipx exch def" << std::endl
	   << "  /taily exch def /tailx exch def" << std::endl
	   << "  /dx tipx tailx sub def" << std::endl
	   << "  /dy tipy taily sub def" << std::endl
	   << "  /arrowlength dx dx mul dy dy mul add sqrt def" << std::endl
	   << "  /angle dy dx atan def" << std::endl
	   << "  /base arrowlength headlength sub def" << std::endl
	   << "  /savematrix mtrx currentmatrix def" << std::endl
	   << "  tailx taily translate" << std::endl
	   << "  angle rotate" << std::endl
	   << "  base 0 moveto" << std::endl
	   << "  base halfheadthickness neg lineto" << std::endl
	   << "  arrowlength 0 lineto" << std::endl
	   << "  base halfheadthickness lineto" << std::endl
	   << "  base 0 lineto" << std::endl
	   << "  closepath" << std::endl
	   << "  savematrix setmatrix" << std::endl
	   << "  end" << std::endl
	   << "} def" << std::endl;

    output << "/tnt {dup dup currentrgbcolor" << std::endl
	   << "  4 -2 roll dup 1 exch sub 3 -1 roll mul add" << std::endl
	   << "  4 -2 roll dup 1 exch sub 3 -1 roll mul add" << std::endl
	   << "  4 -2 roll dup 1 exch sub 3 -1 roll mul add setrgbcolor}" << std::endl
	   << "bind def" << std::endl;

    /*
     * initial configuration
     */

    output << "1 setlinecap" << std::endl                     /* rounded corners */
	   << "0.5 setlinewidth" << std::endl;

    return output;
}

std::ostream&
PostScript::polyline( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::linestyle_type line_style,
		      Graphic::arrowhead_type arrowhead, double scale ) const
{
    const size_t n_points = points.size();
    output << linestyle( line_style )
	   << "gsave newpath "
	   << moveto( points[0] ) << "moveto ";
    for ( unsigned i = 1; i < n_points; ++i ) {
	output << moveto( points[i] ) << "lineto ";
    }
    output << std::endl
	   << "  " << setcolour( pen_colour ) << " stroke ";

    /* Now draw the arrowhead */

    switch ( arrowhead ) {
    case Graphic::CLOSED_ARROW:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, pen_colour );
	break;
    case Graphic::OPEN_ARROW:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, Graphic::WHITE );
	break;
    }

    output << "grestore" << std::endl;
    return output;
}


std::ostream&
PostScript::polygon( std::ostream& output, const std::vector<Point>& points,
		     Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    const size_t n_points = points.size();
    output << linestyle( Graphic::SOLID )
	   << "gsave newpath "
	   << moveto( points[0] ) << "moveto ";
    for ( unsigned i = 1; i < n_points; ++i ) {
	output << moveto( points[i] ) << "lineto ";
    }
    output << moveto( points[0] ) << "lineto "
	   << "closepath " << std::endl
	   << setfill( fill_colour ) << std::endl
	   << stroke( pen_colour )
	   << "grestore" << std::endl;
    return output;
}


std::ostream&
PostScript::circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    output << linestyle( Graphic::SOLID )
	   << "gsave newpath "
	   << moveto( c ) << r << " 0 360 arc "
	   << "closepath " << std::endl
	   << setfill( fill_colour )
	   << stroke( pen_colour )
	   << "grestore" << std::endl;
    return output;
}

std::ostream&
PostScript::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
		       Graphic::colour_type fill_colour, Graphic::linestyle_type line_style ) const
{
    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( origin, extent, points );

    output << "gsave newpath "
	   << linestyle( line_style )
	   << moveto( points[0] ) << "moveto ";
    for ( unsigned i = 1; i < n_points; ++i ) {
	output << moveto( points[i] ) << "lineto ";
    }
    output << moveto( points[0] ) << "lineto "
	   << "closepath " << std::endl
	   << setfill( fill_colour ) << std::endl
	   << stroke( pen_colour )
	   << "grestore" << std::endl;

    return output;
}


std::ostream&
PostScript::roundedRectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
			      Graphic::colour_type fill_colour, Graphic::linestyle_type line_style ) const
{
    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( origin, extent, points );

    const double radius = 12.0;
    Point aPoint = points[0];
    aPoint.moveBy( 0, radius );
    output << "gsave newpath "
	   << linestyle( line_style )
	   << moveto( aPoint ) << "moveto" << std::endl;

    for ( unsigned i = 1; i < n_points-1; ++i ) {
	output << moveto( points[i] )
	       << moveto( points[i+1] ) 
	       << radius << " arcto" << " 4 {pop} repeat" << std::endl;
    }

    aPoint = points[0];
    output << moveto( aPoint );
    aPoint = points[1];
    aPoint.moveBy( 0, radius );
    output << moveto( aPoint ) << radius << " arcto" << " 4 {pop} repeat" << std::endl;

    output << "closepath " << std::endl
//	   << setfill( Graphic::TRANSPARENT ) << std::endl
	   << setfill( fill_colour ) << std::endl
	   << stroke( pen_colour )
	   << "grestore" << std::endl;

    return output;
}


double
PostScript::text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
		  Justification justification, Graphic::colour_type colour, unsigned ) const
{
    output << "gsave "
	   << setfont( font ) << std::endl
	   << setcolour( colour ) << std::endl
	   << moveto( c ) << " moveto ("
	   << s
	   << ") "
	   << justify( justification ) << " "
	   << "show grestore" << std::endl;

    return -fontsize;
}

/*
 * Print out a point in PostScript Format: x y
 */

std::ostream&
PostScript::point( std::ostream& output, const Point& aPoint )
{
    output << aPoint.x() << ' '
	   << aPoint.y() << ' ';
    return output;
}


std::ostream&
PostScript::linestyle_str( std::ostream& output, Graphic::linestyle_type aStyle )
{
//    if ( myOldLineStyle == aStyle ) return output;
//    myOldLineStyle = style;

    switch( aStyle ) {
    default:
	output << "[] 0";
	break;
    case Graphic::DASHED:
	output << "[" << DASH_LENGTH << "] 0";
	break;
    case Graphic::DASHED_DOTTED:
	output << "[" << DASH_LENGTH  << " " << DOT_GAP  << " 1 " << DOT_GAP << "] 0";
	break;
    case Graphic::DOTTED:
	output << "[1 " << DOT_GAP <<"] " << DOT_GAP;
	break;
    }

    output << " setdash " << std::endl;
    return output;
}


std::ostream&
PostScript::setcolour_str( std::ostream& output, Graphic::colour_type colour )
{
//    if ( myOldColour == colour ) return output;
//    myOldColour = colour;

    output << colour_value[static_cast<int>(colour)].red << ' '
	   << colour_value[static_cast<int>(colour)].green << ' '
	   << colour_value[static_cast<int>(colour)].blue << ' '
	   << "setrgbcolor ";
    return output;
}


std::ostream&
PostScript::setfill_str( std::ostream& output, Graphic::colour_type aColour )
{
    output << "  gsave ";
    if ( aColour == Graphic::TRANSPARENT ) {
	output << "0 setgray ";			// Black.
    } else {
	switch ( aColour ) {
	case Graphic::DEFAULT_COLOUR: output << "1 setgray"; break;			// White.
	case Graphic::BLACK:  	      output << "0 setgray"; break;			// Black.
	case Graphic::GREY_10:	      output << "0 setgray 0.9 tnt"; break;		// Black.
	default:		      output << setcolour( aColour ) << " 0.90 tnt";
	}
	output << " eofill ";
    }
    output << "grestore ";
    return output;
}

std::ostream&
PostScript::setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize )
{
    output << font_value[aFont] << " findfont ["
	   << fontSize
	   << " 0 0 "
	   << fontSize
	   << " 0 0] makefont setfont ";
    return output;
}

std::ostream&
PostScript::justify_str( std::ostream& output, const Justification justification )
{
    switch( justification ) {
    case Justification::LEFT:
	break;
    case Justification::RIGHT:
	output << "dup stringwidth pop neg 0 rmoveto ";
	break;
    default:
	output << "dup stringwidth pop 2 div neg 0 rmoveto ";
	break;
    }
    return output;
}

std::ostream&
PostScript::stroke_str( std::ostream& output, Graphic::colour_type aColour )
{
    output << "  gsave " << setcolour( aColour ) << "stroke grestore ";
    return output;
}


std::ostream&
PostScript::arrowHead(std::ostream& output, const Point& src, const Point& dst, const double scale,
		      const Graphic::colour_type pen, const Graphic::colour_type fill ) const
{
    if ( src == dst ) return output;
    output << "gsave "
	   << moveto( src ) << moveto( dst )
	   << 1.0 * scale << " "
	   << 3.0 * scale << " "
	   << 6.0 * scale << " arrow ";
    output << "gsave " << setcolour( fill ) << "eofill grestore ";
    output << "gsave " << setcolour( pen ) << "stroke grestore grestore" << std::endl;
    return output;
}

Colour::ColourManip
PostScript::setcolour( const Graphic::colour_type aColour )
{
    return Colour::ColourManip( PostScript::setcolour_str, aColour );
}

Colour::ColourManip
PostScript::stroke( const Graphic::colour_type aColour )
{
    return Colour::ColourManip( PostScript::stroke_str, aColour );
}

Colour::ColourManip
PostScript::setfill( const Graphic::colour_type aColour )
{
    return Colour::ColourManip( PostScript::setfill_str, aColour );
}

PointManip
PostScript::moveto( const Point& aPoint )
{
    return PointManip( point, aPoint );
}


Colour::FontManip
PostScript::setfont( const Graphic::font_type aFont )
{
    return Colour::FontManip( setfont_str, aFont, Flags::print[FONT_SIZE].opts.value.i );
}

Colour::JustificationManip
PostScript::justify( const Justification justification )
{
    return Colour::JustificationManip( justify_str, justification );
}


Colour::LineStyleManip
PostScript::linestyle( const Graphic::linestyle_type aStyle )
{
    return Colour::LineStyleManip( PostScript::linestyle_str, aStyle );
}

/* -------------------------------------------------------------------- */
/* Scalable Vector Grahpics Ouptut					*/
/* -------------------------------------------------------------------- */

SVG::font_defn SVG::font_value[] =
{
    { "Times", "Roman" },	/* Default */
    { "Times", "Roman" },	/* Times-Roman */
    { "Times", "Italic" },	/* Italic */
    { "Times", "Bold" },
    { "Times", "Roman" }	/* Symbol */
};

/*
 * Initial SVG goop.
 */

std::ostream&
SVG::init( std::ostream& output )
{
    return output;
}

//  <polyline fill="none" stroke="blue" stroke-width="10"
//             points="50,375
//                     150,375 150,325 250,325 250,375
//                     1150,375" />

std::ostream&
SVG::polyline( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::linestyle_type line_style,
		      Graphic::arrowhead_type arrowhead, double scale ) const
{
    /* sodipodi doesn't like polylines of 2 points */
    const size_t n_points = points.size();
    if ( n_points == 2 ) {
	output << "<line x1=\"" << points[0].x()
	       << "\" y1=\"" << points[0].y()
	       << "\" x2=\"" << points[1].x()
	       << "\" y2=\"" << points[1].y()
	       << "\""
	       << linestyle( line_style )
	       << stroke( pen_colour )
	       << "/>" << std::endl;
    } else {
	output << "<polyline "
	       << linestyle( line_style )
	       << stroke( pen_colour )
	       << " fill=\"none\""
	       << " points=\"";
	for ( unsigned i = 0; i < n_points; ++i ) {
	    output << moveto( points[i] );
	}
	output << "\"/>" << std::endl;
    }

    /* Now draw the arrowhead */

    switch ( arrowhead ) {
    case Graphic::CLOSED_ARROW:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, pen_colour );
	break;
    case Graphic::OPEN_ARROW:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, Graphic::WHITE );
	break;
    }

    return output;
}


// <polygon fill="red" stroke="blue" stroke-width="10"
//             points="350,75  379,161 469,161 397,215
//                     423,301 350,250 277,301 303,215
//                     231,161 321,161" />

std::ostream&
SVG::polygon( std::ostream& output, const std::vector<Point>& points,
	      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    const size_t n_points = points.size();
    output << "<polygon "
	   << setfill( fill_colour )
	   << stroke( pen_colour )
	   << " points=\"";
    for ( unsigned i = 0; i < n_points; ++i ) {
	output << moveto( points[i] );
    }
    output << "\"/>" << std::endl;
    return output;
}

/*
 *   <circle cx="600" cy="200" r="100"
 *         fill="red" stroke="blue" stroke-width="10"  />
 */

std::ostream&
SVG::circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    output << "<circle cx=\"" << static_cast<int>(c.x() + 0.5)
	   << "\" cy=\"" << static_cast<int>(c.y() + 0.5)
	   << "\" r=\"" << static_cast<int>(r + 0.5) << "\" "
	   << setfill( fill_colour )
	   << stroke( pen_colour )
	   << "/>" << std::endl;
    return output;
}

std::ostream&
SVG::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
		Graphic::colour_type fill_colour, Graphic::linestyle_type line_style ) const
{
    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( origin, extent, points );

    output << "<polygon "
	   << linestyle( line_style )
	   << setfill( fill_colour )
	   << stroke( pen_colour )
	   << " points=\"";
    for ( unsigned i = 0; i < n_points; ++i ) {
	output << moveto( points[i] );
    }
    output << "\"/>" << std::endl;
    return output;
}


// <text x="574" y="1100" fill="#000000"  font-family="Times"
// 		 font-style="italic" font-weight="normal" font-size="81" text-anchor="middle" >
// 0,0.441,0</text>

double
SVG::text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
	   Justification justification, Graphic::colour_type colour, unsigned ) const
{
    output << "<text x=\"" << static_cast<int>(c.x() + 0.5)
	   << "\" y=\"" << static_cast<int>(c.y() + 0.5) << "\""
	   << setfont( font )
	   << " fill=\"" << setcolour( colour ) << "\""
	   << justify( justification )
	   << ">"
	   << xml_escape( s )
	   << "</text>" << std::endl;

    return -fontsize * SVG_SCALING;
}

/*
 * Print out a point in SVG Format: x y
 */

std::ostream&
SVG::point( std::ostream& output, const Point& aPoint )
{
    output << aPoint.x() << ','
	   << aPoint.y() << ' ';
    return output;
}


std::ostream&
SVG::linestyle_str( std::ostream& output, Graphic::linestyle_type aStyle )
{
    switch( aStyle ) {
    case Graphic::DASHED:
	output << " stroke-dasharray=\""
	       << DASH_LENGTH * SVG_SCALING << ","
	       << DOT_GAP * SVG_SCALING
	       << "\"";
	break;
    case Graphic::DASHED_DOTTED:
	output << " stroke-dasharray=\""
	       << DASH_LENGTH * SVG_SCALING << ","
	       << DOT_GAP * SVG_SCALING << ","
	       << SVG_SCALING << ","
	       << DOT_GAP * SVG_SCALING 
	       << "\"";

    case Graphic::DOTTED:
	output << " stroke-dasharray=\""
	       << DOT_GAP * SVG_SCALING
	       << "\"";
	break;

    default:
	break;
    }

    return output;
}


std::ostream&
SVG::setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize )
{
    output << " font-family=\"" << font_value[aFont].family
	   << "\" font-style=\""  << font_value[aFont].style
	   << "\" font-size=\"" << fontSize * SVG_SCALING << "\"";
    return output;
}

std::ostream&
SVG::justify_str( std::ostream& output, const Justification justification )
{
    output << " text-anchor=\"" ;
    switch( justification ) {
    case Justification::RIGHT:
	output << "right";
	break;
    case Justification::LEFT:
	output << "left";
	break;
    default:
	output << "middle";
	break;
    }
    output << "\"";
    return output;
}

std::ostream&
SVG::setcolour_str( std::ostream& output, Graphic::colour_type aColour )
{
    output << colour_name[static_cast<int>(aColour)];
    return output;
}


/*
 * Tint all colours except black and white.   Default fill colour is white.  Default Pen colour is black.
 */

std::ostream&
SVG::setfill_str( std::ostream& output, Graphic::colour_type aColour )
{
    output << " fill=\"";
    switch ( aColour ) {
    case Graphic::TRANSPARENT:
	output << "none";
	break;

    case Graphic::BLACK:
	output << rgb( 0, 0, 0 );
	break;

    case Graphic::WHITE:
    case Graphic::DEFAULT_COLOUR:
	output << rgb( 1.0, 1.0, 1.0 );
	break;

    default:
	output << rgb( tint( colour_value[aColour].red, 0.9 ),
		       tint( colour_value[aColour].green, 0.9 ),
		       tint( colour_value[aColour].blue, 0.9 ) );
	break;
    }
    output << "\"";
    return output;
}

std::ostream&
SVG::stroke_str( std::ostream& output, Graphic::colour_type aColour )
{
    std::ios_base::fmtflags flags = output.setf( std::ios::hex, std::ios::basefield );
    output << " stroke=\"" << setcolour( aColour )
	   << "\" stroke-width=\"" << SVG_SCALING	// 1 pt.
	   << "\"";
    output.setf( flags );
    return output;
}


/*
 * Escape special characters in XML.  &# is not escaped (font stuff.)
 */

std::ostream&
SVG::arrowHead( std::ostream& output, const Point& src, const Point& dst, const double scale,
		const Graphic::colour_type pen_colour, const Graphic::colour_type fill ) const
{
    const double theta = atan2( dst.y() - src.y(), dst.x() - src.x() );
    const unsigned n_points = 4;

    output << "<polygon";
    std::ios_base::fmtflags flags = output.setf( std::ios::hex, std::ios::basefield );
    output << " fill=\"" << setcolour( fill ) << "\"";
    output.setf( flags );
    output << stroke( pen_colour )
	   << " points=\"";

    Point arrow[n_points];
    arrow[0].x( 0 );  arrow[0].y( 0 );
    arrow[1].x( -6 ); arrow[1].y( -2 );
    arrow[2].x( -5 ); arrow[2].y( 0 );
    arrow[3].x( -6 ); arrow[3].y( 2 );
    Point origin( 0, 0 );
    for ( unsigned i = 0; i < n_points; ++i ) {
	output << moveto( arrow[i].scaleBy( scale, scale ).rotate( origin, theta ).moveBy( dst ) );
    }
    output << "\"/>" << std::endl;
    return output;
}

#if defined(SXD_OUTPUT)
/* -------------------------------------------------------------------- */
/* Open(Star)Office output						*/
/* -------------------------------------------------------------------- */

/*
 * Output style informations (for styles.xml)
 */
const char * SXD::graphics_family  = "style:family=\"graphics\" style:parent-style-name=\"standard\" ";
const char * SXD::paragraph_family = "style:family=\"paragraph\" ";
const char * SXD::text_family      = "style:family=\"text\" ";


std::ostream&
SXD::printStyles( std::ostream& output )
{
    output << indent( 0 ) << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    output << indent( 0 ) << "<!DOCTYPE office:document-styles PUBLIC \"-//OpenOffice.org//DTD OfficeDocument 1.0//EN\" \"office.dtd\">" << std::endl;
    output << indent( +1 ) << "<office:document-styles xmlns:office=\"http://openoffice.org/2000/office\" xmlns:style=\"http://openoffice.org/2000/style\" xmlns:text=\"http://openoffice.org/2000/text\" xmlns:table=\"http://openoffice.org/2000/table\" xmlns:draw=\"http://openoffice.org/2000/drawing\" xmlns:fo=\"http://www.w3.org/1999/XSL/Format\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:number=\"http://openoffice.org/2000/datastyle\" xmlns:presentation=\"http://openoffice.org/2000/presentation\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:chart=\"http://openoffice.org/2000/chart\" xmlns:dr3d=\"http://openoffice.org/2000/dr3d\" xmlns:math=\"http://www.w3.org/1998/Math/MathML\" xmlns:form=\"http://openoffice.org/2000/form\" xmlns:script=\"http://openoffice.org/2000/script\" office:version=\"1.0\">" << std::endl;

    output << indent( +1 ) << "<office:styles>" << std::endl;
    output << indent( 0 ) << "<draw:marker draw:name=\"Arrow\" svg:viewBox=\"0 0 20 30\" svg:d=\"m10 0-10 30h20z\"/>" << std::endl;
    output << indent( 0 ) << "<draw:marker draw:name=\"Arrow concave\" svg:viewBox=\"0 0 1131 1580\" svg:d=\"m1013 1491 118 89-567-1580-564 1580 114-85 136-68 148-46 161-17 161 13 153 46z\"/>" << std::endl;
    output << indent( 0 ) << "<draw:marker draw:name=\"Line Arrow\" svg:viewBox=\"0 0 1122 2243\" svg:d=\"m0 2108v17 17l12 42 30 34 38 21 43 4 29-8 30-21 25-26 13-34 343-1532 339 1520 13 42 29 34 39 21 42 4 42-12 34-30 21-42v-39-12l-4 4-440-1998-9-42-25-39-38-25-43-8-42 8-38 25-26 39-8 42z\"/>" << std::endl;
    output << indent( 0 ) << "<draw:stroke-dash draw:name=\"Dash 1\" draw:style=\"rect\" "
	   << "draw:dots1=\"1\" draw:dots1-length=\"" << DASH_LENGTH * SXD_SCALING
	   << "cm\" draw:distance=\"" << DOT_GAP * SXD_SCALING << "cm\"/>" << std::endl;
    output << indent( 0 ) << "<draw:stroke-dash draw:name=\"Dot 1\" draw:style=\"rect\" "
	   << "draw:dots1=\"1\" draw:distance=\"" << DOT_GAP * SXD_SCALING << "cm\"/>" << std::endl;

    output << indent( +1 ) << "<style:default-style style:family=\"graphics\">" << std::endl;
    output << style_properties( +1 ) << " style:use-window-font-color=\"true\" fo:font-family=\"&apos;Times New Roman&apos;\" style:font-family-generic=\"roman\" style:font-pitch=\"variable\" fo:font-size=\"24pt\" fo:language=\"en\" fo:country=\"US\" style:font-family-asian=\"&apos;Bitstream Vera Sans&apos;\" style:font-pitch-asian=\"variable\" style:font-size-asian=\"24pt\" style:language-asian=\"none\" style:country-asian=\"none\" style:font-family-complex=\"Lucidasans\" style:font-pitch-complex=\"variable\" style:font-size-complex=\"24pt\" style:language-complex=\"none\" style:country-complex=\"none\" style:text-autospace=\"ideograph-alpha\" style:punctuation-wrap=\"simple\" style:line-break=\"strict\" style:writing-mode=\"lr-tb\">" << std::endl;
    output << indent( 0 ) << "<style:tab-stops/>" << std::endl;
    output << style_properties( ~0 ) << std::endl;
    output << indent( -1 ) << "</style:default-style>" << std::endl;

    output << start_style( "standard",  "style:family=\"graphics\"" ) << std::endl;
    output << style_properties( +1 ) << " draw:stroke=\"solid\" svg:stroke-width=\"0.5pt\" "
	   << stroke_colour( Graphic::BLACK )
	   << " draw:marker-start-width=\"0.3cm\" draw:marker-start-center=\"false\" draw:marker-end-width=\"0.3cm\" draw:marker-end-center=\"false\" draw:fill=\"solid\" draw:fill-color=\"#00b8ff\" draw:shadow=\"hidden\" draw:shadow-offset-x=\"0.3cm\" draw:shadow-offset-y=\"0.3cm\" draw:shadow-color=\"#808080\" fo:margin-left=\"0cm\" fo:margin-right=\"0cm\" fo:margin-top=\"0cm\" fo:margin-bottom=\"0cm\" style:use-window-font-color=\"true\" style:text-outline=\"false\" style:text-crossing-out=\"none\" fo:font-family=\"&apos;Times New Roman&apos;\" style:font-family-generic=\"roman\" style:font-pitch=\"variable\" fo:font-size=\"24pt\" fo:font-style=\"normal\" fo:text-shadow=\"none\" style:text-underline=\"none\" fo:font-weight=\"normal\" style:font-family-asian=\"Gothic\" style:font-pitch-asian=\"variable\" style:font-size-asian=\"24pt\" style:font-style-asian=\"normal\" style:font-weight-asian=\"normal\" style:font-family-complex=\"Lucidasans\" style:font-pitch-complex=\"variable\" style:font-size-complex=\"24pt\" style:font-style-complex=\"normal\" style:font-weight-complex=\"normal\" style:font-relief=\"none\" fo:line-height=\"100%\" text:enable-numbering=\"false\">" << std::endl;
    output << style_properties( ~0 ) << std::endl;
    output << end_style() << std::endl;
    output << start_style( "objectwitharrow", graphics_family ) << std::endl;
    output << style_properties( 0 ) << "draw:stroke=\"solid\" svg:stroke-width=\"0.5pt\" "
	   << stroke_colour( Graphic::BLACK ) << "draw:marker-start=\"Arrow\" draw:marker-start-width=\"0.7cm\" draw:marker-start-center=\"true\" draw:marker-end-width=\"0.3cm\"/>" << std::endl;
    output << end_style() << std::endl;
    output << start_style( "objectwithshadow", graphics_family ) << std::endl;
    output << style_properties( 0 ) << " draw:shadow=\"visible\" draw:shadow-offset-x=\"0.3cm\" draw:shadow-offset-y=\"0.3cm\" draw:shadow-color=\"#808080\"/>" << std::endl;
    output << end_style() << std::endl;
    output << start_style( "objectwithoutfill", graphics_family ) << std::endl;
    output << style_properties( 0 ) << " draw:fill=\"none\"/>" << std::endl;
    output << end_style() << std::endl;
    output << start_style( "text", graphics_family ) << std::endl;
    output << style_properties( 0 ) << " draw:stroke=\"none\" draw:fill=\"none\"/>" << std::endl;
    output << end_style() << std::endl;
    output << start_style( "textbody", graphics_family ) << std::endl;
    output << style_properties( 0 ) << " draw:stroke=\"none\" draw:fill=\"none\" fo:font-size=\"16pt\"/>" << std::endl;
    output << end_style() << std::endl;
    output << start_style( "textbodyjustfied", graphics_family ) << std::endl;
    output << style_properties( 0 ) << " draw:stroke=\"none\" draw:fill=\"none\" fo:text-align=\"justify\"/>" << std::endl;
    output << end_style() << std::endl;
    output << start_style( "textbodyindent", graphics_family ) << std::endl;
    output << style_properties( 0 ) << " draw:stroke=\"none\" draw:fill=\"none\" fo:margin-left=\"0cm\" fo:margin-right=\"0cm\"/>" << std::endl;
    output << end_style() << std::endl;
    output << start_style( "measure", graphics_family ) << std::endl;
    output << style_properties( 0 ) << " draw:stroke=\"solid\" draw:marker-start=\"Arrow\" draw:marker-start-width=\"0.2cm\" draw:marker-end=\"Arrow\" draw:marker-end-width=\"0.2cm\" draw:fill=\"none\" fo:font-size=\"12pt\"/>" << std::endl;
    output << end_style() << std::endl;
    output << indent( -1 ) << "</office:styles>" << std::endl;

    output << indent( +1 ) << "<office:automatic-styles>" << std::endl;
    output << indent( +1 ) << "<style:page-master style:name=\"PM0\">" << std::endl;
    output << style_properties( 0 ) << " fo:margin-top=\"2cm\" fo:margin-bottom=\"2cm\" fo:margin-left=\"2cm\" fo:margin-right=\"2cm\" fo:page-width=\"27.94cm\" fo:page-height=\"21.59cm\" style:print-orientation=\"landscape\"/>" << std::endl;
    output << indent( -1 ) << "</style:page-master>" << std::endl;
    output << indent( +1 ) << "<style:page-master style:name=\"PM1\">" << std::endl;
    output << style_properties( 0 ) << " fo:margin-top=\"0.635cm\" fo:margin-bottom=\"0.665cm\" fo:margin-left=\"0.635cm\" fo:margin-right=\"0.665cm\" fo:page-width=\"21.59cm\" fo:page-height=\"27.94cm\" style:print-orientation=\"portrait\"/>" << std::endl;
    output << indent( -1 ) << "</style:page-master>" << std::endl;
    output << start_style( "dp1", "style:family=\"drawing-page\"" ) << std::endl;
    output << style_properties( 0 ) << " draw:background-size=\"border\" draw:fill=\"none\"/>" << std::endl;
    output << end_style() << std::endl;
    output << indent( -1 ) << "</office:automatic-styles>" << std::endl;

    output << indent( +1 ) << "<office:master-styles>" << std::endl;
    output << indent( +1 ) << "<draw:layer-set>" << std::endl;
    output << draw_layer( "layout" ) << "/>"  << std::endl;
    output << draw_layer( "background" ) << "/>" << std::endl;
    output << draw_layer( "backgroundobjects" ) << "/>" << std::endl;
    output << draw_layer( "controls" ) << "/>" << std::endl;
    output << draw_layer( "measurelines" ) << "/>" << std::endl;
    output << indent( -1 ) << "</draw:layer-set>" << std::endl;
    output << indent( 0 ) << "<style:master-page style:name=\"Default\" style:page-master-name=\"PM1\" draw:style-name=\"dp1\"/>" << std::endl;
    output << indent( -1 ) << "</office:master-styles>" << std::endl;

    output << indent( -1 ) << "</office:document-styles>" << std::endl;
    return output;
}


/*
 * Initial SXD goop.  The styles used are defined here.
 */

std::ostream&
SXD::init( std::ostream& output )
{
    static const char * text_style = "fo:margin-left=\"0cm\" fo:line-height=\"75%\" fo:margin-right=\"0cm\" fo:text-indent=\"0cm\" ";
    static const char * draw_dashed = "draw:stroke=\"dash\" draw:stroke-dash=\"Dash 1\" ";
    static const char * draw_dotted = "draw:stroke=\"dash\" draw:stroke-dash=\"Dot 1\" ";

    output << indent( +1 ) << "<office:automatic-styles>" << std::endl;
    output << indent( 0 )  << "<style:style style:name=\"dp1\" style:family=\"drawing-page\"/>" << std::endl;
    output << start_style( "gr1", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::TRANSPARENT ) << setfill( Graphic::TRANSPARENT )
	   << justify( Justification::CENTER ) << "draw:auto-grow-width=\"true\" draw:auto-grow-height=\"true\" fo:min-height=\"0cm\" fo:min-width=\"0cm\"/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr2", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::TRANSPARENT ) << setfill( Graphic::TRANSPARENT )
	   << justify( Justification::LEFT ) << "draw:auto-grow-width=\"true\" draw:auto-grow-height=\"true\" fo:min-height=\"0cm\" fo:min-width=\"0cm\"/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr3", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::TRANSPARENT ) << setfill( Graphic::TRANSPARENT )
	   << justify( Justification::RIGHT ) << "draw:auto-grow-width=\"true\" draw:auto-grow-height=\"true\" fo:min-height=\"0cm\" fo:min-width=\"0cm\"/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr4", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLACK ) << setfill( Graphic::WHITE )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr5", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLACK ) << setfill( Graphic::BLACK )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr6", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLUE )  << setfill( Graphic::BLUE )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr7", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::GREEN ) << setfill( Graphic::GREEN )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr8", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::ORANGE )<< setfill( Graphic::ORANGE )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr9", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::RED )   << setfill( Graphic::RED )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr10", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLACK ) << setfill( Graphic::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr11", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLUE ) << setfill( Graphic::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr12", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::GREEN ) << setfill( Graphic::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr13", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::ORANGE ) << setfill( Graphic::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr14", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::RED ) << setfill( Graphic::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr15", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLACK ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr16", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLUE ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr17", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::GREEN ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr18", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::ORANGE ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr19", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::RED ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr20", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLACK ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::OPEN_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr21", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLUE ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::OPEN_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr22", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::GREEN ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::OPEN_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr23", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::ORANGE ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::OPEN_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr24", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::RED ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::OPEN_ARROW ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr25", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLACK ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr26", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLUE ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr27", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::GREEN ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr28", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::ORANGE ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr29", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::RED ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr30", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLACK ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr31", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::BLUE ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr32", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::GREEN ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr33", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::ORANGE ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr34", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::RED ) << setfill( Graphic::TRANSPARENT )
	   << arrow_style( Graphic::CLOSED_ARROW ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "P1", paragraph_family ) << std::endl
           << style_properties( 0 ) << setfont( Graphic::DEFAULT_FONT ) << "fo:text-align=\"center\"/>" << std::endl
           << end_style() << std::endl;
    output << start_style( "P2", paragraph_family ) << std::endl
           << style_properties( 0 ) << text_style << setfont( Graphic::DEFAULT_FONT ) << " fo:text-align=\"center\"/>" << std::endl
           << end_style() << std::endl;
    output << start_style( "P3", paragraph_family ) << std::endl
           << style_properties( 0 ) << text_style << setfont( Graphic::DEFAULT_FONT ) << " fo:text-align=\"left\"  />" << std::endl
           << end_style() << std::endl;
    output << start_style( "P4", paragraph_family ) << std::endl
           << style_properties( 0 ) << text_style << setfont( Graphic::DEFAULT_FONT ) << " fo:text-align=\"right\" />" << std::endl
           << end_style() << std::endl;
    output << start_style( "P5", paragraph_family ) << std::endl
           << style_properties( 0 ) << setfont( Graphic::DEFAULT_FONT ) << "/>" << std::endl
           << end_style() << std::endl;
    output << std::endl;
    output << start_style( "T1", text_family ) << std::endl
           << style_properties( 0 ) << setfont( Graphic::DEFAULT_FONT ) << "/>" << std::endl
           << end_style() << std::endl;
    output << indent( -1 ) << "</office:automatic-styles>" << std::endl;
    return output;
}


std::ostream&
SXD::init( std::ostream& output, const char * object_name,
	   Graphic::colour_type pen_colour, Graphic::colour_type fill_colour,
	   Graphic::linestyle_type line_style, Graphic::arrowhead_type arrow ) const
{
    output << indent( 0 ) << "<draw:" << object_name << " draw:style-name=\"";
    if ( fill_colour != Graphic::TRANSPARENT ) {
	switch( fill_colour ) {
	default:		output << "gr4"; break;
	case Graphic::BLACK: 	output << "gr5"; break;
	case Graphic::BLUE: 	output << "gr6"; break;
	case Graphic::GREEN: 	output << "gr7"; break;
	case Graphic::ORANGE: 	output << "gr8"; break;
	case Graphic::RED: 	output << "gr9"; break;
	}
    } else switch ( line_style ) {
    default:
	switch ( arrow ) {
	default:
	    switch ( pen_colour ) {
	    default:                output << "gr10"; break;
	    case Graphic::BLUE:     output << "gr11"; break;
	    case Graphic::GREEN:    output << "gr12"; break;
	    case Graphic::ORANGE:   output << "gr13"; break;
	    case Graphic::RED:      output << "gr14"; break;
	    }
	    break;

	case Graphic::CLOSED_ARROW:
	    switch ( pen_colour ) {
	    default:                output << "gr15"; break;
	    case Graphic::BLUE:     output << "gr16"; break;
	    case Graphic::GREEN:    output << "gr17"; break;
	    case Graphic::ORANGE:   output << "gr18"; break;
	    case Graphic::RED:      output << "gr19"; break;
	    }
	    break;

	case Graphic::OPEN_ARROW:
	    switch ( pen_colour ) {
	    default:                output << "gr20"; break;
	    case Graphic::BLUE:     output << "gr21"; break;
	    case Graphic::GREEN:    output << "gr22"; break;
	    case Graphic::ORANGE:   output << "gr23"; break;
	    case Graphic::RED:      output << "gr24"; break;
	    }
	    break;

	}
	break;

    case Graphic::DASHED:		/* Forwarding uses closed arrows. */
	switch ( pen_colour ) {
	default:                output << "gr25"; break;
	case Graphic::BLUE:     output << "gr26"; break;
	case Graphic::GREEN:    output << "gr27"; break;
	case Graphic::ORANGE:   output << "gr28"; break;
	case Graphic::RED:      output << "gr29"; break;
	}
	break;

    case Graphic::DASHED_DOTTED:
    case Graphic::DOTTED:	/* Replies use closed arrows. */
	switch ( pen_colour ) {
	default:                output << "gr30"; break;
	case Graphic::BLUE:     output << "gr31"; break;
	case Graphic::GREEN:    output << "gr32"; break;
	case Graphic::ORANGE:   output << "gr33"; break;
	case Graphic::RED:      output << "gr34"; break;
	}
	break;

    }

    output << "\" draw:text-style-name=\"P1\""
	   << " draw:layer=\"layout\"" << std::endl;
    return output;
}

// <draw:line draw:style-name="gr3" draw:text-style-name="P1" draw:layer="layout"
//  svg:x1="15.799cm" svg:y1="7.757cm" svg:x2="18.534cm" svg:y2="14.853cm"/>
// <draw:polyline draw:style-name="gr2" draw:layer="layout"
//     svg:x="11.483cm" svg:y="13.864cm"
//     svg:width="2.515cm" svg:height="1.906cm"
//     svg:viewBox="0 0 2515 1906"
//     draw:points="0,0 0,1905 2514,1905 2514,0 1257,953"/>


std::ostream&
SXD::polyline( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::linestyle_type line_style,
		      Graphic::arrowhead_type arrowhead, double scale ) const
{
    const size_t n_points = points.size();
    /* OpenOffice doesn't like polylines of 2 points */
    if ( n_points == 2 ) {
	init( output, "line", pen_colour, Graphic::TRANSPARENT, line_style, arrowhead );
	output << temp_indent( 1 )
	       << "svg:x1=\"" << points[0].x() << "cm\" "
	       << "svg:y1=\"" << points[0].y() << "cm\" "
	       << "svg:x2=\"" << points[1].x() << "cm\" "
	       << "svg:y2=\"" << points[1].y() << "cm\"/>" << std::endl;
    } else {
	init( output, "polyline", pen_colour, Graphic::TRANSPARENT, line_style, arrowhead );
	return drawline( output, points );
    }
    return output;
}


// <polygon fill="red" stroke="blue" stroke-width="10"
//             points="350,75  379,161 469,161 397,215
//                     423,301 350,250 277,301 303,215
//                     231,161 321,161" />

std::ostream&
SXD::polygon( std::ostream& output, const std::vector<Point>& points,
	      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    init( output, "polygon", pen_colour, fill_colour, Graphic::SOLID, Graphic::NO_ARROW );
    return drawline( output, points );
}


std::ostream&
SXD::drawline( std::ostream& output, const std::vector<Point>& points ) const
{
    const size_t n_points = points.size();
    Point origin = points[0];
    Point extent = points[0];
    for ( unsigned int i = 1; i < n_points; ++i ) {
	origin.min( points[i] );
	extent.max( points[i] );
    }
    extent -= origin;

    output << temp_indent( 1 ) << box( origin, extent ) << std::endl;
    output << temp_indent( 1 ) << "svg:viewBox=\"0 0 "
	   << static_cast<int>((extent.x() * 1000) + 0.5) << " "
	   << static_cast<int>((extent.y() * 1000) + 0.5) << "\"" << std::endl;
    output << temp_indent( 1 ) << "draw:points=\"";
    for ( unsigned int i = 0; i < n_points; ++i ) {
	Point aPoint = points[i];
	aPoint -= origin;
	output << moveto( aPoint );
    }
    output << "\"/>" << std::endl;

    return output;
}

// <draw:circle draw:style-name="gr2"
//     draw:text-style-name="P1" draw:layer="layout"
//     svg:x="5.533cm" svg:y="1.984cm"
//     svg:width="1.745cm" svg:height="1.745cm"/>


std::ostream&
SXD::circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    init( output, "circle", pen_colour, fill_colour, Graphic::SOLID );
    Point origin = c;
    Point extent( r, r );
    origin -= extent;
    extent *= 2;
    output << temp_indent(1) << box( origin, extent ) << "/>" << std::endl;
    return output;
}

std::ostream&
SXD::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
		Graphic::colour_type fill_colour, Graphic::linestyle_type line_style ) const
{
    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( origin, extent, points );
    init( output, "polygon", pen_colour, fill_colour, line_style, Graphic::NO_ARROW );
    return drawline( output, points );
}


// <!-- within document body -->
// <draw:text-box draw:style-name="gr7" draw:text-style-name="P5"
// draw:layer="layout"
// svg:width="2.781cm" svg:height="0.505cm"
// svg:x="9.327cm" svg:y="2.62cm">

std::ostream&
SXD::begin_paragraph( std::ostream& output, const Point& origin, const Point& extent, const Justification justification ) const
{
    output << indent( 1 ) << "<draw:text-box draw:style-name=\"";
    switch ( justification ) {			/* Alignment of BOX */
    case Justification::LEFT:	output << "gr2"; break;
    case Justification::RIGHT:	output << "gr3"; break;
    default:		output << "gr1"; break;
    }
    output << "\" draw:text-style-name=\"P5\" draw:layer=\"layout\" " << box( origin, extent ) << ">" << std::endl;
    return output;
}

// <text:p text:style-name="P1">Sample Text</text:p>
// </draw:text-box>

double
SXD::text( std::ostream& output, const Point&, const std::string& s, Graphic::font_type font, int fontsize,
	   Justification justification, Graphic::colour_type colour, unsigned ) const
{
    output << indent( 1 )  << "<text:p text:style-name=\"";
    switch ( justification ) {			/* Alignment of Text within BOX */
    case Justification::LEFT:	 output << "P3"; break;
    case Justification::RIGHT:	 output << "P4"; break;
    default:  		 output << "P2"; break;
    }
    output << "\">" << std::endl;
    output << indent( 0 )  << "<text:span text:style-name=\"T1\">" << xml_escape( s ) << "</text:span>" << std::endl;
    output << indent( -1 ) << "</text:p>" << std::endl;

    return 0.0;
}

/*
 * Common expressions for setting styles.  Called from init.
 */

std::ostream&
SXD::arrow_style_str( std::ostream& output, Graphic::arrowhead_type arrowhead )
{
    switch ( arrowhead ) {
    case Graphic::CLOSED_ARROW:
	output << "draw:marker-end=\"Arrow concave\" draw:marker-end-width=\"0.2cm\" ";
	break;
    case Graphic::OPEN_ARROW:
	output << "draw:marker-end=\"Line Arrow\" draw:marker-end-width=\"0.2cm\" ";
	break;
    }
    return output;
}

std::ostream&
SXD::box_str( std::ostream& output, const Point& origin, const Point& extent )
{
    output << "svg:x=\"" << origin.x() << "cm\" "
	   << "svg:y=\"" << origin.y() << "cm\" "
	   << "svg:width=\"" << extent.x() << "cm\" "
	   << "svg:height=\"" << extent.y() << "cm\" ";
    return output;
}

std::ostream&
SXD::draw_layer_str( std::ostream& output, const std::string& name, const std::string& )
{
    output << indent( 0 )  << "<draw:layer draw:name=\"" << name << "\"";
    return output;
}


std::ostream&
SXD::end_paragraph( std::ostream& output ) const
{
    output << indent( -1 ) << "</draw:text-box>" << std::endl;
    return output;
}

std::ostream&
SXD::end_style_str( std::ostream& output, const std::string&s1, const std::string&s2 )
{
    output << indent( -1 ) << "</style:style>";
    return output;
}


/*
 * Tint all colours except black and white.   Default fill colour is white.  Default Pen colour is black.
 */

std::ostream&
SXD::setfill_str( std::ostream& output, const Graphic::colour_type colour )
{
    switch ( colour ) {
    case Graphic::TRANSPARENT:
	output << "draw:fill=\"none\" ";
	break;

    case Graphic::WHITE:
	output << "draw:fill-color=\"" << rgb( Colour::colour_value[colour].red,
					       Colour::colour_value[colour].green,
					       Colour::colour_value[colour].blue ) << "\" ";
	break;

    case Graphic::BLACK:
	output << "draw:fill-color=\"" << rgb( Colour::colour_value[colour].red,
					       Colour::colour_value[colour].green,
					       Colour::colour_value[colour].blue ) << "\" ";
	break;

    default:
	output << "draw:fill-color=\"" << rgb( tint( Colour::colour_value[colour].red,   0.9 ),
					       tint( Colour::colour_value[colour].green, 0.9 ),
					       tint( Colour::colour_value[colour].blue,  0.9 ) ) << "\" ";
	break;
    }
    return output;
}

std::ostream&
SXD::justify_str( std::ostream& output, const Justification justification )
{
    output << "draw:textarea-horizontal-align=\"";
    switch ( justification ) {
    case Justification::LEFT:   output << "left"; break;
    case Justification::RIGHT:  output << "right"; break;
    default:             output << "center"; break;
    }
    output << "\" draw:textarea-vertical-align=\"middle\" ";
    return output;
}

/*
 * Print out a point in SXD Format: x,y (cm * 1000)
 */

std::ostream&
SXD::point( std::ostream& output, const Point& aPoint )
{
    output << static_cast<int>(aPoint.x()*1000.0+0.5) << ','
	   << static_cast<int>(aPoint.y()*1000.0+0.5) << ' ';
    return output;
}

std::ostream&
SXD::start_style_str( std::ostream& output, const std::string&name, const std::string&family )
{
    output << indent( +1 )
	   << "<style:style style:name=\"" << name << "\" "
	   << family << ">";
    return output;
}


std::ostream&
SXD::stroke_colour_str( std::ostream& output, const Graphic::colour_type colour )
{
    output << "svg:stroke-color=\""
	   << rgb( Colour::colour_value[colour].red,
		   Colour::colour_value[colour].green,
		   Colour::colour_value[colour].blue )
	   << "\" ";
    return output;
}


std::ostream&
SXD::setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize )
{
    output << "fo:font-size=\"" << fontSize << "\" ";
    return output;
}

std::ostream&
SXD::style_properties_str( std::ostream& output, const unsigned int j )
{
    output << indent( j );
    if ( static_cast<int>(j) >= 0 ) {
	output << "<style:properties ";
    } else {
	output << "</style:properties>";
    }
    return output;
}


Colour::ArrowManip
SXD::arrow_style( const Graphic::arrowhead_type arrow )
{
    return Colour::ArrowManip( arrow_style_str, arrow );
}

BoxManip
SXD::box( const Point& origin, const Point& extent )
{
    return BoxManip( box_str, origin, extent );
}

Colour::ColourManip
SXD::setfill( const Graphic::colour_type colour )
{
    return Colour::ColourManip( setfill_str, colour );
}

Colour::ColourManip
SXD::stroke_colour( const Graphic::colour_type colour )
{
    return Colour::ColourManip( stroke_colour_str, colour );
}

Colour::FontManip
SXD::setfont( const Graphic::font_type aFont )
{
    return Colour::FontManip( setfont_str, aFont, Flags::print[FONT_SIZE].opts.value.i );
}

UnsignedManip
SXD::style_properties( const unsigned int j )
{
    return UnsignedManip( style_properties_str, j );
}

Colour::JustificationManip
SXD::justify( const Justification justification )
{
    return Colour::JustificationManip( justify_str, justification );
}

PointManip
SXD::moveto( const Point& aPoint )
{
    return PointManip( point, aPoint );
}

XMLString::StringManip
SXD::draw_layer( const std::string& name )
{
    return XMLString::StringManip( draw_layer_str, name );
}

XMLString::StringManip
SXD::end_style()
{
    return XMLString::StringManip( end_style_str );
}

XMLString::StringManip
SXD::start_style( const std::string& name, const std::string& family )
{
    return XMLString::StringManip( start_style_str, name, family );
}

#endif

/* -------------------------------------------------------------------- */
/* EEPIC output								*/
/* -------------------------------------------------------------------- */

/*
 * This one is "best effort."
 * We can't fill solids (except circles), and we can't fill and colour a circle.
 */

std::ostream&
TeX::polyline( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::linestyle_type line_style,
		      Graphic::arrowhead_type arrowhead, double scale ) const
{
    const size_t n_points = points.size();
    output << setcolour( pen_colour );
    switch( line_style ) {
    case Graphic::DASHED:
	output << "\\dashline{" << DASH_LENGTH << "}";
	break;
    case Graphic::DASHED_DOTTED:
    case Graphic::DOTTED:
	output << "\\dottedline{" << DOT_GAP << "}";
	break;
    default:
	output << "\\path";
	break;
    }
    for ( unsigned int i = 0; i < n_points; ++i ) {
	output << moveto( points[i] );
    }
    output << std::endl;

    /* Now draw the arrowhead */

    switch ( arrowhead ) {
    case Graphic::CLOSED_ARROW:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, pen_colour );
	break;
    case Graphic::OPEN_ARROW:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, Graphic::WHITE );
	break;
    }

    return output;
}


std::ostream&
TeX::polygon( std::ostream& output, const std::vector<Point>& points,
	      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    const size_t n_points = points.size();
    /* Fill object */
    output  << setfill( fill_colour ) << "\\path";
    for ( unsigned i = 0; i < n_points; ++i ) {
	output << moveto( points[i] );
    }
    output << moveto( points[0] ) << std::endl;
    /* Now draw it */
    if ( pen_colour != Graphic::DEFAULT_COLOUR ) {
	output << setcolour( pen_colour ) << "\\path";
	for ( unsigned i = 0; i < n_points; ++i ) {
	    output << moveto( points[i] );
	}
	output << moveto( points[0] ) << std::endl;
    }
    return output;
}


std::ostream&
TeX::circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour,
	     Graphic::colour_type fill_colour ) const
{
    output << "\\put" << moveto( c ) << "{"
	   << setfill( fill_colour )
	   << "\\circle{" << static_cast<int>(r * 2 + 0.5) << "}"
	   << setcolour( pen_colour )
	   << "\\circle{" << static_cast<int>(r * 2 + 0.5) << "}"
	   << "}" << std::endl;
    return output;
}

std::ostream&
TeX::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::colour_type pen_colour,
		Graphic::colour_type fill_colour, Graphic::linestyle_type line_style ) const
{
    std::vector<Point> points(5);
    box_to_points( origin, extent, points );
    polygon( output, points, pen_colour, fill_colour );
    return output;
}


double
TeX::text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
	   Justification justification, Graphic::colour_type pen_colour, unsigned ) const
{
    output << "\\put" << moveto( c )
	   << "{\\makebox(0,0)" << justify( justification )
	   << "{"
	   << setcolour( pen_colour )
	   << setfont( font )
	   << "{" << s
	   << "}}}" << std::endl;
    return -fontsize * EEPIC_SCALING;
}



/*
 * Print out a point in TeX Format: x y
 */

std::ostream&
TeX::point( std::ostream& output, const Point& aPoint )
{
    output << '(' << static_cast<int>(aPoint.x() + 0.5)
	   << ',' << static_cast<int>(aPoint.y() + 0.5) << ')';
    return output;
}


std::ostream&
TeX::setcolour_str( std::ostream& output, const Graphic::colour_type colour )
{
    output << "\\color{" << colour_name[colour] << "}";
    return output;
}

std::ostream&
TeX::setfill_str( std::ostream& output, const Graphic::colour_type colour )
{
    switch( colour ) {
    case Graphic::TRANSPARENT: break;
    case Graphic::DEFAULT_COLOUR:  output << "\\whiten"; break;
    case Graphic::BLACK:  output << "\\blacken"; break;
    default: output << "\\whiten"; break;		/* Can't shade -- shade only does gray */
    }
    return output;
}

std::ostream&
TeX::justify_str( std::ostream& output, const Justification justification )
{
    switch( justification ) {
    case Justification::RIGHT:   output << "[r]"; break;
    case Justification::LEFT:    output << "[l]"; break;
    }
    return output;
}

std::ostream&
TeX::setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize )
{
    if ( fontSize  <= 5 ) {
	output << "\\tiny";
    } else if ( fontSize <= 7 ) {
	output << "\\scriptsize";
    } else if ( fontSize <= 8 ) {
	output << "\\footnotesize";
    } else if ( fontSize <= 9 ) {
	output << "\\small";
    } else if ( fontSize <= 10 ) {
	output << "\\normalsize";
    } else if ( fontSize <= 12 ) {
	output << "\\large";
    } else if ( fontSize <= 14 ) {
	output << "\\Large";
    } else if ( fontSize <= 18 ) {
	output << "\\LARGE";
    } else if ( fontSize <= 20 ) {
	output << "\\huge";
    } else {
	output << "\\HUGE";
    }
    return output;
}



std::ostream&
TeX::arrowHead( std::ostream& output, const Point& head, const Point& tail, double scaling,
		Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const
{
    const double theta = atan2( tail.y() - head.y(), tail.x() - head.x() );
    const unsigned n_points = 4;

    Point arrow[n_points];
    arrow[0].moveTo( -6.0, -1.5 );
    arrow[1].moveTo( 0.0 ,  0.0 );
    arrow[2].moveTo( -6.0,  1.5 );
    arrow[3].moveTo( -5.0,  0.0 );
    Point origin( 0, 0 );
    output << setfill( fill_colour ) << "\\path";
    for ( unsigned i = 0; i < n_points; ++i ) {
	output << moveto( arrow[i].scaleBy( scaling, scaling ).rotate( origin, theta ).moveBy( tail ) );
    }
    output << moveto( arrow[0] ) << std::endl;
    output << setcolour( pen_colour ) << "\\path";
    for ( unsigned i = 0; i < n_points; ++i ) {
	output << moveto( arrow[i]);
    }
    output << moveto( arrow[0] ) << std::endl;

    return output;
}


PointManip
TeX::moveto( const Point& aPoint )
{
    return PointManip( point, aPoint );
}


Colour::ColourManip
TeX::setcolour( const Graphic::colour_type aColour )
{
    return Colour::ColourManip( TeX::setcolour_str, aColour );
}

Colour::ColourManip
TeX::setfill( const Graphic::colour_type aColour )
{
    return Colour::ColourManip( TeX::setfill_str, aColour );
}

Colour::JustificationManip
TeX::justify( const Justification justification )
{
    return Colour::JustificationManip( justify_str, justification );
}

Colour::FontManip
TeX::setfont( const Graphic::font_type aFont )
{
    return Colour::FontManip( setfont_str, aFont, Flags::print[FONT_SIZE].opts.value.i );
}

Graphic::colour_type
error_colour( double delta )
{
    delta = fabs( delta );

    if ( delta < 5 ) {
	return Graphic::DEFAULT_COLOUR;
    } else if ( delta < 10 ) {
	return Graphic::BLUE;
    } else if ( delta < 25 ) {
	return Graphic::GREEN;
    } else if ( delta < 50 ) {
	return Graphic::ORANGE;
    } else {
	return Graphic::RED;
    }
}
