/*  -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/unit-test/test9b.cc $
 *
 * Multiserver test - vary customers.
 *
 * ------------------------------------------------------------------------
 * $Id: test9b.cc 8841 2009-07-14 14:21:57Z greg $
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
test( PopVector& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned nn )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;
	
    NCust.grow(classes);			/* Population vector.		*/
    Z.grow(classes);			/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);			/* Queue type.  SS/delay.	*/

    Q[1] = new Infinite_Server(N_ENTRIES,classes);

    unsigned n = nn;
	
    if ( n >= 100 ) {
	n %= 100;
	Q[2] = new Rolia_Multi_Server(2,N_ENTRIES,classes);
    } else {
	Q[2] = new Reiser_Multi_Server(2,N_ENTRIES,classes);
    }

    if ( n == 0 ) {
	NCust[1] = 5;
	NCust[2] = 2;
    } else {
	NCust[1] = n / 10;
	NCust[2] = n % 10;
    }
	
    Q[1]->setService(1,6.0).setVisits(1,1.0);
    Q[1]->setService(2,3.0).setVisits(2,1.0);

    Q[2]->setService(1,1.0).setVisits(1,1.0);
    Q[2]->setService(2,1.0).setVisits(2,1.0);
}


void
special_check( ostream&, const MVA&, const unsigned )
{
}



static double goodL[V22_COUNT][4][N_STATIONS+1][N_CLASSES+1] =
{
    {
        { { 0,0 }, { 0,  4.141,  1.429 }, { 0, 0.8593,  0.571 } },	// Exact
        { { 0,0 }, { 0,  4.163,  1.449 }, { 0, 0.8374, 0.5507 } }, 	// Linearizer 
        { { 0,0 }, { 0,  4.163,  1.449 }, { 0, 0.8374, 0.5507 } }, 	// Fast Linearizer 
        { { 0,0 }, { 0,  4.127,  1.424 }, { 0, 0.8734, 0.5759 } }	// Bard-Schweitzer
    },{
        { { 0,0 }, { 0,  4.065,  1.089 }, { 0,  0.9352, 0.9105} },	// Exact
        { { 0,0 }, { 0,  4.084,  1.121 }, { 0,  0.9161, 0.8793} }, 	// Linearizer 
        { { 0,0 }, { 0,  4.084,  1.121 }, { 0,  0.9161, 0.8793} }, 	// Fast Linearizer 
        { { 0,0 }, { 0,  4.029,  1.068 }, { 0,  0.9709, 0.9323} }	// Bard-Schweitzer
    },{
        { { 0,0 }, { 0,  3.947, 0.6209 }, { 0,   1.053,  1.379} },	// Exact
        { { 0,0 }, { 0,  3.959, 0.6496 }, { 0,   1.041,   1.35} }, 	// Linearizer 
        { { 0,0 }, { 0,  3.959, 0.6496 }, { 0,   1.041,   1.35} }, 	// Fast Linearizer 
        { { 0,0 }, { 0,  3.895, 0.5828 }, { 0,   1.105,  1.417} }	// Bard-Schweitzer
    },{
        { { 0,0 }, { 0,  3.865, 0.3286 }, { 0,   1.135,  1.671} },	// Exact
        { { 0,0 }, { 0,  3.868, 0.3404 }, { 0,   1.132,   1.66} }, 	// Linearizer 
        { { 0,0 }, { 0,  3.868, 0.3404 }, { 0,   1.132,   1.66} }, 	// Fast Linearizer 
        { { 0,0 }, { 0,  3.842, 0.3074 }, { 0,   1.158,  1.693} }	// Bard-Schweitzer
    },{
        { { 0,0 }, { 0,  3.825, 0.1917 }, { 0,   1.175,  1.808} },	// Exact
        { { 0,0 }, { 0,  3.826, 0.1973 }, { 0,   1.174,  1.803} }, 	// Linearizer 
        { { 0,0 }, { 0,  3.826, 0.1973 }, { 0,   1.174,  1.803} }, 	// Fast Linearizer 
        { { 0,0 }, { 0,  3.802,  0.176 }, { 0,   1.198,  1.824} }	// Bard-Schweitzer
    }
};

bool
check( const int solverId, const MVA & solver, const unsigned v22_ix )
{
    return 1;
	
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

