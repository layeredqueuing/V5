/* -*- c++ -*-
 * submodel.C	-- Greg Franks Wed Dec 11 1996
 * $Id: submodel.cc 13685 2020-07-14 02:53:54Z greg $
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
#include <algorithm>
#include <string>
#include <cstdlib>
#include <string.h>
#include <cmath>
#include "lqns.h"
#include "errmsg.h"
#include "fpgoop.h"
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
#include "option.h"

#define	QL_INTERLOCK

/*----------------------------------------------------------------------*/
/*                        Merged Partition Model                        */
/*----------------------------------------------------------------------*/


/*
 * Set my submodel number to n.  Reset submodel numbers of all servers.
 */

Submodel&
Submodel::number( const unsigned n )
{
    _submodel_number = n;
    for_each( _servers.begin(), _servers.end(), Exec1<Entity,const unsigned>( &Entity::setSubmodel, n ) );
    return *this;
}


/*
 * Now create client tables which are simply all callers to
 * the servers at each level.  Note that _clients is an intersection
 * of all clients to all servers.
 */

Submodel&
Submodel::initServers( const Model& )
{
    for_each( _servers.begin(), _servers.end(), ConstExec1<Entity,std::set<Task *>&>( &Entity::clients, _clients ) );
    return *this;
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
    output << "========== Iteration " << iterations << ", "
	   << aSubmodel.submodelType() << " " << aSubmodel.number() << ": "
	   << aSubmodel._clients.size() << " client" << (aSubmodel._clients.size() != 1 ? "s, " : ", ")
	   << aSubmodel._servers.size() << " server" << (aSubmodel._servers.size() != 1 ? "s."  : ".")
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
      _hasReplication(false),
      _hasThreads(false),
      _hasSynchs(false),
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
    closedStation.clear();
    openStation.clear();
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

MVASubmodel&
MVASubmodel::initServers( const Model& aModel )
{
    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	(*server)->initWait();
	aModel.updateWait( (*server) );
	(*server)->computeVariance()
	    .initThroughputBound()
	    .initPopulation()
	    .initThreads()
	    .initialized(true);
    }
    Submodel::initServers( aModel );
    return *this;
}



/*
 * Initialize server's waiting times and populations.
 */

MVASubmodel&
MVASubmodel::reinitServers( const Model& aModel )
{
    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	aModel.updateWait( (*server) );
	(*server)->computeVariance()
	    .initThroughputBound()
	    .initPopulation();
//	aServer->initThreads();
//	aServer->initialized(true);
    }
//    Submodel::initServers( aModel );
    return *this;
}



/*
 * Go through all servers and set up path tables.
 */

MVASubmodel&
MVASubmodel::initInterlock()
{
    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	(*server)->interlock = new Interlock( *server );
    }
    return *this;
}



/*
 * Go through all servers and set up path tables.
 */

MVASubmodel&
MVASubmodel::reinitInterlock()
{
    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	(*server)->interlock->initialize();
    }
    return *this;
}


/*
 * Build a Layer.
 */

