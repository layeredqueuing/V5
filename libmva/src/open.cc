/* -*- c++ -*-
 * $Id: open.cc 14305 2020-12-31 14:51:49Z greg $
 *
 * Open Network solver.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * $Date: 2020-12-31 09:51:49 -0500 (Thu, 31 Dec 2020) $
 * ----------------------------------------------------------------------
 * Conventions:
 *    E - (scalar) number of entries for a given station.
 *    e - index from 1..E
 *    J - (scalar) number of servers at station (marginal probabilities)
 *    j - index from 0..J
 *    K - (scalar) number of chains.
 *    k - index from 1..K
 *    M - (scaler) number of stations.
 *    m - index from 1..E
 *    N - (vector) Population.
 *    n - scalar offset into W,U,L to make computations faster.
 *
 * ----------------------------------------------------------------------
 * Open model solver from:
 *      author =    "Lavenberg, Stephen S. and Sauer, Charles H.",
 *      title =     "Analytical Results for Queueing Models",
 *      booktitle = "Computer Performance Modeling Handbook",
 *      publisher = "Academic Press",
 *      year =      1982,
 *      editor =    "Lavenberg, Stephen S.",
 *      volume =    4,
 *      series =    "Notes and Reports in Computer Science and Applied Mathematics.",
 *      pages =     "56--172",
 *      chapter =   3,
 *      address =   "Toronto, ON"
 * ----------------------------------------------------------------------
 *                              !!! NOTE !!!
 *
 * Visits (V) are the `y' (relative throughputs) in the book.
 *
 * ----------------------------------------------------------------------
 * Performance Notes:
 *    *	Arrays are referenced `backwards' (hence bash caches).  They should
 * 	be dimensioned as [N][k][e].  Artifact of populations.
 *
 * ------------------------------------------------------------------------
 */


#include <config.h>
#include <cmath>
#if HAVE_VALUES_H
#include <values.h>
#endif
#include "fpgoop.h"
#include "open.h"
#include "vector.h"
#include "server.h"
#include "prob.h"



/* ----------------------- Helper Functions --------------------------- */
/*
 * Print all results.
 */

std::ostream&
operator<<( std::ostream& output, Open& model )
{
    return model.print( output );
}

/* ----------------------- Abstract Superclass. ----------------------- */

/*
 * Constructor for MVA.  Initialize global bounds.
 *
 * Instance Variables.
 *	M: stations.
 *	Q: Supplied server types for each station.
 *	L: Queue Length.  (Storage allocated by subclass)
 *	U: Station utilization (Storage allocated by subclass)
 */

Open::Open( Vector<Server *>& q) : M(q.size()), Q(q)
{
    /* Set index in each station */

    for ( unsigned i = 1; i <= M; ++i ) {
	Q[i]->openIndex = i;
    }
}


/*
 * Free storage.
 */

Open::~Open()
{
}


/*
 * Convert mixed (open) to closed model.  Return 0 on success.  Return
 * station number of last station we patched up.  
 */

void
Open::convert( const Population& N ) const
{
    unsigned m_err = 0;
    const unsigned n = N.sum();
	
    for ( unsigned m = 1; m <= M; ++m ) {
	
	try {
	    const double num = Q[m]->alpha( n - 1 );
	    const double den = Q[m]->alpha( n );
	    if ( std::isfinite( num ) && std::isfinite( den ) ) {
		*Q[m] *= (den / num);
	    } else {
		*Q[m] = get_infinity();
		m_err = m;
	    }
	} 
	catch ( const std::range_error& ) {
	    *Q[m] = get_infinity();
	    m_err = m;
	}
	catch ( const std::domain_error& ) {
	    *Q[m] = get_infinity();
	    m_err = m;
	}
    }

    if ( m_err != 0 ) {
	throw std::range_error( "Open::convert" );
    }
}


/*
 * Solve open class stuff after closed model solution.
 */

void
Open::solve( const MVA& closedModel, const Population& N )
{
    unsigned m_err = 0;

    for ( unsigned m = 1; m <= M; ++m ) {
	if ( Q[m]->V(0) == 0.0 ) continue;		// might get this on `z' type messages.

	try {
	    if ( Q[m]->closedIndex != 0 ) {
		Q[m]->mixedWait( closedModel, N );
	    } else {
		Q[m]->openWait();
	    }
	} 
	catch ( const std::range_error& e ) {
	    *Q[m] = get_infinity();
	    m_err = m;
	}
	catch ( const std::domain_error& e ) {
	    *Q[m] = get_infinity();
	    m_err = m;
	}
    }

    if ( m_err != 0 ) {
	throw std::range_error( "Open::solve" );
    }
}


/*
 * Solve open class solution.
 */

void
Open::solve()
{
    unsigned m_err = 0;

    for ( unsigned m = 1; m <= M; ++m ) {
	try {
	    Q[m]->openWait();
	} 
	catch ( const std::range_error& e ) {
	    *Q[m] = get_infinity();
	    m_err = m;
	}
	catch ( const std::domain_error& e ) {
	    *Q[m] = get_infinity();
	    m_err = m;
	}
    }

    if ( m_err != 0 ) {
	throw std::range_error( "Open::solve" );
    }
}


double
Open::throughput( const Server& aStation ) const
{
    double sum = 0.0;
    for ( unsigned e = 1; e <= aStation.nEntries(); ++e ) {
	sum += entryThroughput( aStation, e );
    }
    return sum;
}

/*
 * Get throughput.  For the open model is the the lesser of the arrival rate or the service time. 
 */

double 
Open::entryThroughput( const Server& aStation, const unsigned e ) const
{
    if ( aStation.V( e, 0 ) == 0 ) {
	return 0;
    } else if ( aStation.Rho() < aStation.mu() ) {
	return aStation.V( e, 0 );
    } else if ( !std::isfinite(aStation.mu()) && !std::isfinite(aStation.S( e, 0 )) ) {
	return aStation.V( e, 0 );		/* BUG_566 Infinite Server */
    } else {
	return aStation.V( e, 0 ) * aStation.mu() / aStation.Rho();	/* Server overloaded.  Scale back throughput */
    }
}



double
Open::utilization( const Server& aStation ) const
{
    double sum = 0.0;
    for ( unsigned e = 1; e <= aStation.nEntries(); ++e ) {
	sum += entryThroughput( aStation, e ) * aStation.S(e,0);
    }
    return sum;
}


double 
Open::entryUtilization( const Server& aStation, const unsigned e ) const
{
    return entryThroughput( aStation, e ) * aStation.S(e,0);
}

/*
 * Print throughput and utilization for all stations.
 */

std::ostream&
Open::print( std::ostream& output ) const
{
    int width = output.precision() + 2;

    for ( unsigned m = 1; m <= M; ++m ) {
	unsigned count = 0;
	const unsigned E = Q[m]->nEntries();

	for ( unsigned e = 1; e <= E; ++e ) {
	    const double W = Q[m]->R(e,0);
	    if ( !W ) continue;

	    count += 1;
	    if ( count == 1 ) {
		output << m << ": ";
	    } else {
		output << "   ";
	    }
			
	    output << "W_0" << e << " = " << std::setw(width) << W;
	    output << ", U_0" << e << " = " << std::setw(width) << entryUtilization( *Q[m], e );
	    output << std::endl;
	}
    }
    return output;
}
