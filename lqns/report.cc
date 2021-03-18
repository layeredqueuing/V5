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
 * $Id: report.cc 14563 2021-03-18 02:07:31Z greg $
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <config.h>
#include <iomanip>
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

std::ostream&
MVACount::printHeader( std::ostream& output, int faulty )
{
    output << "  n   k srv     step()       mean     stddev     wait()       mean     stddev    ";
#if defined(HAVE_SYS_TIME_H)
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

std::ostream&
MVACount::print( std::ostream& output ) const
{
    const unsigned precision = output.precision(5);
    std::ios_base::fmtflags flags = output.setf( std::ios::right, std::ios::adjustfield );
    output << std::setw(4)  << _n << " " 
	   << std::setw(3)  << _k << " "
	   << std::setw(3)  << _s << " "
	   << std::setw(10) << step << " " 
	   << std::setw(10) << mean( _n, step ) << " "
	   << std::setw(10) << stddev( _n, step, step_sqr ) << " "
	   << std::setw(10) << wait << " " 
	   << std::setw(10) << mean( _n, wait ) << " "
	   << std::setw(10) << stddev( _n, wait, wait_sqr ) << " ";

#if	defined(HAVE_SYS_TIME_H)
    output << LQIO::DOM::CPUTime::print( _total_time.getUserTime() ) << " " 
	   << LQIO::DOM::CPUTime::print( _total_time.getSystemTime() ) << " ";
#endif
    output << LQIO::DOM::CPUTime::print( _total_time.getRealTime() ) << " ";

    if ( faults ) {
	output << " ***" << std::setw(3)  << faults;
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
    : _document(document), _valid(false), _iterations(0), _convergenceValue(0.0), _faultCount(0),
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
    _faultCount = total.faults;
    if ( _faultCount > 0 ) {
	_valid = false;
    }
	
    return *this;
}


/* ------------------------ Report Generation  ------------------------ */

/*
 * Print the solver report on output.
 */

std::ostream&
SolverReport::print( std::ostream& output ) const
{
    output << "Convergence test value: " << _convergenceValue << std::endl;
    output << "Number of iterations:   " << _iterations << std::endl << std::endl;

    output << "MVA solver information: " << std::endl;
    output << "Submdl";

    MVACount::printHeader( output, total.faults ) << std::endl;

    for ( unsigned i = 1; i <= MVAStats.size(); ++i ) {
	output << std::setw(3) << i << "  " << MVAStats[i] << std::endl;
    }
    output << "Total" << total << std::endl;

    output << std::endl;
#if HAVE_SYS_UTSNAME_H
    struct utsname uu;		/* Get system triva. */
    uname( &uu );

    output << "    " << uu.nodename << " " << uu.sysname << " " << uu.release << std::endl;
#endif
#if HAVE_SYS_RESOURCE_H && HAVE_GETRUSAGE
    struct rusage r_usage;
    if ( getrusage( RUSAGE_SELF, &r_usage ) == 0 ) {
	if ( r_usage.ru_maxrss > 0 ) {
	    output << "    Max RSS (kB): " << r_usage.ru_maxrss << std::endl;
	    output << "         shared:  " << r_usage.ru_ixrss << std::endl;
	    output << "         data:    " << r_usage.ru_idrss << std::endl;
	    output << "         stack:   " << r_usage.ru_isrss << std::endl;
	    output << "    MAJFLT:       " << r_usage.ru_majflt << std::endl;
	    output << "    MINFLT:       " << r_usage.ru_minflt << std::endl;
	}
    }
#endif
    output << "    Real:    " << LQIO::DOM::CPUTime::print( _delta_time.getRealTime() ) << std::endl;
#if defined(HAVE_SYS_TIME_H)
    output << "    User:    " << LQIO::DOM::CPUTime::print( _delta_time.getUserTime() ) << std::endl;
    output << "    System:  " << LQIO::DOM::CPUTime::print( _delta_time.getSystemTime() ) << std::endl;
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

    std::string buf;

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
    buf = LQIO::io_vars.lq_toolname;
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
