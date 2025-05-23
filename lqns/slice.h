/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/slice.h $
 *
 * Everything you wanted to know about an entry, but were afraid to ask
 * for Markov phase 2 servers.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * $Id: slice.h 16546 2023-03-18 22:32:16Z greg $
 *
 * ------------------------------------------------------------------------
 */

#ifndef LQNS_SLICE_H
#define	LQNS_SLICE_H

#include <set>
#include <mva/prob.h>

class Entry;
class Phase;
class Call;

/* -------------------------------------------------------------------- */

class Slice_Info {
public:
	
    Slice_Info(): nSlices(0.0), service(0.0), lambda_ij(0.0), y_ij(0.0), y_ab(0.0), lambda_ik(0.0), y_ik(0.0), t_k(0.0) {}
    void initialize( const double );
    void initialize( const Phase&, const Entry& dst );
    void accumulate( const double, const Phase&, const Entry& );
    double normalize();
	
    void setRates( const double xj, const Probability& prA );
    double Y_ij() const  { return y_ij; }
    double calls() const { return y_ab; }

    std::ostream& print( std::ostream& = std::cout ) const;

    static void prOvertakingStates( Slice_Info rate[], const unsigned max_phases,
				    Probability PrOTState[MAX_PHASES+1][MAX_PHASES+1][2]);
    static void prStartStates( Slice_Info rate[], const Entry& entC, const Entry& entD,
			       Probability PrStart[]);

    static std::ostream& printOvertakingStates( std::ostream& output, const unsigned j, const double x,
					   const unsigned max_phases,
					   Probability PrOTState[MAX_PHASES+1][MAX_PHASES+1][2] );

	
	
private:
    void getCallInfo( const double, const std::set<Call *>&, const double, const Entry& );

    Probability prodOf_b() const { return b / (1.0 - d); }
    Probability denominator( const double product ) const { return 1.0 - ( b * product + d ); }
    Probability next( const double product ) const { return a * product; }
    Probability prOt( const double product ) const { return c * product; }
	
private:
    double nSlices;		/* Mean slice time.		*/
    double service;		/* Mean serice time (s)		*/
    double lambda_ij;		/*				*/
    double y_ij;		/* Mean calls from task i to j	*/
    double y_ab;		/* Mean calls from entry a to b	*/
    double lambda_ik;		/* Mean tput to other tasks.	*/
    double y_ik;		/* Mean calls to other tasks.	*/
    double t_k;			/* Mean time spent at others.	*/

    Probability a;
    Probability b;
    Probability c;
    Probability d;

};

std::ostream& operator<<( std::ostream&, const Slice_Info& );
#endif
