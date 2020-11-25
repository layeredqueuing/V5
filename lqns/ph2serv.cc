/* -*- c++ -*-
 * $Id: ph2serv.cc 14140 2020-11-25 20:24:15Z greg $
 *
 * Server definitions for MVA.  More complicated that those in server.C
 *
 * (1) author =    "Gelenbe, E. and Mitrani I.",
 *     title =     "Analysis and Synthesis of Computer Systems",
 *     publisher = "Academic Press",
 *     year =      1980,
 *     series =    "Computer Science and Applied Mathematics",
 *     address =   "Toronto",
 *     callno =    "QA76.9.E94G44",
 *     isbn =      0122793501,
 *
 * (2) author =    "Rolia, Jerome Alexander",
 *     title =     "Predicting the Performance of Software Systems",
 *     school =    "Univerisity of Toronto",
 *     year =      1992,
 *     address =   "Toronto, Ontario, Canada.  M5S 1A1",
 *     month =     jan,
 *     note =      "Also as: CSRI Technical Report CSRI-260",
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */
//#define	DEBUG

#include "dim.h"
#include <cmath>
#include <stdarg.h>
#include <cstdlib>
#include "ph2serv.h"
#include "vector.h"
#include "server.h"
#include "mva.h"
#include "prob.h"


PHASE2_CORRECTION phase2_correction = COMPLEX_PHASE2;

/*
 * Compute the first half of w[e][d].  (See Eqn 8.).  Residual service time.
 */

double
Phased_Server::residual( const unsigned e, const unsigned k, const unsigned p ) const
{
    Positive sum_x = r(e,k,p);
    for ( unsigned q = p + 1; q <= P; q++ ) {
	sum_x += S(e,k,q);
    }

    return sum_x;
}


/*
 * Service time squared plus variance (for open M/G+G/1 stations).
 */

Positive
Phased_Server::MGplusG1() const
{
    Positive sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	sum += V(e,0) * square( S(e,0) );
    }
	
    return sum / (2.0 * (1.0 - rho()));
}



/*
 * Waiting time for Open models -- [lavenberg] eqn (3.87) and
 * [Gelenbe], chapter 2, for model.
 */

void
Phased_Server::openWait() const
{
    const Positive w = MGplusG1();

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,0) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][0][p] = S(e,0,1) + w;
	}
    }
}

/* ---------------- Multi-phase, Single Entry Server ------------------ */

/*
 * Destructor...
 */

Rolia_Phased_Server::~Rolia_Phased_Server()
{
    for ( unsigned e = 1; e <= E; ++e ) {
	delete [] Gamma[e];
    }
    delete [] Gamma;
}



/*
 * Allocate storage for Gamma, the ``overtaking probability''.  Although
 * this class only supports one entry, Gamma is dimensioned by E because
 * the multi-entry server class is subclassed off this one.  Gamma
 * calculations are performed here.
 */

void
Rolia_Phased_Server::initialize()
{
    Gamma = new double * [E+1];

    Gamma[0] = 0;
    for ( unsigned e = 1; e <= E; ++e ) {
	Gamma[e] = new double [K+1];
    }
}



/*
 * Initialize Gamma for this server.
 * Rendezvous server -- Rolia[2], pg 109.
 */

void
Rolia_Phased_Server::initStep( const MVA& solver )
{
    for ( unsigned e = 1; e <= E; ++e ) {
	unsigned k;

	for ( k = 1; k <= K; ++k ) {
	    const double temp = V(e,k) * S_2(e,k);
	    if ( temp == 0.0 ) {
		Gamma[e][k] = 0.0;
	    } else {
		Gamma[e][k] = temp / ( temp + solver.responseTime(*this,k) );
	    }
	}
    }
}



/*
 * Return overtaking term.  Probably not correct for HVFCFS and more than 2 phases.
 */

