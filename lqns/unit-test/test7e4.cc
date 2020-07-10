/*  -*- c++ -*-
 * $Id: test7e4.cc 13676 2020-07-10 15:46:20Z greg $
 *
 * Four customers, one server (inService probability test).
 * This version has a phased server.
 */

#include <stdlib.h>
#include <cmath>
#include "testmva.h"
#include "server.h"
#include "ph2serv.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	4
#define	N_STATIONS	5
#define	N_ENTRIES	4
#define	N_PHASES	2

#define	NCUST_EQ_0	0
#define	NCUST_EQ_1	1
#define	NCUST_EQ_2	2
#define	NCUST_EQ_3	3
#define Ncust_COUNT	4

static double initPrOt[N_ENTRIES+1] = { 0.8, 0.6, 0.2, 2.0 / 3.0 };

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned ncust_ix )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;
	
    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);			/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);			/* Queue type.  SS/delay.	*/

    NCust[1] = 1;	Z[1] = 0.0;
    NCust[2] = 1;	Z[2] = 0.0;
    NCust[3] = 1;	Z[1] = 0.0;
    NCust[4] = 1;	Z[2] = 0.0;

    Q[1] = new Infinite_Server(classes);
    Q[2] = new Infinite_Server(classes);
    Q[3] = new Infinite_Server(classes);
    Q[4] = new Infinite_Server(classes);
    Q[5] = new Markov_Phased_Server(N_ENTRIES,classes,N_PHASES);


    struct {
	double s;
	double v;
    } sc[N_CLASSES+1];
    double s5[N_CLASSES+1][2+1];
    Probability *** prOt[N_ENTRIES+1];

    sc[1].s  = 1.0; sc[1].v  = 4.0;
    sc[2].s  = 1.0; sc[2].v  = 1.0;
    sc[3].s  = 2.0; sc[3].v  = 1.0;
    sc[4].s  = 0.5; sc[4].v  = 1.0;
    s5[1][1] = 1.0;	s5[1][2] = 1.0;
    s5[2][1] = 0.5;	s5[2][2] = 1.5;
    s5[3][1] = 1.5;	s5[3][2] = 0.5;
    s5[4][1] = 1.0;	s5[4][2] = 1.0;

    for ( unsigned i = 1; i <= N_ENTRIES; ++i ) {
	prOt[i] = Q[5]->getPrOt( i );
	prOt[i][i][1][2] = initPrOt[i];
	prOt[i][i][0][2] = initPrOt[i];
    }

    phase2_correction = SIMPLE_PHASE2;

    switch ( ncust_ix ) {
    case NCUST_EQ_0:
	break;
    case NCUST_EQ_1:
    case NCUST_EQ_3:
    case NCUST_EQ_2:
	NCust[1] = ncust_ix;
	NCust[2] = ncust_ix;
	NCust[3] = ncust_ix;
	NCust[4] = ncust_ix;
	break;
    default:
	cerr << "Invalid S5 index (0-1):" << ncust_ix << endl;
	exit( 1 );
    }
	
    for ( unsigned k = 1; k <= classes; ++k ) {
	Q[k]->setService(k,sc[k].s).setVisits(k,1.0);
	Q[5]->setService(k,k,1,s5[k][1]).setService(k,k,2,s5[k][2]).setVisits(k,k,1,sc[k].v);	}
}


static double exactU[Ncust_COUNT][N_STATIONS+1][N_CLASSES+1] =
{
    { { 0,0,0,0,0 }, {0,0.03402, 0,0,0}, {0,0,0.12689, 0,0}, {0,0,0, 0.20953, 0}, {0,0,0,0, 0.06613,}, {0,0.27216, 0.25378, 0.20953, 0.26452 } }
};



void
special_check( ostream& output, const MVA& solver, const unsigned ncust_ix )
{
    const int width = output.precision() + 2;

    output << "By:              Solver GreatSPN      Difference" << endl;
    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    unsigned e;
			
	    if ( m == 5 ) {
		e = k;
	    } else {
		e = 1;
	    }
	    const double U1 = solver.U[n][m][e][k];
	    const double U2 = exactU[ncust_ix][m][k];

	    if ( U2 == 0.0 ) continue;

	    output << "U_" << m << e << k << solver.U[n][m][e][k];
	    output << ", " << setw(width) << U2;
	    output << ", delta = " << setw(width) << (fabs( U2 - U1 ) * 100.0 / U2) << endl;
	}
    }
}



static double goodL[Ncust_COUNT][4][N_STATIONS+1][N_CLASSES+1] =
{
#ifdef	NOTDEF
    { { { 0,0,0,0,0 }, {0, 0.03335, 0,0,0}, {0,0, 0.1244, 0,0}, {0,0,0, 0.2082, 0}, {0,0,0,0, 0.06478}, {0, 0.9666, 0.8756, 0.7918, 0.9352} },  
      { { 0,0,0,0,0 }, {0, 0.03409, 0,0,0}, {0,0, 0.1263, 0,0}, {0,0,0, 0.2109, 0}, {0,0,0,0, 0.0661,}, {0, 0.9659, 0.8737, 0.7891, 0.9339} },  
      { { 0,0,0,0,0 }, {0, 0.03409, 0,0,0}, {0,0, 0.1263, 0,0}, {0,0,0, 0.2109, 0}, {0,0,0,0, 0.0661,}, {0, 0.9659, 0.8737, 0.7891, 0.9339} },  
      { { 0,0,0,0,0 }, {0, 0.03279, 0,0,0}, {0,0, 0.1234, 0,0}, {0,0,0, 0.2082, 0}, {0,0,0,0, 0.06405}, {0, 0.9672, 0.8766, 0.7918, 0.9359} } },
#else
    { { { 0,0,0,0,0 }, {0, 0.03185, 0,0,0}, {0,0, 0.1268, 0,0}, {0,0,0, 0.1924, 0}, {0,0,0,0, 0.06599}, {0, 0.9682, 0.8732, 0.8076,  0.934} },  
      { { 0,0,0,0,0 }, {0, 0.03325, 0,0,0}, {0,0, 0.1306, 0,0}, {0,0,0, 0.1968, 0}, {0,0,0,0, 0.06863}, {0, 0.9667, 0.8694, 0.8032, 0.9314} },  
      { { 0,0,0,0,0 }, {0, 0.03325, 0,0,0}, {0,0, 0.1306, 0,0}, {0,0,0, 0.1968, 0}, {0,0,0,0, 0.06863}, {0, 0.9667, 0.8694, 0.8032, 0.9314} },  
      { { 0,0,0,0,0 }, {0, 0.03197, 0,0,0}, {0,0, 0.1277, 0,0}, {0,0,0, 0.1944, 0}, {0,0,0,0, 0.06642}, {0,  0.968, 0.8723, 0.8056, 0.9336} } },
#endif
};

bool
check( const int solverId, const MVA & solver, const unsigned ncust_ix )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    unsigned e;
			
	    if ( m == 5 ) {
		e = k;
	    } else {
		e = 1;
	    }
	    if ( fabs( solver.L[n][m][e][k] - goodL[ncust_ix][solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][e][k] << ", Correct= " << goodL[ncust_ix][solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}

