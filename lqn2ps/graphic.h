/* -*- c++ -*-
 * graphic.h	-- Greg Franks
 *
 * $Id: graphic.h 15170 2021-12-07 23:33:05Z greg $
 */

#ifndef _GRAPHIC_H
#define _GRAPHIC_H

#include "lqn2ps.h"
#include <vector>
#if HAVE_GD_H
#include <gd.h>
#endif

class Label;
class Point;
class PointManip;
class BoxManip;
class RGBManip;
class ArrowManip;

class LongManip {
public:
    LongManip( std::ostream& (*ff)(std::ostream&, const unsigned long ), const unsigned long aLong )
	: f(ff), myLong(aLong) {}
private:
    std::ostream& (*f)( std::ostream&, const unsigned long );
    const unsigned long myLong;

    friend std::ostream& operator<<(std::ostream & os, const LongManip& m )
	{ return m.f(os,m.myLong); }
};

class ShortManip {
public:
    ShortManip( std::ostream& (*ff)(std::ostream&, const unsigned short ), const unsigned short aShort )
	: f(ff), myShort(aShort) {}
private:
    std::ostream& (*f)( std::ostream&, const unsigned short );
    const unsigned short myShort;

    friend std::ostream& operator<<(std::ostream & os, const ShortManip& m )
	{ return m.f(os,m.myShort); }
};

class PointManip
{
public:
    PointManip( std::ostream& (*ff)(std::ostream&, const Point& aPoint ),
		const Point& aPoint )
	: f(ff), myPoint(aPoint) {}
private:
    std::ostream& (*f)( std::ostream&, const Point& aPoint );
    const Point& myPoint;

    friend std::ostream& operator<<(std::ostream & os, const PointManip& m )
	{ return m.f( os, m.myPoint ); }
};

class BoxManip
{
public:
    BoxManip( std::ostream& (*ff)(std::ostream&, const Point& origin, const Point& extent ),
		const Point& origin, const Point& extent )
	: f(ff), myOrigin(origin), myExtent(extent) {}
private:
    std::ostream& (*f)( std::ostream&, const Point& origin, const Point& extent );
    const Point& myOrigin;
    const Point& myExtent;

    friend std::ostream& operator<<(std::ostream & os, const BoxManip& m )
	{ return m.f( os, m.myOrigin, m.myExtent ); }
};

class RGBManip
{
public:
    RGBManip( std::ostream& (*ff)(std::ostream&, const float red, const float green, const float blue ),
		const float red, const float green, const float blue )
	: f(ff), myRed(red), myGreen(green), myBlue(blue) {}
private:
    std::ostream& (*f)( std::ostream&, const float red, const float green, const float blue );
    const float myRed;
    const float myGreen;
    const float myBlue;

    friend std::ostream& operator<<(std::ostream & os, const RGBManip& m )
	{ return m.f( os, m.myRed, m.myGreen, m.myBlue ); }
};

class XMLString
{
public:
    class StringManip {
    public:
	StringManip( std::ostream& (*ff)(std::ostream&, const std::string&, const std::string& ) )
	    : f(ff), myS1(nullStr), myS2(nullStr) {}
	StringManip( std::ostream& (*ff)(std::ostream&, const std::string&, const std::string& ), const std::string& s1 )
	    : f(ff), myS1(s1), myS2(nullStr) {}
	StringManip( std::ostream& (*ff)(std::ostream&, const std::string&, const std::string& ), const std::string& s1, const std::string& s2 )
	    : f(ff), myS1(s1), myS2(s2) {}
    private:
	std::ostream& (*f)( std::ostream&, const std::string&, const std::string& );
	const std::string& myS1;
	const std::string& myS2;
	static const std::string nullStr;

	friend std::ostream& operator<<(std::ostream & os, const StringManip& m )
	    { return m.f(os,m.myS1,m.myS2); }
    };

    static StringManip xml_escape( const std::string& s ) { return StringManip( xml_escape_str, s ); }

private:
    static std::ostream& xml_escape_str( std::ostream& output, const std::string&, const std::string& );
};


