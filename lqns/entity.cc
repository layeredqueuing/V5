/* -*- c++ -*-
 * $Id: entity.cc 13975 2020-10-20 19:37:55Z greg $
 *
 * Everything you wanted to know about a task or processor, but were
 * afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <lqio/error.h>
#include <lqio/labels.h>
#include <lqio/dom_extvar.h>
#include "errmsg.h"
#include "vector.h"
#include "fpgoop.h"
#include "entity.h"
#include "lqns.h"
#include "errmsg.h"
#include "entry.h"
#include "task.h"
#include "pragma.h"
#include "call.h"
#include "open.h"
#include "variance.h"
#include "server.h"
#include "ph2serv.h"
#include "submodel.h"
#include "mva.h"


#define DEFERRED_UTULIZATION	false

/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Printing function.
 */

ostream&
operator<<( ostream& output, const Entity& self )
{
    self.print( output );
    return output;
}



/*
 * Comparison function.
 */

int
operator==( const Entity& a, const Entity& b )
{
    return a.name() == b.name();
}

/* ----------------------- Abstract Superclass. ----------------------- */

/*
 * Set me up.
 */

Entity::Entity( LQIO::DOM::Entity* dom, const std::vector<Entry *>& entries )
    : _dom(dom),
      _entries(entries),
      _tasks(),
      myPopulation(1.0),
      myVariance(0.0),
      myThinkTime(0.0),
      myServerStation(0),		/* Reference tasks don't have server stations. */
      _interlock( *this ),
      _submodel(0),
      _maxPhase(1),
      _utilization(0),
      _lastUtilization(-1.0)		/* Force update 		*/
{
    attributes.initialized      = 0;		/* entity was initialized.	*/
    attributes.closed_model	= 0;		/* Stn in in closed model.     	*/
    attributes.open_model	= 0;		/* Stn is in open model.	*/
    attributes.deterministic    = 0;		/* an entry has det. phase.	*/
    attributes.pure_delay       = 0;		/* Special task of some form.	*/
    attributes.pure_server	= 1;		/* Can use FCFS schedulging.	*/
    attributes.variance         = 0;		/* */
}

/*
 * Delete all entries associated with this task.
 */

Entity::~Entity()
{
    delete myServerStation;
}



/*
 * model max phase for this entity.
 */

Entity&
Entity::configure( const unsigned nSubmodels )
{
    if ( !Pragma::variance(Pragma::NO_VARIANCE) && nEntries() > 1 && Pragma::entry_variance() ) {
	attributes.variance = 1;
    }
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	(*entry)->configure( nSubmodels );
	_maxPhase = std::max( _maxPhase, (*entry)->maxPhase() );

	if ( (*entry)->hasDeterministicPhases() ) {
	    attributes.deterministic = 1;
	}
	if ( !Pragma::variance(Pragma::NO_VARIANCE) && (*entry)->hasVariance() ) {
	    attributes.variance = 1;
	}
    }
    return *this;
}



/*
 * Recursively find all children and grand children from `father'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
Entity::findChildren( Call::stack& callStack, const bool ) const
{
    unsigned max_depth = max( submodel(), callStack.depth() );

    const_cast<Entity *>(this)->setSubmodel( max_depth );
    return max_depth;
}



/*
 * Initialize waiting time at my entries.
 */

Entity&
Entity::initWait()
{
    for_each( entries().begin(), entries().end(), Exec<Entry>( &Entry::initWait ) );
    return *this;
}


Entity&
Entity::initInterlock()
{
    _interlock.initialize();
    return *this;
}



/*
 * Copies is always an unsigned integer greater than zero.  Infinite copies are infinite (delay) servers
 * and should have exactly one instance.
 */

unsigned
Entity::copies() const
{
    unsigned int value = 1;
    if ( !getDOM()->isInfinite() ) {
	try { 
	    value = getDOM()->getCopiesValue();
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "multiplicity", getDOM()->getTypeName(), name().c_str(), e.what() );
	    throw_bad_parameter();
	}
    }
    return value;
}


/*
 * Return the number of replicas (default is 1).
 */

