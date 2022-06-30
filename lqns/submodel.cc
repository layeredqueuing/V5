/* -*- c++ -*-
 * submodel.C	-- Greg Franks Wed Dec 11 1996
 * $Id: submodel.cc 15735 2022-06-30 03:18:14Z greg $
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

#include "lqns.h"
#include <cstdlib>
#include <cmath>
#include <mva/mva.h>
#include <mva/open.h>
#include <mva/prob.h>
#include <mva/server.h>
#include "activity.h"
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "generate.h"
#include "group.h"
#include "interlock.h"
#include "flags.h"
#include "model.h"
#include "option.h"
#include "overtake.h"
#include "pragma.h"
#include "processor.h"
#include "report.h"
#include "submodel.h"
#include "task.h"

/*----------------------------------------------------------------------*/
/*                        Merged Partition Model                        */
/*----------------------------------------------------------------------*/

Submodel::Submodel( const unsigned n ) :
    _submodel_number(n),
    _n_chains(0)
{
}

/*
 * Set my submodel number to n.  Reset submodel numbers of all servers.
 */

Submodel&
Submodel::setSubmodelNumber( const unsigned n )
{
    _submodel_number = n;
    std::for_each( _servers.begin(), _servers.end(), Exec1<Entity,const unsigned>( &Entity::setSubmodel, n ) );
    return *this;
}

#if 0
// DEBUG
void
Submodel::setThinkTime( unsigned int k, double thinkTime )
{
    std::cerr << "Submodel " << number() << " chain " << k << ": think time = " << thinkTime << std::endl;
    _thinkTime[k] = thinkTime;
}
#endif

/*
 * Now create client tables which are simply all callers to
 * the servers at each level.  Note that _clients is an intersection
 * of all clients to all servers.
 */

Submodel&
Submodel::addClients()
{
    _clients = std::accumulate( _servers.begin(), _servers.end(), _clients, Entity::add_clients );
    return *this;
}


/*
 * Handy debug function
 */

void
Submodel::debug_stop( const unsigned long iterations, const double delta ) const
{
    if ( !std::cin.eof() && iterations >= flags.single_step ) {
	std::cerr << "**** Submodel " << number() << " **** Delta = " << delta << ", Continue [yna]? ";
	std::cerr.flush();

	char c;

	std::cin >> c;

	switch ( c ) {
	case '\n':
	case '\004':
	    break;

	case 'n':
	case 'N':
	    throw std::runtime_error( "Submodel::debug_stop" );
	    break;

	case 'a':
	    LQIO::internal_error( __FILE__, __LINE__, "debug abort" );
	    break;

	default:
	    std::cin.ignore( 100, '\n' );
	    break;
	}
    }
}


std::ostream&
Submodel::submodel_header_str( std::ostream& output, const Submodel& submodel, const unsigned long iterations )
{
    output << "========== Iteration " << iterations << ", "
	   << submodel.submodelType() << " " << submodel.number() << ": "
	   << submodel._clients.size() << " client" << (submodel._clients.size() != 1 ? "s, " : ", ")
	   << submodel._servers.size() << " server" << (submodel._servers.size() != 1 ? "s."  : ".")
	   << "==========";
    return output;
}

/*----------------------------------------------------------------------*/
/*			     MVA Submodel				*/
/*----------------------------------------------------------------------*/

/*
 * Create a new MVA submodel.
 */

MVASubmodel::MVASubmodel( const unsigned n )
    : Submodel(n),
      _hasThreads(false),
      _hasSynchs(false),
      _hasReplicas(false),
      _customers(),
      _thinkTime(),
      _priority(),
      _closedStation(),
      _openStation(),
      _closedModel(nullptr),
      _openModel(nullptr),
      _overlapFactor()
{
}


/*
 * Free allocated store.  Do not delete stations -- stations are deleted by
 * tasks/processors.
 */

MVASubmodel::~MVASubmodel()
{
    if ( _openModel ) {
	delete _openModel;
    }
    if ( _closedModel ) {
	delete _closedModel;
    }
    if ( _overlapFactor ) {
	delete [] _overlapFactor;
    }
}


/*----------------------------------------------------------------------*/
/*                       Initialize the model.                          */
/*----------------------------------------------------------------------*/


/*
 * Initialize server's waiting times and populations.
 */

