/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/test12s.cc $
 *
 * Test case from:
 *      author =       "de Souza e Silva, Edmundo and Muntz, Richard R.",
 *      title =        "Approximate Solutions for a Class of Non-Product
 *                      Form Queueing Network Models",
 *      journal =      peva,
 *      issn =         "0166-5316",
 *      callno =       "QA402.A1P4",
 *      year =         1987,
 *      volume =       7,
 *      number =       3,
 *      pages =        "221--242",
 *
 * See table 5a.
 *
 * ------------------------------------------------------------------------
 * $Id: test12s.cc 13413 2018-10-23 15:03:40Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testmva.h"
#include "server.h"
#include "multserv.h"
#include "pop.h"
#include "mva.h"

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = 2;
    const unsigned stations = 2;
    const unsigned servers  = 2;

    NCust.resize(classes);			/* Population vector.		*/
    Z.grow(classes);				/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);				/* Queue type.  SS/delay.	*/

    NCust[1] = 5;	NCust[2] = 2;
    Z[1] = 0.0;	Z[2] = 0.0;

    Q[1] = new PS_Server(classes);
    Q[2] = new Conway_Multi_Server(servers,classes);

    Q[1]->setService(1,1.0).setVisits(1,1.0);
    Q[1]->setService(2,2.0).setVisits(2,1.0);
    Q[2]->setService(1,1.0).setVisits(1,1.0);
    Q[2]->setService(2,2.0).setVisits(2,1.0);
//    Q[2]->setService(2,1.0).setVisits(2,2.0);
}


void
special_check( ostream&, const MVA&, const unsigned )
{
}



static double goodL[4][3][3] = {
    /* station       1                     2 */
    { { 0, 0, 0 }, { 0, 4.025, 1.662  }, { 0, 0.9755, 0.3385 } },	// Exact MVA
    { { 0, 0, 0 }, { 0, 4.121, 1.68   }, { 0, 0.8789, 0.3203 } },	// Linearizer
    { { 0, 0, 0 }, { 0, 4.121, 1.68   }, { 0, 0.8789, 0.3203 } },	// Fast Linearizer
    { { 0, 0, 0 }, { 0, 4.168, 1.692  }, { 0, 0.8318, 0.3078 } },	// Bard Schweitzer
};

bool
check( const int solverId, const MVA& solver, const unsigned )
{
    bool ok = true;

    const unsigned n = solver.offset(solver.NCust);			/* Hoist */
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