unsigned
Entity::replicas() const
{
    unsigned int value = 1;
    try {
	value = getDOM()->getReplicasValue();
    }
    catch ( const std::domain_error &e ) {
	solution_error( LQIO::ERR_INVALID_PARAMETER, "replicas", getDOM()->getTypeName(), name().c_str(), e.what() );
	throw_bad_parameter();
    }
    return value;
}


bool 
Entity::isInfinite() const
{
    return getDOM()->isInfinite();
}



bool
Entity::isCalledBy( const Task* task ) const
{
    return find( tasks().begin(), tasks().end(), task ) != tasks().end();
}



/*
 * Add an entry to the list of entries for this task and set local index
 * for MVA.
 */

Entity&
Entity::addEntry( Entry * anEntry )
{
    _entries.push_back( anEntry );
    return *this;
}


/*
 * Return the aggregate throughput of this entity.
 */

double
Entity::throughput() const
{		
    return std::accumulate( entries().begin(), entries().end(), 0., add_using<Entry>( &Entry::throughput ) );
}



/*
 * Return the total open arrival rate to this server.
 */

double
Entity::openArrivalRate() const
{
    return std::accumulate( entries().begin(), entries().end(), 0., add_using<Entry>( &Entry::openArrivalRate ) );
}



/*
 * Return utilization for this entity.  Compute from the entry utilization.
 */

double
Entity::utilization() const
{		
#if !DEFERRED_UTILIZATION
    const_cast<Entity *>(this)->_utilization = std::accumulate( entries().begin(), entries().end(), 0., add_using<Entry>( &Entry::utilization ) );
    if ( Pragma::stopOnBogusUtilization() > 0. && !isInfinite() && _utilization / copies() > Pragma::stopOnBogusUtilization() ) {
	std::ostringstream err;
	err << name() << " utilization=" << _utilization << " exceeds multiplicity=" << copies();
	throw std::range_error( err.str() );
    }
#endif
    return _utilization;
}



/*
 * Return the "saturation" (normalized utilization)
 */

double
Entity::saturation() const
{
    if ( isInfinite() ) { 
	return 0.0;
    } else {
	double value = getDOM()->getCopiesValue(); 
	return utilization() / value;
    }
}


unsigned
Entity::hasServerChain( const unsigned k ) const
{
    return _serverChains.find(k);
}


/*
 * Return the total open arrival rate to this server.
 */

bool
Entity::hasOpenArrivals() const
{
    return find_if( entries().begin(), entries().end(), Predicate<Entry>( &Entry::hasOpenArrivals ) ) != entries().end();
}


/*
 * Find all tasks that call this task and add them to the argument.
 * Return the number of calling tasks ADDED!
 */

const Entity&
Entity::clients( std::set<Task *>& callingTasks ) const
{
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	const std::set<Call *>& callerList = (*entry)->callerList();
	for ( std::set<Call *>::const_iterator call = callerList.begin(); call != callerList.end(); ++call ) {
	    if ( !(*call)->hasForwarding() && (*call)->srcTask()->isUsed() ) {
		callingTasks.insert(const_cast<Task *>((*call)->srcTask()));
	    }
	}
    }

    return *this;
}

double 
Entity::nCustomers() const
{
    std::set<Task *> tasks;
    clients( tasks );
    return std::accumulate( tasks.begin(), tasks.end(), 0., add_using<Task>( &Task::population ) );
}

/*
 * Return true if phased server are used.
 */

bool
Entity::markovOvertaking() const
{
    return (bool)( hasSecondPhase()
		   && !isInfinite()
		   && Pragma::overtaking( Pragma::MARKOV_OVERTAKING ) );
}



/*
 * Return the scheduling type allowed for this object.  Overridden by
 * subclasses if the scheduling type can be something other than FIFO.
 */

unsigned
Entity::validScheduling() const
{
    return 1 << static_cast<unsigned>(defaultScheduling());
}



/*
 * Check the scheduling type.  Return the default type if the value
 * supplied is not kosher.  Overridden by subclasses if the scheduling
 * type can be something other than FIFO.
 */

