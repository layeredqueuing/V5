/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/libmva/regression/test12.cc $
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
 * See table 4a.
 *
 * ------------------------------------------------------------------------
 * $Id: test12.cc 13676 2020-07-10 15:46:20Z greg $
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
    Z.resize(classes);				/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);				/* Queue type.  SS/delay.	*/

    NCust[1] = 5;	NCust[2] = 2;
    Z[1] = 0.0;	Z[2] = 0.0;

    Q[1] = new Infinite_Server(classes);
    Q[2] = new Conway_Multi_Server(servers,classes);

    Q[1]->setService(1,6.0).setVisits(1,1.0);
    Q[1]->setService(2,3.0).setVisits(2,1.0);
    Q[2]->setService(1,1.0).setVisits(1,1.0);
    Q[2]->setService(2,2.0).setVisits(2,1.0);
}


void
special_check( ostream&, const MVA&, const unsigned )
{
}



static double goodL[4][3][3] = {
    /* station       1                     2 */
    { { 0, 0, 0 }, { 0, 3.960, 1.1311 }, { 0, 1.040, 0.8689 } },	// Exact MVA
    { { 0, 0, 0 }, { 0, 3.887, 1.0814 }, { 0, 1.113, 0.9186 } },	// Linearizer
    { { 0, 0, 0 }, { 0, 3.887, 1.0814 }, { 0, 1.113, 0.9186 } },	// Fast Linearizer
    { { 0, 0, 0 }, { 0, 3.866, 1.0665 }, { 0, 1.134, 0.9335 } },	// Bard Schweitzer
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