Positive
Rolia_Phased_Server::overtaking( const unsigned k ) const
{
    Positive sum = 0;

    for ( unsigned e = 1; e <= E; ++e ) {
	sum += eta(e,k) * Gamma[e][k] * residual(e,k,2);
    }
	
    return sum;
}



/*
 * Waiting time expression for Server with Phases.  [rolia], pg 109.
 * NO entries!
 */

void
Rolia_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( k <= K );

    const Positive sum = solver.sumOf_SL_m( *this, N, k ) + overtaking( k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum;
	}
    }
}



/*
 * Print extra information out about this station.
 */

std::ostream&
Rolia_Phased_Server::printOutput( std::ostream& output, const unsigned i ) const
{
    int width = output.precision() + 2;
	
    for ( unsigned e = 1; e <= E; ++e ) {
	if ( e == 1 ) { 
	    output << i << ": ";
	} else {
	    output << "   ";
	}
		
	for ( unsigned k = 1; k <= K; ++k ) {
	    output << "Gamma_" << e << k << " = " << std::setw(width) << Gamma[e][k];
	    if ( k < K ) output << ", ";
	}
	output << std::endl;
    }
    return output;
}

/* --------- Multi-phase, Single Entry Server with Priorities --------- */

/*
 * Waiting time expression for Server with Phases.  [rolia], pg 109.
 * NO entries!
 */

void
HOL_Rolia_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( k <= K );

    const Positive sum = (solver.sumOf_SL_m( *this, N, k ) + overtaking( k ) + solver.sumOf_SU_m( *this, N, k ))
	/ ( 1.0 - solver.priorityInflation( *this, N, k ) );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum;
	}
    }
}

/* --------- Multi-phase, Single Entry Server with Priorities --------- */

/*
 * Waiting time expression for Server with Phases.  [rolia], pg 109.
 * NO entries!
 */

void
PR_Rolia_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    Rolia_Phased_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}

/* ----------------- High Variance Phased FIFO Server ----------------- */

/*
 * Waiting time expression for server with entries and multiple phases.
 * See [rolia], pg 135.
 */

void
HVFCFS_Rolia_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    double sum = solver.sumOf_SQ_m( *this, N, k ) + solver.sumOf_rU_m( *this, N, k ) + overtaking(k);
    if ( sum < 0.0 ) sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum;
	}
    }
}

/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */


/*
 * Waiting time expression for server with entries and multiple phases.
 * See [rolia], pg 135.
 */

void
HOL_HVFCFS_Rolia_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    double sum = (  solver.sumOf_SQ_m( *this, N, k )
		    + solver.sumOf_rU_m( *this, N, k )
		    + overtaking(k)
		    + solver.sumOf_SU_m( *this, N, k ))
		/ ( 1.0 - solver.priorityInflation( *this, N, k ) );
    if ( sum < 0.0 ) sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum;
	}
    }
}

/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */


/*
 * Waiting time expression for server with entries and multiple phases.
 * See [rolia], pg 135.
 */

void
PR_HVFCFS_Rolia_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    HVFCFS_Rolia_Phased_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}

/* --------- Multi-phase, Single Entry Server, U Correction ------------ */

/*
 * Return overtaking component.  There are two parts.  The
 * S2U term compensates for the fact that the waiting time does not
 * include the full amount of service and hence must be added back in.
 */
 
Positive
Simple_Phased_Server::sumOf_S2U( const MVA& solver, const Population& N, const unsigned k ) const
{
    Positive sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	if ( phase2_correction == SIMPLE_PHASE2 ) {
	    sum += ( 1.0 - prOt(e,k,0) ) * solver.sumOf_S2U_m( *this, e, N, k );
	} else {
	    sum += solver.sumOf_USPrOt_m( *this, e, prOt(e,k,0), N, k );
	}
    }
    return sum;
}



/*
 * Waiting time expression for Server with Phases.  
 * Server-based overtaking.
 */

