/* -*- c++ -*-
 * submodel.C	-- Greg Franks Wed Dec 11 1996
 * $Id$
 *
 * MVA submodel creation and solution.  This class is the interface
 * between the input model consisting of processors, tasks, and entries,
 * and the MVA model consisting of chains and stations.
 *
 * Call build() to construct the model, then solve() to solve it.
 * The submodel number (myNumber) MUST be set by layerize.
 *
 * Prior to MVA submodel solution, stations are initilized by calling
 * Entry::elapsedTime() and Call::setVisits().  After MVA solution
 * the input model is updated with Call:saveWait() and Entry::updateWait().
 * Entry::updateWait() returns the difference in waiting times between
 * iterations of this submodel which is used for the stopping criteria.
 *
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * Replication from:
 *     author =    "Pan, Amy M.",
 *     title =     "Solving Stochastic Rendezvous Networks of Large
 *                  Client-Server Systems with Symmetric Replication",
 *     school =    sce,
 *     year =      1996,
 *     month =     sep,
 *     note =      "OCIEE-96-06",
 *
 * Overlap factor from:
 *     author =    "Mak, Victor W. and Lundstrom, Stephen F. ",
 *     title =     "Predicting Performance of Parallel Computations",
 *     journal =   ieeepds,
 *     callno =    "QA76.5.A1I35",
 *     year =      1990,
 *     volume =    1,
 *     number =    3,
 *     pages =     "257--270",
 *     month =     jul,
 *
 * ----------------------------------------------------------------------
 */


#include "dim.h"
#include <string>
#include <cstdlib>
#include <string.h>
#include <cmath>
#include "lqns.h"
#include "errmsg.h"
#include "fpgoop.h"
#include "cltn.h"
#include "vector.h"
#include "prob.h"
#include "submodel.h"
#include "entry.h"
#include "entity.h"
#include "activity.h"
#include "call.h"
#include "generate.h"
#include "mva.h"
#include "open.h"
#include "overtake.h"
#include "pragma.h"
#include "processor.h"
#include "report.h"
#include "server.h"
#include "task.h"
#include "model.h"
#include "interlock.h"

/*----------------------------------------------------------------------*/
/*                         Helper Functions                             */
/*----------------------------------------------------------------------*/

/*
 * Print all results.
 */

ostream&
operator<<( ostream& output, const Submodel& self )
{
    return self.print( output );
}

/*----------------------------------------------------------------------*/
/*                        Merged Partition Model                        */
/*----------------------------------------------------------------------*/


/*
 * Set my submodel number to n.  Reset submodel numbers of all servers.
 */

Submodel&
Submodel::number( const unsigned n )
{
    myNumber = n;

    Sequence<Entity *> nextServer( servers );
    Entity * aServer;

    /* Reset submodel numbers in all servers */

    while ( aServer = nextServer() ) {
	aServer->setSubmodel( n );
    }
    return *this;
}


/*
 * Initialize waiting times for this submodel.
 */

void
Submodel::initWait( Entity * aTask ) const
{
    aTask->updateWait( *this, 1.0 );
}



/* 
 * Now create client tables which are simply all callers to
 * the servers at each level.  We have to make sure that the
 * task only appears once in each level.  
 */

void 
Submodel::initServers( const Model& )
{
    Sequence<Entity *> nextServer( servers );
    const Entity * aServer;

    while ( aServer = nextServer() ) {
	aServer->clients( clients );
    }
}


/*
 * Handy debug function 
 */

void
Submodel::debug_stop( const unsigned long iterations, const double delta ) const
{
    if ( !cin.eof() && iterations >= flags.single_step ) {				
	cerr << "**** Submodel " << number() << " **** Delta = " << delta << ", Continue [yna]? ";
	cerr.flush();

	char c;

	cin >> c;

	switch ( c ) {
	case '\n':
	case '\004':
	    break;

	case 'n':
	case 'N':
	    throw runtime_error( "Submodel::debug_stop" );
	    break;

	case 'a':
	    LQIO::internal_error( __FILE__, __LINE__, "debug abort" );
	    break;

	default:
	    cin.ignore( 100, '\n' );
	    break;
	}
    }
}


ostream&
Submodel::submodel_header_str( ostream& output, const Submodel& aSubmodel, const unsigned long iterations )
{
    output << "========== Iteration " << iterations
	   << ",  Submodel " << aSubmodel.number() << ": "
	   << aSubmodel.clients.size() << " client" << (aSubmodel.clients.size() != 1 ? "s, " : ", ")
	   << aSubmodel.servers.size() << " server" << (aSubmodel.servers.size() != 1 ? "s."  : ".")
	   << "==========";
    return output;
}

