/* -*- c++ -*-
 * $Id: entity.cc 15628 2022-06-02 17:40:08Z greg $
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


#include "lqns.h"
#include <cmath>
#include <numeric>
#include <sstream>
#include <limits>
#include <lqio/error.h>
#include <lqio/labels.h>
#include <lqio/dom_extvar.h>
#include <mva/open.h>
#include <mva/mva.h>
#include <mva/ph2serv.h>
#include <mva/server.h>
#include <mva/vector.h>
#include "call.h"
#include "entity.h"
#include "entry.h"
#include "errmsg.h"
#include "flags.h"
#include "pragma.h"
#include "processor.h"
#include "submodel.h"
#include "task.h"
#include "variance.h"

#define DEFERRED_UTULIZATION	false

/* ---------------------- Overloaded Operators ------------------------ */

/*
 * Printing function.
 */

std::ostream&
operator<<( std::ostream& output, const Entity& self )
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
      _population(1.0),
      _variance(0.0),
      _thinkTime(0.0),
      _station(nullptr),		/* Reference tasks don't have server stations. */
      _interlock( *this ),
      _submodel(0),
      _maxPhase(1),
      _utilization(0),
      _lastUtilization(-1.0),		/* Force update 		*/
      _replica_number(1),		/* This object is not a replica	*/
      _pruned(false)
{
    _attributes[Attributes::initialized]	= false;	/* entity was initialized.		*/
    _attributes[Attributes::closed_server]   	= false;	/* Stn in server in closed model.     	*/
    _attributes[Attributes::closed_client]   	= false;	/* Stn in client in closed model.     	*/
    _attributes[Attributes::open_server]	= false;	/* Stn is server in open model.		*/
    _attributes[Attributes::deterministic]  	= false;	/* an entry has det. phase[		*/
    _attributes[Attributes::variance]		= false;	/* */
}


/*
 * Clone an entity (Not PAN_REPLICATION).
 */

Entity::Entity( const Entity& src, unsigned int replica )
    : _dom(src._dom),
      _entries(),
      _tasks(),
      _population(src._population),
      _variance(src._variance),
      _thinkTime(src._thinkTime),
      _station(nullptr),		/* Reference tasks don't have server stations. */
      _attributes(src._attributes),
      _interlock( *this ),
      _submodel(0),
      _maxPhase(1),
      _utilization(0),
      _lastUtilization(-1.0),		/* Force update 		*/
      _replica_number(replica),		/* This object is a replica	*/
      _pruned(false)
{
}


/*
 * Delete all entries associated with this task.
 */

Entity::~Entity()
{
    delete _station;
}




/*
 * Set the max phase for this entity.
 */

Entity&
Entity::configure( const unsigned nSubmodels )
{
    std::for_each( entries().begin(), entries().end(), Exec1<Entry,unsigned>( &Entry::configure, nSubmodels ) );
    if ( std::any_of( entries().begin(), entries().end(), Predicate<Entry>( &Entry::hasDeterministicPhases ) ) ) _attributes.at(Attributes::deterministic) = true;
    if ( !Pragma::variance(Pragma::Variance::NONE)
	 && ((nEntries() > 1 && Pragma::entry_variance())
	     || std::any_of( entries().begin(), entries().end(), Predicate<Entry>( &Entry::hasVariance ) )) ) _attributes.at(Attributes::variance) = true;
    _maxPhase = (*std::max_element( entries().begin(), entries().end(), Entry::max_phase ))->maxPhase();
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
    unsigned max_depth = std::max( submodel(), callStack.depth() );

#if 0
    std::cerr << "Entity::findChildren: " << print_name() << "->setSubmodel(" << max_depth << ")" << std::endl;
#endif
    const_cast<Entity *>(this)->setSubmodel( max_depth );
    return max_depth;
}



/*
 * Initialize waiting time at my entries.
 */

Entity&
Entity::initWait()
{
    std::for_each( entries().begin(), entries().end(), Exec<Entry>( &Entry::initWait ) );
    return *this;
}


Entity&
Entity::initInterlock()
{
    _interlock.initialize();
    return *this;
}



/*
 * Initialize server's waiting times and populations (called after recalculate dynamic variables)
 */

Entity&
Entity::initServer( const Vector<Submodel *>& submodels )
{
    initWait();
    updateAllWaits( submodels );
    computeVariance();
    initThroughputBound();
    initPopulation();
    initThreads();
    initialized(true);
    return *this;
}


/*
 * Initialize server's waiting times and populations (called after recalculate dynamic variables)
 */

