/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/test2nc.cc $
 *
 * Four customers, one server.
 *
 * ------------------------------------------------------------------------
 * $Id: test2nc.cc 13676 2020-07-10 15:46:20Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testmva.h"
#include "server.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	1
#define	N_STATIONS	2

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;

    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);				/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);				/* Queue type.  SS/delay.	*/

    NCust[1] = 4;	Z[1] = 0.0;

    Q[1] = new Infinite_Server(classes);
    Q[2] = new FCFS_Server(classes);

    Q[1]->setService(1,1.0).setVisits(1,1.0);

    Q[2]->setService(1,0.5).setVisits(1,1.0);
}


void
special_check( ostream&, const MVA&, const unsigned )
{
}



static double goodL[4][N_STATIONS+1][N_CLASSES+1] =
{
    { { 0,0 }, { 0,1.81 },  { 0,2.19 } },
    { { 0,0 }, { 0,1.81 },  { 0,2.19 } },
    { { 0,0 }, { 0,1.81 },  { 0,2.19 } },
    { { 0,0 }, { 0,1.691 }, { 0,2.309 } }
};

bool
check( const int solverId, const MVA & solver, const unsigned )
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

