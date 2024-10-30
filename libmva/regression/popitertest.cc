/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/libmva/regression/popitertest.cc $
 *
 * Population iterator testor.  See usage().
 * ------------------------------------------------------------------------
 *
 * $Id: popitertest.cc 15384 2022-01-25 02:56:14Z greg $
 */

#include "testmva.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "multserv.h"

char * myName;

static void usage ()
{
	std::cerr << myName << " [-a [-m<servers>]] [-b [-m<servers>] [-j<class>]] n1 n2 n3 ... " << std::endl;
	exit( 1 );
}

int main ( int argc, char * argv[] )
{
	int c;
	unsigned m = 1;
	unsigned j = 1;
	Population::Iterator * next;
	Server * aServer = 0;
	enum { A_POP, B_POP, GENERAL_POP } type = GENERAL_POP;

	myName = argv[0];
	
	while (( c = getopt( argc, argv, "abm:j:" )) != EOF) {
		switch ( c ) {
		case 'a':
			type = A_POP;
			break;

		case 'b':
			type = B_POP;
			break;

		case 'j':
			j = atoi( optarg );
			if ( !j ) {
				std::cerr << "Bad value for j:" << optarg << std::endl;
				usage();
			}
			break;

		case'm':
			m = atoi( optarg );
			if ( !m ) {
				std::cerr << "Bad value for m:" << optarg << std::endl;
				usage();
			}
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

	Population NCust(k);		// Limit.	
	Population N(k);			// Current.

	for ( unsigned i = optind; i < argc; ++i ) {
		NCust[i-optind+1] = atoi( argv[i] );
	}
	std::cout << "Limit: " << NCust << std::endl;

	switch ( type ) {
	case A_POP:
	    aServer = new Conway_Multi_Server( m );
	    next = new Conway_Multi_Server::A_Iterator( *aServer, j, NCust, 0 );
	    break;

	case B_POP:
	    aServer = new Conway_Multi_Server( m );
	    next = new Conway_Multi_Server::B_Iterator( *aServer, NCust, 0 );
	    break;

	default:
	    next = new Population::Iterator( NCust );
	    break;
	}

	
	for ( unsigned count = 1; (* next)( N ); ++count ) {
		std::cout << std::setw(3) << count << ": " << N << std::endl;
	}
	if ( aServer ) {
	    delete aServer;
	}
	return 0;
}
