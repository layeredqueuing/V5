/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/libmva/regression/test9c.cc $
 *
 * Multiserver test.  From Conway.
 *
 * ------------------------------------------------------------------------
 * $Id: test9c.cc 15384 2022-01-25 02:56:14Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include <stdlib.h>
#include "testmva.h"
#include "server.h"
#include "multserv.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	4
#define	N_STATIONS	6
#define N_ENTRIES	1

static void setVisits( Vector<Server *>& Q );

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;
	
    NCust.resize(classes);			/* Population vector.		*/
    Z.resize(classes);			/* Think times.			*/
    priority.resize(classes);
    Q.resize(stations);			/* Queue type.  SS/delay.	*/

    NCust[1] = 9;
    NCust[2] = 7;
    NCust[3] = 4;
    NCust[4] = 6;

    Q[1] = new Conway_Multi_Server(5,N_ENTRIES,classes);
    Q[2] = new Conway_Multi_Server(2,N_ENTRIES,classes);
    Q[3] = new Conway_Multi_Server(10,N_ENTRIES,classes);
    Q[4] = new FCFS_Server(N_ENTRIES,classes);
    Q[5] = new FCFS_Server(N_ENTRIES,classes);
    Q[6] = new Infinite_Server(N_ENTRIES,classes);

    Q[1]->setService( 1, 65.83 );
    Q[2]->setService( 1, 72.18 );
    Q[3]->setService( 1, 56.73 );
    Q[4]->setService( 1, 19.62 );
    Q[5]->setService( 1,  7.14 );
    Q[6]->setService( 1,  1.57 );
				       
    Q[1]->setService( 2, 64.09 );
    Q[2]->setService( 2, 62.98 );
    Q[3]->setService( 2, 55.56 );
    Q[4]->setService( 2,  5.23 );
    Q[5]->setService( 2,  5.15 );
    Q[6]->setService( 2, 14.89 );
				       
    Q[1]->setService( 3, 72.39 );
    Q[2]->setService( 3, 64.10 );
    Q[3]->setService( 3, 57.99 );
    Q[4]->setService( 3, 15.11 );
    Q[5]->setService( 3, 12.94 );
    Q[6]->setService( 3,  2.55 );
				       
    Q[1]->setService( 4, 51.73 );
    Q[2]->setService( 4, 56.23 );
    Q[3]->setService( 4, 53.51 );
    Q[4]->setService( 4, 10.60 );
    Q[5]->setService( 4,  7.78 );
    Q[6]->setService( 4, 10.43 );

    setVisits( Q );

}


void
special_check( std::ostream&, const MVA&, const unsigned )
{
}



static double goodL [4][N_STATIONS+1][N_CLASSES+1] =
{
    { {0,0,0,0,0 }, {0, 0.7892, 0.6245, 0.4709, 0.1712 }, {0,  7.404,   5.72,  3.099,  5.151 }, {0, 0.4523, 0.3232, 0.2573, 0.3373 }, {0, 0.2298, 0.09574,0.1094, 0.1954 }, {0, 0.1095, 0.05334,0.05317,0.04473 }, {0, 0.01565,0.1832, 0.01003,0.1007 }  },	// Exact
    { {0,0,0,0,0 }, {0, 0.7768,  0.615, 0.4652, 0.1675 }, {0,  7.421,  5.734,  3.107,  5.159 }, {0, 0.4501, 0.3217, 0.2562, 0.3355 }, {0, 0.2281, 0.09467,0.1086, 0.1936 }, {0, 0.1088, 0.05297,0.05291,0.04442 }, {0, 0.01557,0.1823, 0.00999,0.1002 }  },	// Linearizer
    { {0,0,0,0,0 }, {0, 0.7768,  0.615, 0.4652, 0.1675 }, {0,  7.421,  5.734,  3.107,  5.159 }, {0, 0.4501, 0.3217, 0.2562, 0.3355 }, {0, 0.2281, 0.09467,0.1086, 0.1936 }, {0, 0.1088, 0.05297,0.05291,0.04442 }, {0, 0.01557,0.1823, 0.00999,0.1002 }  },	// Fast Linearizer
    { {0,0,0,0,0 }, {0, 0.7707, 0.6102, 0.4622, 0.1662 }, {0,  7.438,  5.748,  3.115,   5.17 }, {0, 0.4467, 0.3194, 0.2547, 0.3331 }, {0, 0.2224, 0.08927,0.1058, 0.1872 }, {0, 0.1066, 0.05172,0.05232,0.04363 }, {0, 0.01545, 0.181, 0.00993,0.09952 } }	// Bard-Schweitzer
};

