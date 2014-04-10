/* -*- C++ -*-
 *  $Id: expat_document.h 11503 2013-08-30 23:53:06Z greg $
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

	class IntegerManip {
	public:
	IntegerManip( std::ostream& (*f)(std::ostream&, const int ), const int i ) : _f(f), _i(i) {}
	private:
	    std::ostream& (*_f)( std::ostream&, const int );
	    const int _i;

	    friend std::ostream& operator<<(std::ostream & os, const IntegerManip& m ) { return m._f(os,m._i); }
	};

	class SimpleManip {
	public:
	SimpleManip( std::ostream& (*f)(std::ostream& ) ) : _f(f) {}
	private:
	    std::ostream& (*_f)( std::ostream& );

	    friend std::ostream& operator<<(std::ostream & os, const SimpleManip& m ) { return m._f(os); }
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

	class StringTimeManip {
	public:
	StringTimeManip( std::ostream& (*f)(std::ostream&, const std::string& name, clock_t value ), const std::string& name, clock_t value ) : _f(f), _name(name), _value(value) {}

	private:
	    std::ostream& (*_f)( std::ostream&, const std::string&, clock_t );
	    const std::string& _name;
	    const clock_t _value;

	    friend std::ostream& operator<<(std::ostream & os, const StringTimeManip& m ) { return m._f(os,m._name,m._value); }
	};

	class Common_IO {

	public:
	    Common_IO();

	    class Compare {
	    public:
		Compare() {}
		bool operator()( const char * s1, const char * s2 ) const;
	    };

	protected:
	    double invert( const double ) const;
	    static void init_tables();
	    void invalid_argument( const std::string& attr, const std::string& arg ) const throw( std::invalid_argument );

	protected:
	    const ConfidenceIntervals _conf_95;
	    const ConfidenceIntervals _conf_99;

	    static std::map<const char *, scheduling_type,Common_IO::Compare> scheduling_table;
	};


	class ForPhase {
	public:
	    ForPhase();
	    const DOM::Call*& operator[](const unsigned ix) { assert( ix && ix <= DOM::Phase::MAX_PHASE ); return ia[ix-1]; }
	    const DOM::Call* operator[](const unsigned ix) const { assert( ix && ix <= DOM::Phase::MAX_PHASE ); return ia[ix-1]; }
		
	    ForPhase& setMaxPhase( const unsigned mp ) { _maxPhase = mp; return *this; }
	    const unsigned getMaxPhase() const { return _maxPhase; }
	    ForPhase& setType( const DOM::Call::CallType type ) { _type = type; return *this; }
	    const DOM::Call::CallType getType() const { return _type; }

	private:
	    const DOM::Call * ia[DOM::Phase::MAX_PHASE];
	    unsigned _maxPhase;
	    DOM::Call::CallType _type;
	};
    }
}

#endif
