/*  -*- c++ -*-
 * $Id: test7ek.cc 13676 2020-07-10 15:46:20Z greg $
 *
 * Markov Phased server test.  N customers.  2 clients to separate entries.
 * Entry service time varies.
 * ------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <cmath>
#include "testmva.h"
#include "server.h"
#include "ph2serv.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	2
#define	N_STATIONS	3
#define	N_ENTRIES	2
#define	N_PHASES	2

#define	S2_EQ_0		0
#define	S2_EQ_1		1
#define	S2_EQ_2		2
#define S2_COUNT	3


void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned s2_ix )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;
	
    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);			/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);			/* Queue type.  SS/delay.	*/

    NCust[1] = 1;	Z[1] = 0.0;
    NCust[2] = 1;	Z[2] = 0.0;

    Q[1] = new Infinite_Server(classes);
    Q[2] = new Infinite_Server(classes);
    Q[3] = new Markov_Phased_Server(N_ENTRIES,classes,N_PHASES);

    Probability *** prOt1 = Q[3]->getPrOt( 1 );
    Probability *** prOt2 = Q[3]->getPrOt( 2 );

    double s2[N_CLASSES+1][2+1];

    phase2_correction = SIMPLE_PHASE2;

    switch ( s2_ix ) {
    case S2_EQ_0:
	s2[1][1] = 1.0;
	s2[1][2] = 1.0;
	s2[2][1] = 1.0;
	s2[2][2] = 1.0;
	prOt1[1][1][2] = 0.5;
	prOt1[1][0][2] = 0.5;
	prOt2[2][1][2] = 0.5;
	prOt2[2][0][2] = 0.5;
	break;
    case S2_EQ_2:
	NCust[1] = 2;
	NCust[2] = 2;
    case S2_EQ_1:
	s2[1][1] = 1.0;
	s2[1][2] = 1.0;
	s2[2][1] = 0.5;
	s2[2][2] = 1.5;
	prOt1[1][1][2] = 0.5;
	prOt1[1][0][2] = 0.5;
	prOt2[2][1][2] = 0.6;
	prOt2[2][0][2] = 0.6;
	break;
		
    default:
	cerr << "Invalid S2 index (0-1):" << s2_ix << endl;
	exit( 1 );
    }
	
    Q[1]->setService(1,1.0).setVisits(1,1.0);
    Q[2]->setService(2,1.0).setVisits(2,1.0);

    for ( unsigned k = 1; k <= classes; ++k ) {
	Q[3]->setService(k,k,1,s2[k][1]).setService(k,k,2,s2[k][2]).setVisits(k,k,1,1.0);
    }
}


static double exactU[S2_COUNT][N_STATIONS+1][N_CLASSES+1] =
{
    { { 0,0,0 }, {0, 0.2444, 0}, {0,0, 0.2444}, {0, 0.4449, 0.4449 } },
    { { 0,0,0 }, {0, 0.2362, 0}, {0,0, 0.2537}, {0, 0.4725, 0.5073 } },
    { { 0,0,0 }, {0, 0.2463, 0}, {0,0, 0.2537}, {0, 0.4925, 0.5074 } }
};

static double goodL[S2_COUNT][4][N_STATIONS+1][N_CLASSES+1] =
{
    { { { 0,0,0 }, {0, 0.2222, 0}, {0, 0, 0.2222}, {0,  0.7778,  0.7778} },	/* Exact MVA */
      { { 0,0,0 }, {0,  0.244, 0}, {0, 0,  0.244}, {0,   0.756,   0.756} },	/* Linearizer */
      { { 0,0,0 }, {0,  0.244, 0}, {0, 0,  0.244}, {0,   0.756,   0.756} },	/* Fast Linearizer */
      { { 0,0,0 }, {0, 0.2222, 0}, {0, 0, 0.2222}, {0,  0.7778,  0.7778} } },	/* Bard-Schweitzer */
								     
    { { { 0,0,0 }, {0, 0.2034, 0}, {0,0, 0.2273}, {0, 0.7966, 0.7727} },		/* NOT product form... */
      { { 0,0,0 }, {0, 0.2343, 0}, {0,0, 0.2513}, {0, 0.7657, 0.7487} },
      { { 0,0,0 }, {0, 0.2343, 0}, {0,0, 0.2513}, {0, 0.7657, 0.7487} },
      { { 0,0,0 }, {0, 0.2115, 0}, {0,0, 0.2273}, {0, 0.7885, 0.7727} } },

    { { { 0,0,0 }, {0, 0.2305, 0}, {0,0, 0.2417}, {0,  1.77,  1.758} },		/* NOT product form... */
      { { 0,0,0 }, {0,  0.239, 0}, {0,0, 0.2485}, {0, 1.761,  1.751} },
      { { 0,0,0 }, {0,  0.239, 0}, {0,0, 0.2485}, {0, 1.761,  1.751} },
      { { 0,0,0 }, {0, 0.2319, 0}, {0,0, 0.2409}, {0, 1.768,  1.759} } }
};


void
special_check( ostream& output, const MVA& solver, const unsigned s1_ix )
{
    const int width = output.precision() + 2;
	
    output << "By:         Solver GreatSPN      Difference" << endl;
    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    unsigned e;
	    if ( m == 3 ) {
		e = k;
	    } else {
		e = 1;
	    }
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
check( const int solverId, const MVA & solver, const unsigned s2_ix )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    unsigned e;
			
	    if ( m == 3 ) {
		e = k;
	    } else {
		e = 1;
	    }
	    if ( fabs( solver.L[n][m][e][k] - goodL[s2_ix][solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][e][k] << ", Correct= " << goodL[s2_ix][solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}
