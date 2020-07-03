/*  -*- c++ -*-
 * $Id: test13s.cc 13413 2018-10-23 15:03:40Z greg $
 *
 * Suri Multiserver test.
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include <stdlib.h>
#include "testmva.h"
#include "server.h"
#include "multserv.h"
#include "pop.h"
#include "mva.h"

#define	N_CLASSES	1
#define	N_STATIONS	3
#define N_ENTRIES	1

#define N_TESTS 9
static double service_time[][N_TESTS] = {
    { 2, 2, 2, 1, 4, 1, 4, 1, 4 },
    { 2, 4, 8, 2, 2, 4, 4, 8, 8 }
};

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned ix )
{
	const unsigned classes  = N_CLASSES;
	const unsigned stations = N_STATIONS;

	if ( ix >= N_TESTS ) {
	    cerr << "Index is too big. (0 < t <= 8)" << endl;
	    exit( 1 );
	}
	    
	NCust.resize(classes);			/* Population vector.		*/
	Z.grow(classes);			/* Think times.			*/
	priority.grow(classes);
	Q.grow(stations);			/* Queue type.  SS/delay.	*/

	NCust[1] = 8;
	
	Q[1] = new PS_Server(N_ENTRIES,classes);
	Q[2] = new Suri_Multi_Server(2,N_ENTRIES,classes);
	Q[3] = new Suri_Multi_Server(4,N_ENTRIES,classes);

	Q[1]->setService(1,1.0).setVisits(1,1.0);
	Q[2]->setService(1,service_time[0][ix]).setVisits(1,1.0);
	Q[3]->setService(1,service_time[1][ix]).setVisits(1,1.0);

}


void
special_check( ostream&, const MVA&, const unsigned )
{
}


static double goodL[][4][N_STATIONS+1][N_CLASSES+1] =
{
    {	/* 0 */
        { { 0,0 }, { 0, 4.122 }, { 0, 1.939 }, { 0, 1.939 }},	// Exact
        { { 0,0 }, { 0, 3.057 }, { 0, 3.238 }, { 0, 1.705 }},	// Linearizer 
        { { 0,0 }, { 0, 3.057 }, { 0, 3.238 }, { 0, 1.705 }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 3.179 }, { 0, 3.137 }, { 0, 1.684 }}  	// Bard-Schweitzer
    },{	/* 1 */
        { { 0,0 }, { 0, 2.731 }, { 0, 1.756 }, { 0, 3.512 }},	// Exact
        { { 0,0 }, { 0, 2.192 }, { 0, 2.316 }, { 0, 3.492 }}, 	// Linearizer 
        { { 0,0 }, { 0, 2.192 }, { 0, 2.316 }, { 0, 3.492 }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 2.309 }, { 0, 2.275 }, { 0, 3.416 }}	// Bard-Schweitzer
    },{	/* 2 */
        { { 0,0 }, { 0, 0.8712}, { 0, 1.198 }, { 0, 5.931 }},	// Exact
        { { 0,0 }, { 0, 0.8964}, { 0, 1.274 }, { 0, 5.83  }}, 	// Linearizer 
        { { 0,0 }, { 0, 0.8964}, { 0, 1.274 }, { 0, 5.83  }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 0.8048}, { 0, 1.148 }, { 0, 6.047 }}	// Bard-Schweitzer
    },{	/* 3 */
        { { 0,0 }, { 0, 4.712 }, { 0, 1.23  }, { 0, 2.058 }},	// Exact
        { { 0,0 }, { 0, 4.67  }, { 0, 1.254 }, { 0, 2.077 }}, 	// Linearizer 
        { { 0,0 }, { 0, 4.67  }, { 0, 1.254 }, { 0, 2.077 }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 5.003 }, { 0, 1.125 }, { 0, 1.872 }}	// Bard-Schweitzer
    },{	/* 4 */
        { { 0,0 }, { 0, 0.9489}, { 0, 6.054 }, { 0, 0.9973}},	// Exact
        { { 0,0 }, { 0, 0.9342}, { 0, 6.069 }, { 0, 0.997 }}, 	// Linearizer 
        { { 0,0 }, { 0, 0.9342}, { 0, 6.069 }, { 0, 0.997  }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 0.8329}, { 0, 6.227 }, { 0, 0.9404}}	// Bard-Schweitzer
    },{	/* 5 */
        { { 0,0 }, { 0, 2.696 }, { 0, 0.9718}, { 0, 4.332 }},	// Exact
        { { 0,0 }, { 0, 2.798 }, { 0, 1.004 }, { 0, 4.199 }}, 	// Linearizer 
        { { 0,0 }, { 0, 2.798 }, { 0, 1.004 }, { 0, 4.199 }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 2.944 }, { 0, 0.9561}, { 0, 4.1   }}	// Bard-Schweitzer
    },{	/* 6 */
        { { 0,0 }, { 0, 0.8932}, { 0, 5.058 }, { 0, 2.049 }},	// Exact
        { { 0,0 }, { 0, 0.8849}, { 0, 5.03  }, { 0, 2.085 }}, 	// Linearizer 
        { { 0,0 }, { 0, 0.8849}, { 0, 5.03  }, { 0, 2.085 }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 0.7921}, { 0, 5.325 }, { 0, 1.883 }}	// Bard-Schweitzer
    },{	/* 7 */
        { { 0,0 }, { 0, 0.9202}, { 0, 0.5221}, { 0, 6.558 }},	// Exact
        { { 0,0 }, { 0, 0.9346}, { 0, 0.5377}, { 0, 6.528 }}, 	// Linearizer 
        { { 0,0 }, { 0, 0.9346}, { 0, 0.5377}, { 0, 6.528 }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 0.8359}, { 0, 0.5077}, { 0, 6.656 }}	// Bard-Schweitzer
    },{	/* 8 */
        { { 0,0 }, { 0, 0.6681}, { 0, 3.078 }, { 0, 4.254 }},	// Exact
        { { 0,0 }, { 0, 0.6684}, { 0, 3.165 }, { 0, 4.167 }}, 	// Linearizer 
        { { 0,0 }, { 0, 0.6684}, { 0, 3.165 }, { 0, 4.167 }}, 	// Fast Linearizer 
        { { 0,0 }, { 0, 0.643 }, { 0, 3.265 }, { 0, 4.093 }}	// Bard-Schweitzer
    }
};

bool
check( const int solverId, const MVA & solver, const unsigned v22_ix )
{
	bool ok = true;

	unsigned n = solver.offset(solver.NCust);
	for ( unsigned m = 1; m <= solver.M; ++m ) {
		for ( unsigned k = 1; k <= solver.K; ++k ) {
			if ( fabs( solver.L[n][m][1][k] - goodL[v22_ix][solverId][m][k] ) >= 0.001 ) {
				cerr << "Mismatch at m=" << m <<", k=" << k;
				cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[v22_ix][solverId][m][k] << endl;
				ok = false;
			}
		}
	}
	return ok;
}

