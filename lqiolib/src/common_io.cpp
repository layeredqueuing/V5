/* -*- c++ -*-
 * $Id: common_io.cpp 15906 2022-09-25 12:52:43Z greg $
 *
 * Read in XML input files.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ------------------------------------------------------------------------
 * September 2013
 * ------------------------------------------------------------------------
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <cstring>
#include <cmath>
#include <iomanip>
#include <algorithm>
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_PWD_H
#include <pwd.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "dom_document.h"
#include "dom_phase.h"
#include "common_io.h"

namespace LQIO {
    namespace DOM {

	std::map<const std::string, const scheduling_type> Common_IO::scheduling_table =
	{
	    { SCHEDULE::BURST,		    SCHEDULE_BURST },
	    { SCHEDULE::CFS,		    SCHEDULE_CFS },
	    { SCHEDULE::CUSTOMER,	    SCHEDULE_CUSTOMER },
	    { SCHEDULE::DELAY,		    SCHEDULE_DELAY },
	    { SCHEDULE::FIFO,		    SCHEDULE_FIFO },
	    { SCHEDULE::HOL,		    SCHEDULE_HOL },
	    { SCHEDULE::LIFO,		    SCHEDULE_LIFO },
	    { SCHEDULE::POLL,		    SCHEDULE_POLL },
	    { SCHEDULE::PPR,		    SCHEDULE_PPR },
	    { SCHEDULE::PS,		    SCHEDULE_PS },
	    { SCHEDULE::RAND,		    SCHEDULE_RAND },
	    { SCHEDULE::RWLOCK,		    SCHEDULE_RWLOCK },
	    { SCHEDULE::SEMAPHORE,	    SCHEDULE_SEMAPHORE },
	    { SCHEDULE::UNIFORM,	    SCHEDULE_UNIFORM }
	};

	Common_IO::Common_IO()
	    : _conf_95( ConfidenceIntervals( LQIO::ConfidenceIntervals::CONF_95 ) ),
	      _conf_99( ConfidenceIntervals( LQIO::ConfidenceIntervals::CONF_99 ) )
	{
	}

	double
	Common_IO::invert( const double arg ) const
	{
	    return _conf_95.invert( arg );
	}

	/* Hoops to find the phase number */
	unsigned int
	Common_IO::get_phase( const LQIO::DOM::Phase * phase )
	{
	    const LQIO::DOM::Entry * entry = phase->getSourceEntry();
	    const std::map<unsigned, Phase*>& phases = entry->getPhaseList();
	    std::map<unsigned,Phase *>::const_iterator iter = find_if( phases.begin(), phases.end(), LQIO::DOM::Phase::eqPhase( phase ) );
	    if ( iter == phases.end() ) abort();
	    return iter->first;
	}
	
	/* 
	 * This function is used to ignore default values in the
	 * input.  
	 */
	
	bool
	Common_IO::is_default_value( const LQIO::DOM::ExternalVariable * var, double default_value ) 
	{
	    double value = 0.0;
	    return var == nullptr || (var->wasSet() && var->getValue(value) && value == default_value);
	}

	CPUTime
	CPUTime::operator-( const CPUTime& subtrahend ) const
	{
	    CPUTime difference;
	    difference._real   = _real   - subtrahend._real;
	    difference._user   = _user   - subtrahend._user;
	    difference._system = _system - subtrahend._system;
	    return difference;
	}

