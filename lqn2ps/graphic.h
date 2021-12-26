/* -*- c++ -*-
 * graphic.h	-- Greg Franks
 *
 * $Id: graphic.h 15262 2021-12-26 18:55:49Z greg $
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
    enum class Colour {
	TRANSPARENT,
	DEFAULT,
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
	GOLD };
    enum class LineStyle { DEFAULT, SOLID, DASHED, DOTTED, DASHED_DOTTED };
    enum class ArrowHead { NONE, CLOSED, OPEN };
    enum class Fill { NONE, DEFAULT, ZERO, NINETY, SOLID, TINT };
    enum class Font { DEFAULT, NORMAL, OBLIQUE, BOLD, SPECIAL };

private:
    Graphic( const Graphic& );
    Graphic& operator=( const Graphic& );

public:
    explicit Graphic( Colour pen_colour = Colour::DEFAULT, Colour fill_colour = Colour::DEFAULT, LineStyle ls = LineStyle::DEFAULT )
	: _penColour(pen_colour), _fillColour(fill_colour), _fillStyle(Graphic::Fill::TINT), _linestyle(ls), _depth(0)
	{}
    virtual ~Graphic() {}

    Graphic& penColour( Colour colour ) { _penColour = colour; return *this; }
    Colour penColour() const { return _penColour; }
    Graphic& fillColour( Colour colour ) { _fillColour = colour; return *this; }
    Colour fillColour() const { return _fillColour; }
    Graphic& linestyle( LineStyle linestyle ) { _linestyle = linestyle; return *this; }
    LineStyle linestyle() const { return  _linestyle; }
    Graphic& depth( const unsigned depth )  { _depth = depth; return *this; }
    unsigned depth() const { return _depth; }
    Graphic& fillStyle( const Fill fill ) { _fillStyle = fill; return *this; }
    Fill fillStyle() const { return _fillStyle; }

    static bool intersects( const Point&, const Point&, const Point&, const Point& );
    static bool intersects( const Point&, const Point&, const Point& );

    virtual std::ostream& comment( std::ostream& output, const std::string& ) const = 0;

private:
    Colour _penColour;
    Colour _fillColour;
    Fill _fillStyle;
    LineStyle _linestyle;
    int _depth;
};

class RGB
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
	ColourManip( std::ostream& (*ff)(std::ostream&, const Graphic::Colour aColour ),
		     const Graphic::Colour aColour )
	    : f(ff), myColour(aColour) {}
    private:
	std::ostream& (*f)( std::ostream&, const Graphic::Colour aColour );
	const Graphic::Colour myColour;

	friend std::ostream& operator<<(std::ostream & os, const ColourManip& m )
	    { return m.f( os, m.myColour ); }
    };

    class FontManip
    {
    public:
	FontManip( std::ostream& (*ff)(std::ostream&, const Graphic::Font font, const int size ),
		   const Graphic::Font font, const int size )
	    : f(ff), myFont(font), mySize(size) {}
    private:
	std::ostream& (*f)( std::ostream&, const Graphic::Font font, const int size );
	const Graphic::Font myFont;
	const int mySize;


	friend std::ostream& operator<<(std::ostream & os, const FontManip& m )
	    { return m.f( os, m.myFont, m.mySize ); }
    };

    class LineStyleManip
    {
    public:
	LineStyleManip( std::ostream& (*ff)(std::ostream&, const Graphic::LineStyle linestyle ),
			const Graphic::LineStyle linestyle )
	    : f(ff), _lineStyle(linestyle) {}
    private:
	std::ostream& (*f)( std::ostream&, const Graphic::LineStyle linestyle );
	const Graphic::LineStyle _lineStyle;

	friend std::ostream& operator<<(std::ostream & os, const LineStyleManip& m )
	    { return m.f( os, m._lineStyle ); }
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
	ArrowManip( std::ostream& (*ff)(std::ostream&, const Graphic::ArrowHead arrow ),
		    const Graphic::ArrowHead arrow )
	    : f(ff), myArrow(arrow) {}
    private:
	std::ostream& (*f)( std::ostream&, const Graphic::ArrowHead arrow );
	const Graphic::ArrowHead myArrow;

	friend std::ostream& operator<<(std::ostream & os, const ArrowManip& m )
	    { return m.f( os, m.myArrow ); }
    };

protected:
    static float tint( const float, const float );
    static int to_byte( const float x ) { return static_cast<int>( x * 255.0 ); }
    static RGBManip rgb( const float, const float, const float );

    static std::ostream& rgb_str( std::ostream& output, const float, const float, const float );

    static const std::map<const Graphic::Colour,const RGB::colour_defn> __value;
    static const std::map<const Graphic::Colour,const std::string> __name;
};

#if EMF_OUTPUT
class EMF : private RGB
{
public:
    static std::ostream& init( std::ostream&, const double, const double, const std::string& );
    static std::ostream& terminate( std::ostream& );

protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::Colour pen_colour,
			    Graphic::LineStyle linestyle=Graphic::LineStyle::DEFAULT, Graphic::ArrowHead=Graphic::ArrowHead::NONE, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
			   Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, Graphic::LineStyle=Graphic::LineStyle::DEFAULT ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
		 Justification justification, Graphic::Colour colour, unsigned=0 ) const;
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
    std::ostream& draw_dashed_line( std::ostream& output, Graphic::LineStyle line_style, const std::vector<Point>& ) const;
    static ColourManip setfill( const Graphic::Colour );
    static JustificationManip justify( const Justification );
    static PointManip lineto( const Point& );
    static PointManip moveto( const Point& );
    static PointManip point( const Point& );
    static RGBManip rgb( const float, const float, const float );
    static ColourManip setcolour( const Graphic::Colour );
    static FontManip setfont( const Graphic::Font font );
    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&, const double scale,
			     const Graphic::Colour, const Graphic::Colour ) const;


    static std::ostream& setfill_str( std::ostream& output, Graphic::Colour aColour );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static std::ostream& lineto_str( std::ostream& output, const Point& aPoint );
    static std::ostream& moveto_str( std::ostream& output, const Point& aPoint );
    static std::ostream& point_str( std::ostream& output, const Point& aPoint );
    static std::ostream& rgb_str( std::ostream& output, const float, const float, const float );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize );
    static std::ostream& start_record_str( std::ostream& output, const int, const int );
    static std::ostream& writef_str( std::ostream& output, const double );
    static std::ostream& writel_str( std::ostream& output, const unsigned long );
    static std::ostream& writep_str( std::ostream& output, const int, const int );
    static std::ostream& writes_str( std::ostream& output, const unsigned short );
    static std::ostream& writestr_str( std::ostream& output, const std::string& aString );

private:
    static long record_count;
    static Graphic::Colour last_pen_colour;
    static Graphic::Colour last_fill_colour;
    static Graphic::Colour last_arrow_colour;
    static Justification last_justification;
    static Graphic::Font last_font;
    static Graphic::LineStyle last_line_style;
    static int last_font_size;
};
#endif

class Fig : private RGB
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
			    int sub_type, Graphic::Colour pen_colour, Graphic::Colour fill_colour, int depth,
			    Graphic::LineStyle linestyle=Graphic::LineStyle::DEFAULT,
			    Graphic::ArrowHead=Graphic::ArrowHead::NONE, double scale=1.0 ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, int sub_type,
			  Graphic::Colour pen_colour, Graphic::Colour fill_colour, int depth, Graphic::Fill fill_style=Graphic::Fill::DEFAULT ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, int depth, Graphic::LineStyle line_style=Graphic::LineStyle::DEFAULT ) const;
    std::ostream& roundedRectangle( std::ostream& output, const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, int depth, Graphic::LineStyle line_style=Graphic::LineStyle::DEFAULT ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
		 Justification, Graphic::Colour colour, unsigned flags=0 ) const;
    std::ostream& clearBackground( std::ostream& output, const Point&, const Point&, const Graphic::Colour ) const;

private:
    typedef struct
    {
	int type;
	int style;
    } arrowhead_defn;

    std::ostream& init( std::ostream& output, int object_code, int sub_type,
			Graphic::Colour pen_colour, Graphic::Colour fill_colour,
			Graphic::LineStyle line_style, Graphic::Fill, int depth ) const;
    static PointManip moveto( const Point& );
    static std::ostream& arrowHead( std::ostream& output, Graphic::ArrowHead, const double scale );
    static std::ostream& point( std::ostream& output, const Point& aPoint );

    static const std::map<const Graphic::Colour, const int> __colour;
    static const std::map<const Graphic::LineStyle, const int> __linestyle;
    static const std::map<const Graphic::Font, const int> __postscript_font;
    static const std::map<const Graphic::Font, const int> __tex_font;
    static const std::map<const Graphic::Fill, const int> __fill;
    static const std::map<const Graphic::ArrowHead, const arrowhead_defn> __arrowhead;
};

#if HAVE_GD_H && HAVE_LIBGD
bool operator==( const gdPoint&, const gdPoint& );
inline bool operator!=( const gdPoint& p1, gdPoint& p2 ) { return !( p1 == p2 ); }
gdPoint operator-( const gdPoint&, const gdPoint& );
gdPoint operator+( const gdPoint&, const gdPoint& );

class GD : private RGB
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

    GD const & polygon( const std::vector<Point>& points, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    GD const & drawline( const Point& p1, const Point& p2, Graphic::Colour pen_colour, Graphic::LineStyle linestyle ) const;
    GD const & rectangle( const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, Graphic::LineStyle=Graphic::LineStyle::DEFAULT ) const;
    GD const & circle( const Point& center, const double d, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    double text( const Point& p1, const std::string&, Graphic::Font, int, Justification, Graphic::Colour fill_colour ) const;
    GD const & arrowHead( const Point&, const Point&, const double scaling, const Graphic::Colour pen_colour, const Graphic::Colour fill_colour) const;

    gdFont * getfont() const;
    unsigned width( const std::string &, Graphic::Font font, int fontsize ) const;

protected:
    static gdImagePtr im;
    static bool haveTTF;

private:
    enum BRECT { LLx, LLy, LRx, LRy, URx, URy, ULx, ULy };

    static std::map<const Graphic::Colour,int> __pen;
    static std::map<const Graphic::Colour,int> __fill;
    static std::map<const Graphic::Font, const std::string> __font;
};
#endif

class PostScript : private RGB
{
public:
    static std::ostream& init( std::ostream& );

protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::Colour pen_colour,
			    Graphic::LineStyle linestyle=Graphic::LineStyle::DEFAULT, Graphic::ArrowHead=Graphic::ArrowHead::NONE, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
			   Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, Graphic::LineStyle=Graphic::LineStyle::DEFAULT ) const;
    std::ostream& roundedRectangle( std::ostream& output, const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, Graphic::LineStyle=Graphic::LineStyle::DEFAULT ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
		 Justification justification, Graphic::Colour colour, unsigned=0 ) const;

private:
    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&, const double scale,
			     const Graphic::Colour, const Graphic::Colour ) const;

    static ColourManip setcolour( const Graphic::Colour );
    static ColourManip stroke( const Graphic::Colour );
    static ColourManip setfill( const Graphic::Colour );
    static PointManip moveto( const Point& );
    static FontManip setfont( const Graphic::Font font );
    static LineStyleManip linestyle( const Graphic::LineStyle );
    static JustificationManip justify( const Justification );

    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setfill_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static std::ostream& linestyle_str( std::ostream& output, Graphic::LineStyle style );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize );
    static std::ostream& stroke_str( std::ostream& output, const Graphic::Colour );

    static const std::map<const Graphic::Font, const std::string> __font;
};

#if SVG_OUTPUT
class SVG : private RGB, private XMLString
{
    typedef struct
    {
	const char * family;
	const char * style;
    } font_defn;

public:
    static std::ostream& init( std::ostream& );

protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::Colour pen_colour,
			    Graphic::LineStyle linestyle=Graphic::LineStyle::DEFAULT, Graphic::ArrowHead=Graphic::ArrowHead::NONE, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
			   Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, Graphic::LineStyle=Graphic::LineStyle::DEFAULT ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
		 Justification justification, Graphic::Colour colour, unsigned=0 ) const;

private:
    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&, const double scale,
			     const Graphic::Colour, const Graphic::Colour ) const;

    static ColourManip setfill( const Graphic::Colour aColour ) { return ColourManip( SVG::setfill_str, aColour ); }
    static ColourManip setcolour( const Graphic::Colour aColour ) { return ColourManip( SVG::setcolour_str, aColour ); }
    static ColourManip stroke( const Graphic::Colour aColour ) { return ColourManip( SVG::stroke_str, aColour ); }
    static FontManip setfont( const Graphic::Font font ) { return FontManip( setfont_str, font, Flags::print[FONT_SIZE].opts.value.i ); }
    static JustificationManip justify( const Justification justification ) { return JustificationManip( justify_str, justification ); }
    static LineStyleManip linestyle( const Graphic::LineStyle aStyle ) { return LineStyleManip( SVG::linestyle_str, aStyle ); }
    static PointManip moveto( const Point& aPoint ) { return PointManip( point, aPoint ); }

    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setfill_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static std::ostream& linestyle_str( std::ostream& output, Graphic::LineStyle style );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize );
    static std::ostream& stroke_str( std::ostream& output, const Graphic::Colour );
    static colour_defn fill_value[];
    static const std::map<const Graphic::Font, const font_defn> __font;
};
#endif

#if SXD_OUTPUT
class SXD : private RGB, private XMLString
{
public:
    static std::ostream& init( std::ostream& );
    static std::ostream& printStyles( std::ostream& output );

protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::Colour pen_colour,
			    Graphic::LineStyle linestyle=Graphic::LineStyle::DEFAULT, Graphic::ArrowHead=Graphic::ArrowHead::NONE, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
			   Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, Graphic::LineStyle=Graphic::LineStyle::DEFAULT ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;

    std::ostream& begin_paragraph( std::ostream& output, const Point&, const Point&, const Justification ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
		 Justification justification, Graphic::Colour colour, unsigned=0 ) const;
    std::ostream& end_paragraph( std::ostream& output ) const;

private:
    std::ostream& init( std::ostream& output, const char * object_name,
			Graphic::Colour pen_colour, Graphic::Colour fill_colour,
			Graphic::LineStyle line_style, Graphic::ArrowHead=Graphic::ArrowHead::NONE ) const;
    std::ostream& drawline( std::ostream& output, const std::vector<Point>& points ) const;

    static ArrowManip arrow_style( const Graphic::ArrowHead );
    static BoxManip box( const Point&, const Point& );
    static ColourManip setfill( const Graphic::Colour );
    static ColourManip stroke_colour( const Graphic::Colour );
    static FontManip setfont( const Graphic::Font font );
    static UnsignedManip style_properties( const unsigned int );
    static JustificationManip justify( const Justification );
    static PointManip moveto( const Point& );
    static XMLString::StringManip draw_layer( const std::string& );
    static XMLString::StringManip end_style();
    static XMLString::StringManip start_style( const std::string&, const std::string& );

    static std::ostream& arrow_style_str( std::ostream& output, const Graphic::ArrowHead );
    static std::ostream& box_str( std::ostream& output, const Point& origin, const Point& extent );
    static std::ostream& draw_layer_str( std::ostream& output, const std::string&, const std::string& );
    static std::ostream& end_style_str( std::ostream& output, const std::string&, const std::string& );
    static std::ostream& setfill_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize );
    static std::ostream& start_style_str( std::ostream& output, const std::string&, const std::string& );
    static std::ostream& stroke_colour_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& style_properties_str( std::ostream& output, const unsigned int );

private:
    static const char * graphics_family;
    static const char * paragraph_family;
    static const char * text_family;
};
#endif

class TeX : private RGB
{
protected:
    std::ostream& polyline( std::ostream& output, const std::vector<Point>& points, Graphic::Colour pen_colour,
			    Graphic::LineStyle linestyle=Graphic::LineStyle::DEFAULT, Graphic::ArrowHead=Graphic::ArrowHead::NONE, double scale=1 ) const;
    std::ostream& polygon( std::ostream& output, const std::vector<Point>& points,
			   Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    std::ostream& rectangle( std::ostream& output, const Point&, const Point&, Graphic::Colour pen_colour, Graphic::Colour fill_colour, Graphic::LineStyle=Graphic::LineStyle::DEFAULT ) const;
    std::ostream& circle( std::ostream& output, const Point& c, const double r, Graphic::Colour pen_colour, Graphic::Colour fill_colour ) const;
    double text( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
		 Justification justification, Graphic::Colour colour, unsigned=0 ) const;

private:
    static PointManip moveto( const Point& );
    static ColourManip setcolour( const Graphic::Colour );
    static ColourManip setfill( const Graphic::Colour );
    static FontManip setfont( const Graphic::Font font );
    static JustificationManip justify( const Justification );

    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&, const double scale,
			     const Graphic::Colour, const Graphic::Colour ) const;

    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& setfill_str( std::ostream& output, Graphic::Colour aColour );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize );
    static std::ostream& justify_str( std::ostream& output, const Justification );
};

#if X11_OUTPUT
class X11
{
protected:
#if 0
    static std::ostream& init( std::ostream& output ) { return output; }
    static ColourManip setcolour( const Graphic::Colour );
    static ColourManip stroke( const Graphic::Colour );
    static ColourManip setfill( const Graphic::Colour );
    static PointManip moveto( const Point& );
    static FontManip setfont( const Graphic::Font font );
    static JustificationManip justify( const Justification );
#endif

    std::ostream& lineStyle( std::ostream& output, Graphic::LineStyle style ) const { return output; }
    std::ostream& arrowHead( std::ostream& output, const Point&, const Point&,
			     const Graphic::Colour, const Graphic::Colour ) const { return output; }

private:
#if 0
    static std::ostream& point( std::ostream& output, const Point& aPoint );
    static std::ostream& setcolour_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& stroke_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& setfill_str( std::ostream& output, const Graphic::Colour );
    static std::ostream& setfont_str( std::ostream& output, const Graphic::Font font, const int fontSize );
    static std::ostream& justify_str( std::ostream& output, const Justification );
    static const char * font_value[];
#endif
};
#endif


Graphic::Colour error_colour( double delta );
#endif
