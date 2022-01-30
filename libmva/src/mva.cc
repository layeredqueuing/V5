/* -*- c++ -*-
 * $Id: mva.cc 15404 2022-01-28 03:09:36Z greg $
 *
 * MVA solvers: Exact, Bard-Schweitzer, Linearizer and Linearizer2.
 * Abstract superclass does no operation by itself.
 *
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
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
 * All solvers execept fast linearizer from:
 *     author =   "Chandy, K. Mani and Neuse, Doug",
 *     title =    "Linearizer: A Heuristic Algorithm for Queueing
 *                 Network Models of Computing Systems",
 *     journal =  cacm,
 *     year =     1982,
 *     volume =   25,
 *     number =   2,
 *     pages =    "126--134",
 *     month =    feb
 *
 * Other stuff from:
 *     author =   "Lazowska, Edward D. and Zhorjan, John and Graham,
 *                 Scott G. and Sevcik, Kenneth C.",
 *     title =    "Quantitative System Performance; Computer System
 *                 Analysis Using Queueing Network Models",
 *     publisher = "Prentice-Hall",
 *     year =      1984,
 *     address =   "Englewood Cliffs, NJ",
 *     callno =    "QA76.9.E94Q36"
 *
 * Marginal Queue stuff for multiservers from:
 *     author =    "Conway, Adrian E.",
 *     title =     "Fast Approximate Solution of Queueing Networks with
 *                  Multi-Server Chain-Dependent {FCFS} Queues",
 *     booktitle = "Modeling Techniques and Tools for Computer Performance Evaluation",
 *     publisher = "Plenum",
 *     year =      1989,
 *     editor =    "Puigjaner, Ramon and Potier, Dominique",
 *     pages =     "385--396",
 *     address =   "New York",
 *     callno =    "QA76.9.C65I54",
 *     isbn =      "0-306-43368-0",
 *
 * Linearizer algorithm now from:
 *     author =    "Krzesinski, A. and Greyling, J.",
 *     title =     "Improved Linearizer Methods for Queueing Networks
 *                  with Queue Dependent Service Centers",
 *     booktitle = "Proceedings of 1984 {ACM} {SIGMETRICS} on
 *                  Measurement and Modeling of Computer Systems"
 *     year =      1984,
 *     isbn =      "0163-5999",
 *     callno =    "QA76.9.E94P47",
 *
 * Priority MVA from:
 *     author =    "Bryant, Raymond M. and  Krzesinski, Anthony E. and
 *                  Lakshmi, M. Seetha and Chandy, K. Mani",
 *     title =     "The {MVA} Priority Approximation",
 *     journal =   tocs,
 *     callno =    "QA76.A1A25",
 *     year =      1984,
 *     volume =    2,
 *     number =    4,
 *     pages =     "335--359",
 *     month =     nov,
 *
 *     author =    "Eager, Derek L. and Lipscomb, John N.",
 *     title =     "The {AMVA} Priority Approximation",
 *     journal =   perf,
 *     volume =    8,
 *     year =      1988,
 *     pages =     "173--193",
 *
 * Replication from:
 *     author =    "Pan, Amy M.",
 *     title =     "Solving Stochastic Rendezvous Networks of Large
 *                  Client-Server Systems with Symmetric Replication",
 *     school =    sce,
 *     year =      1996,
 *     month =     sep,
 *     note =      "OCIEE-96-06",
 *
 * Schmidt Multiserver from:
 *     author =    "Schmidt, Rainer",
 *     title =     "An Approximate {MVA} Algorithm for Exponential,
 *                  Class-Dependent Multiple Servers",
 *     journal =   "Performance Evaluation"
 *     year =      1997,
 *     volume =    29,
 *     pages =     "245--254",
 * ----------------------------------------------------------------------
 *                              !!! NOTE !!!
 *
 * Common expression hoisting has been performed to reduce computational
 * costs.  The expressions are marked with the comment "Hoist".  It is
 * assumed that the size and the maximum number of customers for all
 * Population vectors for all stations is the same so that the offset for
 * a given population is the same for all population vectors.  The offset
 * computation is hoisted out of the loops where possible.  The integer
 * variable `n' represents the array offset for the customer population
 * `N', i.e., "n = population.offset(N)".  The integer variable `Nej'
 * represents the array offset of customer population `N' with one
 * customer of class j removed.  See also: population.[Ch].
 *
 * The instance variable `c' used in the Linearizer solver and subclasses
 * is also very special.  It represents the class of the customer being
 * removed by the linearizer algorithm.  This variable is not be
 * explicitly visible.  However, it is used, so DO NOT USE `c' as a local
 * variable ANYWHERE.  It is used in ALL CLASSES including MVA (especially
 * MVA::step())
 *
 * define KRZESINSKI for a funky version of the marginal probability
 * calculation.  Too bad it does not work right now.
 * ------------------------------------------------------------------------
 */

//#define DEBUG_MVA	true

#include <config.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <limits>
#include "mva.h"
#include "mvaexception.h"
#include "server.h"
#include "prob.h"

int MVA::__bounds_limit = 0;		/* Enable bounds limiting if non-zero */
double MVA::MOL_multiserver_underrelaxation = 0.5;	/* For MOL Multiservers */
#if DEBUG_MVA
bool MVA::debug_D = false;
bool MVA::debug_L = false;
bool MVA::debug_N = false;
bool MVA::debug_P = false;
bool MVA::debug_U = false;
bool MVA::debug_W = false;
bool MVA::debug_X = false;
#endif

/* ----------------------- Helper Functions --------------------------- */

/*
 * Print all results.
 */

std::ostream& operator<<( std::ostream& output, MVA& model )
{
    return model.print( output );
}


/* ----------------------- Abstract Superclass. ----------------------- */

/*
 * Constructor for MVA.  Initialize global bounds.
 *
 * Instance Variables.
 *	M: stations.
 *	K: classes.
 *	Q: Supplied server types for each station.
 *	NCust: Maximum number of customers in each class.
 *	L: Queue Length.  (Storage allocated by subclass)
 *	U: Station utilization (Storage allocated by subclass)
 */

MVA::MVA( Vector<Server *>& q, const Population& N,
	  const Vector<double>& thinkTime, const Vector<unsigned>& prio,
	  const Vector<double>* of )
    : NCust(N), M(q.size()), K(N.size()), Q(q), Z(thinkTime),
      priority(prio), overlapFactor(of), L(), U(), P(), X(),
      faultCount(0), maxP(q.size()),
      nPrio(0), sortedPrio(), stepCount(0), waitCount(0), _isThread(), maxOffset(0)
{
    assert( M > 0 && K > 0 );
    initialize();
}



/*
 * Release storage.
 */

MVA::~MVA()
{
    dimension( 0 );
}


/*
 * Allocate queue length storage for all M stations and K classes.
 */

void
MVA::dimension( const size_t mapMaxOffset )
{
    if ( maxOffset < mapMaxOffset ) {

	/* If array got bigger, add more space */

	L.resize(mapMaxOffset);
	U.resize(mapMaxOffset);
	P.resize(mapMaxOffset);
	X.resize(mapMaxOffset);

	for ( unsigned n = maxOffset; n < mapMaxOffset; ++n) {
	    L[n] = new double ** [M+1];
	    U[n] = new double ** [M+1];
	    P[n] = new double * [M+1];
	    L[n][0] = nullptr;
	    U[n][0] = nullptr;
	    P[n][0] = nullptr;

	    for ( unsigned m = 1; m <= M; ++m ) {
		const unsigned E = Q[m]->nEntries();
		L[n][m] = new double * [E+1];
		U[n][m] = new double * [E+1];
		P[n][m] = nullptr;
		L[n][m][0] = nullptr;
		U[n][m][0] = nullptr;

		for ( unsigned e = 1; e <= E; ++e ) {
		    L[n][m][e] = new double [K+1];
		    U[n][m][e] = new double [K+1];

		    for ( unsigned k = 0; k <= K; k++ ) {
			L[n][m][e][k] = 0.0;
			U[n][m][e][k] = 0.0;
		    }
		}
	    }

	    X[n] = new double [K+1];

	    for ( unsigned k = 1; k <= K; ++k ) {
		X[n][k] = 0.0;
	    }
	}

    } else if ( maxOffset > mapMaxOffset ) {

	/* if the array got smaller, delete space from the end. */

	for ( unsigned n = maxOffset; n > mapMaxOffset; ) {
	    n -= 1;
	    for ( unsigned m = 1; m <= M; ++m ) {
		const unsigned E = Q[m]->nEntries();

		for ( unsigned e = 1; e <= E; ++e ) {
		    delete [] L[n][m][e];
		    delete [] U[n][m][e];
		}

		delete [] L[n][m];
		delete [] U[n][m];
		delete [] P[n][m];
	    }

	    delete [] L[n];
	    delete [] U[n];
	    delete [] P[n];
	    delete [] X[n];

	}

	L.resize(mapMaxOffset);
	U.resize(mapMaxOffset);
	X.resize(mapMaxOffset);
	P.resize(mapMaxOffset);
    }

    dimension( P, mapMaxOffset );
    setMaxP();
    
    maxOffset = mapMaxOffset;
}



/*
 * Allocate space for stations needing marginal probabilities.  This
 * is a little more hairy than the other dimension case because the
 * size of the marginal probabilities can change too.
 */

bool
MVA::dimension( std::vector<double **>& array, const size_t mapMaxOffset )
{
    bool rc = false;
    for ( unsigned n = 0; n < mapMaxOffset; ++n ) {
	for ( unsigned m = 1; m <= M; ++m ) {
	    const unsigned J = Q[m]->marginalProbabilitiesSize();
	    if ( (J == 0 || J != maxP[m]) && array[n][m] ) {	// Size change
		delete [] array[n][m];
		array[n][m] = nullptr;
		rc = true;
	    }
	    if ( J != 0 && array[n][m] == nullptr ) {
		array[n][m] = new double [J+1];
		for ( unsigned j = 0; j <= J; ++j ) {
		    array[n][m][j] = 0.0;
		}
		array[n][m][0] = 1.0;	// Initially, no servers busy with zero customers.
		rc = true;
	    }
	}
    }
    return rc;
}


/*
 * Set the maximum dimension of the marginals.  It's NOT done in dimension because we may have to redimension two arrays (P and saved_P); the old value is needed.
 */

