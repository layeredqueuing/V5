/* -*- c++ -*-
 * $HeadURL$
 * 
 * Various statistics about solution.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <config.h>
#include <sstream>
#include <limits.h>
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#include <cmath>
#include "report.h"
#include "lqns.h"
#include "model.h"


static TimeManip print_time( clock_t );
static ostream& print_time_func( ostream& output, const clock_t time );
ostream& operator<<( ostream& output, const MVACount& self ) { return self.print( output ); }
ostream& operator<<( ostream& output, const SolverReport& self ) { return self.print( output ); }

int
operator==( const MVACount& a, const MVACount& b )
{
    return a.step == b.step
	&& a.step == b.step_sqr
	&& a.wait == b.wait
	&& a.wait == b.wait_sqr
	&& a._n == b._n;
}


/*
 * Return mean
 */

static double
mean( const unsigned n, const double sum )
{
    if ( n > 0 ) {
	return sum / n;
    } else {
	return 0.0;
    }
}



/*
 * Return the population standard deviation.
 */

static double
stddev( const double n, const double sum, const double sum_sqr )
{
    if ( n >= 2 ) {
	const double temp = sum_sqr - square(sum) / n;
	if ( temp > 0.0 ) {
	    return sqrt( temp / n );
	}
    }
    return 0.0;
}

/*
 * Reset all counters.
 */

MVACount&
MVACount::initialize()
{
    _n = 0; _k = 0; _s = 0; step = 0; step_sqr = 0; wait = 0; wait_sqr = 0; faults = 0; 
    sum_clock = 0;
#if defined(HAVE_SYS_TIMES_H)
    sum_times.tms_utime = 0;
    sum_times.tms_stime = 0;
#endif
    return *this;
}

/*
 * Print a header for the statisical stuff.
 */

ostream&
MVACount::printHeader( ostream& output, int faulty )
{
    output << "  n   k srv     step()       mean     stddev     wait()       mean     stddev    ";
#if defined(HAVE_SYS_TIMES_H)
    output << "    User      System     ";
#endif
    output << "Elapsed   ";
    if ( faulty ) {
	output << "  MVA Flr";
    }
    return output;
}


/*
 * Start the clock
 */

MVACount&
MVACount::start( const unsigned a_k, const unsigned a_s )
{
    _k = a_k;
    _s = a_s;

#if defined(HAVE_SYS_TIMES_H)
    start_clock = times( &start_times );
#else
    start_clock = time( 0 );
#endif
    assert( start_clock != static_cast<clock_t>(-1) );
    return *this;
}



/*
 * Print out a record.
 */

ostream&
MVACount::print( ostream& output ) const
{
    const unsigned precision = output.precision(5);
    FMT_FLAGS flags = output.setf( ios::right, ios::adjustfield );
    output << setw(4)  << _n << " " 
	   << setw(3)  << _k << " "
	   << setw(3)  << _s << " "
	   << setw(10) << step << " " 
	   << setw(10) << mean( _n, step ) << " "
	   << setw(10) << stddev( _n, step, step_sqr ) << " "
	   << setw(10) << wait << " " 
	   << setw(10) << mean( _n, wait ) << " "
	   << setw(10) << stddev( _n, wait, wait_sqr ) << " ";

#if	defined(HAVE_SYS_TIMES_H)
    output << print_time( sum_times.tms_utime ) << " " 
	   << print_time( sum_times.tms_stime ) << " ";
#endif
    output << print_time( sum_clock ) << " ";

    if ( faults ) {
	output << " ***" << setw(3)  << faults;
    }

    output.precision(precision);
    output.flags(flags);
    return output;
}


/*
 * Accumualate a record for the total.
 */

