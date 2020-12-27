/* -*- C++ -*-
 *  $Id: common_io.h 14273 2020-12-27 14:47:06Z greg $
 *
 *  Greg Franks
 */

#ifndef __LQIO_COMMON_IO
#define __LQIO_COMMON_IO

#include <string>
#include <ostream>
#include <map>
#include <cassert>
#include <time.h>
#include "confidence_intervals.h"
#include "dom_phase.h"
#include "dom_call.h"
#include "input.h"

namespace LQIO {
    namespace DOM {
	class ExternalVariable;
	class Document;
	class Phase;

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
	    static SimpleManip svn_id() { return SimpleManip( &printSVNId ); }

	protected:
	    double invert( const double ) const;
	    static void init_tables();
	    static unsigned int get_phase( const LQIO::DOM::Phase * );

	protected:
	    const ConfidenceIntervals _conf_95;
	    const ConfidenceIntervals _conf_99;

	    static std::map<const char *, scheduling_type,Common_IO::Compare> scheduling_table;

	private:
	    static std::ostream& printSVNId( std::ostream& output ) { output << "$" << "Id" << "$"; return output; }
	    
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
	
	class ForPhase {
	public:
	    ForPhase();
	    const Call*& operator[](const unsigned ix) { assert( ix && ix <= Phase::MAX_PHASE ); return ia[ix-1]; }
	    const Call* operator[](const unsigned ix) const { assert( ix && ix <= Phase::MAX_PHASE ); return ia[ix-1]; }
		
	    ForPhase& setMaxPhase( const unsigned mp ) { _maxPhase = mp; return *this; }
	    const unsigned getMaxPhase() const { return _maxPhase; }
	    ForPhase& setType( const Call::CallType type ) { _type = type; return *this; }
	    const Call::CallType getType() const { return _type; }

	private:
	    const Call * ia[Phase::MAX_PHASE];
	    unsigned _maxPhase;
	    Call::CallType _type;
	};

	/*
	 * Collects all calls to a given destination by phase.
	 */

	struct CollectCalls {
	    CollectCalls( std::map<const Entry *, ForPhase>& calls, Call::boolCallFunc test=0 ) : _calls(calls), _test(test) {}
	    void operator()( const std::pair<unsigned, Phase*>& p );
	    
	private:
	    std::map<const Entry *, ForPhase>& _calls;
	    const Call::boolCallFunc _test;
	};

	class GetLogin {
	public:
	    GetLogin() {}
	    friend std::ostream& operator<<( std::ostream& output, const GetLogin& self ) { return self.print( output ); }
	private:
	    std::ostream& print( std::ostream& ) const;
	};
    }
}

#endif