void
MVA::setMaxP()
{
    for ( unsigned m = 1; m <= M; ++m ) {
	maxP[m] = Q[m]->marginalProbabilitiesSize();
    }
}


/*
 * Reset everything back to zero.
 */

void
MVA::reset()
{
    dimension( P, maxOffset );

    for ( unsigned n = 0; n < maxOffset; ++n) {
	assert( L[n][0] == nullptr );
	assert( U[n][0] == nullptr );

	for ( unsigned m = 1; m <= M; ++m ) {
	    const unsigned E = Q[m]->nEntries();
	    assert( L[n][m][0] == nullptr );
	    assert( U[n][m][0] == nullptr );

	    for ( unsigned e = 1; e <= E; ++e ) {
		for ( unsigned k = 0; k <= K; k++ ) {
		    L[n][m][e][k] = 0.0;
		    U[n][m][e][k] = 0.0;
		}
	    }

	    if ( P[n][m] ) {
		const unsigned J = Q[m]->marginalProbabilitiesSize();
		for ( unsigned j = 0; j <= J; ++j ) {
		    P[n][m][j] = 0.0;
		}
		P[n][m][0] = 1.0;	// Initially, no servers busy with zero customers.
	    }
	}

	for ( unsigned k = 1; k <= K; ++k ) {
	    X[n][k] = 0.0;
	}
    }

    setMaxP();
}



/*
 * Initialize the sorted priority array.  Priorities are sorted from
 * highest (= 0) to lowest (= +oo).  Duplicates are removed from the
 * list.
 */

void
MVA::initialize()
{
    for ( unsigned m = 1; m <= M; ++m ) {
	Q[m]->closedIndex = m;					/* Set index in each station */
	Q[m]->setMarginalProbabilitiesSize( NCust.size() );
	maxP[m] = 0;
    }
	
    sortedPrio.resize(K);
    _isThread.resize(K+1);

    for ( unsigned k = 1; k <= K; ++k ) {
	_isThread[k] = 0;
    }
    nPrio = 1;
    sortedPrio[nPrio] = priority[1];

    for ( unsigned k = 2; k <= K; ++k ) {
	unsigned p;
	for ( p = 1; p <= nPrio; ++p ) {
	    if ( priority[k] >= sortedPrio[p] ) break;
	}
	if ( p > nPrio ) {
	    nPrio += 1;
	    sortedPrio[nPrio] = priority[k];
	} else if ( priority[k] > sortedPrio[p] ) {
	    for ( unsigned i = nPrio; i >= p; --i ) {
		sortedPrio[i+1] = sortedPrio[i];
	    }
	    nPrio += 1;
	    sortedPrio[p] = priority[k];
	}
    }
}




/*
 * Step by priority.
 */

void
MVA::step( const Population& N )
{
    stepCount += 1;
    for ( unsigned m = 1; m <= M; ++m ) {
	Q[m]->initStep( *this );
    }

    /* On sorted list do.... */
    for ( unsigned i = 1; i <= nPrio; ++i ) {
	step( N, sortedPrio[i] );
    }
}


/*
 * MVA core solver.  NB: the waiting time for each entry and class for a
 * particular station m, Q[m]->W[e][k], is solved based on the class of
 * the station (eg, Q[m]->wait()).  See Server.c for station
 * types.
 */

void
MVA::step( const Population& N, const unsigned currPri )
{
    unsigned m;			/* Station index.		*/
    unsigned e;			/* Entry index.			*/
    unsigned k;			/* Class index.			*/
    for ( m = 1; m <= M; ++m ) {
	for ( k = 1; k <= K; ++k ) {
	    if ( priority[k] != currPri ) continue;
	    waitCount += 1;
	    Q[m]->wait( *this, k, N );					/* Eq:1 */
	}
    }

#if DEBUG_MVA
    if ( debug_N ) std::cout << N << std::endl;
    if ( debug_W ) printW( std::cout );
#endif

    const unsigned n = offset(N);					/* Hoist */

    for ( k = 1; k <= K; ++k ) {
	if ( priority[k] != currPri ) continue;
	double sum = Z[k];

	for ( m = 1; m <= M; ++m ) {
	    const unsigned E = Q[m]->nEntries();
	    for ( e = 1; e <= E; ++e ) {
		if ( std::isfinite( Q[m]->R(e,k) ) ) {
		    sum += Q[m]->R(e,k);
		} else {
		    sum = std::numeric_limits<double>::infinity();
		    break;
		}
	    }
	}

	if ( sum <= 0.0 ) {
	    X[n][k] = std::numeric_limits<double>::infinity();
	} else if ( !std::isfinite( sum ) ) {
	    X[n][k] = 0.;
	} else {
	    X[n][k] = N[k] / sum;					// throughput
	}

	for ( m = 1; m <= M; ++m ) {
	    const unsigned E = Q[m]->nEntries();

	    for ( e = 1; e <= E; ++e ) {
		if ( !std::isfinite( X[n][k] ) || X[n][k] == 0.0 ) {			/* inf */
		    L[n][m][e][k] = 0.0;
		    U[n][m][e][k] = 0.0;
		} else {
		    L[n][m][e][k] =  Q[m]->interlock( e, k, X[n][k] ) * Q[m]->R(e,k);	/* Eq:6 */

		    /* Don't use interlocked flow to find U. */

		    U[n][m][e][k] = X[n][k] * Q[m]->V(e,k) * Q[m]->S(e,k);	/* Eq:3,7 */
		}
	    }
	}
    }

    /* Step for Multiserver MVA and whatever else needs marginal queue values. */

    for ( m = 1; m <= M; ++m ) {
	if ( Q[m]->vectorProbabilities() ) {
	    marginalProbabilities2( m, N );		/* Schmidt multiserver */
	} else if ( P[n][m] ) {
	    marginalProbabilities( m, N );
	}
    }

#if DEBUG_MVA
    if ( debug_U ) printU( std::cout, N );
    if ( debug_P ) printP( std::cout, N );
    if ( debug_X ) printX( std::cout );
#endif

    if ( !check_fp_ok() ) {
	throw floating_point_error( __FILE__, __LINE__ );
    }
}



/*
 * Return number of customers regardless of class at queue `m'.
 * Subtract 1 customer from class `j'.  Implemented as part of solver
 * because Linearizer2 redefines the operation.  Otherwise, it would
 * belong in the server class.
 *
 * Adjustment for `tau' done here.
 */

double
MVA::sumOf_L_m( const Server& station, const Population &N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] < priority[j] ) continue;
	const double scaling = tau_overlap( station, j, k, N );

	for ( unsigned e = 1; e <= E; ++e ) {
	    sum += L[Nej][m][e][k] * scaling;
	}
    }
    return sum;
}



/*
 * Return number of customers regardless of class at queue `m' multiplied
 * by the service time for the class at the entry.  Subtract 1 customer
 * from class `j'.  Implemented as part of solver because Linearizer2
 * redefines the operation.  Otherwise, it would belong in the server
 * class.
 */

double
MVA::sumOf_SL_m( const Server& station, const Population &N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] < priority[j] ) continue;
	const double scaling = tau_overlap( station, j, k, N );			/* BUG 145 */

	for ( unsigned e = 1; e <= E; ++e ) {
	    const double s = station.S(e,k);
	    if ( !std::isfinite(s) ) return s;					/* Infinitiy */
	    sum += s * L[Nej][m][e][k] * scaling;
	}
    }
    return sum;
}



/*
 * Return number of customers regardless of class at queue `m' multiplied
 * by the service time for the class at the entry.  Subtract 1 customer
 * from class `j'.  Implemented as part of solver because Linearizer2
 * redefines the operation.  Otherwise, it would belong in the server
 * class.
 */

double
MVA::sumOf_SQ_m( const Server& station, const Population &N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] < priority[j] ) continue;
	const double scaling = tau_overlap( station, j, k, N );

	for ( unsigned e = 1; e <= E; ++e ) {
	    const double delta = L[Nej][m][e][k] - U[Nej][m][e][k];
	    if ( delta <= 0.0 ) continue;
	    const double s = station.S(e,k);
	    if ( !std::isfinite(s) ) return s;						/* Infinity */
	    sum += s * delta * scaling;
	}
    }
    return sum;
}



/*
 * Term for HOL priority servers.  Returns service time multiplied by utilization
 * for all classes with lower priorities than class j.
 */

double
MVA::sumOf_SU_m( const Server& station, const Population &N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] >= priority[j] ) continue;
	const double scaling = tau_overlap( station, j, k, N );

	for ( unsigned e = 1; e <= E; ++e ) {
	    sum += station.S(e,k) * U[Nej][m][e][k] * scaling;
	}
    }
    return sum;
}



/*
 * Compute sum over all classes of utilization multiplied by the
 * difference of the mean residual life and the service time for each
 * entry.
 */

double
MVA::sumOf_rU_m( const Server& station, const Population& N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] == 0 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] < priority[j] ) continue;
	const double scaling = tau_overlap( station, j, k, N );

	for ( unsigned e = 1; e <= E; ++e ) {
	    const double r = station.r(e,k);
	    if ( !std::isfinite( r ) ) return r;					/* Infinity */
	    sum += r * U[Nej][m][e][k] * scaling;
	}
    }
    return sum;
}



/*
 * Compute sum over all classes of utilization multiplied by the phase 2+
 * service time for each entry.
 */

double
MVA::sumOf_S2U_m( const Server& station, const unsigned e, const Population& N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] == 0 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] < priority[j] ) continue;
	double Uk = U[Nej][m][e][k];
	sum += station.S_2(e,k) * Uk * tau_overlap( station, j, k, N );
    }

    return sum;
}



/*
 * Compute sum over all classes of utilization multiplied by the phase 2+
 * service time for each entry.
 */

double
MVA::sumOf_S2U_m( const Server& station, const Population& N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] < priority[j] ) continue;
	const double scaling = tau_overlap( station, j, k, N );

	for ( unsigned e = 1; e <= E; ++e ) {
	    sum += station.S_2(e,k) * U[Nej][m][e][k] * scaling;
	}
    }

    return sum;
}



/*
 * Compute sum over all classes of utilization multiplied by the phase 2+
 * service time for each entry.
 */

double
MVA::sumOf_S2_m( const Server& station, const Population& N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum_S = 0.0;
    double sum_X = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] < priority[j] ) continue;

	for ( unsigned e = 1; e <= E; ++e ) {
	    sum_S += station.S_2(e,k) * station.V(e,k) * X[Nej][k];
	    sum_X += station.V(e,k) * X[Nej][k];
	}
    }

    return sum_X ? sum_S / sum_X : 0.0;
}