class Graphic
{
public:
    typedef enum { TRANSPARENT,
		   DEFAULT_COLOUR,
		   BLACK,
		   WHITE,
		   GREY_10,
		   MAGENTA,
		   VIOLET,
		   BLUE,
		   INDIGO,
		   CYAN,
		   TURQUOISE,
		   GREEN,
		   SPRINGGREEN,
		   YELLOW,
		   ORANGE,
		   RED,
		   GOLD } colour_type;
    typedef enum { DEFAULT_LINESTYLE, SOLID, DASHED, DOTTED, DASHED_DOTTED } linestyle_type;
    typedef enum { NO_ARROW, CLOSED_ARROW, OPEN_ARROW } arrowhead_type;
    typedef enum { NO_FILL, DEFAULT_FILL, FILL_ZERO, FILL_90, FILL_SOLID, FILL_TINT } fill_type;
    typedef enum { DEFAULT_FONT, NORMAL_FONT, OBLIQUE_FONT, BOLD_FONT, SYMBOL_FONT } font_type;

private:
    Graphic( const Graphic& );
    Graphic& operator=( const Graphic& );

public:
    explicit Graphic( colour_type pen_colour = DEFAULT_COLOUR, colour_type fill_colour = DEFAULT_COLOUR, linestyle_type ls = DEFAULT_LINESTYLE )
	: myPenColour(pen_colour), myFillColour(fill_colour), myFillStyle(FILL_TINT), myLinestyle(ls), myDepth(0)
	{}
    virtual ~Graphic() {}

    Graphic& penColour( colour_type colour ) { myPenColour = colour; return *this; }
    colour_type penColour() const { return myPenColour; }
    Graphic& fillColour( colour_type colour ) { myFillColour = colour; return *this; }
    colour_type fillColour() const { return myFillColour; }
    Graphic& linestyle( linestyle_type aLinestyle ) { myLinestyle = aLinestyle; return *this; }
    linestyle_type linestyle() const { return  myLinestyle; }
    Graphic& depth( const unsigned aDepth )  { myDepth = aDepth; return *this; }
    unsigned depth() const { return myDepth; }
    Graphic& fillStyle( const fill_type fill ) { myFillStyle = fill; return *this; }
    fill_type fillStyle() const { return myFillStyle; }

    static bool intersects( const Point&, const Point&, const Point&, const Point& );
    static bool intersects( const Point&, const Point&, const Point& );

    virtual std::ostream& comment( std::ostream& output, const std::string& ) const = 0;

private:
    colour_type myPenColour;
    colour_type myFillColour;
    fill_type myFillStyle;
    linestyle_type myLinestyle;
    int myDepth;
};

class Colour
{
public:
    typedef struct
    {
	float red;
	float green;
	float blue;
    } colour_defn;

protected:
class ColourManip
{
public:
    ColourManip( std::ostream& (*ff)(std::ostream&, const Graphic::colour_type aColour ),
		const Graphic::colour_type aColour )
	: f(ff), myColour(aColour) {}
private:
    std::ostream& (*f)( std::ostream&, const Graphic::colour_type aColour );
    const Graphic::colour_type myColour;

    friend std::ostream& operator<<(std::ostream & os, const ColourManip& m )
	{ return m.f( os, m.myColour ); }
};

class FontManip
{
public:
    FontManip( std::ostream& (*ff)(std::ostream&, const Graphic::font_type aFont, const int size ),
		const Graphic::font_type aFont, const int size )
	: f(ff), myFont(aFont), mySize(size) {}
private:
    std::ostream& (*f)( std::ostream&, const Graphic::font_type aFont, const int size );
    const Graphic::font_type myFont;
    const int mySize;


    friend std::ostream& operator<<(std::ostream & os, const FontManip& m )
	{ return m.f( os, m.myFont, m.mySize ); }
};

class LineStyleManip
{
public:
    LineStyleManip( std::ostream& (*ff)(std::ostream&, const Graphic::linestyle_type linestyle ),
		const Graphic::linestyle_type linestyle )
	: f(ff), myLineStyle(linestyle) {}
private:
    std::ostream& (*f)( std::ostream&, const Graphic::linestyle_type linestyle );
    const Graphic::linestyle_type myLineStyle;

    friend std::ostream& operator<<(std::ostream & os, const LineStyleManip& m )
	{ return m.f( os, m.myLineStyle ); }
};

class JustificationManip
{
public:
    JustificationManip( std::ostream& (*ff)(std::ostream&, const Justification justification ),
		const Justification justification )
	: f(ff), j(justification) {}
private:
    std::ostream& (*f)( std::ostream&, const Justification justification );
    const Justification j;

    friend std::ostream& operator<<(std::ostream & os, const JustificationManip& m )
	{ return m.f( os, m.j ); }
};


class ArrowManip
{
public:
    ArrowManip( std::ostream& (*ff)(std::ostream&, const Graphic::arrowhead_type arrow ),
		const Graphic::arrowhead_type arrow )
	: f(ff), myArrow(arrow) {}
private:
    std::ostream& (*f)( std::ostream&, const Graphic::arrowhead_type arrow );
    const Graphic::arrowhead_type myArrow;

