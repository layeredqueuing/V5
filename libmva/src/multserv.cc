/* -*- C++ -*-
 * $Id: multserv.cc 15606 2022-05-28 13:01:28Z greg $
 *
 * Server definitions for Multiserver MVA.
 * From
 * (1) author =    "Reiser, M. and Lavenburg, S.S.",
 *     title =     "Mean Value Analysis of Closed Multichain Queueing Networks",
 *     journal =   jacm,
 *     year =      1980,
 *     volume =    27,
 *     number =    2,
 *     pages =     "313--322",
 *     month =     apr
 *
 * (2) author =    "Conway, Adrian E.",
 *     title =     "Fast Approximate Solution of Queueing Networks with
 *                  Multi-Server Chain-Dependent {FCFS} Queues",
 *     booktitle = "Modeling Techniques and Tools for Computer Performance Evaluation",
 *     publisher = "Plenum",
 *     year =      1989,
 *     editor =    "Puigjaner, Ramon and Potier, Dominique",
 *     pages =     "385--396"
 *
 * (3) author =    "{de Souza e Silva}, Edmundo and Muntz, Richard R.",
 *     title =     "Approximate Solutions for a Class of Non-Product
 *                  Form Queueing Network Models",
 *     journal =   "Performance Evaluation",
 *     year =      1987,
 *     volume =    7,
 *     pages =     "221--242"
 *
 * (4) author =    "Schmidt, Rainer",
 *     title =     "An Approximate {MVA} Algorithm for Exponential,
 *                  Class-Dependent Multiple Servers",
 *     journal =   "Performance Evaluation",
 *     year =      1997,
 *     volume =    29,
 *     pages =     "245--254",
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */

#include <config.h>
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <limits>
#include "multserv.h"
#include "mva.h"
#include "mvaexception.h"
#include "prob.h"
#include "vector.h"

// #define BUG_338	0

#if DEBUG_MVA
bool Conway_Multi_Server::debug_XE = false;
#endif

/* --- Simple Multi-Server.  All chains have identical service time --- */

/*
 * Some preliminary checking.
 */

void
Reiser_Multi_Server::initialize()
{
    assert( J > 0 );

    // I should probably check that the service time for all classes is the same.
}


/*
 * Waiting time expression for server with one entry and no phases.
 * See (3.8) [Reiser], pg 319.  NOTE:  this equation can be generalized
 * to allow for arbitrary service times, (2.14) pg 316. Marginal
 * probabilities will change (to go to |N| instead of J).
 * See also [Schmidt], pg 251, Eqn 17.
 */

void
Reiser_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = sumOf_SL( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = (S(e,k) + sum) / mu();
	}
    }
}



/*
 * Common expression.
 */

Positive
Reiser_Multi_Server::sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const
{
    return solver.sumOf_SL_m( *this, N, k ) + S( solver, N ) * solver.sumOf_P( *this, N, k );
}


/*
 * Queue length for Open models. [lavenberg] (3.128)
 */

void
Reiser_Multi_Server::openWait() const
{
    double num;
    double dem;
    double w;

    if ( rho() == 1.0 ) {
	w = std::numeric_limits<double>::infinity();
    } else if ( J < 50 ) {
	num = rho() * power( J * rho(), J - 1 );
	dem = factorial( J ) *  A() * square( 1.0 - rho() );
	w = S(0) * ( 1.0 + num / dem );
    } else {
	num = log( rho() ) + log( J * rho() ) * (J - 1);
	dem = log_factorial( J ) + log( A() * square( 1.0 - rho() ) );
	w = S(0) * ( 1.0 + exp( num - dem ) );
    }

    /* Update waiting */

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,0) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][0][p] = w;
	}
    }
}


/*
 * Queue length for Open models [lavenberg] (3.324).
 */

void
Reiser_Multi_Server::mixedWait( const MVA& solver, const Population& N ) const
{
    const Positive queue = Rho() * solver.sumOf_alphaP( *this, N );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,0) ) continue;
	W[e][0][0] = queue / V(0);
    }
}



/*
 * Alpha correction for converting mixed network to closed network.
 * See eqn 3.320 [lavenberg].
 */

