/* -*- C++ -*-
 *  $Id: common_io.h 17332 2024-10-03 15:25:44Z greg $
 *
 *  Greg Franks
 */

#ifndef __LQIO_COMMON_IO
#define __LQIO_COMMON_IO

#if defined(__cplusplus)
#include <string>
#include <ostream>
#include <map>
#include <cassert>
#include <time.h>
#include "confidence_intervals.h"
#include "input.h"

namespace LQIO {
    namespace DOM {
	class ExternalVariable;
	class Document;

	class SimpleManip {
	public:
	SimpleManip( std::ostream& (*f)(std::ostream& ) ) : _f(f) {}
	private:
	    std::ostream& (*_f)( std::ostream& );

	    friend std::ostream& operator<<(std::ostream & os, const SimpleManip& m ) { return m._f(os); }
	};

	class IntegerManip {
	public:
	IntegerManip( std::ostream& (*f)(std::ostream&, const int ), const int i ) : _f(f), _i(i) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const int );
	    const int _i;

	    friend std::ostream& operator<<(std::ostream & os, const IntegerManip& m ) { return m._f(os,m._i); }
	};

	class DoubleManip {
	public:
	    DoubleManip( std::ostream& (*f)(std::ostream&, const double ), const double d ) : _f(f), _d(d) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const double );
	    const double _d;
	    friend std::ostream& operator<<(std::ostream & os, const DoubleManip& m ) { return m._f(os,m._d); }
	};

	class StringManip {
	public:
	StringManip( std::ostream& (*f)(std::ostream&, const std::string& ), const std::string& s ) : _f(f), _s(s) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const std::string& );
	    const std::string& _s;

	    friend std::ostream& operator<<(std::ostream & os, const StringManip& m ) { return m._f(os,m._s); }
	};

	class StringStringManip {
	public:
	StringStringManip( std::ostream& (*f)(std::ostream&, const std::string& name, const std::string& value ), const std::string& name, const std::string& value ) : _f(f), _name(name), _value(value) {}

	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, const std::string& );
	    const std::string& _name;
	    const std::string& _value;

	    friend std::ostream& operator<<(std::ostream & os, const StringStringManip& m ) { return m._f(os,m._name,m._value); }
	};

	class StringBooleanManip {
	public:
	StringBooleanManip( std::ostream& (*f)(std::ostream&, const std::string& name, bool value ), const std::string& name, bool value ) : _f(f), _name(name), _value(value) {}

	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, bool );
	    const std::string& _name;
	    const bool _value;

	    friend std::ostream& operator<<(std::ostream & os, const StringBooleanManip& m ) { return m._f(os,m._name,m._value); }
	};

	class StringDoubleManip {
	public:
	StringDoubleManip( std::ostream& (*f)(std::ostream&, const std::string& name, double value ), const std::string& name, double value ) : _f(f), _name(name), _value(value) {}

	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, double );
	    const std::string& _name;
	    const double _value;

	    friend std::ostream& operator<<(std::ostream & os, const StringDoubleManip& m ) { return m._f(os,m._name,m._value); }
	};

	class StringLongManip {
	public:
	StringLongManip( std::ostream& (*f)(std::ostream&, const std::string& name, long value ), const std::string& name, long value ) : _f(f), _name(name), _value(value) {}

	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, long );
	    const std::string& _name;
	    const long _value;

	    friend std::ostream& operator<<(std::ostream & os, const StringLongManip& m ) { return m._f(os,m._name,m._value); }
	};

	class Common_IO {

	public:
	    Common_IO();
	    class Compare {
	    public:
		Compare() {}
		bool operator()( const char * s1, const char * s2 ) const;
	    };

	public:
	    static bool is_default_value( const LQIO::DOM::ExternalVariable *, double );
	    static const std::string& svn_id() { static const std::string __SVNId = std::string("$") + "Id" + "$"; return __SVNId; }

	protected:
	    double invert( const double ) const;

	protected:
	    const ConfidenceIntervals _conf_95;
	    const ConfidenceIntervals _conf_99;

	    static std::map<const std::string,const scheduling_type> scheduling_table;
	};


	class CPUTime {
	public:
	    CPUTime() : _real(0.), _user(0.), _system(0.) {}
	    CPUTime operator-( const CPUTime& ) const;
	    CPUTime& operator=( double t ) { _real = t; _user = t; _system = t; return *this; }
	    CPUTime& operator=( const CPUTime& t ) { _real = t._real; _user = t._user; _system = t._system; return *this; }
	    CPUTime& operator+=( const CPUTime& t ) { _real += t._real; _user += t._user; _system += t._system; return *this; }
	    CPUTime& operator-=( const CPUTime& t ) { _real -= t._real; _user -= t._user; _system -= t._system; return *this; }
    	    
	    bool init();

	    double getRealTime() const { return _real; }
	    double getUserTime() const { return _user; }
	    double getSystemTime() const { return _system; }

	    static DoubleManip print( const double t ) { return DoubleManip( &CPUTime::print, t ); }

	    void insertDOMResults( Document& ) const;
	    
	private:
	    static std::ostream& print( std::ostream& output, const double time );

	private:
	    double _real;
	    double _user;
	    double _system;
	};
	
	class GetLogin {
	public:
	    GetLogin() {}
	    friend std::ostream& operator<<( std::ostream& output, const GetLogin& self ) { return self.print( output ); }
	private:
	    std::ostream& print( std::ostream& ) const;
	};

    }

    std::string createDirectory();


    std::string ltrim(const std::string& s);
    std::string rtrim(const std::string& s);
}
#endif

/* flex scanner */
#if defined(__cplusplus)
extern "C" {
#endif
    extern char * lqio_duplicate_string( char * str, int len );
    extern char * lqio_duplicate_comment( char * str, int len );
#if defined(__cplusplus)
}
#endif
#endif