bool
Entity::schedulingIsOk( const unsigned bits ) const
{
    return ((1 << static_cast<unsigned>(scheduling())) & bits ) != 0;
}



Entity&
Entity::computeUtilization()
{
#if DEFERRED_UTILIZATION
    _utilization = std::accumulate( entries().begin(), entries().end(), 0., add_using<Entry>( &Entry::utilization ) );
    if ( Pragma::stopOnBogusUtilization() > 0. && !isInfinite() && _utilization / copies() > Pragma::stopOnBogusUtilization() ) {
	std::ostringstream err;
	err << name() << " utilization=" << _utilization << " exceeds multiplicity=" << copies();
	throw std::range_error( err.str() );
    }
#endif
    return *this;
}

/*
 * Calculate and set variance for entire entity.
 */

Entity&
Entity::computeVariance() 
{
    for_each( entries().begin(), entries().end(), Exec<Entry>( &Entry::computeVariance ) );
    return *this;
}



/*
 * Return in probability of interlocking.
 */

Probability
Entity::prInterlock( const Task& aClient ) const
{
    const Probability pr = _interlock.interlockedFlow( aClient ) / population();
    if ( flags.trace_interlock ) {
	cout << "Interlock: " 
	     << aClient.name() << "(" << aClient.population() << ") -> " 
	     << name()         << "(" << population()         << ")  = " << pr << endl;
    }
    return pr;
}



/*
 * Update utilization for this entity and return
 */

double
Entity::deltaUtilization() const
{
    const double thisUtilization = utilization();
    double delta;
    if ( isinf( thisUtilization ) && isinf( _lastUtilization ) ) {
	delta = 0.0;
    } else {
	delta = thisUtilization - _lastUtilization;
    }
    _lastUtilization = thisUtilization;
    return delta;
}



/*
 * Calculate and set myThinkTime.  Note that population returns the
 * maximum number of customers possible at a station.  It is used,
 * rather than copies, because some multi-servers may have more
 * threads specified than can possibly be active.
 */


void
Entity::setIdleTime( const double relax ) 
{
    double z;

    if ( utilization() >= population() ) {
	z = 0.0;
    } else if ( throughput() > 0.0 ) {
	z = ( population() - utilization() ) / throughput();
    } else {
	z = get_infinity();	/* INFINITY */
    }
    if ( flags.trace_idle_time ) {
	cout << "\nEntity::setIdleTime():" << name() << "   Idle Time:  " << z << endl;
    }
    under_relax( myThinkTime, z, relax );
}



/*
 * Check results for sanity.
 * Note -- make sure all errors are advisories, or no output will be generated.
 */

const Entity&
Entity::sanityCheck() const
{
    if ( !isInfinite() && utilization() > copies() * 1.05 ) {
	LQIO::solution_error( ADV_INVALID_UTILIZATION, utilization(), 
			      getDOM()->getTypeName(),
			      name().c_str(), copies() );
    }
    return *this;
}


/*
 * Return true if any entry overflows in open model solution.
 */

bool
Entity::openModelInfinity() const
{
    bool rc = false;
    const Server * station = serverStation();
    for ( unsigned int e = 1; e <= nEntries(); ++e ) {
	if ( !isfinite( station->R(e,0) ) && station->V(e,0) != 0 && station->S(e,0) != 0 ) {
	    LQIO::solution_error( ERR_ARRIVAL_RATE, station->V(e,0), entryAt(e-1)->name().c_str(), station->mu()/station->S(e,0) );
	    rc = true;
	}
    }
    return rc;
}

Entity&
Entity::clear()
{
    serverStation()->clear();
    return *this;
}


/*
 * Initialize the service and visit parameters for a server.  Also set myChains to
 * all chains that visit this server.
 */