/*----------------------------------------------------------------------*/
/*			     MVA Submodel				*/
/*----------------------------------------------------------------------*/

/*
 * Create a new MVA submodel.
 */

MVASubmodel::MVASubmodel( const unsigned n, const Model * anOwner )
    : Submodel(n,anOwner),
      hasReplication(false), 
      hasThreads(false), 
      hasSynchs(false),
      closedStnNo(0), 
      openStnNo(0),
      closedStation(0),
      openStation(0), 
      closedModel(0),
      openModel(0), 
      overlapFactor(0) 
{
}


/*
 * Free allocated store.  Do not delete stations -- stations are deleted by
 * tasks/processors.
 */

MVASubmodel::~MVASubmodel()
{
    if ( openModel ) {
	delete openModel;
    }
    if ( closedModel ) {
	delete closedModel;
    }

    if ( overlapFactor ) {
	delete [] overlapFactor;
    }
}


/*----------------------------------------------------------------------*/
/*                       Initialize the model.                          */
/*----------------------------------------------------------------------*/

/*
 * Initialize server's waiting times and populations.
 */

void
MVASubmodel::initServers( const Model& aModel )
{
    Sequence<Entity *> nextServer( servers );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	aServer->initWait();
	aModel.updateWait( aServer );
	aServer->computeVariance();
	aServer->initThroughputBound();
	aServer->initPopulation();
	aServer->initThreads();
	aServer->initialized(true);
    }
    Submodel::initServers( aModel );
}



/*
 * Initialize server's waiting times and populations.
 */

void
MVASubmodel::reinitServers( const Model& aModel )
{
    Sequence<Entity *> nextServer( servers );
    Entity * aServer;
    while ( aServer = nextServer() ) {
	aModel.updateWait( aServer );
	aServer->computeVariance();
	aServer->initThroughputBound();
	aServer->initPopulation();
//	aServer->initThreads();
//	aServer->initialized(true);
    }
//    Submodel::initServers( aModel );
}



/*
 * Go through all servers and set up path tables.
 */

void
MVASubmodel::initInterlock()
{
    if ( pragma.getInterlock() !=  THROUGHPUT_INTERLOCK ) return;

    Sequence<Entity *> next( servers );
    Entity * aServer;

    while ( aServer = next() ) {
	aServer->interlock = new Interlock( aServer );
    }
}



/*
 * Go through all servers and set up path tables.
 */

void
MVASubmodel::reinitInterlock()
{
    if ( pragma.getInterlock() != THROUGHPUT_INTERLOCK ) return;

    Sequence<Entity *> next( servers );
    Entity * aServer;

    while ( aServer = next() ) {
	aServer->interlock->initialize();
    }
}


/*
 * Build a Layer.
 */

