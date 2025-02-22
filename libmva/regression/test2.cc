/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/libmva/regression/test2.cc $
 *
 * Lazowska, Ch 7, Page 131
 * 
 * ------------------------------------------------------------------------
 * $Id: test2.cc 15384 2022-01-25 02:56:14Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include "testmva.h"
#include "server.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	2
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

    NCust[1] = 10;	Z[1] = 0.0;		/* Batch */
    NCust[2] = 25;	Z[2] = 30.0;		/* Interactive */

    Q[1] = new PS_Server(classes);		/* CPU */
    Q[2] = new PS_Server(classes);		/* Disk */

    Q[1]->setService(1,1.000).setVisits(1,1);
    Q[2]->setService(1,0.090).setVisits(1,1);
    Q[1]->setService(2,0.100).setVisits(2,1);
    Q[2]->setService(2,0.900).setVisits(2,1);
}


void special_check( std::ostream&, const MVA&, const unsigned )
{
}



static double goodL[5][N_STATIONS+1][N_CLASSES+1] =
{
    /* station   1                    2 */
    { { 0,0,0 }, { 0,9.717 ,0.8496 }, { 0,0.2827,2.090 } },		/* Exact MVA */
    { { 0,0,0 }, { 0,9.721 ,0.8515 }, { 0,0.2792,2.055 } },		/* Linearizer */
    { { 0,0,0 }, { 0,9.721 ,0.8515 }, { 0,0.2792,2.055 } },		/* Fast Linearizer */
    { { 0,0,0 }, { 0,9.7047,0.8385 }, { 0,0.2953,2.303 } },		/* Bard Schweitzer */
    { { 0,0,0 }, { 0,9.761 ,0.8496 }, { 0,0.2841,2.086 } },		/* Experimental */
};

bool
check( const int solverId, const MVA & solver, const unsigned )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    if ( fabs( solver.L[n][m][1][k] - goodL[solverId][m][k] ) >= 0.001 ) {
		std::cerr << "Mismatch at m=" << m <<", k=" << k;
		std::cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[solverId][m][k] << std::endl;
		ok = false;
	    }
	}
    }
    return ok;
}

