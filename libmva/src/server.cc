/*  -*- C++ -*-
 * $Id: server.cc 15089 2021-10-22 16:14:46Z greg $
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * Server definitions for MVA.
 * From:
 * (1) author =    "Chandy, K. Mani and Neuse, Doug",
 *     title =     "Linearizer: A Heuristic Algorithm for Queueing
 *                  Network Models of Computing Systems",
 *     journal =   cacm,
 *     year =      1982,
 *     volume =    25,
 *     number =    2,
 *     pages =     "126--134",
 *     month =     feb
 *
 * Open class:
 *     author =    "Lavenberg, Stephen S. and Sauer, Charles H.",
 *     title =     "Analytical Results for Queueing Models",
 *     booktitle = "Computer Performance Modeling Handbook",
 *     publisher = "Academic Press",
 *     year =      1982,
 *     editor =    "Lavenberg, Stephen S.",
 *     volume =    4,
 *     series =    "Notes and Reports in Computer Science and Applied Mathematics.",
 *     pages =     "56--172",
 *     chapter =   3,
 *     address =   "Toronto, ON"
 *
 * Note: k = 0 is reserved for OPEN classes.
 *
 * ------------------------------------------------------------------------
 */

#include <cstdlib>
#include <cmath>
#include <limits>
#include "mva.h"
#include "open.h"
#include "prob.h"
#include "server.h"
#include "vector.h"

//#define		DEBUG
/* ----------------------- Helper Functions --------------------------- */

/*
 * Print all results.
 */

std::ostream&
operator<<( std::ostream& output, const Server& self )
{
    return self.print( output );
}

/* ----------------------- Abstract Superclass. ----------------------- */

/*
 * Allocate storage for arrays.  L and U are allocated by the
 * appropriate MVA solver.  
 */

void
Server::initialize()
{
    unsigned e, k;
	
    if ( E == 0 ) {
	throw std::out_of_range( "Server::initialize -- entries" );
    }
    if ( P == 0 || MAX_PHASES < P ) {
	throw std::out_of_range( "Server::initialize -- phases" );
    }

    W = new double ** [E+1];
    v = new double ** [E+1];
    s = new double ** [E+1];

    IL = new Probability * [E+1];

    W[0] = 0;
    s[0] = 0;
    IL[0] = 0;
    for ( e = 1; e <= E; ++e ) {
	v[e] = new double * [K+1];
	W[e] = new double * [K+1];
	s[e] = new double * [K+1];
	IL[e] = new Probability[K+1];
	for ( k = 0; k <= K; ++k ) {
	    v[e][k] = new double [MAX_PHASES+1];
	    s[e][k] = new double [P+1];
	    W[e][k] = new double [MAX_PHASES+1];
	    IL[e][k] = 0;

	    unsigned p;
	    for ( p = 0; p <= P; ++p ) {
		s[e][k][p] = 0.0;
	    }
	    for ( p = 0; p <= MAX_PHASES; ++p ) {
		v[e][k][p] = 0.0;
		W[e][k][p] = 0.0;
	    }
	}
    }

    /* v[0] is used to cache totals */

    v[0] = new double * [K+1];
    for ( k = 0; k <= K; ++k ) {
	v[0][k] = new double [MAX_PHASES+1];
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    v[0][k][p] = 0.0;
	}
    }
}


/*
 * Free storage.
 */

Server::~Server()
{
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 0; k <= K; ++k ) {
	    delete [] W[e][k];
	    delete [] v[e][k];
	    delete [] s[e][k];
	}
	delete [] W[e];
	delete [] v[e];
	delete [] s[e];
	delete [] IL[e];
    }
    for ( unsigned k = 0; k <= K; ++k ) {
	delete [] v[0][k];
    }
    delete [] v[0];
    delete [] W;
    delete [] v;
    delete [] s;
    delete [] IL;
}



/*
 * This method is called before the MVA step for a given population.
 * Some server types need initialization.  By default, no operation.
 */

void
Server::initStep( const MVA& )
{
}



/*
 * Clear all visit ratios for this entry.
 */

void
Server::clear()
{
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 0; k <= K; ++k ) {
	    for ( unsigned p = 0; p <= P; ++p ) {
		s[e][k][p] = 0.0;
	    }
	    for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
		v[e][k][p] = 0.0;
		W[e][k][p] = 0.0;
	    }
	}
    }

    for ( unsigned k = 0; k <= K; ++k ) {
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    v[0][k][p] = 0.0;
	}
    }
}



