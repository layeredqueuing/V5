/* graphic.cc	-- Greg Franks Wed Feb 12 2003
 *
 * $Id: graphic.cc 15382 2022-01-25 01:42:07Z greg $
 */

#include <cassert>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <set>
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

const std::map<const Graphic::Colour,const RGB::colour_defn> RGB::__value =
{
    { Graphic::Colour::TRANSPARENT,     {0.000, 0.000, 0.000} },
    { Graphic::Colour::DEFAULT,         {0.000, 0.000, 0.000} },
    { Graphic::Colour::BLACK,           {0.000, 0.000, 0.000} },
    { Graphic::Colour::WHITE,           {1.000, 1.000, 1.000} },
    { Graphic::Colour::GREY_10,         {0.900, 0.900, 0.900} },
    { Graphic::Colour::MAGENTA,         {1.000, 0.000, 1.000} },
    { Graphic::Colour::VIOLET,          {0.500, 0.000, 1.000} },
    { Graphic::Colour::BLUE,            {0.000, 0.000, 1.000} },
    { Graphic::Colour::INDIGO,          {0.000, 0.434, 1.000} },
    { Graphic::Colour::CYAN,            {0.000, 1.000, 1.000} },
    { Graphic::Colour::TURQUOISE,       {0.000, 1.000, 0.500} },
    { Graphic::Colour::GREEN,           {0.000, 1.000, 0.000} },
    { Graphic::Colour::SPRINGGREEN,     {0.500, 1.000, 0.000} },
    { Graphic::Colour::YELLOW,          {1.000, 1.000, 0.000} },
    { Graphic::Colour::ORANGE,          {1.000, 0.500, 0.000} },
    { Graphic::Colour::RED,             {1.000, 0.000, 0.000} },
    { Graphic::Colour::GOLD,            {1.000, 0.840, 0.000} }         
};


const std::map<const Graphic::Colour, const std::string> RGB::__name =
{
    { Graphic::Colour::TRANSPARENT,     "White" },
    { Graphic::Colour::DEFAULT,         "black" },
    { Graphic::Colour::BLACK,           "black" },
    { Graphic::Colour::WHITE,           "White" },
    { Graphic::Colour::GREY_10,         "Grey" },
    { Graphic::Colour::MAGENTA,         "Magenta" },
    { Graphic::Colour::VIOLET,          "Violet" },
    { Graphic::Colour::BLUE,            "Blue" },
    { Graphic::Colour::INDIGO,          "Indigo" },
    { Graphic::Colour::CYAN,            "Cyan" },
    { Graphic::Colour::TURQUOISE,       "Turquoise" },
    { Graphic::Colour::GREEN,           "Green" },
    { Graphic::Colour::SPRINGGREEN,     "SpringGreen" },
    { Graphic::Colour::YELLOW,          "Yellow" },
    { Graphic::Colour::ORANGE,          "Orange" },
    { Graphic::Colour::RED,             "Red" },
    { Graphic::Colour::GOLD,            "Goldenrod" }
};

float
RGB::tint( const float x, const float tint )
{
    assert( 0 <= x && x <= 1.0 && 0 <= tint && tint <= 1.0 );
    return x * (1.0 - tint) + tint;
}

RGBManip
RGB::rgb( float red, float green, float blue )
{
    return RGBManip( RGB::rgb_str, red, green, blue );
}

std::ostream&
RGB::rgb_str( std::ostream& output, float red, float green, float blue )
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
Graphic::Colour EMF::last_pen_colour    = Graphic::Colour::BLACK;
Graphic::Colour EMF::last_fill_colour   = Graphic::Colour::BLACK;
Graphic::Colour EMF::last_arrow_colour  = Graphic::Colour::BLACK;
Graphic::Font EMF::last_font		= Graphic::Font::NORMAL;
Graphic::LineStyle EMF::last_line_style = Graphic::LineStyle::SOLID;
Justification EMF::last_justification   = Justification::CENTER;
int EMF::last_font_size			= 0;

std::ostream&
EMF::init( std::ostream& output, const double xmax, const double ymax, const std::string& command_line )
{
    const double twips_to_mm = .01763888;
    const double twips_to_pixels = twips_to_mm * 5.0;

    std::string aDescription = description( command_line );
    const unsigned n = aDescription.size();

    record_count = 0;

    /* Force redraw */

    last_pen_colour   	= static_cast<Graphic::Colour>(-1);
    last_fill_colour  	= static_cast<Graphic::Colour>(-1);
    last_arrow_colour 	= static_cast<Graphic::Colour>(-1);
    last_justification  = Justification::CENTER;
    last_line_style     = Graphic::LineStyle::SOLID;
    last_font		= Graphic::Font::NORMAL;
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
    output << setfont( Graphic::Font::NORMAL );
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
	       Graphic::Colour pen_colour, Graphic::LineStyle line_style,
	       Graphic::ArrowHead arrowhead, double scale ) const
{
    const unsigned int n_points = points.size();
    if ( n_points > 1 ) {
	output << setcolour( pen_colour );

	switch( line_style ) {
	case Graphic::LineStyle::DASHED:
	case Graphic::LineStyle::DOTTED:
	case Graphic::LineStyle::DASHED_DOTTED:
	    draw_dashed_line( output, line_style, points );
	    break;
	default:
	    draw_line( output, EMR_POLYLINE, points );
	    break;
	}

	/* Now draw the arrowhead */

	switch ( arrowhead ) {
	case Graphic::ArrowHead::CLOSED:
	    arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, pen_colour );
	    break;
	case Graphic::ArrowHead::OPEN:
	    arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, Graphic::Colour::WHITE );
	    break;
	}
    }

    return output;
}


