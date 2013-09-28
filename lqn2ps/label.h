/* -*- c++ -*- node.h	-- Greg Franks
 *
 * $Id$
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

ostream& operator<<( ostream&, const Label& );

class Label : public Graphic
{
private:
    Label( const Label& );
    Label& operator=( const Label& );

public:
    Label();
    explicit Label( const Point& aPoint );
    virtual ~Label();

    static Label * newLabel();

    Label& operator <<( const LabelStringManip& m) { return appendLSM( m ); }
    Label& operator <<( const SRVNCallManip& m ) { return appendSCM( m ); }
    Label& operator <<( const SRVNEntryManip& m) { return appendSEM( m ); }
    Label& operator <<( const TaskCallManip& m ) { return appendSTM( m ); }
    Label& operator <<( const char * s ) { return appendPC( s ); }
    Label& operator <<( const char c ) { return appendC( c ); }
    Label& operator <<( const double d ) { return appendD( d ); }
    Label& operator <<( const int i ) { return appendI( i ); }
    Label& operator <<( const string& s ) { return appendS( s ); }
    Label& operator <<( const unsigned i ) { return appendUI( i ); }
    Label& operator <<( const LQIO::DOM::ExternalVariable& v ) { return appendV( v ); }

    virtual Label& moveTo( const double x, const double y ) { origin.moveTo( x, y ); return *this; }
    virtual Label& moveTo( const Point& aPoint ) { origin.moveTo( aPoint.x(), aPoint.y() ); return *this; }
    virtual Label& moveBy( const double x, const double y ) { origin.moveBy( x, y ); return *this; }
    virtual Label& moveBy( const Point& aPoint ) { origin.moveBy( aPoint.x(), aPoint.y() ); return *this; }
    virtual Label& scaleBy( const double, const double );
    virtual Label& translateY( const double );

    virtual Label& newLine();
    Label& initialize( const string& );

    Label& font( const font_type );
    Label& colour( const colour_type );
    Label& backgroundColour( const colour_type aColour ) { myBackgroundColour = aColour; return *this; }
    colour_type backgroundColour() const { return myBackgroundColour; }
    Label& justification( const justification_type justify ) { myJustification = justify; return *this; }
    justification_type justification() const { return myJustification; }

    virtual double width() const;
    virtual double height() const;

    virtual Label& beginMath();
    virtual Label& endMath();
    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& times();
    virtual Label& sigma();
    virtual Label& mu();
    Label& percent();

    virtual ostream& print( ostream& ) const = 0;
    virtual ostream& comment( ostream& output, const string& ) const { return output; }

protected:
    virtual Label& appendLSM( const LabelStringManip& m);
    virtual Label& appendSCM( const SRVNCallManip& m );
    virtual Label& appendSEM( const SRVNEntryManip& m);
    virtual Label& appendSTM( const TaskCallManip& m );
    virtual Label& appendPC( const char * s ) { *myStrings.back() << s; return *this; }
    virtual Label& appendC( const char c ) { *myStrings.back() << c; return *this; }
    virtual Label& appendD( const double );
    virtual Label& appendI( const int i ) { *myStrings.back() << i; return *this; }
    virtual Label& appendS( const string& s ) { *myStrings.back() << s; return *this; }
    virtual Label& appendUI( const unsigned i ) { *myStrings.back() << i; return *this; }
    virtual Label& appendV( const LQIO::DOM::ExternalVariable& v );

    int size() const { return myStrings.size(); }
    const Label& boundingBox( Point& boxOrigin, Point& boxExtent, const double scaling ) const;
    virtual Point initialPoint() const = 0;

private:
    Label& clear();
    void grow( unsigned size );

protected:
    Point origin;
    vector<Graphic::font_type> myFont;
    vector<int> myColour;
    vector<ostringstream *> myStrings;
    justification_type myJustification;
    colour_type myBackgroundColour;
    bool mathMode;
};

#if defined(EMF_OUTPUT)
class LabelEMF : public Label, private EMF
{
public:
    virtual double width() const;

    virtual Label& epsilon();
    virtual Label& infty();
    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& sigma();
    virtual Label& times();
    virtual ostream& print( ostream& ) const;

protected:
    virtual Label& appendLSM( const LabelStringManip& m);
    virtual Label& appendSCM( const SRVNCallManip& m );
    virtual Label& appendSEM( const SRVNEntryManip& m);
    virtual Label& appendSTM( const TaskCallManip& m );
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
    virtual ostream& print( ostream& ) const;

protected:
    virtual Label& appendPC( const char * s );
    virtual Point initialPoint() const;
};

#if HAVE_GD_H && HAVE_LIBGD
class LabelGD : public Label, private GD
{
public:
    virtual double width() const;

    virtual Label& lambda();
    virtual Label& mu();
    virtual Label& times();
    virtual Label& sigma();
    virtual Label& infty();
    virtual Label& appendPC( const char * s );

    virtual ostream& print( ostream& output ) const;

protected:
    virtual Point initialPoint() const;
};
#endif

class LabelNull : public Label
{
public:
    virtual ostream& print( ostream& output ) const { return output; }

protected:
    virtual Point initialPoint() const;
};

class LabelPostScript : public Label, private PostScript
{
public:
    virtual Label& infty();
    virtual Label& times();
    virtual ostream& print( ostream& ) const;

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
    virtual Label& sigma();
    virtual Label& times();
    virtual ostream& print( ostream& ) const;

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
    virtual Label& sigma();
    virtual Label& times();
    virtual ostream& print( ostream& ) const;

protected:
    virtual Label& appendPC( const char * s );
    virtual Point initialPoint() const;
};
#endif

#if defined(X11_OUTPUT)
class LabelX11 : public Label, private X11
{
public:
    virtual ostream& print( ostream& output ) const { return output; }
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
    virtual Label& sigma();
    virtual Label& times();
    virtual ostream& print( ostream& ) const;

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
    virtual Label& sigma();
    virtual Label& times();
    virtual ostream& print( ostream& output ) const;

protected:
    virtual Point initialPoint() const;
};

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
LabelManip _sigma();
LabelManip _times();
LabelManip _percent();
#endif