MVACount&
MVACount::operator+=( const MVACount& arg )
{
    _n        += arg._n;
    step      += arg.step;				
    step_sqr  += arg.step_sqr;			
    wait      += arg.wait;				
    wait_sqr  += arg.wait_sqr;			
    faults    += arg.faults;

    sum_clock += arg.sum_clock;
#if defined(HAVE_SYS_TIMES_H)
    sum_times.tms_utime += arg.sum_times.tms_utime;
    sum_times.tms_stime += arg.sum_times.tms_stime;
#endif
    return *this;
}


/*
 * Accumualate a record for the total.
 */

MVACount&
MVACount::operator=( const MVACount& arg )
{
    _n        = arg._n;
    _k	      = arg._k;
    _s	      = arg._s;
    step      = arg.step;				
    step_sqr  = arg.step_sqr;			
    wait      = arg.wait;				
    wait_sqr  = arg.wait_sqr;			
    faults    = arg.faults;

    sum_clock = arg.sum_clock;
#if defined(HAVE_SYS_TIMES_H)
    sum_times = arg.sum_times;
#endif

    return *this;
}


/*
 * Accumualate a record.
 */

MVACount&
MVACount::accumulate( const unsigned long iterations, const unsigned long inflation, const unsigned long nfaults )
{
    _n       += 1;
    step     += iterations;
    step_sqr += square(iterations);
    wait     += iterations * inflation;
    wait_sqr += square(iterations * inflation);
    faults   += nfaults;

    if ( start_clock != static_cast<clock_t>(-1) ) {
#if	defined(HAVE_SYS_TIMES_H)
	struct tms stop_times;
	clock_t stop_clock = times( &stop_times );
		
	sum_times.tms_utime += stop_times.tms_utime - start_times.tms_utime;
	sum_times.tms_stime += stop_times.tms_stime - start_times.tms_stime;
#else
	clock_t stop_clock = time( NULL );
#endif
	assert( stop_clock != static_cast<clock_t>(-1) );

	sum_clock += stop_clock - start_clock;
    }
    return *this;
}

SolverReport::SolverReport( LQIO::DOM::Document * document )
    : _document(document), _valid(false), _iterations(0), _convergenceValue(0.0), 
#if defined(HAVE_SYS_TIMES_H)
      _start_times(), _delta_times(),
#endif
      _start_clock(0), _delta_clock(0),
      MVAStats(), total()
{
    start();
}


/*
 * Set the start time for this report.
 */

void
SolverReport::start()
{
#if defined(HAVE_SYS_TIMES_H)
    _start_clock = times( &_start_times );
#else
    _start_clock = time( 0 );
#endif
    assert( _start_clock != static_cast<clock_t>(-1) );
}



/*
 * Set the stop time for this report and save statistics.
 */

SolverReport&
SolverReport::finish( const double convergence, const Model& aModel )
{
    _convergenceValue = convergence;
    _valid            = aModel._converged;
    _iterations       = aModel._iterations;
    MVAStats          = aModel.MVAStats;
	
    if ( _start_clock != static_cast<clock_t>(-1) ) {
#if	defined(HAVE_SYS_TIMES_H)
	struct tms stop_times;
	clock_t stop_clock = times( &stop_times );
		
	_delta_times.tms_utime = stop_times.tms_utime - _start_times.tms_utime;
	_delta_times.tms_stime = stop_times.tms_stime - _start_times.tms_stime;
#else
	clock_t stop_clock = time( NULL );
#endif
	assert( stop_clock != static_cast<clock_t>(-1) );

	_delta_clock = stop_clock - _start_clock;
    }

    /* Total statistics */
	
    for ( unsigned i = 1; i <= MVAStats.size(); ++i ) {
	total += MVAStats[i];
    }
    if ( total.faults > 0 ) _valid = 0;
	
    return *this;
}


/* ------------------------ Report Generation  ------------------------ */

/*
 * Print the solver report on output.
 */

