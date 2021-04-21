/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/regression/disttest.cc $
 *
 * Distribution function tests.
 * ------------------------------------------------------------------------
 *
 * $Id: disttest.cc 14616 2021-04-21 13:20:38Z greg $
 */

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "randomvar.h"
#include <mva/vector.h>

char * myName;

static void usage ()
{
    std::cerr << myName << " " << std::endl;
    exit( 1 );
}

int main ( int argc, char * argv[] )
{
    int c;

    myName = argv[0];
	
    while (( c = getopt( argc, argv, "" )) != EOF) {
	switch ( c ) {
	default:
	    std::cerr << "Unkown option." << std::endl;
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
    std::cout << cdf1 << std::endl;
    DiscretePoints cdf2;
    cdf2.setCDF( t2, A2 );
    cdf2.meanVar();
    std::cout << cdf2 << std::endl;
    DiscretePoints cdf3( cdf2 );

//    cdf2.pointByPointAdd( cdf1 );
    cdf2.pointByPointMul( cdf1 );
    cdf3 *= cdf1;

    cdf2.meanVar();
    std::cout << cdf2 << std::endl;
    cdf3.meanVar();
    std::cout << cdf3 << std::endl;
//    cdf1 += 2;
//    cdf1.meanVar();
//    std::cout << cdf1 << std::endl;
    
    return 0;
}

template class Vector<double>;
template class Vector<unsigned int>;
template class Vector<unsigned long>;
template class VectorMath<unsigned int>;
template class VectorMath<double>;
template class Vector<Vector<unsigned> >;
template class Vector<VectorMath<double> >;
template class Vector<VectorMath<unsigned> >;

