/*  -*- c++ -*-
 * $HeadURL$
 *
 * Example from:
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#include "testmva.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmath>
#include "mva.h"
#include "open.h"
#include "server.h"
#include "pop.h"
#include "prob.h"
#include "vector.h"
#include "fpgoop.h"

static bool doIt( solverId solver, Vector<Server *>& Q, const PopVector & NCust, const VectorMath<double>& thinkTime, const VectorMath<unsigned>& priority, const unsigned special );
static bool run( const unsigned solver_set, const unsigned special );

/* -- */

static int silencio_flag = 0;			/* Don't print results if 1	*/
static int verbose_flag = 0;			/* Print iteration count if 1	*/
static int debug_flag = 0;			/* Print station info.		*/
static int nocheck_flag = 0;			/* Don't check if 1.		*/


/*
 * Main line.
 */

int main (int argc, char *argv[])
{
    unsigned solver_set = 0;
    int c;
    bool ok;
    unsigned long count = 1;
    unsigned special = 0;
	
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
	    if ( sscanf( optarg, "%lu", &count ) != 1 ) {
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
	    if ( sscanf( optarg, "%u", &special ) != 1 ) {
		cerr << "Bogus \"special\": " << optarg << endl;
		exit( 1 );
	    }
	    break;
			
	default:
	    exit(1);
	}
    }

    if ( !solver_set ) {
	solver_set = LINEARIZER_SOLVER_BIT;
    }

    if ( optind != argc ) {
	cerr << "Arg count." << endl;
    }

    ok = true;
    for ( unsigned long i = 0; i < count && ok; ++i ) {
	ok = run( solver_set, special );
    }
    if ( !ok ) { 
	cerr << argv[0] << " fails." << endl;
	return 1;
    } else {
	return 0;
    }
}


/*
 * run one iteration.
 */

static bool 
run( const unsigned solver_set, const unsigned special )
{
    bool ok = true;

    for ( unsigned i = 0; i <= 3; ++i ) {
	PopVector N(0);
	VectorMath<double> Z(0);
	VectorMath<unsigned> priority(0);
	Vector<Server *> Q;
	try {
	    test( N, Q, Z, priority, special );
	}
	catch ( runtime_error& error ) {
	    cerr << "runtime error - " << error.what() << endl;
	    return 0;
	}
	
	if ( debug_flag ) {
	    for ( unsigned j = 1; j <= Q.size(); ++j ) {
		cout << *Q[j] << endl;
	    }
	}

	if ( (1 << i) & solver_set ) {
	    ok = ok && doIt( (solverId)i, Q, N, Z, priority, special );
	}
    }

    return ok;
}


/*
 * run solver.
 */

static const char * names[] =
{
    "Exact MVA",
    "Linearizer",
    "Fast Linearizer",
    "Bard-Schweitzer",
};

static bool
doIt( solverId solver, Vector<Server *>& Q, const PopVector & N, const VectorMath<double> &Z, const VectorMath<unsigned>& priority,
      const unsigned special )
{
    bool ok = true;
    MVA * closedModel;
    Open * openModel = new Open( Q );

    try {
	openModel->convert( N );
    }
    catch ( range_error& error ) {
	cerr << "range error - " << error.what() << endl;
	ok = false;
    }

    switch ( solver ) {
    case EXACT_SOLVER:
	closedModel = new ExactMVA( Q, N, Z, priority );
	break;
    case LINEARIZER_SOLVER:
	closedModel = new Linearizer( Q, N, Z, priority );
	break;
    case LINEARIZER2_SOLVER:
	closedModel = new Linearizer2( Q, N, Z, priority );
	break;
    case BARD_SCHWEITZER_SOLVER:
	closedModel = new Schweitzer( Q, N, Z, priority );
	break;
    }

    try {
	closedModel->solve();
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
    try {
	openModel->solve( *closedModel, N );	/* Calculate L[0] queue lengths. */
    }
    catch ( runtime_error& error ) {
	cerr << "runtime error - " << error.what() << endl;
	ok = false;
    }

	
    if ( ok ) {
	if ( !silencio_flag ) {
	    cout << names[(int)solver] << " solver." << endl;
	    cout.precision(4);
	    cout << *openModel;
	    cout << *closedModel;
	    special_check( cout, *closedModel, special );
	}
	if ( verbose_flag ) {
	    cout << "Number of iterations of core step: " << closedModel->iterations() << endl;
	}

	if ( !nocheck_flag ) {
	    ok = check( (int)solver, *closedModel, special );
	}
    }
	
    delete closedModel;
    return ok;
}