    friend std::ostream& operator<<(std::ostream & os, const ArrowManip& m )
	{ return m.f( os, m.myArrow ); }
};

protected:
    static float tint( const float, const float );
    static int to_byte( const float x ) { return static_cast<int>( x * 255.0 ); }
    static RGBManip rgb( const float, const float, const float );

    static std::ostream& rgb_str( std::ostream& output, const float, const float, const float );

    static colour_defn colour_value[];
    static const char * colour_name[];
};

#if defined(EMF_OUTPUT)
class EMF : private Colour
{
public:
    static std::ostream& init( std::ostream&, const double, const double, const std::string& );
    static std::ostream& terminate( std::ostream& );

protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
		 Justification justification, Graphic::colour_type colour, unsigned=0 ) const;
    static DoubleManip writef( const double );
    static LongManip writel( const unsigned long );
    static ShortManip writes( const unsigned short );
    static Integer2Manip start_record( const int, const int );
    static Integer2Manip writep( const int, const int );
    static StringManip2 writestr( const std::string& );

private:
    typedef enum { EMF_HANDLE_PEN=1, EMF_HANDLE_FONT=2, EMF_HANDLE_BRUSH=3, EMF_HANDLE_MAX=4 } emf_handle_type;
    typedef enum { EMR_NOP, EMR_HEADER, EMR_POLYBEZIER, EMR_POLYGON, EMR_POLYLINE,
		   EMR_POLYBEZIERTO, EMR_POLYLINETO, EMR_POLYPOLYLINE, EMR_POLYPOLYGON, EMR_SETWINDOWEXTEX,
		   EMR_SETWINDOWORGEX, EMR_SETVIEWPORTEXTEX, EMR_SETVIEWPORTORGEX, EMR_SETBRUSHORGEX, EMR_EOF,
		   EMR_SETPIXELV, EMR_SETMAPPERFLAGS, EMR_SETMAPMODE, EMR_SETBKMODE, EMR_SETPOLYFILLMODE,
		   EMR_SETROP2, EMR_SETSTRETCHBLTMODE, EMR_SETTEXTALIGN, EMR_SETCOLORADJUSTMENT, EMR_SETTEXTCOLOR,
		   EMR_SETBKCOLOR, EMR_OFFSETCLIPRGN, EMR_MOVETOEX, EMR_SETMETARGN, EMR_EXCLUDECLIPRECT,
		   EMR_INTERSECTCLIPRECT, EMR_SCALEVIEWPORTEXTEX, EMR_SCALEWINDOWEXTEX, EMR_SAVEDC, EMR_RESTOREDC,
		   EMR_SETWORLDTRANSFORM, EMR_MODIFYWORLDTRANSFORM, EMR_SELECTOBJECT, EMR_CREATEPEN, EMR_CREATEBRUSHINDIRECT,
		   EMR_DELETEOBJECT, EMR_ANGLEARC, EMR_ELLIPSE, EMR_RECTANGLE, EMR_ROUNDRECT,
		   EMR_ARC, EMR_CHORD, EMR_PIE, EMR_SELECTPALETTE, EMR_CREATEPALETTE,
		   EMR_SETPALETTEENTRIES, EMR_RESIZEPALETTE, EMR_REALIZEPALETTE, EMR_EXTFLOODFILL, EMR_LINETO,
		   EMR_ARCTO, EMR_POLYDRAW, EMR_SETARCDIRECTION, EMR_SETMITERLIMIT, EMR_BEGINPATH,
		   EMR_ENDPATH, EMR_CLOSEFIGURE, EMR_FILLPATH, EMR_STROKEANDFILLPATH, EMR_STROKEPATH,
		   EMR_FLATTENPATH, EMR_WIDENPATH, EMR_SELECTCLIPPATH, EMR_ABORTPATH, EMR_UNUSED1,
		   EMR_GDICOMMENT, EMR_FILLRGN, EMR_FRAMERGN, EMR_INVERTRGN, EMR_PAINTRGN,
		   EMR_EXTSELECTCLIPRGN, EMR_BITBLT, EMR_STRETCHBLT, EMR_MASKBLT, EMR_PLGBLT,
		   EMR_SETDIBITSTODEVICE, EMR_STRETCHDIBITS, EMR_EXTCREATEFONTINDIRECTW, EMR_EXTTEXTOUTA, EMR_EXTTEXTOUTW,
		   EMR_POLYBEZIER16, EMR_POLYGON16, EMR_POLYLINE16, EMR_POLYBEZIERTO16, EMR_POLYLINETO16,
		   EMR_POLYPOLYLINE16, EMR_POLYPOLYGON16, EMR_POLYDRAW16, EMR_CREATEMONOBRUSH, EMR_CREATEDIBPATTERNBRUSHPT,
		   EMR_EXTCREATEPEN, EMR_POLYTEXTOUTA, EMR_POLYTEXTOUTW
    } extended_meta_record;
    typedef enum { PS_SOLID=0, PS_DASH=1, PS_DOT=2, PS_DASHDOT=3, PS_DASHDOTDOT=4, PS_NULL=5, PS_INSIDEFRAME=6, PS_USERSTYLE=7 } pen_style;

    static const unsigned long EMF_STOCK_OBJECT_FLAG		= 0x80000000L;
    static const unsigned long EMF_STOCK_OBJECT_WHITE_BRUSH 	= (EMF_STOCK_OBJECT_FLAG + 0x00);
    static const unsigned long EMF_STOCK_OBJECT_BLACK_PEN  	= (EMF_STOCK_OBJECT_FLAG + 0x07);
    static const unsigned long EMF_STOCK_OBJECT_DEFAULT_FONT    = (EMF_STOCK_OBJECT_FLAG + 0x0A);
    static const unsigned long EMF_PX_SCALING			= 20;

    static std::string description( const std::string& );
    static std::ostream& printUnicode( std::ostream& output, const std::string& aString );
    std::ostream& draw_line( std::ostream& output, extended_meta_record, const std::vector<Point>& ) const;
    std::ostream& draw_dashed_line( std::ostream& output, Graphic::linestyle_type line_style, const std::vector<Point>& ) const;
    static ColourManip setfill( const Graphic::colour_type );
    static JustificationManip justify( const Justification );
    static PointManip lineto( const Point& );
    static PointManip moveto( const Point& );
    static PointManip point( const Point& );
    static RGBManip rgb( const float, const float, const float );
    static ColourManip setcolour( const Graphic::colour_type );
    static FontManip setfont( const Graphic::font_type aFont );
    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&, const double scale,
			const Graphic::colour_type, const Graphic::colour_type ) const;


    static std::ostream& setfill_str( std::ostream& output, Graphic::colour_type aColour );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static std::ostream& lineto_str( std::ostream& output, const Point& aPoint );
    static std::ostream& moveto_str( std::ostream& output, const Point& aPoint );
    static std::ostream& point_str( std::ostream& output, const Point& aPoint );
    static std::ostream& rgb_str( std::ostream& output, const float, const float, const float );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize );
    static std::ostream& start_record_str( std::ostream& output, const int, const int );
    static std::ostream& writef_str( std::ostream& output, const double );
    static std::ostream& writel_str( std::ostream& output, const unsigned long );
    static std::ostream& writep_str( std::ostream& output, const int, const int );
    static std::ostream& writes_str( std::ostream& output, const unsigned short );
    static std::ostream& writestr_str( std::ostream& output, const std::string& aString );

