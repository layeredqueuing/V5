/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/testB.cc $
 *
 * Test case from:
 *
 * ------------------------------------------------------------------------
 * $Id: testB.cc 13413 2018-10-23 15:03:40Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testopen.h"
#include "server.h"
#include "multserv.h"
#include "open.h"

void test( Vector<Server *>& Q, const unsigned )
{
    const unsigned stations = 6;
    const double lambda = 0.2;

    Q.grow(stations);		/* Queue type.  SS/delay.	*/

    Q[1] = new FCFS_Server(0);
    Q[2] = new Reiser_Multi_Server(2,0);
    Q[3] = new Reiser_Multi_Server(4,0);

    Q[4] = new FCFS_Server(0);
    Q[5] = new Reiser_Multi_Server(2,0);
    Q[6] = new Reiser_Multi_Server(4,0);

    Q[1]->setService(0,4.0).setVisits(0,lambda);
    Q[2]->setService(0,8.0).setVisits(0,lambda);
    Q[3]->setService(0,16.0).setVisits(0,lambda);

    Q[4]->setService(0,4.0).setVisits(0,lambda/10.0);
    Q[5]->setService(0,8.0).setVisits(0,lambda/10.0);
    Q[6]->setService(0,16.0).setVisits(0,lambda/10.0);
}


void
special_check( ostream&, const Open&, const unsigned )
{
}



static double goodL[3] = { 0, 0, 0 };

bool
check( ostream&, const Open& solver, const unsigned )
{
    bool ok = true;

#ifdef	NOTDEF
    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	if ( fabs( solver.L[n][m][1][k] - goodL[m] ) >= 0.001 ) {
	    cerr << "Mismatch at m=" << m <<", k=" << k;
	    cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[solverId][m][k] << endl;
	    ok = false;
	}
    }
#endif
    return ok;
}

#include <vector.cc>

template class Vector<double>;
template class Vector<unsigned int>;
template class Vector<unsigned long>;
template class Vector<Server *>;
template class VectorMath<unsigned int>;
template class VectorMath<double>;
template class Vector<Vector<unsigned> >;
template class Vector<VectorMath<double> >;
template class Vector<VectorMath<unsigned> >;

