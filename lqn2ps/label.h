/* -*- c++ -*- node.h	-- Greg Franks
 *
 * $Id: label.h 15612 2022-06-01 01:06:26Z greg $
 */

#ifndef _LABEL_H
#define _LABEL_H

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <sstream>
#include <vector>
#include "point.h"
#include "graphic.h"

class Call;
class Entry;
class Label;
class LabelStringManip;
class SRVNEntryManip;
class SRVNCallManip;
class TaskCallManip;

class Label : public Graphic
{
public:
    typedef Label * (*create_func)();
    
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
	Line& setFont( const Font font ) { _font = font; return *this; }
	Line& setColour( const Colour colour ) { _colour = colour; return *this; }
	const std::string getStr() const { return _string.str(); }
	Graphic::Font getFont() const { return _font; }
	Graphic::Colour getColour() const { return _colour; }
    private:
	Graphic::Font _font;
	Graphic::Colour _colour;
	std::ostringstream _string;
    };

protected:
    class Width {
    public:
	Width( size_t w ) : _w(w) {}
	void operator()( const Line& line ) { _w = std::max( _w, line.width() ); }
	size_t width() const { return _w; }
    private:
	size_t _w;
    };

private:
    Label& operator=( const Label& );

protected:
    Label( const Label& );

public:
    Label();
    explicit Label( const Point& aPoint );
    virtual Label * clone() = 0;

    static Label * newLabel();

    Label& operator<<( const LabelStringManip& m) { return appendLSM( m ); }
    Label& operator<<( const SRVNCallManip& m ) { return appendSCM( m ); }
    Label& operator<<( const SRVNEntryManip& m) { return appendSEM( m ); }
    Label& operator<<( const TaskCallManip& m ) { return appendSTM( m ); }
    Label& operator<<( const DoubleManip& m ) { return appendDM( m ); }
    Label& operator<<( const char * s ) { return appendPC( static_cast<const char *>(s) ); }
    Label& operator<<( const char c ) { return appendC( c ); }
    Label& operator<<( const double d ) { return appendD( d ); }
    Label& operator<<( const int i ) { return appendI( i ); }
    Label& operator<<( const std::string& s ) { return appendS( s ); }
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

    Label& font( const Font );
    Label& colour( const Colour );
    Label& backgroundColour( const Colour aColour ) { _backgroundColour = aColour; return *this; }
    Colour backgroundColour() const { return _backgroundColour; }
    Label& justification( const Justification justify ) { _justification = justify; return *this; }
    Justification justification() const { return _justification; }

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

    virtual const Label& draw( std::ostream& ) const = 0;
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const { return output; }

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
    virtual Label& appendS( const std::string& s ) { _lines.back() << s; return *this; }
    virtual Label& appendUI( const unsigned i ) { _lines.back() << i; return *this; }
    virtual Label& appendV( const LQIO::DOM::ExternalVariable& v );

    const Label& boundingBox( Point& boxOrigin, Point& boxExtent, const double scaling ) const;
    virtual Point initialPoint() const = 0;

protected:
    Point _origin;
    std::vector<Line> _lines;
    Colour _backgroundColour;
    Justification _justification;
    bool _mathMode;
};

inline std::ostream& operator<<( std::ostream& output, const Label& self ) { self.draw( output ); return output; }

#if defined(EMF_OUTPUT)
class LabelEMF : public Label, private EMF
{
private:
    LabelEMF() : Label(), EMF() {}
    LabelEMF( const LabelEMF& src ) : Label( src ) {}

public:
    static Label * create() { return new LabelEMF(); }
    virtual LabelEMF * clone() { return new LabelEMF( *this ); }

    virtual double width() const;

    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelEMF& draw( std::ostream& ) const;

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
    virtual Label& appendS( const std::string& s );
    virtual Label& appendUI( const unsigned i );
    virtual Point initialPoint() const;
};
#endif

class LabelFig : public Label, private Fig
{
private:
    LabelFig() : Label(), Fig() {}
    LabelFig( const LabelFig& src ) : Label( src ), Fig() {}

public:
    static Label * create() { return new LabelFig(); }
    virtual LabelFig * clone() { return new LabelFig( *this ); }

    virtual Label& infty();
    virtual Label& times();
    virtual std::ostream& comment( std::ostream& output, const std::string& ) const;
    virtual const LabelFig& draw( std::ostream& ) const;

protected:
    virtual Label& appendPC( const char * s );
    virtual Point initialPoint() const;
};

#if HAVE_GD_H && HAVE_LIBGD
class LabelGD : public Label, private GD
{
private:
    LabelGD() : Label(), GD() {}
    LabelGD( const LabelGD& src ) : Label( src ), GD() {}

public:
    static Label * create() { return new LabelGD(); }
    virtual LabelGD * clone() { return new LabelGD( *this ); }

    virtual double width() const;

    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual Label& appendPC( const char * s );

    virtual const LabelGD& draw( std::ostream& output ) const;

protected:
    virtual Point initialPoint() const;
};
#endif

class LabelNull : public Label
{
private:
    LabelNull() : Label() {}
    LabelNull( const LabelNull& src ) : Label( src ) {}

public:
    static Label * create() { return new LabelNull(); }
    virtual LabelNull * clone() { return new LabelNull( *this ); }

    virtual const LabelNull& draw( std::ostream& output ) const { return *this; }

protected:
    virtual Point initialPoint() const;
};

class LabelPostScript : public Label, private PostScript
{
private:
    LabelPostScript() : Label(), PostScript() {}
    LabelPostScript( const LabelPostScript& src ) : Label( src ), PostScript() {}

public:
    static Label * create() { return new LabelPostScript(); }
    virtual LabelPostScript * clone() { return new LabelPostScript( *this ); }

