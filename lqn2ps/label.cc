/* label.cc	-- Greg Franks Wed Jan 29 2003
 * 
 * $Id: label.cc 14139 2020-11-25 18:38:03Z greg $
 */

#include "lqn2ps.h"
#include <algorithm>
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
    LabelStringManip( std::ostream& (*ff)(std::ostream&, const char * ), 
		      const char * aStr  )
	: f(ff), myStr(aStr) {}
private:
    std::ostream& (*f)( std::ostream&, const char * );
    const char * myStr;

    friend std::ostream& operator<<(std::ostream & os, const LabelStringManip& m )
	{ return m.f(os,m.myStr); }
};

Label::Line::Line() : _font(NORMAL_FONT), _colour(DEFAULT_COLOUR), _string()
{
    if ( Flags::print[PRECISION].value.i > 0 ) {
	_string.precision( Flags::print[PRECISION].value.i );
    }
}

Label::Line::Line( const Line& src ) : _font(src._font), _colour(src._colour), _string() 
{
    if ( Flags::print[PRECISION].value.i > 0 ) {
	_string.precision( Flags::print[PRECISION].value.i );
    }
    _string << src.getStr(); 
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
    default:
	return new LabelNull();
    }
    abort();
    return nullptr;
}

Label::Label() 
    : Graphic(), 
      _origin(),
      _lines(1),
      _backgroundColour(TRANSPARENT), 
      _justification(CENTER_JUSTIFY), 
      _mathMode(false)
{
}


Label::Label( const Point& aPoint ) 
    : Graphic(), 
      _origin(aPoint),
      _lines(1),
      _backgroundColour(TRANSPARENT), 
      _justification(CENTER_JUSTIFY), 
      _mathMode(false)
{
}

Label&
Label::scaleBy( const double sx, const double sy )
{	
    _origin.x( sx * _origin.x() );
    _origin.y( sy * _origin.y() );
    return *this;
}


Label&
Label::translateY( const double dy )
{
    _origin.y( dy - _origin.y() );
    return *this;
}


Label&
Label::newLine()
{
    _mathMode = false;
    _lines.push_back( Line() );
    return *this;
}



Label&
Label::font( const font_type font )
{
    _lines.back().setFont(font);
    return *this;
}


Label&
Label::colour( const colour_type colour )
{
    _lines.back().setColour( colour );
    return *this;
}


double 
Label::width() const
{
    return for_each( _lines.begin(), _lines.end(), Width( 0 ) ).width() * normalized_font_size() / 2.0;		// A guess.
}



double 
Label::height() const
{
    return size() * Flags::print[FONT_SIZE].value.i;
}


Label&
Label::appendLSM( const LabelStringManip& m)
{ 
    _lines.back() << m; 
    return *this;
}


Label&
Label::appendSCM( const SRVNCallManip& m )
{ 
    _lines.back() << m; 
    return *this;
}


Label&
Label::appendSEM( const SRVNEntryManip& m)
{ 
    _lines.back() << m; 
    return *this;
}


Label&
Label::appendSTM( const TaskCallManip& m )
{ 
    _lines.back() << m; 
    return *this;
}


Label&
Label::appendDM( const DoubleManip& m )
{ 
    _lines.back() << m; 
    return *this;
}


Label& 
Label::appendD( const double aDouble )
{
    if ( !std::isfinite( aDouble ) ) {
	if ( aDouble < 0.0 ) {
	    _lines.back() << '-'; 
	}
	infty();
    } else {
	_lines.back() << aDouble; 
    }
    return *this; 
}

Label&
Label::appendV( const LQIO::DOM::ExternalVariable& v ) 
{
    if ( Flags::instantiate ) {
	appendD( to_double( v ) );
    } else {
	std::stringstream s;		/* For Unicode - We need to expand the string */
	s << v;
	appendS( s.str() );
    }
    return *this;
}

