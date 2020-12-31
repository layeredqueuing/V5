/*  -*- c++ -*-
 * $Id: test8.cc 13676 2020-07-10 15:46:20Z greg $
 *
 * Conway Multiserver test.
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include <stdlib.h>
#include "testmva.h"
#include "server.h"
#include "multserv.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	2
#define	N_STATIONS	2
#define N_ENTRIES	1

#define	V22_EQ_1	0
#define	V22_EQ_2	1
#define	V22_EQ_5	2
#define	V22_EQ_11	3
#define	V22_EQ_20	4
#define	V22_COUNT	5

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned v22_ix )
{
	const unsigned classes  = N_CLASSES;
	const unsigned stations = N_STATIONS;
	
	NCust.resize(classes);			/* Population vector.		*/
	Z.resize(classes);			/* Think times.			*/
	priority.resize(classes);
	Q.resize(stations);			/* Queue type.  SS/delay.	*/

	NCust[1] = 5;
	NCust[2] = 2;
	
	Q[1] = new Infinite_Server(N_ENTRIES,classes);
	Q[2] = new Conway_Multi_Server(2,N_ENTRIES,classes);

	Q[1]->setService(1,6.0).setVisits(1,1.0);
	Q[1]->setService(2,3.0).setVisits(2,1.0);
	Q[2]->setService(1,1.0).setVisits(1,1.0);

	switch ( v22_ix ) {
	case V22_EQ_1:
		Q[2]->setService(2,1.0).setVisits(2,1.0);
		break;
	case V22_EQ_2:
		Q[2]->setService(2,1.0).setVisits(2,2.0);
		break;
	case V22_EQ_5:
		Q[2]->setService(2,1.0).setVisits(2,5.0);
		break;
	case V22_EQ_11:
		Q[2]->setService(2,1.0).setVisits(2,11.0);
		break;
	case V22_EQ_20:
		Q[2]->setService(2,1.0).setVisits(2,20.0);
		break;
	default:
		cerr << "Invalid S22 index (0-4):" << v22_ix << endl;
		exit( 1 );
	}
}


void
special_check( ostream&, const MVA&, const unsigned )
{
}



static const double goodL[V22_COUNT][4][N_STATIONS+1][N_CLASSES+1] =
{
	{
        { { 0,0,0 }, { 0, 4.141,  1.429 }, { 0, 0.8593,  0.571 } },	// Exact
        { { 0,0,0 }, { 0, 4.121,  1.407 }, { 0, 0.8791,  0.593 } }, 	// Linearizer 
        { { 0,0,0 }, { 0, 4.121,  1.407 }, { 0, 0.8791,  0.593 } }, 	// Fast Linearize
        { { 0,0,0 }, { 0, 4.113,  1.401 }, { 0, 0.8875, 0.5988 } }	// Bard-Schweitzer
	},{
        { { 0,0,0 }, { 0, 4.065,  1.089 }, { 0, 0.9352, 0.9105 } },	// Exact
        { { 0,0,0 }, { 0, 4.009,  1.025 }, { 0, 0.9913, 0.9746 } }, 	// Linearizer 
        { { 0,0,0 }, { 0, 4.009,  1.025 }, { 0, 0.9913, 0.9746 } }, 	// Fast Linearizer 
        { { 0,0,0 }, { 0, 3.993,  1.011 }, { 0,  1.007, 0.9891 } }	// Bard-Schweitzer
	},{
        { { 0,0,0 }, { 0, 3.947, 0.6209 }, { 0,  1.053,  1.379 } },	// Exact
        { { 0,0,0 }, { 0, 3.862, 0.5377 }, { 0,  1.138,  1.462 } }, 	// Linearizer 
        { { 0,0,0 }, { 0, 3.862, 0.5377 }, { 0,  1.138,  1.462 } }, 	// Fast Linearizer 
        { { 0,0,0 }, { 0, 3.846, 0.5216 }, { 0,  1.154,  1.478 } }	// Bard-Schweitzer
	},{
        { { 0,0,0 }, { 0, 3.865, 0.3286 }, { 0,  1.135,  1.671 } },	// Exact
        { { 0,0,0 }, { 0, 3.784, 0.2702 }, { 0,  1.216,   1.73 } }, 	// Linearizer 
        { { 0,0,0 }, { 0, 3.784, 0.2702 }, { 0,  1.216,   1.73 } }, 	// Fast Linearizer 
        { { 0,0,0 }, { 0,  3.77, 0.2605 }, { 0,   1.23,  1.739 } }	// Bard-Schweitzer
	},{
        { { 0,0,0 }, { 0, 3.825, 0.1917 }, { 0,  1.175,  1.808 } },	// Exact
        { { 0,0,0 }, { 0, 3.751, 0.1541 }, { 0,  1.249,  1.846 } }, 	// Linearizer 
        { { 0,0,0 }, { 0, 3.751, 0.1541 }, { 0,  1.249,  1.846 } }, 	// Fast Linearizer 
        { { 0,0,0 }, { 0, 3.739, 0.1484 }, { 0,  1.261,  1.852 } }	// Bard-Schweitzer
	}
};

bool
check( const int solverId, const MVA & solver, const unsigned v22_ix )
{
	bool ok = true;

	unsigned n = solver.offset(solver.NCust);
	for ( unsigned m = 1; m <= solver.M; ++m ) {
		for ( unsigned k = 1; k <= solver.K; ++k ) {
			unsigned e;

			if ( m == 5 ) {
				e = k;
			} else {
				e = 1;
			}
			if ( fabs( solver.L[n][m][e][k] - goodL[v22_ix][solverId][m][k] ) >= 0.001 ) {
				cerr << "Mismatch at m=" << m <<", k=" << k;
				cerr << ".  Computed=" << solver.L[n][m][e][k] << ", Correct= " << goodL[v22_ix][solverId][m][k] << endl;
				ok = false;
			}
		}
	}
	return ok;
}