MVASubmodel&
MVASubmodel::initServers( const Model& model )
{
    std::for_each( _servers.begin(), _servers.end(), Exec1<Entity,const Vector<Submodel *>&>( &Entity::initServer, model.getSubmodels() ) );
    return *this;
}



/*
 * Initialize server's waiting times and populations.
 */

MVASubmodel&
MVASubmodel::reinitServers( const Model& model )
{
    std::for_each( _servers.begin(), _servers.end(), Exec1<Entity,const Vector<Submodel *>&>( &Entity::reinitServer, model.getSubmodels() ) );
    return *this;
}



/*
 * Go through all servers and set up path tables.
 */

MVASubmodel&
MVASubmodel::initInterlock()
{
    std::for_each( _servers.begin(), _servers.end(), Exec<Entity>( &Entity::initInterlock ) );
    return *this;
}


#if PAN_REPLICATION
/*
 * Return true if we are using Amy Pan's replication code and replicas are present.
 */

bool
MVASubmodel::usePanReplication() const
{
    return Pragma::pan_replication() && hasReplicas();
}
#endif


/*
 * Build a submodel.
 */

MVASubmodel&
MVASubmodel::build()
{
    static const std::map<const Pragma::MVA, MVA::new_solver> solvers = {
	{ Pragma::MVA::EXACT, 	    	    ExactMVA::create },
	{ Pragma::MVA::SCHWEITZER,	    Schweitzer::create },
	{ Pragma::MVA::LINEARIZER,	    Linearizer::create },
	{ Pragma::MVA::FAST,	    	    Linearizer2::create },
	{ Pragma::MVA::ONESTEP,	    	    OneStepMVA::create },
	{ Pragma::MVA::ONESTEP_LINEARIZER,  OneStepLinearizer::create },
    };

    /* BUG 144 */
    if ( _servers.empty() ) {
	if ( !Pragma::allowCycles()  ) {
	    LQIO::runtime_error( ADV_EMPTY_SUBMODEL, number() );
	}
	return *this;
    }

    /* -------------- Initialize instance variables. ------------------ */

    _hasThreads  = std::any_of( _clients.begin(), _clients.end(), Predicate<Task>( &Task::hasThreads ) );
    _hasSynchs   = std::any_of( _servers.begin(), _servers.end(), Predicate<Entity>( &Entity::hasSynchs ) );
    _hasReplicas = std::any_of( _clients.begin(), _clients.end(), Predicate<Task>( &Task::isReplicated ) )
                || std::any_of( _servers.begin(), _servers.end(), Predicate<Entity>( &Entity::isReplicated ) );
    
    /* --------------------- Count the stations. ---------------------- */

    const unsigned n_stations  = _clients.size() + _servers.size();
    _closedStation.resize( std::count_if( _clients.begin(), _clients.end(), Predicate<Entity>( &Entity::isClosedModelClient ) )
			  + std::count_if( _servers.begin(), _servers.end(), Predicate<Entity>( &Entity::isClosedModelServer ) ) );
    _openStation.resize( std::count_if( _servers.begin(), _servers.end(), Predicate<Entity>( &Entity::isOpenModelServer ) ) );

    /* ----------------------- Create Chains.  ------------------------ */

    setNChains( makeChains() );
    if ( nChains() > MAX_CLASSES ) {
	LQIO::runtime_error( ADV_MANY_CLASSES, nChains(), number() );
    }

    /* --------------------- Create the clients. ---------------------- */

    unsigned closedStnNo = 0;
    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	if ( (*client)->isClosedModelClient() ) {
	    closedStnNo += 1;
	    _closedStation[closedStnNo] = (*client)->makeClient( nChains(), number() );
	}
    }

    /* ------------------- Create servers for model. ------------------ */

    unsigned openStnNo = 0;
    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	if ( (*server)->nEntries() == 0 ) continue;	/* Null server. */
	Server * aStation = (*server)->makeServer( nChains() );
	if ( (*server)->isClosedModelServer() ) {
	    closedStnNo += 1;
	    _closedStation[closedStnNo] = aStation;
	}
	if ( (*server)->isOpenModelServer() ) {
	    openStnNo += 1;
	    _openStation[openStnNo] = aStation;
	}
    }

    /* --------- Create overlap probabilities and durations. ---------- */

    if ( ( hasThreads() || hasSynchs() ) && !Pragma::threads(Pragma::Threads::NONE) ) {
	_overlapFactor = new Vector<double> [nChains()+1];
	for ( unsigned i = 1; i <= nChains(); ++i ) {
	    _overlapFactor[i].resize( nChains(), 1.0 );
	}
    }

    /* ------------------------ Generate Model. ----------------------- */

    assert ( closedStnNo <= n_stations && openStnNo <= n_stations );

    if ( n_openStns() > 0 && !flags.no_execute ) {
	_openModel = new Open( _openStation );
    }

    if ( nChains() > 0 && n_closedStns() > 0 ) {
	const MVA::new_solver solver = solvers.at(Pragma::mva());
	_closedModel = (*solver)( _closedStation, _customers, _thinkTime, _priority, _overlapFactor );
    }

    std::for_each( _clients.begin(), _clients.end(), ConstExec1<Task,MVASubmodel&>( &Task::setChains, *this ) );