/*
 * Return utilization with a customer from class `j' removed.
 */

double
MVA::sumOf_U_m( const Server& station, const Population& N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;

    for ( unsigned k = 1; k <= K; ++k ) {
	const double scaling = tau_overlap( station, j, k, N );

	for ( unsigned e = 1; e <= E; ++e ) {
	    sum += U[Nej][m][e][k] * scaling;
	}
    }

    return sum;
}



/*
 * Second term for overtaking to compensate for missing utilization.
 */

double
MVA::sumOf_USPrOt_m( const Server& station, const unsigned e, const Probability& PrOt, const Population &N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] == 0 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	const double s = station.S(e,k);
	if ( !std::isfinite(s) || s == 0 ) continue;
	const double Uk = U[Nej][m][e][k];
	if ( Uk == 0.0 ) continue;

	const double SOt = ( 1.0 - PrOt ) * square( station.S_2(e,k) ) / s - PrOt * station.S(e,k,1);
	if ( SOt <= 0.0 ) continue;

	const double scaling = tau_overlap( station, j, k, N );

	sum += SOt * Uk * scaling;
    }

    return sum;
}


/*
 * Compute sum over all classes of utilization multiplied by the phase 2+
 * service time for each entry.
 */

double
MVA::sumOf_U2_m( const Server& station, const unsigned k, const Population& N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] == 0 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum  = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	const double service = station.S(e,k);
	if ( service == 0.0 || ( station.priorityServer() && priority[k] < priority[j] )) continue;
	const double scaling = tau_overlap( station, j, k, N );
	sum  += U[Nej][m][e][k] * (station.S_2(e,k) / service) * scaling;
    }

    return sum;
}



/*
 * Compute sum over all classes of phase 2 utilization.
 */

double
MVA::sumOf_U2_m( const Server& station, const Population& N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] == 0 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);					/* Hoist */

    double sum  = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( station.priorityServer() && priority[k] < priority[j] ) continue;
	double scaling = tau_overlap( station, j, k, N );

	for ( unsigned e = 1; e <= E; ++e ) {
	    const double service = station.S(e,k);
	    if ( service == 0.0 ) continue;

	    sum  += U[Nej][m][e][k] * (station.S_2(e,k) / service) * scaling;
	}
    }

    return sum;
}



/*
 * Return probability term. (Reiser. eqn 3.8)
 */

double
MVA::sumOf_P( const Server& station, const Population &N, const unsigned k ) const
{
    assert( 0 < k && k <= K );
    if ( N[k] < 1 ) return 0.0;

    const unsigned J   = static_cast<unsigned>(station.mu());
    if ( J < 2 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned Nek = offset_e_j(N,k);					/* Hoist */

    double sum = 0.0;
    for ( unsigned j = 0; j <= J - 2; ++j ) {
	sum += ( J - 1 - j ) * P[Nek][m][j];
    }
    return sum;
}



/*
 * Return probability term multiplied by class dependent service time.
 * See Bruell Eqn 5.
 */

double
MVA::sumOf_SP2( const Server& station, const Population &N, const unsigned k ) const
{
    assert( 0 < k && k <= K );
    if ( N[k] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned Nek = offset_e_j(N,k);

    Population::IteratorOffset next( NCust, N );
    Population I(K);				// Need to sequence over this.

    /* Iterate over all populations, excluding P(0). */

    double sum = 0.0;
    while ( next( I ) ) {
	if ( I[k] == 0 ) continue;

	const unsigned Iek = next.offset_e_j(I,k);

	sum += Q[m]->muS( I, k ) * P[Nek][m][Iek];
    }

    return sum;
}


/*
 * Return probability term [levenberg] eqn ...
 */

double
MVA::sumOf_alphaP( const Server& station, const Population &N ) const
{
    const unsigned J   = static_cast<unsigned>(station.mu());
    if ( J < 2 ) return 0.0;

    const unsigned n   = offset(N);						/* Hoist */
    const unsigned m   = station.closedIndex;

    double sum = 0.0;
    for ( unsigned j = 0; j <= J; ++j ) {
	sum += ((j+1) * station.alpha(j+1)) / (station.mu(j+1) * station.alpha(j)) * P[n][m][j];
    }

    return sum;
}



/*
 * Return the marginal probability that all stations are busy.
 */

double
MVA::PB( const Server& station, const Population &N, const unsigned k ) const
{
    assert( 0 < k && k <= K );

    if ( N[k] < 1 ) return 0.0;

    const unsigned J = station.marginalProbabilitiesSize();
    const unsigned m = station.closedIndex;
    const unsigned NeK = offset_e_j(N,k);

    return P[NeK][m][J];
}



/*
 * Return the marginal probability that all stations are busy.
 */

double
MVA::PB2( const Server& station, const Population &N, const unsigned k ) const
{
    const double U1_m   = std::min( 1.0, sumOf_U_m( station, N, k ) / station.mu() );
    return power( U1_m, static_cast<unsigned>(station.mu()) );
}



double
MVA::throughput( const unsigned k ) const
{
    return X[offset(NCust)][k];
}



double
MVA::throughput( const unsigned k, const Population& N ) const
{
    return X[offset(N)][k];
}


/*
 * Return throughtput at `station'.
 */

double
MVA::throughput( const Server& station ) const
{
    const unsigned m = station.closedIndex;
    const unsigned n = offset(NCust);						/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( std::isfinite( X[n][k] ) ) {						/* inf */
	    sum += Q[m]->V(k) * X[n][k];
	} else if ( Q[m]->V(k) > 0.0 ) {
	    sum = X[n][k];
	    break;
	}
    }

    return sum;
}



/*
 * Return throughtput at `station'.
 */

double
MVA::throughput( const Server& station, const unsigned k ) const
{
    const unsigned m = station.closedIndex;
    const unsigned n = offset(NCust);						/* Hoist */

    if ( std::isfinite( X[n][k] ) ) {
	return Q[m]->V(k) * X[n][k];
    } else {
	return X[n][k];
    }
}



/*
 * Return throughtput at `station', entry `e'
 */

double
MVA::entryThroughput( const Server& station, const unsigned e ) const
{
    const unsigned m = station.closedIndex;
    const unsigned n = offset(NCust);						/* Hoist */

    double sum = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( !std::isfinite( X[n][k] ) ) return X[n][k];
	if (getThreadChain(k)){
	    sum += Q[m]->V(e,k) * X[n][getThreadChain(k)];
	} else {
	    sum += Q[m]->V(e,k) * X[n][k];
	}
    }
    return sum;
}


/*
 * Return throughtput at `station', entry `e'
 */

double
MVA::throughput( const Server& station, const unsigned e, const unsigned k ) const
{
    const unsigned m = station.closedIndex;
    const unsigned n = offset(NCust);						/* Hoist */

    return Q[m]->V(e,k) * X[n][k];
}


/*
 * Return throughtput at `station', entry `e', normalized for
 * all customers that visit the station.
 */

double
MVA::normalizedThroughput( const Server& station, const unsigned e,  const unsigned k ) const
{
    const unsigned m = station.closedIndex;
    const unsigned n = offset(NCust);						/* Hoist */


    double sum = 0.0;
    double totCust = 0;

    if ( std::isfinite( X[n][k] ) ) {
	sum += Q[m]->V(e,k) * X[n][k];
    }
    if ( Q[m]->V(e,k) ) {
	totCust += NCust[k];
    }

    if ( totCust ) {
	return sum / totCust;
    } else {
	return 0.0;
    }
}


double
MVA::throughput( const unsigned m, const unsigned k, const Population& N ) const
{
    const unsigned n = offset(N);						/* Hoist */
    if ( std::isfinite( X[n][k] ) ) {
	return X[n][k] * Q[m]->V(k);
    } else {
	return 0;
    }
}


/*
 * Return utilization for class `k'.
 */

double
MVA::utilization( const unsigned m, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const unsigned n = offset(N);
    const unsigned E = Q[m]->nEntries();

    double sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	sum += U[n][m][e][k];
    }

    return sum;
}



/*
 * Return utilization.
 */

double
MVA::utilization( const unsigned m, const Population& N ) const
{
    const unsigned n = offset(N);
    const unsigned E = Q[m]->nEntries();

    double sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 1; k <= K; ++k ) {
	    sum += U[n][m][e][k];
	}
    }

    return sum;
}



/*
 * Return utilization.
 */

double
MVA::utilization( const Server& station, const Population& N ) const
{
    const unsigned m = station.closedIndex;
    const unsigned n = offset(N);
    const unsigned E = station.nEntries();

    double sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 1; k <= K; ++k ) {
	    sum += U[n][m][e][k];
	}
    }

    return sum;
}



double
MVA::utilization( const Server& station, const unsigned k ) const
{
    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned n   = offset(NCust);						/* Hoist */

    double sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	sum += U[n][m][e][k];
    }

    return sum;
}

/*
 * Return the *fraction* of class `k' utilization with a customer
 * from class `j' removed.  Eqn Conway (11)
 */

double
MVA::utilization( const Server& station, const unsigned k, const Population& N, const unsigned j ) const
{
    assert( 0 < k && k <= K && 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);						/* Hoist */

    double num = 0.0;
    double den = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	num += U[Nej][m][e][k];
	for ( unsigned i = 1; i <= K; ++i ) {
	    den += U[Nej][m][e][i];
	}
    }

    return (den ? num / den : 0.0);
}



/*
 * Return the queue length for station at population N.
 */

double
MVA::queueLength( const unsigned m, const Population& N ) const
{
    const unsigned E   = Q[m]->nEntries();
    const unsigned n   = offset(N);							/* Hoist */

    double sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 1; k <= K; ++k ) {
	    sum += L[n][m][e][k];
	}
    }

    return sum;
}



/*
 * Return the queue length for station at population N for chain k.
 */

double
MVA::queueLength( const unsigned m, const unsigned k, const Population& N ) const
{
    const unsigned E   = Q[m]->nEntries();
    const unsigned n   = offset(N);							/* Hoist */

    double sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	sum += L[n][m][e][k];
    }

    return sum;
}



/*
 * Return the queue length for station at population N.
 */

double
MVA::queueLength( const Server& station, const Population& N ) const
{
    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned n   = offset(N);							/* Hoist */

    double sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 1; k <= K; ++k ) {
	    sum += L[n][m][e][k];
	}
    }

    return sum;
}



