/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/test4.cc $
 *
 * Pg 267, Lazowska test.
 * FCFS with high variability in service times.
 *
 *
 * ------------------------------------------------------------------------
 * $Id: test4.cc 13413 2018-10-23 15:03:40Z greg $
 * ------------------------------------------------------------------------
 */

#include <cmath>
#include <stdlib.h>
#include "testmva.h"
#include "server.h"
#include "mva.h"
#include "pop.h"
#include "mva.h"


#define V5_EQ_QUARTER	0
#define V5_EQ_HALF	1
#define V5_EQ_1		2
#define V5_EQ_2		3
#define V5_EQ_4		4
#define V5_COUNT	5

#define N_STATIONS	6

void
test( Population& NCust, Vector<Server *>& Q, VectorMath<double>& Z, VectorMath<unsigned>& priority, const unsigned v5_ix )
{
    const unsigned classes  = 1;
    const unsigned stations = N_STATIONS;

    NCust.resize(classes);		/* Population vector.		*/
    Z.grow(classes);			/* Think times.			*/
    priority.grow(classes);
    Q.grow(stations);			/* Queue type.  SS/delay.	*/

    Q[1] = new FCFS_Server(classes);	/* Disk1 */
    Q[2] = new FCFS_Server(classes);	/* Disk2 */
    Q[3] = new FCFS_Server(classes);	/* Disk3 */
    Q[4] = new FCFS_Server(classes);	/* Disk4 */
    Q[5] = new HVFCFS_Server(classes);	/* CPU */
    Q[6] = new Infinite_Server(classes);	/* Terminals */

    NCust[1] = 10;	Z[1] = 0.0;

    /* Disks */

    Q[1]->setService(1,1.0).setVisits(1,2.0);
    Q[2]->setService(1,1.0).setVisits(1,2.0);
    Q[3]->setService(1,1.0).setVisits(1,2.0);
    Q[4]->setService(1,1.0).setVisits(1,2.0);

    /* CPU */

#define case5

    double variance;

    switch ( v5_ix ) {
		
    case V5_EQ_QUARTER:
	variance = 0.25;
	break;
    case V5_EQ_HALF:
	variance = 0.5;
	break;
    case V5_EQ_1:
	variance = 1.0;
	break;
    case V5_EQ_2:
	variance = 2.0;
	break;
    case V5_EQ_4:
	variance = 4.0;
	break;
    default:
	cerr << "Invalid V5 index (0-4):" << v5_ix << endl;
	exit( 1 );
    }

    Q[5]->setService(1,0.5).setVisits(1,8.0).setVariance(1,1,1,variance);

    /* Terminal */

    Q[6]->setService(1,10.0).setVisits(1,1.0);
}



void
special_check( ostream& output, const MVA& solver, const unsigned )
{
    const unsigned m = 6;
    const unsigned k = 1;

    output << "Terminal Response Time (by sum of R()) = " << solver.responseTime(  *solver.Q[m], k ) << endl;

    const double response_time = solver.NCust[k] / solver.throughput( m, k ) - solver.Q[m]->S(k);
    output << "Terminal Response Time (by throughput) = " << response_time << endl;
}


static double goodL[V5_COUNT][4][7][2] = {
    { { {0,0}, {0, 0.8384}, {0, 0.8384}, {0, 0.8384}, {0, 0.8384}, {0, 4.299}, {0, 2.348} },
      { {0,0}, {0, 0.8418}, {0, 0.8418}, {0, 0.8418}, {0, 0.8418}, {0, 4.282}, {0, 2.351} },
      { {0,0}, {0, 0.8418}, {0, 0.8418}, {0, 0.8418}, {0, 0.8418}, {0, 4.282}, {0, 2.351} },
      { {0,0}, {0, 0.7564}, {0, 0.7564}, {0, 0.7564}, {0, 0.7564}, {0, 4.725}, {0,  2.25} }
    },{   								 	    
	{ {0,0}, {0, 0.7484}, {0, 0.7484}, {0, 0.7484}, {0, 0.7484}, {0, 4.811}, {0, 2.196} },
	{ {0,0}, {0, 0.7562}, {0, 0.7562}, {0, 0.7562}, {0, 0.7562}, {0, 4.764}, {0, 2.211} },
	{ {0,0}, {0, 0.7562}, {0, 0.7562}, {0, 0.7562}, {0, 0.7562}, {0, 4.764}, {0, 2.211} },
	{ {0,0}, {0, 0.6932}, {0, 0.6932}, {0, 0.6932}, {0, 0.6932}, {0, 5.093}, {0, 2.134} }
    },{   								 	    
	{ {0,0}, {0, 0.6412}, {0, 0.6412}, {0, 0.6412}, {0, 0.6412}, {0, 5.438}, {0, 1.997} },
	{ {0,0}, {0, 0.6538}, {0, 0.6538}, {0, 0.6538}, {0, 0.6538}, {0, 5.361}, {0, 2.024} },
	{ {0,0}, {0, 0.6538}, {0, 0.6538}, {0, 0.6538}, {0, 0.6538}, {0, 5.361}, {0, 2.024} },
	{ {0,0}, {0, 0.6106}, {0, 0.6106}, {0, 0.6106}, {0, 0.6106}, {0, 5.588}, {0,  1.97} }
    },{   								 	    
	{ {0,0}, {0, 0.5286}, {0, 0.5286}, {0, 0.5286}, {0, 0.5286}, {0, 6.123}, {0, 1.763} },
	{ {0,0}, {0, 0.5427}, {0, 0.5427}, {0, 0.5427}, {0, 0.5427}, {0, 6.033}, {0, 1.796} },
	{ {0,0}, {0, 0.5427}, {0, 0.5427}, {0, 0.5427}, {0, 0.5427}, {0, 6.033}, {0, 1.796} },
	{ {0,0}, {0, 0.5153}, {0, 0.5153}, {0, 0.5153}, {0, 0.5153}, {0, 6.179}, {0,  1.76} }
    },{   								 
	{ {0,0}, {0, 0.421},  {0, 0.421},  {0, 0.421},  {0, 0.421},  {0,  6.81}, {0, 1.506} },
	{ {0,0}, {0, 0.4341}, {0, 0.4341}, {0, 0.4341}, {0, 0.4341}, {0, 6.724}, {0,  1.54} },
	{ {0,0}, {0, 0.4341}, {0, 0.4341}, {0, 0.4341}, {0, 0.4341}, {0, 6.724}, {0,  1.54} },
	{ {0,0}, {0, 0.4176}, {0, 0.4176}, {0, 0.4176}, {0, 0.4176}, {0, 6.813}, {0, 1.517} }
    }
};

bool
check( const int solverId, const MVA & solver, const unsigned v5_ix )
{
    bool ok = true;

    unsigned n = solver.offset(solver.NCust);
    for ( unsigned m = 1; m <= solver.M; ++m ) {
	for ( unsigned k = 1; k <= solver.K; ++k ) {
	    if ( fabs( solver.L[n][m][1][k] - goodL[v5_ix][solverId][m][k] ) >= 0.001 ) {
		cerr << "Mismatch at m=" << m <<", k=" << k;
		cerr << ".  Computed=" << solver.L[n][m][1][k] << ", Correct= " << goodL[v5_ix][solverId][m][k] << endl;
		ok = false;
	    }
	}
    }
    return ok;
}