#if PAN_REPLICATION
    if ( usePanReplication() ) {
	unsigned not_used = 0;
	std::for_each( _clients.begin(), _clients.end(), ExecSum2<Task,double,const Submodel&,unsigned&>( &Task::updateWaitReplication, *this, not_used ) );
    }
#endif
    return *this;
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
	const std::set<Entity *> clientsServers = (*client)->getServers( _servers );	/* Get all servers for this client	*/
	const unsigned threads = (*client)->nThreads();

#if PAN_REPLICATION
	if ( !usePanReplication() || (*client)->replicas() <= 1 ) {

	    /* ---------------- Simple case --------------- */
#endif

	    for ( unsigned i = 1; i <= threads; ++i ) {
		k += 1;				// Add one chain.

		if ( std::isfinite( (*client)->population() ) ) {
		    _customers[k] = static_cast<unsigned>((*client)->population());
		} else {
		    _customers[k] = 0;
		}
		_priority[k]  = (*client)->priority();
	    }

#if PAN_REPLICATION
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

		    if ( std::isfinite( (*client)->population() ) ) {
			_customers[k] = static_cast<unsigned>((*client)->population()) * (*server)->fanIn(*client);
		    } else {
			_customers[k] = 0;
		    }
		}
	    }
	}
#endif
    }

    /* ------------------ Recreate servers for model. -----------------	*/

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	if ( (*server)->nEntries() == 0 ) continue;		/* Null server. */
	Server * oldStation = (*server)->serverStation();	/* Get old station		*/
	const unsigned closedIndex = oldStation->closedIndex;	/* Copy over indicies.		*/
	const unsigned openIndex   = oldStation->openIndex;

 	Server * newStation = (*server)->makeServer( nChains() );	/* Returns NULL on no change	*/
	if ( !newStation )  continue;				/* Out with the old...		*/

	newStation->setMarginalProbabilitiesSize( _customers );

	if ( closedIndex ) {
	    newStation->closedIndex = closedIndex;
	    _closedStation[closedIndex] = newStation;		/* ... and in with the new...	*/
	}
	if ( openIndex ) {
	    newStation->openIndex = openIndex;
	    _openStation[openIndex] = newStation;		/* ... and in with the new...	*/
	}

	delete oldStation;
    }

    return *this;
}



/*
 * Look for disjoint chains.  This works as-is for the simple case.
 * However, it will fail for fan-in.  So:
 *   1) starting from the first client, form a set of tasks consisting of all the servers and clients to those servers.  
 *   2) Repeat Using the next client not found in this set and form a second set.  Compare the two (or more) sets for
 *      intersection.  Non-intersecting sets which match based on name (but not replica) can be pruned.  Non intersecting
 *      sets that fail pruning can be partitioned into a new submodel.  Don't forget to ++__sync_submodel and renumber
 *      all submodels.
 *   3) For pruned tasks, I believe the easiest thing to do for update is to copy the waiting times across in
 *      delta_wait. Multiplying Call::rendezvousDelay() doesn't seem to work.
 *
 */

