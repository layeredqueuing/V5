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
 * $Id: report.h 16546 2023-03-18 22:32:16Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_SOLVERREPORT_H
#define	LQNS_SOLVERREPORT_H

#include <lqio/common_io.h>
#include <mva/vector.h>

namespace LQIO {
    namespace DOM {
	class Document;
    }
}

class MVACount;

int operator==( const MVACount&, const MVACount& );	// For Vector.cc

/* ------------------------- Status Reporting. ------------------------ */


class MVACount {
    friend class SolverReport;
    friend class MVASubmodel;
    friend int operator==( const MVACount& a, const MVACount& b );

public:
    static std::ostream& printHeader( std::ostream&, int );

public:
    MVACount() { initialize(); }
    MVACount& operator=( const MVACount& );		/* For copying		*/
    MVACount& operator+=( const MVACount& );		/* For totalling	*/
    MVACount& accumulate( const unsigned long, const unsigned long, const unsigned long );	/* For adding to record	*/
    MVACount& start( const unsigned, const unsigned );	/* Start timing		*/
    MVACount& initialize();				/* Reset counters	*/
    std::ostream& print( std::ostream& ) const;
	
private:
    unsigned _n;			/* number of times called	*/
    unsigned _k;			/* Chains in model		*/
    unsigned _s;			/* Servers in model		*/
    double step;			/* MVA step()'s			*/
    double step_sqr;			/* MVA step()'s			*/
    double wait;			/* wait()'s			*/
    double wait_sqr;			/* wait()'s			*/
    unsigned long faults;		/* MVA failures.		*/

    LQIO::DOM::CPUTime _start_time;       
    LQIO::DOM::CPUTime _total_time;
};


class SolverReport {

private:
    SolverReport( const SolverReport& );
    SolverReport& operator=( const SolverReport& );

public:
    SolverReport( LQIO::DOM::Document *, const Vector<MVACount>& );

    void start();
    SolverReport& finish( bool valid, const double convergence, unsigned long );
    std::ostream& print( std::ostream& ) const;
    const SolverReport& insertDOMResults() const;

    double stepCount() const { return total.step; }
    double waitCount() const { return total.wait; } 
    unsigned long iterations() const { return _iterations; }
    unsigned long faultCount() const { return _faultCount; }
	    
private:
    LQIO::DOM::Document * _document;
    bool _valid;			/* valid solution.		*/
    unsigned long _iterations;		/* Number of major loops.	*/
    double _convergenceValue;		/* Convergence test value.	*/
    unsigned long _faultCount;		/* Number of MVA faults.	*/

    LQIO::DOM::CPUTime _start_time;
    LQIO::DOM::CPUTime _delta_time;

    const Vector<MVACount>& MVAStats;
    MVACount total;
};

inline std::ostream& operator<<( std::ostream& output, const MVACount& self ) { return self.print( output ); }
inline std::ostream& operator<<( std::ostream& output, const SolverReport& self ) { return self.print( output ); }
#endif
