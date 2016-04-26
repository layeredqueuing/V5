/*  -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/unit-test/test9i.cc $
 *
 * Vary the number of servers.  Test all multi-server models.
 *
 * ------------------------------------------------------------------------
 * $Id: test9i.cc 9164 2010-01-28 19:27:03Z greg $
 * ------------------------------------------------------------------------
 */

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include "testmva.h"
#include "mva.h"
#include "server.h"
#include "multserv.h"
#include "pop.h"
#include "prob.h"
#include "vector.h"
#include "fpgoop.h"

#define N_CUST		9	/* Tune me! */

#define	N_CLASSES	1
#define	N_STATIONS	2
#define N_ENTRIES	1

#define REISER		1
#define CONWAY		2
#define ROLIA		3
#define INFINITE	4

static bool doIt( solverId solver, Vector<Server *>& Q, const PopVector & N, const VectorMath<double>& thinkTime, const VectorMath<unsigned>& priority, const unsigned special, const unsigned n_servers );
static bool run( const unsigned solver_set, const unsigned special );
static void test( PopVector& N, Vector<Server *>& Q, const unsigned stnType, const unsigned n_servers );
static bool check( const MVA &model, const unsigned special, const unsigned solver, const unsigned n_servers );

/* -- */

static int silencio_flag = 0;			/* Don't print results if 1	*/
static int verbose_flag = 0;			/* Print iteration count if 1	*/
static int debug_flag = 0;			/* Print station info.		*/
static int nocheck_flag = 0;			/* Don't check if 1.		*/


/*
 * Main line.
 */

int
main (int argc, char *argv[])
{
    unsigned solver_set = 0;
    int c;
    int status;
    unsigned long count = 1;
    unsigned special = REISER;
	
    set_fp_abort();

    while (( c = getopt( argc, argv, "adelfbsStvn:z:" )) != EOF) {
	switch( c ) {
	case 'a':
	    solver_set = (unsigned)-1;
	    break;
			
	case 'b':
	    solver_set |= BARD_SCHWEITZER_SOLVER_BIT;
	    break;


	case 'd':
	    debug_flag = 1;
	    break;
			    
	case 'e':
	    solver_set |= EXACT_SOLVER_BIT;
	    break;
			
	case 'f':
	    solver_set |= LINEARIZER2_SOLVER_BIT;
	    break;

	case 'l':
	    solver_set |= LINEARIZER_SOLVER_BIT;
	    break;
			
	case 'n':
	    count = atol( optarg );
	    if ( count == 0 ) {
		cerr << "Bogus loop count: " << optarg << endl;
		exit( 1 );
	    }
	    break;
			
	case 's':
	    silencio_flag = 1;
	    break;

	case 'S':
	    nocheck_flag = 1;
	    break;
			
	case 'v':
	    verbose_flag = 1;
	    break;
			
	case 'z':
	    special = atoi( optarg );
	    if ( special <= 0 || INFINITE < special ) {
		cerr << "Bogus \"special\": " << optarg << endl;
		exit( 1 );
	    }
	    break;
			
	default:
	    exit(1);
	}
    }

    if ( !solver_set ) {
	solver_set = LINEARIZER2_SOLVER_BIT;
    }

    if ( optind != argc ) {
	cerr << "Arg count." << endl;
    }

    status = 1;
    for ( unsigned long i = 0; i < count && status; ++i ) {
	status = run( solver_set, special );
    }

    return !status;		/* 0 == o.k. for exit call. */
}


static const char * special_str[] = {
    "",
    "Reiser",
    "Conway",
    "Rolia",
    "Infinite"
};

static const char * names[] =
{
    "Exact MVA",
    "Linearizer",
    "Fast Linearizer",
    "Bard-Schweitzer",
};


/*
 * run one interation.
 */

static bool
run( const unsigned solver_set, const unsigned special )
{
    const unsigned classes  = N_CLASSES;
    const unsigned stations = N_STATIONS;

    PopVector N(classes);
    VectorMath<double> thinkTime(classes);
    VectorMath<unsigned> priority(classes);
    Vector<Server *> Q(stations);

    int status = 1;

    if ( !silencio_flag ) {
	cout << "Multi-server model is " << special_str[special] << endl;
    }
	
    for ( unsigned i = 0; i <= 3; ++i ) {
	if ( (1 << i) & solver_set ) {
	    if ( !silencio_flag ) {
		cout << names[i] << endl;
	    }
	    const unsigned max_n = (special == INFINITE) ? 1 : 10;
	    for ( unsigned n = 1; n <= max_n; ++n ) {
		try {
		    test( N, Q, special, n );
		}
		catch ( runtime_error& error ) {
		    cerr << "runtime error - " << error.what() << endl;
		    return 0;
		}

		if ( debug_flag ) {
		    for ( unsigned j = 1; j <= stations; ++j ) {
			cout << Q[j] << endl;
		    }
		}

		status &= doIt( (solverId)i, Q, N, thinkTime, priority, special, n );
	    }
	}
    }

    return status;
}


/*
 * run solver.
 */