Submodel&
Submodel::optimize()
{
    if ( _clients.size() <= 1 ) return *this;	/* No operation */
    std::vector<submodel_group_t> groups;
    
    /* Collect all servers for each client */
    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	const std::set<Task *>& group = groups.back().first;
	if ( !groups.empty() && std::find( group.begin(), group.end(), *client ) != group.end() ) continue;	/* already there */
	groups.resize( groups.size() + 1 );
	addToGroup( *client, groups.back() );
    }

    /* locate groups which match */
    std::vector<submodel_group_t*> disjoint;
    for ( std::vector<submodel_group_t>::iterator i = groups.begin(); i != groups.end(); ++i ) {
	bool can_prune = true;
	for ( std::vector<submodel_group_t>::const_iterator j = groups.begin(); can_prune && j != groups.end(); ++j ) {
	    if ( i == j ) continue;
	    std::set<Entity *> intersection;
	    std::set_intersection( i->second.begin(), i->second.end(),
				   j->second.begin(), j->second.end(),
				   std::inserter( intersection, intersection.end() ) );
	    if ( !intersection.empty() ) can_prune = false;
	}
	if ( can_prune ) {
	    disjoint.push_back( &(*i) );
	}
    }
    
#if BUG_299_PRUNE
    /* pruneble if clients are replicas of each other */
    if ( Pragma::replication() == Pragma::Replication::PRUNE ) {
	std::set<submodel_group_t*> purgeable;
	for ( std::vector<submodel_group_t*>::const_iterator i = disjoint.begin(); i != disjoint.end(); ++i ) {
	    for ( std::vector<submodel_group_t*>::const_iterator j = std::next(i); j != disjoint.end(); ++j ) {
		/* If all the clients in j are replicas of i, then prune j */
		if ( !replicaGroups( (*i)->first, (*j)->first ) ) continue;
#if 0
		std::cerr << "Submodel " << number() << ", can prune: ";
		std::set<Task *>& clients = (*j)->first;
		for ( std::set<Task *>::const_iterator client = clients.begin(); client != clients.end(); ++client ) {
		    std::cerr << (*client)->name() << ":" << (*client)->getReplicaNumber();
		}
		std::cerr << std::endl;
#endif
		purgeable.insert( *j );
	    }
	}

	for ( std::set<submodel_group_t*>::const_iterator purge = purgeable.begin(); purge != purgeable.end(); ++purge ) {
	    std::for_each( (*purge)->first.begin(), (*purge)->first.end(), erase_from<Task *>(  _clients ) );
	    std::for_each( (*purge)->second.begin(), (*purge)->second.end(), erase_from<Entity *>( _servers ) );
	}
    }
#endif
    
    return *this;
}



void
Submodel::addToGroup( Task * task, submodel_group_t& group ) const
{
    group.first.insert( task );

    /* Find the servers of this client (in this submodel) */
    const std::set<Entity *> servers = task->getServers( _servers );
    group.second.insert( servers.begin(), servers.end() );

    /* Now, find all of the clients of the servers just found, and repeat this for any new client in this submodel's _clients */

    for ( std::set<Entity *>::const_iterator server = servers.begin(); server != servers.end(); ++server ) {
	std::set<Task *> clients;
	(*server)->getClients( clients );

	/* If a client is in this submodel and has not been found already, recurse */
	for ( std::set<Task *>::const_iterator client = clients.begin(); client != clients.end(); ++client ) {
	    if ( _clients.find( *client ) == _clients.end() || group.first.find( *client ) != group.first.end() ) continue;
	    addToGroup( *client, group );
	}
    }
}



/*
 * Check b for replicas in found in a.  They must all be.
 */

bool
Submodel::replicaGroups( const std::set<Task *>& a, const std::set<Task *>& b ) const
{
    std::set<Task *> dst = b;	/* Copy for erasure */
    for ( std::set<Task *>::const_iterator src = a.begin(); src != a.end(); ++src ) {
	std::set<Task *>::iterator item = std::find_if( dst.begin(), dst.end(), Entity::matches( (*src)->name() ) );
	if ( item == dst.end() ) return false;
	dst.erase( item );
    }
    return dst.empty();
}