private:
    static long record_count;
    static Graphic::colour_type last_pen_colour;
    static Graphic::colour_type last_fill_colour;
    static Graphic::colour_type last_arrow_colour;
    static Justification last_justification;
    static Graphic::font_type last_font;
    static Graphic::linestyle_type last_line_style;
    static int last_font_size;
};
#endif

class Fig : private Colour
{
    friend class Model;

public:
    static std::ostream& initColours( std::ostream& );

protected:
    typedef enum {RIDGID=0x1, SPECIAL=0x02, POSTSCRIPT=0x04, HIDDEN=0x08} font_flags;
    typedef enum {POLYLINE=1, BOX=2, POLYGON=3, ARC_BOX=4 } sub_type_flag;

    std::ostream& startCompound( std::ostream& output, const Point& origin, const Point& extent ) const;
    std::ostream& endCompound( std::ostream& output ) const;
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points,
		       int sub_type, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE,
		       Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1.0 ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, int sub_type,
		     Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth, Graphic::fill_type fill_style=Graphic::DEFAULT_FILL ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth, Graphic::linestyle_type line_style=Graphic::DEFAULT_LINESTYLE ) const;
    std::ostream& roundedRectangle( std::ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth, Graphic::linestyle_type line_style=Graphic::DEFAULT_LINESTYLE ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
		 Justification, Graphic::colour_type colour, unsigned flags=0 ) const;
    std::ostream& clearBackground( std::ostream& output, const Point&, const Point&, const Graphic::colour_type ) const;

