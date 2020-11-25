/* overtake.C	-- Greg Franks Mon Mar 17 1997
 * $Id: overtake.cc 14140 2020-11-25 20:24:15Z greg $
 * 
 * Overtaking calculation.  See also slice.[Ch].
 * See
 *  author =       "Greg Franks",
 *  title =        "Performance Analysis of Client-Server Systems with
 *                  Aggressive Server Replies",
 *  note =         "In progress",
 *  year =         1995,
 *  month =        oct
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * ----------------------------------------------------------------------
 */


#include "dim.h"
#include <algorithm>
#include "overtake.h"
#include "entry.h"
#include "task.h"
#include "lqns.h"
#include "server.h"
#include <lqio/srvn_output.h>

/*
 * Initialize.
 */

Overtaking::Overtaking( const Task* client, const Entity* server )
    : _client( client ), _server( server )
{
    ij_info.initialize( client, server );
}



/*
 * Clear unused entries in tables in case the number of phases of Entry D does not
 * match the number of phases of entry B.
 */

void
Overtaking::clearOvertakingStates( const unsigned max_phases, Probability PrOTState[MAX_PHASES+1][MAX_PHASES+1][2] )
{
    for ( unsigned i = 0; i <= max_phases; ++i ) {
	for ( unsigned r = 1; r <= max_phases; ++r ) {
	    PrOTState[i][r][0] = 0.0;	/* Overtaking.	*/
	    PrOTState[i][r][1] = 0.0;	/* Next j..	*/
	}
    }
}



/*
 * Find overtaking from client to server for this submodel.
 */

void
Overtaking::compute( const PrintHelper * print_func )
{
    for ( std::vector<Entry *>::const_iterator entA = _client->entries().begin(); entA != _client->entries().end(); ++entA ) {
	for ( std::vector<Entry *>::const_iterator entB = _server->entries().begin(); entB != _server->entries().end(); ++entB ) {
	    Slice_Info ab_info[MAX_PHASES+1];
	    double y_aj[MAX_PHASES+1];

	    /* ---------------------- Set up ab_info  --------------------- */
	
	    (*entA)->sliceTime( *(*entB), ab_info, y_aj );		// Set ab_info.

	    if ( y_aj[0] == 0.0 ) break;
	
	    if ( flags.trace_overtaking ) {
		printSlice( std::cout, *(*entA), *(*entB), ab_info );
	    }

	    /* --------------- Set overtaking probabilities. -------------- */

	    for ( std::vector<Entry *>::const_iterator entD = _server->entries().begin(); entD != _server->entries().end(); ++entD ) {
		unsigned j;
		
		/* -------- Solve Markov Chain for overtaking. -------- */
		
		for ( j = 0; j <= (*entD)->maxPhase(); ++j ) {
		    const double x_j = ( j == 0 ) ? 0.0 : (*entD)->elapsedTimeForPhase(j);	// server is NEVER idle!
			
		    for ( unsigned i = 0; i < (*entA)->maxPhase(); ++i ) {
			ab_info[i].setRates( x_j, Probability(1.0) );
		    }
		    ab_info[(*entA)->maxPhase()].setRates( x_j, (*entA)->prVisit() );
			
		    Slice_Info::prOvertakingStates( ab_info, (*entA)->maxPhase(),
						    PrOtState[j] );

		    if ( flags.trace_overtaking ) {
			Slice_Info::printOvertakingStates( std::cout, j, x_j,
							   (*entA)->maxPhase(),
							   PrOtState[j] );
		    }
		}
		for ( j = (*entD)->maxPhase()+1; j <= MAX_PHASES; ++j ) {
		    clearOvertakingStates( (*entA)->maxPhase(), PrOtState[j] );
		}
		
		for ( std::vector<Entry *>::const_iterator entC = _client->entries().begin(); entC != _client->entries().end(); ++entC ) {
		    computeOvertaking( **entA, **entB, **entC, **entD, y_aj, ab_info, print_func );
		}
	    }
	}
    }
}



/*
 * Calculate overtaking for A sending to B conditioned on the last
 * message was sent from C to D.
 *
 * NOTE: totOt is allocated in ph2serv.C.  Beware that NO RANGE checking
 * is performed, so if the k or p indecies don't match up. BOOM!
 */