/*
 * Store phase service time.  Store total service time.
 */

Server&
Server::setService( const unsigned e, const unsigned k, const unsigned p, const double value )
{
    assert( k <= K && 0 < e && e <= E );
    setAndTotal( s[e][k], p, value );
    return *this;
}



/*
 * Store phase visits.  Store total visits.
 */

Server&
Server::setVisits( const unsigned e, const unsigned k, const unsigned p, const double value )
{
    assert( k <= K && 0 < e && e <= E && 0 < p && p <= MAX_PHASES && value >= 0.0);
    v[e][k][p] = value;
    totalVisits( e, k );
    return *this;
}



/*
 * Add more phase visits.  Phase is by CALLING task.
 */

Server&
Server::addVisits( const unsigned e, const unsigned k, const unsigned p, const double value )
{
    assert( k <= K && 0 < e && e <= E && 0 < p && p <= MAX_PHASES && value >= 0.0 );
    v[e][k][p] += value;
    totalVisits( e, k );
    return *this;
}



/*
 * Store variance.  Implemented by subclasses.
 */

Server&
Server::setVariance( const unsigned, const unsigned, const unsigned, const double  )
{
    throw subclass_responsibility( "Server::setVariance", __FILE__, __LINE__ );
    return *this;
}



/*
 * Set client cahin.  Implemented by subclasses.
 */

Server&
Server::setClientChain( const unsigned, const unsigned )
{
    throw subclass_responsibility( "Server::setClientChain", __FILE__, __LINE__ );
    return *this;
}



/*
 * Return overtaking probability array for entry `e' for fancy phase 2
 * server.  Implemented by subclasses.
 */

Probability ***
Server::getPrOt( const unsigned ) const
{
    throw subclass_responsibility( "Server::prOt", __FILE__, __LINE__  );
    return 0;
}


/*
 * Set interlocked flow component.
 */

Server&
Server::setInterlock( const unsigned e, const unsigned k, const Probability& flow )
{
    assert( 0 < e && e <= E && k <= K );

    IL[e][k] = flow;

    return *this;
}



/*
 * Mean service time.
 */

double
Server::S() const
{
    double sumOfV = 0.0;
    double sumOfS = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	for ( unsigned e = 1; e <= E; ++e ) {
	    sumOfS += V(e,k) * S(e,k);
	    sumOfV += V(e,k);
	}
    }
	
    return sumOfV ? sumOfS / sumOfV : 0.0;
}


/*
 * Mean service time (adjusted by throughput).
 */

double
Server::S( const MVA& solver, const Population& N ) const
{
    double sumOfV    = 0.0;
    double sumOfS    = 0.0;
    const unsigned n = solver.offset(N);					/* Hoist */
	
    for ( unsigned k = 1; k <= K; ++k ) {
	const double visits = V(k);
	if ( visits == 0.0 ) continue;
	const double tput = solver.X[n][k];
	sumOfV += visits * tput;
	for ( unsigned e = 1; e <= E; ++e ) {
	    sumOfS += V(e,k) * tput * S(e,k);
	}
    }
	
    if ( !std::isfinite( sumOfV ) ) {							/* BUG_26 */
	return 0;
    } else if ( sumOfV > 0.0 ) {
	return sumOfS / sumOfV;
    } else {
	return S();
    }
}



/*
 * Mean service time for chain k.
 */

double
Server::S( const unsigned k ) const
{
    const double visits = V(k);
	
    if ( visits == 0.0 ) return 0.0;
	
    double sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	sum += V(e,k) * S(e,k);
    }
	
    return sum / visits;
}



/*
 * Mean phase 2 (and above) service time, not including open chain.
 */

double
Server::S_2() const
{
    double sumOfV = 0.0;
    double sumOfS = 0.0;
    for ( unsigned k = 1; k <= K; ++k ) {
	for ( unsigned e = 1; e <= E; ++e ) {
	    sumOfS += V(e,k) * S_2(e,k);
	    sumOfV += V(e,k);
	}
    }
	
    return sumOfV ? sumOfS / sumOfV : 0.0;
}


double
Server::S_2( const unsigned k ) const
{
    const double visits = V(k);
	
    if ( visits == 0.0 ) return 0.0;
	
    double sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	sum += V(e,k) * S_2(e,k);
    }
	
    return sum / visits;
}