std::ostream&
EMF::polygon( std::ostream& output, const std::vector<Point>& points,
	      Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
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
EMF::draw_dashed_line( std::ostream& output, Graphic::LineStyle line_style, const std::vector<Point>& points ) const
{
    const unsigned n_points = points.size();
    double delta[2];
    double residual = 0.0;		/* Nothing left over */
    int stroke = 1;			/* Start with stroke */

    delta[0] = EMF_SCALING * 2;
    delta[1] = EMF_SCALING * 2;

    switch( line_style ) {
    case Graphic::LineStyle::DASHED:
	delta[1] = DOT_GAP * EMF_SCALING;
	break;
    case Graphic::LineStyle::DOTTED:
    case Graphic::LineStyle::DASHED_DOTTED:
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
EMF::circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour,
	     Graphic::Colour fill_colour ) const
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
EMF::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::Colour pen_colour,
		Graphic::Colour fill_colour, Graphic::LineStyle line_style ) const
{
    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( origin, extent, points );

    output << setcolour( pen_colour ) << setfill( fill_colour );
    switch( line_style ) {
    case Graphic::LineStyle::DASHED:
    case Graphic::LineStyle::DOTTED:
    case Graphic::LineStyle::DASHED_DOTTED:
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
EMF::text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
	   Justification justification, Graphic::Colour colour, unsigned ) const
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

    output << start_record( EMR_SETTEXTCOLOR, 12 ) << rgb( __value.at(colour).red, __value.at(colour).green, __value.at(colour).blue );
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
		Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
{
    if ( last_arrow_colour != fill_colour ) {
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_STOCK_OBJECT_WHITE_BRUSH );
	output << start_record( EMR_DELETEOBJECT, 12 )
	       << writel( EMF_HANDLE_BRUSH );
	output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )
	       << writel( EMF_HANDLE_BRUSH )
	       << writel( 0 )		/* type */
	       << rgb( __value.at(fill_colour).red,
		       __value.at(fill_colour).green,
		       __value.at(fill_colour).blue )
	       << writel( 0 );		/* hatch */
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_HANDLE_BRUSH );
       	last_arrow_colour = fill_colour;
    }
    last_fill_colour = static_cast<Graphic::Colour>(-1);		/* Force redraw */

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
EMF::setcolour_str( std::ostream& output, const Graphic::Colour pen_colour )
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
	       << rgb( __value.at(pen_colour).red, __value.at(pen_colour).green, __value.at(pen_colour).blue );
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
EMF::setfill_str( std::ostream& output, const Graphic::Colour colour )
{
    if ( colour != last_fill_colour ) {
	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_STOCK_OBJECT_WHITE_BRUSH );
	output << start_record( EMR_DELETEOBJECT, 12 )
	       << writel( EMF_HANDLE_BRUSH );
	switch ( colour ) {
	case Graphic::Colour::TRANSPARENT:
	    output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )	/* transparent brush for polygons */
		   << writel( EMF_HANDLE_BRUSH )
		   << writel( 1 )		/* type */
		   << writel( 0 )
		   << writel( 0 );		/* hatch */
	    break;

	case Graphic::Colour::DEFAULT:
	case Graphic::Colour::WHITE:
	    output << start_record( EMR_CREATEBRUSHINDIRECT, 24 )
		   << writel( EMF_HANDLE_BRUSH )
		   << writel( 0 )		/* type */
		   << writel( 0x00ffffff )
		   << writel( 0 );		/* hatch */
	    break;

	case Graphic::Colour::BLACK:
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
		   << rgb( tint( __value.at(colour).red, 0.9 ),
			   tint( __value.at(colour).green, 0.9 ),
			   tint( __value.at(colour).blue, 0.9 ) )
		   << writel( 0 );		/* hatch */
	    break;
	}

	output << start_record( EMR_SELECTOBJECT, 12 )
	       << writel( EMF_HANDLE_BRUSH );

	last_fill_colour = colour;
    }

    last_arrow_colour = static_cast<Graphic::Colour>(-1);		/* Force redraw */
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
EMF::setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize )
{
    if ( font != last_font || fontSize != last_font_size ) {
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
	       << static_cast<char>(font==Graphic::Font::OBLIQUE) /* italic */  /* 32 */
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

	last_font = font;
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

RGB::ColourManip
EMF::setfill( const Graphic::Colour aColour )
{
    return RGB::ColourManip( EMF::setfill_str, aColour );
}

RGB::JustificationManip
EMF::justify( const Justification justification )
{
    return RGB::JustificationManip( justify_str, justification );
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


RGB::ColourManip
EMF::setcolour( const Graphic::Colour aColour )
{
    return RGB::ColourManip( EMF::setcolour_str, aColour );
}

RGB::FontManip
EMF::setfont( const Graphic::Font font )
{
    return RGB::FontManip( setfont_str, font, Flags::font_size() );
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

const std::map<const Graphic::Colour, const int> Fig::__colour = 
{
        { Graphic::Colour::TRANSPARENT,     -1 },
        { Graphic::Colour::DEFAULT,         -1 },
        { Graphic::Colour::BLACK,           0 },
        { Graphic::Colour::WHITE,           7 },
        { Graphic::Colour::GREY_10,         7 },
        { Graphic::Colour::MAGENTA,         5 },
        { Graphic::Colour::VIOLET,          32 },
        { Graphic::Colour::BLUE,            1 },
        { Graphic::Colour::INDIGO,          33 },
        { Graphic::Colour::CYAN,            3 },
        { Graphic::Colour::TURQUOISE,       34 },
        { Graphic::Colour::GREEN,           2 },
        { Graphic::Colour::SPRINGGREEN,     35 },
        { Graphic::Colour::YELLOW,          6 },
        { Graphic::Colour::ORANGE,          36 },
        { Graphic::Colour::RED,             4 },
        { Graphic::Colour::GOLD,            37 }
};

const std::map<const Graphic::LineStyle, const int> Fig::__linestyle =
{
    { Graphic::LineStyle::DEFAULT,	    0 },
    { Graphic::LineStyle::SOLID,	    0 },
    { Graphic::LineStyle::DASHED,	    1 },
    { Graphic::LineStyle::DOTTED,	    2 },
    { Graphic::LineStyle::DASHED_DOTTED,    4 }	
};

const std::map<const Graphic::ArrowHead, const Fig::arrowhead_defn> Fig::__arrowhead =
{
    { Graphic::ArrowHead::NONE,	    {0, 0} },
    { Graphic::ArrowHead::CLOSED,   {2, 1} },
    { Graphic::ArrowHead::OPEN,	    {2, 0} } 
};

const std::map<const Graphic::Font, const int> Fig::__postscript_font =
{
    { Graphic::Font::DEFAULT ,	-1 },
#if 1
    { Graphic::Font::NORMAL,	0 },
    { Graphic::Font::OBLIQUE,	1 },
    { Graphic::Font::BOLD,	2 },
#else
    { Graphic::Font::NORMAL ,	20 },
    { Graphic::Font::OBLIQUE ,	21 },
    { Graphic::Font::BOLD,	22 },
#endif
    { Graphic::Font::SPECIAL,	32 }
};

const std::map<const Graphic::Font, const int> Fig::__tex_font =
{
    { Graphic::Font::DEFAULT,	0 },
    { Graphic::Font::NORMAL,   	1 },
    { Graphic::Font::OBLIQUE,  	3 },
    { Graphic::Font::BOLD,     	2 },
    { Graphic::Font::SPECIAL,   0 }
};

const std::map<const Graphic::Fill, const int> Fig::__fill =
{
    { Graphic::Fill::NONE,    	-1 },
    { Graphic::Fill::DEFAULT,   38 },
    { Graphic::Fill::ZERO,    	0 },
    { Graphic::Fill::NINETY,    18 },
    { Graphic::Fill::SOLID,    	20 },
    { Graphic::Fill::TINT, 	38 }
};

/*
 * Add any extra colours to the palette.
 */

std::ostream&
Fig::initColours( std::ostream& output )
{
    static const std::set<Graphic::Colour> reject = { Graphic::Colour::TRANSPARENT, Graphic::Colour::DEFAULT, Graphic::Colour::BLACK, Graphic::Colour::WHITE };
    
    for ( std::map<const Graphic::Colour, const RGB::colour_defn>::const_iterator i = __value.begin(); i != __value.end(); ++i ) {
	if ( reject.find(i->first) != reject.end() || __colour.at(i->first) < 32 ) continue;
	output << "0 " << __name.at(i->first) << " " << rgb( i->second.red, i->second.green, i->second.blue ) << std::endl;
    }
    return output;
}



std::ostream&
Fig::init( std::ostream& output, int object_code, int sub_type,
	   Graphic::Colour pen_colour, Graphic::Colour fill_colour,
	   Graphic::LineStyle line_style, Graphic::Fill fill_style, int depth ) const
{
    output << object_code 				// int	object_code	(always 2)
	   << ' ' << sub_type 				// int	sub_type	(3: polygon)
	   << ' ' << __linestyle.at(line_style)		// int	line_style	(enumeration type)
	   << ' ';

    /* Pen */
    if ( pen_colour == Graphic::Colour::TRANSPARENT ) {
	output << "0";					// int	thickness	(1/80 inch)
    } else {
	output << "1";					// int	thickness	(1/80 inch)
    }
    output << ' ' << __colour.at(pen_colour);		// int	pen_color	(enumeration type, pen color)

    /* Fill colour */
    if ( fill_colour == Graphic::Colour::DEFAULT ) {
	output << ' ' << __colour.at(Graphic::Colour::WHITE); 	// int	fill_color	(enumeration type, fill color)
    } else {
	output << ' ' << __colour.at(fill_colour); 	// int	fill_color	(enumeration type, fill color)
    }
    output << ' ' << depth				// int	depth		(enumeration type)
	   << " 0 ";					// int	pen_style	(pen style, not used)

    /* Area fill */
    if ( fill_style == Graphic::Fill::DEFAULT ) {
	switch ( fill_colour ) {			// int	area_fill	(enumeration type, -1 = no fill)
	case Graphic::Colour::TRANSPARENT:
	    output << __fill.at(Graphic::Fill::NONE);	// No fill.
	    break;
	case Graphic::Colour::DEFAULT:
	case Graphic::Colour::BLACK:
	    output << __fill.at(Graphic::Fill::SOLID);
	    break;
	case Graphic::Colour::GREY_10:
	    output << __fill.at(Graphic::Fill::NINETY);
	    break;
	default:
	    output << __fill.at(Graphic::Fill::DEFAULT);				// TINT.
	    break;
	}
    } else {
	output << __fill.at(fill_style);
    }

    switch ( line_style ) {
    case Graphic::LineStyle::DASHED:
	output << " " << static_cast<float>(DASH_LENGTH); // float style_val	(dash length/dot gap 1/80 inch)
	break;
    case Graphic::LineStyle::DASHED_DOTTED:
    case Graphic::LineStyle::DOTTED:
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
	       int sub_type, Graphic::Colour pen_colour, Graphic::Colour fill_colour, int depth,
	       Graphic::LineStyle line_style, Graphic::ArrowHead arrowhead, double scaling ) const
{
    const size_t n_points = points.size();
    init( output, 2, sub_type, pen_colour, fill_colour, line_style, Graphic::Fill::DEFAULT, depth );
    output << " 0"					// int	join_style	(enumeration type)
	   << " 0";					// int	cap_style	(enumeration type, only used for POLYLINE)

    if ( sub_type == ARC_BOX ) {
	output << " 12";				// int	radius		(1/80 inch, radius of arc-boxes)
    } else {
	output << " -1";
    }

    switch ( arrowhead ) {
    case Graphic::ArrowHead::CLOSED:
    case Graphic::ArrowHead::OPEN:
	output << " 1"	// int	forward_arrow		(0: off, 1: on)
	       << " 0 "	// int	backward_arrow		(0: off, 1: on)
	       << (sub_type == POLYGON ? n_points + 1 : n_points )
	       << std::endl;
	arrowHead( output, arrowhead, scaling );
	break;
    case Graphic::ArrowHead::NONE:
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
	     int sub_type, Graphic::Colour pen_colour, Graphic::Colour fill_colour, int depth, Graphic::Fill fill_style ) const
{
    Point radius( r, r );
    Point top( c );
    top.moveBy( 0, r );
    init( output, 1, sub_type, pen_colour, fill_colour, Graphic::LineStyle::DEFAULT, fill_style, depth );
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
Fig::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::Colour pen_colour,
		Graphic::Colour fill_colour, int depth, Graphic::LineStyle line_style ) const
{
    std::vector<Point> points(5);
    box_to_points( origin, extent, points );
    return polyline( output, points, BOX, pen_colour, fill_colour, depth, line_style );
}


std::ostream&
Fig::roundedRectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::Colour pen_colour,
		       Graphic::Colour fill_colour, int depth, Graphic::LineStyle line_style ) const
{
    std::vector<Point> points(5);
    box_to_points( origin, extent, points );
    return polyline( output, points, ARC_BOX, pen_colour, fill_colour, depth, line_style );
}


double
Fig::text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
	   Justification justification, Graphic::Colour colour, unsigned flags ) const
{
    int sub_type = 1;
    switch ( justification ) {
    case Justification::LEFT:	 sub_type = 0; break;
    case Justification::RIGHT:	 sub_type = 2; break;
    default:  		 sub_type = 1; break;	/* Center */
    }
    output << "4 "		//int  	object 	     (always 4)
	   << sub_type 		//int	sub_type     (1: Center justified)
	   << " " << __colour.at(colour)	//int	color	     (enumeration type)
	   << " 5"		//int	depth	     (enumeration type)
	   << " 0 ";		//int	pen_style    (enumeration , not used)
    if ( flags & POSTSCRIPT ) {	//int	font 	     (enumeration type)
	output << __postscript_font.at(font);
    } else {
	output << __tex_font.at(font);
    }
    output << " " << fontsize 	//float	font_size    (font size in points)
	   << " 0.0000 "	//float	angle	     (radians, the angle of the text)
	   << flags		//int	font_flags   (bit vector)
	   << " " << static_cast<unsigned int>(fontsize * FIG_SCALING + 0.5)			//float	height	     (Fig units - ignored)
	   << " " << static_cast<unsigned int>(s.length() * fontsize * FIG_SCALING + 0.5)	//float	length	     (Fig units - ignored)
	   << " " << moveto( c )
	   << s
	   << "\\001" << std::endl;

    return fontsize * FIG_SCALING;
}


std::ostream&
Fig::arrowHead( std::ostream& output, Graphic::ArrowHead style, const double scale )
{
    output << '\t' << __arrowhead.at(style).type		// int 	arrow_type	(enumeration type)
	   << ' '  << __arrowhead.at(style).style		// int	arrow_style	(enumeration type)
	   << " 1.00"						// float arrow_thickness(1/80 inch)
	   << " " << std::setprecision(2) << 3.0 * scale	// float arrow_width	(Fig units)
	   << " " << std::setprecision(2) << 6.0 * scale	// float arrow_height	(Fig units)
	   << std::endl;
    return output;
}


std::ostream&
Fig::clearBackground( std::ostream& output, const Point& anOrigin, const Point& anExtent, const Graphic::Colour background_colour ) const
{
    if ( background_colour == Graphic::Colour::TRANSPARENT ) return output;

    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( anOrigin, anExtent, points );
    polyline( output, points, Fig::BOX,
	      Graphic::Colour::TRANSPARENT,		/* PEN */
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
    assert ( 0 <= aPoint.x() && aPoint.x() < std::numeric_limits<int>::max() );
    assert ( 0 <= aPoint.y() && aPoint.y() < std::numeric_limits<int>::max() );
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

std::map<const Graphic::Colour,int> GD::__pen;
std::map<const Graphic::Colour,int> GD::__fill;

/* See http://fontconfig.org/fontconfig-user.html */
std::map<const Graphic::Font,const std::string> GD::__font =
{
    { Graphic::Font::DEFAULT,   "times-12" },
    { Graphic::Font::NORMAL,    "times-12" },
    { Graphic::Font::OBLIQUE,   "times-12:italic" },
    { Graphic::Font::BOLD,      "times-12:bold" },
    { Graphic::Font::SPECIAL,   "times-12" }
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

    __pen[Graphic::Colour::TRANSPARENT] = gdTransparent;
    __fill[Graphic::Colour::TRANSPARENT] = gdTransparent;

    std::map<const Graphic::Colour, const RGB::colour_defn>::const_iterator i;
    i = __value.find(Graphic::Colour::WHITE);
    __pen[Graphic::Colour::WHITE] = gdImageColorAllocate( im, to_byte( i->second.red ), to_byte( i->second.green ), to_byte( i->second.blue) );
    __fill[Graphic::Colour::WHITE] = __pen.at(Graphic::Colour::WHITE);

    i = __value.find(Graphic::Colour::BLACK);
    __pen[Graphic::Colour::BLACK] = gdImageColorAllocate( im, to_byte( i->second.red ), to_byte( i->second.green ), to_byte( i->second.blue) );
    __fill[Graphic::Colour::BLACK] = __pen.at(Graphic::Colour::BLACK);
    __pen[Graphic::Colour::DEFAULT] = __pen.at(Graphic::Colour::BLACK);
    __fill[Graphic::Colour::DEFAULT] = __pen.at(Graphic::Colour::WHITE);

    i = __value.find(Graphic::Colour::GREY_10);
    __pen[Graphic::Colour::GREY_10] = gdImageColorAllocate( im, to_byte( i->second.red ), to_byte( i->second.green ), to_byte( i->second.blue) );
    __fill[Graphic::Colour::GREY_10] = __pen.at(Graphic::Colour::GREY_10);

    for ( std::map<const Graphic::Colour, const RGB::colour_defn>::const_iterator i = __value.begin(); i != __value.end(); ++i ) {
	if ( __pen[i->first] != 0 ) continue;
	__pen[i->first]  = gdImageColorAllocate( im, to_byte( i->second.red ), to_byte( i->second.green ), to_byte( i->second.blue ) );
	__fill[i->first] = gdImageColorAllocate( im, to_byte( tint( i->second.red, 0.9 ) ), to_byte( tint( i->second.green, 0.9 ) ), to_byte( tint( i->second.blue, 0.9 ) ) );
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
GD::polygon( const std::vector<Point>& points, Graphic::Colour pen_colour,
	     Graphic::Colour fill_colour ) const
{
    const unsigned int n_points = points.size();
    gdPoint * gd_points = new gdPoint[n_points];
    for ( unsigned i = 0; i < n_points; ++i ) {
	gd_points[i] = moveto( points[i] );
    }

    gdImageFilledPolygon( im, gd_points, n_points, __fill.at(fill_colour) );
#if HAVE_GDIMAGESETANTIALIASED
    gdImageSetAntiAliased( im, __pen.at(pen_colour) );
    gdImagePolygon( im, gd_points, n_points, gdAntiAliased );
#else
    gdImagePolygon( im, gd_points, n_points, __pen.at(pen_colour) );
#endif
    delete [] gd_points;
    return *this;
}



GD const &
GD::drawline( const Point &p1, const Point &p2, Graphic::Colour pen_colour, Graphic::LineStyle line_style ) const
{
    static int styleDotted[2], styleDashed[6], styleDashedDotted[9];		/* Historical */

    switch ( line_style ) {
    case Graphic::LineStyle::DASHED:
	styleDashed[0] = __pen.at(pen_colour);
	styleDashed[1] = __pen.at(pen_colour);
	styleDashed[2] = __pen.at(pen_colour);
	styleDashed[3] = __pen.at(pen_colour);
	styleDashed[4] = gdTransparent;
	styleDashed[5] = gdTransparent;
	gdImageSetStyle(im, styleDashed, 6);
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), gdStyled );
	break;

    case Graphic::LineStyle::DASHED_DOTTED:
	styleDashedDotted[0] = __pen.at(pen_colour);
	styleDashedDotted[1] = __pen.at(pen_colour);
	styleDashedDotted[2] = __pen.at(pen_colour);
	styleDashedDotted[3] = __pen.at(pen_colour);
	styleDashedDotted[4] = gdTransparent;
	styleDashedDotted[5] = __pen.at(pen_colour);
	styleDashedDotted[6] = gdTransparent;
	styleDashedDotted[7] = __pen.at(pen_colour);
	styleDashedDotted[8] = gdTransparent;
	gdImageSetStyle(im, styleDashedDotted, 9);
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), gdStyled );

    case Graphic::LineStyle::DOTTED:
	styleDotted[0] = __pen.at(pen_colour);
	styleDotted[1] = gdTransparent;
	gdImageSetStyle(im, styleDotted, 2);
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), gdStyled );
	break;

    default:
#if HAVE_GDIMAGESETANTIALIASED
	gdImageSetAntiAliased( im, __pen.at(pen_colour));
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), gdAntiAliased );
#else
	gdImageLine( im, static_cast<int>(p1.x()), static_cast<int>(p1.y()),
		     static_cast<int>(p2.x()), static_cast<int>(p2.y()), __pen.at(pen_colour) );
#endif
	break;
    }
    return *this;
}

