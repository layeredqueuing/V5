/*
 * Apple-intel works too, but this is an alternate method.
 */

#include <stdio.h>
#include <ieeefp.h>

int main( int argc, char ** argv )
{
    double x = 1.0;
    double y = 0.0;
    double z = x / y;
    (void) fprintf( stderr, "First division by zero: %g.\n", z );
    fpsetmask( FP_X_DZ );
    double z2 = x / y;
    (void) fprintf( stderr, "Second division by zero: %g.\n", z );
    return 0;
}
