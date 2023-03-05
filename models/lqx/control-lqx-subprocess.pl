#!/usr/bin/perl -w
# script to test the creation and control of an lqns solver subprocess using the LQX language
# and synchronous data reading and feedback output

use FileHandle;
use IPC::Open2;

@phases = ( 0.0, 0.25, 0.5, 0.75, 1.0 );
@calls = ( 0.1, 3.0, 10.0 );

# run lqnx as subprocess receiving data from standard input
open2( *lqnxOutput, *lqnxInput, "lqnx 99-peva-pipe.lqnx" );

for $call (@calls) {
    for $phase (@phases) {
	print( lqnxInput "y ", $call, " p ", $phase, " STOP_READ " );
	while( ($response = <lqnxOutput>) !~ m/subprocess lqns run/ ){}
	print( "Response from lqnx subprocess: ", $response );
    }
}

# send data to terminate lqnx process
print( lqnxInput "continue_processing false STOP_READ" );