double
Reiser_Multi_Server::alpha( const unsigned n ) const
{
    if ( rho() >= 1.0 ) {
	throw std::range_error( "Reiser_Multi_Server::alpha" );
    } else if ( n < J - 1 ) {
	return mu() / ( mu( J - 1 ) * power( 1.0 - rho(), n + 1 ) ) + sumOf_rho( n );
    } else {
	return Server::alpha( n );
    }
}



/*
 * `A' term for open solutions.  [lavenberg] eqn (3.135).  Optimized.
 */

double
Reiser_Multi_Server::A() const
{
    const double rho_J = J * rho();
    double product     = 1.0;
    double sum         = 0.0;

    for ( unsigned i = 1; i < J; ++i ) {
	product *= rho_J / static_cast<double>(i);
	sum     += product;
    }

    if ( rho() < 1.0 ) {
	sum += product * rho_J / ( static_cast<double>(J) * ( 1.0 - rho() ));
    } else {
	sum = std::numeric_limits<double>::infinity();
    }

    return sum;
}



/*
 * Summation term of [lavenberg] eqn (3.320).
 */

double
Reiser_Multi_Server::sumOf_rho( const unsigned n ) const
{
    double sum = 0.0;

    for ( unsigned i = 0; i <= J - 2; ++i ) {

	double product = 1.0;
	for ( unsigned h = n + 1; h <= n + i; ++h ) {
	    product *= mu( h );
	}

	const double diff = (1.0 / product) - (1.0 / (mu(J-1) * power( mu(), i-1 )));

	sum += binomial_coef( (n+i), i ) * power( Rho(), i ) * diff;
    }

    return sum;
}



/*
 * Print information about this station.
 */

std::ostream&
Reiser_Multi_Server::printHeading( std::ostream& output ) const
{
    output << ", " << J << " servers";
    return output;
}

/* -------------------- Phased Simple Multi-Server -------------------- */

/*
 * Waiting time expression for server no entries but phases.
 * Not the most brilliant solution because the utilization
 * will be underestimated getting progressively worse as %ph2 -> 100.
 */

void
Phased_Reiser_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = sumOf_SL( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	W[e][k][0] = (sum + S(e,k)) / mu();
	const double w = W[e][k][0] - S(e,k);

	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = w + S(e,k,1);
	}
    }
}



/*
 * Common expression.
 */

Positive
Phased_Reiser_Multi_Server::sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const
{
    return Reiser_Multi_Server::sumOf_SL( solver, N, k ) + solver.sumOf_S2U_m( *this, N, k );
}

/* -----------------Markov Phased Simple Multi-Server ----------------- */

/*
 * Waiting time expression for server with one entry and phases.
 * Overtaking is divided by mu() because there is pr(1/mu()) of
 * meeting our own previous service.
 */

void
Markov_Phased_Reiser_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = sumOf_SL( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = (S(e,k,1) + sum  + overtaking( k, p )) / mu();
	}
    }
}

/* ----------------- Processor Sharing Multi Server ------------------- */

void
Reiser_PS_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum  = 1.0 + sumOf_L( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) / mu() * sum;
	}
    }
}



/*
 * Common expression hoist
 */

Positive
Reiser_PS_Multi_Server::sumOf_L( const MVA& solver, const Population& N, const unsigned k ) const
{
    return solver.sumOf_L_m( *this, N, k ) + solver.sumOf_P( *this, N, k );
}

/*----------------------------------------------------------------------*/
/*                         Conway Multi Server.                         */
/*----------------------------------------------------------------------*/

/*
 * Waiting time expressions as per de Souza e Silva and Muntz, (19).
 * Also: Conway (2.7)
 */

void
Conway_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    const Positive sum = effectiveBacklog( solver, N, k ) + solver.PB( *this, N, k ) * departureTime( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) + sum;
	}
    }
}

/*
 * XE term: Eqn (18).
 */