private:
    typedef struct
    {
	int arrow_type;
	int arrow_style;
    } arrowhead_defn;

    std::ostream& init( std::ostream& output, int object_code, int sub_type,
		   Graphic::colour_type pen_colour, Graphic::colour_type fill_colour,
		   Graphic::linestyle_type line_style, Graphic::fill_type, int depth ) const;
    static PointManip moveto( const Point& );
    static std::ostream& arrowHead( std::ostream& output, Graphic::arrowhead_type, const double scale );
    static std::ostream& point( std::ostream& output, const Point& aPoint );

    static int colour_index[];
    static int linestyle_value[];
    static int postscript_font_value[];
    static int tex_font_value[];
    static int fill_value[];
    static arrowhead_defn arrowhead_value[];
};

#if HAVE_GD_H && HAVE_LIBGD
bool operator==( const gdPoint&, const gdPoint& );
inline bool operator!=( const gdPoint& p1, gdPoint& p2 ) { return !( p1 == p2 ); }
gdPoint operator-( const gdPoint&, const gdPoint& );
gdPoint operator+( const gdPoint&, const gdPoint& );

class GD : private Colour
{
public:
    static void create( int x, int y );
    static void destroy();
    static void testForTTF();
#if HAVE_GDIMAGEGIFPTR
    static std::ostream& outputGIF( std::ostream& );
#endif
#if HAVE_LIBPNG
    static std::ostream& outputPNG( std::ostream& );
#endif
#if HAVE_LIBJPEG
    static std::ostream& outputJPG( std::ostream& );
#endif

protected:
    static gdPoint moveto( const Point& );

    GD const & polygon( const std::vector<Point>& points, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    GD const & drawline( const Point& p1, const Point& p2, Graphic::colour_type pen_colour, Graphic::linestyle_type linestyle ) const;
    GD const & rectangle( const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    GD const & circle( const Point& center, const double d, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( const Point& p1, const std::string&, Graphic::font_type, int, Justification, Graphic::colour_type fill_colour ) const;
    GD const & arrowHead( const Point&, const Point&, const double scaling, const Graphic::colour_type pen_colour, const Graphic::colour_type fill_colour) const;

    gdFont * getfont() const;
    unsigned width( const std::string &, Graphic::font_type font, int fontsize ) const;

protected:
    static gdImagePtr im;
    static bool haveTTF;

private:
    enum BRECT { LLx, LLy, LRx, LRy, URx, URy, ULx, ULy };

    static std::vector<int> pen_value;
    static std::vector<int> fill_value;
    static const char * font_value[];
};
#endif

class PostScript : private Colour
{
public:
    static std::ostream& init( std::ostream& );

protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    std::ostream& roundedRectangle( std::ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
		 Justification justification, Graphic::colour_type colour, unsigned=0 ) const;

private:
    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&, const double scale,
			const Graphic::colour_type, const Graphic::colour_type ) const;

    static ColourManip setcolour( const Graphic::colour_type );
    static ColourManip stroke( const Graphic::colour_type );
    static ColourManip setfill( const Graphic::colour_type );
    static PointManip moveto( const Point& );
    static FontManip setfont( const Graphic::font_type aFont );
    static LineStyleManip linestyle( const Graphic::linestyle_type );
    static JustificationManip justify( const Justification );

    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setfill_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static std::ostream& linestyle_str( std::ostream& output, Graphic::linestyle_type style );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize );
    static std::ostream& stroke_str( std::ostream& output, const Graphic::colour_type );
    static const char * font_value[];
};

#if defined(SVG_OUTPUT)
class SVG : private Colour, private XMLString
{
    typedef struct
    {
	const char * family;
	const char * style;
    } font_defn;

public:
    static std::ostream& init( std::ostream& );

protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
		 Justification justification, Graphic::colour_type colour, unsigned=0 ) const;

private:
    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&, const double scale,
			const Graphic::colour_type, const Graphic::colour_type ) const;

    static ColourManip setfill( const Graphic::colour_type aColour ) { return ColourManip( SVG::setfill_str, aColour ); }
    static ColourManip setcolour( const Graphic::colour_type aColour ) { return ColourManip( SVG::setcolour_str, aColour ); }
    static ColourManip stroke( const Graphic::colour_type aColour ) { return ColourManip( SVG::stroke_str, aColour ); }
    static FontManip setfont( const Graphic::font_type aFont ) { return FontManip( setfont_str, aFont, Flags::print[FONT_SIZE].opts.value.i ); }
    static JustificationManip justify( const Justification justification ) { return JustificationManip( justify_str, justification ); }
    static LineStyleManip linestyle( const Graphic::linestyle_type aStyle ) { return LineStyleManip( SVG::linestyle_str, aStyle ); }
    static PointManip moveto( const Point& aPoint ) { return PointManip( point, aPoint ); }

    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setfill_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static std::ostream& linestyle_str( std::ostream& output, Graphic::linestyle_type style );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize );
    static std::ostream& stroke_str( std::ostream& output, const Graphic::colour_type );
    static colour_defn fill_value[];
    static font_defn font_value[];
};
#endif