void
MVASubmodel::build()
{
    Task * aClient;
    Entity * aServer;

    /* BUG 144 */
    if ( servers.size() == 0 ) {
	if ( pragma.getCycles() == DISALLOW_CYCLES ) {
	    LQIO::solution_error( ADV_EMPTY_SUBMODEL, number() );
	}
	return;
    }

    const unsigned n_stations  = clients.size() + servers.size();
    unsigned n_servers         = 0;
    Sequence<Task *> nextClient(clients);
    Sequence<Entity *> nextServer(servers);

    closedStnNo	= 0;
    openStnNo   = 0;

    /* ------------------- Count the stations. -------------------- */

    while ( aClient = nextClient() ) {
	closedStnNo += 1;
	if ( aClient->replicas() > 1 ) {
	    hasReplication = true; 
	}
	if ( aClient->hasThreads() ) {
	    hasThreads = true;
	}
    }

    while ( aServer = nextServer() ) {
	if ( aServer->nEntries() == 0 ) continue;	/* Null server. */
	if ( aServer->replicas() > 1 ) {
	    hasReplication = true;
	}
	if ( aServer->hasSynchs() ) {
	    hasSynchs = true;
	}
	if ( aServer->isInClosedModel() ) {
	    closedStnNo += 1;
	}
	if ( aServer->isInOpenModel() ) {
	    openStnNo += 1;
	}
    }

    closedStation.grow(closedStnNo);
    openStation.grow(openStnNo);

    closedStnNo	= 0;
    openStnNo   = 0;

    /* --------------------- Create Chains.  ---------------------- */

    n_chains = makeChains();

    /* ------------------- Create the clients. -------------------- */

    while ( aClient = nextClient() ) {
	closedStnNo += 1;
	closedStation[closedStnNo] = aClient->makeClient( n_chains, number() );
    }

    /* ----------------- Create servers for model. ---------------- */

    while ( aServer = nextServer() ) {
	Server * aStation;
	if ( aServer->nEntries() == 0 ) continue;	/* Null server. */
	aStation = aServer->makeServer( n_chains );
	if ( aServer->isInClosedModel() ) {
	    n_servers   += 1;
	    closedStnNo += 1;
	    closedStation[closedStnNo] = aStation;
	}
	if ( aServer->isInOpenModel() ) {
	    openStnNo += 1;
	    openStation[openStnNo] = aStation;
	}
    }

    /* ------- Create overlap probabilities and durations. -------- */

    if ( ( hasThreads || hasSynchs ) && pragma.getThreads() != NO_THREADS ) {
	overlapFactor = new VectorMath<double> [n_chains+1];
	for ( unsigned i = 1; i <= n_chains; ++i ) {
	    overlapFactor[i].grow( n_chains, 1.0 );
	}
    }

    /* ---------------------- Generate Model. --------------------- */

    assert ( closedStnNo <= n_stations && openStnNo <= n_stations );

    if ( openStnNo > 0 && !flags.no_execute ) {
	openModel = new Open( openStation );
    }

    if ( n_chains > 0 && n_servers > 0 && !flags.no_execute ) {
	switch ( pragma.getMVA() ) {
	case EXACT_MVA:
	    closedModel = new ExactMVA(          closedStation, myCustomers, myThinkTime, myPriority, overlapFactor );
	    break;
	case SCHWEITZER_MVA:
	    closedModel = new Schweitzer(        closedStation, myCustomers, myThinkTime, myPriority, overlapFactor );
	    break;
	case LINEARIZER_MVA:
	    closedModel = new Linearizer(        closedStation, myCustomers, myThinkTime, myPriority, overlapFactor );
	    break;
	case FAST_MVA:
	    closedModel = new Linearizer2(       closedStation, myCustomers, myThinkTime, myPriority, overlapFactor );
	    break;
	case ONESTEP_MVA:
	    closedModel = new OneStepMVA(        closedStation, myCustomers, myThinkTime, myPriority, overlapFactor );
	    break;
	case ONESTEP_LINEARIZER:
	    closedModel = new OneStepLinearizer( closedStation, myCustomers, myThinkTime, myPriority, overlapFactor );
	    break;
	}
    }

    while ( aClient = nextClient() ) {
	aClient->callsPerform(&Call::setChain, number()); 
	if ( aClient->replicas() > 1 ) {
	    unsigned n_delta = 0;
	    aClient->updateWaitReplication( *this, n_delta );
	}
    }
}



/*
 * Rebuild stations and customers as needed.
 */

void
MVASubmodel::rebuild()
{
    unsigned k = 0;			/* Chain number.		*/
    Entity * aServer;
    Sequence<Entity *> nextServer( servers );

    /* ----- Set think times, customers and chains for this pass. ----- */

    Sequence<Task *> nextClient(clients);
    Task * aClient;
    while ( aClient = nextClient() ) {
	Cltn<Entity *> clientsServers;
	Entity * aServer;

	aClient->servers( clientsServers, servers );	/* Get all servers for this client	*/
	Sequence<Entity *> nextServer( clientsServers );
	const unsigned threads = aClient->nThreads();

	if ( aClient->replicas() <= 1 ) {

	    /* ---------------- Simple case --------------- */

	    for ( unsigned i = 1; i <= threads; ++i ) {
		k += 1;				// Add one chain.

		if ( finite( aClient->population() ) ) {
		    myCustomers[k] = static_cast<unsigned>(aClient->population());
		} else {
		    myCustomers[k] = 0;
		}
		myPriority[k]  = aClient->priority();
	    }

	} else {
	    //REPL changes
	    /* --------------- Complex case --------------- */

	    //!!! If chains are extended to entries to handle
	    //!!! fanins, modify delta_chains and Entity::fanIn()
	    //!!! for the entry-to-entry case.

	    /*
	     * Do all the chains for to a server at once.  This makes
	     * Task::threadIndex() simpler.
	     */

	    while ( aServer = nextServer() ) {
		for ( unsigned i = 1; i <= threads; ++i ) {
		    k += 1;

		    if ( finite( aClient->population() ) ) {
			myCustomers[k] = static_cast<unsigned>(aClient->population()) * aServer->fanIn(aClient); 
		    } else {
			myCustomers[k] = 0;
		    }
		}
	    }
	}
    }

    /* ------------------ Recreate servers for model. -----------------	*/

    while ( aServer = nextServer() ) {
	if ( aServer->nEntries() == 0 ) continue;	/* Null server. */
	Server * oldStation = aServer->serverStation();		/* Get old station		*/
	const unsigned closedIndex = oldStation->closedIndex;	/* Copy over indicies.		*/
	const unsigned openIndex   = oldStation->openIndex;

 	Server * newStation = aServer->makeServer( nChains() );	/* Returns NULL on no change	*/
	if ( !newStation )  continue;			/* Out with the old...		*/

	newStation->setMarginalProbabilitiesSize( myCustomers );

	if ( closedIndex ) {
	    newStation->closedIndex = closedIndex;
	    closedStation[closedIndex] = newStation;	/* ... and in with the new...	*/
	}
	if ( openIndex ) {
	    newStation->openIndex = openIndex;
	    openStation[openIndex] = newStation;	/* ... and in with the new...	*/
	}
    }
}