Entity&
Entity::initServer( Submodel& submodel )
{
    Server * station = serverStation();
    if ( !station ) return *this;

    if ( !Pragma::init_variance_only() ) {
	computeVariance();
    }

    const ChainVector& aChain = serverChains();
    const std::vector<Entry *>& server_entries = entries();
    for ( std::vector<Entry *>::const_iterator entry = server_entries.begin(); entry != server_entries.end(); ++entry ) {
	const unsigned e = (*entry)->index();
	const double openArrivalRate = (*entry)->openArrivalRate();

	if ( openArrivalRate > 0.0 ) {
	    station->setVisits( e, 0, 1, openArrivalRate );	// Chain 0 reserved for open class.
	}

	/* -- Set service time for entries with visits only. -- */

	for ( unsigned ix = 1; ix <= aChain.size(); ++ix ) {
	    setServiceTime( (*entry), aChain[ix] );
	}

	/*
	 * Open arrivals and other open models use chain zero which are special
	 * and won't work in the above loop anyway)
	 */

	if ( isInOpenModel() ) {
	    setServiceTime( (*entry), 0 );
	}

    }
    /* Overtaking -- compute for MARKOV overtaking only. */

    if ( markovOvertaking() ) {
	const std::set<Task *>& clients = submodel.getClients();
	for_each( clients.begin(), clients.end(), Exec1<Task,Entity*>( &Task::computeOvertaking, this ) );
    }

    /* Set interlock */

    if ( isInClosedModel() && Pragma::interlock() ) {
	setInterlock( submodel );
    }

    if ( hasSynchs() && !Pragma::threads(Pragma::NO_THREADS) ) {
	joinOverlapFactor( submodel );
    }
    
    return *this;
}


/*
 * Set the service time for my station.
 */

void
Entity::setServiceTime( const Entry * entry, unsigned k ) const
{
    Server * station = serverStation();
    const unsigned e = entry->index();

    if ( station->V( e, k ) == 0 ) return;

    for ( unsigned p = 1; p <= entry->maxPhase(); ++p ) {
	station->setService( e, k, p, entry->elapsedTimeForPhase(p) );

	if ( hasVariance() ) {
	    station->setVariance( e, k, p, entry->varianceForPhase(p) );
	}
    }
}

void
Entity::setInterlock( Submodel& submodel ) const
{
    Server * station = serverStation();
    const std::set<Task *>& clients = submodel.getClients();

    for ( std::set<Task *>::const_iterator client = clients.begin(); client != clients.end(); ++client ) {
	if ( (*client)->throughput() == 0.0 ) continue;

	const Probability PrIL = prInterlock( *(*client) );
	if ( PrIL == 0.0 ) continue;

	const ChainVector& chain = (*client)->clientChains( submodel.number() );

	for ( unsigned ix = 1; ix <= chain.size(); ++ix ) {
	    const unsigned k = chain[ix];
	    if ( hasServerChain(k) ) {
		for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
		    station->setInterlock( (*entry)->index(), k, PrIL );
		}
	    }
	}
    }
}


/*
 * Save server results.  Servers only occur in one submodel.
 */

Entity&
Entity::saveServerResults( const MVASubmodel& submodel, double relax )
{
    const Server * station = serverStation();

    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	const unsigned e = (*entry)->index();
	double lambda = 0.0;

	if ( isInOpenModel() && submodel.openModel ) {
	    lambda = submodel.openModel->entryThroughput( *station, e );		/* BUG_168 */
	}

	if ( isInClosedModel() && submodel.closedModel ) {
	    const double tput = submodel.closedModel->entryThroughput( *station, e );
	    if ( isfinite( tput ) ) {
		lambda += tput;
	    } else if ( tput < 0.0 ) {
		throw domain_error( "MVASubmodel::saveServerResults" );
	    } else {
		lambda = tput;
		break;
	    }
	}
	(*entry)->saveThroughput( lambda )
	    .saveOpenWait( station->R( e, 0 ) );
    }

    computeUtilization();
    setIdleTime( relax );

    return *this;
}

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

/*
 * Print chains for this client.
 */

/* static */ ostream&
Entity::output_server_chains( ostream& output, const Entity& aServer ) 
{
    output << "Chains:" << aServer.serverChains() << endl;
    return output;
}

/* static */  ostream&
Entity::output_entity_info( ostream& output, const Entity& aServer )
{
    if ( aServer.serverStation() ) {
	output << "(" << aServer.serverStation()->typeStr() << ")";
    }
    return output;
}