MVASubmodel&
MVASubmodel::build()
{
    /* BUG 144 */
    if ( _servers.size() == 0 ) {
	if ( pragma.getCycles() == DISALLOW_CYCLES ) {
	    LQIO::solution_error( ADV_EMPTY_SUBMODEL, number() );
	}
	return *this;
    }

    const unsigned n_stations  = _clients.size() + _servers.size();
    unsigned n_servers         = 0;

    closedStnNo	= 0;
    openStnNo   = 0;

    /* ------------------- Count the stations. -------------------- */

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	closedStnNo += 1;
	if ( (*client)->replicas() > 1 ) {
	    _hasReplication = true;
	}
	if ( (*client)->hasThreads() ) {
	    _hasThreads = true;
	}
    }

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	if ( (*server)->nEntries() == 0 ) continue;	/* Null server. */
	if ( (*server)->replicas() > 1 ) {
	    _hasReplication = true;
	}
	if ( (*server)->hasSynchs() ) {
	    _hasSynchs = true;
	}
	if ( (*server)->isInClosedModel() ) {
	    closedStnNo += 1;
	}
	if ( (*server)->isInOpenModel() ) {
	    openStnNo += 1;
	}
    }

    closedStation.resize(closedStnNo);
    openStation.resize(openStnNo);

    closedStnNo	= 0;
    openStnNo   = 0;

    /* --------------------- Create Chains.  ---------------------- */

    setNChains( makeChains() );
    if ( nChains() > MAX_CLASSES ) {
	LQIO::solution_error( ADV_MANY_CLASSES, nChains(), number() );
    }

    /* ------------------- Create the clients. -------------------- */

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	closedStnNo += 1;
	closedStation[closedStnNo] = (*client)->makeClient( nChains(), number() );
    }

    /* ----------------- Create servers for model. ---------------- */

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	Server * aStation;
	if ( (*server)->nEntries() == 0 ) continue;	/* Null server. */
	aStation = (*server)->makeServer( nChains() );
	if ( (*server)->isInClosedModel() ) {
	    n_servers   += 1;
	    closedStnNo += 1;
	    closedStation[closedStnNo] = aStation;
	}
	if ( (*server)->isInOpenModel() ) {
	    openStnNo += 1;
	    openStation[openStnNo] = aStation;
	}
    }

    /* ------- Create overlap probabilities and durations. -------- */

    if ( ( hasThreads() || hasSynchs() ) && pragma.getThreads() != NO_THREADS ) {
	overlapFactor = new VectorMath<double> [nChains()+1];
	for ( unsigned i = 1; i <= nChains(); ++i ) {
	    overlapFactor[i].resize( nChains(), 1.0 );
	}
    }

    /* ---------------------- Generate Model. --------------------- */

    assert ( closedStnNo <= n_stations && openStnNo <= n_stations );

    if ( openStnNo > 0 && !flags.no_execute ) {
	openModel = new Open( openStation );
    }

    if ( nChains() > 0 && n_servers > 0 ) {
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

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	(*client)->callsPerform(&Call::setChain, number());
	if ( (*client)->replicas() > 1 ) {
	    unsigned n_delta = 0;
	    (*client)->updateWaitReplication( *this, n_delta );
	}
	if ( (*client)->nThreads() > 1 ) {
	    setThreadChain();
	}
    }

    return *this;
}



void
MVASubmodel::setThreadChain() const
{
    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	const ChainVector& aChain = (*client)->clientChains( number() );
	const unsigned kk = aChain[1];
	for ( unsigned ix = 2; ix <= aChain.size(); ++ix ) {
	    const unsigned k = aChain[ix];
	    closedModel->setThreadChain(k, kk);
	}
    }
}



/*
 * Rebuild stations and customers as needed.
 */

