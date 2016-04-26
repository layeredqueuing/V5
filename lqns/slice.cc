/*  -*- c++ -*-
 * $Id: slice.cc 11963 2014-04-10 14:36:42Z greg $
 *
 * Everything you wanted to know about a slice, but were afraid to ask.
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
#include "fpgoop.h"
#include "slice.h"
#include "prob.h"
#include "lqns.h"
#include "entry.h"
#include "activity.h"
#include "call.h"



/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Print out service time of entry in standard output format.
 */

ostream&
operator<<( ostream& output, const Slice_Info& self )
{
    return self.print( output );
}

/* ------------------------ Constructors etc. ------------------------- */

/*
 * Set initial values -- assumes NO calls to other tasks.
 */
 
void
Slice_Info::initialize( const double s )
{
    nSlices = 1.0;
    service = s;
}



/*
 * Set initial values for aPhase.
 */
 
void
Slice_Info::initialize( const Phase& aPhase, const Entry& dst )
{
    nSlices = aPhase.numberOfSlices();
    service = nSlices * aPhase.processorWait();
    if ( nSlices < 1 ) throw logic_error( "Slice_Info::initialize" );

    getCallInfo( 1.0, aPhase.callList(), aPhase.entry()->throughput(), dst );
}



/*
 * Accumulate values for anActivity being called from src.
 */

void
Slice_Info::accumulate( const double multiplier, const Phase& src, const Entry& dst )
{
    const double n = src.numberOfSlices();

    nSlices += n;
    service += n * src.processorWait();

    getCallInfo( multiplier, src.callList(), src.throughput(), dst );
}




/*
 * Extract and accumulate call information.
 */

void
Slice_Info::getCallInfo( const double scale, const Cltn<Call *>& callList, const double throughput, const Entry& dst )
{
    Sequence<Call *> nextCall( callList );
    const Call * aCall;

    while ( aCall = nextCall() ) {
		
	const double y_adp = scale * aCall->rendezvous();
	if ( y_adp == 0 ) continue;
			
	/* Ratio for per phase flows */

	const Entry& entD = *(aCall->dstEntry());
			
	if ( aCall->dstTask() == dst.owner() ) {
	    lambda_ij += throughput * y_adp;
	    y_ij      += y_adp;
	    if ( dst == entD ) {
		y_ab += y_adp;
	    }

	    /* Visits to ``other'' replicas are not overtaking events. */
			
	    if ( aCall->fanOut() > 1 ) {
		const double n_other = aCall->fanOut() - 1.0;
		lambda_ik += throughput * y_adp * n_other;
		y_ik      += y_adp * n_other;
		t_k	  += scale * aCall->rendezvousDelay() * n_other / aCall->fanOut();
	    }
			
	} else {
	    lambda_ik += throughput * y_adp * aCall->fanOut();
	    y_ik      += y_adp * aCall->fanOut();
	    t_k	  += scale * aCall->rendezvousDelay();
	}
    }
}



/*
 * Final normalization step.
 */

double
Slice_Info::normalize()
{
    if ( y_ik > 0.0 ) {
	t_k /= y_ik;
    }
    return y_ij;
}

/* ------------------------ Instance Methods -------------------------- */

/*
 * For markov phased servers.
 * Calculate the rate parameters for Markov chains.
 */

void
Slice_Info::setRates( const double xj, const Probability& prA )
{
    const double y_sum = y_ij + y_ik + 1.0;
    double temp        = xj + t_k;
    const double slice = (nSlices == 0.0 ? 0.0 : service / nSlices);

    Probability q[7];
	
    if ( !isfinite(temp) ) {
	q[0] = 1.0;
	q[3] = 0.0;
    } else if ( temp ) { 
	q[0] = xj  / temp;
	q[3] = t_k / temp;
    } else {
	q[0] = 0.0;
	q[3] = 1.0;
    }
	
    temp = xj + slice;
    if ( !isfinite( temp ) ) {
	q[1] = 0.0;
	q[5] = 1.0;
    } else if ( temp ) {
	q[1] = xj    / temp;
	q[5] = slice / temp;
    } else {
	q[1] = 1.0;
	q[5] = 0.0;
    }
	
    q[2] = y_ik / y_sum;
    q[4] = y_ij / y_sum;
    q[6] = prA  / y_sum;

    /* Smash into more digestible quantities */

    a = q[5] + q[1] * q[2] * q[3];
    b = q[1] * q[6];
    c = q[1] * q[4];
    d = q[0] * q[1] * q[2];

#ifdef	DEBUG
    cout << "x_jp=" << xj << "\t";
    for ( unsigned i = 0; i <=6; ++i ) {
	cout << "\tq[" << i << "]= " << setw(6) <<q[i];
    }
    cout << endl;
#endif
}