GD const &
GD::circle( const Point& c, const double r, Graphic::Colour pen_colour,
	    Graphic::Colour fill_colour ) const
{
    const int d = static_cast<int>(r * 2.0);
#if HAVE_GDIMAGEFILLEDARC
    gdImageFilledArc( im, static_cast<int>(c.x()), static_cast<int>(c.y()), d, d, 0, 360,
		      __fill.at(fill_colour), gdArc );
#endif
#if HAVE_GDIMAGESETANTIALIASED
    gdImageSetAntiAliased( im, __pen.at(pen_colour) );
    gdImageArc( im, static_cast<int>(c.x()), static_cast<int>(c.y()), d, d, 0, 360, gdAntiAliased );
#else
    gdImageArc( im, static_cast<int>(c.x()), static_cast<int>(c.y()), d, d, 0, 360, __pen.at(pen_colour) );
#endif

    return *this;
}


GD const&
GD::rectangle( const Point& origin, const Point& extent, Graphic::Colour pen_colour,
	       Graphic::Colour fill_colour, Graphic::LineStyle line_style ) const
{
    std::vector<Point> points(5);
    box_to_points( origin, extent, points );
    gdPoint p1 = moveto( points[0] );
    gdPoint p2 = moveto( points[2] );
    gdImageFilledRectangle( im, p1.x, p1.y, p2.x, p2.y, __fill.at(fill_colour) );
    switch( line_style ) {
    case Graphic::LineStyle::DASHED:
    case Graphic::LineStyle::DASHED_DOTTED:
    case Graphic::LineStyle::DOTTED:
	for ( unsigned i = 1; i < points.size(); ++i ) {
	    drawline( points[i-1], points[i], pen_colour, line_style );
	}
	break;
    default:
#if HAVE_GDIMAGESETANTIALIASED
	gdImageSetAntiAliased( im, __pen.at(pen_colour) );
	gdImageRectangle( im, p1.x, p1.y, p2.x, p2.y, gdAntiAliased );
#else
	gdImageRectangle( im, p1.x, p1.y, p2.x, p2.y, __pen.at(pen_colour) );
#endif
	break;
    }
    return *this;
}


