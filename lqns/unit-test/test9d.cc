/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/test9d.cc $
 *
 * Multiserver test.  Conway / de Souza e Silva and Muntz waiting time.
 *
 * ------------------------------------------------------------------------
 * $Id: test9d.cc 13676 2020-07-10 15:46:20Z greg $
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
#define	S22_EQ_1	5
#define	S22_EQ_2	6
#define	S22_EQ_5	7
#define	S22_EQ_11	8
#define	S22_EQ_20	9
#define	S22_COUNT	10

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned s22_ix )
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

    switch ( s22_ix ) {

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

    case S22_EQ_1:
	Q[2]->setService(2,1.0).setVisits(2,1.0);
	break;
    case S22_EQ_2:
	Q[2]->setService(2,2.0).setVisits(2,1.0);
	break;
    case S22_EQ_5:
	Q[2]->setService(2,5.0).setVisits(2,1.0);
	break;
    case S22_EQ_11:
	Q[2]->setService(2,11.0).setVisits(2,1.0);
	break;
    case S22_EQ_20:
	Q[2]->setService(2,20.0).setVisits(2,1.0);
	break;
		
    default:
	cerr << "Invalid S22 index (0-4):" << s22_ix << endl;
	exit( 1 );
    }
}


void
special_check( ostream&, const MVA&, const unsigned )
{
}



static double goodL[S22_COUNT][4][N_STATIONS+1][N_CLASSES+1] =
{
    /* Vary visits.	 Note that this network is product form. */
	
    { 
	{ { 0,0 }, { 0, 4.141,	1.429 }, { 0, 0.8593,  0.571 } },	// Exact
	{ { 0,0 }, { 0, 4.121,	1.407 }, { 0,  0.879,  0.593 } },	// Linearizer 
	{ { 0,0 }, { 0, 4.121,	1.407 }, { 0,  0.879,  0.593 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 4.113,	1.402 }, { 0, 0.8866, 0.5984 } }	// Bard-Schweitzer
    },{	     
	{ { 0,0 }, { 0, 4.065,	1.089 }, { 0, 0.9352, 0.9105 } },	// Exact
	{ { 0,0 }, { 0, 4.009,	1.025 }, { 0, 0.9912, 0.9745 } },	// Linearizer 
	{ { 0,0 }, { 0, 4.009,	1.025 }, { 0, 0.9912, 0.9745 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 3.993,	1.011 }, { 0,  1.007, 0.9892 } }	// Bard-Schweitzer
    },{	     
	{ { 0,0 }, { 0, 3.947, 0.6209 }, { 0,  1.053,  1.379 } },	// Exact
	{ { 0,0 }, { 0, 3.862, 0.5377 }, { 0,  1.138,  1.462 } },	// Linearizer 
	{ { 0,0 }, { 0, 3.862, 0.5377 }, { 0,  1.138,  1.462 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 3.846, 0.5216 }, { 0,  1.154,  1.478 } }	// Bard-Schweitzer
    },{	     
	{ { 0,0 }, { 0, 3.865, 0.3286 }, { 0,  1.135,  1.671 } },	// Exact
	{ { 0,0 }, { 0, 3.784, 0.2702 }, { 0,  1.216,	1.73 } },	// Linearizer 
	{ { 0,0 }, { 0, 3.784, 0.2702 }, { 0,  1.216,	1.73 } },	// Fast Linearizer 
	{ { 0,0 }, { 0,	 3.77, 0.2606 }, { 0,	1.23,  1.739 } }	// Bard-Schweitzer
    },{	     
	{ { 0,0 }, { 0, 3.825, 0.1917 }, { 0,  1.175,  1.808 } },	// Exact
	{ { 0,0 }, { 0, 3.751, 0.1541 }, { 0,  1.249,  1.846 } },	// Linearizer 
	{ { 0,0 }, { 0, 3.751, 0.1541 }, { 0,  1.249,  1.846 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 3.739, 0.1484 }, { 0,  1.261,  1.852 } }	// Bard-Schweitzer
    },	     
		     
    /* Very service time. */
		     
    {	     
	{ { 0,0 }, { 0, 4.141,	1.429 }, { 0, 0.8593,  0.571 } },	// Exact
	{ { 0,0 }, { 0, 4.121,	1.407 }, { 0,  0.879,  0.593 } },	// Linearizer 
	{ { 0,0 }, { 0, 4.121,	1.407 }, { 0,  0.879,  0.593 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 4.113,	1.402 }, { 0, 0.8866, 0.5984 } }	// Bard-Schweitzer
    },{	     
	{ { 0,0 }, { 0,	 3.96,	1.131 }, { 0,	1.04, 0.8689 } },	// Exact
	{ { 0,0 }, { 0, 3.887,	1.081 }, { 0,  1.113, 0.9185 } },	// Linearizer 
	{ { 0,0 }, { 0, 3.887,	1.081 }, { 0,  1.113, 0.9185 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 3.866,	1.067 }, { 0,  1.134, 0.9335 } }	// Bard-Schweitzer
    },{	     
	{ { 0,0 }, { 0, 3.359, 0.7053 }, { 0,  1.641,  1.295 } },	// Exact
	{ { 0,0 }, { 0,	 3.21, 0.6559 }, { 0,	1.79,  1.344 } },	// Linearizer 
	{ { 0,0 }, { 0,	 3.21, 0.6559 }, { 0,	1.79,  1.344 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 3.182, 0.6297 }, { 0,  1.818,	1.37 } }	// Bard-Schweitzer
    },{	     
	{ { 0,0 }, { 0, 2.488, 0.4088 }, { 0,  2.512,  1.591 } },	// Exact
	{ { 0,0 }, { 0, 2.355, 0.3835 }, { 0,  2.645,  1.616 } },	// Linearizer 
	{ { 0,0 }, { 0, 2.355, 0.3835 }, { 0,  2.645,  1.616 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 2.336, 0.3606 }, { 0,  2.664,  1.639 } }	// Bard-Schweitzer
    },{	     
	{ { 0,0 }, { 0, 1.795, 0.2524 }, { 0,  3.205,  1.748 } },	// Exact
	{ { 0,0 }, { 0, 1.699, 0.2408 }, { 0,  3.301,  1.759 } },	// Linearizer 
	{ { 0,0 }, { 0, 1.699, 0.2408 }, { 0,  3.301,  1.759 } },	// Fast Linearizer 
	{ { 0,0 }, { 0, 1.685, 0.2261 }, { 0,  3.315,  1.774 } }	// Bard-Schweitzer
    }
};

bool
check( const int solverId, const MVA & solver, const unsigned s22_ix )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    if ( fabs( solver.L[n][m][1][k] - goodL[s22_ix][solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[s22_ix][solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}

