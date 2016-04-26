/*  -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/unit-test/disttest.cc $
 *
 * Distribution function tests.
 * ------------------------------------------------------------------------
 *
 * $Id: disttest.cc 9164 2010-01-28 19:27:03Z greg $
 */

#include "testmva.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "randomvar.h"
#include "vector.h"
#include "cltn.h"

char * myName;

static void usage ()
{
    cerr << myName << " " << endl;
    exit( 1 );
}

int main ( int argc, char * argv[] )
{
    int c;

    myName = argv[0];
	
    while (( c = getopt( argc, argv, "" )) != EOF) {
	switch ( c ) {
	default:
	    cerr << "Unkown option." << endl;
	    usage();
	}
    }

    VectorMath<double> t1(6);
    VectorMath<double> A1(6);
    const double u1 = 1.0 / 6.0;;

    VectorMath<double> t2(4);
    VectorMath<double> A2(4);
    const double u2 = 1.0 / 4.0;;

    for ( unsigned i = 1; i <= 6; ++i ) {
	t1[i] = i;
	A1[i] = u1 * static_cast<double>(i);
    }
	
    for ( unsigned i = 1; i <= 4; ++i ) {
	t2[i] = i;
	A2[i] = u2 * static_cast<double>(i);
    }
	
    DiscretePoints cdf1;
    cdf1.setCDF( t1, A1 );
    cdf1.meanVar();
    cout << cdf1 << endl;
    DiscretePoints cdf2;
    cdf2.setCDF( t2, A2 );
    cdf2.meanVar();
    cout << cdf2 << endl;
    DiscretePoints cdf3( cdf2 );

//    cdf2.pointByPointAdd( cdf1 );
    cdf2.pointByPointMul( cdf1 );
    cdf3 *= cdf1;

    cdf2.meanVar();
    cout << cdf2 << endl;
    cdf3.meanVar();
    cout << cdf3 << endl;
//    cdf1 += 2;
//    cdf1.meanVar();
//    cout << cdf1 << endl;
    
    return 0;
}