double
MVA::queueLength( const Server& station, const unsigned k ) const
{
    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned n   = offset(NCust);							/* Hoist */

    double sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	sum += L[n][m][e][k];
    }

    return sum;
}



/*
 * Return the queue length for class `k' with one customer from class `j' removed.
 * Note that we only want phase 1 utilization.  Phase 2 represents ``forked'' customers.
 */

double
MVA::queueOnly( const Server& station, const unsigned k, const Population& N, const unsigned j ) const
{
    assert( 0 < k && k <= K && 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    const unsigned m   = station.closedIndex;
    const unsigned E   = station.nEntries();
    const unsigned Nej = offset_e_j(N,j);						/* Hoist */

    double sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	const double service = station.S(e,k);
	if ( service == 0.0 ) continue;
	sum += std::max( L[Nej][m][e][k] - U[Nej][m][e][k], 0.);	/* Can't have negative length */
    }
    return sum;
}



/*
 * Calculate and return response time for class `k' at `station'.
 * Method used is to sum residence times for all but `station'.
 */

double
MVA::responseTime( const Server& station, const unsigned k ) const
{
    assert( 0 < k && k <= K );

    double sum = Z[k];				/* Think time.		*/

    for ( unsigned m = 1; m <= M; ++m ) {
	if ( Q[m] == &station ) continue; 	/* Don't count self.	*/

	sum += Q[m]->R(k);
    }
    return sum;
}



/*
 * Calculate and return response time for class `k'.  Response time is system response time.
 */

double
MVA::responseTime( const unsigned k ) const
{
    assert( 0 < k && k <= K );
    double sum = 0.0;

    for ( unsigned m = 1; m <= M; ++m ) {
	sum += Q[m]->R(k);
    }
    return sum;
}



/*
 * $\lambda$ term.
 */

Positive
MVA::arrivalRate( const Server& station, const unsigned e, const unsigned k, const Population &N ) const
{
    const unsigned n = offset(N);
    Positive retval;
    if ( N[k] == 0 || X[n][k] == 0 ) {
	double sum = Z[k];			/* Think time.		*/

	for ( unsigned m = 1; m <= M; ++m ) {
	    if ( Q[m] == &station ) continue; 	/* Don't count self.	*/

	    sum += Q[m]->S(k);
	}
	retval = station.V(e,k) / sum;

    } else {

	const unsigned m = station.closedIndex;
	const double temp = N[k] - L[n][m][e][k];
	if ( temp < 0 ) {
	    retval = 0.0;
	} else {
	    const Positive divisor = temp / X[n][k];
	    retval = N[k] * station.V(e,k) / divisor;
	}
    }
#if DEBUG_MVA
    std::cerr << "arrivalRate(" << station.closedIndex << "," << e << "," << k << ") = " << retval << std::endl;
#endif
    return retval;
}



/*
 * Return the queue length for chain `k' with one customer removed from chain j.
 */

double
MVA::syncDelta( const Server& station, const unsigned e, const unsigned k, const Population& N ) const
{
    if ( N[k] == 0 ) return 0.0;

    const unsigned n = offset(N);						/* Hoist */
    const unsigned m = station.closedIndex;

    return (L[n][m][e][k] / X[n][k] - station.S(e,k)) / (N[k] / X[n][k]);

}



/*
 * REP N-R
 * Adjustment factor for Newton Raphson phase.
 * (5.18) [Pan, pg 73].
 */

double
MVA::nrFactor( const Server& station, const unsigned e, const unsigned k ) const
{
    const unsigned m = station.closedIndex;
    const unsigned n = offset(NCust);							/* Hoist */

    assert( 0 < k && k <= K && e <= station.nEntries() );

    return std::isfinite( X[n][k] ) ? X[n][k] * L[n][m][e][k] / NCust[k] : 0.0;
}



/*
 * Common expression: tau and overlap corrections
 */

double
MVA::tau_overlap( const Server& station, const unsigned j, const unsigned k, const Population& N ) const
{
    double scaling = 1.0;

    if ( overlapFactor && k != j ) {
	scaling *= overlapFactor[k][j];
    }

    /* tau correction. */

    if ( station.hasTau() && k != j && __bounds_limit > 0 ) {
	scaling *= tau( station, j, k, N );
    }

    return scaling;
}


/*
 * Tau correction.
 */

double
MVA::tau( const Server& station, const unsigned j, const unsigned k, const Population& N ) const
{
    const unsigned n = offset(N);						/* Hoist */
    if ( N[j] == 0 || !std::isfinite( X[n][j] ) || !std::isfinite( X[n][k] ) ) return 1.0;

    const double lambda_mj = X[n][j] * station.V(j);				/* BUG 547 */
    const double lambda_mk = X[n][k] * station.V(k);

    if ( lambda_mj == 0.0 || lambda_mk == 0.0 ) return 1.0;

    const unsigned Nej = offset_e_j(N,j);					/* Hoist */
    const unsigned E   = station.nEntries();
    const unsigned m   = station.closedIndex;

    double Lk = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	Lk += L[Nej][m][e][k];
    }

    if ( overlapFactor && k != j ) {
	Lk *= overlapFactor[k][j];
    }

    const double tau = 1.0 / pow( (1.0 + pow( (Lk * lambda_mj) / (lambda_mk * N[j]), (double)__bounds_limit ) ),
				  1.0 / (double)__bounds_limit );
    return tau;
}



/*
 * Print throughput and utilization for all stations.
 */

std::ostream&
MVA::print( std::ostream& output ) const
{
    const int width = output.precision() + 2;
    unsigned m;

    printZ( output );
    printPri( output );
    printX( output );

    for ( m = 1; m <= M; ++m ) {
	unsigned count = 0;
	const unsigned E = Q[m]->nEntries();

	double L_sum = 0.;
	double W_sum = 0.;
	double U_sum = 0.;
	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned k = 1; k <= K; ++k ) {
		if ( Q[m]->V(e,k) == 0.0 ) continue;

		count += 1;
		if ( count == 1 ) {
		    output << m << ": ";
		} else {
		    output << "   ";
		}

		L_sum += L[offset(NCust)][m][e][k];
		U_sum += U[offset(NCust)][m][e][k];
		//TF NOTE: Need to support printing output
		output <<   "L_" << e << k << " = " << std::setw(width) << L[offset(NCust)][m][e][k];
		output << ", W_" << e << k << " = " << std::setw(width) << Q[m]->W[e][k][0];
//	output << ", R_" << e << k << " = " << setw(width) << Q[m]->R(e,k);
//	output << ", Y_" << e << k << " = " << setw(width) << throughput( e, k );
		output << ", U_" << e << k << " = " << std::setw(width) << U[offset(NCust)][m][e][k];
		output << std::endl;
	    }
	}
	if ( count > 1 ) {
	    output <<  "   SumL = " << L_sum
		   << ",   SumW = " << W_sum
		   << ",   SumU = " << U_sum << std::endl;
	}
	const unsigned n = offset(NCust);
	if ( Q[m]->vectorProbabilities() ) {
	    printVectorP( output, m, NCust );
	} else if ( P[n][m] ) {
	    const unsigned J = Q[m]->marginalProbabilitiesSize();
	    for ( unsigned j = 0; j <= J; ++j ) {
		if ( j == 0 ) {
		    output << m << ":";
		} else {
		    output << ",";
		}
		if ( j < J ) {
		    output << " P_" << j;
		} else {
		    output << " PB";
		}
		output << " = " << std::setw(width) << P[n][m][j];
	    }
	    output << std::endl;
	}
    }

    return output;
}



/*
 * Print think time.
 */

std::ostream&
MVA::printZ( std::ostream& output ) const
{
    int width = output.precision() + 2;

    for ( unsigned k = 1; k <= K; ++k ) {
	if ( k > 1 ) output << ", ";
	output << "Z_" << k << " = " << std::setw(width) << Z[k];
    }
    output << std::endl;
    return output;
}



/*
 * Print throughput
 */

std::ostream&
MVA::printX( std::ostream& output ) const
{
    int width = output.precision() + 2;
    const unsigned n = offset(NCust);						/* Hoist */

    for ( unsigned k = 1; k <= K; ++k ) {
	if ( k > 1 ) output << ", ";
	output << "X_" << k << NCust << " = " << std::setw(width) << X[n][k];
    }
    output << std::endl;
    return output;
}


/*
 * Print priorities.
 */

std::ostream&
MVA::printPri( std::ostream& output ) const
{
    int width = output.precision();

    for ( unsigned k = 1; k <= K; ++k ) {
	if ( k > 1 ) output << ", ";
	output << "Pri_" << k << " = " << std::setw(width) << priority[k];
    }
    output << std::endl;
    return output;
}



#if	DEBUG_MVA
/*
 * Jiffy printer of Queue Length L at NCust.
 */

std::ostream&
MVA::printL( std::ostream& output, const Population & N ) const
{
    const unsigned n = offset(N);
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned k = 1; k <= K; ++k ) {
		if ( k > 1 ) output << ", ";
		output << "L_{" << m << "," << e << "," << k << "}" << N << " = " << L[n][m][e][k];
	    }
	    output << std::endl;
	}
    }
    return output;
}



/*
 * Jiffy printer of Population X at NCust.
 */

std::ostream&
MVA::printW( std::ostream& output ) const
{
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned k = 1; k <= K; ++k ) {
		if ( k > 1 ) output << ", ";
		output << "W_{" << m << "," << e << "," << k << "} = " << Q[m]->W[e][k][0];
	    }
	    output << std::endl;
	}
    }
    return output;
}


/*
 * Jiffy printer of Utilization at NCust.
 */

std::ostream&
MVA::printU( std::ostream& output, const Population& N  ) const
{
    const unsigned n = offset(N);
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned k = 1; k <= K; ++k ) {
		if ( k > 1 ) output << ", ";
		output << "U_{" << m << "," << e << "," << k << "}" << N << " = " << U[n][m][e][k];
	    }
	    output << std::endl;
	}
    }
    return output;
}


/*
 * Jiffy printer of marginal Queue Probabilties.
 */