double
Conway_Multi_Server::effectiveBacklog( const MVA& solver, const Population& N, const unsigned k ) const
{
    Positive sum = 0.0;

    if ( N[k] == 0 || V(k) == 0.0 ) return sum;

    for ( unsigned i = 1; i <= K; ++i ) {
	if ( N[i] == 0 ) continue;

	A_Iterator nextA( *this, i, N, k );
	const double xe = sumOf_PS_k( solver, N, k, nextA );
	const double q = solver.queueOnly( *this, i, N, k );
#if	DEBUG_MVA
	if ( debug_XE ) printXE( std::cout, i, N, k, xe, q );
#endif
	sum += xe * q;
    }
    return sum;
}


/*
 * XR term: Eqn (16).
 */

double
Conway_Multi_Server::departureTime( const MVA& solver, const Population& N, const unsigned k ) const
{
    if ( N[k] == 0 || V(k) == 0.0 ) return 0.0;

    B_Iterator nextB( *this, N, k );

    return  sumOf_PS_k( solver, N, k, nextB );
}



/*
 * Common expression to XE and XR calculation.  The `next' argument
 * determines the values that are used in the sum. Eqns (9) and (13).
 */

double
Conway_Multi_Server::sumOf_PS_k( const MVA& solver, const Population& N, const unsigned k, Population::Iterator& next ) const
{
    Positive sumOf_C = 0.0;
    Positive sumOf_A = 0.0;
    Population n(K);				// Need to sequence over this.

    while ( next( n ) ) {
	assert( n.sum() == mu() );
	const double A_ = A( solver, n, N, k );
	sumOf_A += A_ * meanMinimumService( n );
	sumOf_C += A_;
    }

    return (sumOf_C ? sumOf_A / sumOf_C : 0.0);
}


/*
 * `A' term: Eqn (10).
 */

double
Conway_Multi_Server::A( const MVA& solver, const Population& n, const Population& N, const unsigned k ) const
{
    if ( mu() > 10.0 ) {
	double prodOf_F = 0.0;
	double prodOf_n = 0.0;
	for ( unsigned i = 1; i <= K; ++i ) {
	    if ( n[i] == 0 ) continue;		// Fast short cut.
	    double u = solver.utilization( *this, i, N, k );
	    if ( u > 0.0 ) {
		prodOf_F += log( u ) * n[i];
	    }
	    prodOf_n += log_factorial( n[i] );
	}
	return exp( log_factorial( static_cast<unsigned>(mu()) ) + prodOf_F - prodOf_n );
    } else {
	double prodOf_F = 1.0;
	double prodOf_n = 1.0;
	for ( unsigned i = 1; i <= K; ++i ) {
	    if ( n[i] == 0 ) continue;		// Fast short cut.
	    double u = solver.utilization( *this, i, N, k );
	    if ( u > 0.0 ) {
		prodOf_F *= power( u, n[i] );
	    }
	    prodOf_n *= factorial( n[i] );
	}
	return factorial( static_cast<unsigned>(mu()) ) * prodOf_F / prodOf_n;
    }

}




/*
 * The elapsed time for the arrival of this customer until the next
 * departure is simply the minimum of the service times of customers in
 * service for exponentially distributed service times: Eqn (15).
 */

double
Conway_Multi_Server::meanMinimumService( const Population& n ) const
{
    double sum = 0.0;

    for ( unsigned k = 1; k <= K; ++k ) {
	if ( n[k] == 0 || V(k) == 0.0 ) continue;
	const double service = S(k);
	if ( service == 0.0 ) return 0.0;	/* One term is zero, ergo... */
	sum += n[k] / service;
    }

    return sum > 0.0 ? 1.0 / sum : 0.0;
}


#if	DEBUG_MVA
std::ostream&
Conway_Multi_Server::printXE( std::ostream& output, const unsigned int i, const Population& N, const unsigned int k, const double xe, const double q ) const
{
    output << "XE_{" << closedIndex << "," << k << "," << i << "}" << N << " = " << xe << ", Q* = " << q << std::endl;
    return output;
}


std::ostream&
Conway_Multi_Server::printXR( std::ostream& output, const Population& N, const unsigned int k, const double xe, const double pb ) const
{
    output << "XR_{" << closedIndex << "," << k << "}" << N << " = " << xe << ", PB = " << pb << std::endl;
    return output;
}
#endif

/* ----------------------Phased Multi-Server -------------------------- */

/*
 * Waiting time expressions as per de Souza e Silva and Muntz, (19).
 * Also: Conway (2.7)
 */

