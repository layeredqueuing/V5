/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/libmva/regression/poptest.cc $
 *
 * Population test ( for X[N] ).
 * ------------------------------------------------------------------------
 *
 * $Id: poptest.cc 15384 2022-01-25 02:56:14Z greg $
 */

#include "testmva.h"
#include <vector>
#include <set>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "pop.h"
#include "vector.h"

char * myName;

static void usage ()
{
    std::cerr << myName << " [-[els]] n1 n2 n3 ... " << std::endl;
    exit( 1 );
}

int main ( int argc, char * argv[] )
{
    typedef enum { EXACT, LINEARIZER, SCHWEITZER } test_case_t;
    std::set<test_case_t> test_case;

    myName = argv[0];
	
    for ( ;; ) {
	const int c = getopt( argc, argv, "els" );
	if ( c == EOF) break;
	
	switch ( c ) {
	case 'e':
	    test_case.insert( EXACT );
	    break;

	case 'l':
	    test_case.insert( LINEARIZER );
	    break;

	case 's':
	    test_case.insert( SCHWEITZER );
	    break;

	default:
	    std::cerr << "Unkown option." << std::endl;
	    usage();
	}
    }
	    

    const unsigned k = argc - optind;

    if ( k == 0 ) {
	std::cerr << "Arg count." << std::endl;
	usage();
    }

    Population n(k);		// Limit.	
    Population N(k);			// Current.

    for ( unsigned i = optind; i < argc; ++i ) {
	N[i-optind+1] = atoi( argv[i] );
    }
    std::cout << "Limit: " << N << std::endl;

    /* Iterate over all populations. */

    return 0;
}
