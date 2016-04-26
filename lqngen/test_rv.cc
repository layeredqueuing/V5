#include <cstdio>
#include "randomvar.h"

int
main( int argc, char ** argv )
{
    RV::Poisson rv(4,0);

    double sum = 0;
    for ( unsigned int i = 1; i < 100; ++i ) {
	double x = rv();
	sum += x;
	fprintf( stderr, "Variarte is %f\n", x );
    }
    fprintf( stderr, "Mean is %f\n", sum/100. );
}
