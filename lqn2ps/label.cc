/* label.cc	-- Greg Franks Wed Jan 29 2003
 * 
 * $Id: label.cc 11987 2014-04-16 20:57:40Z greg $
 */

#include "lqn2ps.h"
#include <stdarg.h>
#include <cstdlib>
#include <ctype.h>
#include <cmath>
#if HAVE_IEEEFP_H && !defined(MSDOS)
#include <ieeefp.h>
#endif
#include <lqio/dom_extvar.h>
#include "label.h"
#include "entry.h"
#include "call.h"
#if HAVE_GD_H
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontg.h>
#endif

class LabelStringManip {
public:
    LabelStringManip( ostream& (*ff)(ostream&, const char * ), 
		      const char * aStr  )
	: f(ff), myStr(aStr) {}
private:
    ostream& (*f)( ostream&, const char * );
    const char * myStr;

    friend ostream& operator<<(ostream & os, const LabelStringManip& m )
	{ return m.f(os,m.myStr); }
};

/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Printing function.
 */

ostream&
operator<<( ostream& output, const Label& self )
{
    return self.print( output );
}

Label *
Label::newLabel()
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
    case FORMAT_EEPIC:
	return new LabelTeX();
#if defined(EMF_OUTPUT)
    case FORMAT_EMF:
	return new LabelEMF();
#endif
    case FORMAT_FIG:
	return new LabelFig();
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
	return new LabelGD();
#endif	/* HAVE_LIBGD */
    case FORMAT_POSTSCRIPT:
	return new LabelPostScript();	/* the graphical object		*/
    case FORMAT_PSTEX:
	return new LabelPsTeX();	/* the graphical object		*/
#if defined(SVG_OUTPUT)
    case FORMAT_SVG:
	return new LabelSVG();
#endif
#if defined(SXD_OUTPUT)
    case FORMAT_SXD:
	return new LabelSXD();
#endif
#if defined(X11_OUTPUT)
    case FORMAT_X11:
	return new LabelX11();
#endif
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
	return new LabelNull();

    default:
	abort();
    }
    return 0;
}

Label::Label() 
    : Graphic(), 
      myJustification(CENTER_JUSTIFY), 
      myBackgroundColour(TRANSPARENT), 
      mathMode(false)
{
    grow( 1 );
}


Label::Label( const Point& aPoint ) 
    : Graphic(), 
      origin(aPoint), 
      myJustification(CENTER_JUSTIFY), 
      myBackgroundColour(TRANSPARENT), 
      mathMode(false)
{
    grow( 1 );
}



Label::~Label()
{
    clear();
}


void
Label::grow( unsigned size )
{
    const unsigned int sz = myStrings.size();
    myStrings.reserve(size+sz);
    myFont.reserve(size+sz);
    myColour.reserve(size+sz);
    for ( unsigned i = 0; i < size; ++i ) {
	ostringstream * s = new ostringstream;
	s->precision(Flags::print[PRECISION].value.i);
	myStrings.push_back( s );
	myFont.push_back( DEFAULT_FONT );
	myColour.push_back( DEFAULT_COLOUR );
    }
}


/*
 * Clear old labels.
 */
 
Label&
Label::clear()
{	
    for ( vector<ostringstream *>::const_iterator s = myStrings.begin(); s != myStrings.end(); ++s ) {
	delete (*s);
    }
    myStrings.clear();
    myFont.clear();
    myColour.clear();
    return *this;
}


Label&
Label::initialize( const string& s ) 
{
    clear();
    grow(1);
    *this << s;
    return *this;
}

Label&
Label::scaleBy( const double sx, const double sy )
{	
    origin.x( sx * origin.x() );
    origin.y( sy * origin.y() );
    return *this;
}


Label&
Label::translateY( const double dy )
{
    origin.y( dy - origin.y() );
    return *this;
}


Label&
Label::newLine()
{
    mathMode = false;
    grow( 1 );
    return *this;
}



Label&
Label::font( const font_type font )
{
    myFont[size()-1] = font;
    return *this;
}


Label&
Label::colour( const colour_type colour )
{
    myColour[size()-1] = colour;
    return *this;
}