/*
 * Print out slice info.
 */

ostream&
Slice_Info::print( ostream& output ) const
{
    int width = output.precision() + 2;

    output << "  slice=" << setw(width) << (nSlices == 0.0 ? 0.0 : service / nSlices)
	   << ", y_ij=" << setw(3) << y_ij
	   << ", y_ab=" << setw(width) << y_ab
	   << ", y_ik=" << setw(3) << y_ik
	   << ", t_k=" << setw(width) << t_k
	   << ", lambda_ij=" << setw(width) << lambda_ij
	   << ", lambda_ik=" << setw(width) << lambda_ik
	   << endl;

    return output;
}

/*
 * Solve overtaking Markov chain.
 */

void
Slice_Info::prOvertakingStates( Slice_Info rate[],						/* In	*/
				const unsigned max_phases,					/* In	*/
				Probability PrOTState[MAX_PHASES+1][MAX_PHASES+1][2])		/* Out	*/
{
    for ( unsigned i = 0; i <= max_phases; ++i ) {
		
	Probability temp = 1.0;
	unsigned r;

	for ( r = 0; r <= max_phases; ++r ) {
	    if ( r == i ) continue;
	    temp *= rate[r].prodOf_b();
	}
	double product = 1.0 / rate[i].denominator( temp );
		
	r = i;
	do { 
	    PrOTState[i][r][0] = rate[i].prOt( product );	/* Overtaking.	*/
	    PrOTState[i][r][1] = rate[i].next( product );	/* Next j..	*/
			
	    r = ( r == 0 ) ? max_phases : r - 1;
	    product *= rate[r].prodOf_b();
	} while ( r != i );
    }
}



/*
 * Find the starting probabilities for the Markov Chain.  entry A != entry C.
 */

void
Slice_Info::prStartStates( Slice_Info rate[], const Entry& entC, const Entry& entD, Probability PrStart[] )
{
    unsigned i;
    double sum[MAX_PHASES+1];
	
    for ( i = 1; i <= entC.maxPhase(); i++ ) {
	sum[i] = 0.0;
    }

    /* Have to further adjust starting probabilities. */

    for ( unsigned j = 2; j <= entD.maxPhase(); ++j ) {
	const double x_j = entD.elapsedTime(j);
			
	for ( i = 0; i < entC.maxPhase(); ++i ) {
	    rate[i].setRates( x_j, Probability(1.0) );
	}
	rate[i].setRates( x_j, entC.prVisit() );
			
	double product = 1.0;
	for ( i = entC.maxPhase(); i > 0; i -= 1 ) {
	    product *= rate[i].prodOf_b();
	    sum[i]  += product;
	}
    }
	
    for ( i = 1; i <= entC.maxPhase(); i++ ) {
	PrStart[i] *= sum[i];
    }
}



/*
 * Debugging...
 */
	
ostream&
Slice_Info::printOvertakingStates(ostream& output, const unsigned j, const double x,
				  const unsigned max_phases,
				  Probability PrOTState[MAX_PHASES+1][MAX_PHASES+1][2]) 
{
    unsigned i;
    unsigned r;
    int width = output.precision() + 2;
	
    output << "x_j(" << j << ")= " << x << endl;

    for ( i = 0; i <= max_phases; ++i ) {
	for ( r = 0; r <= max_phases; ++r ) {
	    output << "  P(" << i << j << "4|" << r << j << "0)="
		   << setw(width) << PrOTState[i][r][0];
	    if ( r != max_phases ) { 
		output << ", ";
	    }
	}
	output << endl;
    }

    for ( i = 0; i <= max_phases; ++i ) {
	for ( r = 0; r <= max_phases; ++r ) {
	    output << "  P(" << i+1 << j << "0|" << r << j << "0)="
		   << setw(width) << PrOTState[i][r][1];
	    if ( r != max_phases ) { 
		output << ", ";
	    } else {
		output << endl;
	    }
	}
    }

    return output;
}