double
GD::text( const Point& p1, const std::string& aStr, Graphic::Font font, int fontsize, Justification justification,
	  Graphic::Colour pen_colour ) const
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

	gdImageStringFT( im, brect,__pen.at(pen_colour),
			 const_cast<char *>(__font.at(font).data()),
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
		       __pen.at(pen_colour) );


	return -f->h;
#if HAVE_GDFTUSEFONTCONFIG
    }
#endif
}



gdFont *
GD::getfont() const
{
    if ( Flags::font_size() <= 10 ) {
	return gdFontTiny;
    } else if ( Flags::font_size() <= 14 ) {
	return gdFontSmall;
    } else if ( Flags::font_size() <= 18 ) {
	return gdFontLarge;
    } else {
	return gdFontGiant;
    }
}



unsigned
GD::width( const std::string& aStr, Graphic::Font font, int fontsize ) const
{
#if HAVE_GDFTUSEFONTCONFIG
    if ( haveTTF ) {
	int brect[8];

	/* Compute bounding box */

	gdImageStringFT( 0, brect, __pen.at(Graphic::Colour::DEFAULT),
			 const_cast<char *>(__font.at(font).data()),
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
	       const Graphic::Colour pen_colour, const Graphic::Colour fill_colour ) const
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

    gdImageFilledPolygon( im, arrow, n_points, __pen.at(fill_colour) );
#if HAVE_GDIMAGESETANTIALIASED
    gdImageSetAntiAliased( im, __pen.at(pen_colour) );
    gdImagePolygon( im, arrow, n_points, gdAntiAliased );
#else
    gdImagePolygon( im, arrow, n_points, __pen.at(pen_colour) );
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

const std::map<const Graphic::Font, const std::string> PostScript::__font =
{
    { Graphic::Font::DEFAULT,       "/Times-Roman" },
    { Graphic::Font::NORMAL,        "/Times-Roman" },
    { Graphic::Font::OBLIQUE,       "/Times-Italic" },
    { Graphic::Font::BOLD,          "/Times-Bold" },
    { Graphic::Font::SPECIAL,        "/Symbol" }
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
		      Graphic::Colour pen_colour, Graphic::LineStyle line_style,
		      Graphic::ArrowHead arrowhead, double scale ) const
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
    case Graphic::ArrowHead::CLOSED:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, pen_colour );
	break;
    case Graphic::ArrowHead::OPEN:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, Graphic::Colour::WHITE );
	break;
    }

    output << "grestore" << std::endl;
    return output;
}