ostream&
SolverReport::print( ostream& output ) const
{
    output << "Convergence test value: " << _convergenceValue << endl;
    output << "Number of iterations:   " << _iterations << endl << endl;

    output << "MVA solver information: " << endl;
    output << "Submdl";

    MVACount::printHeader( output, total.faults ) << endl;

    for ( unsigned i = 1; i <= MVAStats.size(); ++i ) {
	output << setw(3) << i << "  " << MVAStats[i] << endl;
    }
    output << "Total" << total << endl;

    output << endl;
#if HAVE_SYS_UTSNAME_H
    struct utsname uu;		/* Get system triva. */
    uname( &uu );

    output << "    " << uu.nodename << " " << uu.sysname << " " << uu.release << endl;
#endif
#if defined(HAVE_SYS_TIMES_H)
    output << "    User:    " << print_time( _delta_times.tms_utime ) << endl;
    output << "    System:  " << print_time( _delta_times.tms_stime ) << endl;
#endif
    output << "    Elapsed: " << print_time( _delta_clock ) << endl;

#if HAVE_SYS_RESOURCE_H && HAVE_GETRUSAGE
    struct rusage r_usage;
    if ( getrusage( RUSAGE_SELF, &r_usage ) == 0 && r_usage.ru_maxrss > 0 ) {
	output << "    Max RSS (kB): " << r_usage.ru_maxrss << endl;
	output << "         shared:  " << r_usage.ru_ixrss << endl;
	output << "         data:    " << r_usage.ru_idrss << endl;
	output << "         stack:   " << r_usage.ru_isrss << endl;
	output << "    MAJFLT:       " << r_usage.ru_majflt << endl;
	output << "    MINFLT:       " << r_usage.ru_minflt << endl;
    }
#endif
    return output;
}



void
SolverReport::insertDOMResults() const
{
    if ( !_document ) return;

    _document->setResultConvergenceValue(_convergenceValue)
	.setResultValid(_valid)
	.setResultIterations(_iterations)
	.setMVAStatistics(MVAStats.size(),total._n,total.step,total.step_sqr,total.wait,total.wait_sqr,total.faults);

    string buf;

#if defined(HAVE_SYS_UTSNAME_H)
    struct utsname uu;		/* Get system triva. */
    uname( &uu );

    buf = uu.nodename;
    buf += " ";
    buf += uu.sysname;
    buf += " ";
    buf += uu.release;
    _document->setResultPlatformInformation(buf);
#endif
    buf = io_vars.lq_toolname;
    buf += " ";
    buf += VERSION;
    _document->setResultSolverInformation(buf);

#if defined(HAVE_SYS_TIMES_H)
    _document->setResultUserTime(_delta_times.tms_utime);
    _document->setResultSysTime(_delta_times.tms_stime);
#endif
    _document->setResultElapsedTime(_delta_clock);
}


static TimeManip
print_time( clock_t aTime )
{
    return TimeManip( &print_time_func, aTime );
}


/*
 * Print out a time value with a nice format.
 */

static ostream&
print_time_func( ostream& output, const clock_t time )
{
#if defined(HAVE_SYS_TIME_H)
#if defined(CLK_TCK)
    const double dtime = static_cast<double>(time) / static_cast<double>(CLK_TCK);
#else
    const double dtime = static_cast<double>(time) / static_cast<double>(sysconf(_SC_CLK_TCK));
#endif
    const double csecs = fmod( dtime * 100.0, 100.0 );
#else
    const double dtime = time;
    const double csecs = 0.0;
#endif

    const double secs  = fmod( floor( dtime ), 60.0 );
    const double mins  = fmod( floor( dtime / 60.0 ), 60.0 );
    const double hrs   = floor( dtime / 3600.0 );

    FMT_FLAGS flags = output.setf( ios::dec|ios::fixed, ios::basefield|ios::fixed );
    int precision = output.precision(0);
    output.setf( ios::right, ios::adjustfield );
	
    output << setw(2) << hrs;
    char fill = output.fill('0');
    output << ':' << setw(2) << mins;
    output << ':' << setw(2) << secs;
    output << '.' << setw(2) << csecs;

    output.flags(flags);
    output.precision(precision);
    output.fill(fill);
    return output;
}