/*
 * Determine the number of chains, their populations, and their think times.
 * Customers and thinkTime are dimensioned by CHAIN (k).  The array `chains'
 * is indexed by client (i) -- it is the link from customer to chain.
 */

unsigned
MVASubmodel::makeChains() 
{
    unsigned k = 0;			/* Chain number.		*/

    /* --- Set think times, customers and chains for this pass. --- */

    Sequence<Task *> nextClient(clients);
    Task * aClient;
    while ( aClient = nextClient() ) {
	Cltn<Entity *> clientsServers;
	Entity * aServer;

	aClient->servers( clientsServers, servers );	/* Get all servers for this client	*/
	Sequence<Entity *> nextServer( clientsServers );
	const unsigned threads = aClient->nThreads();

	if ( aClient->replicas() <= 1 ) {

	    /* ---------------- Simple case --------------- */

	    myCustomers.grow( threads ); /* N.B. -- Vector class.  Must*/
	    myThinkTime.grow( threads ); /* grow() explicitly.	*/
	    myPriority.grow( threads );

	    for ( unsigned i = 1; i <= threads; ++i ) {
		k += 1;				// Add one chain.

		aClient->addClientChain( number(), k );	// Set my chain number.
		if ( finite( aClient->population() ) ) {
		    myCustomers[k] = static_cast<unsigned>(aClient->population());
		} else {
		    myCustomers[k] = 0;
		}
		myPriority[k]  = aClient->priority();

		/* add chain to all servers of this client */

		while ( aServer = nextServer() ) {
		    aServer->addServerChain( k );
		}
	    }

	} else {
	    const unsigned sz = threads * clientsServers.size();

	    //REPL changes
	    /* --------------- Complex case --------------- */

	    //!!! If chains are extended to entries to handle
	    //!!! fanins, modify delta_chains and Entity::fanIn()
	    //!!! for the entry-to-entry case.

	    myCustomers.grow( sz );   //Expand vectors to accomodate
	    myThinkTime.grow( sz );   //new chains.
	    myPriority.grow( sz );

	    /*
	     * Do all the chains for to a server at once.  This makes
	     * Task::threadIndex() simpler.
	     */

	    while ( aServer = nextServer() ) {
		for ( unsigned i = 1; i <= threads; ++i ) {
		    k += 1;

		    aClient->addClientChain( number(), k );
		    aServer->addServerChain( k );
		    myPriority[k]  = aClient->priority();
		    if ( finite( aClient->population() ) ) {
			myCustomers[k] = static_cast<unsigned>(aClient->population()) * aServer->fanIn(aClient); 
		    } else {
			myCustomers[k] = 0;
		    }
		}
	    }

	}
    }
    return k;
}

//REPL changes


/*
 * Set service time and visits to clients.
 */

void
MVASubmodel::initClient( Task * aClient )
{
    Sequence<Entry *> nextEntry( aClient->entries() );
    const ChainVector& aChain = aClient->clientChains( number() );
    Server * aStation = aClient->clientStation( number() );

    for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	const unsigned k = aChain[ix];
	const Entry * anEntry;

	while( anEntry = nextEntry() ) {
	    const unsigned e = anEntry->index;

	    for ( unsigned p = 1; p <= anEntry->maxPhase(); ++p ) {
		const double s = anEntry->waitExcept( number(), k, p );
		aStation->setService( e, k, p, s );
	    }
	    aStation->setVisits( e, k, 1, anEntry->prVisit() );	// As client, called-by phase does not matter.
	}
 
	/* Set idle times for stations. */

	myThinkTime[k] = aClient->thinkTime( number(), k );
    }
}


