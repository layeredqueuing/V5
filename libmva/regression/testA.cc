/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/libmva/regression/testA.cc $
 *
 * Test case from:
 * ------------------------------------------------------------------------
 * $Id: testA.cc 15384 2022-01-25 02:56:14Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testopen.h"
#include "server.h"
#include "open.h"

void test( Vector<Server *>& Q, const unsigned )
{
    const unsigned stations = 2;
    Q.resize(stations);		/* Queue type.  SS/delay.	*/

    Q[1] = new FCFS_Server(0);
    Q[2] = new FCFS_Server(0);

    Q[1]->setService(0,8.0).setVisits(0,0.1);
    Q[2]->setService(0,1.0).setVisits(0,0.5);
}


void special_check( std::ostream&, const Open&, const unsigned )
{
}



static double goodL[3] = { 0, 0, 0 };

bool
check( std::ostream&, const Open& solver, const unsigned )
{
    bool ok = true;

#ifdef	NOTDEF
    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	if ( fabs( solver.L[n][m][1][k] - goodL[m] ) >= 0.001 ) {
	    std::cerr << "Mismatch at m=" << m <<", k=" << k;
	    std::cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[solverId][m][k] << std::endl;
	    ok = false;
	}
    }
#endif
    return ok;
}
