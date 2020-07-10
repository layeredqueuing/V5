/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/report.cc $
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
 * $Id: report.cc 13676 2020-07-10 15:46:20Z greg $
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <config.h>
#include <iomanip>
#include <sstream>
#include <limits.h>
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include <cmath>
#include "lqns.h"
#include "report.h"
#include <lqio/dom_document.h>

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
    _total_time = 0;
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
    _start_time.init();
    return *this;
}



/*
 * Print out a record.
 */

ostream&
MVACount::print( ostream& output ) const
{
    const unsigned precision = output.precision(5);
    ios_base::fmtflags flags = output.setf( ios::right, ios::adjustfield );
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
    output << LQIO::DOM::CPUTime::print( _total_time.getUserTime() ) << " " 
	   << LQIO::DOM::CPUTime::print( _total_time.getSystemTime() ) << " ";
#endif
    output << LQIO::DOM::CPUTime::print( _total_time.getRealTime() ) << " ";

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

    _total_time  += arg._total_time;
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

    _total_time  = arg._total_time;

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

    LQIO::DOM::CPUTime stop_time;
    stop_time.init();
    _total_time += stop_time  - _start_time;
    return *this;
}

SolverReport::SolverReport( LQIO::DOM::Document * document, const Vector<MVACount>& stats )
    : _document(document), _valid(false), _iterations(0), _convergenceValue(0.0), 
      _start_time(), _delta_time(), MVAStats(stats), total()
{
    start();
}


/*
 * Set the start time for this report.
 */

void
SolverReport::start()
{
    _start_time.init();
}



/*
 * Set the stop time for this report and save statistics.
 */

SolverReport&
SolverReport::finish( bool valid, const double convergence, unsigned long iterations )
{
    _convergenceValue = convergence;
    _valid            = valid;
    _iterations       = iterations;
	
    LQIO::DOM::CPUTime stop_time;
    stop_time.init();
    _delta_time = stop_time - _start_time;

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
#if HAVE_SYS_RESOURCE_H && HAVE_GETRUSAGE
    struct rusage r_usage;
    if ( getrusage( RUSAGE_SELF, &r_usage ) == 0 ) {
	if ( r_usage.ru_maxrss > 0 ) {
	    output << "    Max RSS (kB): " << r_usage.ru_maxrss << endl;
	    output << "         shared:  " << r_usage.ru_ixrss << endl;
	    output << "         data:    " << r_usage.ru_idrss << endl;
	    output << "         stack:   " << r_usage.ru_isrss << endl;
	    output << "    MAJFLT:       " << r_usage.ru_majflt << endl;
	    output << "    MINFLT:       " << r_usage.ru_minflt << endl;
	}
    }
#endif
    output << "    Real:    " << LQIO::DOM::CPUTime::print( _delta_time.getRealTime() ) << endl;
#if defined(HAVE_SYS_TIMES_H)
    output << "    User:    " << LQIO::DOM::CPUTime::print( _delta_time.getUserTime() ) << endl;
    output << "    System:  " << LQIO::DOM::CPUTime::print( _delta_time.getSystemTime() ) << endl;
#endif

    return output;
}



const SolverReport& 
SolverReport::insertDOMResults() const
{
    if ( !_document ) return *this;

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

    _delta_time.insertDOMResults( *_document );
    return *this;

#if HAVE_SYS_RESOURCE_H && HAVE_GETRUSAGE
    struct rusage r_usage;
    if ( getrusage( RUSAGE_SELF, &r_usage ) == 0 && r_usage.ru_maxrss > 0 ) {
	_document->setResultMaxRSS( r_usage.ru_maxrss );
    }
#endif
    return *this;
}
