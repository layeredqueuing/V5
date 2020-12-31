/*  -*- c++ -*-
 * $Id: test7e.cc 13676 2020-07-10 15:46:20Z greg $
 *
 * Markov Phased server test.  N customers.  2 clients to separate entries.
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
#define	N_ENTRIES	2
#define	N_PHASES	2

#define	N1_EQ_0		0
#define	N1_EQ_1		1
#define	N1_EQ_2		2
#define	N1_EQ_3		3
#define	N1_EQ_4		4
#define	N1_EQ_5		5
#define N1_COUNT	6


void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned n1_ix )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;
	
    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);			/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);			/* Queue type.  SS/delay.	*/

    Z[1] = 0.0;

    Q[1] = new Infinite_Server(classes);
    Q[2] = new Markov_Phased_Server(N_ENTRIES,classes,N_PHASES);

    Probability *** prOt1 = Q[2]->getPrOt( 1 );
    Probability *** prOt2 = Q[2]->getPrOt( 2 );

    prOt1[1][1][2] = 0.25;
    prOt1[1][0][2] = 0.25;
    prOt2[1][1][2] = 0.3;
    prOt2[1][0][2] = 0.3;
    Q[2]->setService(1,1,1,1.0).setService(1,1,2,1.0).setVisits(1,1,1,0.5);
    Q[2]->setService(2,1,1,0.5).setService(2,1,2,1.5).setVisits(2,1,1,0.5);

    phase2_correction = SIMPLE_PHASE2;
	
    NCust[1] = 2;
    switch ( n1_ix ) {
    case N1_EQ_0:
	NCust[1] = 1;
	break;

    case N1_EQ_1:
    case N1_EQ_2:
    case N1_EQ_3:
    case N1_EQ_4:
    case N1_EQ_5:
	NCust[1] = n1_ix;
	break;
	
    default:
	cerr << "Invalid N1 index (0-" << N1_COUNT << "):" << n1_ix << endl;
	exit( 1 );
    }
	
    Q[1]->setService(1,1.0).setVisits(1,1.0);
}


static double exactU[N1_COUNT][N_STATIONS+1][N_ENTRIES+1] =
{
    { { 0,0,0 }, {0,0.40816,0}, {0,0.40816,0.40816} },	/* 0 */
    { { 0,0,0 }, {0,0.40816,0}, {0,0.40816,0.40816} },	// 1
    { { 0,0,0 }, {0,0.48989,0}, {0,0.48989,0.48989} },	// 2
    { { 0,0,0 }, {0,0.49932,0}, {0,0.49932,0.49932} },	// 3
    { { 0,0,0 }, {0,0.49997,0}, {0,0.49997,0.49997} },	// 4
    { { 0,0,0 }, {0,0.49997,0}, {0,0.49997,0.49997} },	// 5
};

static double goodL[N1_COUNT][4][N_STATIONS+1][N_ENTRIES+1] =
{
    { { {0,0,0}, {0, 0.4082, 0}, {0, 0.3469, 0.2449} },	// Exact MVA
      { {0,0,0}, {0, 0.4082, 0}, {0, 0.3469, 0.2449} }, 	// Lin
      { {0,0,0}, {0, 0.4082, 0}, {0, 0.3469, 0.2449} },	// Fast Linearizer
      { {0,0,0}, {0, 0.4082, 0}, {0, 0.3469, 0.2449} } },	// Bard

    { { {0,0,0}, {0, 0.4082, 0}, {0, 0.3469, 0.2449} },	// Exact MVA
      { {0,0,0}, {0, 0.4082, 0}, {0, 0.3469, 0.2449} }, 	// Lin
      { {0,0,0}, {0, 0.4082, 0}, {0, 0.3469, 0.2449} },	// Fast Linearizer
      { {0,0,0}, {0, 0.4082, 0}, {0, 0.3469, 0.2449} } },	// Bard

    { { {0,0,0}, {0, 0.4578, 0}, {0, 0.8283, 0.7139} },
      { {0,0,0}, {0, 0.5006, 0}, {0, 0.8123, 0.6871} }, 
      { {0,0,0}, {0, 0.5006, 0}, {0, 0.8123, 0.6871} },
      { {0,0,0}, {0, 0.4541, 0}, {0, 0.8297, 0.7162} } },

    { { {0,0,0}, {0, 0.4718, 0}, {0, 1.323,  1.205} },
      { {0,0,0}, {0, 0.4965, 0}, {0, 1.314,   1.19} }, 
      { {0,0,0}, {0, 0.4965, 0}, {0, 1.314,   1.19} },
      { {0,0,0}, {0, 0.4697, 0}, {0, 1.324,  1.206} } },

    { { {0,0,0}, {0, 0.4787, 0}, {0,  1.82,  1.701} },
      { {0,0,0}, {0, 0.4927, 0}, {0, 1.815,  1.692} }, 
      { {0,0,0}, {0, 0.4927, 0}, {0, 1.815,  1.692} },
      { {0,0,0}, {0, 0.4774, 0}, {0, 1.821,  1.702} } },

    { { {0,0,0}, {0, 0.4829, 0}, {0, 2.319,  2.198} },
      { {0,0,0}, {0,  0.483, 0}, {0, 2.319,  2.198} }, 
      { {0,0,0}, {0,  0.483, 0}, {0, 2.319,  2.198} },
      { {0,0,0}, {0,  0.482, 0}, {0, 2.319,  2.199} } },
};


void
special_check( ostream& output, const MVA& solver, const unsigned s1_ix )
{
    const int width = output.precision() + 2;
    const unsigned k = 1;
	
    output << "By:        Solver  GreatSPN       Difference" << endl;
    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned e = 1; e <= N_ENTRIES; ++e ) {
	    if ( m == 1 && e > 1 ) continue;
	    const double U1 = solver.U[n][m][e][k];
	    const double U2 = exactU[s1_ix][m][e];

	    if ( U2 == 0.0 ) continue;

	    output << "U_" << m << e << k << solver.U[n][m][e][k];
	    output << ", " << setw(width) << U2;
	    output << ", delta = " << setw(width) << (fabs( U2 - U1 ) * 100.0 / U2) << endl;
	}
    }
}



bool
check( const int solverId, const MVA & solver, const unsigned n1_ix )
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
	    if ( fabs( solver.L[n][m][e][k] - goodL[n1_ix][solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][e][k] << ", Correct= " << goodL[n1_ix][solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}
