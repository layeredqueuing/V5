/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/report.h $
 *
 * Various statistics of marginal use.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: report.h 11963 2014-04-10 14:36:42Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if	!defined(SOLVERREPORT_H)
#define	SOLVERREPORT_H

#include <config.h>
#include "dim.h"
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_TIMES_H)
#include <sys/times.h>
#endif
#include <time.h>
#include <lqio/dom_document.h>
#include "vector.h"

#if	defined(MSDOS)
#define	clock_t time_t 
#endif

class SolverReport;
class MVACount;
class Model;

ostream& operator<<( ostream& output, const MVACount& self );
ostream& operator<<( ostream& output, const SolverReport& self );
int operator==( const MVACount&, const MVACount& );	// For Vector.cc

/* ------------------------- Status Reporting. ------------------------ */


class MVACount {
    friend class SolverReport;
    friend class MVASubmodel;
    friend int operator==( const MVACount& a, const MVACount& b );

public:
    static ostream& printHeader( ostream&, int );

public:
    MVACount() { initialize(); }
    MVACount& operator=( const MVACount& );		/* For copying		*/
    MVACount& operator+=( const MVACount& );		/* For totalling	*/
    MVACount& accumulate( const unsigned long, const unsigned long, const unsigned long );	/* For adding to record	*/
    MVACount& start( const unsigned, const unsigned );	/* Start timing		*/
    MVACount& initialize();					/* Reset counters	*/
    ostream& print( ostream& ) const;
	
private:
    unsigned _n;			/* number of times called	*/
    unsigned _k;			/* Chains in model		*/
    unsigned _s;			/* Servers in model		*/
    double step;			/* MVA step()'s			*/
    double step_sqr;			/* MVA step()'s			*/
    double wait;			/* wait()'s			*/
    double wait_sqr;			/* wait()'s			*/
    unsigned long faults;		/* MVA failures.		*/

    clock_t start_clock;		/* Time run begins		*/
    clock_t sum_clock;
#if defined(HAVE_SYS_TIMES_H)
    struct tms start_times;
    struct tms sum_times;
#endif
};


class SolverReport {

private:
    SolverReport( const SolverReport& );
    SolverReport& operator=( const SolverReport& );

public:
    SolverReport( LQIO::DOM::Document * document=0 );

    void start();
    SolverReport& finish( const double convergence, const Model& );
    ostream& print( ostream& ) const;
    void insertDOMResults() const;

    double stepCount() const { return total.step; }
    double waitCount() const { return total.wait; } 
    unsigned long iterations() const { return _iterations; }

private:
    LQIO::DOM::Document * _document;
    short _valid;			/* valid solution.		*/
    unsigned long _iterations;		/* Number of major loops.	*/
    double _convergenceValue;		/* Convergence test value.	*/

#if defined(HAVE_SYS_TIMES_H)
    struct tms _start_times;
    struct tms _delta_times;
#endif
    clock_t _start_clock;
    clock_t _delta_clock;

    Vector<MVACount> MVAStats;
    MVACount total;
};

class TimeManip {
public:
    TimeManip( ostream& (*ff)( ostream&, const clock_t ), const clock_t aTime )
	: f(ff), theTime(aTime) {}
private:
    ostream& (*f)( ostream&, const clock_t );
    const clock_t theTime;

    friend ostream& operator<<(ostream & os, const TimeManip& m ) { return m.f(os,m.theTime); }
};
#endif