MVASubmodel&
MVASubmodel::rebuild()
{
    unsigned k = 0;			/* Chain number.		*/

    /* ----- Set think times, customers and chains for this pass. ----- */

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	const std::set<Entity *,Entity::LT> clientsServers = (*client)->servers( _servers );	/* Get all servers for this client	*/
	const unsigned threads = (*client)->nThreads();

	if ( (*client)->replicas() <= 1 ) {

	    /* ---------------- Simple case --------------- */

	    for ( unsigned i = 1; i <= threads; ++i ) {
		k += 1;				// Add one chain.

		if ( isfinite( (*client)->population() ) ) {
		    myCustomers[k] = static_cast<unsigned>((*client)->population());
		} else {
		    myCustomers[k] = 0;
		}
		myPriority[k]  = (*client)->priority();
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

	    for ( std::set<Entity *>::const_iterator server = clientsServers.begin(); server != clientsServers.end(); ++server ) {
		for ( unsigned i = 1; i <= threads; ++i ) {
		    k += 1;

		    if ( isfinite( (*client)->population() ) ) {
			myCustomers[k] = static_cast<unsigned>((*client)->population()) * (*server)->fanIn(*client);
		    } else {
			myCustomers[k] = 0;
		    }
		}
	    }
	}
    }

    /* ------------------ Recreate servers for model. -----------------	*/

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	if ( (*server)->nEntries() == 0 ) continue;	/* Null server. */
	Server * oldStation = (*server)->serverStation();		/* Get old station		*/
	const unsigned closedIndex = oldStation->closedIndex;	/* Copy over indicies.		*/
	const unsigned openIndex   = oldStation->openIndex;

 	Server * newStation = (*server)->makeServer( nChains() );	/* Returns NULL on no change	*/
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

	delete oldStation;
    }

    return *this;
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

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	const std::set<Entity *,Entity::LT> clientsServers = (*client)->servers( _servers );	/* Get all servers for this client	*/
	const unsigned threads = (*client)->nThreads();

	if ( (*client)->replicas() <= 1 ) {

	    /* ---------------- Simple case --------------- */

	    size_t new_size = myCustomers.size() + threads;
	    myCustomers.resize( new_size ); /* N.B. -- Vector class.  Must*/
	    myThinkTime.resize( new_size ); /* grow() explicitly.	*/
	    myPriority.resize( new_size );

	    for ( unsigned i = 1; i <= threads; ++i ) {
		k += 1;				// Add one chain.

		(*client)->addClientChain( number(), k );	// Set my chain number.
		if ( isfinite( (*client)->population() ) ) {
		    myCustomers[k] = static_cast<unsigned>((*client)->population());
		} else {
		    myCustomers[k] = 0;
		}
		myPriority[k]  = (*client)->priority();

		/* add chain to all servers of this client */

		for_each( clientsServers.begin(), clientsServers.end(), Exec1<Entity,unsigned int>( &Entity::addServerChain, k ) );
	    }

	} else {
	    const unsigned sz = threads * clientsServers.size();

	    //REPL changes
	    /* --------------- Complex case --------------- */

	    //!!! If chains are extended to entries to handle
	    //!!! fanins, modify delta_chains and Entity::fanIn()
	    //!!! for the entry-to-entry case.

	    size_t new_size = myCustomers.size() + sz;
	    myCustomers.resize( new_size );   //Expand vectors to accomodate
	    myThinkTime.resize( new_size );   //new chains.
	    myPriority.resize( new_size );

	    /*
	     * Do all the chains for to a server at once.  This makes
	     * Task::threadIndex() simpler.
	     */

	    for ( std::set<Entity *>::const_iterator server = clientsServers.begin(); server != clientsServers.end(); ++server ) {
		for ( unsigned i = 1; i <= threads; ++i ) {
		    k += 1;

		    (*client)->addClientChain( number(), k );
		    (*server)->addServerChain( k );
		    myPriority[k]  = (*client)->priority();
		    if ( isfinite( (*client)->population() ) ) {
			myCustomers[k] = static_cast<unsigned>((*client)->population()) * (*server)->fanIn((*client));
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
    const ChainVector& aChain = aClient->clientChains( number() );
    Server * aStation = aClient->clientStation( number() );

    for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	const unsigned k = aChain[ix];

	for ( std::vector<Entry *>::const_iterator entry = aClient->entries().begin(); entry != aClient->entries().end(); ++entry ) {
	    const unsigned e = (*entry)->index();

	    for ( unsigned p = 1; p <= (*entry)->maxPhase(); ++p ) {
		const double s = (*entry)->waitExcept( number(), k, p );
		aStation->setService( e, k, p, s );
	    }
	    aStation->setVisits( e, k, 1, (*entry)->prVisit() );	// As client, called-by phase does not matter.
	}

	/* Set idle times for stations. */
	myThinkTime[k] = aClient->thinkTime( number(), k );
    }
}


MVASubmodel&
MVASubmodel::reinitClients()
{
    unsigned k = 0;			/* Chain number.		*/

    /* --- Set think times, customers and chains for this pass. --- */

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	const unsigned threads = (*client)->nThreads();

	if ( (*client)->replicas() <= 1 ) {

	    /* ---------------- Simple case --------------- */

	    for ( unsigned i = 1; i <= threads; ++i ) {
		k += 1;				// Add one chain.

		if ( isfinite( (*client)->population() ) ) {
		    myCustomers[k] = static_cast<unsigned>((*client)->population());
		} else {
		    myCustomers[k] = 0;
		}
		myPriority[k]  = (*client)->priority();
	    }

	} else {
	    const std::set<Entity *,Entity::LT> clientsServers = (*client)->servers( _servers );	/* Get all servers for this client	*/

	    //REPL changes
	    /* --------------- Complex case --------------- */

	    for ( std::set<Entity *>::const_iterator server = clientsServers.begin(); server != clientsServers.end(); ++server ) {
		for ( unsigned i = 1; i <= threads; ++i ) {
		    k += 1;

		    myPriority[k]  = (*client)->priority();
		    if ( isfinite( (*client)->population() ) ) {
			myCustomers[k] = static_cast<unsigned>((*client)->population()) * (*server)->fanIn((*client));
		    } else {
			myCustomers[k] = 0;
		    }
		}
	    }
	}
    }
    return *this;
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

    for ( std::vector<Entry *>::const_iterator entry = aClient->entries().begin(); entry != aClient->entries().end(); ++entry ) {
	const unsigned e = (*entry)->index();

	const ChainVector& aChain = aClient->clientChains( number() );
	for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	    const unsigned k = aChain[ix];
	    for ( unsigned p = 1; p <= (*entry)->maxPhase(); ++p ) {
		aStation->setService( e, k, p, (*entry)->waitExceptChain( number(), k, p ) );
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
    if ( isfinite( s ) && closedModel ) {  //tomari
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
    const std::vector<Entry *>& server_entries = aServer->entries();
    for ( std::vector<Entry *>::const_iterator entry = server_entries.begin(); entry != server_entries.end(); ++entry ) {
	const unsigned e = (*entry)->index();
	const double openArrivalRate = (*entry)->openArrivalRate();

	if ( openArrivalRate > 0.0 ) {
	    aStation->setVisits( e, 0, 1, openArrivalRate );	// Chain 0 reserved for open class.
	}

	/* -- Set service time for entries with visits only. -- */

	for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	    setServiceTime( aServer, (*entry), aChain[ix] );
	}

	/*
	 * Open arrivals and other open models use chain zero which are special
	 * and won't work in the above loop anyway)
	 */

	if ( aServer->isInOpenModel() ) {
	    setServiceTime( aServer, (*entry), 0 );
	}

    }
    /* Overtaking -- compute for MARKOV overtaking only. */

    if ( aServer->markovOvertaking() ) {
	aServer->setOvertaking( number(), _clients );
    }

    /* Set interlock */

    if ( aServer->isInClosedModel() && pragma.getInterlock() == THROUGHPUT_INTERLOCK ) {
	setInterlock( aServer );
    }
}



/*
 * Set the service time for my station.
 */

void
MVASubmodel::setServiceTime( Entity * aServer, const Entry * anEntry, unsigned k ) const
{
    const unsigned e = anEntry->index();
    Server * aStation = aServer->serverStation();

    if ( aStation->V( e, k ) == 0 ) return;

    for ( unsigned p = 1; p <= anEntry->maxPhase(); ++p ) {
	aStation->setService( e, k, p, anEntry->elapsedTimeForPhase(p) );

	if ( aServer->hasVariance() ) {
	    aStation->setVariance( e, k, p, anEntry->varianceForPhase(p) );
	}
    }
}



void
MVASubmodel::setInterlock( Entity * aServer ) const
{
    Server * aStation = aServer->serverStation();

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	if ( (*client)->throughput() == 0.0 ) continue;

	const Probability PrIL = aServer->prInterlock( *(*client) );
	if ( PrIL == 0.0 ) continue;

	const ChainVector& aChain = (*client)->clientChains( number() );

	for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	    const unsigned k = aChain[ix];
	    if ( aServer->hasServerChain(k) ) {
		for ( std::vector<Entry *>::const_iterator entry = aServer->entries().begin(); entry != aServer->entries().end(); ++entry ) {
		    aStation->setInterlock( (*entry)->index(), k, PrIL );
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
    if ( _servers.size() == 0 ) return *this;
    if ( flags.verbose ) cerr << '.';

    const bool trace = flags.trace_mva && (flags.trace_submodel == 0 || flags.trace_submodel == number() );

    MVAStats.start( nChains(), _servers.size() );
    double deltaRep	= 0.0;
    unsigned iter       = 0; //REP N-R

    if ( trace || Options::Debug::variance() ) {
	cout << print_submodel_header( *this, iterations ) << endl;
    }

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	(*server)->serverStation()->clear();	/* Clear visit ratios and what have you */
    }

    /* ------------------- Create the clients. -------------------- */

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	initClient( (*client) );
	if ( (*client)->hasThreads() && pragma.getThreads() != NO_THREADS ) {
	    (*client)->forkOverlapFactor( *this, overlapFactor );
	}

	/* Set visit ratios to all servers for this client */
	/* This will also set arrival rates for open class from sendNoReply */

	(*client)->callsPerform( &Call::setVisits, number() ).openCallsPerform( &Call::setLambda, number() );
    }

    /* ----------------- Create servers for model. ---------------- */

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	initServer( (*server) );
	if ( (*server)->hasSynchs() && pragma.getThreads() != NO_THREADS ) {
	    (*server)->joinOverlapFactor( *this, overlapFactor );
	}
    }

    /* ------------------- Replication Iteration ------------------- */

    do {
	//REP N-R
	iter += 1;

	/* ---- Adjust Client service times for replication ---- */

	if ( hasReplication() ) {

	    if ( flags.trace_mva  ) {
		cout << "\nCurrent master iteration ="<< iterations<<endl;
		cout <<"Replication Iteration Number (submodel=" <<number() <<") = " <<iter ;
		cout << "\ndeltaRep=" << deltaRep;
		cout << ", convergence_value=" << Model::convergence_value<< endl;

	    }

	    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
		modifyClientServiceTime( *client );
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
		catch ( const range_error& error ) {
		    MVAStats.faults += 1;
		    if ( pragma.getStopOnMessageLoss() ) {
			for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
			    const Server * aStation = (*server)->serverStation();
			    for ( unsigned int e = 1; e <= (*server)->nEntries(); ++e ) {
				if ( !isfinite( aStation->R(e,0) ) && aStation->V(e,0) != 0 && aStation->S(e,0) != 0 ) {
				    LQIO::solution_error( ERR_ARRIVAL_RATE, aStation->V(e,0), (*server)->entryAt(e)->name().c_str(), aStation->mu()/aStation->S(e,0) );
				}
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
	    catch ( const range_error& error ) {
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
	    catch ( const range_error& error ) {
		if ( pragma.getStopOnMessageLoss() ) {
		    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
			const Server * aStation = (*server)->serverStation();
			for ( unsigned int e = 1; e <= (*server)->nEntries(); ++e ) {
			    if ( !isfinite( aStation->R(e,0) ) && aStation->V(e,0) != 0 && aStation->S(e,0) != 0 ) {
				LQIO::solution_error( ERR_ARRIVAL_RATE, aStation->V(e,0), (*server)->entryAt(e)->name().c_str(), aStation->mu()/aStation->S(e,0) );
			    }
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
	    ios_base::fmtflags oldFlags = cout.setf( ios::right, ios::adjustfield );
	    cout << *closedModel << endl << endl;
	    cout.flags( oldFlags );
	}

	/* ---------- Set wait and think times for next pass. --------- */

	if (flags.trace_throughput || flags.trace_idle_time) {
	    cout <<"MVASubmodel::solve( ) .... completed solving the MVA model......." << endl;
	}


	for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {

	    /* Get and save the waiting time results for all servers to this client */

	    (*client)->callsPerform( &Call::saveWait, number() )
		.openCallsPerform( &Call::saveOpen, number() );
	    /* Other results (only useful for references tasks. */

	    if ( closedModel ) {
		saveClientResults( (*client) );
	    }
	}


	for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	    saveServerResults( (*server) );
	    (*server)->setIdleTime( relax, this );
	}

	/* --- Compute and save new values for entry service times. --- */

	if ( flags.trace_delta_wait ) {
	    cout << "------ updateWait for submodel " << number() << ", iteration " << iterations << " ------" << endl;
	}

	/* Update waits for replication */

	deltaRep = 0.0;
	if ( hasReplication() ) {
	    unsigned n_deltaRep = 0;
	    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
		deltaRep += (*client)->updateWaitReplication( *this, n_deltaRep );
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

	for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	    (*client)->updateWait( *this, relax );
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
    const ChainVector& myChain( aClient->clientChains(number()));

    for ( std::vector<Entry *>::const_iterator entry = aClient->entries().begin(); entry != aClient->entries().end(); ++entry ) {
	/*Positive*/ double lambda = 0; // to get rid of the exception
	//when I set the service time of an activity "Reply" to zero
	//the throughput will be infinity when using the Gamma distribution.

	if ( hasReplication() ) {

	    /*
	     * Get throughput PER CUSTOMER because replication
	     * monkeys with the population levels.  Fix for
	     * multiservers.
	     */

	    lambda = closedModel->normalizedThroughput( *aStation, (*entry)->index(), myChain[1] ) * aClient->population();
	} else {
	    lambda = closedModel->throughput( *aStation, (*entry)->index(), myChain[1] );
	}
	(*entry)->setThroughput( lambda );
    }
}


/*
 * Save server results.  Servers only occur in one submodel.
 */
void
MVASubmodel::saveServerResults( Entity * aServer )
{
    const Server * aStation = aServer->serverStation();

    for ( std::vector<Entry *>::const_iterator entry = aServer->entries().begin(); entry != aServer->entries().end(); ++entry ) {
	const unsigned e = (*entry)->index();
	double lambda = 0.0;

	if ( aServer->isInOpenModel() && openModel ) {
	    lambda = openModel->entryThroughput( *aStation, e );		/* BUG_168 */
	}

	if ( aServer->isInClosedModel() && closedModel ) {
	    const double tput = closedModel->entryThroughput( *aStation, e );
	    if ( isfinite( tput ) ) {
		lambda += tput;
	    } else if ( tput < 0.0 ) {
		throw domain_error( "MVASubmodel::saveServerResults" );
	    } else {
		lambda = tput;
		break;
	    }
	}
	(*entry)->setThroughput( lambda );
	(*entry)->saveOpenWait( aStation->R( e, 0 ) );
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

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	output << setw(2) << "  " << *(*client) << endl;
    }
    output << endl << "Servers: " << endl;

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	output << setw(2) << "  " << **server << endl;
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
    unsigned stnNo = 1;

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	output << "[closed=" << stnNo << "] " << **client << endl
	       << Task::print_client_chains( **client, number() )
	       << *(*client)->clientStation( number() ) << endl;
	stnNo += 1;
    }

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	if ( (*server)->isInClosedModel() ) {
	    output << "[closed=" << stnNo << "] " << **server << endl
		   << Entity::print_server_chains( **server )
		   << *(*server)->serverStation() << endl;
	    (*server)->serverStation()->printOutput( output, stnNo );
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
    unsigned stnNo = 1;
    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	if ( (*server)->isInOpenModel() ) {
	    output << "[open=" << stnNo << "] " << **server << endl
		   << *(*server)->serverStation() << endl;
	    stnNo += 1;
	}
    }
    return output;
}
