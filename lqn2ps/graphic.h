/* -*- c++ -*-
 * graphic.h	-- Greg Franks
 *
 * $Id: graphic.h 13477 2020-02-08 23:14:37Z greg $
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
    LongManip( ostream& (*ff)(ostream&, const unsigned long ), const unsigned long aLong )
	: f(ff), myLong(aLong) {}
private:
    ostream& (*f)( ostream&, const unsigned long );
    const unsigned long myLong;

    friend ostream& operator<<(ostream & os, const LongManip& m )
	{ return m.f(os,m.myLong); }
};

class ShortManip {
public:
    ShortManip( ostream& (*ff)(ostream&, const unsigned short ), const unsigned short aShort )
	: f(ff), myShort(aShort) {}
private:
    ostream& (*f)( ostream&, const unsigned short );
    const unsigned short myShort;

    friend ostream& operator<<(ostream & os, const ShortManip& m )
	{ return m.f(os,m.myShort); }
};

class PointManip
{
public:
    PointManip( ostream& (*ff)(ostream&, const Point& aPoint ),
		const Point& aPoint )
	: f(ff), myPoint(aPoint) {}
private:
    ostream& (*f)( ostream&, const Point& aPoint );
    const Point& myPoint;

    friend ostream& operator<<(ostream & os, const PointManip& m )
	{ return m.f( os, m.myPoint ); }
};

class BoxManip
{
public:
    BoxManip( ostream& (*ff)(ostream&, const Point& origin, const Point& extent ),
		const Point& origin, const Point& extent )
	: f(ff), myOrigin(origin), myExtent(extent) {}
private:
    ostream& (*f)( ostream&, const Point& origin, const Point& extent );
    const Point& myOrigin;
    const Point& myExtent;

    friend ostream& operator<<(ostream & os, const BoxManip& m )
	{ return m.f( os, m.myOrigin, m.myExtent ); }
};

class RGBManip
{
public:
    RGBManip( ostream& (*ff)(ostream&, const float red, const float green, const float blue ),
		const float red, const float green, const float blue )
	: f(ff), myRed(red), myGreen(green), myBlue(blue) {}
private:
    ostream& (*f)( ostream&, const float red, const float green, const float blue );
    const float myRed;
    const float myGreen;
    const float myBlue;

    friend ostream& operator<<(ostream & os, const RGBManip& m )
	{ return m.f( os, m.myRed, m.myGreen, m.myBlue ); }
};

class XMLString
{
public:
    class StringManip {
    public:
	StringManip( ostream& (*ff)(ostream&, const std::string&, const std::string& ) )
	    : f(ff), myS1(nullStr), myS2(nullStr) {}
	StringManip( ostream& (*ff)(ostream&, const std::string&, const std::string& ), const std::string& s1 )
	    : f(ff), myS1(s1), myS2(nullStr) {}
	StringManip( ostream& (*ff)(ostream&, const std::string&, const std::string& ), const std::string& s1, const std::string& s2 )
	    : f(ff), myS1(s1), myS2(s2) {}
    private:
	ostream& (*f)( ostream&, const std::string&, const std::string& );
	const std::string& myS1;
	const std::string& myS2;
	static const std::string nullStr;

	friend ostream& operator<<(ostream & os, const StringManip& m )
	    { return m.f(os,m.myS1,m.myS2); }
    };

    static StringManip xml_escape( const std::string& s ) { return StringManip( xml_escape_str, s ); }

private:
    static ostream& xml_escape_str( ostream& output, const std::string&, const std::string& );
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

    virtual ostream& comment( ostream& output, const string& ) const = 0;

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
    ColourManip( ostream& (*ff)(ostream&, const Graphic::colour_type aColour ),
		const Graphic::colour_type aColour )
	: f(ff), myColour(aColour) {}
private:
    ostream& (*f)( ostream&, const Graphic::colour_type aColour );
    const Graphic::colour_type myColour;

    friend ostream& operator<<(ostream & os, const ColourManip& m )
	{ return m.f( os, m.myColour ); }
};

class FontManip
{
public:
    FontManip( ostream& (*ff)(ostream&, const Graphic::font_type aFont, const int size ),
		const Graphic::font_type aFont, const int size )
	: f(ff), myFont(aFont), mySize(size) {}
private:
    ostream& (*f)( ostream&, const Graphic::font_type aFont, const int size );
    const Graphic::font_type myFont;
    const int mySize;


    friend ostream& operator<<(ostream & os, const FontManip& m )
	{ return m.f( os, m.myFont, m.mySize ); }
};

class LineStyleManip
{
public:
    LineStyleManip( ostream& (*ff)(ostream&, const Graphic::linestyle_type linestyle ),
		const Graphic::linestyle_type linestyle )
	: f(ff), myLineStyle(linestyle) {}
private:
    ostream& (*f)( ostream&, const Graphic::linestyle_type linestyle );
    const Graphic::linestyle_type myLineStyle;

    friend ostream& operator<<(ostream & os, const LineStyleManip& m )
	{ return m.f( os, m.myLineStyle ); }
};

class JustificationManip
{
public:
    JustificationManip( ostream& (*ff)(ostream&, const justification_type justification ),
		const justification_type justification )
	: f(ff), myJustification(justification) {}
private:
    ostream& (*f)( ostream&, const justification_type justification );
    const justification_type myJustification;

    friend ostream& operator<<(ostream & os, const JustificationManip& m )
	{ return m.f( os, m.myJustification ); }
};


class ArrowManip
{
public:
    ArrowManip( ostream& (*ff)(ostream&, const Graphic::arrowhead_type arrow ),
		const Graphic::arrowhead_type arrow )
	: f(ff), myArrow(arrow) {}
private:
    ostream& (*f)( ostream&, const Graphic::arrowhead_type arrow );
    const Graphic::arrowhead_type myArrow;

    friend ostream& operator<<(ostream & os, const ArrowManip& m )
	{ return m.f( os, m.myArrow ); }
};

protected:
    static float tint( const float, const float );
    static int to_byte( const float x ) { return static_cast<int>( x * 255.0 ); }
    static RGBManip rgb( const float, const float, const float );

    static ostream& rgb_str( ostream& output, const float, const float, const float );

    static colour_defn colour_value[];
    static const char * colour_name[];
};

#if defined(EMF_OUTPUT)
class EMF : private Colour
{
public:
    static ostream& init( ostream&, const double, const double, const string& );
    static ostream& terminate( ostream& );

protected:
    ostream& polyline( ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    ostream& polygon( ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    ostream& rectangle( ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    ostream& circle( ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( ostream& output, const Point& c, const string& s, Graphic::font_type font, int fontsize,
		 justification_type justification, Graphic::colour_type colour, unsigned=0 ) const;
    static DoubleManip writef( const double );
    static LongManip writel( const unsigned long );
    static ShortManip writes( const unsigned short );
    static Integer2Manip start_record( const int, const int );
    static Integer2Manip writep( const int, const int );
    static StringManip2 writestr( const string& );

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

    static string description( const string& );
    static ostream& printUnicode( ostream& output, const string& aString );
    ostream& draw_line( ostream& output, extended_meta_record, const std::vector<Point>& ) const;
    ostream& draw_dashed_line( ostream& output, Graphic::linestyle_type line_style, const std::vector<Point>& ) const;
    static ColourManip setfill( const Graphic::colour_type );
    static JustificationManip justify( const justification_type );
    static PointManip lineto( const Point& );
    static PointManip moveto( const Point& );
    static PointManip point( const Point& );
    static RGBManip rgb( const float, const float, const float );
    static ColourManip setcolour( const Graphic::colour_type );
    static FontManip setfont( const Graphic::font_type aFont );
    ostream& arrowHead( ostream& output, const Point&, const Point&, const double scale,
			const Graphic::colour_type, const Graphic::colour_type ) const;


    static ostream& setfill_str( ostream& output, Graphic::colour_type aColour );
    static ostream& justify_str( ostream& output, const justification_type );
    static ostream& lineto_str( ostream& output, const Point& aPoint );
    static ostream& moveto_str( ostream& output, const Point& aPoint );
    static ostream& point_str( ostream& output, const Point& aPoint );
    static ostream& rgb_str( ostream& output, const float, const float, const float );
    static ostream& setcolour_str( ostream& output, const Graphic::colour_type );
    static ostream& setfont_str( ostream& output, const Graphic::font_type aFont, const int fontSize );
    static ostream& start_record_str( ostream& output, const int, const int );
    static ostream& writef_str( ostream& output, const double );
    static ostream& writel_str( ostream& output, const unsigned long );
    static ostream& writep_str( ostream& output, const int, const int );
    static ostream& writes_str( ostream& output, const unsigned short );
    static ostream& writestr_str( ostream& output, const string& aString );

private:
    static long record_count;
    static Graphic::colour_type last_pen_colour;
    static Graphic::colour_type last_fill_colour;
    static Graphic::colour_type last_arrow_colour;
    static justification_type last_justification;
    static Graphic::font_type last_font;
    static Graphic::linestyle_type last_line_style;
    static int last_font_size;
};
#endif

class Fig : private Colour
{
    friend class Model;

public:
    static ostream& initColours( ostream& );

protected:
    typedef enum {RIDGID=0x1, SPECIAL=0x02, POSTSCRIPT=0x04, HIDDEN=0x08} font_flags;
    typedef enum {POLYLINE=1, BOX=2, POLYGON=3, ARC_BOX=4 } sub_type_flag;

    ostream& startCompound( ostream& output, const Point& origin, const Point& extent ) const;
    ostream& endCompound( ostream& output ) const;
    ostream& polyline( ostream& output, const std::vector<Point>& points,
		       int sub_type, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE,
		       Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1.0 ) const;
    ostream& circle( ostream& output, const Point& c, const double r, int sub_type,
		     Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth, Graphic::fill_type fill_style=Graphic::DEFAULT_FILL ) const;
    ostream& rectangle( ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth, Graphic::linestyle_type line_style=Graphic::DEFAULT_LINESTYLE ) const;
    ostream& roundedRectangle( ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, int depth, Graphic::linestyle_type line_style=Graphic::DEFAULT_LINESTYLE ) const;
    double text( ostream& output, const Point& c, const string& s, Graphic::font_type font, int fontsize,
		 justification_type, Graphic::colour_type colour, unsigned flags=0 ) const;
    ostream& clearBackground( ostream& output, const Point&, const Point&, const Graphic::colour_type ) const;

private:
    typedef struct
    {
	int arrow_type;
	int arrow_style;
    } arrowhead_defn;

    ostream& init( ostream& output, int object_code, int sub_type,
		   Graphic::colour_type pen_colour, Graphic::colour_type fill_colour,
		   Graphic::linestyle_type line_style, Graphic::fill_type, int depth ) const;
    static PointManip moveto( const Point& );
    static ostream& arrowHead( ostream& output, Graphic::arrowhead_type, const double scale );
    static ostream& point( ostream& output, const Point& aPoint );

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
    static ostream& outputGIF( ostream& );
#endif
#if HAVE_LIBPNG
    static ostream& outputPNG( ostream& );
#endif
#if HAVE_LIBJPEG
    static ostream& outputJPG( ostream& );
#endif

protected:
    static gdPoint moveto( const Point& );

    GD const & polygon( const std::vector<Point>& points, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    GD const & drawline( const Point& p1, const Point& p2, Graphic::colour_type pen_colour, Graphic::linestyle_type linestyle ) const;
    GD const & rectangle( const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    GD const & circle( const Point& center, const double d, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( const Point& p1, const string&, Graphic::font_type, int, justification_type, Graphic::colour_type fill_colour ) const;
    GD const & arrowHead( const Point&, const Point&, const double scaling, const Graphic::colour_type pen_colour, const Graphic::colour_type fill_colour) const;

    gdFont * getfont() const;
    unsigned width( const string &, Graphic::font_type font, int fontsize ) const;

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
    static ostream& init( ostream& );

protected:
    ostream& polyline( ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    ostream& polygon( ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    ostream& rectangle( ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    ostream& roundedRectangle( ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    ostream& circle( ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( ostream& output, const Point& c, const string& s, Graphic::font_type font, int fontsize,
		 justification_type justification, Graphic::colour_type colour, unsigned=0 ) const;

private:
    ostream& arrowHead( ostream& output, const Point&, const Point&, const double scale,
			const Graphic::colour_type, const Graphic::colour_type ) const;

    static ColourManip setcolour( const Graphic::colour_type );
    static ColourManip stroke( const Graphic::colour_type );
    static ColourManip setfill( const Graphic::colour_type );
    static PointManip moveto( const Point& );
    static FontManip setfont( const Graphic::font_type aFont );
    static LineStyleManip linestyle( const Graphic::linestyle_type );
    static JustificationManip justify( const justification_type );

    static ostream& point( ostream& output, const Point& aPoint );
    static ostream& setfill_str( ostream& output, const Graphic::colour_type );
    static ostream& justify_str( ostream& output, const justification_type );
    static ostream& linestyle_str( ostream& output, Graphic::linestyle_type style );
    static ostream& setcolour_str( ostream& output, const Graphic::colour_type );
    static ostream& setfont_str( ostream& output, const Graphic::font_type aFont, const int fontSize );
    static ostream& stroke_str( ostream& output, const Graphic::colour_type );
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
    static ostream& init( ostream& );

protected:
    ostream& polyline( ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    ostream& polygon( ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    ostream& rectangle( ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    ostream& circle( ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( ostream& output, const Point& c, const string& s, Graphic::font_type font, int fontsize,
		 justification_type justification, Graphic::colour_type colour, unsigned=0 ) const;

private:
    ostream& arrowHead( ostream& output, const Point&, const Point&, const double scale,
			const Graphic::colour_type, const Graphic::colour_type ) const;

    static ColourManip setfill( const Graphic::colour_type aColour ) { return ColourManip( SVG::setfill_str, aColour ); }
    static ColourManip setcolour( const Graphic::colour_type aColour ) { return ColourManip( SVG::setcolour_str, aColour ); }
    static ColourManip stroke( const Graphic::colour_type aColour ) { return ColourManip( SVG::stroke_str, aColour ); }
    static FontManip setfont( const Graphic::font_type aFont ) { return FontManip( setfont_str, aFont, Flags::print[FONT_SIZE].value.i ); }
    static JustificationManip justify( const justification_type justification ) { return JustificationManip( justify_str, justification ); }
    static LineStyleManip linestyle( const Graphic::linestyle_type aStyle ) { return LineStyleManip( SVG::linestyle_str, aStyle ); }
    static PointManip moveto( const Point& aPoint ) { return PointManip( point, aPoint ); }

    static ostream& point( ostream& output, const Point& aPoint );
    static ostream& setfill_str( ostream& output, const Graphic::colour_type );
    static ostream& justify_str( ostream& output, const justification_type );
    static ostream& linestyle_str( ostream& output, Graphic::linestyle_type style );
    static ostream& setcolour_str( ostream& output, const Graphic::colour_type );
    static ostream& setfont_str( ostream& output, const Graphic::font_type aFont, const int fontSize );
    static ostream& stroke_str( ostream& output, const Graphic::colour_type );
    static colour_defn fill_value[];
    static font_defn font_value[];
};
#endif

#if defined(SXD_OUTPUT)
class SXD : private Colour, private XMLString
{
public:
    static ostream& init( ostream& );
    static ostream& printStyles( ostream& output );

protected:
    ostream& polyline( ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    ostream& polygon( ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    ostream& rectangle( ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    ostream& circle( ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;

    ostream& begin_paragraph( ostream& output, const Point&, const Point&, const justification_type ) const;
    double text( ostream& output, const Point& c, const string& s, Graphic::font_type font, int fontsize,
		 justification_type justification, Graphic::colour_type colour, unsigned=0 ) const;
    ostream& end_paragraph( ostream& output ) const;

private:
    ostream& init( ostream& output, const char * object_name,
		   Graphic::colour_type pen_colour, Graphic::colour_type fill_colour,
		   Graphic::linestyle_type line_style, Graphic::arrowhead_type=Graphic::NO_ARROW ) const;
    ostream& drawline( ostream& output, const std::vector<Point>& points ) const;

    static ArrowManip arrow_style( const Graphic::arrowhead_type );
    static BoxManip box( const Point&, const Point& );
    static ColourManip setfill( const Graphic::colour_type );
    static ColourManip stroke_colour( const Graphic::colour_type );
    static FontManip setfont( const Graphic::font_type aFont );
    static UnsignedManip style_properties( const unsigned int );
    static JustificationManip justify( const justification_type );
    static PointManip moveto( const Point& );
    static XMLString::StringManip draw_layer( const std::string& );
    static XMLString::StringManip end_style();
    static XMLString::StringManip start_style( const std::string&, const std::string& );

    static ostream& arrow_style_str( ostream& output, const Graphic::arrowhead_type );
    static ostream& box_str( ostream& output, const Point& origin, const Point& extent );
    static ostream& draw_layer_str( ostream& output, const std::string&, const std::string& );
    static ostream& end_style_str( ostream& output, const std::string&, const std::string& );
    static ostream& setfill_str( ostream& output, const Graphic::colour_type );
    static ostream& justify_str( ostream& output, const justification_type );
    static ostream& point( ostream& output, const Point& aPoint );
    static ostream& setfont_str( ostream& output, const Graphic::font_type aFont, const int fontSize );
    static ostream& start_style_str( ostream& output, const std::string&, const std::string& );
    static ostream& stroke_colour_str( ostream& output, const Graphic::colour_type );
    static ostream& style_properties_str( ostream& output, const unsigned int );

private:
    static const char * graphics_family;
    static const char * paragraph_family;
    static const char * text_family;
};
#endif

class TeX : private Colour
{
protected:
    ostream& polyline( ostream& output, const std::vector<Point>& points, Graphic::colour_type pen_colour,
		       Graphic::linestyle_type linestyle=Graphic::DEFAULT_LINESTYLE, Graphic::arrowhead_type=Graphic::NO_ARROW, double scale=1 ) const;
    ostream& polygon( ostream& output, const std::vector<Point>& points,
		      Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    ostream& rectangle( ostream& output, const Point&, const Point&, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour, Graphic::linestyle_type=Graphic::DEFAULT_LINESTYLE ) const;
    ostream& circle( ostream& output, const Point& c, const double r, Graphic::colour_type pen_colour, Graphic::colour_type fill_colour ) const;
    double text( ostream& output, const Point& c, const string& s, Graphic::font_type font, int fontsize,
		 justification_type justification, Graphic::colour_type colour, unsigned=0 ) const;

private:
    static PointManip moveto( const Point& );
    static ColourManip setcolour( const Graphic::colour_type );
    static ColourManip setfill( const Graphic::colour_type );
    static FontManip setfont( const Graphic::font_type aFont );
    static JustificationManip justify( const justification_type );

    ostream& arrowHead( ostream& output, const Point&, const Point&, const double scale,
			const Graphic::colour_type, const Graphic::colour_type ) const;

    static ostream& point( ostream& output, const Point& aPoint );
    static ostream& setcolour_str( ostream& output, const Graphic::colour_type );
    static ostream& setfill_str( ostream& output, Graphic::colour_type aColour );
    static ostream& setfont_str( ostream& output, const Graphic::font_type aFont, const int fontSize );
    static ostream& justify_str( ostream& output, const justification_type );
};

#if defined(X11_OUTPUT)
class X11
{
protected:
#if 0
    static ostream& init( ostream& output ) { return output; }
    static ColourManip setcolour( const Graphic::colour_type );
    static ColourManip stroke( const Graphic::colour_type );
    static ColourManip setfill( const Graphic::colour_type );
    static PointManip moveto( const Point& );
    static FontManip setfont( const Graphic::font_type aFont );
    static JustificationManip justify( const justification_type );
#endif

    ostream& lineStyle( ostream& output, Graphic::linestyle_type style ) const { return output; }
    ostream& arrowHead( ostream& output, const Point&, const Point&,
	const Graphic::colour_type, const Graphic::colour_type ) const { return output; }

private:
#if 0
    static ostream& point( ostream& output, const Point& aPoint );
    static ostream& setcolour_str( ostream& output, const Graphic::colour_type );
    static ostream& stroke_str( ostream& output, const Graphic::colour_type );
    static ostream& setfill_str( ostream& output, const Graphic::colour_type );
    static ostream& setfont_str( ostream& output, const Graphic::font_type aFont, const int fontSize );
    static ostream& justify_str( ostream& output, const justification_type );
    static const char * font_value[];
#endif
};
#endif


Graphic::colour_type error_colour( double delta );
#endif
