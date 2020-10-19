/* -*- c++ -*- node.h	-- Greg Franks
 *
 * $Id: label.h 13936 2020-10-16 14:31:26Z greg $
 */

#ifndef _LABEL_H
#define _LABEL_H

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <sstream>
#include "point.h"
#include "graphic.h"
#include <vector>

class Call;
class Label;
class LabelStringManip;
class SRVNEntryManip;
class SRVNCallManip;
class TaskCallManip;

class Label : public Graphic
{
public:
    class Line {
    public:
	Line();
	Line( const Line& src );
	Line& operator=( const Line& src );
	Line& operator<<( const LabelStringManip& m);
	Line& operator<<( const SRVNCallManip& m );
	Line& operator<<( const SRVNEntryManip& m);
	Line& operator<<( const TaskCallManip& m );
	Line& operator<<( const DoubleManip& m );
	Line& operator<<( const char * s ) { _string << s; return *this; }
	Line& operator<<( const std::string& s ) { _string << s; return *this; }
	Line& operator<<( const char c ) { _string << c; return *this; }
	Line& operator<<( const double d ) { _string << d; return *this; }
	Line& operator<<( const int i ) { _string << i; return *this; }
	Line& operator<<( const unsigned int u ) { _string << u; return *this; }
	size_t width() const { return _string.str().length(); }
	Line& setFont( const font_type font ) { _font = font; return *this; }
	Line& setColour( const colour_type colour ) { _colour = colour; return *this; }
	const std::string getStr() const { return _string.str(); }
	Graphic::font_type getFont() const { return _font; }
	Graphic::colour_type getColour() const { return _colour; }
    private:
	Graphic::font_type _font;
	Graphic::colour_type _colour;
	ostringstream _string;
    };

protected:
    class Width {
    public:
	Width( size_t w ) : _w(w) {}
	void operator()( const Line& line ) { _w = max( _w, line.width() ); }
	size_t width() const { return _w; }
    private:
	size_t _w;
    };

private:
    Label( const Label& );
    Label& operator=( const Label& );

public:
    Label();
    explicit Label( const Point& aPoint );

    static Label * newLabel();

    Label& operator<<( const LabelStringManip& m) { return appendLSM( m ); }
    Label& operator<<( const SRVNCallManip& m ) { return appendSCM( m ); }
    Label& operator<<( const SRVNEntryManip& m) { return appendSEM( m ); }
    Label& operator<<( const TaskCallManip& m ) { return appendSTM( m ); }
    Label& operator<<( const DoubleManip& m ) { return appendDM( m ); }
    Label& operator<<( const char * s ) { return appendPC( s ); }
    Label& operator<<( const char c ) { return appendC( c ); }
    Label& operator<<( const double d ) { return appendD( d ); }
    Label& operator<<( const int i ) { return appendI( i ); }
    Label& operator<<( const string& s ) { return appendS( s ); }
    Label& operator<<( const unsigned i ) { return appendUI( i ); }
    Label& operator<<( const LQIO::DOM::ExternalVariable& v ) { return appendV( v ); }

    int size() const { return _lines.size(); }

    virtual Label& moveTo( const double x, const double y ) { _origin.moveTo( x, y ); return *this; }
    virtual Label& moveTo( const Point& aPoint ) { _origin.moveTo( aPoint.x(), aPoint.y() ); return *this; }
    virtual Label& moveBy( const double x, const double y ) { _origin.moveBy( x, y ); return *this; }
    virtual Label& moveBy( const Point& aPoint ) { _origin.moveBy( aPoint.x(), aPoint.y() ); return *this; }
    virtual Label& scaleBy( const double, const double );
    virtual Label& translateY( const double );

    virtual Label& newLine();

    Label& font( const font_type );
    Label& colour( const colour_type );
    Label& backgroundColour( const colour_type aColour ) { _backgroundColour = aColour; return *this; }
    colour_type backgroundColour() const { return _backgroundColour; }
    Label& justification( const justification_type justify ) { _justification = justify; return *this; }
    justification_type justification() const { return _justification; }