/*
 * Return ratio of visits.
 */

double
Server::eta( const unsigned e, const unsigned k ) const
{
    const double visits = V(k);

    return visits ? (V(e,k) / visits) : 0.0;
}



/*
 * Return eta * S for e, k.
 */

double
Server::etaS( const unsigned e, const unsigned k ) const
{
    assert( k <= K && 0 < e && e <= E );
    return S(e,k) * eta(e,k);
}



/*
 * Schmidt multiserver.  Overriden by Schmidt multiserver.  Not used
 * otherwise.
 */

double
Server::muS( const Population&, const unsigned ) const
{
    throw should_not_implement( "Server::muS", __FILE__, __LINE__ );
    return 0.0;
}


/*
 * Common expression for setting various data structures.
 */

void
Server::setAndTotal( double *item, const unsigned phase, const double value )
{
    //assert( value >= 0.0 &&  0 < phase && phase <= P );
    assert( /*value >= 0.0 && */ 0 < phase && phase <= P );
	
    item[phase] = value;

    item[0] = 0.0;
    for ( unsigned p = 1; p <= P; ++p ) {
	item[0] += item[p];
    }
}

/*
 * Total visits over phase/entries for use elsewhere.
 */

void
Server::totalVisits( const unsigned e, const unsigned k )
{
    v[e][k][0] = 0.0;
    v[0][k][0] = 0.0;
    for ( unsigned pp = 1; pp <= MAX_PHASES; ++pp ) {
	v[e][k][0] += v[e][k][pp];
    }
    for ( unsigned ee = 1; ee <= E; ++ee ) {
	v[0][k][0] += v[ee][k][0];
    }
}



/*
 * Return the total visits to the station, not including
 * open classes.
 */

double
Server::V() const
{
    double sumOfV = 0.0;

    for ( unsigned k = 1; k <= K; ++k ) {
	sumOfV += V(k);
    }
    return sumOfV;
}



/*
 * Return the residence time for this server, not including
 * open classes.
 */

double
Server::R() const
{
    double sumOfR = 0.0;

    for ( unsigned k = 1; k <= K; ++k ) {
	sumOfR += R(k);
    }

    return sumOfR;
}



/*
 * Return residence time for class `k' for this server regardless of
 * entry.
 */

double
Server::R( const unsigned k ) const
{
    double sum = 0.0;

    for ( unsigned e = 1; e <= E; ++e ) {
	sum += R(e,k);
    }

    return sum;
}



/*
 * Return residence time for entry `e', class `k'.  Note that for open
 * models, the 'v' values are the relative throughputs, so don't include
 * them in the computation.
 */

double
Server::R( const unsigned e, const unsigned k ) const
{
    assert( k <= K && 0 < e && e <= E );
    if ( k > 0 ) {
	return V(e,k) * W[e][k][0];
    } else {
	return W[e][0][0];
    }
}




/*
 * Return residence time for entry `e', class `k'.  Note that for open
 * models, the 'v' values are the relative throughputs, so don't include
 * them in the computation.
 */

double
Server::R( const unsigned e, const unsigned k, const unsigned p ) const
{
    assert( k <= K && 0 < e && e <= E );
    if ( k > 0 ) {
	return V(e,k,p) * W[e][k][p];
    } else {
	return W[e][0][0];
    }
}



/*
 * Queue length for Mixed models.
 */

void
Server::mixedWait( const MVA& solver, const Population& N ) const
{
    const Positive queue = 1.0 + solver.queueLength( closedIndex, N );

    openWait();			/* Open queue lengths 	*/

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,0) ) continue;
	W[e][0][0] *= queue;	/* Now scale-em 	*/
    }
}



/*
 * rho, [lavenberg] eqn. 3.280
 */

Probability
Server::rho() const
{
    Probability u( Rho() / mu() );
    return u;
}	


/*
 * Rho~, [lavenberg] eqn. (3.284).
 */

double
Server::Rho() const
{
    double sum = 0.0;
    for ( unsigned e = 1; e <= E; ++e ) {
	sum += V(e,0) * S(e,0);
    }
    return sum;
}



/*
 * Alpha correction for converting mixed to closed network
 * [lavenberg] eqn (3.321).
 */

