/*  -*- c++ -*-
 * $HeadURL: http://franks.dnsalias.com/svn/lqn/branches/interlock-merge/lqns/unit-test/test0.cc $
 *
 * Lazowska, Ch 6, Page 117
 *
 * ------------------------------------------------------------------------
 * $Id: test0.cc 8841 2009-07-14 14:21:57Z greg $
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
    const unsigned stations = 3;

    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);				/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);				/* Queue type.  SS/delay.	*/

    Q[1] = new FCFS_Server(classes);		/* Client */
    Q[2] = new FCFS_Server(classes);		/* Server */
    Q[3] = new FCFS_Server(classes);		/* Server */

    NCust[1] = 3;	Z[1] = 15;

    Q[1]->setService(1,0.005).setVisits(1,121);
    Q[2]->setService(1,0.03).setVisits(1,70);
    Q[3]->setService(1,0.027).setVisits(1,50);
    Q[1]->setMaxCustomers(1,1,3);
   Q[2]->setMaxCustomers(1,1,3);
   Q[3]->setMaxCustomers(1,1,3);

}



void
special_check( ostream&, const MVA&, const unsigned )
{
}




static double goodL[4][4][2] = {
    { { 0, 0 }, {0, 0.0976}, {0, 0.3947}, {0, 0.2350} },	/* Exact MVA */
    { { 0, 0 }, {0, 0.0976}, {0, 0.3947}, {0, 0.2350} },	/* Linearizer */
    { { 0, 0 }, {0, 0.0976}, {0, 0.3947}, {0, 0.2350} },	/* Fast Linearize  */
    { { 0, 0 }, {0, 0.0973}, {0, 0.4012}, {0, 0.2359} },	/* Bard Schweitzer */
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

