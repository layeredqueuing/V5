#include <stdio.h>

/*
 * feenableexcept exists, but it's a pain to use.  The apple-intel version works too.
 */

#define __USE_GNU
#include <fenv.h>

#pragma STDC FENV_ACCESS ON

int main( int argc, char ** argv )
{
    double x = 1.0;
    double y = 0.0;
    double z = x / y;
    (void) fprintf( stderr, "First division by zero: %g.\n", z );
    feenableexcept( FE_DIVBYZERO );
    double z2 = x / y;
    (void) fprintf( stderr, "Second division by zero: %g.\n", z );
    return 0;
}
