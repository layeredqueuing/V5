/*  -*- c++ -*-
 * $HeadURL$
 *
 * Pg 264, Lazowska test.
 * FCFS with class dependent average service times. 
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include <stdlib.h>
#include "testmva.h"
#include "server.h"
#include "mva.h"
#include "pop.h"
#include "mva.h"


#define S5K_EQ_HALF	0
#define S5K_EQ_2	1
#define S5K_EQ_8	2
#define S5K_EQ_32	3
#define S5K_EQ_128	4
#define S5K_COUNT	5

#define N_STATIONS	6

void
test( PopVector& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned s5k_ix )
{
    const unsigned classes  = 2;
    const unsigned stations = N_STATIONS;
    
    Z.grow(classes);				/* Think times.			*/
    NCust.grow(classes);			/* Population vector.		*/
    priority.grow(classes);
    Q.grow(stations);				/* Queue type.  SS/delay.	*/

    Q[1] = new FCFS_Server(classes);		/* Disk1 */
    Q[2] = new FCFS_Server(classes);		/* Disk2 */
    Q[3] = new FCFS_Server(classes);		/* Disk3 */
    Q[4] = new FCFS_Server(classes);		/* Disk4 */
    Q[5] = new FCFS_Server(classes);		/* CPU */
    Q[6] = new Infinite_Server(classes);	/* Terminals */

    NCust[1] = 10;	Z[1] = 0.0;
    NCust[2] = 6;	Z[2] = 0.0;

    for ( unsigned i = 1; i <= 4; ++i ) {
	Q[i]->setService(1,1,1,1.0).setVisits(1,1,1,2.0);
	Q[i]->setService(1,2,1,1.0).setVisits(1,2,1,2.0*(double)i);
    }
							      	 
    /* CPU */

    double service = 0.0;
    switch ( s5k_ix ) {
    case S5K_EQ_HALF:
	service = 2.0;
	break;
    case S5K_EQ_2:
	service = 0.5;
	break;
    case S5K_EQ_8:
	service = 0.125;
	break;
    case S5K_EQ_32:
	service = (1.0/32.0);
	break;
    case S5K_EQ_128:
	service = (1.0/128.0);
	break;
    default:
	cerr << "Invalid S5K index (0-4):" << s5k_ix << endl;
	exit( 1 );
    }

    Q[5]->setService(1,1,1,service).setVisits(1,1,1,8.0);
    Q[5]->setService(1,2,1,2.0).setVisits(1,2,1,20.0);
							      
    /* Terminal */					      
							      
    Q[6]->setService(1,1,1,10.0).setVisits(1,1,1,1.0);
    Q[6]->setService(1,2,1,0.0).setVisits(1,2,1,0.0);
}


void special_check( ostream& output, const MVA& solver, const unsigned )
{
    const unsigned m = 6;
    const unsigned k = 1;

    output << "Terminal Response Time (by sum of R()) = " << solver.responseTime(  *solver.Q[m], k ) << endl;

    const double response_time = solver.NCust[k] / solver.throughput( m, k ) - solver.Q[m]->S(k);
    output << "Terminal Response Time (by throughput) = " << response_time << endl;
}