double 
Label::width() const
{
    unsigned int w = 0;
    for ( vector<ostringstream *>::const_iterator s = myStrings.begin(); s != myStrings.end(); ++s ) {
	w = ::max( w, (*s)->str().length() );
    }
    return w * normalized_font_size() / 2.0;		// A guess.
}



double 
Label::height() const
{
    return size() * Flags::print[FONT_SIZE].value.i;
}


Label&
Label::appendLSM( const LabelStringManip& m)
{ 
    *myStrings.back() << m; 
    return *this;
}


Label&
Label::appendSCM( const SRVNCallManip& m )
{ 
    *myStrings.back() << m; 
    return *this;
}


Label&
Label::appendSEM( const SRVNEntryManip& m)
{ 
    *myStrings.back() << m; 
    return *this;
}


Label&
Label::appendSTM( const TaskCallManip& m )
{ 
    *myStrings.back() << m; 
    return *this;
}


Label& 
Label::appendD( const double aDouble )
{
    if ( !isfinite( aDouble ) ) {
	if ( aDouble < 0.0 ) {
	    *myStrings.back() << '-'; 
	}
	infty();
    } else {
	*myStrings.back() << aDouble; 
    }
    return *this; 
}

Label&
Label::appendV( const LQIO::DOM::ExternalVariable& v ) 
{
    if ( Flags::instantiate ) {
	appendD( to_double( v ) );
    } else {
	stringstream s;		/* For Unicode - We need to expand the string */
	s << v;
	appendS( s.str() );
    }
    return *this;
}

Label& 
Label::beginMath()
{
    if ( !mathMode ) {
	font(Graphic::SYMBOL_FONT);
	mathMode = true;
    }
    return *this;
}


Label& 
Label::endMath()
{
    return *this;
}


Label&
Label::epsilon()
{
    font(Graphic::SYMBOL_FONT) << "e";
    return *this;
}


Label&
Label::infty()
{
    (*this) << "inf";
    return *this;
}


Label&
Label::lambda()
{
    font(Graphic::SYMBOL_FONT) << "l";
    return *this;
}


Label&
Label::mu()
{
    font(Graphic::SYMBOL_FONT) << "m";
    return *this;
}


Label&
Label::rho()
{
    font(Graphic::SYMBOL_FONT) << "r";
    return *this;
}


Label& 
Label::sigma()
{
    (*this) << "s2";
    return *this;
}


Label&
Label::times()
{
    (*this) << "* ";
    return *this;
}


Label&
Label::percent()
{
    (*this) << "%";
    return *this;
}


/*
 * Compute:
 *  The point to draw the object.
 *  The origin and extent of the complete label.
 */

const Label&
Label::boundingBox( Point& boxOrigin, Point& boxExtent, const double scaling ) const
{
    boxOrigin = origin;
    boxExtent.moveTo( width(), height() );		/* A guess... width is half of height */
    boxExtent *= scaling;
    
    switch ( justification() ) {
    case LEFT_JUSTIFY:	
	boxOrigin.moveBy( 0.0, -boxExtent.y() / 2.0 );
	break;

    case RIGHT_JUSTIFY:	
	boxOrigin.moveBy( -boxExtent.x(), -boxExtent.y() / 2.0 );
	break;

    default:
	boxOrigin.moveBy( -boxExtent.x() / 2.0, -boxExtent.y() / 2.0 );
	break;
    }
    return *this;
}

#if defined(EMF_OUTPUT)
/* -------------------------------------------------------------------- */
/* Windows Enhanced Meta File output					*/
/* -------------------------------------------------------------------- */

double
LabelEMF::width() const
{
    unsigned int w = 0;
    for ( vector<ostringstream *>::const_iterator s = myStrings.begin(); s != myStrings.end(); ++s ) {
	w = ::max( w, (*s)->str().length() );
    }
    return w * normalized_font_size() / 4.5;		// A guess.
}



/*
 * EMF uses UNICode.  Little Endian
 */

Label& 
LabelEMF::epsilon()
{
    *myStrings.back() << static_cast<char>(0x03) << static_cast<char>(0xb5);	/* Greek 0x3b5 */
    return *this;
}

