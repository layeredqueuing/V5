/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/test1hvp.cc $
 *
 * Test case from:
 *     author =   "Chandy, K. Mani and Neuse, Doug",
 *     title =    "Linearizer: A Heuristic Algorithm for Queueing
 *                 Network Models of Computing Systems",
 *     journal =  cacm,
 *     year =     1982,
 *     volume =   25,
 *     number =   2,
 *     pages =    "126--134",
 *     month =    feb
 *
 * Uses highly variable phased server instead of regular FCFS server.
 * Results should match FCFS Server exactly.
 * ------------------------------------------------------------------------
 * $Id: test1hvp.cc 13676 2020-07-10 15:46:20Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testmva.h"
#include "server.h"
#include "ph2serv.h"
#include "pop.h"
#include "mva.h"

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = 2;
    const unsigned stations = 2;

    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);				/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);				/* Queue type.  SS/delay.	*/

    NCust[1] = 8;	NCust[2] = 1;
    Z[1] = 0.0;	Z[2] = 0.0;

    Q[1] = new Infinite_Server(classes,2);
    Q[2] = new HVFCFS_Simple_Phased_Server(classes,2);

    Q[1]->setService(1,1,1,8.0).setVisits(1,1,1,1.0);
    Q[1]->setService(1,2,1,0.0).setVisits(1,2,1,0.0);
    Q[2]->setService(1,1,1,1.0).setVisits(1,1,1,1.0).setVariance(1,1,1,1.0);
    Q[2]->setService(1,2,1,1.0).setVisits(1,2,1,1.0).setVariance(1,2,1,1.0);
}


void
special_check( ostream&, const MVA&, const unsigned )
{
}



static double goodL[4][3][3] = {
    { { 0, 0, 0 }, { 0, 5.227, 0 }, { 0, 2.773, 1 } },
    { { 0, 0, 0 }, { 0, 5.231, 0 }, { 0, 2.769, 1 } },
    { { 0, 0, 0 }, { 0, 5.231, 0 }, { 0, 2.769, 1 } },
    { { 0, 0, 0 }, { 0, 5.108, 0 }, { 0, 2.892, 1 } },
};

bool
check( const int solverId, const MVA& solver, const unsigned )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    if ( fabs( solver.L[n][m][1][k] - goodL[solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}

