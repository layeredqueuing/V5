/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/libmva/regression/test2p.cc $
 *
 * Four customers, one server (inService probability test).
 * This version has a phased server.
 *
 * ------------------------------------------------------------------------
 * $Id: test2p.cc 13676 2020-07-10 15:46:20Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testmva.h"
#include "server.h"
#include "ph2serv.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	4
#define	N_STATIONS	5
#define	N_PHASES	2

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;

    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);				/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);				/* Queue type.  SS/delay.	*/

    NCust[1] = 1;   Z[1] = 0.0;
    NCust[2] = 1;   Z[2] = 0.0;
    NCust[3] = 1;   Z[3] = 0.0;
    NCust[4] = 1;   Z[4] = 0.0;

    Q[1] = new FCFS_Server(classes);
    Q[2] = new FCFS_Server(classes);
    Q[3] = new FCFS_Server(classes);
    Q[4] = new FCFS_Server(classes);
    Q[5] = new Rolia_Phased_Server(classes,N_PHASES);

    unsigned i;
    for ( i = 1; i <= classes; ++i ) {
	Q[i]->setService(1,i,1,1.0).setVisits(1,i,1,1.0);
    }

    for ( i = 1; i <= classes; ++i ) {
	Q[5]->setService(1,i,1,0.5).setService(1,i,2,0.5).setVisits(1,i,1,1.0);
    }
}


void
special_check( ostream&, const MVA&, const unsigned )
{
}



static double goodL[4][N_STATIONS+1][N_CLASSES+1] =
{
    { { 0,0,0,0 }, { 0,0.2813,0,0,0 }, { 0,0,0.2813,0,0 }, { 0,0,0,0.2813,0 }, { 0,0,0,0,0.2813 }, {0, 0.7187,  0.7187,  0.7187,  0.7187} },
    { { 0,0,0,0 }, { 0,0.2784,0,0,0 }, { 0,0,0.2784,0,0 }, { 0,0,0,0.2784,0 }, { 0,0,0,0,0.2784 }, {0, 0.7216,  0.7216,  0.7216,  0.7216} },
    { { 0,0,0,0 }, { 0,0.2784,0,0,0 }, { 0,0,0.2784,0,0 }, { 0,0,0,0.2784,0 }, { 0,0,0,0,0.2784 }, {0, 0.7216,  0.7216,  0.7216,  0.7216} },
    { { 0,0,0,0 }, { 0,0.2566,0,0,0 }, { 0,0,0.2566,0,0 }, { 0,0,0,0.2566,0 }, { 0,0,0,0,0.2566 }, {0, 0.7434,  0.7434,  0.7434,  0.7434} }
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