void
MVASubmodel::reinitClients()
{
    unsigned k = 0;			/* Chain number.		*/

    /* --- Set think times, customers and chains for this pass. --- */

    Sequence<Task *> nextClient(clients);
    Task * aClient;
    while ( aClient = nextClient() ) {
	const unsigned threads = aClient->nThreads();

	if ( aClient->replicas() <= 1 ) {

	    /* ---------------- Simple case --------------- */

	    for ( unsigned i = 1; i <= threads; ++i ) {
		k += 1;				// Add one chain.

		if ( finite( aClient->population() ) ) {
		    myCustomers[k] = static_cast<unsigned>(aClient->population());
		} else {
		    myCustomers[k] = 0;
		}
		myPriority[k]  = aClient->priority();
	    }

	} else {
	    Cltn<Entity *> clientsServers;
	    Entity * aServer;

	    aClient->servers( clientsServers, servers );	/* Get all servers for this client	*/
	    Sequence<Entity *> nextServer( clientsServers );
	    
	    //REPL changes
	    /* --------------- Complex case --------------- */

	    while ( aServer = nextServer() ) {
		for ( unsigned i = 1; i <= threads; ++i ) {
		    k += 1;

		    myPriority[k]  = aClient->priority();
		    if ( finite( aClient->population() ) ) {
			myCustomers[k] = static_cast<unsigned>(aClient->population()) * aServer->fanIn(aClient); 
		    } else {
			myCustomers[k] = 0;
		    }
		}
	    }
	}
    }
}

/*
 * If I am replicated and I have multiple chains, I have to add on the
 * waiting time made to all other tasks in my partition but NOT in my
 * chain too.  This step must be performed after BOTH the clients and
 * servers have been created so that all of the chain information is
 * avaiable at all stations.  Chain information is initialized in
 * makeChains.  Submodel information is initialized in initServer.
 */

//++ REPL changes

void
MVASubmodel::modifyClientServiceTime( Task * aClient )
{
    Server * aStation = aClient->clientStation( number() );

    Sequence<Entry *> nextEntry( aClient->entries() );
    Entry * anEntry;

    while( anEntry = nextEntry() ) {
			 
	const unsigned e = anEntry->index;

	const ChainVector& aChain = aClient->clientChains( number() );
	for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	    //cout << "entry=" << anEntry->name() <<", ix =" <<ix << endl;
	    const unsigned k = aChain[ix];
	    for ( unsigned p = 1; p <= anEntry->maxPhase(); ++p ) {
		aStation->setService( e, k, p, anEntry->waitExceptChain( number(), k, p ) );
	    }
	}
    }
}



/*
 * Query the closed model to determine the factor needed for
 * Newton Raphson stuff.
 * (5.18) [Pan, pg 73].
 */

double
MVASubmodel::nrFactor( const Server * aStation, const unsigned e, const unsigned k ) const
{
    const double s = aStation->S( e, k, 1 );
    if ( finite( s ) && closedModel ) {  //tomari 
	//Solution for replicated models with open arrivals.
	// See bug # 87.

	return closedModel->nrFactor( *aStation, e, k ) * s;

    } else {
	return s;
    }
}

//-- REP N-R



/*
 * Initialize the service and visit parameters for a server.  Also set myChains to
 * all chains that visit this server.
 */

void
MVASubmodel::initServer( Entity * aServer ) 
{
    Server * aStation = aServer->serverStation();

    if ( !aStation ) return;

    if ( !pragma.init_variance_only() ) {
	aServer->computeVariance();
    }

    const ChainVector& aChain = aServer->serverChains();
    Sequence<Entry *> nextEntry( aServer->entries() );
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {

	const unsigned e = anEntry->index;
	const double openArrivalRate = anEntry->openArrivalRate();

	if ( openArrivalRate > 0.0 ) {
	    aStation->setVisits( e, 0, 1, openArrivalRate );	// Chain 0 reserved for open class.
	}

	/* -- Set service time for entries with visits only. -- */

	for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	    setServiceTime( aServer, anEntry, aChain[ix] );
	}
			  
	/*
	 * Open arrivals and other open models use chain zero which are special
	 * and won't work in the above loop anyway)
	 */

	if ( aServer->isInOpenModel() ) {
	    setServiceTime( aServer, anEntry, 0 );
	} 

    }
    /* Overtaking -- compute for MARKOV overtaking only. */
         
    if ( aServer->markovOvertaking() ) {
	aServer->setOvertaking( number(), clients );
    }
    
    /* Set interlock */
 
    if ( aServer->isInClosedModel() && pragma.getInterlock() == THROUGHPUT_INTERLOCK ) {
	setInterlock( aServer );
    }

    if ( aServer->trace() ) {
	cout << "Submodel " << number() << ", server" << endl << *aServer << endl
	     << print_server_chains( *aServer )
	     << *aServer->serverStation() 
	     << "  Throughput = " << aServer->throughput() << endl << endl;
    }
}



/*
 * Set the service time for my station.
 */