void
MVASubmodel::setChains( const ChainVector& chain )
{
    const unsigned k1 = chain[1];
    for ( ChainVector::const_iterator k2 = std::next(chain.begin()); k2 != chain.end(); ++k2 ) {
	_closedModel->setThreadChain( *k2, k1 );
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

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	const std::set<Entity *> clientsServers = (*client)->getServers( _servers );	/* Get all servers for this client	*/
	const unsigned threads = (*client)->nThreads();

#if PAN_REPLICATION
	if ( !usePanReplication() || (*client)->replicas() <= 1 ) {

	    /* ---------------- Simple case --------------- */
#endif

	    const size_t new_size = _customers.size() + threads;
	    _customers.resize( new_size ); /* N.B. -- Vector class.  Must*/
	    _thinkTime.resize( new_size, static_cast<double>(0.0) ); /* grow() explicitly.	*/
	    _priority.resize( new_size, static_cast<unsigned int>(0) );

	    for ( unsigned i = 1; i <= threads; ++i ) {
		k += 1;				// Add one chain.

		(*client)->addClientChain( number(), k );	// Set my chain number.
		if ( std::isfinite( (*client)->population() ) ) {
		    _customers[k] = static_cast<unsigned>((*client)->population());
		} else {
		    _customers[k] = 0;
		}
		_priority[k]  = (*client)->priority();

		/* add chain to all servers of this client */

		std::for_each( clientsServers.begin(), clientsServers.end(), Exec1<Entity,unsigned int>( &Entity::addServerChain, k ) );
	    }

#if PAN_REPLICATION
	} else {
	    const unsigned sz = threads * clientsServers.size();

	    //REPL changes
	    /* --------------- Complex case --------------- */

	    //!!! If chains are extended to entries to handle
	    //!!! fanins, modify delta_chains and Entity::fanIn()
	    //!!! for the entry-to-entry case.

	    const size_t new_size = _customers.size() + sz;
	    _customers.resize( new_size );   //Expand vectors to accomodate
	    _thinkTime.resize( new_size, static_cast<double>(0.0) ); /* grow() explicitly.	*/
	    _priority.resize( new_size, static_cast<unsigned int>(0) );

	    /*
	     * Do all the chains for to a server at once.  This makes
	     * Task::threadIndex() simpler.
	     */

	    for ( std::set<Entity *>::const_iterator server = clientsServers.begin(); server != clientsServers.end(); ++server ) {
		for ( unsigned i = 1; i <= threads; ++i ) {
		    k += 1;

		    (*client)->addClientChain( number(), k );
		    (*server)->addServerChain( k );
		    _priority[k]  = (*client)->priority();
		    if ( std::isfinite( (*client)->population() ) ) {
			_customers[k] = static_cast<unsigned>((*client)->population()) * (*server)->fanIn((*client));
		    } else {
			_customers[k] = 0;
		    }
		}
	    }
	}
#endif
    }
    return k;
}


#if PAN_REPLICATION
//REPL changes
/*
 * Query the closed model to determine the factor needed for
 * Newton Raphson stuff.
 * (5.18) [Pan, pg 73].
 */

double
MVASubmodel::nrFactor( const Server * aStation, const unsigned e, const unsigned k ) const
{
    const double s = aStation->S( e, k, 1 );
    if ( std::isfinite( s ) && _closedModel ) {  //tomari
	//Solution for replicated models with open arrivals.
	// See bug # 87.

	return _closedModel->nrFactor( *aStation, e, k ) * s;

    } else {
	return s;
    }
}
#endif

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
    if ( _servers.empty() ) return *this;
    if ( Options::Trace::verbose() ) std::cerr << '.';
    if ( flags.reset_mva ) { _closedModel->reset(); }

    const bool trace = Options::Trace::mva( number() );

    MVAStats.start( nChains(), _servers.size() );

    if ( trace || Options::Debug::variance() ) {
	std::cout << print_submodel_header( *this, iterations ) << std::endl;
    }


    /* ----------------- initialize the stations ------------------ */

    std::for_each( _servers.begin(), _servers.end(), Exec<Entity>( &Entity::clear ) );	/* Clear visit ratios and what have you */
    std::for_each( _clients.begin(), _clients.end(), Exec1<Task,Submodel&>( &Task::initClientStation, *this ) );
    std::for_each( _servers.begin(), _servers.end(), Exec1<Entity,Submodel&>( &Entity::initServerStation, *this ) );

