/*  -*- c++ -*-
 * $Id: test7.cc 13676 2020-07-10 15:46:20Z greg $
 *
 * Markov Phased server test.  N customers.  2 clients to common entry.
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
    Z.resize(classes);			/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);			/* Queue type.  SS/delay.	*/

    NCust[1] = 1;	Z[1] = 0.0;
    NCust[2] = 1;	Z[2] = 0.0;

    Q[1] = new Infinite_Server(classes);
    Q[2] = new Infinite_Server(classes);
    Q[3] = new Markov_Phased_Server(classes,N_PHASES);

    Probability *** prOt = Q[3]->getPrOt( 1 );

    phase2_correction = SIMPLE_PHASE2;

    double s1;
    switch ( s1_ix ) {
    case S1_EQ_0:
    case S1_EQ_1:
	s1   = 1.0;
	prOt[1][1][2] = 0.5;
	prOt[2][1][2] = 0.5;
	prOt[1][0][2] = 0.5;
	prOt[2][0][2] = 0.5;
	break;
    case S1_EQ_2:
	s1   = 2.0;
	prOt[1][1][2] = 1.0 / 3.0;
	prOt[2][1][2] = 1.0 / 3.0;
	prOt[1][0][2] = 1.0 / 3.0;
	prOt[2][0][2] = 1.0 / 3.0;
	break;
    case S1_EQ_3:
	s1   = 1.0;
	prOt[1][1][2] = 0.5;
	prOt[2][1][2] = 0.5;
	prOt[1][0][2] = 0.5;
	prOt[2][0][2] = 0.5;
	NCust[1] = 3;
	NCust[2] = 2;
	break;
    case S1_EQ_4:
	s1   = 2.0;
	prOt[1][1][2] = 1.0 / 3.0;
	prOt[2][1][2] = 1.0 / 3.0;
	prOt[1][0][2] = 1.0 / 3.0;
	prOt[2][0][2] = 1.0 / 3.0;
	NCust[1] = 3;
	NCust[2] = 2;
	break;
    default:
	cerr << "Invalid S1 index (0-4):" << s1_ix << endl;
	exit( 1 );
    }
	
    Q[1]->setService(1,s1).setVisits(1,1.0);
    Q[2]->setService(2,s1).setVisits(2,1.0);

    for ( unsigned k = 1; k <= classes; ++k ) {
	Q[3]->setService(1,k,1,1.0).setService(1,k,2,1.0).setVisits(1,k,1,1.0);
    }
}


static double exactU[S1_COUNT][N_STATIONS+1][N_CLASSES+1] =
{
    { { 0,0,0 }, {0, 0.2444, 0}, {0,0, 0.2444}, {0, 0.4889, 0.4889 } },
    { { 0,0,0 }, {0, 0.2444, 0}, {0,0, 0.2444}, {0, 0.4889, 0.4889 } },
    { { 0,0,0 }, {0, 0.4444, 0}, {0,0, 0.4444}, {0, 0.4444, 0.4444 } }
};

static double goodL[S1_COUNT][4][N_STATIONS+1][N_CLASSES+1] =
{
    { { { 0,0,0 }, {0, 0.2439, 0}, {0,0, 0.2439}, {0, 0.7561, 0.7561 } },		/* NOT product form... */
      { { 0,0,0 }, {0, 0.2554, 0}, {0,0, 0.2554}, {0, 0.7446, 0.7446 } },
      { { 0,0,0 }, {0, 0.2554, 0}, {0,0, 0.2554}, {0, 0.7446, 0.7446 } },
      { { 0,0,0 }, {0, 0.2344, 0}, {0,0, 0.2344}, {0, 0.7656, 0.7656 } } },
								     
    { { { 0,0,0 }, {0, 0.2439, 0}, {0,0, 0.2439}, {0, 0.7561, 0.7561 } },		/* NOT product form... */
      { { 0,0,0 }, {0, 0.2554, 0}, {0,0, 0.2554}, {0, 0.7446, 0.7446 } },
      { { 0,0,0 }, {0, 0.2554, 0}, {0,0, 0.2554}, {0, 0.7446, 0.7446 } },
      { { 0,0,0 }, {0, 0.2344, 0}, {0,0, 0.2344}, {0, 0.7656, 0.7656 } } },
								     
    { { { 0,0,0 }, {0, 0.4511, 0}, {0, 0, 0.4511}, {0, 0.5489, 0.5489} },		/* NOT product form... */
      { { 0,0,0 }, {0, 0.4602, 0}, {0, 0, 0.4602}, {0, 0.5398, 0.5398} },
      { { 0,0,0 }, {0, 0.4602, 0}, {0, 0, 0.4602}, {0, 0.5398, 0.5398} },
      { { 0,0,0 }, {0, 0.4261, 0}, {0, 0, 0.4261}, {0, 0.5739, 0.5739} } },

    { { { 0,0,0 }, {0,    0.3, 0}, {0, 0,    0.2}, {0,    2.7,    1.8} },         
      { { 0,0,0 }, {0, 0.3034, 0}, {0, 0, 0.2022}, {0,  2.697,  1.798} },  
      { { 0,0,0 }, {0, 0.3034, 0}, {0, 0, 0.2022}, {0,  2.697,  1.798} },  
      { { 0,0,0 }, {0, 0.2969, 0}, {0, 0,  0.198}, {0,  2.703,  1.802} } },

    { { { 0,0,0 }, {0, 0.6122, 0}, {0, 0, 0.4081}, {0,  2.388,  1.592} },         
      { { 0,0,0 }, {0,  0.619, 0}, {0, 0, 0.4127}, {0,  2.381,  1.587} },  
      { { 0,0,0 }, {0,  0.619, 0}, {0, 0, 0.4127}, {0,  2.381,  1.587} },  
      { { 0,0,0 }, {0, 0.5911, 0}, {0, 0, 0.3941}, {0,  2.409,  1.606} } }
};


void
special_check( ostream& output, const MVA& solver, const unsigned s1_ix )
{
    const unsigned e = 1;
    const int width = output.precision() + 2;
	
    output << "By:         Solver GreatSPN      Difference" << endl;
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