void
MVASubmodel::setServiceTime( Entity * aServer, const Entry * anEntry, unsigned k ) const
{
    const unsigned e = anEntry->index;
    Server * aStation = aServer->serverStation();

    if ( aStation->V( e, k ) == 0 ) return;

    for ( unsigned p = 1; p <= anEntry->maxPhase(); ++p ) {
	aStation->setService( e, k, p, anEntry->elapsedTime(p) );

	if ( aServer->hasVariance() ) {
	    aStation->setVariance( e, k, p, anEntry->variance(p) );
	}
    }
}



/*
 * Set interlocking.
 */

void
MVASubmodel::setInterlock( Entity * aServer ) const
{
    Task * aClient;
    Server * aStation = aServer->serverStation();
    Sequence<Entry *> nextEntry( aServer->entries() );
    Sequence<Task *> nextClient( clients );

    while ( aClient = nextClient() ) {
	if ( aClient->throughput() == 0.0 ) continue;

	const Probability PrIL = aServer->prInterlock( *aClient );
	if ( PrIL == 0.0 ) continue;

	const ChainVector& aChain = aClient->clientChains( number() );
	const Entry * anEntry;

	for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	    const unsigned k = aChain[ix];
	    if ( aServer->hasServerChain(k) ) {
		while ( anEntry = nextEntry() ) {	/* My entries. */
		    aStation->setInterlock( anEntry->index, k, PrIL );
		}
	    }
	}
    }
}

/*----------------------------------------------------------------------*/
/*                          Solve the model.                            */
/*----------------------------------------------------------------------*/

/*
 * Solve a layer.  Return the difference between the current and previous
 * solutions.  NOTE: Layers of tasks are passed as sequences.  Be sure to
 * always step through the entire sequence, or rewind the list.
 */