std::ostream&
PostScript::polygon( std::ostream& output, const std::vector<Point>& points,
		     Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
{
    const size_t n_points = points.size();
    output << linestyle( Graphic::LineStyle::SOLID )
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
PostScript::circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
{
    output << linestyle( Graphic::LineStyle::SOLID )
	   << "gsave newpath "
	   << moveto( c ) << r << " 0 360 arc "
	   << "closepath " << std::endl
	   << setfill( fill_colour )
	   << stroke( pen_colour )
	   << "grestore" << std::endl;
    return output;
}

std::ostream&
PostScript::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::Colour pen_colour,
		       Graphic::Colour fill_colour, Graphic::LineStyle line_style ) const
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
PostScript::roundedRectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::Colour pen_colour,
			      Graphic::Colour fill_colour, Graphic::LineStyle line_style ) const
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
//	   << setfill( Graphic::Colour::TRANSPARENT ) << std::endl
	   << setfill( fill_colour ) << std::endl
	   << stroke( pen_colour )
	   << "grestore" << std::endl;

    return output;
}


double
PostScript::text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
		  Justification justification, Graphic::Colour colour, unsigned ) const
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
PostScript::linestyle_str( std::ostream& output, Graphic::LineStyle aStyle )
{
//    if ( myOldLineStyle == aStyle ) return output;
//    myOldLineStyle = style;

    switch( aStyle ) {
    default:
	output << "[] 0";
	break;
    case Graphic::LineStyle::DASHED:
	output << "[" << DASH_LENGTH << "] 0";
	break;
    case Graphic::LineStyle::DASHED_DOTTED:
	output << "[" << DASH_LENGTH  << " " << DOT_GAP  << " 1 " << DOT_GAP << "] 0";
	break;
    case Graphic::LineStyle::DOTTED:
	output << "[1 " << DOT_GAP <<"] " << DOT_GAP;
	break;
    }

    output << " setdash " << std::endl;
    return output;
}


std::ostream&
PostScript::setcolour_str( std::ostream& output, Graphic::Colour colour )
{
//    if ( myOldColour == colour ) return output;
//    myOldColour = colour;

    output << __value.at(colour).red << ' '
	   << __value.at(colour).green << ' '
	   << __value.at(colour).blue << ' '
	   << "setrgbcolor ";
    return output;
}


std::ostream&
PostScript::setfill_str( std::ostream& output, Graphic::Colour aColour )
{
    output << "  gsave ";
    if ( aColour == Graphic::Colour::TRANSPARENT ) {
	output << "0 setgray ";			// Black.
    } else {
	switch ( aColour ) {
	case Graphic::Colour::DEFAULT:	output << "1 setgray"; break;			// White.
	case Graphic::Colour::BLACK:  	output << "0 setgray"; break;			// Black.
	case Graphic::Colour::GREY_10:	output << "0 setgray 0.9 tnt"; break;		// Black.
	default:		      	output << setcolour( aColour ) << " 0.90 tnt";
	}
	output << " eofill ";
    }
    output << "grestore ";
    return output;
}