Label&
LabelEMF::lambda()
{
    *myStrings.back() << static_cast<char>(0x03) << static_cast<char>(0xbb);	/* Greek 0x3bb */
    return *this;
}

Label&
LabelEMF::mu()
{
    *myStrings.back() << static_cast<char>(0x03) << static_cast<char>(0xbc);	/* Greek 0x3bc */
    return *this;
}

Label&
LabelEMF::rho()
{
    *myStrings.back() << static_cast<char>(0x03) << static_cast<char>(0xc1);	/* Greek 0x3c1 */
    return *this;
}

Label&
LabelEMF::times()
{
    *myStrings.back() << static_cast<char>(0x00) << static_cast<char>(0xd7);	/* 0x00d7 */
    return *this;
}

Label&
LabelEMF::sigma()
{
    *myStrings.back() << static_cast<char>(0x03) << static_cast<char>(0xc3);	/* Greek 0x3c3 */
    return *this;
}

Label&
LabelEMF::infty()
{
    *myStrings.back() << static_cast<char>(0x22) << static_cast<char>(0x1e);
    return *this;
}


/*
 * Convert to Unicode.
 */

Label&
LabelEMF::appendC( const char c )
{
    *myStrings.back() << static_cast<char>(0x00) << c;
    return *this;
}


/*
 * Convert to Unicode.
 */

Label&
LabelEMF::appendPC( const char * s )
{
    for ( ; *s; ++s ) {
	*myStrings.back() << static_cast<char>(0x00) << *s;
    }
    return *this;
}


/*
 * Convert to Unicode.
 */

Label&
LabelEMF::appendS( const string& s )
{
    for ( unsigned i = 0; i < s.size(); ++i ) {
	*myStrings.back() << static_cast<char>(0x00) << s[i];
    }
    return *this;
}


/*
 * We have to convert to unicode.  Ugh.
 */

Label& 
LabelEMF::appendD( const double aDouble )
{
    if ( !isfinite( aDouble ) ) {
	if ( aDouble < 0.0 ) {
	    appendC( '-' ); 
	}
	infty();
    } else {
	ostringstream s;
	s << aDouble;
	appendS( s.str() );
    }
    return *this; 
}


/*
 * We have to convert to unicode.  Ugh.
 */

Label& 
LabelEMF::appendI( const int anInt )
{
    ostringstream s;
    s << anInt;
    appendS( s.str() );
    return *this; 
}


/*
 * We have to convert to unicode.  Ugh.
 */

Label& 
LabelEMF::appendUI( const unsigned anInt )
{
    ostringstream s;
    s << anInt;
    appendS( s.str() );
    return *this; 
}