static bool
doIt( solverId solver, Vector<Server *>& Q, const PopVector & N, const VectorMath<double> &thinkTime, const VectorMath<unsigned>& priority,
      const unsigned special, const unsigned n_servers )
{
    bool ok = true;
    MVA * model;

    switch ( solver ) {
    case EXACT_SOLVER:
	model = new ExactMVA( Q, N, thinkTime, priority );
	break;
    case LINEARIZER_SOLVER:
	model = new Linearizer( Q, N, thinkTime, priority );
	break;
    case LINEARIZER2_SOLVER:
	model = new Linearizer2( Q, N, thinkTime, priority );
	break;
    case BARD_SCHWEITZER_SOLVER:
	model = new Schweitzer( Q, N, thinkTime, priority );
	break;
    }

    try {
	model->solve();
    }
    catch ( runtime_error& error ) {
	cerr << "runtime error - " << error.what() << endl;
	ok = false;
    }
    catch ( logic_error& error ) {
	cerr << "logic error - " << error.what() << endl;
	ok = false;
    }
    catch ( floating_point_error& error ) {
	cerr << "floating point error - " << error.what() << endl;
	ok = false;
    }
    catch ( exception_handled& error ) {
    }
	
    if ( ok ) {
	if ( !silencio_flag ) {
	    cout << names[(int)solver] << " solver." << endl;
	    cout.precision(4);
	    cout << *model;
	    special_check( cout, *model, special );
	}
	if ( verbose_flag ) {
	cout << "Number of iterations of core step: " << model->iterations() << endl;
	}

	if ( !nocheck_flag ) {
	    ok = check( *model, special, solver, n_servers );
	}
    }
	
    delete model;
    return ok;
}

void
test( PopVector& N, Vector<Server *>& Q, const unsigned stnType, const unsigned n_servers )
{
    const unsigned int classes = N.size();

    N[1] = N_CUST;
    Q[1] = new Infinite_Server(N_ENTRIES,classes);

    if ( n_servers == 1 && stnType != INFINITE) {
	Q[2] = new FCFS_Server(N_ENTRIES,classes);
    } else switch ( stnType ) {
	default:
	case REISER:
	    Q[2] = new Reiser_Multi_Server(n_servers,N_ENTRIES,classes);
	    break;
	case CONWAY:
	    Q[2] = new Conway_Multi_Server(n_servers,N_ENTRIES,classes);
	    break;
	case ROLIA:
	    Q[2] = new Rolia_Multi_Server(n_servers,N_ENTRIES,classes);
	    break;
	case INFINITE:
	    Q[2] = new Infinite_Server(N_ENTRIES,classes);
	    break;
	} 

    Q[1]->setService(1,2.0).setVisits(1,1.0);
    Q[2]->setService(1,1.0).setVisits(1,1.0);
}

static double goodX[4][4][10] =
{
    /* Reiser */
    { { 0.999809, 1.95635, 2.61208, 2.88997, 2.97569, 2.9961,  2.9996,  2.99998, 3,       3 },		/* E */	
      { 0.997293, 1.95454, 2.64773, 2.91119, 3.00694, 3.0445,  3.0561,  3.05601, 3.05063, 3.04365 },	/* L */	
      { 0.997293, 1.95454, 2.64773, 2.91119, 3.00694, 3.0445,  3.0561,  3.05601, 3.05063, 3.04365 },	/* F */	
      { 0.97052,  1.84346, 2.50846, 2.85497, 2.9863,  3.03647, 3.05377, 3.05543, 3.05151, 3.04406 } },	/* B */	
    /* Conway */
    { { 0.999809, 1.95635, 2.61208, 2.88997, 2.97569, 2.9961,  2.9996,  2.99998, 3,       3 },
      { 0.997293, 1.84024, 2.45868, 2.80066, 2.94162, 2.98487, 2.99618, 2.99903, 3,       3 },
      { 0.997293, 1.84024, 2.45868, 2.80066, 2.94162, 2.98487, 2.99618, 2.99903, 3,       3 },
      { 0.97052,  1.79009, 2.42024, 2.79054, 2.94023, 2.98413, 2.99595, 2.99877, 3,       3 } },
    /* Rolia */
    { { 0.999809, 1.86676, 2.53924, 2.87918, 2.97739, 2.99658, 2.99956, 2.99995, 2.99999, 3 },
      { 0.997293, 2.04435, 2.64526, 2.89792, 2.97918, 2.99723, 2.9996,  2.99991, 2.99994, 2.99995 },
      { 0.997293, 2.04435, 2.64526, 2.89792, 2.97918, 2.99723, 2.9996,  2.99991, 2.99994, 2.99995 },
      { 0.97052,  1.92472, 2.56768, 2.88287, 2.97696, 2.99573, 2.99866, 2.99906, 2.9991,  2.99911 } },

    /* Infinite */
    { { 3 },
      { 3 },
      { 3 },
      { 3 } }
};

static bool
check( const MVA &model, const unsigned special, const unsigned solver, const unsigned n_servers )
{
    if ( fabs( goodX[special-1][solver][n_servers-1] - model.throughput(1) ) < 0.001 ) {
	return true;
    } else {
	cerr << "Mismatch at " << special
	     << ".  Computed=" << model.throughput(1)
	     << ", Correct= " << goodX[special-1][solver][n_servers-1] << endl;
	return false;
    }
}

void
special_check( ostream& output, const MVA& solver, const unsigned )
{
    output << ": X = " << setw(4) << solver.throughput(1)
	   << ", U" << setw(4) << solver.U[2][1][1];
}