std::ostream&
PostScript::setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize )
{
    output << __font.at(font) << " findfont ["
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
PostScript::stroke_str( std::ostream& output, Graphic::Colour aColour )
{
    output << "  gsave " << setcolour( aColour ) << "stroke grestore ";
    return output;
}


std::ostream&
PostScript::arrowHead(std::ostream& output, const Point& src, const Point& dst, const double scale,
		      const Graphic::Colour pen, const Graphic::Colour fill ) const
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

RGB::ColourManip
PostScript::setcolour( const Graphic::Colour aColour )
{
    return RGB::ColourManip( PostScript::setcolour_str, aColour );
}

RGB::ColourManip
PostScript::stroke( const Graphic::Colour aColour )
{
    return RGB::ColourManip( PostScript::stroke_str, aColour );
}

RGB::ColourManip
PostScript::setfill( const Graphic::Colour aColour )
{
    return RGB::ColourManip( PostScript::setfill_str, aColour );
}

PointManip
PostScript::moveto( const Point& aPoint )
{
    return PointManip( point, aPoint );
}


RGB::FontManip
PostScript::setfont( const Graphic::Font font )
{
    return RGB::FontManip( setfont_str, font, Flags::font_size() );
}

RGB::JustificationManip
PostScript::justify( const Justification justification )
{
    return RGB::JustificationManip( justify_str, justification );
}


RGB::LineStyleManip
PostScript::linestyle( const Graphic::LineStyle aStyle )
{
    return RGB::LineStyleManip( PostScript::linestyle_str, aStyle );
}

/* -------------------------------------------------------------------- */
/* Scalable Vector Grahpics Ouptut					*/
/* -------------------------------------------------------------------- */

const std::map<const Graphic::Font, const SVG::font_defn> SVG::__font =
{
    { Graphic::Font::DEFAULT,       { "Times", "Roman" } },
    { Graphic::Font::NORMAL,        { "Times", "Roman" } },
    { Graphic::Font::OBLIQUE,       { "Times", "Italic" } },
    { Graphic::Font::BOLD,          { "Times", "Bold" } },
    { Graphic::Font::SPECIAL,       { "Times", "Roman" } }
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
		      Graphic::Colour pen_colour, Graphic::LineStyle line_style,
		      Graphic::ArrowHead arrowhead, double scale ) const
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
    case Graphic::ArrowHead::CLOSED:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, pen_colour );
	break;
    case Graphic::ArrowHead::OPEN:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, Graphic::Colour::WHITE );
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
	      Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
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
SVG::circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
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
SVG::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::Colour pen_colour,
		Graphic::Colour fill_colour, Graphic::LineStyle line_style ) const
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
SVG::text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
	   Justification justification, Graphic::Colour colour, unsigned ) const
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
SVG::linestyle_str( std::ostream& output, Graphic::LineStyle aStyle )
{
    switch( aStyle ) {
    case Graphic::LineStyle::DASHED:
	output << " stroke-dasharray=\""
	       << DASH_LENGTH * SVG_SCALING << ","
	       << DOT_GAP * SVG_SCALING
	       << "\"";
	break;
    case Graphic::LineStyle::DASHED_DOTTED:
	output << " stroke-dasharray=\""
	       << DASH_LENGTH * SVG_SCALING << ","
	       << DOT_GAP * SVG_SCALING << ","
	       << SVG_SCALING << ","
	       << DOT_GAP * SVG_SCALING 
	       << "\"";

    case Graphic::LineStyle::DOTTED:
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
SVG::setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize )
{
    output << " font-family=\""  << __font.at(font).family
	   << "\" font-style=\"" << __font.at(font).style
	   << "\" font-size=\""  << fontSize * SVG_SCALING << "\"";
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
SVG::setcolour_str( std::ostream& output, Graphic::Colour colour )
{
    output << __name.at(colour);
    return output;
}


/*
 * Tint all colours except black and white.   Default fill colour is white.  Default Pen colour is black.
 */

std::ostream&
SVG::setfill_str( std::ostream& output, Graphic::Colour aColour )
{
    output << " fill=\"";
    switch ( aColour ) {
    case Graphic::Colour::TRANSPARENT:
	output << "none";
	break;

    case Graphic::Colour::BLACK:
	output << rgb( 0, 0, 0 );
	break;

    case Graphic::Colour::WHITE:
    case Graphic::Colour::DEFAULT:
	output << rgb( 1.0, 1.0, 1.0 );
	break;

    default:
	output << rgb( tint( __value.at(aColour).red, 0.9 ),
		       tint( __value.at(aColour).green, 0.9 ),
		       tint( __value.at(aColour).blue, 0.9 ) );
	break;
    }
    output << "\"";
    return output;
}

std::ostream&
SVG::stroke_str( std::ostream& output, Graphic::Colour aColour )
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
		const Graphic::Colour pen_colour, const Graphic::Colour fill ) const
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
	   << stroke_colour( Graphic::Colour::BLACK )
	   << " draw:marker-start-width=\"0.3cm\" draw:marker-start-center=\"false\" draw:marker-end-width=\"0.3cm\" draw:marker-end-center=\"false\" draw:fill=\"solid\" draw:fill-color=\"#00b8ff\" draw:shadow=\"hidden\" draw:shadow-offset-x=\"0.3cm\" draw:shadow-offset-y=\"0.3cm\" draw:shadow-color=\"#808080\" fo:margin-left=\"0cm\" fo:margin-right=\"0cm\" fo:margin-top=\"0cm\" fo:margin-bottom=\"0cm\" style:use-window-font-color=\"true\" style:text-outline=\"false\" style:text-crossing-out=\"none\" fo:font-family=\"&apos;Times New Roman&apos;\" style:font-family-generic=\"roman\" style:font-pitch=\"variable\" fo:font-size=\"24pt\" fo:font-style=\"normal\" fo:text-shadow=\"none\" style:text-underline=\"none\" fo:font-weight=\"normal\" style:font-family-asian=\"Gothic\" style:font-pitch-asian=\"variable\" style:font-size-asian=\"24pt\" style:font-style-asian=\"normal\" style:font-weight-asian=\"normal\" style:font-family-complex=\"Lucidasans\" style:font-pitch-complex=\"variable\" style:font-size-complex=\"24pt\" style:font-style-complex=\"normal\" style:font-weight-complex=\"normal\" style:font-relief=\"none\" fo:line-height=\"100%\" text:enable-numbering=\"false\">" << std::endl;
    output << style_properties( ~0 ) << std::endl;
    output << end_style() << std::endl;
    output << start_style( "objectwitharrow", graphics_family ) << std::endl;
    output << style_properties( 0 ) << "draw:stroke=\"solid\" svg:stroke-width=\"0.5pt\" "
	   << stroke_colour( Graphic::Colour::BLACK ) << "draw:marker-start=\"Arrow\" draw:marker-start-width=\"0.7cm\" draw:marker-start-center=\"true\" draw:marker-end-width=\"0.3cm\"/>" << std::endl;
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
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::TRANSPARENT ) << setfill( Graphic::Colour::TRANSPARENT )
	   << justify( Justification::CENTER ) << "draw:auto-grow-width=\"true\" draw:auto-grow-height=\"true\" fo:min-height=\"0cm\" fo:min-width=\"0cm\"/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr2", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::TRANSPARENT ) << setfill( Graphic::Colour::TRANSPARENT )
	   << justify( Justification::LEFT ) << "draw:auto-grow-width=\"true\" draw:auto-grow-height=\"true\" fo:min-height=\"0cm\" fo:min-width=\"0cm\"/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr3", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::TRANSPARENT ) << setfill( Graphic::Colour::TRANSPARENT )
	   << justify( Justification::RIGHT ) << "draw:auto-grow-width=\"true\" draw:auto-grow-height=\"true\" fo:min-height=\"0cm\" fo:min-width=\"0cm\"/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr4", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLACK ) << setfill( Graphic::Colour::WHITE )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr5", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLACK ) << setfill( Graphic::Colour::BLACK )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr6", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLUE )  << setfill( Graphic::Colour::BLUE )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr7", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::GREEN ) << setfill( Graphic::Colour::GREEN )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr8", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::ORANGE )<< setfill( Graphic::Colour::ORANGE )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr9", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::RED )   << setfill( Graphic::Colour::RED )
	   << justify( Justification::CENTER ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr10", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLACK ) << setfill( Graphic::Colour::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr11", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLUE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr12", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::GREEN ) << setfill( Graphic::Colour::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr13", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::ORANGE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr14", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::RED ) << setfill( Graphic::Colour::TRANSPARENT )
	   << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr15", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLACK ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr16", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLUE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr17", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::GREEN ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr18", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::ORANGE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr19", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::RED ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr20", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLACK ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::OPEN ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr21", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLUE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::OPEN ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr22", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::GREEN ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::OPEN ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr23", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::ORANGE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::OPEN ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr24", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::RED ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::OPEN ) << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr25", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLACK ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr26", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLUE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr27", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::GREEN ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr28", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::ORANGE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr29", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::RED ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dashed << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "gr30", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLACK ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr31", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::BLUE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr32", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::GREEN ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr33", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::ORANGE ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << start_style( "gr34", graphics_family ) << std::endl
	   << style_properties( 0 ) << stroke_colour( Graphic::Colour::RED ) << setfill( Graphic::Colour::TRANSPARENT )
	   << arrow_style( Graphic::ArrowHead::CLOSED ) << draw_dotted << "/>" << std::endl
	   << end_style() << std::endl;
    output << std::endl;

    output << start_style( "P1", paragraph_family ) << std::endl
           << style_properties( 0 ) << setfont( Graphic::Font::DEFAULT ) << "fo:text-align=\"center\"/>" << std::endl
           << end_style() << std::endl;
    output << start_style( "P2", paragraph_family ) << std::endl
           << style_properties( 0 ) << text_style << setfont( Graphic::Font::DEFAULT ) << " fo:text-align=\"center\"/>" << std::endl
           << end_style() << std::endl;
    output << start_style( "P3", paragraph_family ) << std::endl
           << style_properties( 0 ) << text_style << setfont( Graphic::Font::DEFAULT ) << " fo:text-align=\"left\"  />" << std::endl
           << end_style() << std::endl;
    output << start_style( "P4", paragraph_family ) << std::endl
           << style_properties( 0 ) << text_style << setfont( Graphic::Font::DEFAULT ) << " fo:text-align=\"right\" />" << std::endl
           << end_style() << std::endl;
    output << start_style( "P5", paragraph_family ) << std::endl
           << style_properties( 0 ) << setfont( Graphic::Font::DEFAULT ) << "/>" << std::endl
           << end_style() << std::endl;
    output << std::endl;
    output << start_style( "T1", text_family ) << std::endl
           << style_properties( 0 ) << setfont( Graphic::Font::DEFAULT ) << "/>" << std::endl
           << end_style() << std::endl;
    output << indent( -1 ) << "</office:automatic-styles>" << std::endl;
    return output;
}