Label&
LabelEMF::appendLSM( const LabelStringManip& aManip )
{
    ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


Label&
LabelEMF::appendSEM( const SRVNEntryManip& aManip )
{
    ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


Label&
LabelEMF::appendSCM( const SRVNCallManip& aManip )
{
    ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


Label&
LabelEMF::appendSTM( const TaskCallManip& aManip )
{
    ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


ostream&
LabelEMF::print( ostream& output ) const
{
    Point boxOrigin;
    Point boxExtent;
    boundingBox( boxOrigin, boxExtent, EMF_SCALING );

    if ( boxExtent.x() == 0 || boxExtent.y() == 0 ) return output;

    /* Now put out stuff */
    Point aPoint = initialPoint();
    for ( int i = 0; i < size(); ++i ) {
	const std::string& str = myStrings[i]->str();
	if ( str.size() ) {
	    const double dy = text( output, aPoint, str,
				    myFont[i], Flags::print[FONT_SIZE].value.i,
				    justification(), static_cast<Graphic::colour_type>(myColour[i]) );
	    aPoint.moveBy( 0, dy );
	}
    }

    return output;
}



Point
LabelEMF::initialPoint() const
{
    Point aPoint = origin;
    double dx = 0.0;
    switch ( justification() ) {
    case LEFT_JUSTIFY:
	dx =  Flags::print[FONT_SIZE].value.i / 2.0 * EMF_SCALING;
	break;
    case RIGHT_JUSTIFY:
	dx = -Flags::print[FONT_SIZE].value.i / 2.0 * EMF_SCALING;
	break;
    }
    aPoint.moveBy( dx, (2.0 - size()) / 2.0 * Flags::print[FONT_SIZE].value.i * EMF_SCALING );
    return aPoint;
}
#endif

/* -------------------------------------------------------------------- */
/* XFIG output								*/
/* -------------------------------------------------------------------- */

Label&
LabelFig::infty()
{
    if ( !mathMode ) {
	Label::infty();
    } else {
	 *myStrings.back() << "\\245";
    }
    return *this;
}


Label&
LabelFig::times()
{
    if ( !mathMode ) {
	Label::times();
    } else {
	 *myStrings.back() << "\\264";
    }
    return *this;
}


/*
 * Need to escape certain characters.
 */

Label&
LabelFig::appendPC( const char * s )
{
    for ( ; *s; ++s ) {
	if ( *s == '\\' ) {
	    *myStrings.back() << '\\';		/* Tack on another Backslash */
	}
	*myStrings.back() << *s;
    }
    return *this;
}


ostream&
LabelFig::comment( ostream& output, const string& aString ) const
{
    output << "# " << aString << endl;
    return output;
}


ostream&
LabelFig::print( ostream& output ) const
{
    Point boxOrigin;
    Point boxExtent;		/* A guess... width is half of height */
    boundingBox( boxOrigin, boxExtent, FIG_SCALING );

    if ( boxExtent.x() == 0 || boxExtent.y() == 0 ) return output;

    startCompound( output, boxOrigin, boxExtent );
    if ( Flags::clear_label_background ) {
	Fig::clearBackground( output, boxOrigin, boxExtent, backgroundColour() );
    }

    /* Now put out stuff */

    Point aPoint = initialPoint();
    for ( int i = 0; i < size(); ++i ) {
	const double dy = text( output, aPoint, myStrings[i]->str(), myFont[i],
				Flags::print[FONT_SIZE].value.i, justification(), 
				static_cast<Graphic::colour_type>(myColour[i]), Fig::POSTSCRIPT );
	aPoint.moveBy( 0, dy );
    }

    endCompound( output );
    return output;
}


Point
LabelFig::initialPoint() const
{
    Point aPoint = origin;
    double dx = 0.0;
    switch ( justification() ) {
    case LEFT_JUSTIFY:
	dx =  Flags::print[FONT_SIZE].value.i / 2.0 * FIG_SCALING;
	break;
    case RIGHT_JUSTIFY:
	dx = -Flags::print[FONT_SIZE].value.i / 2.0 * FIG_SCALING;
	break;
    }
    aPoint.moveBy( dx, (((1-size())*Flags::print[FONT_SIZE].value.i)/2 + 2) * FIG_SCALING );
    return aPoint;
}

#if HAVE_GD_H && HAVE_LIBGD 
/* -------------------------------------------------------------------- */
/* GD (Jpeg, PNG, GIF ) output						*/
/* -------------------------------------------------------------------- */

double
LabelGD::width() const
{
    unsigned w = 0;
    for ( int i = 1; i <= size(); ++i ) {
	w = ::max( w, GD::width( myStrings[i]->str(), 
				 myFont[i], 
				 Flags::print[FONT_SIZE].value.i ) );
    }
    return w;
}



Label&
LabelGD::lambda()
{
    if ( haveTTF && mathMode ) {
//	*myStrings.back() << "&#0955;";	/* Greek 0x3bb */
	(*this) << "T";
    } else {
	(*this) << "T";
    }
    return *this;
}


/* GD uses 8859-2 */

Label&
LabelGD::mu()
{
    if ( haveTTF && mathMode ) {
//	*myStrings.back() << "&#0956;";	/* Greek 0x3bc */
	(*this) << "U";
    } else {
	(*this) << "U";
    }
    return *this;
}


/* GD uses 8859-2 */

Label&
LabelGD::rho()
{
    if ( haveTTF && mathMode ) {
//	*myStrings.back() << "&#0956;";	/* Greek 0x3bc */
	(*this) << "U";
    } else {
	(*this) << "U";
    }
    return *this;
}


Label&
LabelGD::times()
{
    if ( haveTTF && mathMode ) {
//	*myStrings.back() << "&#0215;";	/* 0x00d7 */
	(*this) << static_cast<char>(0xD7);	/* http://www.unicode.org/charts/ -- Chart starts at 0080  Latin-1 Supplement */
    } else {
	(*this) << static_cast<char>(0xD7);	/* http://www.unicode.org/charts/ -- Chart starts at 0080  Latin-1 Supplement */
    }
    return *this;
}


Label&
LabelGD::sigma()
{
    if ( haveTTF ) {
//	*myStrings.back() << "&#0955;";	/* Greek 0x3bb */
	Label::sigma();
    } else {
	Label::sigma();
    }
    return *this;
}


Label&
LabelGD::infty()
{
    if ( haveTTF && mathMode ) {
//	*myStrings.back() << "&#8734;";	
	Label::infty();
    } else {
	Label::infty();
    }
    return *this;
}


/*
 * Need to escape certain characters.
 */

Label&
LabelGD::appendPC( const char * s )
{
    for ( ; *s; ++s ) {
	if ( haveTTF && *s == '&' ) {
	    *myStrings.back() << "&#0038;";	/* Ampersand */
	} else {
	    *myStrings.back() << *s;
	}
    }
    return *this;
}


ostream&
LabelGD::print( ostream& output ) const
{
    Point aPoint = initialPoint();

    /* Now put out stuff */

    for ( int i = size()-1; i >= 0; --i ) {
	const double dy = text( aPoint, myStrings[i]->str(), 
				myFont[i], Flags::print[FONT_SIZE].value.i,
				justification(), static_cast<Graphic::colour_type>(myColour[i]) );
	aPoint.moveBy( 0, dy );
    }
    return output;
}



Point
LabelGD::initialPoint() const
{
    Point aPoint = origin;
    double dx = 0.0;

    if ( haveTTF ) {
	switch ( justification() ) {
	case LEFT_JUSTIFY:	dx =  Flags::print[FONT_SIZE].value.i / 2.0; break;
	case RIGHT_JUSTIFY:	dx = -Flags::print[FONT_SIZE].value.i / 2.0; break;
	}

	aPoint.moveBy( dx, -(((-1.2-size())*Flags::print[FONT_SIZE].value.i)/2.0) );
    } else {
	gdFont * font = getfont();

	switch ( justification() ) {
	case LEFT_JUSTIFY:	dx =  font->w / 2; break;
	case RIGHT_JUSTIFY:	dx = -font->w / 2; break;
	}
	aPoint.moveBy( dx, -((2-size())*font->h)/2 );
    }
    return aPoint;
}
#endif

Point
LabelNull::initialPoint() const
{
    Point aPoint = origin;
    return aPoint;
}

/* -------------------------------------------------------------------- */
/* PostScript output							*/
/* -------------------------------------------------------------------- */

Label&
LabelPostScript::infty()
{
    if ( !mathMode ) {
	Label::infty();
    } else {
	 *myStrings.back() << "\\245";
    }
    return *this;
}


Label&
LabelPostScript::times()
{
    if ( !mathMode ) {
	Label::times();
    } else {
	 *myStrings.back() << "\\264";
    }
    return *this;
}


/* 
 * Now put out stuff 
 */

ostream&
LabelPostScript::print( ostream& output ) const
{
    Point aPoint = initialPoint();
    for ( int i = 0; i < size(); ++i ) {
	const double dy = text( output, aPoint, myStrings[i]->str(),
				myFont[i], Flags::print[FONT_SIZE].value.i,
				justification(), static_cast<Graphic::colour_type>(myColour[i]) );
	aPoint.moveBy( 0, dy );
    }

    return output;
}



Point
LabelPostScript::initialPoint() const
{
    Point aPoint = origin;
    double dx = 0.0;
    switch ( justification() ) {
    case LEFT_JUSTIFY:	dx =  Flags::print[FONT_SIZE].value.i / 2.0; break;
    case RIGHT_JUSTIFY:	dx = -Flags::print[FONT_SIZE].value.i / 2.0; break;
    }

    aPoint.moveBy( dx, -(((1.6-size())*Flags::print[FONT_SIZE].value.i)/2.0) );
    return aPoint;
}

#if defined(SVG_OUTPUT)
/* -------------------------------------------------------------------- */
/* Scalable Vector Grahpics Ouptut					*/
/* -------------------------------------------------------------------- */

/* http://www.unicode.org/Public/UNIDATA/NamesList.txt */

Label&
LabelSVG::epsilon()
{
    *myStrings.back() << "&#0949;";	/* Greek 0x3b5 */
    return *this;
}

Label&
LabelSVG::lambda()
{
    *myStrings.back() << "&#0955;";	/* Greek 0x3bb */
    return *this;
}

Label&
LabelSVG::mu()
{
    *myStrings.back() << "&#0956;";	/* Greek 0x3bc */
    return *this;
}

Label&
LabelSVG::rho()
{
    *myStrings.back() << "&#0961;";	/* Greek 0x3c1 */
    return *this;
}

Label&
LabelSVG::times()
{
    *myStrings.back() << "&#0215;";	/* 0x00d7 */
    return *this;
}

Label&
LabelSVG::sigma()
{
    *myStrings.back() << "&#0963;";	/* Greek 0x3c3 */
    return *this;
}

Label&
LabelSVG::infty()
{
    *myStrings.back() << "&#8734;";
    return *this;
}


/*
 * Need to escape certain characters.
 */

Label&
LabelSVG::appendPC( const char * s )
{
    for ( ; *s; ++s ) {
	if ( *s == '&' ) {
	    *myStrings.back() << "&#0038;";	/* Ampersand */
	} else {
	    *myStrings.back() << *s;
	}
    }
    return *this;
}


ostream&
LabelSVG::print( ostream& output ) const
{
    Point aPoint = initialPoint();

    /* Now put out stuff */
    for ( int i = size() - 1; i >= 0; --i ) {
	const double dy = text( output, aPoint, myStrings[i]->str(),
				myFont[i], Flags::print[FONT_SIZE].value.i,
				justification(), static_cast<Graphic::colour_type>(myColour[i]) );
	aPoint.moveBy( 0, dy );
    }
    return output;
}



Point
LabelSVG::initialPoint() const
{
    Point aPoint = origin;
    double dx = 0.0;
    switch ( justification() ) {
    case LEFT_JUSTIFY:	dx =  Flags::print[FONT_SIZE].value.i / 2.0; break;
    case RIGHT_JUSTIFY:	dx = -Flags::print[FONT_SIZE].value.i / 2.0; break;
    }

    aPoint.moveBy( dx, ( (size()/2.0) * Flags::print[FONT_SIZE].value.i * SVG_SCALING) / 1.2 );
    return aPoint;
}

#endif

#if defined(SXD_OUTPUT)
/* -------------------------------------------------------------------- */
/* Open(Star)Office output						*/
/* -------------------------------------------------------------------- */

Label&
LabelSXD::epsilon()
{
    *myStrings.back() << "&#0949;";	/* Greek 0x3b5 */
    return *this;
}

Label&
LabelSXD::lambda()
{
    *myStrings.back() << "&#0955;";	/* Greek 0x3bb */
    return *this;
}

Label&
LabelSXD::mu()
{
    *myStrings.back() << "&#0956;";	/* Greek 0x3bc */
    return *this;
}

Label&
LabelSXD::rho()
{
    *myStrings.back() << "&#0961;";	/* Greek 0x3c1 */
    return *this;
}

Label&
LabelSXD::times()
{
    *myStrings.back() << "&#0215;";	/* 0x00d7 */
    return *this;
}

Label&
LabelSXD::sigma()
{
    *myStrings.back() << "&#0963;";	/* Greek 0x3c3 */
    return *this;
}

Label&
LabelSXD::infty()
{
    *myStrings.back() << "&#8734;";	/* 0x221e */
    return *this;
}


/*
 * Need to escape certain characters.
 */

Label&
LabelSXD::appendPC( const char * s )
{
    for ( ; *s; ++s ) {
	if ( *s == '&' ) {
	    *myStrings.back() << "&#0038;";	/* Ampersand */
	} else {
	    *myStrings.back() << *s;
	}
    }
    return *this;
}


ostream&
LabelSXD::print( ostream& output ) const
{
    Point boxOrigin;
    Point boxExtent;
    boundingBox( boxOrigin, boxExtent, SXD_SCALING );

    if ( boxExtent.x() == 0 || boxExtent.y() == 0 ) return output;

    SXD::begin_paragraph( output, boxOrigin, boxExtent, justification() );

    /* Now put out stuff */
    for ( int i = 0; i < size(); ++i ) {
	if ( myStrings[i]->str().size() ) {
	    text( output, myStrings[i]->str(),
		  myFont[i], Flags::print[FONT_SIZE].value.i,
		  justification(), static_cast<Graphic::colour_type>(myColour[i]) );
	}
    }

    end_paragraph( output );
    return output;
}



/*
 * Effectively No-operation because we don't need it. 
 */

Point
LabelSXD::initialPoint() const
{
    Point aPoint = origin;
    return aPoint;
}

#endif

/* -------------------------------------------------------------------- */
/* LaTeX/EEPIC output							*/
/* -------------------------------------------------------------------- */

/*
 * Insert special characters into the label.  Access the instance variable directly
 * because we don't want the "append()" function to alter the special characters in
 * the strings.
 */

Label&
LabelTeXCommon::beginMath()
{
    if ( !mathMode ) {
	*myStrings.back() << "$";
	mathMode = true;
    }
    return *this;
}

Label&
LabelTeXCommon::endMath()
{
    if ( mathMode ) {
	*myStrings.back() << "$";
	mathMode = false;
    }
    return *this;
}


/*
 * Save s in a safe format for TeX.
 */

Label&
LabelTeXCommon::appendPC( const char * s )
{
    for ( ; *s; ++s ) {
	switch ( *s ) {
	case '\\':
	case '$':
	case '&':
	case '_':
	    *myStrings.back() << '\\';
	    break;
	case ' ':
	    *myStrings.back() << '~';
	    continue;
	}
	*myStrings.back() << *s;
    }
    return *this;
}

Label&
LabelTeX::infty()
{
    if ( mathMode ) {
	*myStrings.back() << "\\infty";
    } else {
	*myStrings.back() << "$\\infty$";
    }
    return *this;
}


Label&
LabelTeX::epsilon()
{
    *myStrings.back() << "\\epsilon";
    return *this;
}


Label&
LabelTeX::lambda()
{
    *myStrings.back() << "\\lambda";
    return *this;
}


Label&
LabelTeX::mu()
{
    *myStrings.back() << "\\mu";
    return *this;
}


Label&
LabelTeX::rho()
{
    *myStrings.back() << "\\rho";
    return *this;
}


Label&
LabelTeX::sigma()
{
    *myStrings.back() << "\\sigma^{2}";
    return *this;
}


Label&
LabelTeX::times()
{
    *myStrings.back() << "\\times ";
    return *this;
}

ostream&
LabelTeX::print( ostream& output ) const
{
    Point aPoint = initialPoint();

    for ( int i = 0; i < size(); ++i ) {
	const double dy = text( output, aPoint, myStrings[i]->str(),
				myFont[i], Flags::print[FONT_SIZE].value.i,
				justification(), static_cast<Graphic::colour_type>(myColour[i]) );
	aPoint.moveBy( 0, dy );
    }
    return output;
}




Point
LabelTeX::initialPoint() const
{
    Point aPoint = origin;
    double dx = 0.0;
    switch ( justification() ) {
    case LEFT_JUSTIFY:	dx =  Flags::print[FONT_SIZE].value.i / 2.0 * EEPIC_SCALING; break;
    case RIGHT_JUSTIFY:	dx = -Flags::print[FONT_SIZE].value.i / 2.0 * EEPIC_SCALING; break;
    }

    aPoint.moveBy( dx, -(((1-size())*Flags::print[FONT_SIZE].value.i)/2. + 1.) * EEPIC_SCALING);
    return aPoint;
}

Label&
LabelPsTeX::infty()
{
    if ( mathMode ) {
	*Label::myStrings.back() << "\\\\infty";
    } else {
	*Label::myStrings.back() << "$\\\\infty$";
    }
    return *this;
}


Label&
LabelPsTeX::epsilon()
{
    *myStrings.back() << "\\\\epsilon";
    return *this;
}


Label&
LabelPsTeX::lambda()
{
    *myStrings.back() << "\\\\lambda";
    return *this;
}


Label&
LabelPsTeX::mu()
{
    *myStrings.back() << "\\\\mu";
    return *this;
}


Label&
LabelPsTeX::rho()
{
    *myStrings.back() << "\\\\rho";
    return *this;
}


Label&
LabelPsTeX::sigma()
{
    *myStrings.back() << "\\\\sigma^{2}";
    return *this;
}


Label&
LabelPsTeX::times()
{
    *myStrings.back() << "\\\\times ";
    return *this;
}

ostream&
LabelPsTeX::print( ostream& output ) const
{
    Point boxOrigin;
    Point boxExtent;		/* A guess... width is half of height */
    boundingBox( boxOrigin, boxExtent, FIG_SCALING );

    if ( boxExtent.x() == 0 || boxExtent.y() == 0 ) return output;

    startCompound( output, boxOrigin, boxExtent );
    if ( Flags::clear_label_background  ) {
	Fig::clearBackground( output, boxOrigin, boxExtent, backgroundColour() );
    }

    /* Now put out stuff */

    Point aPoint = initialPoint();
    for ( int i = 0; i < size(); ++i ) {
	text( output, aPoint, myStrings[i]->str(), myFont[i],
	      Flags::print[FONT_SIZE].value.i, justification(), static_cast<Graphic::colour_type>(myColour[i]),
	      Fig::SPECIAL );
	aPoint.moveBy( 0, Flags::print[FONT_SIZE].value.i * FIG_SCALING );
    }

    endCompound( output );
    return output;
}



Point
LabelPsTeX::initialPoint() const
{
    Point aPoint = origin;
    double dx = 0.0;
    switch ( justification() ) {
    case LEFT_JUSTIFY:
	dx =  Flags::print[FONT_SIZE].value.i / 2.0 * FIG_SCALING;
	break;
    case RIGHT_JUSTIFY:
	dx = -Flags::print[FONT_SIZE].value.i / 2.0 * FIG_SCALING;
	break;
    }
    aPoint.moveBy( dx, (((1-size())*Flags::print[FONT_SIZE].value.i)/2 + 2) * FIG_SCALING );
    return aPoint;
}

static Label&
beginMathFunc( Label& aLabel, labelFuncPtr aFunc )
{
    aLabel.beginMath();
    if ( aFunc ) {
	(aLabel.*aFunc)();
    }
    return aLabel;
}

static Label&
endMathFunc( Label& aLabel, labelFuncPtr )
{
    aLabel.endMath();
    return aLabel;
}

static Label&
mathFunc( Label& aLabel, labelFuncPtr aFunc )
{
    (aLabel.*aFunc)();
    return aLabel;
}

LabelManip begin_math( labelFuncPtr aFunc ) {  return LabelManip( &beginMathFunc, aFunc ); }
LabelManip end_math() { return LabelManip( &endMathFunc, 0 ); }
LabelManip _epsilon() { return LabelManip( &mathFunc, &Label::epsilon ); }
LabelManip _infty() { return LabelManip( &mathFunc, &Label::infty ); }
LabelManip _lambda() { return LabelManip( &mathFunc, &Label::lambda ); }
LabelManip _mu() { return LabelManip( &mathFunc, &Label::mu ); }
LabelManip _percent() { return LabelManip( &mathFunc, &Label::percent ); }
LabelManip _rho() { return LabelManip( &mathFunc, &Label::rho ); }
LabelManip _sigma() { return LabelManip( &mathFunc, &Label::sigma ); }
LabelManip _times() { return LabelManip( &mathFunc, &Label::times ); }