std::ostream&
MVA::printP( std::ostream& output, const Population & N ) const
{
    const unsigned n = offset(N);
    for ( unsigned m = 1; m <= M; ++m ) {
	if ( Q[m]->vectorProbabilities() ) {
	    Population I(K);			// Need to sequence over this.
	    Population::IteratorOffset next( NCust, N );
	    unsigned i = 0;

	    do {
		if ( i > 0 ) output << ", ";
		output << "P_{" << m << I << "}" << N << " = " << P[n][m][i];
		i = next( I );
	    } while ( i != 0 );
	    output << std::endl;

	} else if ( P[n][m] ) {
	    const unsigned J = Q[m]->marginalProbabilitiesSize();

	    for ( unsigned j = 0; j <= J; ++j ) {
		if ( j > 0 ) output << ", ";
		if ( j < J ) {
		    output << "P_{" << m << "," << j << "}";
		} else {
		    output << "PB_{" << m << "}";
		}
		output << N << " = " << P[n][m][j];
	    }
	    output << std::endl;
	}
    }
    return output;
}
#endif


/*
 *
 */

std::ostream&
MVA::printVectorP( std::ostream& output, const unsigned m, const Population& N ) const
{
    const int width = output.precision() + 2;
    Population I(K);			// Need to sequence over this.
    Population::IteratorOffset next( NCust, N );
    unsigned i = 0;

    const unsigned n = offset(N);							/* Hoist */
    unsigned j;
    const unsigned J = static_cast<unsigned>(Q[m]->mu());
    Probability * tempP = new Probability[J+1];

    for ( j = 0; j <= J; ++j ) {
	tempP[j] = 0.0;
    }

    do {
	j = I.sum();
	if ( j > J ) j = J;
	tempP[j] += P[n][m][i];
	i = next( I );
    } while ( i != 0 );

    for ( j = 0; j <= J; ++j ) {
	if ( j == 0 ) {
	    output << "   ";
	} else {
	    output << ", ";
	}
	output << "P_" << j << N << " = " << std::setw(width) << tempP[j];
    }
    output << std::endl;

    output << "   -- Full marginals -- " << std::endl;
    output << "   P_" << I << P[0][m];
    while ( (i = next( I )) ) {
	output << ", P_" << I << P[i][m];
    }
    output << std::endl;

    return output;
}

/* ---------------------------- Exact MVA. ---------------------------- */

const char * const ExactMVA::__typeName = "Exact MVA";

ExactMVA::ExactMVA( Vector<Server *>&q, const Population& N, const Vector<double>& z, const Vector<unsigned>& prio, const Vector<double>* of )
    : MVA( q, N, z, prio, of), map(N)
{
}


/*
 * Recursively solve for population vector N starting at [0,0,...,0]
 * to nCust.  The dimensionality of N is limited by stack size...
 */

bool
ExactMVA::solve()
{
    /* Allocate array space and initialize */
    reset();				/* Reset all vectors to zero. */
    dimension( map.dimension( NCust ).maxOffset() );
    clearCount();

    /* Let er rip! */

    for ( PopulationMap::iterator n = map.begin(); n != map.end(); ++n ) {
	step( *n );
    }
    return true;
}



/*
 * Compute marginal probabilities component.  Subclasses assign
 * P(0,N) and may revise PB(N).
 */

void
ExactMVA::marginalProbabilities( const unsigned m, const Population& N )
{
    const unsigned J = static_cast<unsigned>(Q[m]->mu());
    if ( J == 0 ) return;

    const unsigned n = offset(N);						/* Hoist */
    double sum_of_P = 0.0;

    for ( unsigned j = 1; j < J; ++j ) {

	double sum = 0.0;
	for ( unsigned k = 1; k <= K; ++k ) {					/* Eqn 2.4 */
	    if ( N[k] < 1 ) continue;
	    const unsigned Nek = offset_e_j(N,k);				/* Hoist */
	    sum += utilization(m,k,N) * P[Nek][m][j-1];
	}

	const double temp = sum / j;
	P[n][m][j] = temp;
	sum_of_P  += temp;
    }

    double sum = 0.0;								/* Eqn 2.5 */
    for ( unsigned k = 1; k <= K; ++k ) {
	if ( N[k] < 1 ) continue;
	const unsigned Nek = offset_e_j(N,k);					/* Hoist */
	sum += utilization(m,k,N) * ( P[Nek][m][J] + P[Nek][m][J-1] );
    }

    const double temp = sum / Q[m]->mu();
    P[n][m][J] = temp;				// PB_m(N)
    sum_of_P  += temp;

    /* KLUDGE -- adjust probabilties if necessary. */

    if ( sum_of_P > 1.0 ) {
	P[n][m][0] = 0.0;			// P_m(0,N)			   Eqn 2.6
	const double normalization = 1.0 / sum_of_P;
	for ( unsigned j = 1; j <= J; ++j ) {
	    P[n][m][j] *= normalization;		/* Renormalization */
	}
    } else {
	P[n][m][0] = 1.0 - sum_of_P;		// P_m(0,N)			   Eqn 2.6
    }
}



/*
 * Compute marginal probabilities of the type P(I,N) where I and N
 * are vectors.
 */

void
ExactMVA::marginalProbabilities2( const unsigned m, const Population& N )
{
    Population I(K);				// Need to sequence over this.
    Population::IteratorOffset next( NCust, N );
    unsigned i;
    const unsigned n = offset( N );

    double sum = 0.0;

    while ( (i = next( I )) ) {

	double sum1 = 0.0;
	for ( unsigned k = 1; k <= K; ++k ) {
	    if ( N[k] < 1 || I[k] < 1 ) continue;
	    const unsigned Nek = offset_e_j(N,k);
	    const unsigned Iek = next.offset_e_j(I,k);

	    sum1 += utilization(m,k,N) * P[Nek][m][Iek];
	}
	const double temp = sum1 / Q[m]->mu(I.sum());

	P[n][m][i] = temp;
	sum       += temp;
    }

    /* Find P(0,N) */

    if ( sum > 1.0 ) {
	const unsigned maxI = next.maxOffset();
	P[n][m][0] = 0.0;
	for ( i = 1; i <= maxI; ++i ) {
	    P[n][m][i] /= sum;		/* Renormalization */
	}
    } else {
	P[n][m][0] = 1.0 - sum;			// P_m(0,N)			   Eqn 2.6
    }
}


/*
 * Inflation factor for priority MVA.
 */

Probability
ExactMVA::priorityInflation( const Server& station, const Population &N, const unsigned j ) const
{
    const unsigned m = station.closedIndex;
    const unsigned E = station.nEntries();
    const unsigned n = offset(N);							/* Hoist */
    Probability util = 0.0;

    for ( unsigned k = 1; k <= K; ++k ) {
	if ( priority[k] > priority[j] ) {
	    for ( unsigned e = 1; e <= E; ++e ) {
		if ( N[k] == 0 ) continue;
		const double L_mk = L[n][m][e][k];
		const Probability delta = fmod(L_mk,1.0);
		Population N_hi(N);
		Population N_lo(N);
		N_lo[k] -= (unsigned)floor(L_mk);
		N_hi[k] -= (unsigned)ceil(L_mk);
		util += (1.0 - delta) * U[offset(N_lo)][m][e][k] + delta * U[offset(N_hi)][m][e][k];
	    }
	}
    }
    return util;
}

/* ------------------------- Bard Schweitzer. ------------------------- */

SchweitzerCommon::SchweitzerCommon( Vector<Server *>&q, const Population & N, const Vector<double>& z, const Vector<unsigned>& prio, const Vector<double>* of )
    : MVA( q, N, z, prio, of), termination_test(1.0 / ( 4000 + 16 * N.sum() )), initialized(false), last_L(0)
{
    last_L = new double ** [M+1];
    last_L[0] = 0;

    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	last_L[m] = new double * [E+1];
	last_L[m][0] = 0;

	for ( unsigned e = 1; e <= E; ++e ) {
	    last_L[m][e] = new double [K+1];
	    for ( unsigned k = 0; k <= K; k++ ) {
		last_L[m][e][k] = 0.0;	//Initialize with zero content
	    }
	}
    }
}

/*
 * Free storage.
 */

SchweitzerCommon::~SchweitzerCommon()
{
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    delete [] last_L[m][e];
	}
	delete [] last_L[m];
    }
    delete [] last_L;
}


void
SchweitzerCommon::reset()
{
    MVA::reset();

    initialized = false;
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned k = 0; k <= K; k++ ) {
		last_L[m][e][k] = 0.0;	//Initialize with zero content
	    }
	}
    }
}




/*
 * Hairy initialization for marginal probabilties.
 */

