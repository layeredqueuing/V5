/*  -*- c++ -*-
 * $HeadURL$
 *
 * Menasce, chapter 13, q 4.
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

void
test( PopVector& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = 1;
    const unsigned stations = 3;
    const unsigned entries = 2;

    NCust.grow(classes);			/* Population vector.		*/
    Z.grow(classes);				/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);				/* Queue type.  SS/delay.	*/

    Q[1] = new FCFS_Server(entries,classes);	/* CPU */
    Q[2] = new FCFS_Server(entries,classes);	/* Disk1 */
    Q[3] = new FCFS_Server(entries,classes);	/* Disk2 */

    NCust[1] = 50;
    Z[1] = 15;

    /* Q */
    Q[1]->setService(1,0,1,0.06).setVisits(1,0,1,3.0*1.95);
    Q[2]->setService(1,0,1,0.03).setVisits(1,0,1,3.0*1.95);
    Q[3]->setService(1,0,1,0.06).setVisits(1,0,1,3.0*1.95);

    /* U */
    Q[1]->setService(2,0,1,0.10).setVisits(2,0,1,1.5);
    Q[2]->setService(2,0,1,0.03).setVisits(2,0,1,1.5);
    Q[3]->setService(2,0,1,0.09).setVisits(2,0,1,1.5);

    /* I */
    Q[1]->setService(1,0.09).setVisits(1,1);
    Q[2]->setService(1,0.045).setVisits(1,1);
    Q[3]->setService(1,0.0).setVisits(1,1);
}



void
special_check( ostream&, const MVA&, const unsigned )
{
}




static double goodL[4][4][2] = {
    { { 0, 0 }, {0, 1.3177}, {0, 0.2280}, {0, 0.0000} },	/* Exact MVA */
    { { 0, 0 }, {0, 1.3177}, {0, 0.2280}, {0, 0.0000} },	/* Linearizer */
    { { 0, 0 }, {0, 1.3177}, {0, 0.2280}, {0, 0.0000} },	/* Fast Linearize  */
    { { 0, 0 }, {0, 1.3642}, {0, 0.2279}, {0, 0.0000} },	/* Bard Schweitzer */
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

