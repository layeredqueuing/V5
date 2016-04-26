/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/* August 2003.								*/
/* January 2005.							*/
/************************************************************************/

/*
 * $Id: results.h 11061 2012-07-06 14:25:08Z greg $
 *
 * Solve LQN using petrinets.
 */

#if !defined(PETRISRVN_RESULTS_H)
#define PETRISRVN_RESULTS_H

#include <iostream>

/* Results from solution. */
struct solution_stats_t 
{
    solution_stats_t() : tangible(0), vanishing(0), precision(0) {}	
    int tangible;			/* Tangible markings in net	*/
    int vanishing;			/* Vanishing markings in net.	*/
    double precision;			/* Precision of result.		*/

    std::ostream& print( std::ostream& ) const;
};
    
inline std::ostream& operator<<( std::ostream& output, const solution_stats_t& self ) { return self.print( output ); }

void get_results(void);
double get_pmmean( const char * fmt, ...);
double get_tput( short kind, const char * fmt, ... );
double get_prob( int m, const char * fmt, ...);

#endif