void
SchweitzerCommon::initialize()
{
    MVA::initialize();
    
    unsigned m;
    const unsigned n = offset(NCust);						/* Hoist */
    unsigned k;

    /* Precompute */

    double * Dm = new double [K+1];
    double * Lk = new double [M+1];
    for ( k = 1; k <= K; ++k ) {
	Dm[k] = 0.0;
	for ( m = 1; m <= M; ++m ) {
	    const unsigned E = Q[m]->nEntries();
	    for ( unsigned e = 1; e <= E; ++e ) {
		if ( Q[m]->V(e,k) == 0.0 || !std::isfinite( Dm[k] ) ) continue;	/* inf */
		Dm[k] += Q[m]->S(e,k) * Q[m]->V(e,k);
	    }
	}
    }

    /*
     * Set initial queue length.  This scheme is a touch different from
     * the original Chandy version.  (See Conway)
     */

    for ( m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	Lk[m] = 0.0;

	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( k = 1; k <= K; ++k ) {
		if ( Dm[k] > 0.0 ) {
		    if ( !std::isfinite( Q[m]->S(e,k) ) ) {				/* inf */
			L[n][m][e][k] = Q[m]->S(e,k);
			Lk[m] = L[n][m][e][k];
		    } else {
			L[n][m][e][k] = Q[m]->S(e,k) * Q[m]->V(e,k) * NCust[k] / Dm[k];	/* Conway init	*/
			Lk[m] += L[n][m][e][k];
		    }
		} else {
		    L[n][m][e][k] = NCust[k] / (M * E);				/* Chandy init.	*/
		    Lk[m] += L[n][m][e][k];
		}
	    }
	}
    }

    /*
     * Estimate throughputs everywhere.  We do have queue lengths.
     */

    for ( k = 1; k <= K; ++k ) {
	X[n][k] = 0.0;

	if ( NCust[k] < 1 ) continue;

	double sum = 0.0;
	for ( m = 1; m <= M; ++m ) {
	    const double J = Q[m]->mu();
	    const unsigned E = Q[m]->nEntries();

	    if ( !std::isfinite( Lk[m] ) ) {						/* inf */
		sum = Lk[m];
		continue;
	    }

	    for ( unsigned e = 1; e <= E; ++e ) {
		double temp = 1.0;
		if ( J != 0 ) {
		    temp += ( Lk[m] - L[n][m][e][k] / NCust[k] ) / J;
		} else if ( Q[m]->infiniteServer() == 0 ) {
		    temp += ( Lk[m] - L[n][m][e][k] / NCust[k] );
		}
		temp *= Q[m]->S(e,k) * Q[m]->V(e,k);

		sum += temp;
	    }
	}

	if ( sum <= 0.0 ) {
	    X[n][k] = std::numeric_limits<double>::infinity();
	} else if ( !std::isfinite( sum ) ) {
	    X[n][k] = 0.;
	} else {
	    X[n][k] = NCust[k] / sum;
	}
    }

    /*
     * Now set utilizations and marginal probabilities.
     */

    for ( m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	/*
	 * Set initial values of utilization and waiting.
	 */

	for ( k = 1; k <= K; ++k ) {
	    for ( unsigned e = 1; e <= E; ++e ) {
		if ( Q[m]->V(e,k) == 0.0 || Q[m]->S(e,k) == 0.0 || X[n][k] == 0.0 || !std::isfinite(X[n][k]) ) {
		    U[n][m][e][k] = 0.0;
		    Q[m]->W[e][k][0] = 0.0;
		} else {
		    U[n][m][e][k] = Q[m]->V(e,k) * Q[m]->S(e,k) * X[n][k];
		    Q[m]->W[e][k][0] = (L[n][m][e][k] * Q[m]->S(e,k)) / U[n][m][e][k];
		}
	    }
	}

	/*
	 * Set marginal probabilities for those stations that
	 * are in need of them.
	 */

#if 0
	if ( Q[m]->vectorProbabilities() ) {

	    marginalProbabilities2( m, N );

	} else
#endif
	    if ( P[n][m] ) {

		const double pop = NCust.sum();
		const unsigned J = Q[m]->marginalProbabilitiesSize();
//		const Probability temp = pop > 0.0 ? 2.0 * pop / (J * pop * (pop + 1.0)) : 0.0;
		const Probability temp = pop > 0.0 ? 2.0 / (J * (pop + 1.0)) : 0.0;
		Probability sum  = 0.0;

		for ( unsigned j = 1; j < J; ++j ) {
		    P[n][m][j] = temp;
		    sum       += temp;
		}
		const double PmjN = std::min( 1.0 - sum, pop >= J ? temp * (pop + 1.0 - J) : 0.0 );
		P[n][m][J] = PmjN;
		P[n][m][0] = 1.0 - (sum + PmjN);
	    }
    }

    delete [] Dm;
    delete [] Lk;
}



/*
 * Core solver for Bard Schweitzer and Linearizer.
 */

void
SchweitzerCommon::core( const Population& N, const unsigned n )
{
    unsigned i = 0;

#if DEBUG_MVA
    if ( debug_L || debug_P ) std::cout << "Initially..." << std::endl;
    if ( debug_L ) printL( std::cout, N );
    if ( debug_P ) printP( std::cout, N );
#endif

    do {
	copy_L( n );

	estimate_L( N );
	estimate_P( N );
	step( N );

	/* Emergency stop... */

	i += 1;
#if DEBUG_MVA
	if ( i > 45 && debug_L ) {
	    std::cout << "After " << iterations() << " Iterations..." << std::endl;
	    if ( debug_L ) printL( std::cout, N );
	}
#endif
	if ( i > 80 ) {
	    throw MVA::iteration_limit( "Schweitzer::core" );
	} else if ( i > 50 ) {
	    /* Underrelax L increasing each iteration. */
	    for ( unsigned m = 1; m <= M; ++m ) {
		const unsigned E = Q[m]->nEntries();
		for ( unsigned e = 1; e <= E; ++e ) {
		    for ( unsigned k = 1; k <= K; ++k ) {
			if ( N[k] == 0 ) continue;

			L[n][m][e][k]  = ( static_cast<double>(100 - i) * L[n][m][e][k] + static_cast<double>(i) * last_L[m][e][k] ) / 100.0;
		    }
		}
	    }
	}

    } while ( max_delta_L( n, N ) >= termination_test );

#if DEBUG_MVA
    if ( debug_L ) {
	std::cout << "After " << iterations() << " Iterations..." << std::endl;
	printL( std::cout, N );
    }
#endif
}



/*
 * Find Queue length based on fraction `F' of class `k' jobs at station for
 * population `N'.  Initialize the utilization array $U \forall m,e,k$ for population $N -
 * e_j$ for $1 \leq j \leq K$.
 */

void
SchweitzerCommon::estimate_L( const Population & N )
{
    const unsigned n = offset(N);
    for ( unsigned m = 1; m <= M; ++m ) {
	estimate_Lm( m, N, n );
    }
}



/*
 * Core step of estimate_L.
 */

void
SchweitzerCommon::estimate_Lm( const unsigned m, const Population & N, const unsigned n ) const
{
    const unsigned E = Q[m]->nEntries();

    for ( unsigned j = 1; j <= K; ++j ) {
	if ( N[j] < 1 ) continue;				/* Eq:11 */
	const unsigned Nej = offset_e_j(N, j);

	for ( unsigned k = 1; k <= K; ++k ) {
	    if ( N[k] < 1 ) continue;
	    const double N_k = static_cast<double>(N[k]);

	    for ( unsigned e = 1; e <= E; ++e ) {
		const double L_n_m_e_k = L[n][m][e][k];
		if ( !std::isfinite( L_n_m_e_k ) ) continue;

		const double F  = L_n_m_e_k / N_k;		/* Eq:9 */
		const double L_ej = std::max( (N_k - static_cast<double>( k == j )) * (F + D_mekj(m,e,k,j)), 0. );

		L[Nej][m][e][k] = L_ej;

		/* Now estimate utilizations based on queue lengths. */

		if ( L_n_m_e_k > 0.0 ) {
		    U[Nej][m][e][k] = L_ej / L_n_m_e_k * U[n][m][e][k];
		} else {
		    U[Nej][m][e][k] = 0.0;
		}
	    }
	}
    }
}



/*
 * Compute marginal probabilities component.  This version is based on:
 *
 *   author =       "Krzesinski, A. and Greyling, J.",
 *   title =        "Improved Linearizer Methods for Queueing Networks
 *                   with Queue Dependent Service Centers",
 *   booktitle =    sigmetrics-84,
 *   year =         1984,
 *   isbn =         "0163-5999",
 *   callno =       "QA76.9.E94P47",
 *
 */

void
SchweitzerCommon::marginalProbabilities( const unsigned m, const Population& N )
{
    const unsigned n  = offset(N);					/* Hoist */

    if ( P[n][m] == 0 ) return;

    const unsigned J  = Q[m]->marginalProbabilitiesSize();
    const unsigned JJ = std::min( J, N.sum() );	/* Note: loops end at the minimum of servers, customers. */
    const double U_m  = std::min( static_cast<double>(J), utilization( m, N ) );

    if ( U_m < Q[m]->mu() ) {
	double sum        = 1.0;					/* Eqn 5.5 */
	double prod       = U_m / Q[m]->mu(1);

	/* Find P(1,N) thru P(J-1,N) */

	for ( unsigned int j = 1; j < JJ; ++j ) {
	    P[n][m][j] = prod;						/* Eqn 5.3 */
	    sum  += P[n][m][j];
	    prod *= U_m / Q[m]->mu(j+1);
	}
	for ( unsigned int j = JJ + 1; j <= J; ++j ) {
	    P[n][m][j] = 0.0;
	}

	/* Find P(J,N) */

	P[n][m][JJ] = prod *  U_m / ( J - U_m );			/* Eqn 5.4 */
	sum        += P[n][m][JJ];

	/* Find P(0,N) */

	const Probability P0 = 1.0 / sum;				/* Eqn 5.5 */

	P[n][m][0] = P0;
	if ( P0 != 0.0 ) {
	    for ( unsigned int j = 1; j <= JJ; ++j ) {
		P[n][m][j] *= P0;
	    }
	}
    } else {
	for ( unsigned int j = 1; j < JJ; ++j ) {
	    P[n][m][j] = 0.0;
	}
	P[n][m][JJ] = 1.0;
    }
}



/*
 * Basically, substitute P(j,N-ek) with P(j,N).  Also, with this
 * technique, the marginal probabilites will always sum to one.
 *
 * ----------
 *   author =       "Schmidt, Rainer",
 *   title =        "An Approximate {MVA} Algorithm for Exponential,
 *                   Class-Dependent Multiple Servers",
 *   journal =      peva,
 *   year =         1997,
 *   volume =       29,
 *   pages =        "245--254",
 *
 * Binomial approximation for marginal rates.
 */

void
SchweitzerCommon::marginalProbabilities2( const unsigned m, const Population& N )
{
    Population I(N);				// Need to sequence over this.
    Population::IteratorOffset next( NCust, N );
    unsigned    i = 0;
    unsigned    n = offset( N );

#if 1
    do {
	double prod = 1.0;
	for ( unsigned k = 1; k <= K; ++k ) {
	    const Probability L_frac = queueLength( m, k, I ) / (double)NCust[k];

	    prod *= choose( NCust[k], I[k] )
		* power( L_frac, I[k] )
		* power( 1.0 - L_frac, NCust[k] - I[k] );	/* Eqn 18 */
	}

	P[n][m][i] = prod;

	i = next( I );

    } while ( i > 0 );
#else
    double sum = 1.0;

    /* Find P(1,N) thru P(J-1,N) */

    while ( (i = next(I)) ) {

	double pow  = power( utilization( m, N ), I.sum() );
	double prod = 1.0;
	for ( unsigned j = 1; j <= I.sum(); ++j ) {
	    prod *= Q[m]->mu(j);
	}

	P[n][m][i] = pow / prod;
	sum       += P[n][m][i];
    }

    Probability P0 = 1.0 / sum;
    P[n][m][0] = P0;
    unsigned maxI = next.maxOffset();
    for ( i = 1; i <= maxI; ++i ) {
	P[n][m][i] *= P0;
    }
#endif
}



void
SchweitzerCommon::copy_L( const unsigned n ) const
{
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned k = 1; k <= K; ++k ) {
		last_L[m][e][k] = L[n][m][e][k];
	    }
	}
    }
}