MVASubmodel&
MVASubmodel::solve( long iterations, MVACount& MVAStats, const double relax )
{
    if ( servers.size() == 0 ) return *this;
    if ( flags.verbose ) cerr << '.';

    const bool trace = flags.trace_mva && (flags.trace_submodel == 0 || flags.trace_submodel == number() );
    Sequence<Task *> nextClient( clients );
    Sequence<Entity *> nextServer( servers );

    MVAStats.start( nChains(), servers.size() );
    double deltaRep	= 0.0;
    Entity * aServer;
    Task * aClient;
    unsigned iter       = 0; //REP N-R

    if ( trace ) {
	cout << print_submodel_header( *this, iterations ) << endl;
    }

    while ( aServer = nextServer() ) {
	aServer->serverStation()->clear();	/* Clear visit ratios and what have you */
    }

    /* ------------------- Create the clients. -------------------- */
    while ( aClient = nextClient() ) {
			   
	initClient( aClient );

	if ( aClient->hasThreads() && pragma.getThreads() != NO_THREADS ) {
	    aClient->forkOverlapFactor( *this, overlapFactor );
				 
	}	
			 
	/* Set visit ratios to all servers for this client */
	/* This will also set arrival rates for open class from sendNoReply */

	aClient->callsPerform( &Call::setVisits, number() ).openCallsPerform( &Call::setLambda, number() );

    }


    /* ----------------- Create servers for model. ---------------- */
    while ( aServer = nextServer() ) {
	initServer( aServer );
	if ( aServer->hasSynchs() && pragma.getThreads() != NO_THREADS ) {
	    aServer->joinOverlapFactor( *this, overlapFactor );
	}
    }

 
    /* ------------------- Replication Iteration ------------------- */
    do { 
	//REP N-R
	iter += 1;

	/* ---- Adjust Client service times for replication ---- */

	if ( hasReplication ) {

	    if ( flags.trace_mva  ) {
		cout << "\nCurrent master iteration ="<< iterations<<endl;
		cout <<"Replication Iteration Number (submodel=" <<number() <<") = " <<iter ;
		cout << "\ndeltaRep=" << deltaRep;
		cout << ", convergence_value=" << Model::convergence_value<< endl;

	    }

	    while ( aClient = nextClient() ) {
		if ( aClient->replicas() > 1 ) {
		    modifyClientServiceTime( aClient );

		    if ( aClient->trace() ) {
			cout << "Submodel " << number() << ":" << iter <<", client" << endl << *aClient << endl
			     << print_client_chains( *aClient, number() )
			     << *aClient->clientStation( number() ) 
			     << "  Throughput = " << aClient->throughput() << endl << endl;
		    }
		}
	    }
	}

	//REP N-R
 
	if ( flags.generate ) {			// Print out MVA model as C++ source file.
	    Generate::print( *this );
	}

	/* ----------------- Solve the model. ----------------- */

	if ( closedModel ) {

	    if ( openModel ) {
		if ( trace ) {
		    printOpenModel( cout );
		}

		/*
		 * If model has any open classes, convert for closed model.
		 */

		try {
		    openModel->convert( myCustomers );
		}
		catch ( range_error& error ) {
		    MVAStats.faults += 1;
		    if ( pragma.getStopOnMessageLoss() ) {
			while ( aServer = nextServer() ) {
			    const Server * aStation = aServer->serverStation();
			    if ( !finite( aStation->R(0) ) ) {
				LQIO::solution_error( ERR_ARRIVAL_RATE, aStation->V(0), aServer->name(), aStation->S(0) / aStation->mu() );
			    }
			}
			throw exception_handled( "MVA::submodel -- open model overflow" );
		    }
		}
	    }

	    if ( trace ) {
		printClosedModel( cout );
	    }
	    try {
		closedModel->solve();
	    }
	    catch ( range_error& error ) {
		throw;
	    }

	    /* Statistics by level -- we can use this to find performance bottlenecks */

	    MVAStats.accumulate( closedModel->iterations(), closedModel->waits(), closedModel->faults() );
	}

	if ( openModel ) {
	    if ( trace && !closedModel ) {
		printOpenModel( cout );
	    }

	    try {
		if ( closedModel ) {
		    openModel->solve( *closedModel, myCustomers );	/* Calculate L[0] queue lengths. */
		} else {
		    openModel->solve();
		}
	    } 
	    catch ( range_error& error ) {
		if ( pragma.getStopOnMessageLoss() ) {
		    while ( aServer = nextServer() ) {
			const Server * aStation = aServer->serverStation();
			if ( !finite( aStation->R(0) ) ) {
			    LQIO::solution_error( ERR_ARRIVAL_RATE, aStation->V(0), aServer->name(), aStation->S(0) / aStation->mu() );
			}
		    }
		    throw exception_handled( "MVA::submodel -- open model overflow" );
		}
	    }

	    if ( trace ) {
		cout << *openModel << endl << endl;
	    }
	}

	if ( closedModel && trace ) {
	    FMT_FLAGS oldFlags = cout.setf( ios::right, ios::adjustfield );
	    cout << *closedModel << endl << endl;
	    cout.flags( oldFlags );
	}

	/* ---------- Set wait and think times for next pass. --------- */

	if (flags.trace_throughput || flags.trace_idle_time) {
			  
	    cout <<"\nMVASubmodel::solve( ) .... completed solving the MVA model.......\n" << endl;
	}


	while ( aClient = nextClient() ) {

	    /* Get and save the waiting time results for all servers to this client */

	    aClient->callsPerform( &Call::saveWait, number() )
		.openCallsPerform( &Call::saveOpen, number() );
	    /* Other results (only useful for references tasks. */

	    if ( closedModel ) {
		saveClientResults( aClient );

	    }
	}


	while ( aServer = nextServer() ) {
	    saveServerResults( aServer );
	    aServer->setIdleTime( relax, this );
	}

	/* --- Compute and save new values for entry service times. --- */

	if ( flags.trace_delta_wait ) {
	    cout << "------ updateWait for submodel " << number() << ", iteration " << iterations << " ------" << endl;
	}

	/* Update waits for replication */

	deltaRep = 0.0;
	if ( hasReplication ) {
	    unsigned n_deltaRep = 0;
	    while ( aClient = nextClient() ) {
		deltaRep += aClient->updateWaitReplication( *this, n_deltaRep );
	    }
	    if ( n_deltaRep ) {
		deltaRep = sqrt( deltaRep / n_deltaRep );	/* Take RMS value over all phases */
	    }
	    if ( iter >= Model::iteration_limit ) {
		LQIO::solution_error( ADV_REPLICATION_ITERATION_LIMIT, number(), iter, deltaRep, Model::convergence_value );
		deltaRep = 0;		/* Break out of loop */
	    }
	}

	/* Update waits for everyone else. */

	while ( aClient = nextClient() ) {
	    aClient->updateWait( *this, relax );
	}

	if ( !check_fp_ok() ) {
	    throw floating_point_error( __FILE__, __LINE__ );
	}

	if ( flags.single_step ) {
	    debug_stop( iterations, 0 );
	}


    } while ( deltaRep > Model::convergence_value );

    /* ----------------End of Replication Iteration --------------- */
    // REP N-R

    return *this;
}

/*----------------------------------------------------------------------*/
/*                          Save Results                                */
/*----------------------------------------------------------------------*/

/*
 * Save throughput and utilization for all reference tasks.
 * These values are computed for servers only when they act as servers.
 */

