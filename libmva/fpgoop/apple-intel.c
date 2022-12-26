#include <stdio.h>
#include <fenv.h>
#include <xmmintrin.h>

#pragma STDC FENV_ACCESS ON

int main( int argc, char ** argv )
{
    double x = 1.0;
    double y = 0.0;
    double z = x / y;
    (void) fprintf( stderr, "First division by zero: %g.\n", z );
    _mm_setcsr(_MM_MASK_MASK & ~_MM_MASK_DIV_ZERO);
    double z2 = x / y;
    (void) fprintf( stderr, "Second division by zero: %g.\n", z );
    return 0;
}
