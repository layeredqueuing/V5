/*  -*- c++ -*-
 * synmodel.C	-- Greg Franks Fri Aug  7 1998
 * $Id: synmodel.cc 13725 2020-08-04 03:58:02Z greg $
 *
 * Special submodel to handle synchronization.  These delays are added into
 * the waiting time arrays in the usual fashion (I hope...)
 */



#include "dim.h"
#include <cstdlib>
#include <string.h>
#include <cmath>
#include "fpgoop.h"
#include "submodel.h"
#include "synmodel.h"
#include "lqns.h"
#include "entity.h"
#include "task.h"
#include "report.h"
#include "pragma.h"

SynchSubmodel::SynchSubmodel( const unsigned n, const Model * anOwner )
	: Submodel( n, anOwner )
{
}


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
	printSyncModel( cout );
    }

    if ( flags.trace_delta_wait ) {
	cout << "------ updateWait for submodel " << number() << ", iteration " << iterations << " ------" << endl;
    }
		
    /* Delta Wait will re-compute the join delay */

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	if ( !Pragma::init_variance_only() ) {
	    (*client)->computeVariance();
	}
	(*client)->updateWait( *this, relax );
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

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	output << "[" << stnNo << "] " << **client
	       << Task::print_client_chains( **client, number() ) << endl;
	stnNo += 1;
    }

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	output << "[" << stnNo << "] " << **server << endl;
	(*server)->printJoinDelay( output );
	output << endl;
	stnNo += 1;
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
