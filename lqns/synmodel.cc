/*  -*- c++ -*-
 * synmodel.C	-- Greg Franks Fri Aug  7 1998
 * $Id: synmodel.cc 14052 2020-11-08 03:04:43Z greg $
 *
 * Special submodel to handle synchronization.  These delays are added into
 * the waiting time arrays in the usual fashion (I hope...)
 */



#include "dim.h"
#include <cstdlib>
#include <string.h>
#include <cmath>
#include "entity.h"
#include "fpgoop.h"
#include "lqns.h"
#include "pragma.h"
#include "report.h"
#include "synmodel.h"
#include "task.h"


SynchSubmodel::~SynchSubmodel()
{
}
	       
/*
 * Prune join paths to find sync points.
 */

SynchSubmodel&
SynchSubmodel::initServers( const Model& aSolver )
{
    Submodel::initServers( aSolver );
    return *this;
}



/*
 * Solving the sync models amounts to computing the join delays for
 * all of the servers that have join delays.
 */

SynchSubmodel&
SynchSubmodel::solve( long iterations, MVACount& MVAStats, const double relax )
{
    MVAStats.start( nChains(), _servers.size() );

    const bool trace = flags.trace_mva && (flags.trace_submodel == 0 || flags.trace_submodel == number() );
	
    if ( trace ) {
	cout << print_submodel_header( *this, iterations ) << endl;
    }

    if ( flags.trace_delta_wait ) {
	cout << "------ updateWait for submodel " << number() << ", iteration " << iterations << " ------" << endl;
    }
		
    /* Delta Wait will re-compute the join delay, normal clients don't care about variance, but join delay needs it. */

    if ( !Pragma::init_variance_only() ) {
	for_each ( _clients.begin(), _clients.end(), Exec<Task>( &Task::computeVariance ) );
    }
    for_each ( _clients.begin(), _clients.end(), Exec2<Task,const Submodel&,double>( &Task::updateWait, *this, relax ) );
	
    if ( trace ) {
	printSyncModel( cout );
    }

    MVAStats.accumulate( 0, 0, 0 );

    if ( flags.single_step ) {
	debug_stop( iterations, 0 );
    }	
    if ( !check_fp_ok() ) {
	throw floating_point_error( __FILE__, __LINE__ );
    }
 
    return *this;
}



/*
 * tracing -- print out sync model.
 */

ostream&
SynchSubmodel::printSyncModel( ostream& output ) const
{
    unsigned stnNo = 1;

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client, ++stnNo ) {
	output << stnNo << ": " << **client
	       << " " << Task::print_client_chains( **client, number() ) << endl;
	(*client)->printJoinDelay( output );
	output << endl;
    }

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server, ++stnNo ) {
	output << stnNo << ": " << **server << endl;
	(*server)->printJoinDelay( output );
	output << endl;
    }
    return output;
}



/*
 * Debugging -- print out sync model.
 */

ostream&
SynchSubmodel::print( ostream& output ) const
{
    output << "----------------------- Submodel  " << number() << " -----------------------" << endl
	   << "Servers: " << endl;

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	output << "  " << *(*server);
    }
    output << endl;
    return output;
}