#if HAVE_SYS_RESOURCE_H && HAVE_GETRUSAGE
static inline double tv_to_double( struct timeval& tv ) { return (static_cast<double>(tv.tv_sec)*1.e6 + static_cast<double>(tv.tv_usec))/1.e6; }
#endif

	bool
	CPUTime::init()
	{
#if HAVE_GETRUSAGE && HAVE_GETTIMEOFDAY
	    struct rusage r_usage;
	    if ( getrusage( RUSAGE_SELF, &r_usage ) != 0 ) return false;
	    _user   = tv_to_double( r_usage.ru_utime );
	    _system = tv_to_double( r_usage.ru_stime );
	    if ( getrusage( RUSAGE_CHILDREN, &r_usage ) != 0 ) return false;
	    _user   += tv_to_double( r_usage.ru_utime );
	    _system += tv_to_double( r_usage.ru_stime );
	    struct timeval tv;
	    if ( gettimeofday( &tv, 0 ) != 0 ) return false;
	    _real   = tv_to_double( tv );
#elif HAVE_SYS_TIMES_H
#if defined(CLK_TCK)
	    const double tick = static_cast<double>(CLK_TCK);
#else
	    const double tick = static_cast<double>(sysconf(_SC_CLK_TCK));
#endif
	    struct tms start_tms;
	    _real   = times( &start_tms ) / tick;
	    _user   = (start_tms.tms_utime + start_tms.tms_cutime) / tick;
	    _system = (start_tms.tms_stime + start_tms.tms_cstime) / tick;
#else
	    _real   = time( 0 );
#if HAVE_TIME_H
	    clock_t t = clock();
	    _user   = static_cast<double>(t) / static_cast<double>(CLOCKS_PER_SEC);
#else
	    _user   = 0.;
#endif
	    _system = 0.;
#endif
	    return true;
	}


	void
	CPUTime::insertDOMResults( Document& document ) const
	{
	    document.setResultUserTime( getUserTime() );
	    document.setResultSysTime( getSystemTime() );
	    document.setResultElapsedTime( getRealTime() );
#if HAVE_SYS_RESOURCE_H && HAVE_GETRUSAGE
	    struct rusage r_usage;
	    if ( getrusage( RUSAGE_SELF, &r_usage ) == 0 && r_usage.ru_maxrss > 0 ) {
		document.setResultMaxRSS( r_usage.ru_maxrss );
	    }
#endif
	}


	/* static */ std::ostream&
	CPUTime::print( std::ostream& output, const double time )
	{
	    const double secs  = fmod( floor( time ), 60.0 );
	    const double mins  = fmod( floor( time / 60.0 ), 60.0 );
	    const double hrs   = floor( time / 3600.0 );
	    const double msecs = floor( (time - floor( time )) * 1000.0 );

	    const std::ios_base::fmtflags flags = output.setf( std::ios::dec|std::ios::fixed, std::ios::basefield|std::ios::fixed );
	    const int precision = output.precision(0);
	    output.setf( std::ios::right, std::ios::adjustfield );

	    output << std::setw(2) << hrs;
	    char fill = output.fill('0');
	    output << ':' << std::setw(2) << mins;
	    output << ':' << std::setw(2) << secs;
	    output << '.' << std::setw(3) << msecs;

	    output.flags(flags);
	    output.precision(precision);
	    output.fill(fill);
	    return output;
	}

	ForPhase::ForPhase()
	    : _maxPhase(DOM::Phase::MAX_PHASE), _type(DOM::Call::Type::NULL_CALL)
	{
	    for ( unsigned p = 0; p < DOM::Phase::MAX_PHASE; ++p ) {
		ia[p] = 0;
	    }
	}

	void
	CollectCalls::operator()( const std::pair<unsigned, Phase*>& p )
	{
	    const unsigned n = p.first;
	    const std::vector<Call *>& calls = p.second->getCalls();
	    for ( std::vector<Call*>::const_iterator c = calls.begin(); c != calls.end(); ++c ) {
		const Call* call = *c;
		if ( !_test || (call->*_test)() ) {
		    ForPhase& y = _calls[call->getDestinationEntry()];
		    y.setType( call->getCallType() );
		    y[n] = call;
		}
	    }
	}

	std::ostream&
	GetLogin::print( std::ostream& output ) const
	{
#if HAVE_GETEUID && HAVE_GETPWUID
	    struct passwd * passwd = getpwuid(geteuid());
	    output << passwd->pw_name;
#elif defined(__WINNT__)
	    output << getenv("USERNAME");
#endif
	    return output;
	}
    }

    static const std::string WHITESPACE = " \n\r\t\f\v";
 
    std::string ltrim(const std::string& s)
    {
	size_t start = s.find_first_not_of(WHITESPACE);
	return (start == std::string::npos) ? "" : s.substr(start);
    }
 
    std::string rtrim(const std::string& s)
    {
	size_t end = s.find_last_not_of(WHITESPACE);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
    }    
}

/*
 * Copy str to mallocated storage.
 */

char *
lqio_duplicate_string( char * str, int len )
{
    char * p = (char *)malloc( (unsigned)len+1 );

    if ( p ) {
	(void) strncpy( p, (char *)str, len );
	/* strip trailing whitespace */
	for ( ; len > 0 && isspace( p[len-1] ); len-- );
	p[len] = '\0';
    }
    return p;
}


/*
 * Copy str to mallocated storage.
 * Strip out '\' sequences.
 */

char *
lqio_duplicate_comment( char * str, int len )
{
    char * p = (char *)malloc( (unsigned)len+1 );
    char * q = p;

    if ( p ) {
	while ( *str && len ) {
	    if ( *str != '\\' ) {
		*p++ = *str;
	    }
	    ++str;
	    --len;
	}
	*p = '\0';
    }
    return q;
}