    virtual double width() const;
    virtual double height() const;

    virtual Label& beginMath();
    virtual Label& endMath();
    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    Label& percent();

    virtual const Label& draw( ostream& ) const = 0;
    virtual ostream& comment( ostream& output, const string& ) const { return output; }

protected:
    virtual Label& appendLSM( const LabelStringManip& m);
    virtual Label& appendSCM( const SRVNCallManip& m );
    virtual Label& appendSEM( const SRVNEntryManip& m);
    virtual Label& appendSTM( const TaskCallManip& m );
    virtual Label& appendDM( const DoubleManip& m );
    virtual Label& appendPC( const char * s ) { _lines.back() << s; return *this; }
    virtual Label& appendC( const char c ) { _lines.back() << c; return *this; }
    virtual Label& appendD( const double );
    virtual Label& appendI( const int i ) { _lines.back() << i; return *this; }
    virtual Label& appendS( const string& s ) { _lines.back() << s; return *this; }
    virtual Label& appendUI( const unsigned i ) { _lines.back() << i; return *this; }
    virtual Label& appendV( const LQIO::DOM::ExternalVariable& v );

    const Label& boundingBox( Point& boxOrigin, Point& boxExtent, const double scaling ) const;
    virtual Point initialPoint() const = 0;

protected:
    Point _origin;
    vector<Line> _lines;
    colour_type _backgroundColour;
    justification_type _justification;
    bool _mathMode;
};

inline ostream& operator<<( ostream& output, const Label& self ) { self.draw( output ); return output; }

#if defined(EMF_OUTPUT)
class LabelEMF : public Label, private EMF
{
public:
    virtual double width() const;

    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelEMF& draw( ostream& ) const;

protected:
    virtual Label& appendLSM( const LabelStringManip& m);
    virtual Label& appendSCM( const SRVNCallManip& m );
    virtual Label& appendSEM( const SRVNEntryManip& m);
    virtual Label& appendSTM( const TaskCallManip& m );
    virtual Label& appendDM( const DoubleManip& m );
    virtual Label& appendPC( const char * s );
    virtual Label& appendC( const char c );
    virtual Label& appendD( const double );
    virtual Label& appendI( const int i );
    virtual Label& appendS( const string& s );
    virtual Label& appendUI( const unsigned i );
    virtual Point initialPoint() const;
};
#endif

class LabelFig : public Label, private Fig
{
public:
    virtual Label& infty();
    virtual Label& times();
    virtual ostream& comment( ostream& output, const string& ) const;
    virtual const LabelFig& draw( ostream& ) const;

protected:
    virtual Label& appendPC( const char * s );
    virtual Point initialPoint() const;
};

#if HAVE_GD_H && HAVE_LIBGD
class LabelGD : public Label, private GD
{
public:
    virtual double width() const;

    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual Label& appendPC( const char * s );

    virtual const LabelGD& draw( ostream& output ) const;

protected:
    virtual Point initialPoint() const;
};
#endif

class LabelNull : public Label
{
public:
    virtual const LabelNull& draw( ostream& output ) const { return *this; }

protected:
    virtual Point initialPoint() const;
};

class LabelPostScript : public Label, private PostScript
{
public:
    virtual Label& infty();
    virtual Label& times();
    virtual const LabelPostScript& draw( ostream& ) const;

protected:
    virtual Point initialPoint() const;
};

#if defined(SVG_OUTPUT)
class LabelSVG : public Label, private SVG
{
public:
    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelSVG& draw( ostream& ) const;

protected:
    virtual Label& appendPC( const char * s );
    virtual Point initialPoint() const;
};
#endif

#if defined(SXD_OUTPUT)
class LabelSXD : public Label, private SXD
{
public:
    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelSXD& draw( ostream& ) const;

protected:
    virtual Label& appendPC( const char * s );
    virtual Point initialPoint() const;
};
#endif

#if defined(X11_OUTPUT)
class LabelX11 : public Label, private X11
{
public:
    virtual const LabelX11& draw( ostream& output ) const { return output; }
};
#endif