double
Server::alpha( const unsigned n ) const
{
    const double u = rho();
    const double den = power( 1.0 - u, n + 1 );
    if ( den == 0.0 ) {
	throw std::range_error( "Server::alpha" );
    }
    return 1.0 / den;
}



double
Server::priorityInflation( const MVA& solver, const Population &N, const unsigned k ) const
{
    return 1.0 / ( 1.0 - solver.priorityInflation( *this, N, k ) );
}

/*
 * Scale all closed service times by alpha.
 */

Server&
Server::operator*=( const double alpha )
{
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 1; k <= K; ++k ) {
	    for ( unsigned p = 0; p <= P; ++p ) {
		s[e][k][p] *= alpha;
	    }
	}
    }
    return *this;
}



/*
 * Scale all closed service times by alpha.
 */

Server&
Server::operator/=( const double alpha )
{
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 1; k <= K; ++k ) {
	    for ( unsigned p = 0; p <= P; ++p ) {
		s[e][k][p] /= alpha;
	    }
	}
    }
    return *this;
}



/*
 * Set all closed service times to alpha.
 */

Server&
Server::operator=( const double alpha )
{
    for ( unsigned e = 1; e <= E; ++e ) {
	/* Set service times for closed classes */
	for ( unsigned k = 1; k <= K; ++k ) {
	    for ( unsigned p = 0; p <= P; ++p ) {
		if ( v[e][k][p] ) {
		    s[e][k][p] = alpha;
		}
	    }
	}
	/* Set waits for open classes */
	for ( unsigned p = 0; p <= P; ++p ) {
	    W[e][0][p] = alpha;
	}
    }
    return *this;
}



/*
 * Print information about this station.
 */

std::ostream&
Server::print( std::ostream& output ) const
{
    output << typeStr();
    printHeading( output ) << ":" << std::endl;
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 0; k <= K; ++k ) {
	    if ( S(e,k) == 0.0 && V(e,k) == 0.0 ) continue;
	    printInput( output, e, k );
	}
    }
    return output;
}



/*
 * Print out data for this entry and class.
 */

std::ostream&
Server::printInput( std::ostream& output, const unsigned e, const unsigned k ) const
{
    unsigned maxP = 1;
    for ( unsigned int p = 1; p <= MAX_PHASES; ++p ) {
	if ( V(e,k,p) > 0 ) {
	    maxP = p;
	}
    }
    output << "  V(e=" << e << ",k=" << k;
    if ( maxP > 1 ) {
	output << ",p_i";
    }
    output << ") = ";
    for ( unsigned int p = 1; p <= maxP; ++p ) {
	if ( p > 1 ) output << ", ";
	output << V(e,k,p);
    }
    output << std::endl;

    output << "  S(e=" << e << ",k=" << k;
    if ( P > 1 ) {
	output << ",p_j";
    }
    output << ") = ";
    for ( unsigned int p = 1; p <= P; ++p ) {
	if ( p > 1 ) output << ", ";
	output << S(e,k,p);
    }
    output << std::endl;

    return output;
}



/* --------------------- Generic Infinite Server ---------------------- */

/*
 * Capacity function.	For a delay server, the capacity is infinite.
 * We are assuming IEEE arithmetic here.
 */

double
Infinite_Server::mu() const
{
    return std::numeric_limits<double>::infinity();
}



/*
 * Waiting time expression for an infinite server with phases.
 */

void
Infinite_Server::wait( const MVA&, const unsigned k, const Population & ) const
{
    assert( k <= K );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1);						/* Eq:2 */
	}
    }
}



/*
 * Queue length for Open models -- same as open server (for now).
 * One never waits at an infinite server.  :-)
 */

void
Infinite_Server::openWait() const
{
    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,0) ) continue;

	W[e][0][0] = S(e,0);
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    W[e][0][p] = S(e,0,1);						/* Eq:2 */
	}
    }
}

/*
 * Queue length for Open models -- same as open server (for now).
 */

void
Infinite_Server::mixedWait( const MVA& , const Population& ) const
{
    openWait();
}


/*
 * Alpha conversion for delay servers.
 */

double
Infinite_Server::alpha( const unsigned ) const
{
    return exp( Rho() );
}

/* ----------------------------- Client ------------------------------- */


/*
 * Waiting time expression for an infinite server with NO phases.
 */

void
Client::wait( const MVA&, const unsigned k, const Population & ) const
{
    assert( k <= K );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	W[e][k][0] = S(e,k);							/* Eq:2 */

	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k,1);						/* Eq:2 */
	}
    }
}

