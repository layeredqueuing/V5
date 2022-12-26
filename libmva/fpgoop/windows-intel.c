/*
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <signal.h>

void
my_handler ( int )
{
    fprintf( stderr, "my_handler caught sigfpe.\n" );
    exit( 255 );
}

int main( int argc, char ** argv )
{
    signal( SIGFPE, my_handler );
    double x = 1.0;
    double y = 0.0;
    double z = x / y;
    (void) fprintf( stderr, "First division by zero: %g.  Status is: %x\n", z, _statusfp() );
    _clearfp();
    _controlfp_s(NULL, ~(_EM_ZERODIVIDE | _EM_OVERFLOW), _MCW_EM);
    double z2 = x / y;
    (void) fprintf( stderr, "Second division by zero: %g.\n", z );
    return 0;
}
