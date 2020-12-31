/* -*- C++ -*-
 *  $Id: expat_document.h 13717 2020-08-03 00:04:28Z greg $
 *
 *  Created by Martin Mroz on 24/02/09.
 */

#ifndef __LQIO__XML_OUTPUT__
#define __LQIO__XML_OUTPUT__

#include <string>
#include <iostream>
namespace LQIO {
    namespace DOM {
	class ExternalVariable;
    }
}

namespace XML {
	
    class BooleanManip {
    public:
    BooleanManip( std::ostream& (*f)(std::ostream&, const std::string&, const bool ), const std::string& a, const bool b ) : _f(f), _a(a), _b(b) {}
    private:
	std::ostream& (*_f)( std::ostream&, const std::string&, const bool );
	const std::string& _a;
	const bool _b;
	friend std::ostream& operator<<(std::ostream & os, const BooleanManip& m ) { return m._f(os,m._a,m._b); }
    };

    class DoubleManip {
    public:
    DoubleManip( std::ostream& (*f)(std::ostream&, const std::string&, const double ), const std::string& a, const double v ) : _f(f), _a(a), _v(v) {}
    private:
	std::ostream& (*_f)( std::ostream&, const std::string&, const double );
	const std::string& _a;
	const double _v;
	friend std::ostream& operator<<(std::ostream & os, const DoubleManip& m ) { return m._f(os,m._a,m._v); }
    };

    class ExternalVariableManip {
    public:
	ExternalVariableManip( std::ostream& (*f)(std::ostream&, const std::string&, const LQIO::DOM::ExternalVariable& ), const std::string& a, const LQIO::DOM::ExternalVariable& v ) : _f(f), _a(a), _v(v) {}
    private:
	std::ostream& (*_f)( std::ostream&, const std::string&, const LQIO::DOM::ExternalVariable& );
	const std::string& _a;
	const LQIO::DOM::ExternalVariable& _v;

	friend std::ostream& operator<<(std::ostream & os, const ExternalVariableManip& m ) { return m._f(os,m._a,m._v); }
    };

    class InlineElementManip {
    public:
    InlineElementManip( std::ostream& (*f)(std::ostream&, const std::string&, const std::string&, const std::string&, double ), const std::string& e, const std::string& a, const std::string& v, double d )
	: _f(f), _e(e), _a(a), _v(v), _d(d) {}
    private:
	std::ostream& (*_f)( std::ostream&, const std::string&, const std::string&, const std::string&, double );
	const std::string& _e;
	const std::string& _a;
	const std::string& _v;
	const double _d;
	friend std::ostream& operator<<(std::ostream & os, const InlineElementManip& m ) { return m._f(os,m._e,m._a,m._v,m._d); }
    };

    class IntegerManip {
    public:
    IntegerManip( std::ostream& (*f)(std::ostream&, const int ), const int i ) : _f(f), _i(i) {}
    private:
	std::ostream& (*_f)( std::ostream&, const int );
	const int _i;
	friend std::ostream& operator<<(std::ostream & os, const IntegerManip& m ) { return m._f(os,m._i); }
    };

    class StringManip {
    public:
    StringManip( std::ostream& (*f)(std::ostream&, const std::string&, const std::string& ), const std::string& a, const std::string& v=0 ) : _f(f), _a(a), _v(v) {}
    private:
	std::ostream& (*_f)( std::ostream&, const std::string&, const std::string& );
	const std::string& _a;
	const std::string& _v;
	friend std::ostream& operator<<(std::ostream & os, const StringManip& m ) { return m._f(os,m._a,m._v); }
    };

    class UnsignedManip {
    public:
    UnsignedManip( std::ostream& (*f)(std::ostream&, const std::string&, const unsigned int ), const std::string& a, const unsigned int u ) : _f(f), _a(a), _u(u) {}
    private:
	std::ostream& (*_f)( std::ostream&, const std::string&, const unsigned int );
	const std::string& _a;
	const unsigned int _u;
	friend std::ostream& operator<<(std::ostream & os, const UnsignedManip& m ) { return m._f(os,m._a,m._u); }
    };


    int set_indent( int );
    IntegerManip indent( int );
    IntegerManip temp_indent( int );
	
    StringManip comment( const std::string& s );
    BooleanManip start_element( const std::string& e, bool b=true );
    BooleanManip end_element( const std::string& e, bool b=true );
    BooleanManip simple_element( const std::string& e );
    InlineElementManip inline_element( const std::string& e, const std::string& a, const std::string& v, double d );
    StringManip attribute( const std::string& a, const std::string& v );
    DoubleManip attribute( const std::string& a, double v );
    UnsignedManip attribute( const std::string& a, unsigned int v );
    BooleanManip  attribute( const std::string& a, bool v );
    ExternalVariableManip attribute( const std::string& a, const LQIO::DOM::ExternalVariable& v );
    DoubleManip time_attribute( const std::string& a, const double v );
    StringManip comment( const std::string& s );
    StringManip cdata( const std::string& s );
}
#endif