class LabelTeXCommon: virtual public Label
{
public:
    virtual Label& beginMath();
    virtual Label& endMath();

protected:
    virtual Label& appendPC( const char * s );
};

class LabelTeX : public LabelTeXCommon, private TeX
{
public:
    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelTeX& draw( ostream& ) const;

protected:
    virtual Point initialPoint() const;
};

class LabelPsTeX : public LabelTeXCommon, public Fig
{
public:
    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelPsTeX& draw( ostream& output ) const;

protected:
    virtual Point initialPoint() const;
};

template <class Type1> class DrawText {
public:
    typedef double (Type1::*textFPtr)( ostream& output, const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
				       justification_type justification, Graphic::colour_type colour, unsigned flags ) const;
    DrawText( ostream& output, const Type1& self, const textFPtr text, Point point, const justification_type j, unsigned flags=0 ) : _output(output), _self(self), _text(text), _point(point), _justification(j), _flags(flags) {}
    void operator()( const Label::Line& line ) {
	const std::string& str = line.getStr();
	if ( str.size() ) {
	    _point.moveBy( 0, (_self.*_text)( _output, _point, str, line.getFont(), Flags::print[FONT_SIZE].value.i, _justification, line.getColour(), _flags ) );
	}
    }
private:
    ostream& _output;
    const Type1& _self;
    const textFPtr _text;
    Point _point;
    const justification_type _justification;
    const unsigned int _flags;
};

#if HAVE_GD_H && HAVE_LIBGD
template<> class DrawText<LabelGD> {
public:
    typedef double (LabelGD::*textFPtr)( const Point& c, const std::string& s, Graphic::font_type font, int fontsize,
					 justification_type justification, Graphic::colour_type colour  ) const;
    DrawText<LabelGD>( ostream&, const LabelGD& self, const textFPtr text, Point point, const justification_type j, unsigned flags=0 ) : _self(self), _text(text), _point(point), _justification(j) {}
    void operator()( const Label::Line& line ) {
	const std::string& str = line.getStr();
	if ( str.size() ) {
	    _point.moveBy( 0, (_self.*_text)( _point, str, line.getFont(), Flags::print[FONT_SIZE].value.i, _justification, line.getColour() ) );
	}
    }
private:
    const LabelGD& _self;
    const textFPtr _text;
    Point _point;
    const justification_type _justification;
};
#endif    

inline Label& newLine( Label& aLabel ) { return aLabel.newLine(); }

typedef Label& (Label::*labelFuncPtr)();

class LabelManip {
public:
    LabelManip( Label& (*ff)(Label&, labelFuncPtr ), 
		    labelFuncPtr aFunc  )
	: f(ff), myFunc(aFunc) {}
private:
    Label& (*f)( Label&, labelFuncPtr );
    const labelFuncPtr myFunc;

    friend Label& operator<<(Label &l, const LabelManip& m )
	{ return m.f(l,m.myFunc); }
};

class LabelCallManip {
public:
    LabelCallManip( Label& (*ff)(Label&, const Call & ), const Call & aCall  )
	: f(ff), myCall(aCall) {}
private:
    Label& (*f)( Label&, const Call & );
    const Call & myCall;

    friend Label& operator<<(Label & aLabel, const LabelCallManip& m )
	{ return m.f(aLabel,m.myCall); }
};

class LabelEntryManip {
public:
    LabelEntryManip( Label& (*ff)(Label&, const Entry & ), const Entry & anEntry  )
	: f(ff), myEntry(anEntry) {}
private:
    Label& (*f)( Label&, const Entry & );
    const Entry & myEntry;

    friend Label& operator<<(Label & aLabel, const LabelEntryManip& m )
	{ return m.f(aLabel,m.myEntry); }
};

LabelManip begin_math( labelFuncPtr = 0 );
LabelManip end_math();
LabelManip _epsilon();
LabelManip _infty();
LabelManip _lambda();
LabelManip _mu();
LabelManip _percent();
LabelManip _rho();
LabelManip _sigma();
LabelManip _times();
#endif