#if PAN_REPLICATION
    /* ------------------- Replication Iteration ------------------- */

    double deltaRep	= 0.0;
    unsigned iter       = 0; //REP N-R

    do {

	iter += 1;

	/* ---- Adjust Client service times for replication ---- */

	if ( usePanReplication() ) {

	    if ( trace  ) {
		std::cout << std::endl << "Current master iteration = " << iterations << std::endl
		     << "  Replication Iteration Number (submodel=" <<number() <<") = " << iter << std::endl
		     << "  deltaRep = " << deltaRep << ", convergence_value = " << Model::__convergence_value << std::endl;

	    }

	    std::for_each( _clients.begin(), _clients.end(), Exec1<Task,const MVASubmodel&>( &Task::modifyClientServiceTime, *this ) );
	}
#endif

	if ( flags.generate ) {			// Print out MVA model as C++ source file.
	    Generate::program( *this );
	}

	/* ----------------- Solve the model. ----------------- */

	if ( _closedModel ) {

	    if ( _openModel ) {
		if ( trace ) {
		    printOpenModel( std::cout );
		}

		/*
		 * If model has any open classes, convert for closed model.
		 */

		try {
		    _openModel->convert( _customers );
		}
		catch ( const std::range_error& error ) {
		    MVAStats.faults += 1;
		    if ( Pragma::stopOnMessageLoss() && std::any_of( _servers.begin(), _servers.end(), Predicate<Entity>( &Entity::openModelInfinity ) ) ) {
			throw;
		    }
		}
	    }

	    if ( trace ) {
		printClosedModel( std::cout );
	    }
	    try {
		_closedModel->solve();
	    }
	    catch ( const std::range_error& error ) {
		throw;
	    }

	    /* Statistics by level -- we can use this to find performance bottlenecks */

	    MVAStats.accumulate( _closedModel->iterations(), _closedModel->waits(), _closedModel->faults() );
	}

	if ( _openModel ) {
	    if ( trace && !_closedModel ) {
		printOpenModel( std::cout );
	    }

	    try {
		if ( _closedModel ) {
		    _openModel->solve( *_closedModel, _customers );	/* Calculate L[0] queue lengths. */
		} else {
		    _openModel->solve();
		}
	    } 
	    catch ( const std::range_error& error ) {
		if ( Pragma::stopOnMessageLoss() && std::any_of( _servers.begin(), _servers.end(), Predicate<Entity>( &Entity::openModelInfinity ) ) ) {
		    throw;
		}
	    }

	    if ( trace ) {
		std::cout << *_openModel << std::endl << std::endl;
	    }
	}

	if ( _closedModel && trace ) {
	    std::ios_base::fmtflags oldFlags = std::cout.setf( std::ios::right, std::ios::adjustfield );
	    std::cout << *_closedModel << std::endl << std::endl;
	    std::cout.flags( oldFlags );
	}

	/* ---------- Set wait and think times for next pass. --------- */

	if (flags.trace_throughput || flags.trace_idle_time) {
	    std::cout <<"MVASubmodel::solve( ) .... completed solving the MVA model......." << std::endl;
	}

	std::for_each( _clients.begin(), _clients.end(), Exec1<Task,const MVASubmodel&>( &Task::saveClientResults, *this ) );
	std::for_each( _servers.begin(), _servers.end(), Exec2<Entity,const MVASubmodel&,double>( &Entity::saveServerResults, *this, relax ) );

	/* --- Compute and save new values for entry service times. --- */

	if ( Options::Trace::delta_wait( number() ) ) {
	    std::cout << "------ updateWait for submodel " << number() << ", iteration " << iterations << " ------" << std::endl;
	}

#if PAN_REPLICATION
	/* Update waits for replication */

	deltaRep = 0.0;
	if ( usePanReplication() ) {
	    unsigned n_deltaRep = 0;
	    deltaRep = std::for_each( _clients.begin(), _clients.end(), ExecSum2<Task,double,const Submodel&,unsigned&>( &Task::updateWaitReplication, *this, n_deltaRep ) ).sum();
	    if ( n_deltaRep ) {
		deltaRep = sqrt( deltaRep / n_deltaRep );	/* Take RMS value over all phases */
	    }
	    if ( iter >= Model::__iteration_limit ) {
		LQIO::runtime_error( ADV_REPLICATION_ITERATION_LIMIT, number(), iter, deltaRep, Model::__convergence_value );
		deltaRep = 0;		/* Break out of loop */
	    }
	}
#endif

	/* Update waits for everyone else. */

	std::for_each( _clients.begin(), _clients.end(), Exec2<Task,const Submodel&,double>( &Task::updateWait,  *this, relax ) );

	if ( !check_fp_ok() ) {
	    throw floating_point_error( __FILE__, __LINE__ );
	}

	if ( flags.single_step ) {
	    debug_stop( iterations, 0 );
	}

#if PAN_REPLICATION
    } while ( usePanReplication() && deltaRep > Model::__convergence_value );

    /* ----------------End of Replication Iteration --------------- */
#endif

    return *this;
}