double
SchweitzerCommon::max_delta_L( const unsigned n, const Population &N ) const
{
    /* Iteration termination test. */

    double max_delta = 0.0;
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned k = 1; k <= K; ++k ) {
		if ( N[k] == 0 ) continue;

		double temp = fabs( L[n][m][e][k] - last_L[m][e][k] ) / N[k];
		if ( temp > max_delta ) max_delta = temp;
	    }
	}
    }
    return max_delta;
}



/*
 * Inflation factor for priority MVA.
 */

Probability
SchweitzerCommon::priorityInflation( const Server& station, const Population &N, const unsigned j ) const
{
    const unsigned m = station.closedIndex;
    const unsigned E = station.nEntries();
    const unsigned n = offset(N);						/* Hoist */
    Probability util = 0.0;

    for ( unsigned k = 1; k <= K; ++k ) {
	if ( priority[k] > priority[j] ) {
	    const unsigned Nek = offset_e_j(N,k);				/* Hoist */
	    for ( unsigned e = 1; e <= E; ++e ) {
		const double temp = U[n][m][e][k] - L[n][m][e][k]
		    * (U[n][m][e][k] - U[Nek][m][e][k]);	// **Table 5**
		if ( temp > 0 ) {
		    util += temp;	// Might be negative on initialization.
		}
	    }
	}
    }
    return util;
}

/* ------------------------- Bard Schweitzer. ------------------------- */

const char * const Schweitzer::__typeName = "Bard-Schweitzer";

/*
 * Allocate storage and distribute customers to queues.  All populations
 * are of type SpecialPop since we don't need to go through the entire
 * population space.
 */

Schweitzer::Schweitzer( Vector<Server *>&q, const Population & N, const Vector<double>& z, const Vector<unsigned>& prio, const Vector<double>* of )
    : SchweitzerCommon( q, N, z, prio, of), map(N)
{
    /* Allocate array space and initialize */

    dimension( map.dimension( NCust ).maxOffset() );		/* Set up L, U, X and P */
}

Schweitzer::~Schweitzer()
{
}


/*
 * Find Marginal Probabilities based on fraction `F' of class `k' jobs at
 * station for population `N'.  This routine corresponds to the
 * "estimate" function in Conway.
 */

void
Schweitzer::estimate_P( const Population & N )
{
    const unsigned n = offset_e_j(N,0);				/* Hoist */

    for ( unsigned m = 1; m <= M; ++m ) {
	if ( P[n][m] == 0 ) continue;

	const unsigned J = Q[m]->marginalProbabilitiesSize();
	for ( unsigned k = 1; k <= K; ++k ) {
	    if ( N[k] < 1 ) continue;
	    const unsigned Nek = offset_e_j(N,k);		/* Hoist */

	    for ( unsigned j = 0; j <= J; ++j ) {
		P[Nek][m][j] = P[n][m][j];
	    }
	}
    }
}



/*
 * Solve the model.  This member function is very very complicated.
 */

bool
Schweitzer::solve()
{
    map.dimension( NCust );				/* Reset ALL associated arrays */
    clearCount();

    bool reset = !initialized;
    reset = dimension( P, getMap().maxOffset() ) || reset;
    if ( reset ) {
	setMaxP();
	initialize();
	initialized = true;
    }

    try {
	core( NCust, offset_e_j( NCust, 0 ) );		/* May not need to do this... */
    }
    catch ( const MVA::iteration_limit& error ) {
	faultCount += 1;
	return false;
    }
    return true;
}

/* -------------------------- One Step MVA. --------------------------- */

const char * const OneStepMVA::__typeName = "One-Step";

/*
 * Constructor...
 */

OneStepMVA::OneStepMVA( Vector<Server *>&q, const Population & n, const Vector<double>& z, const Vector<unsigned>& prio, const Vector<double>* of )
    : Schweitzer( q, n, z, prio, of)
{
}


/*
 * Perform ONE STEP of the MVA algorithm (Schweitzer approximaation)
 * and one step only.
 */

bool
OneStepMVA::solve()
{
    map.dimension( NCust );				/* Reset ALL associated arrays */
    clearCount();

    bool reset = !initialized;
    reset = dimension( P, getMap().maxOffset() ) || reset;

    if ( reset ) {
	setMaxP();
	initialize();
	initialized = true;
    }

    estimate_L( NCust );
    estimate_P( NCust );
    step( NCust );
    return true;
}

/* --------------------------- Linearizer. ---------------------------- */

const char * const Linearizer::__typeName = "Linearizer";

/*
 * Constructor...
 * Allocate storage for D and F variables.  We also need to save L.
 */

Linearizer::Linearizer( Vector<Server *>&q, const Population & N, const Vector<double>& z, const Vector<unsigned>& prio, const Vector<double>* of )
    : SchweitzerCommon( q, N, z, prio, of), c(0), map(N)
{
    dimension( map.dimension( NCust ).maxOffset() );		/* Set up L, U, X and P */

    const size_t max_offset = getMap().maxOffset();
    saved_L.resize(max_offset);
    saved_U.resize(max_offset);
    saved_P.resize(max_offset);

    for ( unsigned n = 0; n < max_offset; ++n ) {
	saved_L[n] = new double ** [M+1];
	saved_U[n] = new double ** [M+1];
	saved_P[n] = new double *[M+1];

	for ( unsigned m = 1; m <= M; ++m ) {
	    const unsigned E = Q[m]->nEntries();

	    saved_L[n][m] = new double * [E+1];
	    saved_U[n][m] = new double * [E+1];
	    saved_P[n][m] = 0;

	    saved_L[n][m][0] = 0;
	    saved_U[n][m][0] = 0;

	    for ( unsigned e = 1; e <= E; ++e ) {
		saved_L[n][m][e] = new double [K+1];
		saved_U[n][m][e] = new double [K+1];
		//No initialization needed, we only use what we save
	    }
	}
    }

    dimension( saved_P, max_offset );		/* Allocate space for maringals */
//    setMaxP();				/* Done in SchweitzerCommon     */

    D    = new double *** [M+1];
    D[0] = 0;

    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	D[m]    = new double  ** [E+1];
	D[m][0] = 0;

	for ( unsigned e = 1; e <= E; ++e ) {
	    D[m][e]    = new double   * [K+1];
	    D[m][e][0] = 0;
	    for ( unsigned j = 1; j <= K; ++j ) {
		D[m][e][j] = new double[K+1];

		for ( unsigned l = 1; l <= K; ++l ) {
		    D[m][e][j][l] = 0.0;
		}
	    }
	}
    }
  
}



/*
 * Free storage.
 */

Linearizer::~Linearizer()
{
    const size_t max_offset = getMap().maxOffset();
    for ( unsigned n = 0; n < max_offset; n++) {
	for ( unsigned m = 1; m <= M; ++m ) {
	    const unsigned E = Q[m]->nEntries();
	    for ( unsigned e = 1; e <= E; ++e ) {
		delete [] saved_L[n][m][e];
		delete [] saved_U[n][m][e];
	    }
	    delete [] saved_L[n][m];
	    delete [] saved_U[n][m];

	    if ( saved_P[n][m] ) {
		delete [] saved_P[n][m];
	    }
	}
	delete [] saved_L[n];
	delete [] saved_U[n];
	delete [] saved_P[n];
    }

    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned j = 1; j <= K; ++j ) {
		delete [] D[m][e][j];
	    }
	    delete [] D[m][e];
	}
	delete [] D[m];
    }
    delete [] D;
}


void
Linearizer::reset() 
{
    SchweitzerCommon::reset();

    const size_t max_offset = getMap().maxOffset();
    for ( unsigned n = 0; n < max_offset; ++n ) {
	for ( unsigned m = 1; m <= M; ++m ) {
	    saved_P[n][m] = 0;
	    saved_L[n][m][0] = 0;
	    saved_U[n][m][0] = 0;
	}
    }

    D[0] = nullptr;

    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	D[m][0] = nullptr;

	for ( unsigned e = 1; e <= E; ++e ) {
	    D[m][e][0] = nullptr;
	    for ( unsigned j = 1; j <= K; ++j ) {
		for ( unsigned l = 1; l <= K; ++l ) {
		    D[m][e][j][l] = 0.0;
		}
	    }
	}
    }

    initialized = false;
}



/*
 * Solver for linearizer.  See reference for description.
 */

bool
Linearizer::solve()
{
    Population N;			/* Population vector.		*/

    map.dimension( NCust );		/* Reset ALL associated arrays */
    clearCount();

    /* Initialize */

    c = 0;
    initialize();
    
    estimate_L( NCust );
    estimate_P( NCust );
    initialized = true;

    for ( unsigned I = 1; I <= 2 ; ++I ) {

	/*
	 * NB: `c' is an instance variable used by our Lm function.
	 */
	for ( c = 0; c <= K; ++c ) {
	    if ( c > 0 && NCust[c] == 0 ) continue;	/* NOP  BUG 345 */
	    N = NCust;

	    if ( c > 0 ) N[c] -= 1;

	    save_L();
	    try {
		core( N, offset_e_c_e_j(c, 0) );	/* Hoist */
	    }
	    catch ( const MVA::iteration_limit& error ) {
		/* Ignore iteration problems in lower level models */
	    }
	    restore_L();

	}

	c = 0;
	update_Delta( NCust );
    }

    c = 0;
    try {
	core( NCust, offset_e_c_e_j(c, 0) );
    }
    catch ( const MVA::iteration_limit& error ) {
	faultCount += 1;
	return false;
    }
    return true;
}



void
Linearizer::initialize()
{
    /* BUG 628 -- Recompute iff marginals change. */

    bool reset = !initialized;
    const size_t max_offset = getMap().maxOffset();
    reset = dimension( P, max_offset ) || reset;		/* Don't short circuit this!!! */
    reset = dimension( saved_P, max_offset ) || reset;
    if ( reset ) {
	setMaxP();
	SchweitzerCommon::initialize();
	estimate_L( NCust );
	estimate_P( NCust );
	initialized = true;
    }
}


/*
 * Save L matrix.
 */
