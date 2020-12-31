/*  -*- c++ -*-
 * $Id: variance.cc 14308 2020-12-31 16:00:47Z greg $
 *
 * Variance calculations.  Pick and choose as desired.
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
#include <mva/prob.h>
#include "variance.h"
#include "call.h"
#include "entry.h"
#include "entity.h"
#include "lqns.h"

/*
 * Return variance.
 */

double
Variance::variance() const
{
    const Positive variance = mean_sqr - square( mean );
    return variance;
}

/*
 * Perform hyperexponential calculation for one branch of the
 * `SP' Model [pg 125].  There are 2 branches.
 */

#define N_BRANCHES	2

void
SeriesParallel::addStage( const Probability& pi_e, const double S_e, const Positive& Cv_sqr )
{
    unsigned n_branches;		/* Number of branches for stage */
    Probability pi[N_BRANCHES+1];	/* Branch probability.		*/
    double S[N_BRANCHES+1];		/* Service of a stage on branch	*/
    double r[N_BRANCHES+1];		/* Number of stages on branch.	*/

    /* Set up branches -- ordered most likely first. */

    if ( Cv_sqr > (1.0 + EPSILON) ) {

	/* Hyper exponential type event */

	n_branches = 2;
	const double a = (1.0 - sqrt( (Cv_sqr - 1.0) / (Cv_sqr + 1.0) )) / 2.0;
	pi[1] = pi_e * a;
	pi[2] = pi_e * (1.0 - a);
	r[1]  = 1.0;
	r[2]  = 1.0;
	S[1]  = a ? S_e / (2.0 * a) : 0.0;
	S[2]  = S_e / (2.0 * (1.0 - a));

    } else if ( Cv_sqr > (1.0 - EPSILON) ) {

	/* Exponential type event */

	n_branches = 1;
	pi[1] = pi_e;
	r[1]  = 1.0;
	S[1]  = S_e;

    } else if ( Cv_sqr > EPSILON ) {

	/* Erlang type event */

	n_branches = 1;
	pi[1] = pi_e;
	r[1]  = floor( 1.0 / Cv_sqr + 0.5 );
	S[1]  = S_e;

    } else {

	/* Deterministic phase (or Erlang with 1e6 or more stages) - truncate at 1e6. */

	n_branches = 1;
	pi[1] = pi_e;
	r[1]  = 1e6;
	S[1]  = S_e;
    }
		
    /* Compute for this branch... */

    for ( unsigned b = 1; b <= n_branches; ++b ) {
	mean_sqr += pi[b] * square(S[b]) * (r[b]+1) / r[b];
	mean     += pi[b] * S[b];
    }
}


/*
 * General case: create a series parallel model and solve.
 */
	
double
SeriesParallel::totalVariance( const Entity & anEntity )
{
    SeriesParallel sum;
	
    for ( std::vector<Entry *>::const_iterator entry = anEntity.entries().begin(); entry != anEntity.entries().end(); ++entry ) {
	sum.addStage( (*entry)->prVisit(), (*entry)->elapsedTime(), (*entry)->computeCV_sqr() );
    }
    return sum.variance();
}

void
OrVariance::addStage( const Probability& pi_e, const double S_e, const Positive& Cv_sqr )
{
    double s_sqr = square( S_e );
    double variance = sqrt( Cv_sqr * s_sqr );

    mean_sqr += pi_e * ( s_sqr + variance );
    mean     += pi_e * S_e;
}


/*
 * Variance over a set of choices.  
 * Smith, 178.
 */
	
double
OrVariance::totalVariance( const Entity & anEntity )
{
    OrVariance sum;

    const std::vector<Entry *>& entries = anEntity.entries();
    for ( std::vector<Entry *>::const_iterator entry_1 = entries.begin(); entry_1 != entries.end(); ++entry_1 ) {
	sum.addStage( (*entry_1)->prVisit(), (*entry_1)->elapsedTime(), (*entry_1)->computeCV_sqr() );

	for ( std::vector<Entry *>::const_iterator entry_2 = entries.begin(); entry_2 != entries.end(); ++entry_2 ) {
	    Probability pr = (*entry_1)->prVisit() * (*entry_2)->prVisit();
	    sum.mean_sqr += ((*entry_1)->elapsedTime() - (*entry_2)->elapsedTime()) * pr;
	}
    }
    return sum.variance();
}