void
Overtaking::computeOvertaking( const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD,
			       double y_aj[MAX_PHASES+1], Slice_Info ab_info[], const PrintHelper * print_func ) const
{
    unsigned i, j;			/* Phase indecies 	*/
    Slice_Info cd_info[MAX_PHASES+1];
    Probability nextProb[MAX_PHASES+1];
    double y_cj[MAX_PHASES+1];
    double Y_ij = ij_info.rendezvous();

    /* -------------- Starting Probability. ---------------	*/
	
    entC.sliceTime( entD, cd_info, y_cj );
	
    if ( y_cj[0] == 0 || Y_ij == 0.0 ) {
	return;		/* No calls from c to task j. -- ignore. */
    }

    for ( i = 1; i <= entA.maxPhase(); i++ ) {
	nextProb[i] = cd_info[i].Y_ij() / Y_ij;
    }

    if ( &entA != &entC ) {
	Slice_Info::prStartStates( cd_info, entC, entD, nextProb );
    }
	
    if ( flags.trace_overtaking ) {
	printStart( std::cout, entA, entB, entC, entD, nextProb );
    }

    /* -------------------- overtaking -------------------- */
	
    Probability sum[MAX_PHASES+1];
    for ( i = 1; i <= entA.maxPhase(); i++ ) {
	sum[i] = 0.0;
    }
	
    double y_cd = 0.0;
    for ( i = 1; i <= entC.maxPhase(); ++i ) {
	y_cd += cd_info[i].calls();
    }

    for ( j = 2; j <= entB.maxPhase(); ++j ) {

	Probability prOt[MAX_PHASES+1];		/* Overtaking probabilities.	*/
	Probability prNext[MAX_PHASES+1];

	prOt[0] = 0.0;
		
	for ( i = 1; i <= entA.maxPhase(); i++ ) {
	    prOt[i]   = 0.0;
	    prNext[i] = 0.0;

	    if ( y_aj[i] == 0 ) continue;

	    const double temp = (ab_info[i].calls() / y_aj[i]) * (y_cd / y_cj[0]);
			
	    for ( unsigned r = 1; r <= entA.maxPhase(); ++r ) {
		prOt[i]   += temp * nextProb[r] * PrOtState[j][i][r][0];
		prNext[i] += temp * nextProb[r] * PrOtState[j][i][r][1];
	    }
	}
		
	for ( i = 1; i <= entA.maxPhase(); i++ ) {
	    nextProb[i] = prNext[i];		// DO NOT Combine for loops!

	    if ( !ab_info[i].calls() ) continue;
			
	    /* print_func != 0 if we are printing only. */

	    const double prOtC = prOt[i] * ( y_aj[0] / y_aj[i] ); // Condition!
	    if ( print_func == 0 ) {
		Server * aStation = _server->serverStation();
		Probability *** totOt = aStation->getPrOt(entB.index());
		const Task * srcTask = dynamic_cast<const Task *>(entA.owner());
				
		const ChainVector& aChain = const_cast<Task *>(srcTask)->clientChains(_server->submodel());
		for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
		    const unsigned k = aChain[ix];
		    totOt[k][0][j] += prOt[i];
					
		    /*
		     * Some numerical instability at
		     * start up results in some
		     * probabilities being slightly
		     * too high.  Truncate.
		     */
					
		    const double temp = totOt[k][i][j] + prOtC;
		    if (  1.0 < temp && temp < 1.01 ) {
			totOt[k][i][j] = 1.0;
		    } else {				
			totOt[k][i][j] = temp;
		    }
		}
	    }
				
	    prOt[i] = prOtC;
	    sum[i] += prOtC;
	}

	if ( print_func ) {
	    (*print_func)( entA, entB, entC, entD, j, prOt );
	} else if ( flags.trace_overtaking ) {
	    std::cout << "OT: " << entA.name() << ' ' << entB.name() << ' ' << entC.name() << ' ' << entD.name() << ' ' << j << ' ';
	    for ( i = 1; i <= entA.maxPhase(); ++i ) {
		std::cout << prOt[i] << ' ';
	    }
	    std::cout << std::endl;
	}
    }
}

/*
 * Print overtaking probabilities.  Since the amount of array space
 * is exorbitant, recompute them based on the last solution.
 */

std::ostream&
Overtaking::print( std::ostream& output ) 
{
    const PrintHelper print( output );
    compute( &print );
    return output;
}




/*
 * Print Overtaking probability.
 */

void
Overtaking::PrintHelper::operator()( const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD, const unsigned j, const Probability pr[] ) const
{
    unsigned i;
    double sum = 0.0;
    for ( i = 1; i <= entA.maxPhase(); ++i ) {
	sum += pr[i];
    }
    if ( sum == 0 ) return;
	
    if ( j == 2 ) {
	_output << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << entA.name() << " "
		<< std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << entB.name() << " " 
		<< std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << entC.name() << " "
		<< std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen-1) << entD.name() << " ";
    } else {
	_output << std::setw(LQIO::SRVN::ObjectOutput::__maxStrLen*4) << " ";
    }
    _output << " " << j << "  ";
	
    for ( i = 1; i <= Entry::max_phases; ++i ) {
	_output << std::setw(LQIO::SRVN::ObjectOutput::__maxDblLen-1) << pr[i] << " ";
    }
    _output << "OT" << std::endl;
}



/*
 * Debugging function.
 */

std::ostream&
Overtaking::printSlice( std::ostream& output, const Entry& src, const Entry& dst, const Slice_Info phase_info[] ) const
{
    output << src.name() << " -> " << dst.name() << std::endl;
	
    for ( unsigned p = 1; p <= src.maxPhase(); ++p ) {
	output << "  p=" << p << ": " << phase_info[p];
    }
    return output;
}


/*
 * Print Starting probability.
 */

std::ostream&
Overtaking::printStart( std::ostream& output,
			const Entry& entA, const Entry& entB, const Entry& entC, const Entry& entD,
			const Probability pr[] ) const
{
    output << "Pr{Start( " << entA.name() << ", " << entB.name() << ", " << entC.name() << ", " << entD.name() << " )} = ";
    for ( unsigned i = 1; i <= entA.maxPhase(); ++i ) {
//	output << std::setw( std::ostream::maxDblLen-1 ) << pr[i] << " ";
    }
    output << std::endl;

    return output;
}

/* ------------------------------------------------------------------------ */


/*
 * Grow the array.
 */

Overtaking::ijInfo::ijInfo() :
    _rendezvous( Entry::max_phases )
{
}
	


/*
 * Return total number of rendezvous from the receiver to dst.
 */

void
Overtaking::ijInfo::initialize( const Task * srcTask, const Entity * dstTask )
{
    _rendezvous = 0.0;
    const std::vector<Entry *>& entries = srcTask->entries();
    for_each( entries.begin(), entries.end(), ConstExec2<Entry,const Entity *,VectorMath<double>&>( &Entry::rendezvous, dstTask, _rendezvous ) );
}
