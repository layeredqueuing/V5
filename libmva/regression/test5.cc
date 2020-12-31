/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/libmva/regression/test5.cc $
 *
 * ??
 *
 * ------------------------------------------------------------------------
 * $Id: test5.cc 13676 2020-07-10 15:46:20Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testmva.h"
#include "server.h"
#include "pop.h"
#include "mva.h"

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = 1;
    const unsigned stations = 4;
    
    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);				/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);				/* Queue type.  SS/delay.	*/

    Q[1] = new FCFS_Server(classes);		/* Client */
    Q[2] = new FCFS_Server(classes);		/* Server */
    Q[3] = new FCFS_Server(classes);		/* Server */
    Q[4] = new FCFS_Server(classes);		/* Server */

    NCust[1] = 1;	Z[1] = 0.0;

    Q[1]->setService(1,2.0).setVisits(1,1.0);
    Q[2]->setService(1,1.0).setVisits(1,3.0);
    Q[3]->setService(1,1.0).setVisits(1,3.0);
    Q[4]->setService(1,1.0).setVisits(1,3.0);
}



void
special_check( ostream&, const MVA&, const unsigned )
{
}




static double goodL[4][5][2] = {
    { { 0, 0 }, {0, 0.1818}, {0, 0.2727}, {0, 0.2727}, {0, 0.2727} },
    { { 0, 0 }, {0, 0.1818}, {0, 0.2727}, {0, 0.2727}, {0, 0.2727} },
    { { 0, 0 }, {0, 0.1818}, {0, 0.2727}, {0, 0.2727}, {0, 0.2727} },
    { { 0, 0 }, {0, 0.1818}, {0, 0.2727}, {0, 0.2727}, {0, 0.2727} }
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