std::ostream&
SXD::init( std::ostream& output, const char * object_name,
	   Graphic::Colour pen_colour, Graphic::Colour fill_colour,
	   Graphic::LineStyle line_style, Graphic::ArrowHead arrow ) const
{
    output << indent( 0 ) << "<draw:" << object_name << " draw:style-name=\"";
    if ( fill_colour != Graphic::Colour::TRANSPARENT ) {
	switch( fill_colour ) {
	default:			output << "gr4"; break;
	case Graphic::Colour::BLACK: 	output << "gr5"; break;
	case Graphic::Colour::BLUE: 	output << "gr6"; break;
	case Graphic::Colour::GREEN: 	output << "gr7"; break;
	case Graphic::Colour::ORANGE: 	output << "gr8"; break;
	case Graphic::Colour::RED: 	output << "gr9"; break;
	}
    } else switch ( line_style ) {
    default:
	switch ( arrow ) {
	default:
	    switch ( pen_colour ) {
	    default:  		            output << "gr10"; break;
	    case Graphic::Colour::BLUE:     output << "gr11"; break;
	    case Graphic::Colour::GREEN:    output << "gr12"; break;
	    case Graphic::Colour::ORANGE:   output << "gr13"; break;
	    case Graphic::Colour::RED:      output << "gr14"; break;
	    }
	    break;

	case Graphic::ArrowHead::CLOSED:
	    switch ( pen_colour ) {
	    default:              	    output << "gr15"; break;
	    case Graphic::Colour::BLUE:     output << "gr16"; break;
	    case Graphic::Colour::GREEN:    output << "gr17"; break;
	    case Graphic::Colour::ORANGE:   output << "gr18"; break;
	    case Graphic::Colour::RED:      output << "gr19"; break;
	    }
	    break;

	case Graphic::ArrowHead::OPEN:
	    switch ( pen_colour ) {
	    default:                	    output << "gr20"; break;
	    case Graphic::Colour::BLUE:     output << "gr21"; break;
	    case Graphic::Colour::GREEN:    output << "gr22"; break;
	    case Graphic::Colour::ORANGE:   output << "gr23"; break;
	    case Graphic::Colour::RED:      output << "gr24"; break;
	    }
	    break;

	}
	break;

    case Graphic::LineStyle::DASHED:		/* Forwarding uses closed arrows. */
	switch ( pen_colour ) {
	default:                	output << "gr25"; break;
	case Graphic::Colour::BLUE:     output << "gr26"; break;
	case Graphic::Colour::GREEN:    output << "gr27"; break;
	case Graphic::Colour::ORANGE:   output << "gr28"; break;
	case Graphic::Colour::RED:      output << "gr29"; break;
	}
	break;

    case Graphic::LineStyle::DASHED_DOTTED:
    case Graphic::LineStyle::DOTTED:	/* Replies use closed arrows. */
	switch ( pen_colour ) {
	default:               		output << "gr30"; break;
	case Graphic::Colour::BLUE:     output << "gr31"; break;
	case Graphic::Colour::GREEN:    output << "gr32"; break;
	case Graphic::Colour::ORANGE:   output << "gr33"; break;
	case Graphic::Colour::RED:      output << "gr34"; break;
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
		      Graphic::Colour pen_colour, Graphic::LineStyle line_style,
		      Graphic::ArrowHead arrowhead, double scale ) const
{
    const size_t n_points = points.size();
    /* OpenOffice doesn't like polylines of 2 points */
    if ( n_points == 2 ) {
	init( output, "line", pen_colour, Graphic::Colour::TRANSPARENT, line_style, arrowhead );
	output << temp_indent( 1 )
	       << "svg:x1=\"" << points[0].x() << "cm\" "
	       << "svg:y1=\"" << points[0].y() << "cm\" "
	       << "svg:x2=\"" << points[1].x() << "cm\" "
	       << "svg:y2=\"" << points[1].y() << "cm\"/>" << std::endl;
    } else {
	init( output, "polyline", pen_colour, Graphic::Colour::TRANSPARENT, line_style, arrowhead );
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
	      Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
{
    init( output, "polygon", pen_colour, fill_colour, Graphic::LineStyle::SOLID, Graphic::ArrowHead::NONE );
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
SXD::circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
{
    init( output, "circle", pen_colour, fill_colour, Graphic::LineStyle::SOLID );
    Point origin = c;
    Point extent( r, r );
    origin -= extent;
    extent *= 2;
    output << temp_indent(1) << box( origin, extent ) << "/>" << std::endl;
    return output;
}

std::ostream&
SXD::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::Colour pen_colour,
		Graphic::Colour fill_colour, Graphic::LineStyle line_style ) const
{
    const unsigned n_points = 5;
    std::vector<Point> points(n_points);
    box_to_points( origin, extent, points );
    init( output, "polygon", pen_colour, fill_colour, line_style, Graphic::ArrowHead::NONE );
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
SXD::text( std::ostream& output, const Point&, const std::string& s, Graphic::Font font, int fontsize,
	   Justification justification, Graphic::Colour colour, unsigned ) const
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
SXD::arrow_style_str( std::ostream& output, Graphic::ArrowHead arrowhead )
{
    switch ( arrowhead ) {
    case Graphic::ArrowHead::CLOSED:
	output << "draw:marker-end=\"Arrow concave\" draw:marker-end-width=\"0.2cm\" ";
	break;
    case Graphic::ArrowHead::OPEN:
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
SXD::setfill_str( std::ostream& output, const Graphic::Colour colour )
{
    switch ( colour ) {
    case Graphic::Colour::TRANSPARENT:
	output << "draw:fill=\"none\" ";
	break;

    case Graphic::Colour::WHITE:
	output << "draw:fill-color=\"" << rgb( __value.at(colour).red,
					       __value.at(colour).green,
					       __value.at(colour).blue ) << "\" ";
	break;

    case Graphic::Colour::BLACK:
	output << "draw:fill-color=\"" << rgb( __value.at(colour).red,
					       __value.at(colour).green,
					       __value.at(colour).blue ) << "\" ";
	break;

    default:
	output << "draw:fill-color=\"" << rgb( tint( __value.at(colour).red,   0.9 ),
					       tint( __value.at(colour).green, 0.9 ),
					       tint( __value.at(colour).blue,  0.9 ) ) << "\" ";
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
SXD::stroke_colour_str( std::ostream& output, const Graphic::Colour colour )
{
    output << "svg:stroke-color=\""
	   << rgb( __value.at(colour).red,
		   __value.at(colour).green,
		   __value.at(colour).blue )
	   << "\" ";
    return output;
}


std::ostream&
SXD::setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize )
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


RGB::ArrowManip
SXD::arrow_style( const Graphic::ArrowHead arrow )
{
    return RGB::ArrowManip( arrow_style_str, arrow );
}

BoxManip
SXD::box( const Point& origin, const Point& extent )
{
    return BoxManip( box_str, origin, extent );
}

RGB::ColourManip
SXD::setfill( const Graphic::Colour colour )
{
    return RGB::ColourManip( setfill_str, colour );
}

RGB::ColourManip
SXD::stroke_colour( const Graphic::Colour colour )
{
    return RGB::ColourManip( stroke_colour_str, colour );
}

RGB::FontManip
SXD::setfont( const Graphic::Font font )
{
    return RGB::FontManip( setfont_str, font, Flags::font_size() );
}

UnsignedManip
SXD::style_properties( const unsigned int j )
{
    return UnsignedManip( style_properties_str, j );
}

RGB::JustificationManip
SXD::justify( const Justification justification )
{
    return RGB::JustificationManip( justify_str, justification );
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
		      Graphic::Colour pen_colour, Graphic::LineStyle line_style,
		      Graphic::ArrowHead arrowhead, double scale ) const
{
    const size_t n_points = points.size();
    output << setcolour( pen_colour );
    switch( line_style ) {
    case Graphic::LineStyle::DASHED:
	output << "\\dashline{" << DASH_LENGTH << "}";
	break;
    case Graphic::LineStyle::DASHED_DOTTED:
    case Graphic::LineStyle::DOTTED:
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
    case Graphic::ArrowHead::CLOSED:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, pen_colour );
	break;
    case Graphic::ArrowHead::OPEN:
	arrowHead( output, points[n_points-2], points[n_points-1], scale, pen_colour, Graphic::Colour::WHITE );
	break;
    }

    return output;
}


std::ostream&
TeX::polygon( std::ostream& output, const std::vector<Point>& points,
	      Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
{
    const size_t n_points = points.size();
    /* Fill object */
    output  << setfill( fill_colour ) << "\\path";
    for ( unsigned i = 0; i < n_points; ++i ) {
	output << moveto( points[i] );
    }
    output << moveto( points[0] ) << std::endl;
    /* Now draw it */
    if ( pen_colour != Graphic::Colour::DEFAULT ) {
	output << setcolour( pen_colour ) << "\\path";
	for ( unsigned i = 0; i < n_points; ++i ) {
	    output << moveto( points[i] );
	}
	output << moveto( points[0] ) << std::endl;
    }
    return output;
}


std::ostream&
TeX::circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour,
	     Graphic::Colour fill_colour ) const
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
TeX::rectangle( std::ostream& output, const Point& origin, const Point& extent, Graphic::Colour pen_colour,
		Graphic::Colour fill_colour, Graphic::LineStyle line_style ) const
{
    std::vector<Point> points(5);
    box_to_points( origin, extent, points );
    polygon( output, points, pen_colour, fill_colour );
    return output;
}


double
TeX::text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
	   Justification justification, Graphic::Colour pen_colour, unsigned ) const
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
TeX::setcolour_str( std::ostream& output, const Graphic::Colour colour )
{
    output << "\\color{" << __name.at(colour) << "}";
    return output;
}

std::ostream&
TeX::setfill_str( std::ostream& output, const Graphic::Colour colour )
{
    switch( colour ) {
    case Graphic::Colour::TRANSPARENT: break;
    case Graphic::Colour::DEFAULT:  output << "\\whiten"; break;
    case Graphic::Colour::BLACK:  output << "\\blacken"; break;
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
TeX::setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize )
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
		Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const
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


RGB::ColourManip
TeX::setcolour( const Graphic::Colour aColour )
{
    return RGB::ColourManip( TeX::setcolour_str, aColour );
}

RGB::ColourManip
TeX::setfill( const Graphic::Colour aColour )
{
    return RGB::ColourManip( TeX::setfill_str, aColour );
}

RGB::JustificationManip
TeX::justify( const Justification justification )
{
    return RGB::JustificationManip( justify_str, justification );
}

RGB::FontManip
TeX::setfont( const Graphic::Font font )
{
    return RGB::FontManip( setfont_str, font, Flags::font_size() );
}

Graphic::Colour
error_colour( double delta )
{
    delta = fabs( delta );

    if ( delta < 5 ) {
	return Graphic::Colour::DEFAULT;
    } else if ( delta < 10 ) {
	return Graphic::Colour::BLUE;
    } else if ( delta < 25 ) {
	return Graphic::Colour::GREEN;
    } else if ( delta < 50 ) {
	return Graphic::Colour::ORANGE;
    } else {
	return Graphic::Colour::RED;
    }
}