    virtual Label& infty();
    virtual Label& times();
    virtual const LabelPostScript& draw( std::ostream& ) const;

protected:
    virtual Point initialPoint() const;
};

#if defined(SVG_OUTPUT)
class LabelSVG : public Label, private SVG
{
private:
    LabelSVG() : Label(), SVG() {}
    LabelSVG( const LabelSVG& src ) : Label( src ), SVG() {}

public:
    static Label * create() { return new LabelSVG(); }
    virtual LabelSVG * clone() { return new LabelSVG( *this ); }

    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelSVG& draw( std::ostream& ) const;

protected:
    virtual Label& appendPC( const char * s );
    virtual Point initialPoint() const;
};
#endif

#if SXD_OUTPUT
class LabelSXD : public Label, private SXD
{
private:
    LabelSXD() : Label(), SXD() {}
    LabelSXD( const LabelSXD& src ) : Label( src ), SXD() {}

public:
    static Label * create() { return new LabelSXD(); }
    virtual LabelSXD * clone() { return new LabelSXD( *this ); }

    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelSXD& draw( std::ostream& ) const;

protected:
    virtual Label& appendPC( const char * s );
    virtual Point initialPoint() const;
};
#endif

#if X11_OUTPUT
class LabelX11 : public Label, private X11
{
private:
    LabelX11() : Label(), X11() {}
    LabelX11( const LabelX11& src ) : Label( src ), X11() {}

public:
    static LabelX * create() { return new LabelX11(); }
    virtual LabelX11 * clone() { return new LabelX11( *this ); }

    virtual const LabelX11& draw( std::ostream& output ) const { return output; }
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
private:
    LabelTeX() : LabelTeXCommon(), TeX() {}
    LabelTeX( const LabelTeX& src ) : LabelTeXCommon( src ), TeX() {}

public:
    static Label * create() { return new LabelTeX(); }
    virtual LabelTeX * clone() { return new LabelTeX( *this ); }

    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelTeX& draw( std::ostream& ) const;

protected:
    virtual Point initialPoint() const;
};

class LabelPsTeX : public LabelTeXCommon, public Fig
{
private:
    LabelPsTeX( const LabelTeX& src ) : LabelTeXCommon( src ), Fig() {}
    LabelPsTeX() : LabelTeXCommon(), Fig() {}

public:
    static Label * create() { return new LabelPsTeX(); }
    virtual LabelPsTeX * clone() { return new LabelPsTeX( *this ); }

    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& rho();
    virtual Label& sigma();
    virtual Label& times();
    virtual const LabelPsTeX& draw( std::ostream& output ) const;

protected:
    virtual Point initialPoint() const;
};

template <class Type1> class DrawText {
public:
    typedef double (Type1::*textFPtr)( std::ostream& output, const Point& c, const std::string& s, Graphic::Font font, int fontsize,
				       Justification justification, Graphic::Colour colour, unsigned flags ) const;
    DrawText( std::ostream& output, const Type1& self, const textFPtr text, Point point, const Justification j, unsigned flags=0 ) : _output(output), _self(self), _text(text), _point(point), _justification(j), _flags(flags) {}
    void operator()( const Label::Line& line ) {
	const std::string& str = line.getStr();
	if ( str.size() ) {
	    _point.moveBy( 0, (_self.*_text)( _output, _point, str, line.getFont(), Flags::print[FONT_SIZE].opts.value.i, _justification, line.getColour(), _flags ) );
	}
    }
private:
    std::ostream& _output;
    const Type1& _self;
    const textFPtr _text;
    Point _point;
    const Justification _justification;
    const unsigned int _flags;
};

#if HAVE_GD_H && HAVE_LIBGD
template<> class DrawText<LabelGD> {
public:
    typedef double (LabelGD::*textFPtr)( const Point& c, const std::string& s, Graphic::Font font, int fontsize,
					 Justification justification, Graphic::Colour colour  ) const;
    DrawText<LabelGD>( std::ostream&, const LabelGD& self, const textFPtr text, Point point, const Justification j, unsigned flags=0 ) : _self(self), _text(text), _point(point), _justification(j) {}
    void operator()( const Label::Line& line ) {
	const std::string& str = line.getStr();
	if ( str.size() ) {
	    _point.moveBy( 0, (_self.*_text)( _point, str, line.getFont(), Flags::print[FONT_SIZE].opts.value.i, _justification, line.getColour() ) );
	}
    }
private:
    const LabelGD& _self;
    const textFPtr _text;
    Point _point;
    const Justification _justification;
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

    friend Label& operator<<(Label & label, const LabelCallManip& m )
	{ return m.f(label,m.myCall); }
};

class LabelEntryManip {
public:
    LabelEntryManip( Label& (*ff)(Label&, const Entry & ), const Entry & anEntry  )
	: f(ff), myEntry(anEntry) {}
private:
    Label& (*f)( Label&, const Entry & );
    const Entry & myEntry;

    friend Label& operator<<(Label & label, const LabelEntryManip& m )
	{ return m.f(label,m.myEntry); }
};

class LabelDoubleManip {
public:
    LabelDoubleManip( Label& (*ff)(Label&, double value ), double value  )
	: f(ff), _value(value) {}
private:
    Label& (*f)( Label&, double );
    const double _value;

    friend Label& operator<<(Label & label, const LabelDoubleManip& m )
	{ return m.f(label,m._value); }
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
LabelDoubleManip opt_pct( double );

LabelEntryManip execution_time_of( const Entry& entry );
LabelEntryManip queueing_time_of( const Entry& entry );
LabelEntryManip variance_of( const Entry& entry );
#endif
