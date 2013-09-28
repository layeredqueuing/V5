/*  -*- c++ -*-
 * $HeadURL$
 *
 * Four customers, one server.
 *
 * ------------------------------------------------------------------------
 * $Id$
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
test( PopVector& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;

    NCust.grow(classes);			/* Population vector.		*/
    Z.grow(classes);				/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);				/* Queue type.  SS/delay.	*/

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