void
Simple_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( k <= K );

    const Positive sum = solver.sumOf_SL_m( *this, N, k ) + overtaking( k ) + sumOf_S2U( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum;
	}
    }
}



/*
 * Print extra information out about this station.
 */

std::ostream&
Simple_Phased_Server::printOutput( std::ostream& output, const unsigned i ) const
{
    return Phased_Server::printOutput( output, i );
}

/* -- Multi-phase, Single Entry Server with Priorities, U Correction -- */

/*
 */

void
HOL_Simple_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( k <= K );

    const Positive sum = (solver.sumOf_SL_m( *this, N, k ) + overtaking( k ) + sumOf_S2U( solver, N, k ) + solver.sumOf_SU_m( *this, N, k ))
	/ ( 1.0 - solver.priorityInflation( *this, N, k ) );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum;
	}
    }
}

/* -- Multi-phase, Single Entry Server with Priorities, U Correction -- */

/*
 */

void
PR_Simple_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    Simple_Phased_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}

/* ---------- High Variance Phased FIFO Server, U Correction ---------- */

/*
 * Waiting time expression for server with entries and multiple phases.
 * See [rolia], pg 135.
 */

void
HVFCFS_Simple_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    double sum = solver.sumOf_SQ_m( *this, N, k ) + solver.sumOf_rU_m( *this, N, k )
		+ overtaking(k) + sumOf_S2U( solver, N, k );
    if ( sum < 0.0 ) sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum;
	}
    }
}

/* Multiple-phase, Multiple Entry Server with Priorities, U Correction */


/*
 * Waiting time expression for server with entries and multiple phases.
 * See [rolia], pg 135.
 */

void
HOL_HVFCFS_Simple_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    double sum = (solver.sumOf_SQ_m( *this, N, k ) + solver.sumOf_rU_m( *this, N, k )
		  + overtaking(k) + sumOf_S2U( solver, N, k )
		  + solver.sumOf_SU_m( *this, N, k ))
		/ ( 1.0 - solver.priorityInflation( *this, N, k ) );
    if ( sum < 0.0 ) sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1) + sum;
	}
    }
}

/* Multiple-phase, Multiple Entry Server with Priorities, U Correction */


/*
 * Waiting time expression for server with entries and multiple phases.
 * See [rolia], pg 135.
 */

void
PR_HVFCFS_Simple_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    HVFCFS_Simple_Phased_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}

/* ------------------- Markov based Phased Server  -------------------- */

/*
 * Destructor.
 */

Markov_Phased_Server::~Markov_Phased_Server()
{
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 1; k <= K; ++k ) {
	    for ( unsigned i = 0; i <= MAX_PHASES; ++i ) {
		delete [] prOvertake[e][k][i];
	    }
	    delete [] prOvertake[e][k];
	}
	delete [] prOvertake[e];
    }
    delete [] prOvertake;
}



/*
 * Allocate memory for overtaking probabilities.
 */

void
Markov_Phased_Server::initialize()
{
    prOvertake = new Probability *** [E+1];
    prOvertake[0] = 0;
	
    for ( unsigned e = 1; e <= E; ++e ) {

	prOvertake[e] = new Probability ** [K+1];
	prOvertake[e][0] = 0;
		
	for ( unsigned k = 1; k <= K; ++k ) {
	    prOvertake[e][k] = new Probability * [MAX_PHASES+1];

	    for ( unsigned i = 0; i <= MAX_PHASES; ++i ) {		// Calling Phase.
		prOvertake[e][k][i] = new Probability [P+1];

		for ( unsigned j = 0; j <= P; ++j ) {		// Called Phase.
		    prOvertake[e][k][i][j] = 0.0;
		}
	    }
	}
    }
}



/*
 * Allocate memory for overtaking probabilities.
 */