/* -------------------- Processor Sharing Server ---------------------- */

/*
 * Waiting time expression for PS server.  No phases and one entry.
 * Same results as PF FIFO...
 */
void
PS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );
	
    const Positive sum = 1.0 + solver.sumOf_L_m( *this, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) * sum;						/* Eq:1 */
	}
    }
}



/*
 * Queue length for Open models (M/M/1) (PS,LCFSPR,PS - all servers same
 * service time) [lavenberg] eqn. (3.55)
 */

void
PS_Server::openWait() const
{
    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,0) ) continue;
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][0][p] = S(e,0) / ( 1.0 - rho() );
	}
    }
}

/* ------------------- PS Server with priorities. ------------------- */

/*
 * Waiting time expression for PS server.  No phases and one entry.
 */

void
PR_PS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    PS_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}



/*
 * Queue length for Open models (M/M/1) (PS,LCFSPR,PS - all servers same
 * service time) [lavenberg] eqn. (3.55)
 */

void
PR_PS_Server::openWait() const
{
    throw not_implemented( "Prio_PS_Server::openWait", __FILE__, __LINE__  );
}

/* ------------------- PS Server with priorities. ------------------- */

/*
 * Waiting time expression for PS server.  No phases and one entry.
 */

void
HOL_PS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = (solver.sumOf_SL_m( *this, N, k ) + solver.sumOf_SU_m( *this, N, k ))
		/ ( 1.0 - solver.priorityInflation( *this, N, k ) );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) + sum;						/* Eq:1 */
	}
    }
}



/*
 * Queue length for Open models (M/M/1) (PS,LCFSPR,PS - all servers same
 * service time) [lavenberg] eqn. (3.55)
 */

void
HOL_PS_Server::openWait() const
{
    throw not_implemented( "Prio_PS_Server::openWait", __FILE__, __LINE__  );
}

/* ---------------------- Generic FIFO Server -------------------------	*/

/*
 * Waiting time expression for FCFS server.  No phases and one entry.
 */

void
FCFS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = solver.sumOf_SL_m( *this, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) + sum;						/* Eq:1 */
	}

    }
}


/*
 * Queue length for Open models (M/M/1) (PS,LCFSPR,FCFS - all servers same
 * service time) [lavenberg] eqn. (3.55)
 */

void
FCFS_Server::openWait() const
{
    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,0) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][0][p] = S(0) / ( 1.0 - rho() );
	}
    }
}

/* ------------------- FCFS Server with priorities. ------------------- */

/*
 * Waiting time expression for FCFS server.  No phases and one entry.
 */

void
PR_FCFS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    FCFS_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}



/*
 * Queue length for Open models (M/M/1) (PS,LCFSPR,FCFS - all servers same
 * service time) [lavenberg] eqn. (3.55)
 */

void
PR_FCFS_Server::openWait() const
{
    throw not_implemented( "Prio_FCFS_Server::openWait", __FILE__, __LINE__  );
}

/* ------------------- FCFS Server with priorities. ------------------- */

/*
 * Waiting time expression for FCFS server.  No phases and one entry.
 */

void
HOL_FCFS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const Positive sum = (solver.sumOf_SL_m( *this, N, k ) + solver.sumOf_SU_m( *this, N, k ))
		/ ( 1.0 - solver.priorityInflation( *this, N, k ) );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) + sum;						/* Eq:1 */
	}
    }
}



/*
 * Queue length for Open models (M/M/1) (PS,LCFSPR,FCFS - all servers same
 * service time) [lavenberg] eqn. (3.55)
 */

void
HOL_FCFS_Server::openWait() const
{
    throw not_implemented( "Prio_FCFS_Server::openWait", __FILE__, __LINE__  );
}

/* ------------------- High Variation FIFO Server --------------------- */

/*
 * Free storage
 */

HVFCFS_Server::~HVFCFS_Server()
{
    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 0; k <= K; ++k ) {
	    delete [] myVariance[e][k];
	}
	delete [] myVariance[e];
    }
    delete [] myVariance;
}



/*
 * Allocate memory for variance.
 */