void
Phased_Conway_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    const Positive sum = effectiveBacklog( solver, N, k ) + solver.PB( *this, N, k ) * departureTime( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	W[e][k][0] = sum + S(e,k);
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = sum + S(e,k,1);
	}
    }
}

/* ----------------- Markov Phased Conway Multi Server ---------------- */

/*
 * Waiting time expressions as per de Souza e Silva and Muntz, (19).
 * Also: Conway (2.7)
 */

void
Markov_Phased_Conway_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    const Positive sum = effectiveBacklog( solver, N, k ) + solver.PB( *this, N, k ) * departureTime( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum + meanMinimumOvertaking( solver, N, k, p );
	}
    }
}



/*
 * The elapsed time for the arrival of this customer until the next
 * departure is simply the minimum of the service times of customers in
 * service for exponentially distributed service times: Eqn (15).
 *
 * Overtaking amounts to extra service time, so...
 */

Positive
Markov_Phased_Conway_Multi_Server::meanMinimumOvertaking( const MVA& solver, const Population& N, const unsigned k, const unsigned p_i ) const
{
    if ( N[k] == 0 ) return 0.0;

    const unsigned n_server = static_cast<unsigned>(mu());
    const unsigned n_client = std::min( n_server, N.sum() );
    const Probability prob = PrOT( k, p_i );

    const double mean_cust = static_cast<double>(n_server)/static_cast<double>(n_client);
    const double x1 = floor(mean_cust);
    const double x2 = ceil(mean_cust);

    double y1 = 0.0;
    double y2 = 0.0;
    double ot = 0.0;

    /* Initialize y1 and y2 */

    Probability prod = 1.0;
    double sum = 0.0;
    for ( unsigned i = 1; i <= n_server; ++i ) {
	prod *= prob;
	if ( prod == 0.0 ) break;
	sum += 1.0 / (prod * S_2(k));
	if ( i == static_cast<unsigned>(x1) ) {
	    y1 = 1.0 / sum;
	}
	if ( i == static_cast<unsigned>(x2) ) {
	    y2 = 1.0 / sum;
	}
    }

    /* Find overtaking component.  Interpolate if necessary */

    if ( x2 != x1 && y1 ) {
	/* Interpolate as exponential curve */
	const double m = log( y2 / y1 ) / ( x2 - x1 );
	const double k = y1 * exp( -m * x1 );
	ot = k * exp( m * mean_cust );
    } else {
	ot = y1;
    }

    /* Add back in "missing" utilization. */

    ot += sumOf_S2U( solver, p_i, N, k );

    return ot;
}

/*----------------------------------------------------------------------*/
/*                          Rolia Multi Server                          */
/*----------------------------------------------------------------------*/

/*
 * Waiting time expressions as per [rolia], page 157.
 */

void
Rolia_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    try {
	const Positive sum = sumOf_SL( solver, N, k );

	for ( unsigned e = 1; e <= E; ++e ) {
	    if ( !V(e,k) ) continue;

	    const double w = S(e,k) + sum;

	    for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
		W[e][k][p] = filter( solver, w, e, k, p );
	    }
	}
    }
    catch ( const std::domain_error &e ) {
	for ( unsigned e = 1; e <= E; ++e ) {
	    if ( !V(e,k) ) continue;
	    for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
		W[e][k][p] = std::numeric_limits<double>::infinity();
	    }
	}
    }
}



/*
 * Common expression -- summation term.
 */

Positive
Rolia_Multi_Server::sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const
{
    const double s = solver.sumOf_SL_m( *this, N, k );
    if ( !std::isfinite( s ) ) throw std::domain_error( "Rolia_Multi_Server::sumOf_SL" );
    return PBusy( solver, N, k ) * s  / mu();
}



/*
 * Return the probability that all stations are busy.
 */

Probability
Rolia_Multi_Server::PBusy( const MVA& solver, const Population &N, const unsigned k ) const
{
    return power( std::min( 1.0, solver.sumOf_U_m( *this, N, k ) / mu() ), static_cast<unsigned>(mu()) );
}


/*
 * Filtering function.
 */