void
MVASubmodel::saveClientResults( Task * aClient )
{
    if ( aClient->isCalled() || !aClient->isReferenceTask() ) return;

    const Server * aStation = aClient->clientStation( number() );
    Sequence<Entry *> nextEntry( aClient->entries() );
    const ChainVector& myChain( aClient->clientChains(number()));
    Entry * anEntry;

    if ( aClient->hasActivities() ) {
	Sequence<Activity *>nextActivity(aClient->activities());
	Activity * anActivity;

	while ( anActivity = nextActivity() ) {
	    anActivity->clearThroughput();
	}
    }

    while ( anEntry = nextEntry() ) {
	/*Positive*/ double lambda = 0; // to get rid of the exception
	//when I set the service time of an activity "Reply" to zero
	//the throughput will be infinity when using the Gamma distribution.

	if ( aClient->replicas() > 1 ) {

	    /*
	     * Get throughput PER CUSTOMER because replication
	     * monkeys with the population levels.  Fix for
	     * multiservers.
	     */

	    lambda = closedModel->normalizedThroughput( *aStation, anEntry->index, myChain[1] ) * aClient->population() ;
	} else {
	    lambda = closedModel->throughput( *aStation, anEntry->index, myChain[1] );
	}
	anEntry->throughput( lambda );
    }
}


/*
 * Save server results.  Servers only occur in one submodel.
 */
void
MVASubmodel::saveServerResults( Entity * aServer )
{
    const Server * aStation = aServer->serverStation();

    if ( aServer->hasActivities() ) {
	Sequence<Activity *>nextActivity(dynamic_cast<const Task *>(aServer)->activities());
	Activity * anActivity;

	while ( anActivity = nextActivity() ) {
	    anActivity->clearThroughput();
	}
    }


    Sequence<Entry *> nextEntry(aServer->entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	const unsigned e = anEntry->index;
	double lambda = 0.0;

	if ( aServer->isInOpenModel() && openModel ) {
	    lambda = openModel->entryThroughput( *aStation, e );		/* BUG_168 */
	}

	if ( aServer->isInClosedModel() && closedModel ) {
	    const double tput = closedModel->entryThroughput( *aStation, e );
	    if ( finite( tput ) ) {
		lambda += tput;
	    } else if ( tput < 0.0 ) {
		throw domain_error( "MVASubmodel::saveServerResults" );
	    } else {
		lambda = tput;
		break;
	    }
	}

	anEntry->throughput( lambda );
	anEntry->saveOpenWait( aStation->R( e, 0 ) );
    }
}

/*----------------------------------------------------------------------*/
/* Printing functions.							*/
/*----------------------------------------------------------------------*/

/*
 * Print out a submodel.
 */

ostream&
MVASubmodel::print( ostream& output ) const
{
    output << "----------------------- Submodel  " << number() << " -----------------------" << endl
	   << "Customers: " <<  myCustomers << endl
	   << "Clients: " << endl;

    Sequence<Task *> nextClient(clients);
    const Task * aClient;

    while ( aClient = nextClient() ) {
	output << setw(2) << "  " << *aClient << endl;
    }
    output << endl << "Servers: " << endl;

    Sequence<Entity *> nextServer(servers);
    const Entity * aServer;
    while ( aServer = nextServer() ) {
	output << setw(2) << "  " << *aServer << endl;
    }
    output << endl;
    return output;
}



/*
 * Print stations of closed model.
 */

ostream&
MVASubmodel::printClosedModel( ostream& output ) const
{
    Sequence<Task *> nextClient( clients );
    Sequence<Entity *> nextServer( servers );

    unsigned stnNo = 1;

    const Task *aClient;
    while ( aClient = nextClient() ) {
	output << "[closed=" << stnNo << "] " << *aClient << endl
	       << print_client_chains( *aClient, number() )
	       << *aClient->clientStation( number() ) << endl;
	stnNo += 1;
    }

    const Entity * aServer;
    while ( aServer = nextServer() ) {
	if ( aServer->isInClosedModel() ) {
	    output << "[closed=" << stnNo << "] " << *aServer << endl
		   << print_server_chains( *aServer )
		   << *aServer->serverStation() << endl;
	    aServer->serverStation()->printOutput( output, stnNo );
	    stnNo += 1;
	}
    }
    return output;
}



/*
 * Print stations of open model.
 */

ostream&
MVASubmodel::printOpenModel( ostream& output ) const
{
    Sequence<Entity *> nextServer( servers );

    unsigned stnNo = 1;
    const Entity * aServer;
    while ( aServer = nextServer() ) {
	if ( aServer->isInOpenModel() ) {
	    output << "[open=" << stnNo << "] " << *aServer << endl
		   << *aServer->serverStation() << endl;
	    stnNo += 1;
	}
    }
    return output;
}

/* ---------------------------------------------------------------------- */

SubModelManip
print_submodel_header( const Submodel & aSubModel, const unsigned long iterations  )
{
    return SubModelManip( &Submodel::submodel_header_str, aSubModel, iterations );
}