double
MVASubmodel::openModelThroughput( const Server& station, unsigned int e ) const
{
    return _openModel != nullptr ? _openModel->entryThroughput( station, e ) : 0.0;
}

double
MVASubmodel::closedModelThroughput( const Server& station, unsigned int e ) const
{
    return _closedModel != nullptr ? _closedModel->entryThroughput( station, e ) : 0.0;
}

double
MVASubmodel::closedModelThroughput( const Server& station, unsigned int e, unsigned int k ) const
{
    return _closedModel != nullptr ? _closedModel->throughput( station, e, k ) : 0.0;
}

#if PAN_REPLICATION
double
MVASubmodel::closedModelNormalizedThroughput( const Server& station, unsigned int e, unsigned int k ) const
{
    return _closedModel != nullptr ? _closedModel->normalizedThroughput( station, e, k ) : 0.0;
}
#endif

double
MVASubmodel::closedModelUtilization( const Server& station, unsigned int k ) const
{
    return _closedModel != nullptr ? _closedModel->utilization( station, k ) : 0.0;
}

double
MVASubmodel::openModelUtilization( const Server& station ) const
{
    return _openModel != nullptr ? _openModel->utilization( station ) : 0.0;
}

/*----------------------------------------------------------------------*/
/* Printing functions.							*/
/*----------------------------------------------------------------------*/

/*
 * Print out a submodel.
 */

std::ostream&
MVASubmodel::print( std::ostream& output ) const
{
    if ( !Options::Debug::submodels( number() ) ) return output;
    
    output << "----------------------- Submodel  " << number() << " -----------------------" << std::endl
	   << "Customers: " <<  _customers << std::endl
	   << "Clients: " << std::endl;
    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	output << std::setw(2) << "  " << **client << std::endl;
    }

    if ( std::any_of( _servers.begin(), _servers.end(), Predicate<Entity>( &Entity::isClosedModelServer ) ) ) {
	output << std::endl << "Closed Model Servers: " << std::endl;
	std::for_each( _servers.begin(), _servers.end(), print_server( output, &Entity::isClosedModelServer ) );
    }
    if ( std::any_of( _servers.begin(), _servers.end(), Predicate<Entity>( &Entity::isOpenModelServer ) ) ) {
	output << std::endl << "Open Model Servers: " << std::endl;
	std::for_each( _servers.begin(), _servers.end(), print_server( output, &Entity::isOpenModelServer ) );
    }

    output << std::endl << "Calls: " << std::endl;
    std::for_each( Model::__entry.begin(), Model::__entry.end(), ConstPrint1<Entry,unsigned int>( &Entry::printCalls, output, number() ) );
    std::for_each( Model::__processor.begin(), Model::__processor.end(), ConstPrint1<Processor,unsigned int>( &Processor::printTasks, output, number() ) );
    output << std::endl;
    return output;
}


void
MVASubmodel::print_server::operator()( const Entity * server ) const
{
    if ( (server->*_predicate)() ) {
	_output << std::setw(2) << "  " << *server << std::endl;
    }
}



/*
 * Print stations of closed model.
 */

std::ostream&
MVASubmodel::printClosedModel( std::ostream& output ) const
{
    unsigned stnNo = 1;

    for ( std::set<Task *>::const_iterator client = _clients.begin(); client != _clients.end(); ++client ) {
	if ( (*client)->isClosedModelClient() ) {
	    output << "[closed=" << stnNo << "] " << **client << std::endl
		   << Task::print_client_chains( **client, number() )
		   << *(*client)->clientStation( number() ) << std::endl;
	    stnNo += 1;
	}
    }

    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	if ( (*server)->isClosedModelServer() ) {
	    output << "[closed=" << stnNo << "] " << **server << std::endl
		   << Entity::print_server_chains( **server )
		   << *(*server)->serverStation() << std::endl;
	    (*server)->serverStation()->printOutput( output, stnNo );
	    stnNo += 1;
	}
    }
    return output;
}



/*
 * Print stations of open model.
 */

std::ostream&
MVASubmodel::printOpenModel( std::ostream& output ) const
{
    unsigned stnNo = 1;
    for ( std::set<Entity *>::const_iterator server = _servers.begin(); server != _servers.end(); ++server ) {
	if ( (*server)->isOpenModelServer() ) {
	    output << "[open=" << stnNo << "] " << **server << std::endl
		   << *(*server)->serverStation() << std::endl;
	    stnNo += 1;
	}
    }
    return output;
}