double
Rolia_Multi_Server::filter( const MVA& solver, const double w, const unsigned e, const unsigned k, const unsigned p ) const
{
    return solver.filter() * w + (1.0 - solver.filter()) * W[e][k][p];
}

/* ------------------------ Rolia Multi Server  ----------------------- */

/*
 * Waiting time expressions as per [rolia], page 157.
 */

void
Rolia_PS_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum  = 1.0 + sumOf_L( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	const double w = S(e,k) * sum;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = filter( solver, w, e, k, p );
	}
    }
}



/*
 * Common expression -- summation term.
 */

Positive
Rolia_PS_Multi_Server::sumOf_L( const MVA& solver, const Population& N, const unsigned k ) const
{
    return PBusy( solver, N, k ) * solver.sumOf_L_m( *this, N, k );
}

/* ------------------------ Rolia Multi Server  ----------------------- */

/*
 * Waiting time expressions as per [rolia], page 157.
 */

void
Phased_Rolia_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = sumOf_SL( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	const double w = S(e,k,1) + sum;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = filter( solver, w, e, k, p );
	}
    }
}



/*
 * Common expression -- summation term.		BUG_555
 */

Positive
Phased_Rolia_Multi_Server::sumOf_SL( const MVA& solver, const Population& N, const unsigned k ) const
{
    return PBusy( solver, N, k ) * (solver.sumOf_SL_m( *this, N, k ) + solver.sumOf_S2U_m( *this, N, k )) / mu();
}

/* --------------------- Markov Rolia Multi Server  ------------------- */

/*
 * Waiting time expressions as per [rolia], page 157.
 * Overtaking is divided by mu() because there is pr(1/mu()) of
 * meeting our own previous service.
 */

void
Markov_Phased_Rolia_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = sumOf_SL( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	const double w = S(e,k,1) + sum;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = filter( solver, w + overtaking( k, p ) / mu(), e, k, p );
	}
    }
}

/* ------------------------ Rolia Multi Server  ----------------------- */

/*
 * Waiting time expressions as per [rolia], page 157.
 */

void
Phased_Rolia_PS_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum  = sumOf_L( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	const Positive w = S(e,k,1) + S(e,k) * sum;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = filter( solver, w, e, k, p );
	}
    }
}



/*
 * Common expression -- summation term.
 */

Positive
Phased_Rolia_PS_Multi_Server::sumOf_L( const MVA& solver, const Population& N, const unsigned k ) const
{
    return PBusy( solver, N, k ) * ( solver.sumOf_L_m( *this, N, k ) + solver.sumOf_U2_m( *this, N, k ) );
}

/* ------------------------ Rolia Multi Server  ----------------------- */

/*
 * Waiting time expressions as per [rolia], page 157.
 * Overtaking is divided by mu() because there is pr(1/mu()) of
 * meeting our own previous service.
 */

void
Markov_Phased_Rolia_PS_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = sumOf_L( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	const Positive w = S(e,k,1) + S(e,k) * sum;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = filter( solver, w + overtaking( k, p ) / mu(), e, k, p );
	}
    }
}

/*----------------------------------------------------------------------*/
/*                          Zhou Multi Server.                          */
/*----------------------------------------------------------------------*/

/*
 * This is the same as the Rolia multi server.  Subclass?  But there are
 * possible issues with the marginals.
 */

void
Zhou_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    /* Compute the time spent in the queue */
    const Positive sum = sumOf_SL( solver, N, k );	// Wait_AB in thesis

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) + sum;			// Call from logindelay to login.
	}
    }
}


/*
 * wait_AB() function.  renamed for possible classification.
 */