void
Markov_Phased_Server::clear()
{
    Server::clear();
	
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 1; k <= K; ++k ) {
	    for ( unsigned i = 0; i <= MAX_PHASES; ++i ) {		// Calling Phase.
		for ( unsigned j = 0; j <= P; ++j ) {		// Called Phase.
		    prOvertake[e][k][i][j] = 0.0;
		}
	    }
	}
    }
}



/*
 * Set overtaking probability.
 */

Probability ***
Markov_Phased_Server::getPrOt( const unsigned e ) const
{
    assert( 0 < e && e <= E );

    return prOvertake[e];
}



/*
 * Return overtaking component.  There are two parts.  The residual term
 * is the waiting due to our arrival seeing ourself in phase 2 or 3.  
 */

Positive
Markov_Phased_Server::overtaking( const unsigned k, const unsigned p_i ) const
{
    Positive sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned p_j = 2; p_j <= P; ++p_j ) {
	    sum += prOvertake[e][k][p_i][p_j] * residual( e, k, p_j );
	}
    }
    return sum;
}


/*
 * Return overtaking probability regardless of phase of caller.
 */

Probability
Markov_Phased_Server::PrOT( const unsigned k, const unsigned p_i ) const
{
    Probability sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned p_j = 2; p_j <= P; ++p_j ) {
	    sum += prOvertake[e][k][p_i][p_j];
	}
    }
    return sum;
}



/*
 * Return overtaking component.  There are two parts.  The
 * S2U term compensates for the fact that the waiting time does not
 * include the full amount of service and hence must be added back in.
 */

Positive
Markov_Phased_Server::sumOf_S2U( const MVA& solver, const unsigned p_i, const Population& N, const unsigned k ) const
{
    Positive sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	Probability sumOf_prOt = 0.0;
	for ( unsigned p_j = 2; p_j <= P; ++p_j ) {
	    sumOf_prOt += prOvertake[e][k][p_i][p_j];
	}

	if ( phase2_correction == SIMPLE_PHASE2 ) {
	    sum += ( 1.0 - sumOf_prOt ) * solver.sumOf_S2U_m( *this, e, N, k );
	} else {
	    sum += solver.sumOf_USPrOt_m( *this, e, sumOf_prOt, N, k );
	}
    }
    return sum;
}


/*
 * Waiting time expression for server with entries and multiple phases.
 * The S2U term compensates for the fact that the waiting time does not
 * include the full amount of service and hence must be added back in.
 * (i.e., the ``new customers'').  See [].
 */

void
Markov_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = solver.sumOf_SL_m( *this, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    if ( !V(e,k,p) ) continue;
#if 0
	    std::cout << closedIndex << ": S(" << e << "," << k << "," << p << ")=" << S(e,k,p)
		 << ", overtaking(k,p)=" << overtaking( k, p )
		 << ", sumOf_S2U(p,N,k)=" << sumOf_S2U( solver, p, N, k ) << std::endl;
#endif
	    W[e][k][p] = S(e,k,1) + sum + overtaking( k, p ) + sumOf_S2U( solver, p, N, k );
	    //	cout<<"S(e,k,1)= "<<S(e,k,1) <<" ,sum= "<<sum<<", overtaking( k, p )="<< overtaking( k, p ) <<", sumOf_S2U( solver, p, N, k )="<< sumOf_S2U( solver, p, N, k )<<endl;
	}
    }
}



/*
 * Print information about this station.
 */

std::ostream&
Markov_Phased_Server::printInput( std::ostream& output, const unsigned e, const unsigned k  ) const
{
    Server::printInput( output, e, k );
    printOvertaking( output, e, k );
    return output;
}



/*
 * Print overtaking information.
 */

std::ostream&
Markov_Phased_Server::printOvertaking( std::ostream& output, const unsigned e, const unsigned k  ) const
{
    if ( k == 0 ) return output;	/* No overtaking for open class. */
	
    for ( unsigned p_j = 2; p_j <= P; ++p_j ) {
	output << "  Pr{Ot(" << e << ',' << k << ",p_i," << p_j << ")} = ";
	for ( unsigned p_i = 1; p_i <= MAX_PHASES; ++p_i ) {
	    output << prOvertake[e][k][p_i][p_j] << ", ";
	}
	output << '[' << prOvertake[e][k][0][p_j] << ']' << std::endl;
    }
    return output;
}