void
HVFCFS_Server::initialize()
{
    myVariance = new double ** [E+1];
    myVariance[0] = 0;
    for ( unsigned e = 1; e <= E; ++e ) {
		
	myVariance[e] = new double *[K+1];
	for ( unsigned k = 0; k <= K; ++k ) {
	    myVariance[e][k] = new double [P+1];

	    for ( unsigned p = 0; p <= P; ++p ) {
		myVariance[e][k][p] = 0.0;
	    }
	}
    }
}



/*
 * Allocate memory for variance.
 */

void
HVFCFS_Server::clear()
{
    Server::clear();

    for ( unsigned e = 1; e <= E; ++e ) {
	for ( unsigned k = 0; k <= K; ++k ) {
	    for ( unsigned p = 0; p <= P; ++p ) {
		myVariance[e][k][p] = 0.0;
	    }
	}
    }
}



/*
 * Set variance...
 */

Server&
HVFCFS_Server::setVariance( const unsigned e, const unsigned k, const unsigned p, const double value )
{
    assert( k <= K && 0 < e && e <= E && p <= P );

    if ( p == 0 ) {
	myVariance[e][k][0] = value;
    } else {
	setAndTotal( myVariance[e][k], p, value );
    }
    return *this;
}




/*
 * Return the Mean Residual Life for the entry and chain.
 * See [lazow], page 265.
 */

double
HVFCFS_Server::r( const unsigned e, const unsigned k, const unsigned p ) const
{
    const double service = S(e,k,p);

    if ( service == 0.0 ) {
	return 0.0;
    } else if ( !std::isfinite(service) ) {
	return service;
    } else {
	return ( service + myVariance[e][k][p] / service ) / 2.0;
    }
}



/*
 * Waiting time expression for FIFO servers with high variabilty in
 * service times.  See [lazow], page 263.  S_Lm is the sum over all
 * classes of service time multiplied by queue length.  Res_Lm is the sum
 * over all classes of utilization multiplied by the difference of
 * service time and mean residual life.
 */



void
HVFCFS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    const double sum = std::max( solver.sumOf_SQ_m( *this, N, k ) + solver.sumOf_rU_m( *this, N, k ), 0.0 );
 	 
    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) + sum;						// Eq:1
	}
    }
}


/*
 * Service time squared plus variance (for open M/G/1 stations).
 */

Positive
HVFCFS_Server::MG1( const unsigned e ) const
{
    const Positive sum = S(0);

    if ( sum == 0 ) {
	return 0.0;		/* No service time, so no queue. */
    } else {
	return rho() * ( sum + myVariance[e][0][0] / sum ) / (2.0 * (1.0 - rho()));
    }
}



/*
 * Queue length for Open models (M/G/1) (FCFS)
 * [lavenberg] eqn. (3.54)
 */

void
HVFCFS_Server::openWait() const
{
    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,0) ) continue;
	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][0][p] = S(e,0) + MG1(e);
	}
    }
}



/*
 * Print out data for this entry and class.
 */

std::ostream&
HVFCFS_Server::printInput( std::ostream& output, const unsigned e, const unsigned k ) const
{
    Server::printInput( output, e, k );

    unsigned p;
	
    output << "  Var(" << e << ',' << k;
    if ( P > 1 ) {
	output << ",p_j";
    }
    output << ") = ";

    for ( p = 1; p <= P; ++p ) {
	if ( p > 1 ) output << ", ";
	output << myVariance[e][k][p];
    }
    output << std::endl;
    return output;
}

/* ------------------- FCFS Server with priorities. ------------------- */

/*
 * Waiting time expression for FCFS server.  No phases and one entry.
 */

void
PR_HVFCFS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    HVFCFS_Server::wait( solver, k, N );
    const double inflation = priorityInflation( solver, N, k );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] *= inflation;
	}
    }
}

/* -------------- HOL Priority High Variation FIFO Server ------------- */

/*
 * Waiting time expression for FIFO servers with high variabilty in
 * service times.
 */

void
HOL_HVFCFS_Server::wait( const MVA& solver, const unsigned k, const Population& N ) const
{
    assert( 0 < k && k <= K );

    Positive sum = (solver.sumOf_SQ_m( *this, N, k ) + solver.sumOf_rU_m( *this, N, k )
		    + solver.sumOf_SU_m( *this, N, k ))
		/ ( 1.0 - solver.priorityInflation( *this, N, k ) );

    for ( unsigned e = 1; e <= E; ++e ) {
	if ( !V(e,k) ) continue;

	for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	    W[e][k][p] = S(e,k) + sum;
	}
    }
}