Positive
Zhou_Multi_Server::sumOf_SL( const MVA& solver, const Population& N, const unsigned ) const
{
    const unsigned N_sum = sumOf_N( N );		/* Number of customers visting this station */
    if ( N_sum == 0 ) return 0.;			/* No customers */

    const Probability P = P_mean( solver, N );		// Residence time divided by cycle time (1/lambba)
    if ( P == 0.0 ) return 0.;				/* server not used at all */
    
    const unsigned m = static_cast<unsigned>(mu());
    const double S = S_mean( solver );			// Ratio of service times (by throughput)

    if ( P == 1.0 ) return static_cast<double>(N_sum - m) * S
		      / static_cast<double>(m);		/* server full utilized */

    const unsigned nMax = std::min(N_sum,m);		// nMax = min(N,m);
    const unsigned Nm1 = N_sum - 1;			// Nm1 = N-1;

    const double pw_0 = std::pow( 1.0 - P, Nm1 );	// pw[0]= pow(1-P,Nm1); 
    Probability pDash = pw_0;				// pDash = pw[0];
    double M2 = 0.;					// M2 = 0;
    if ( pw_0 > 1.0e-10 ) {
	double pw_im1 = pw_0;
	for ( unsigned int i = 1; i < nMax; ++i ) {		// for (i = 1; i<=nMaxM1; i = i+1) {
	    const double pw_i = pw_im1 * P * static_cast<double>(N_sum - i)
		/ (static_cast<double>(i) * (1.0 - P));		// pw[i] = pw[i-1]*P*(N-i)/(i*(1-P));
	    pDash += pw_i;					// pDash = pDash + pw[i];
	    M2 += i * pw_i;					// M2 + i*pw[i];
	    pw_im1 = pw_i;
	}
    } else {
	double pw_im1 = log( 1.0 - P ) * Nm1;
	for ( unsigned int i = 1; i < nMax; ++i ) {		// for (i = 1; i<=nMaxM1; i = i+1) {
	    const double pw_i = pw_im1 + log( P * static_cast<double>(N_sum - i) )
		- log( static_cast<double>(i) * (1.0 - P) );	// pw[i] = pw[i-1]*P*(N-i)/(i*(1-P));
	    pDash += exp( pw_i );				// pDash = pDash + pw[i];
	    M2 += i * exp( pw_i );				// M2 + i*pw[i];
	    pw_im1 = pw_i;
	}
    }

    const double L = P * Nm1  				// L = P*(Nm1)-(m-1)*(1-pDash) - M2;
	- static_cast<double>(m - 1) * (1.0 - pDash) - M2;
    if ( 0. > L && L > -0.0000001 ) return 0;		// Floating point precision
#if DEBUG_MVA
    if ( MVA::debug_P ) std::cout << closedIndex << ": P=" << P << ", pw[0]=" << pw_0 << ", pDash=" << pDash << ", M2=" << M2 << ", L=" << L << ", S=" << S << std::endl;
#endif
    return L * S / static_cast<double>(m);		// return(L*S/m);
}


/*
 * Return the total number of customers that can possibly be at this
 * station
 */

unsigned int
Zhou_Multi_Server::sumOf_N( const Population& N ) const
{
    unsigned int N_sum = 0;
    for ( unsigned int k = 1; k <= K; ++k ) {
	if ( V(k) == 0 ) continue;
	N_sum += N[k];
    }
    return N_sum;
}

/*
 * Mean service time by throughput.
 *   Prop1 = tp1/(tp1+tp2); 
 *   Prop2 = tp2/(tp1+tp2);
 *   MergedS = Prop1*S1 + Prop2*S2; //weighted average service time
 */

double
Zhou_Multi_Server::S_mean( const MVA& solver ) const
{
    double sumOf_X = 0.0;
    double sumOf_S = 0.0;
    for ( unsigned int k = 1; k <= K; ++k ) {
	if ( V(k) == 0 ) continue;
	const double X = solver.throughput( *this, k ); 
	sumOf_X += X;
	sumOf_S += this->S(k) * X;
    }
    return sumOf_X > 0. ? sumOf_S / sumOf_X : 0.0;
}

/*
 * Probability term.  W is queueing time at this station.  S + W = R (residence time).
 * Mean think time by throughput.  This think time is NOT the chain think time.
 * Rather, it's the think time for this station (cycle time - residence time).
 * 
 *   //chain1 think times (submodel)
 *   ZZ1 = $n1/tp1-S1-$W; 
 *   ZZ2 = $n2/tp2-S2-$W; 
 *   MergedZ = Prop1*ZZ1 + Prop2*ZZ2; //weighted average think time
 *
 *   MergedP = (MergedS+$W)/(MergedZ + MergedS + $W); //corresponding probability 
 */

