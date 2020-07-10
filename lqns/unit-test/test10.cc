/*  -*- c++ -*-
 * $Id: test10.cc 13676 2020-07-10 15:46:20Z greg $
 *
 * Rolia Multiserver test.
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
	Q[2] = new Rolia_Multi_Server(2,N_ENTRIES,classes);

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



static double goodL[V22_COUNT][4][N_STATIONS+1][N_CLASSES+1] =
{
	{
        { { 0,0,0 }, { 0,   4.19,  1.458 }, { 0, 0.8097, 0.5421 } },	// Exact
        { { 0,0,0 }, { 0,  4.198,  1.464 }, { 0, 0.8015, 0.5364 } }, 	// Linearizer 
        { { 0,0,0 }, { 0,  4.198,  1.464 }, { 0, 0.8015, 0.5364 } }, 	// Fast Linearize
        { { 0,0,0 }, { 0,  4.189,  1.457 }, { 0, 0.8108, 0.5435 } }	// Bard-Schweitzer
	},{
        { { 0,0,0 }, { 0,  4.091,   1.12 }, { 0,  0.909, 0.8803 } },	// Exact
        { { 0,0,0 }, { 0,  4.109,  1.138 }, { 0, 0.8906, 0.8621 } }, 	// Linearizer 
        { { 0,0,0 }, { 0,  4.109,  1.138 }, { 0, 0.8906, 0.8621 } }, 	// Fast Linearizer 
        { { 0,0,0 }, { 0,  4.091,  1.116 }, { 0,  0.909, 0.8836 } }	// Bard-Schweitzer
	},{
        { { 0,0,0 }, { 0,  3.887, 0.6274 }, { 0,  1.113,  1.373 } },	// Exact
        { { 0,0,0 }, { 0,  3.914, 0.6696 }, { 0,  1.086,   1.33 } }, 	// Linearizer 
        { { 0,0,0 }, { 0,  3.914, 0.6696 }, { 0,  1.086,   1.33 } }, 	// Fast Linearizer 
        { { 0,0,0 }, { 0,  3.901, 0.6292 }, { 0,  1.099,  1.371 } }	// Bard-Schweitzer
	},{
        { { 0,0,0 }, { 0,  3.745, 0.3184 }, { 0,  1.255,  1.682 } },	// Exact
        { { 0,0,0 }, { 0,  3.745, 0.3603 }, { 0,  1.255,   1.64 } }, 	// Linearizer 
        { { 0,0,0 }, { 0,  3.745, 0.3603 }, { 0,  1.255,   1.64 } }, 	// Fast Linearizer 
        { { 0,0,0 }, { 0,  3.767, 0.3279 }, { 0,  1.233,  1.672 } }	// Bard-Schweitzer
	},{
        { { 0,0,0 }, { 0,  3.687, 0.1798 }, { 0,  1.313,   1.82 } },	// Exact
        { { 0,0,0 }, { 0,  3.658, 0.2108 }, { 0,  1.342,  1.789 } }, 	// Linearizer 
        { { 0,0,0 }, { 0,  3.658, 0.2108 }, { 0,  1.342,  1.789 } }, 	// Fast Linearizer 
        { { 0,0,0 }, { 0,  3.703, 0.1899 }, { 0,  1.297,   1.81 } }	// Bard-Schweitzer
	}
};

bool
check( const int solverId, const MVA & solver, const unsigned v22_ix )
{
	bool ok = true;

	unsigned n = solver.offset(solver.NCust);
	for ( unsigned m = 1; m <= solver.M; ++m ) {
		for ( unsigned k = 1; k <= solver.K; ++k ) {
			if ( fabs( solver.L[n][m][1][k] - goodL[v22_ix][solverId][m][k] ) >= 0.001 ) {
				cerr << "Mismatch at m=" << m <<", k=" << k;
				cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[v22_ix][solverId][m][k] << endl;
				ok = false;
			}
		}
	}
	return ok;
}