bool
check( const int solverId, const MVA & solver, const unsigned )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    if ( fabs( solver.L[n][m][1][k] - goodL[solverId][m][k] ) >= 0.001 ) {
		std::cerr << "Mismatch at m=" << m <<", k=" << k
		     << ".  Computed=" << solver.L[n][m][1][k]
		     << ", Correct= " << goodL[solverId][m][k] << std::endl;
		ok = false;
	    }
	}
    }
    return ok;
}


/* 		i  k  j */
double route_Pr[6][4][6] = {
    { { 0.19, 0.19, 0.19, 0.08, 0.15, 0.19 },
      { 0.10, 0.10, 0.16, 0.02, 0.22, 0.41 },
      { 0.31, 0.14, 0.13, 0.31, 0.07, 0.04 },
      { 0.19, 0.15, 0.22, 0.21, 0.05, 0.20 } },

    { { 0.27, 0.05, 0.09, 0.05, 0.30, 0.23 },
      { 0.17, 0.10, 0.26, 0.07, 0.18, 0.21 },
      { 0.21, 0.03, 0.34, 0.16, 0.11, 0.15 },
      { 0.03, 0.08, 0.17, 0.25, 0.15, 0.32 } },

    { { 0.17, 0.24, 0.14, 0.17, 0.21, 0.06 },
      { 0.22, 0.04, 0.19, 0.16, 0.15, 0.25 },
      { 0.21, 0.24, 0.15, 0.07, 0.23, 0.09 },
      { 0.03, 0.32, 0.12, 0.30, 0.12, 0.11 } },

    { { 0.26, 0.25, 0.16, 0.14, 0.22, 0.07 },
      { 0.15, 0.21, 0.01, 0.29, 0.09, 0.25 },
      { 0.16, 0.25, 0.05, 0.29, 0.10, 0.16 },
      { 0.03, 0.02, 0.13, 0.32, 0.17, 0.33 } },

    { { 0.11, 0.15, 0.17, 0.19, 0.19, 0.19 },
      { 0.50, 0.30, 0.01, 0.06, 0.01, 0.12 },
      { 0.24, 0.17, 0.22, 0.10, 0.08, 0.19 },
      { 0.14, 0.32, 0.24, 0.04, 0.14, 0.13 } },

    { { 0.18, 0.19, 0.05, 0.19, 0.12, 0.26 },
      { 0.01, 0.25, 0.07, 0.20, 0.23, 0.24 },
      { 0.27, 0.15, 0.08, 0.08, 0.19, 0.23 },
      { 0.05, 0.21, 0.04, 0.36, 0.03, 0.32 } }
};


/*
 * V(1,1) = 1.18
 * V(2,1) = 1.07
 * V(3,1) = 0.8
 * V(4,1) = 0.82
 * V(5,1) = 1.19
 * V(6,1) = 1
 *
 * V(1,2) = 1.15
 * V(2,2) = 1
 * V(3,2) = 0.7
 * V(4,2) = 0.8
 * V(5,2) = 0.88
 * V(6,2) = 1.48
 *
 * V(1,3) = 1.4
 * V(2,3) = 0.98
 * V(3,3) = 0.97
 * V(4,3) = 1.01
 * V(5,3) = 0.78
 * V(6,3) = 0.86
 *
 * V(1,4) = 0.47
 * V(2,4) = 1.1
 * V(3,4) = 0.92
 * V(4,4) = 1.48
 * V(5,4) = 0.66
 * V(6,4) = 1.41
 */

static void
setVisits( Vector<Server *>& Q )
{
    for ( unsigned k = 0; k < N_CLASSES; ++k ) {
	for ( unsigned j = 0; j < N_STATIONS; ++j ) {
	    double sum = 0.0;

	    for ( unsigned i = 0; i < N_STATIONS; ++i ) {
		sum += route_Pr[i][k][j];
	    }
//			std::cerr << "V(" << j+1 << ',' << k+1 << ") = " << sum << std::endl;

	    Q[j+1]->setVisits(k+1,sum);

	}
    }
}
