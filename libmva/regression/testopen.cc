/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/libmva/regression/testopen.cc $
 *
 * Example from:
 *
 * ------------------------------------------------------------------------
 * $Id: testopen.cc 15384 2022-01-25 02:56:14Z greg $
 * ------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cmath>
#include "testopen.h"
#include "open.h"
#include "server.h"
#include "pop.h"
#include "prob.h"
#include "vector.h"
#include "fpgoop.h"

static bool doIt( Vector<Server *>& Q, const unsigned special );
static bool run( const unsigned special );

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
    int status;
    unsigned long count = 1;
    unsigned special = 0;
	
    set_fp_abort();

    while (( c = getopt( argc, argv, "dsSvn:z:" )) != EOF) {
	switch( c ) {
	case 'd':
	    debug_flag = 1;
	    break;
			    
	case 'n':
	    count = atol( optarg );
	    if ( count == 0 ) {
		std::cerr << "Bogus loop count: " << optarg << std::endl;
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
	    if ( special == 0 ) {
		std::cerr << "Bogus \"special\": " << optarg << std::endl;
		exit( 1 );
	    }
	    break;
			
	default:
	    exit(1);
	}
    }

    if ( optind != argc ) {
	std::cerr << "Arg count." << std::endl;
    }

    status = 1;
    for ( unsigned long i = 0; i < count && status; ++i ) {
	status = run( special );
    }

    return !status;		/* 0 == o.k. for exit call. */
}


/*
 * run one interation.
 */

static bool
run( const unsigned special )
{
    unsigned stations;
    Vector<Server *> Q;
    int status = 1;

    try {
	test( Q, special );
    }
    catch ( runtime_error& error ) {
	std::cerr << "runtime error - " << error.what() << std::endl;
	return 0;
    }

    if ( debug_flag ) {
	for ( unsigned j = 1; j <= Q.size(); ++j ) {
	    std::cout << Q[j] << std::endl;
	}
    }

    status &= doIt( Q, special );

    return status;
}


/*
 * run solver.
 */

static bool
doIt( Vector<Server *>& Q, const unsigned special )
{
    bool ok = true;
    Open * model = new Open( Q );
    try {
	model->solve();
    }
    catch ( runtime_error& error ) {
	std::cerr << "runtime error - " << error.what() << std::endl;
	ok = false;
    }
    catch ( logic_error& error ) {
	std::cerr << "logic error - " << error.what() << std::endl;
	ok = false;
    }
    catch ( floating_point_error& error ) {
	std::cerr << "floating point error - " << error.what() << std::endl;
	ok = false;
    }
    if ( ok ) {
	if ( !silencio_flag ) {
	    std::cout << "Open solver." << std::endl;
	    std::cout.precision(4);
	    std::cout << *model;
	    special_check( std::cout, *model, special );
	}
	if ( verbose_flag ) {
	}

	if ( !nocheck_flag ) {
	    ok = check( std::cout, *model, special );
	}
    }
	
    delete model;
    return ok;
}