#define BUG_349_COMMENT_8 1
Probability
Zhou_Multi_Server::P_mean( const MVA& solver, const Population& N ) const
{
#if BUG_349_COMMENT_8
    double sumOf_WX = 0.;
    unsigned int sumOf_N  = 0;
    for ( unsigned int k = 1; k <= K; ++k ) {
	if ( V(k) == 0 ) continue;				// No visits for this class.
	sumOf_N += N[k];
	const double X = solver.throughput( *this, k );		// Hoist offset(NCust);
	for ( unsigned int e = 1; e <= E; ++e ) {
	    sumOf_WX += W[e][k][0] * V(e,k) * X;
	}
    }
    /* Orignal expression from Murray was S+W, but THAT W is queueing only..., so
     * don't bother with S_mean...*/
//    return f * (W) / sumOf_N;
    return std::min( sumOf_WX / static_cast<double>(sumOf_N), 1.0 );	// truncate at 1
#else
//  double sumOf_X = 0.0;					// sumOf_X cancels out.
    double sumOf_Z = 0.0;
    double sumOf_R = 0.0;
    for ( unsigned int k = 1; k <= K; ++k ) {
	if ( V(k) == 0 ) continue;				// No visits for this class.
	const double X_k = solver.throughput( *this, k );
	if ( X_k == 0. ) continue;
	const double R_k = this->R(k);
//	sumOf_X += X_k;						// sumOf_X cancels out.
	sumOf_Z += std::max( (N[k] / X_k) - R_k, 0.0 ) * X_k;	// don't allow negative numbers
	sumOf_R += R_k * X_k;					// Weighted mean
    }
    return sumOf_R / (sumOf_R + sumOf_Z);			// (R/X)/((R/X+Z/X) = (R/X)/((R+Z)/X) = R/(R+Z)
#endif
}

/* -------------------- Phased Simple Multi-Server -------------------- */

void
Phased_Zhou_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    throw LibMVA::not_implemented( "Phased_Zhou_Multi_Server::wait", __FILE__, __LINE__ );
}


void
Markov_Phased_Zhou_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    throw LibMVA::not_implemented( "Markov_Phased_Zhou_Multi_Server::wait", __FILE__, __LINE__ );
}

/*----------------------------------------------------------------------*/
/*                         Bruell Multi Server.                         */
/*----------------------------------------------------------------------*/


/*
 * Set the size of the marginal probabilities needed based on the incoming
 * population vector.
 */

void
Bruell_Multi_Server::setMarginalProbabilitiesSize( const Population& N )
{
    Population::IteratorOffset next( N, N );

    marginalSize = next.maxOffset();
}



/*
 * Return the State dependent S term.  See Bruell, Eqn 5.
 */

double
Bruell_Multi_Server::muS( const Population& N, const unsigned k ) const
{
    return N.sum() * S(k) / mu(N.sum());
}



/*
 * Waiting time expressions as per de Souza e Silva and Muntz, (19).
 * Also: Bruell (2.7)
 */

void
Bruell_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    const double sum = solver.sumOf_SP2( *this, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = sum;
	}
    }
}

/*----------------------------------------------------------------------*/
/*                         Schmidt Multi Server.                        */
/*----------------------------------------------------------------------*/


void
Schmidt_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    const double sum = solver.sumOf_SP2( *this, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = sum;
	}
    }
}



/*
 * `B' term: Eqn (6).  Our waiting times are expressed per visit,
 * so adjust accordingly.
 */

double
Schmidt_Multi_Server::muS( const Population& N, const unsigned k ) const
{
    double sum = S(k);
    const double V_k = V(k);

    if ( N.sum() > mu() && V_k > 0.0 ) {
	double sum1 = 0.0;
	for ( unsigned i = 1; i <= K; ++i ) {
	    sum1 += N[i] * V(i) * S(i);
	}
	sum1 /= V_k;
	sum += (static_cast<double>(N.sum()) - mu()) / ( mu() * static_cast<double>(N.sum() - 1) ) * (sum1 - S(k));
    }

    return sum;
}

/*----------------------------------------------------------------------*/
/*                           Suri Multi Server                          */
/*----------------------------------------------------------------------*/