void
Linearizer::save_L()
{
    const unsigned nClasses = NCust.size();

    for ( unsigned j = 0; j <= nClasses; ++j) {
	const unsigned n = offset_e_c_e_j( c, j );

	for ( unsigned m = 1; m <= M; ++m ) {
	    const unsigned E = Q[m]->nEntries();

	    for ( unsigned e = 1; e <= E; ++e ) {
		for ( unsigned k = 1; k <= K; ++k ) {
		    saved_L[n][m][e][k] = L[n][m][e][k];
		    saved_U[n][m][e][k] = U[n][m][e][k];
		}
	    }

	    const unsigned J = Q[m]->marginalProbabilitiesSize();
	    if ( J != 0 ) {
		for ( unsigned i = 0; i <= J; ++i ) {
		    saved_P[n][m][i] = P[n][m][i];
		}
	    }
	}
    }
}



/*
 * Restore L matrix - but don't clobber solution at population of N.
 */
void
Linearizer::restore_L()
{
    unsigned nClasses = NCust.size();

    for ( unsigned j = 1; j <= nClasses; ++j ) {
	//@ Only restore for N-j entries (see Population::restore)
	const unsigned n = offset_e_c_e_j( c, j );

	for ( unsigned m = 1; m <= M; ++m ) {
	    const unsigned E = Q[m]->nEntries();

	    for ( unsigned e = 1; e <= E; ++e ) {
		for ( unsigned k = 1; k <= K; ++k ) {
		    L[n][m][e][k] = saved_L[n][m][e][k];
		    U[n][m][e][k] = saved_U[n][m][e][k];
		}
	    }

	    const unsigned J = Q[m]->marginalProbabilitiesSize();
	    if ( J != 0 ) {
		for ( unsigned i = 0; i <= J; ++i ) {
		    P[n][m][i] = saved_P[n][m][i];
		}
	    }
	}
    }
}


/*
 * Find Marginal Probabilities based on fraction `F' of class `k' jobs at
 * station for population `N'.  This routine corresponds to the
 * "estimate" function in Conway.
 */

void
Linearizer::estimate_P( const Population & N )
{
    const unsigned n = offset_e_c_e_j(c,0);				/* Hoist */

    for ( unsigned m = 1; m <= M; ++m ) {
	if ( P[n][m] == 0 ) continue;

	const unsigned J = Q[m]->marginalProbabilitiesSize();
	for ( unsigned k = 1; k <= K; ++k ) {
	    if ( N[k] < 1 ) continue;
	    const unsigned Nek = offset_e_c_e_j(c,k);		/* Hoist */

	    for ( unsigned j = 0; j <= J; ++j ) {
		P[Nek][m][j] = P[n][m][j];
	    }
	}
    }
}



/*
 * Update fraction of jobs at station.
 */

void
Linearizer::update_Delta( const Population & N )
{
    const unsigned n = offset_e_c_e_j(0, 0);			/* Hoist */

    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	for ( unsigned j = 1; j <= K; ++j ) {
	    if ( N[j] < 1 ) continue;
	    const unsigned Nej = offset_e_c_e_j(0, j);		/* Hoist */

	    for ( unsigned e = 1; e <= E; ++e ) {
		for ( unsigned k = 1; k <= K; ++k ) {
		    if ( N[k] > ( k == j ) ) {
			D[m][e][k][j] = ( L[Nej][m][e][k] / (N[k] - ( k == j )) )
			    -  ( L[n][m][e][k] / N[k] );
		    } else {
			D[m][e][k][j] = 0.0;
		    }
		}
	    }
	}
    }

#if DEBUG_MVA
    if ( debug_D ) printD( std::cout, N );
#endif
}



/*
 * Jiffy printer of D at N.
 */

std::ostream&
Linearizer::printD( std::ostream& output, const Population & N ) const
{
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    for ( unsigned k = 1; k <= K; ++k ) {
		for ( unsigned j = 1; j <= K; ++j ) {
		    output << "D_{" << m << e << k << j << "}" << N << " = " << D[m][e][k][j] << "\t";
		}
	    }
	    output << std::endl;
	}
    }
    return output;
}

/* ----------------------- One Step Linearizer. ----------------------- */

const char * const OneStepLinearizer::__typeName = "One-Step Linearizer";

/*
 * Constructor...
 */

OneStepLinearizer::OneStepLinearizer( Vector<Server *>&q, const Population & n, const Vector<double>& z, const Vector<unsigned>& prio, const Vector<double>* of )
    : Linearizer( q, n, z, prio, of)
{
}


/*
 * Perform ONE STEP of the MVA algorithm (linearizer approximaation)
 * and one step only.
 */

bool
OneStepLinearizer::solve()
{
    Population N;			/* Population vector.		*/

    /* Initialize */

    map.dimension( NCust );		/* Reset ALL associated arrays 	*/
    clearCount();

    c = 0;

    initialize();

    /*
     * NB: `c' is an instance variable used by our Lm function.
     */

    for ( c = 0; c <= K; ++c ) {
	N = NCust;

	if ( c > 0 ) N[c] -= 1;

	save_L();
	estimate_L( N );
	estimate_P( N );
	step( N );

	restore_L();

    }

    c = 0;
    update_Delta( NCust );

    estimate_L( NCust );
    estimate_P( NCust );
    step( NCust );
    return true;
}

/* ------------------------ Linearizer variant. ----------------------- */

/*
 * Solve using fancy algorithm.  $_$ comments denote equation numbers
 * from:
 *     author =   "{de Souza e Silva}, E. and Muntz Richard R.}",
 *     title =    "A Note on the Computational Cost of the Linearizer Algorithm",
 *     journal =  ieeetc,
 *     year =     1990,
 *     volume =   39,
 *     number =   6,
 *     pages =    "840--842",
 *     month =    jun,
 *
 * Warning: This algorithm has FP stability problems.
 */

const char * const Linearizer2::__typeName = "Fast Linearizer";


/*
 * Constructor...
 * We save computation time at the cost of more storage.
 */

Linearizer2::Linearizer2( Vector<Server *>&q, const Population & N, const Vector<double>& z, const Vector<unsigned>& prio, const Vector<double>* of )
    : Linearizer( q, N, z, prio, of)
{
    Lm.resize(getMap().maxOffset());

    for ( unsigned n = 0; n < getMap().maxOffset(); ++n ) {
	Lm[n] = new double [M+1];
    }

    D_k = new double ** [M+1];
    D_k[0] = nullptr;
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	D_k[m] = new double * [E+1];

	D_k[m][0] = nullptr;
	for ( unsigned e = 1; e <= E; ++e ) {
	    D_k[m][e] = new double [K+1];

	    for ( unsigned j = 1; j <= K; ++j ) {
		D_k[m][e][j] = 0.0;
	    }
	}
    }
}



/*
 * Destructor...
 */

Linearizer2::~Linearizer2()
{
    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();
	for ( unsigned e = 1; e <= E; ++e ) {
	    delete [] D_k[m][e];
	}
	delete [] D_k[m];
    }
    delete [] D_k;

    for ( unsigned n = 0; n < getMap().maxOffset(); ++n ) {
	delete [] Lm[n];
    }
}



/*
 * Initialize D matrix.
 */

void
Linearizer2::update_Delta( const Population & N )
{
    const unsigned n = offset_e_c_e_j(0, 0);			/* Hoist */

    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	for ( unsigned j = 1; j <= K; ++j ) {
	    const unsigned Nej = offset_e_c_e_j(0, j);		/* Hoist */
	    for ( unsigned e = 1; e <= E; ++e ) {

		D_k[m][e][j] = 0.0;					/* $$$ */
		for ( unsigned k = 1; k <= K; ++k ) {

		    double temp;
		    if ( N[k] > ( k == j ) ) {
			temp = ( L[Nej][m][e][k] / (N[k] - ( k == j )) )
			    -  ( L[n][m][e][k] / N[k] );
		    } else {
			temp = 0.0;
		    }
		    D[m][e][k][j] = temp;
		    D_k[m][e][j] += Q[m]->S(e,k) * N[k] * temp;		/* $$$ */
		}
	    }
	}
    }

#if DEBUG_MVA
    if ( debug_D ) printD( std::cout, N );
#endif
}



/*
 * Pre-compute Lm and save.  S_L_m must be pre-computed because it relies
 * on the old value of L for all populations of N.  Therefore, we
 * can't compute it on-the-fly in Lm().
 */

void
Linearizer2::estimate_L( const Population & N )
{
    const unsigned n = offset_e_c_e_j(c, 0);			/* Hoist */

    for ( unsigned m = 1; m <= M; ++m ) {
	const unsigned E = Q[m]->nEntries();

	for ( unsigned j = 1; j <= K; ++j ) {
	    if ( N[j] < 1 ) continue;
	    const unsigned Nej = offset_e_c_e_j(c, j);		/* Hoist */
	    Lm[Nej][m] = 0.0;
	}

	estimate_Lm( m, N, n );				// Needed for U and termination tests...

	for ( unsigned e = 1; e <= E; ++e ) {

	    double sum = 0.0;
	    for ( unsigned k = 1; k <= K; ++k ) {
		if ( N[k] < 1 ) continue;
		sum += Q[m]->S(e,k) * L[n][m][e][k];
	    }

	    for ( unsigned j = 1; j <= K; ++j ) {

		if ( N[j] < 1 ) continue;				/* $8$ */

		double temp = Q[m]->S(e,j) * ((L[n][m][e][j] / N[j]) + D[m][e][j][j]);

		if ( c != 0 ) {
		    temp += Q[m]->S(e,c) * D[m][e][c][j];		/* $9$ */
		}


		/*
		 * Due to FP precision problems (subtraction is always
		 * a pain), there is a possibility that Lm will be
		 * negative.  Fix it.
		 */

		const unsigned Nej = offset_e_c_e_j(c,j);
		Lm[Nej][m] += std::max( 0.0, sum + D_k[m][e][j] - temp );
	    }
	}
    }
}



/*
 * This version has pre-computed Lm in estimate_L().  Return this value.
 * The precomputation has already multiplied through by S, so we can't
 * use the values here.  But since we don't use this version of
 * Linearizer anyway, who cares?
 */

double
Linearizer2::sumOf_L_m( const Server&, const Population &N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;

    throw LibMVA::not_implemented( "Linearizer2::sumOf_L_m", __FILE__, __LINE__ );
    return 0.0;
}



/*
 * This version has pre-computed Lm in estimate_L().  Return this value.
 */

double
Linearizer2::sumOf_SL_m( const Server& station, const Population &N, const unsigned j ) const
{
    assert( 0 < j && j <= K );

    if ( N[j] < 1 ) return 0.0;
    const unsigned Nej = offset_e_c_e_j(c,j);

    return Lm[Nej][station.closedIndex];
}