#if defined(SXD_OUTPUT)
class SXD : private Colour, private XMLString
{
public:
    static std::ostream& init( std::ostream& );
    static std::ostream& printStyles( std::ostream& output );

protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;

    std::ostream& begin_paragraph( std::ostream& output, const Point&, const Point&, const Justification ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
		 Justification justification, Graphic::colour_type colour, unsigned=0 ) const;
    std::ostream& end_paragraph( std::ostream& output ) const;

private:
    std::ostream& init( std::ostream& output, const char * object_name,
		   Graphic::colour_type pen_colour, Graphic::colour_type fill_colour,
		   Graphic::linestyle_type line_style, Graphic::arrowhead_type=Graphic::NO_ARROW ) const;
    std::ostream& drawline( std::ostream& output, const std::vector<Point>& points ) const;

    static ArrowManip arrow_style( const Graphic::arrowhead_type );
    static BoxManip box( const Point&, const Point& );
    static ColourManip setfill( const Graphic::colour_type );
    static ColourManip stroke_colour( const Graphic::colour_type );
    static FontManip setfont( const Graphic::font_type aFont );
    static UnsignedManip style_properties( const unsigned int );
    static JustificationManip justify( const Justification );
    static PointManip moveto( const Point& );
    static XMLString::StringManip draw_layer( const std::string& );
    static XMLString::StringManip end_style();
    static XMLString::StringManip start_style( const std::string&, const std::string& );

    static std::ostream& arrow_style_str( std::ostream& output, const Graphic::arrowhead_type );
    static std::ostream& box_str( std::ostream& output, const Point& origin, const Point& extent );
    static std::ostream& draw_layer_str( std::ostream& output, const std::string&, const std::string& );
    static std::ostream& end_style_str( std::ostream& output, const std::string&, const std::string& );
    static std::ostream& setfill_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize );
    static std::ostream& start_style_str( std::ostream& output, const std::string&, const std::string& );
    static std::ostream& stroke_colour_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& style_properties_str( std::ostream& output, const unsigned int );

private:
    static const char * graphics_family;
    static const char * paragraph_family;
    static const char * text_family;
};
#endif

class TeX : private Colour
{
protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
		 Justification justification, Graphic::colour_type colour, unsigned=0 ) const;

private:
    static PointManip moveto( const Point& );
    static ColourManip setcolour( const Graphic::colour_type );
    static ColourManip setfill( const Graphic::colour_type );
    static FontManip setfont( const Graphic::font_type aFont );
    static JustificationManip justify( const Justification );

    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&, const double scale,
			const Graphic::colour_type, const Graphic::colour_type ) const;

    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& setfill_str( std::ostream& output, Graphic::colour_type aColour );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize );
    static std::ostream& justify_str( std::ostream& output, const Justification );
};

#if defined(X11_OUTPUT)
class X11
{
protected:
#if 0
    static std::ostream& init( std::ostream& output ) { return output; }
    static ColourManip setcolour( const Graphic::colour_type );
    static ColourManip stroke( const Graphic::colour_type );
    static ColourManip setfill( const Graphic::colour_type );
    static PointManip moveto( const Point& );
    static FontManip setfont( const Graphic::font_type aFont );
    static JustificationManip justify( const Justification );
#endif

    std::ostream& lineStyle( std::ostream& output, Graphic::linestyle_type style ) const { return output; }
    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&,
	const Graphic::colour_type, const Graphic::colour_type ) const { return output; }

private:
#if 0
    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& stroke_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& setfill_str( std::ostream& output, const Graphic::colour_type );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::font_type aFont, const int fontSize );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static const char * font_value[];
#endif
};
#endif


Graphic::colour_type error_colour( double delta );
#endif