const double Suri_Multi_Server::alpha = 4.464;		// Eqn (17), Suri et. al.
const double Suri_Multi_Server::beta  = 0.676;

/*
 * Waiting time expressions as per Suri et. al.
 */

void
Suri_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    const double C = mu();
    Positive sum = 1.0;
    if ( N.sum() > C ) {
	const double rho = solver.utilization( *this, N ) / C;
	const double L_m = solver.sumOf_L_m( *this, N, k );
	sum += L_m * pow( rho, alpha * ( pow( C, beta ) - 1.0 ) ) / C;
    }

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) * sum;
	}
    }
}

void
Suri_Multi_Server::mixedWait( const MVA& solver, const Population& N ) const
{
    throw LibMVA::not_implemented( "Suri_Multi_Server::mixedWait", __FILE__, __LINE__ );
}


void
Suri_Multi_Server::openWait() const
{
    throw LibMVA::not_implemented( "Suri_Multi_Server::openWait", __FILE__, __LINE__ );
}

/* ----------------- Markov Phased Franks Multi Server  --------------- */

/*
 * Waiting time expressions as per [franks], page 157.
 */

void
Markov_Phased_Suri_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    throw LibMVA::not_implemented( "Markov_Phased_Suri_Multi_Server::wait", __FILE__, __LINE__ );
}

/* ---------------------------- Iterators ----------------------------- */

Conway_Multi_Server::B_Iterator::B_Iterator( const Server& aServer, const Population& N, const unsigned k )
    : Population::Iterator(N), J(static_cast<unsigned>(aServer.mu())), K(N.size()), index(0)
{
    for ( unsigned i = 1; i <= K; ++i ) {
	if ( !aServer.V(i) ) {
	    limit[i] = 0;
	}
    }
    initialize(k);
}

/*
 * Subtract one customer from class k.
 */

void
Conway_Multi_Server::B_Iterator::initialize( const unsigned j )
{
    if ( j == 0 ) return;		/* No class j.	*/
    assert( limit[j] > 0 );
    limit[j] -= 1;
}


/*
 * Generate population vectors.  Overrides superclass.
 */

int
Conway_Multi_Server::B_Iterator::operator()( Population& N )
{
    return step( N, 1, J );
}


/*
 * Ultra-funky recursive step function.  It returns 1 if the population
 * vector was updated properly, and zero otherwise.  The inherited value
 * of limit sets an upper bound to the population in a given class.
 * Note: an index value of zero will initialize the vector to the first
 * feasible population.
 */

int
Conway_Multi_Server::B_Iterator::step( Population& n, const unsigned k, const unsigned n_k )
{
    /*
     * Update the present value at `k', but ONLY if `k' is greater
     * than `index'.  When `k' is larger than the number of elements
     * in the population vector, the termination test (end of the
     * line) checks for a zero value for n.  A non-zero value denotes
     * an in-feasible population.
     */

    if ( k > K ) {
	return n_k == 0;		/* End of the line, mine.	*/
    } else if ( k > index ) {
	n[k] = std::min( n_k, limit[k] );	/* Updating past "index".	*/
    } else if ( k == index ) {
	n[k] -= 1;			/* Updating at "index".		*/
    }

    /*
     * Now that we have updated ourself, update all values to our
     * right, i.e., k > index.  step() will return with 0 if we have
     * an infeasible population.  If this is the case, move left in
     * the vector and try again.
     */

    while ( !step( n, k + 1, n_k - n[k] ) ) {
	if ( n[k] == 0 ) {
	    index = k - 1;		/* Reset back one more class.	*/
	    return 0;			/* Back off even more.		*/
	} else {
	    n[k] -= 1;
	    index = k;			/* Reset index due to back off.	*/
	}
    }

    /*
     * Set index to the largest value of k which is not zero.  Next
     * call from root will start from this value.
     */

    if ( k > index && n[k] > 0 ) {
	index = k;
    }
    return 1;
}



/*
 * Generate population vectors using rule for type `A'.  Basically, we
 * always want a non-zero population in class `j'.
 */

int
Conway_Multi_Server::A_Iterator::operator()( Population& n )
{
    int rc;
    do {
	rc = step( n, 1, J );
    } while ( rc && n[class_i] == 0 );
    return rc;
}
