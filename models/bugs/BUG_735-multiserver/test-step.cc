#include <iostream>
#include "multserv.h"

#define CLASSES	1

int main( int, char *[] )
{
    const unsigned int K = CLASSES;
    const unsigned int J = 2;
    const unsigned int P = 4;		// population.
    Conway_Multi_Server t_t0(J,1,K,1);
    PopVector N(K);
    const unsigned int k = 1;
    PopVector n(K);

    N[1] = P;
    t_t0.setVisits(1,1,1,1);
#if CLASSES > 1
    N[2] = 5;
    t_t0.setVisits(1,2,1,1);
#endif
    cout << "A_iterator" << std::endl;
    for ( unsigned i = 1; i <= K; ++i ) {
	cout << "i = " << i << endl;
	A_Iterator next( t_t0, i, N, k );

	while ( next( n ) ) {
	    cout << "    ";
	    n.print( cout ) << std::endl;
	}
    }
    cout << std::endl << "B_iterator" << std::endl;
    B_Iterator next( t_t0, N, k);

    while ( next( n ) ) {
	cout << "    ";
	n.print( cout ) << std::endl;
    }
}
