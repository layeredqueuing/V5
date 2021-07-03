/* -*- C++ -*-
 * $Id: multserv.cc 14871 2021-07-03 03:20:32Z greg $
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
#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#include "multserv.h"
#include "mva.h"
#include "prob.h"
#include "vector.h"

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

    Positive sum = sumOf_SL( solver, N, k );
	
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

    if ( rho() >= 1.0 ) {
	w = get_infinity();
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

    if ( 1.0 - rho() > 0. ) {
	sum += product * rho_J / ( static_cast<double>(J) * ( 1.0 - rho() ));
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



/*
 * Queue length for Open models.
 */

void
Markov_Phased_Reiser_Multi_Server::mixedWait( const MVA& solver, const Population& N ) const
{
    Reiser_Multi_Server::mixedWait( solver, N );
}



/*
 * Queue length for Open models -- same as open server (for now).
 */

void
Markov_Phased_Reiser_Multi_Server::openWait() const
{
    Reiser_Multi_Server::openWait();
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
    const Positive sum = effectiveBacklog( solver, N, k ) + PBusy( solver, N, k ) * departureTime( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum + meanMinimumOvertaking( solver, N, k, p );
	}
    }
}



/*
 * Queue length for Open models.
 */

void
Markov_Phased_Conway_Multi_Server::mixedWait( const MVA& solver, const Population& N ) const
{
    Reiser_Multi_Server::mixedWait( solver, N );
}


/*
 * Queue length for Open models -- same as open server (for now).
 */

void
Markov_Phased_Conway_Multi_Server::openWait() const
{
    Reiser_Multi_Server::openWait();
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




Probability
Markov_Phased_Conway_Multi_Server::PBusy( const MVA& solver, const Population& N, const unsigned k ) const
{
    return solver.PB( *this, N, k );
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
		W[e][k][p] = get_infinity();
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
    return solver.PB2( *this, N, k ) * s  / mu();
}



/*
 * Filtering function.
 */

double
Rolia_Multi_Server::filter( const MVA& solver, const double w, const unsigned e, const unsigned k, const unsigned p ) const
{
    return solver.filter() * w + (1.0 - solver.filter()) * W[e][k][p];
}


/*
 * Queue length for Open models.
 */

void
Rolia_Multi_Server::mixedWait( const MVA& solver, const Population& N ) const
{
    /* BUG 70 */
    throw not_implemented( "Rolia_Multi_Server::mixedWait", __FILE__, __LINE__ );
}


/*
 * Queue length for Open models -- same as open server (for now).
 */

void
Rolia_Multi_Server::openWait() const
{
    Reiser_Multi_Server::openWait();
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
    return solver.PB2( *this, N, k ) * solver.sumOf_L_m( *this, N, k );
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
    return solver.PB2( *this, N, k ) * (solver.sumOf_SL_m( *this, N, k ) + solver.sumOf_S2U_m( *this, N, k )) / mu();
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



/*
 * Queue length for Open models.
 */

void
Markov_Phased_Rolia_Multi_Server::mixedWait( const MVA& solver, const Population& N ) const
{
    Rolia_Multi_Server::mixedWait( solver, N );
}


/*
 * Queue length for Open models -- same as open server (for now).
 */

void
Markov_Phased_Rolia_Multi_Server::openWait() const
{
    Reiser_Multi_Server::openWait();
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
    return solver.PB2( *this, N, k ) * ( solver.sumOf_L_m( *this, N, k ) + solver.sumOf_U2_m( *this, N, k ) );
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



/*
 * Queue length for Open models.
 */

void
Markov_Phased_Rolia_PS_Multi_Server::mixedWait( const MVA& solver, const Population& N ) const
{
    Rolia_Multi_Server::mixedWait( solver, N );
}


/*
 * Queue length for Open models -- same as open server (for now).
 */

void
Markov_Phased_Rolia_PS_Multi_Server::openWait() const
{
    Reiser_Multi_Server::openWait();
}

/*----------------------------------------------------------------------*/
/*                         Bruell Multi Server.                         */
/*----------------------------------------------------------------------*/


/*
 * Set the size of the marginal probabilities needed based on the incoming
 * population vector.
 */

void
Bruell_Multi_Server::setMarginalProbabilitiesSize( const Population &N )
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


/*
 * Queue length for Open models -- same as open server (for now).
 */

void
Suri_Multi_Server::openWait() const
{
}

/* ----------------- Markov Phased Franks Multi Server  --------------- */

/*
 * Waiting time expressions as per [franks], page 157.
 */

void
Markov_Phased_Suri_Multi_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
}


/*
 * Queue length for Open models.
 */

void
Markov_Phased_Suri_Multi_Server::mixedWait( const MVA& solver, const Population& N ) const
{
}


/*
 * Queue length for Open models -- same as open server (for now).
 */

void
Markov_Phased_Suri_Multi_Server::openWait() const
{
}

/* ---------------------------- Iterators ----------------------------- */

B_Iterator::B_Iterator( const Server& aServer, const Population& N, const unsigned k )
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
B_Iterator::initialize( const unsigned j )
{
    if ( j == 0 ) return;		/* No class j.	*/
    assert( limit[j] > 0 );
    limit[j] -= 1;
}

 
/*
 * Generate population vectors.  Overrides superclass.
 */

int
B_Iterator::operator()( Population& N )
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
B_Iterator::step( Population& n, const unsigned k, const unsigned n_k )
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
A_Iterator::operator()( Population& n )
{
    int rc;
    do {
	rc = step( n, 1, J );
    } while ( rc && n[class_i] == 0 );
    return rc;
}
