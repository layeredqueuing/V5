/* -*- C++ -*-
 * $Id: syncserv.cc 14319 2021-01-02 04:11:00Z greg $
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */

#include "dim.h"
#include <cmath>
#include <stdarg.h>
#include <cstdlib>
#include <mva/vector.h>
#include <mva/server.h>
#include <mva/ph2serv.h>
#include <mva/prob.h>
#include <mva/mva.h>
#include "syncserv.h"

#define DEBUG


/* --- Simple Multi-Server.  All chains have identical service time --- */


void
Synch_Server::initialize()
{
    if ( E != 2 ) {
	throw domain_error( "Synch_Server::initialize" );
    }
}


/*
 * Catch bogus synch server constructors.
 */

void
Synch_Server::shouldNotImplement()
{
	throw should_not_implement( "Synch_Server", __FILE__, __LINE__ );
}


/*
 * Store client chain for entry.  
 */

Server&
Synch_Server::setClientChain( const unsigned e, const unsigned k )
{
	assert( 0 < k && k <= K && 0 < e && e <= 2 );
	chainForEntry[e] = k;
	return *this;
}





/*
 * See [rolia](pg 167)
 */

void
Synch_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
	assert( 0 < k && k <= K );

	const Positive sum  = solver.sumOf_SL_m( *this, N, k );

	for ( unsigned e = 1; e <= E; ++e ) {
		if ( !V(e,k) ) continue;
		const double sum2 = Upsilon( solver, e, N, k ) + gamma( solver, e, N, k );

		for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
			Positive temp = S(e,k) + sum + sum2;				/* Eq:1 */
			W[e][k][p] = temp;
		}
	}
}



/*
 * Queue length for Open models. [lavenberg] (3.128)
 */

void
Synch_Server::openWait() const
{
	throw should_not_implement( "Synch_Server::openWait", __FILE__, __LINE__ );
}


/*
 * Queue length for Open models [lavenberg] (3.324).
 */

void
Synch_Server::mixedWait( const MVA&, const Population& ) const
{
	throw should_not_implement( "Synch_Server::mixedWait", __FILE__, __LINE__ );
}



/*
 *
 */

Positive
Synch_Server::Upsilon( const MVA& solver, const unsigned e,
		       const Population &N, const unsigned k ) const
{
	if ( N[k] == 0 ) return 0;
	
	Population Nej = N;
	Nej[k] -= 1;
	
	Positive rate[3];

	rate[1] = solver.arrivalRate( *this, 1, chainForEntry[1], Nej );
	rate[2] = solver.arrivalRate( *this, 2, chainForEntry[2], Nej );
	if ( rate[1] == 0.0 || rate[2] == 0.0 ) return 0.0;
	
	Positive syncDelay = max( rate[1], rate[2] ) - 1.0 / rate[e];
	Positive delay = syncDelay * alpha( solver, e, Nej );

#if	defined(DEBUG)
	cerr << "Upsilon( " << N << "," << e << "," << k << ") = " << delay << endl;
#endif
	return delay;
}



/*
 *
 */

Positive
Synch_Server::gamma( const MVA& solver, const unsigned e,
		     const Population &N, const unsigned k ) const
{
	if ( N[k] == 0 ) return 0;

	const unsigned m = closedIndex;
	Population Nej    = N;
	Nej[k] -= 1;

	const unsigned e2 = ( e == 1 ? 2 : 1 );

	const Positive delay = solver.arrivalRate( *this, e2, chainForEntry[e2], Nej )
		* V(e,k) * solver.L[Nej][m][e][k] * (1.0 - solver.U[N][m][e][k]);

#if	defined(DEBUG)
	cerr << "gamma( " << N << "," << e << "," << k << ") = " << delay << endl;
#endif
	return delay;
}

/*
 * \Upsilon term from page 168.
 */

Positive
Synch_Server::max( const double lambda_1, const double lambda_2 ) const
{
	const Positive temp = 1.0 / lambda_1 + 1.0 / lambda_2 - 1.0 / ( lambda_1 + lambda_2 );
	return temp;
}



/*
 *
 */

double
Synch_Server::alpha( const MVA& solver, const unsigned e, const Population &N ) const
{
	const unsigned n = N[chainForEntry[e]];
	if ( n <= 1 ) return 1.0;

	const Probability delta = solver.syncDelta( *this, e, chainForEntry[e], N );

	return power( 1.0 - delta, n - 1 );
}



/*
 * Print information about this station.
 */

ostream&
Synch_Server::printHeading( ostream& output ) const
{
	return output;
}