/* ------------ Markov based Phased Server with Priorities ------------ */

/*
 * Waiting time expression for server with entries and multiple phases.
 * See [].
 */

void
HOL_Markov_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = solver.sumOf_SL_m( *this, N, k ) + solver.sumOf_SU_m( *this, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    if ( !V(e,k,p) ) continue;
	    W[e][k][p] = S(e,k,1) + (sum + overtaking( k, p ) + sumOf_S2U( solver, p, N, k ))
		/ ( 1.0 - solver.priorityInflation( *this, N, k ) );
	}
    }
}

/* ------------ Markov based Phased Server with Priorities ------------ */

/*
 * Waiting time expression for server with entries and multiple phases.
 * See [].
 */

void
PR_Markov_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    Markov_Phased_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;
		
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}

/* ----------------- High Variance Phased FIFO Server ----------------- */

/*
 * Clear variables.
 */

void
HVFCFS_Markov_Phased_Server::clear()
{
    Markov_Phased_Server::clear();
    HVFCFS_Server::clear();
}


/*
 * Waiting time expression for server with entries and multiple phases.
 */

void
HVFCFS_Markov_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    double sum = solver.sumOf_SQ_m( *this, N, k ) + solver.sumOf_rU_m( *this, N, k );
    if ( sum < 0.0 ) sum = 0.0;
#if 0
    std::cout << "m=" << closedIndex << ": N" << N << ", k=" << k
	 << ", SQ_m=" << solver.sumOf_SQ_m( *this, N, k )
	 << ", Ru_m=" << solver.sumOf_rU_m( *this, N, k ) << std::endl;
#endif

    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    if ( !V(e,k,p) ) continue;
#if 0
	    std::cout << closedIndex << ": S(" << e << "," << k << "," << p << ")=" << S(e,k,p)
		 << ", OT(k,p)=" << overtaking( k, p )
		 << ", S2U(p,N,k)=" << sumOf_S2U( solver, p, N, k ) << std::endl;
#endif
	    W[e][k][p] = S(e,k,1) + sum + overtaking( k, p ) + sumOf_S2U( solver, p, N, k );
	    //cout<<"S(e,k,1)= "<<S(e,k,1) <<" ,sum= "<<sum<<", overtaking( k, p )="<< overtaking( k, p ) <<", sumOf_S2U( solver, p, N, k )="<< sumOf_S2U( solver, p, N, k )<<endl;
	}
    }
}



/*
 * Print input.
 */

std::ostream&
HVFCFS_Markov_Phased_Server::printInput( std::ostream& output, const unsigned e, const unsigned k ) const
{
    HVFCFS_Server::printInput( output, e, k );
    printOvertaking( output, e, k );

    return output;
}



/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */

/*
 * Waiting time expression for server with entries and multiple phases.
 */

void
HOL_HVFCFS_Markov_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    double sum = solver.sumOf_SQ_m( *this, N, k ) + solver.sumOf_rU_m( *this, N, k ) + solver.sumOf_SU_m( *this, N, k );
    if ( sum < 0.0 ) sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    if ( !V(e,k,p) ) continue;
	    W[e][k][p] = S(e,k,1) + (sum + overtaking( k, p ) + sumOf_S2U( solver, p, N, k ))
		/ ( 1.0 - solver.priorityInflation( *this, N, k ) );
	}
    }
}

/* ------ Multiple-phase, Multiple Entry Server with Priorities ------- */

/*
 * Waiting time expression for server with entries and multiple phases.
 */

void
PR_HVFCFS_Markov_Phased_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    HVFCFS_Markov_Phased_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}