Label& 
Label::beginMath()
{
    if ( !_mathMode ) {
	font(Graphic::SYMBOL_FONT);
	_mathMode = true;
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
    boxOrigin = _origin;
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
    return for_each( _lines.begin(), _lines.end(), Width( 0 ) ).width() * normalized_font_size() / 4.5;		// A guess.
}



/*
 * EMF uses UNICode.  Little Endian
 */

Label& 
LabelEMF::epsilon()
{
    _lines.back() << static_cast<char>(0x03) << static_cast<char>(0xb5);	/* Greek 0x3b5 */
    return *this;
}

Label&
LabelEMF::lambda()
{
    _lines.back() << static_cast<char>(0x03) << static_cast<char>(0xbb);	/* Greek 0x3bb */
    return *this;
}

Label&
LabelEMF::mu()
{
    _lines.back() << static_cast<char>(0x03) << static_cast<char>(0xbc);	/* Greek 0x3bc */
    return *this;
}

Label&
LabelEMF::rho()
{
    _lines.back() << static_cast<char>(0x03) << static_cast<char>(0xc1);	/* Greek 0x3c1 */
    return *this;
}

Label&
LabelEMF::times()
{
    _lines.back() << static_cast<char>(0x00) << static_cast<char>(0xd7);	/* 0x00d7 */
    return *this;
}

Label&
LabelEMF::sigma()
{
    _lines.back() << static_cast<char>(0x03) << static_cast<char>(0xc3);	/* Greek 0x3c3 */
    return *this;
}

Label&
LabelEMF::infty()
{
    _lines.back() << static_cast<char>(0x22) << static_cast<char>(0x1e);
    return *this;
}


/*
 * Convert to Unicode.
 */

Label&
LabelEMF::appendC( const char c )
{
    _lines.back() << static_cast<char>(0x00) << c;
    return *this;
}


/*
 * Convert to Unicode.
 */

Label&
LabelEMF::appendPC( const char * s )
{
    for ( ; *s; ++s ) {
	_lines.back() << static_cast<char>(0x00) << *s;
    }
    return *this;
}


/*
 * Convert to Unicode.
 */

Label&
LabelEMF::appendS( const std::string& s )
{
    for ( unsigned i = 0; i < s.size(); ++i ) {
	_lines.back() << static_cast<char>(0x00) << s[i];
    }
    return *this;
}


/*
 * We have to convert to unicode.  Ugh.
 */

Label& 
LabelEMF::appendD( const double aDouble )
{
    if ( !std::isfinite( aDouble ) ) {
	if ( aDouble < 0.0 ) {
	    appendC( '-' ); 
	}
	infty();
    } else {
	std::ostringstream s;
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
    std::ostringstream s;
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
    std::ostringstream s;
    s << anInt;
    appendS( s.str() );
    return *this; 
}


Label&
LabelEMF::appendLSM( const LabelStringManip& aManip )
{
    std::ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


Label&
LabelEMF::appendSEM( const SRVNEntryManip& aManip )
{
    std::ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


Label&
LabelEMF::appendSCM( const SRVNCallManip& aManip )
{
    std::ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


Label&
LabelEMF::appendSTM( const TaskCallManip& aManip )
{
    std::ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


Label&
LabelEMF::appendDM( const DoubleManip& aManip )
{
    std::ostringstream s;
    s << aManip;
    appendS( s.str() );
    return *this;
}


const LabelEMF&
LabelEMF::draw( std::ostream& output ) const
{
    Point boxOrigin;
    Point boxExtent;
    boundingBox( boxOrigin, boxExtent, EMF_SCALING );

    if ( boxExtent.x() == 0 || boxExtent.y() == 0 ) return *this;

    /* Now put out stuff */
    for_each( _lines.begin(), _lines.end(), DrawText<LabelEMF>( output, *this, &LabelEMF::text, initialPoint(), justification() ) );
    return *this;
}


Point
LabelEMF::initialPoint() const
{
    Point aPoint = _origin;
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
    if ( !_mathMode ) {
	Label::infty();
    } else {
	 _lines.back() << "\\245";
    }
    return *this;
}


Label&
LabelFig::times()
{
    if ( !_mathMode ) {
	Label::times();
    } else {
	 _lines.back() << "\\264";
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
	    _lines.back() << '\\';		/* Tack on another Backslash */
	}
	_lines.back() << *s;
    }
    return *this;
}


std::ostream&
LabelFig::comment( std::ostream& output, const std::string& aString ) const
{
    output << "# " << aString << std::endl;
    return output;
}


const LabelFig&
LabelFig::draw( std::ostream& output ) const
{
    Point boxOrigin;
    Point boxExtent;		/* A guess... width is half of height */
    boundingBox( boxOrigin, boxExtent, FIG_SCALING );

    if ( boxExtent.x() == 0 || boxExtent.y() == 0 ) return *this;

    startCompound( output, boxOrigin, boxExtent );
    if ( Flags::clear_label_background ) {
	Fig::clearBackground( output, boxOrigin, boxExtent, backgroundColour() );
    }

    /* Now put out stuff */
    for_each( _lines.begin(), _lines.end(), DrawText<LabelFig>( output, *this, &LabelFig::text, initialPoint(), justification(), Fig::POSTSCRIPT ) );

    endCompound( output );
    return *this;
}


Point
LabelFig::initialPoint() const
{
    Point aPoint = _origin;
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
    return for_each( _lines.begin(), _lines.end(), Width( 0 ) ).width();		// A guess.
}



Label&
LabelGD::lambda()
{
    if ( haveTTF && _mathMode ) {
//	_lines.back() << "&#0955;";	/* Greek 0x3bb */
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
    if ( haveTTF && _mathMode ) {
//	_lines.back() << "&#0956;";	/* Greek 0x3bc */
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
    if ( haveTTF && _mathMode ) {
//	_lines.back() << "&#0956;";	/* Greek 0x3bc */
	(*this) << "U";
    } else {
	(*this) << "U";
    }
    return *this;
}


Label&
LabelGD::times()
{
    if ( haveTTF && _mathMode ) {
//	_lines.back() << "&#0215;";	/* 0x00d7 */
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
//	_lines.back() << "&#0955;";	/* Greek 0x3bb */
	Label::sigma();
    } else {
	Label::sigma();
    }
    return *this;
}


Label&
LabelGD::infty()
{
    if ( haveTTF && _mathMode ) {
//	_lines.back() << "&#8734;";	
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
	    _lines.back() << "&#0038;";	/* Ampersand */
	} else {
	    _lines.back() << *s;
	}
    }
    return *this;
}


const LabelGD&
LabelGD::draw( std::ostream& output ) const
{
    for_each( _lines.rbegin(), _lines.rend(), DrawText<LabelGD>( output, *this, &LabelGD::text, initialPoint(), justification() ) );
    return *this;
}



Point
LabelGD::initialPoint() const
{
    Point aPoint = _origin;
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
    Point aPoint = _origin;
    return aPoint;
}

/* -------------------------------------------------------------------- */
/* PostScript output							*/
/* -------------------------------------------------------------------- */

Label&
LabelPostScript::infty()
{
    if ( !_mathMode ) {
	Label::infty();
    } else {
	 _lines.back() << "\\245";
    }
    return *this;
}


Label&
LabelPostScript::times()
{
    if ( !_mathMode ) {
	Label::times();
    } else {
	 _lines.back() << "\\264";
    }
    return *this;
}


/* 
 * Now put out stuff 
 */

const LabelPostScript&
LabelPostScript::draw( std::ostream& output ) const
{
    for_each( _lines.begin(), _lines.end(), DrawText<LabelPostScript>( output, *this, &LabelPostScript::text, initialPoint(), justification() ) );
    return *this;
}



Point
LabelPostScript::initialPoint() const
{
    Point aPoint = _origin;
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
    _lines.back() << "&#0949;";	/* Greek 0x3b5 */
    return *this;
}

Label&
LabelSVG::lambda()
{
    _lines.back() << "&#0955;";	/* Greek 0x3bb */
    return *this;
}

Label&
LabelSVG::mu()
{
    _lines.back() << "&#0956;";	/* Greek 0x3bc */
    return *this;
}

Label&
LabelSVG::rho()
{
    _lines.back() << "&#0961;";	/* Greek 0x3c1 */
    return *this;
}

Label&
LabelSVG::times()
{
    _lines.back() << "&#0215;";	/* 0x00d7 */
    return *this;
}

Label&
LabelSVG::sigma()
{
    _lines.back() << "&#0963;";	/* Greek 0x3c3 */
    return *this;
}

Label&
LabelSVG::infty()
{
    _lines.back() << "&#8734;";
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
	    _lines.back() << "&#0038;";	/* Ampersand */
	} else {
	    _lines.back() << *s;
	}
    }
    return *this;
}


const LabelSVG&
LabelSVG::draw( std::ostream& output ) const
{
    for_each( _lines.rbegin(), _lines.rend(), DrawText<LabelSVG>( output, *this, &LabelSVG::text, initialPoint(), justification() ) );
    return *this;
}



Point
LabelSVG::initialPoint() const
{
    Point aPoint = _origin;
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
    _lines.back() << "&#0949;";	/* Greek 0x3b5 */
    return *this;
}

Label&
LabelSXD::lambda()
{
    _lines.back() << "&#0955;";	/* Greek 0x3bb */
    return *this;
}

Label&
LabelSXD::mu()
{
    _lines.back() << "&#0956;";	/* Greek 0x3bc */
    return *this;
}

Label&
LabelSXD::rho()
{
    _lines.back() << "&#0961;";	/* Greek 0x3c1 */
    return *this;
}

Label&
LabelSXD::times()
{
    _lines.back() << "&#0215;";	/* 0x00d7 */
    return *this;
}

Label&
LabelSXD::sigma()
{
    _lines.back() << "&#0963;";	/* Greek 0x3c3 */
    return *this;
}

Label&
LabelSXD::infty()
{
    _lines.back() << "&#8734;";	/* 0x221e */
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
	    _lines.back() << "&#0038;";	/* Ampersand */
	} else {
	    _lines.back() << *s;
	}
    }
    return *this;
}


const LabelSXD&
LabelSXD::draw( std::ostream& output ) const
{
    Point boxOrigin;
    Point boxExtent;
    boundingBox( boxOrigin, boxExtent, SXD_SCALING );

    if ( boxExtent.x() == 0 || boxExtent.y() == 0 ) return *this;

    /* Now put out stuff */
    SXD::begin_paragraph( output, boxOrigin, boxExtent, justification() );
    for_each( _lines.begin(), _lines.end(), DrawText<LabelSXD>( output, *this, &LabelSXD::text, initialPoint(), justification() ) );
    end_paragraph( output );

    return *this;
}



/*
 * Effectively No-operation because we don't need it. 
 */

Point
LabelSXD::initialPoint() const
{
    Point aPoint = _origin;
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
    if ( !_mathMode ) {
	_lines.back() << "$";
	_mathMode = true;
    }
    return *this;
}

Label&
LabelTeXCommon::endMath()
{
    if ( _mathMode ) {
	_lines.back() << "$";
	_mathMode = false;
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
	    _lines.back() << '\\';
	    break;
	case ' ':
	    _lines.back() << '~';
	    continue;
	}
	_lines.back() << *s;
    }
    return *this;
}

Label&
LabelTeX::infty()
{
    if ( _mathMode ) {
	_lines.back() << "\\infty";
    } else {
	_lines.back() << "$\\infty$";
    }
    return *this;
}


Label&
LabelTeX::epsilon()
{
    _lines.back() << "\\epsilon";
    return *this;
}


Label&
LabelTeX::lambda()
{
    _lines.back() << "\\lambda";
    return *this;
}


Label&
LabelTeX::mu()
{
    _lines.back() << "\\mu";
    return *this;
}


Label&
LabelTeX::rho()
{
    _lines.back() << "\\rho";
    return *this;
}


Label&
LabelTeX::sigma()
{
    _lines.back() << "\\sigma^{2}";
    return *this;
}


Label&
LabelTeX::times()
{
    _lines.back() << "\\times ";
    return *this;
}

const LabelTeX&
LabelTeX::draw( std::ostream& output ) const
{
    for_each( _lines.begin(), _lines.end(), DrawText<LabelTeX>( output, *this, &LabelTeX::text, initialPoint(), justification() ) );
    return *this;
}




Point
LabelTeX::initialPoint() const
{
    Point aPoint = _origin;
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
    if ( _mathMode ) {
	_lines.back() << "\\\\infty";
    } else {
	_lines.back() << "$\\\\infty$";
    }
    return *this;
}


Label&
LabelPsTeX::epsilon()
{
    _lines.back() << "\\\\epsilon";
    return *this;
}


Label&
LabelPsTeX::lambda()
{
    _lines.back() << "\\\\lambda";
    return *this;
}


Label&
LabelPsTeX::mu()
{
    _lines.back() << "\\\\mu";
    return *this;
}


Label&
LabelPsTeX::rho()
{
    _lines.back() << "\\\\rho";
    return *this;
}


Label&
LabelPsTeX::sigma()
{
    _lines.back() << "\\\\sigma^{2}";
    return *this;
}


Label&
LabelPsTeX::times()
{
    _lines.back() << "\\\\times ";
    return *this;
}

const LabelPsTeX&
LabelPsTeX::draw( std::ostream& output ) const
{
    Point boxOrigin;
    Point boxExtent;		/* A guess... width is half of height */
    boundingBox( boxOrigin, boxExtent, FIG_SCALING );

    if ( boxExtent.x() == 0 || boxExtent.y() == 0 ) return *this;

    startCompound( output, boxOrigin, boxExtent );
    if ( Flags::clear_label_background  ) {
	Fig::clearBackground( output, boxOrigin, boxExtent, backgroundColour() );
    }

    /* Now put out stuff */

    for_each( _lines.begin(), _lines.end(), DrawText<LabelPsTeX>( output, *this, &LabelPsTeX::text, initialPoint(), justification(), Fig::SPECIAL ) );

    endCompound( output );
    return *this;
}



Point
LabelPsTeX::initialPoint() const
{
    Point aPoint = _origin;
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

Label::Line&
Label::Line::operator=( const Line& src )
{
    if ( &src == this ) return *this;
    _font = src._font;
    _colour = src._colour;
    _string.seekp(0);		// rewind.
    _string << src.getStr();
    return *this;
}

Label::Line& Label::Line::operator<<( const LabelStringManip& m) { _string << m; return *this; }
Label::Line& Label::Line::operator<<( const SRVNCallManip& m ) { _string << m; return *this; }
Label::Line& Label::Line::operator<<( const SRVNEntryManip& m) { _string << m; return *this; }
Label::Line& Label::Line::operator<<( const TaskCallManip& m ) { _string << m; return *this; }
Label::Line& Label::Line::operator<<( const DoubleManip& m ) { _string << m; return *this; }

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
