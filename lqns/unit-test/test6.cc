/*  -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/unit-test/test6.cc $
 *
 * Hardware submodel for Flow.
 * ------------------------------------------------------------------------
 * $Id: test6.cc 8841 2009-07-14 14:21:57Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testmva.h"
#include "server.h"
#include "pop.h"
#include "mva.h"

static double responseTime( Server **, const unsigned, const unsigned );

static const unsigned nStations		= 6;
static const unsigned maxEntries	= 5;

void
test( PopVector& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = 5;
    const unsigned stations = nStations;
	
    NCust.grow(classes);			/* Population vector.		*/
    Z.grow(classes);			/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);			/* Queue type.  SS/delay.	*/

    Q[1] = new Infinite_Server(classes);	/* Act1 */
    Q[2] = new Infinite_Server(classes);	/* Act2 */
    Q[3] = new Infinite_Server(classes);	/* Act3 */
    Q[4] = new Infinite_Server(classes);	/* Act4 */
    Q[5] = new Infinite_Server(classes);	/* Act5 */
    Q[6] = new FCFS_Server(6,classes);	/* CPU */

    for ( unsigned k = 1; k <= classes; ++k ) {
	NCust[k] = 1;
    }

    Z[1] = 0.00000;
    Z[2] = 0.00000;
    Z[3] = 9.35280;
    Z[4] = 1.67779;
    Z[5] = 2.27825;

    /* Tasks */
    //  e,k,p,...                e,k,...
    Q[1]->setService(1,7.67204).setVisits(1,1);
    Q[2]->setService(2,7.63629).setVisits(2,1);
    Q[3]->setService(3,0.00000).setVisits(3,1);
    Q[4]->setService(4,1.54921).setVisits(4,1);
    Q[5]->setService(5,0.00000).setVisits(5,1);

    /* Processor */

    Q[6]->setService(1,1,1,0.333333).setVisits(1,1,1,3);
    Q[6]->setService(2,2,1,0.333333).setVisits(2,2,1,3);
    Q[6]->setService(3,3,1,1.000000).setVisits(3,3,1,1);
    Q[6]->setService(4,4,1,0.500000).setVisits(4,4,1,2);
    Q[6]->setService(5,5,1,1.000000).setVisits(5,5,1,1);
}



void
special_check( ostream&, const MVA&, const unsigned )
{
}


static double goodL[4][nStations+1][maxEntries+1] =
{
    /*0              1                      2                      3                 4                      5                6*/
    /*0*/   { {0,0,0,0,0,0}, {0, 0.6924, 0,0,0,0 }, {0,0, 0.6914, 0,0,0 }, {0,0,0, 0, 0,0 }, {0,0,0,0, 0.28,   0 }, {0,0,0,0,0, 0 }, {0, 0.3076,  0.3086,  0.1553,  0.4167,  0.3815 } },
    /*1*/	{ {0,0,0,0,0,0}, {0, 0.6931, 0,0,0,0 }, {0,0, 0.6922, 0,0,0 }, {0,0,0, 0, 0,0 }, {0,0,0,0, 0.28,   0 }, {0,0,0,0,0, 0 }, {0, 0.3069,  0.3078,  0.1551,  0.416,   0.3812 } },
    /*2*/	{ {0,0,0,0,0,0}, {0, 0.6931, 0,0,0,0 }, {0,0, 0.6922, 0,0,0 }, {0,0,0, 0, 0,0 }, {0,0,0,0, 0.28,   0 }, {0,0,0,0,0, 0 }, {0, 0.3069,  0.3078,  0.1551,  0.416,   0.3812 } },
    /*3*/	{ {0,0,0,0,0,0}, {0, 0.6727, 0,0,0,0 }, {0,0, 0.6717, 0,0,0 }, {0,0,0, 0, 0,0 }, {0,0,0,0, 0.2661, 0 }, {0,0,0,0,0, 0 }, {0, 0.3273,  0.3283,  0.1655,  0.4457,  0.4136 } }
};

bool
check( const int solverId, const MVA & solver, const unsigned )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {

	    unsigned e;
	    if ( m < 6 ) {
		e = 1;
	    } else {
		e = k;
	    }
	    if ( fabs( solver.L[n][m][e][k] - goodL[solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][e][k] << ", Correct= " << goodL[solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}

