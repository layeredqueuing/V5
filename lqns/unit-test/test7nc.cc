/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/test7nc.cc $
 *
 * Markov Phased server test.  N customers.  One client to common entry.
 * ------------------------------------------------------------------------
 * $Id: test7nc.cc 13413 2018-10-23 15:03:40Z greg $
 * ------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <cmath>
#include "testmva.h"
#include "server.h"
#include "ph2serv.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	1
#define	N_STATIONS	2
#define	N_PHASES	2

#define	S1_EQ_0		0
#define	S1_EQ_1		1
#define	S1_EQ_2		2
#define	S1_EQ_3		3
#define	S1_EQ_4		4
#define S1_COUNT	5

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned s1_ix )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;
    
    NCust.resize(classes);			/* Population vector.		*/
    Z.grow(classes);				/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);				/* Queue type.  SS/delay.	*/

    NCust[1] = 2;	Z[1] = 0.0;

    Q[1] = new Infinite_Server(classes);
    Q[2] = new Markov_Phased_Server(classes,N_PHASES);

    Probability *** prOt = Q[2]->getPrOt( 1 );
    double s1;
	
    phase2_correction = SIMPLE_PHASE2;

    switch ( s1_ix ) {
    case S1_EQ_0:
	s1   = 1.0;
	prOt[1][1][2] = 0.5;
	NCust[1] = 1;
	break;
    case S1_EQ_1:
	s1   = 1.0;
	prOt[1][1][2] = 0.5;
	break;
    case S1_EQ_2:
	s1   = 2.0;
	prOt[1][1][2] = 1.0 / 3.0;
	break;
    case S1_EQ_3:
	s1   = 1.0;
	prOt[1][1][2] = 0.5;
	NCust[1] = 5;
	break;
    case S1_EQ_4:
	s1   = 2.0;
	prOt[1][1][2] = 1.0 / 3.0;
	NCust[1] = 5;
	break;
    default:
	cerr << "Invalid S1 index (0-2):" << s1_ix << endl;
	exit( 1 );
    }
    prOt[1][0][2] = prOt[1][1][2];	
    Q[1]->setService(1,s1).setVisits(1,1.0);
    Q[2]->setService(1,1,1,1.0).setService(1,1,2,1.0).setVisits(1,1,1,1.0);
}


static double exactU[S1_COUNT][N_STATIONS+1][N_CLASSES+1] =
{
    { {0,0}, { 0,0.4 }, { 0,0.8 } },
    { {0,0}, { 0,0.4889 }, { 0,0.9778 } },
    { {0,0}, { 0,0.8888 }, { 0,0.8888 } },
    { {0,0}, { 0,0.5 }, { 0,1.0 } }
};

static double goodL[S1_COUNT][4][N_STATIONS+1][N_CLASSES+1] =
{
    { { { 0,0 }, { 0,0.4 }, { 0,0.6 } },		/* NOT product form... */
      { { 0,0 }, { 0,0.4 }, { 0,0.6 } },
      { { 0,0 }, { 0,0.4 }, { 0,0.6 } },
      { { 0,0 }, { 0,0.4 }, { 0,0.6 } } },
	   	      	      	    
    { { { 0,0 }, {0, 0.4878}, {0, 1.512} },		/* NOT product form... */
      { { 0,0 }, {0, 0.5107}, {0, 1.489} },
      { { 0,0 }, {0, 0.5107}, {0, 1.489} },
      { { 0,0 }, {0, 0.4689}, {0, 1.531} } },
	   	      	      	    
    { { { 0,0 }, {0, 0.8824}, {0, 1.118} },		/* NOT product form... */
      { { 0,0 }, {0, 0.9088}, {0, 1.091} },
      { { 0,0 }, {0, 0.9088}, {0, 1.091} },
      { { 0,0 }, {0, 0.8378}, {0, 1.162} } },

    { { { 0,0 }, {0,    0.5}, {0,   4.5} },		/* NOT product form... */
      { { 0,0 }, {0, 0.5056}, {0, 4.494} },
      { { 0,0 }, {0, 0.5056}, {0, 4.494} },
      { { 0,0 }, {0, 0.4949}, {0, 4.505} } }
};

void special_check( ostream& output, const MVA& solver, const unsigned s1_ix )
{
    const unsigned e = 1;
    const int width = output.precision() + 2;
	
    output << "By:        Solver  GreatSPN        Difference" << endl;
    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    const double U1 = solver.U[n][m][e][k];
	    const double U2 = exactU[s1_ix][m][k];

	    if ( U2 == 0.0 ) continue;

	    output << "U_" << m << e << k << solver.U[n][m][e][k];
	    output << ", " << setw(width) << U2;
	    output << ", delta = " << setw(width) << (fabs( U2 - U1 ) * 100.0 / U2) << endl;
	}
    }
}



bool
check( const int solverId, const MVA & solver, const unsigned s1_ix )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    if ( fabs( solver.L[n][m][1][k] - goodL[s1_ix][solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[s1_ix][solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}

