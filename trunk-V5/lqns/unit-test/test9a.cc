/*  -*- c++ -*-
 * $HeadURL$
 *
 * Multiserver test.
 *
 * ------------------------------------------------------------------------
 * $Id$
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
#define	N_STATIONS	3
#define N_ENTRIES	1

#define	V22_EQ_1	0
#define	V22_EQ_2	1
#define	V22_EQ_5	2
#define	V22_EQ_11	3
#define	V22_EQ_20	4
#define	V22_COUNT	5

void
test( PopVector& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned v22_ix )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;
	
    NCust.grow(classes);			/* Population vector.		*/
    Z.grow(classes);			/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);			/* Queue type.  SS/delay.	*/

    NCust[1] = 5;
    NCust[2] = 2;
	
    Q[1] = new Infinite_Server(N_ENTRIES,classes);
    Q[2] = new Infinite_Server(N_ENTRIES,classes);
    Q[3] = new Reiser_Multi_Server(2,N_ENTRIES,classes);

    Q[1]->setService(1,6.0).setVisits(1,1.0);
    Q[2]->setService(2,3.0).setVisits(2,1.0);
    Q[3]->setService(1,1.0).setVisits(1,1.0);

    switch ( v22_ix ) {
    case V22_EQ_1:
	Q[3]->setService(2,1.0).setVisits(2,1.0);
	break;
    case V22_EQ_2:
	Q[3]->setService(2,1.0).setVisits(2,2.0);
	break;
    case V22_EQ_5:
	Q[3]->setService(2,1.0).setVisits(2,5.0);
	break;
    case V22_EQ_11:
	Q[3]->setService(2,1.0).setVisits(2,11.0);
	break;
    case V22_EQ_20:
	Q[3]->setService(2,1.0).setVisits(2,20.0);
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
	{ { 0,0 }, { 0, 4.141, 0 }, { 0, 0,  1.429 }, { 0, 0.8593,  0.571 } },	// Exact
	{ { 0,0 }, { 0, 4.132, 0 }, { 0, 0,  1.431 }, { 0, 0.8683, 0.5689 } },	// Linearizer 
	{ { 0,0 }, { 0, 4.132, 0 }, { 0, 0,  1.431 }, { 0, 0.8683, 0.5689 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 4.098, 0 }, { 0, 0,  1.408 }, { 0, 0.9018, 0.5923 } }	// Bard-Schweitzer
    },{					    
	{ { 0,0 }, { 0, 4.065, 0 }, { 0, 0,  1.089 }, { 0, 0.9352, 0.9105 } },	// Exact
	{ { 0,0 }, { 0, 4.061, 0 }, { 0, 0,  1.103 }, { 0, 0.9386,  0.897 } },	// Linearizer 
	{ { 0,0 }, { 0, 4.061, 0 }, { 0, 0,  1.103 }, { 0, 0.9386,  0.897 } },	// Fast Linearizer 
	{ { 0,0 }, { 0,  4.01, 0 }, { 0, 0,  1.055 }, { 0,   0.99, 0.9451 } }	// Bard-Schweitzer
    },{								   
	{ { 0,0 }, { 0, 3.947, 0 }, { 0, 0, 0.6209 }, { 0,  1.053,  1.379 } },	// Exact
	{ { 0,0 }, { 0, 3.952, 0 }, { 0, 0, 0.6415 }, { 0,  1.048,  1.359 } },	// Linearizer 
	{ { 0,0 }, { 0, 3.952, 0 }, { 0, 0, 0.6415 }, { 0,  1.048,  1.359 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 3.887, 0 }, { 0, 0, 0.5787 }, { 0,  1.113,  1.421 } }	// Bard-Schweitzer
    },{								   
	{ { 0,0 }, { 0, 3.865, 0 }, { 0, 0, 0.3286 }, { 0,  1.135,  1.671 } },	// Exact
	{ { 0,0 }, { 0, 3.867, 0 }, { 0, 0, 0.3396 }, { 0,  1.133,   1.66 } },	// Linearizer 
	{ { 0,0 }, { 0, 3.867, 0 }, { 0, 0, 0.3396 }, { 0,  1.133,   1.66 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 3.814, 0 }, { 0, 0, 0.2978 }, { 0,  1.186,  1.702 } }	// Bard-Schweitzer
    },{								   
	{ { 0,0 }, { 0, 3.825, 0 }, { 0, 0, 0.1917 }, { 0,  1.175,  1.808 } },	// Exact
	{ { 0,0 }, { 0, 3.825, 0 }, { 0, 0, 0.1964 }, { 0,  1.175,  1.804 } },	// Linearizer 
	{ { 0,0 }, { 0, 3.825, 0 }, { 0, 0, 0.1964 }, { 0,  1.175,  1.804 } }, 	// Fast Linearizer 
	{ { 0,0 }, { 0, 3.781, 0 }, { 0, 0, 0.1716 }, { 0,  1.219,  1.828 } }	// Bard-Schweitzer
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

