/*  -*- c++ -*-
 * synmodel.C	-- Greg Franks Fri Aug  7 1998
 * $Id: synmodel.cc 11963 2014-04-10 14:36:42Z greg $
 *
 * Special submodel to handle synchronization.  These delays are added into
 * the waiting time arrays in the usual fashion (I hope...)
 */



#include "dim.h"
#include <cstdlib>
#include <string.h>
#include <cmath>
#include "fpgoop.h"
#include "cltn.h"
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
	       
void
SynchSubmodel::initClients( const Model& )
{
}



/*
 * Prune join paths to find sync points.
 */

void
SynchSubmodel::initServers( const Model& aSolver )
{
    Submodel::initServers( aSolver );
}



void
SynchSubmodel::initInterlock()
{
}



void
SynchSubmodel::build()
{
}



/*
 * Solving the sync models amounts to computing the join delays for
 * all of the servers that have join delays.
 */

SynchSubmodel&
SynchSubmodel::solve( long iterations, MVACount& MVAStats, const double relax )
{
    MVAStats.start( nChains(), servers.size() );

    const bool trace = flags.trace_mva && (flags.trace_submodel == 0 || flags.trace_submodel == number() );
	
    if ( trace ) {
	cout << print_submodel_header( *this, iterations ) << endl;
	printSyncModel( cout );
    }

    if ( flags.trace_delta_wait ) {
	cout << "------ updateWait for submodel " << number() << ", iteration " << iterations << " ------" << endl;
    }
		
    /* Delta Wait will re-compute the join delay */

    Sequence<Task *> nextClient( clients );
    Task * aClient;

    while ( aClient = nextClient() ) {
	if ( !pragma.init_variance_only() ) {
	    aClient->computeVariance();
	}
	aClient->updateWait( *this, relax );
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
    Sequence<Task *> nextClient( clients );
    Sequence<Entity *> nextServer( servers );

    unsigned stnNo = 1;

    Task * aClient;
    while ( aClient = nextClient() ) {
	output << "[" << stnNo << "] " << *aClient
	       << print_client_chains( *aClient, number() ) << endl;
	stnNo += 1;
    }

    Entity * aServer;
    while ( aServer = nextServer() ) {
	output << "[" << stnNo << "] " << *aServer << endl;
	aServer->printJoinDelay( output );
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

    Sequence<Entity *> nextServer(servers);
    const Entity * aServer;
    while ( aServer = nextServer() ) {
	output << "  " << *aServer;
    }
    output << endl;
    return output;
}
