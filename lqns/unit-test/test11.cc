/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/test11.cc $
 *
 * Priority MVA Test case from:
 *
 * ------------------------------------------------------------------------
 * $Id: test11.cc 13413 2018-10-23 15:03:40Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testmva.h"
#include "server.h"
#include "pop.h"
#include "mva.h"

void test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = 2;
    const unsigned stations = 2;

    NCust.resize(classes);		/* Population vector.		*/
    Z.grow(classes);			/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);			/* Queue type.  SS/delay.	*/

    NCust[1] = 3;	NCust[2] = 3;
    Z[1] = 0.0;	Z[2] = 0.0;
    priority[1] = 0;priority[2] = 0;

    Q[1] = new Infinite_Server(classes);
    Q[2] = new FCFS_Server(classes);

    Q[1]->setService(1,4.0).setVisits(1,1.0);
    Q[1]->setService(2,4.0).setVisits(2,1.0);
    Q[2]->setService(1,1.0).setVisits(1,1.0);
    Q[2]->setService(2,1.0).setVisits(2,1.0);
}


void special_check( ostream&, const MVA&, const unsigned )
{
}



static double goodL[4][3][3] = {
    { { 0, 0, 0 }, { 0, 1.766, 1.766 }, { 0, 1.234, 1.234 } },	// Exact MVA
    { { 0, 0, 0 }, { 0, 1.771, 1.771 }, { 0, 1.229, 1.229 } },	// Linearizer
    { { 0, 0, 0 }, { 0, 1.771, 1.771 }, { 0, 1.229, 1.229 } },	// Fast Linearizer
    { { 0, 0, 0 }, { 0, 1.659, 1.659 }, { 0, 1.341, 1.341 } },	// Bard Schweitzer
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