Entity&
Entity::reinitServer( const Vector<Submodel *>& submodels )
{
    updateAllWaits( submodels );
    computeVariance();
    initThroughputBound();
    initPopulation();
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
    return std::find( tasks().begin(), tasks().end(), task ) != tasks().end();
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



bool
Entity::hasServerChain( const unsigned k ) const
{
    return _serverChains.find(k) != _serverChains.end();
}


/*
 * Return the total open arrival rate to this server.
 */

bool
Entity::hasOpenArrivals() const
{
    return std::any_of( entries().begin(), entries().end(), Predicate<Entry>( &Entry::hasOpenArrivals ) );
}


/*
 * Find all tasks that call this task and add them to the argument.
 */

std::set<Task *>&
Entity::getClients( std::set<Task *>& clients ) const
{
    std::for_each( entries().begin(), entries().end(), Entry::get_clients( clients ) );
    return clients;
}


double
Entity::nCustomers() const
{
    std::set<Task *> clients;
    getClients( clients );
    return std::accumulate( clients.begin(), clients.end(), 0., add_using<Task>( &Task::population ) );
}

/*
 * Return true if phased server are used.
 */

bool
Entity::markovOvertaking() const
{
    return (bool)( hasSecondPhase()
		   && !isInfinite()
		   && Pragma::overtaking( Pragma::Overtaking::MARKOV ) );
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



/*
 * Upated the waiting time over all submodels.
 */

Entity&
Entity::updateAllWaits( const Vector<Submodel *>& submodels )
{
    std::for_each( submodels.begin(), submodels.end(), update_wait( *this ) );
    return *this;
}



double
Entity::computeUtilization( const MVASubmodel& submodel )
{
    return std::accumulate( entries().begin(), entries().end(), 0., add_using<Entry>( &Entry::utilization ) );
}


/*
 * Calculate and set variance for entire entity.
 */

Entity&
Entity::computeVariance()
{
    std::for_each( entries().begin(), entries().end(), Exec<Entry>( &Entry::computeVariance ) );
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
	std::cout << "Interlock: "
	     << aClient.name() << "(" << aClient.population() << ") -> "
	     << name()         << "(" << population()         << ")  = " << pr << std::endl;
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
    if ( std::isinf( thisUtilization ) && std::isinf( _lastUtilization ) ) {
	delta = 0.0;
    } else {
	delta = thisUtilization - _lastUtilization;
    }
    _lastUtilization = thisUtilization;
    return delta;
}



/*
 * Calculate and set think time.  Note that population returns the
 * maximum number of customers possible at a station.  It is used,
 * rather than copies, because some multi-servers may have more
 * threads specified than can possibly be active.
 */

void
Entity::setIdleTime( const double relax )
{
    if ( !std::isfinite( population() ) ) {
	_thinkTime = 0.0;
    } else if ( utilization() >= population() ) {
	_thinkTime = 0.0;
    } else if ( throughput() > 0.0 ) {
	_thinkTime = under_relax( _thinkTime, (population() - utilization()) / throughput(), relax );
    } else {
	_thinkTime = std::numeric_limits<double>::infinity();
    }
    if ( flags.trace_idle_time ) {
	std::cout << "\nEntity::setIdleTime():" << name() << "   Idle Time:  " << _thinkTime << std::endl;
    }
}



/*
 * Check results for sanity.
 * Note -- make sure all errors are advisories, or no output will be generated.
 */

const Entity&
Entity::sanityCheck() const
{
    if ( !std::isfinite( utilization() ) ) {
	LQIO::solution_error( ADV_INFINITE_UTILIZATION, getDOM()->getTypeName(), name().c_str() );
    } else if ( !isInfinite() && utilization() > copies() * 1.05 ) {
	LQIO::solution_error( ADV_INVALID_UTILIZATION, getDOM()->getTypeName(), name().c_str(), copies(), utilization() );
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
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	const unsigned e = (*entry)->index();
	if ( !std::isfinite( station->R(e,0) ) && station->V(e,0) != 0 && station->S(e,0) != 0 ) {
	    LQIO::solution_error( ERR_ARRIVAL_RATE, station->V(e,0), (*entry)->name().c_str(), station->mu()/station->S(e,0) );
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
Entity::initServerStation( Submodel& submodel )
{
    Server * station = serverStation();
    if ( !station ) return *this;

    if ( !Pragma::init_variance_only() ) {
	computeVariance();
    }

    /* If this entity has been pruned, remap to the base replica */
#if BUG_299_PRUNE
    const Entity * entity = nullptr;
    if ( !isPruned() ) {
	entity = this;
    } else if ( isProcessor() ) {
	entity = Processor::find( name() );
    } else {
	entity = Task::find( name() );
    }
    const std::vector<Entry *>& entries = entity->entries();
#else
    const std::vector<Entry *>& entries = this->entries();
#endif

    const ChainVector& chains = serverChains();
    for ( std::vector<Entry *>::const_iterator entry = entries.begin(); entry != entries.end(); ++entry ) {
	const unsigned e = (*entry)->index();
	const double openArrivalRate = (*entry)->openArrivalRate();

	if ( openArrivalRate > 0.0 ) {
	    station->setVisits( e, 0, 1, openArrivalRate );	// Chain 0 reserved for open class.
	}

	/* -- Set service time for entries with visits only. -- */
	if ( isClosedModelServer() ) {
	    for ( ChainVector::const_iterator k = chains.begin(); k != chains.end(); ++k ) {
		setServiceTime( (*entry), *k );
	    }
	}

	/*
	 * Open arrivals and other open models use chain zero which are special
	 * and won't work in the above loop anyway)
	 */

	if ( isOpenModelServer() ) {
	    setServiceTime( (*entry), 0 );
	}

    }

    /* Overtaking -- compute for MARKOV overtaking only. */

    if ( markovOvertaking() ) {
	const std::set<Task *>& clients = submodel.getClients();
	std::for_each( clients.begin(), clients.end(), Exec1<Task,Entity*>( &Task::computeOvertaking, this ) );
    }

    /* Set interlock */

    if ( isClosedModelServer() && Pragma::interlock() ) {
	setInterlock( submodel );
    }

    if ( hasSynchs() && !Pragma::threads(Pragma::Threads::NONE) ) {
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

	const ChainVector& chains = (*client)->clientChains( submodel.number() );
	for ( ChainVector::const_iterator k = chains.begin(); k != chains.end(); ++k ) {
	    if ( hasServerChain(*k) ) {
		for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
		    station->setInterlock( (*entry)->index(), *k, PrIL );
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

	if ( isOpenModelServer() ) {
	    lambda = submodel.openModelThroughput( *station, e );		/* BUG_168 */
	    (*entry)->saveOpenWait( station->R( e, 0 ) );
	}

	if ( isClosedModelServer() ) {
	    const double tput = submodel.closedModelThroughput( *station, e );
	    if ( std::isfinite( tput ) ) {
		lambda += tput;
	    } else if ( tput < 0.0 ) {
		throw std::domain_error( "MVASubmodel::saveServerResults" );
	    } else {
		lambda = tput;
		break;
	    }
	}
	(*entry)->saveThroughput( lambda );
    }

    setUtilization( computeUtilization( submodel ) );
    setIdleTime( relax );

    return *this;
}


Entity&
Entity::setUtilization( double utilization )
{
    _utilization = utilization;
    if ( Pragma::stopOnBogusUtilization() > 0. && !isInfinite() && _utilization / copies() > Pragma::stopOnBogusUtilization() ) {
	std::ostringstream err;
	err << name() << " utilization=" << _utilization << " exceeds multiplicity=" << copies();
	throw std::range_error( err.str() );
    }
    return *this;
}

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

/* static */ std::string
Entity::fold( const std::string& s1, const Entity* e2 )
{
    std::string s = s1;
    if ( !s.empty() ) s += " ";
    s += e2->name();
    return s;
}

/*
 * Print chains for this client.
 */

/* static */ std::ostream&
Entity::output_server_chains( std::ostream& output, const Entity& entity )
{
    output << "Chains:" << entity.serverChains() << std::endl;
    return output;
}


/* static */ std::ostream&
Entity::output_entries( std::ostream& output, const Entity& entity )
{
    const std::vector<Entry *>& entries = entity.entries();
    const Entry& e1 = *entries.front();
    std::ostringstream name;
    name << e1.print_name();
    output << std::accumulate( std::next(entries.begin()), entries.end(), name.str(), &Entry::fold );
    return output;
}



/* static */ std::ostream&
Entity::output_name( std::ostream& output, const Entity& entity )
{
    output << entity.name();
    if ( entity.isReplicated() ) {
	output << "." << entity.getReplicaNumber();
    }
    return output;
}

/* static */ std::ostream&
Entity::output_info( std::ostream& output, const Entity& entity )
{
    if ( entity.serverStation() ) {
	output << entity.serverStation()->typeStr();
    } else {
	output << "--";
    }
    return output;
}


/* static */ std::ostream&
Entity::output_type( std::ostream& output, const Entity& entity )
{
    char buf[12];
    const unsigned n = entity.copies();

    if ( entity.scheduling() == SCHEDULE_CUSTOMER ) {
	sprintf( buf, "ref(%d)", n );
    } else if ( entity.isInfinite() ) {
	sprintf( buf, "inf" );
    } else if ( n > 1 ) {
	sprintf( buf, "mult(%d)", n );
    } else {
	sprintf( buf, "serv" );
    }
    output << buf;
    return output;
}