static double goodL[S5K_COUNT][4][7][3] =
{
    {
        { { 0, 0, 0 }, {0, 0.08489, 0.02136}, {0, 0.08687, 0.04355}, {0, 0.08893, 0.06661}, {0, 0.09108, 0.0906},  {0,   9.264,  5.778}, {0, 0.3845,  0} },
	{ { 0, 0, 0 }, {0, 0.08487, 0.02135}, {0, 0.08684, 0.04353}, {0, 0.08889, 0.06658}, {0, 0.09103, 0.09055}, {0,   9.264,  5.778}, {0, 0.3845,  0} },
	{ { 0, 0, 0 }, {0, 0.08487, 0.02135}, {0, 0.08684, 0.04353}, {0, 0.08889, 0.06658}, {0, 0.09103, 0.09055}, {0,   9.264,  5.778}, {0, 0.3845,  0} },
	{ { 0, 0, 0 }, {0, 0.08407, 0.02112}, {0, 0.08589, 0.04301}, {0, 0.08779, 0.06573}, {0, 0.08978, 0.08934}, {0,   9.269,  5.781}, {0, 0.3832,  0} }
    },{
        { { 0, 0, 0 }, {0, 0.1675, 0.04471},  {0,  0.1755, 0.09352}, {0,  0.1842,  0.147 }, {0,  0.1939,  0.2058}, {0,   8.58,   5.509}, {0, 0.6988,  0} },
	{ { 0, 0, 0 }, {0, 0.1674, 0.04459},  {0,  0.1753, 0.09315}, {0,  0.1839,  0.1462}, {0,  0.1934,  0.2043}, {0,   8.582,  5.512}, {0, 0.6985,  0} },
	{ { 0, 0, 0 }, {0, 0.1674, 0.04459},  {0,  0.1753, 0.09315}, {0,  0.1839,  0.1462}, {0,  0.1934,  0.2043}, {0,   8.582,  5.512}, {0, 0.6985,  0} },
	{ { 0, 0, 0 }, {0, 0.1659, 0.04286},  {0,  0.1733, 0.08899}, {0,  0.1812,  0.1388}, {0,  0.1898,  0.1927}, {0,   8.594,  5.537}, {0, 0.6958,  0} }
    },{
	{ { 0, 0, 0 }, { 0, 0.2196,  0.06129 }, { 0,  0.2337,   0.131 }, { 0,  0.2498,  0.2112 }, { 0, 0.2685,  0.3045 }, { 0,  8.154,   5.292 },  { 0,  0.8744,  0 } },
	{ { 0, 0, 0 }, { 0, 0.2191,  0.06057 }, { 0,  0.2329,  0.1289 }, { 0,  0.2487,  0.2065 }, { 0, 0.2666,  0.2953 }, { 0,   8.16,   5.309 },  { 0,  0.8731,  0 } },
	{ { 0, 0, 0 }, { 0, 0.2191,  0.06057 }, { 0,  0.2329,  0.1289 }, { 0,  0.2487,  0.2065 }, { 0, 0.2666,  0.2953 }, { 0,   8.16,   5.309 },  { 0,  0.8731,  0 } },
	{ { 0, 0, 0 }, { 0, 0.2174,  0.05674 }, { 0,  0.2303,  0.1193 }, { 0,  0.2447,  0.1888 }, { 0, 0.2607,  0.2663 }, { 0,  8.179,   5.369 },  { 0,  0.8679,  0 } }
    },{
        { { 0, 0, 0 }, {0,  0.238, 0.06759},  {0, 0.2547,  0.1458},  {0, 0.2742,  0.2374},  {0, 0.2972,  0.3465},  {0,  8.003,   5.203}, {0, 0.9332,  0} },
	{ { 0, 0, 0 }, {0, 0.2372, 0.06641},  {0, 0.2536,  0.1423},  {0, 0.2724,  0.2299},  {0, 0.2942,  0.3316},  {0,  8.012,    5.23}, {0,  0.931,  0} },
	{ { 0, 0, 0 }, {0, 0.2372, 0.06641},  {0, 0.2536,  0.1423},  {0, 0.2724,  0.2299},  {0, 0.2942,  0.3316},  {0,  8.012,    5.23}, {0,  0.931,  0} },
	{ { 0, 0, 0 }, {0, 0.2353, 0.06156},  {0, 0.2505,    0.13},  {0, 0.2676,  0.2067},  {0, 0.2869,  0.2932},  {0,  8.036,   5.308}, {0, 0.9239,  0} }
    },{
        { { 0, 0, 0 }, {0,  0.2431, 0.06937}, {0,  0.2606,  0.1499}, {0,  0.2811,  0.2449}, {0,  0.3053,  0.3587}, {0,  7.961,   5.177}, {0, 0.9491,  0} },
	{ { 0, 0, 0 }, {0,  0.2422, 0.06805}, {0,  0.2593,  0.1461}, {0,  0.2791,  0.2365}, {0,  0.3021,  0.3421}, {0,  7.971,   5.207}, {0, 0.9467,  0} }, 
	{ { 0, 0, 0 }, {0,  0.2422, 0.06805}, {0,  0.2593,  0.1461}, {0,  0.2791,  0.2365}, {0,  0.3021,  0.3421}, {0,  7.971,   5.207}, {0, 0.9467,  0} }, 
	{ { 0, 0, 0 }, {0,  0.2402, 0.06288}, {0,  0.2561,   0.133}, {0,   0.274,  0.2117}, {0,  0.2943,  0.3008}, {0,  7.996,   5.292}, {0,  0.939,  0} }
    }
};

bool
check( const int solverId, const MVA & solver, const unsigned s5k_ix )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    if ( fabs( solver.L[n][m][1][k] - goodL[s5k_ix][solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[s5k_ix][solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}

