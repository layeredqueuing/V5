/* -*- c++ -*-
 * $Id: entity.cc 13725 2020-08-04 03:58:02Z greg $
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
#include "overtake.h"
#include "submodel.h"

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
    : interlock(nullptr),
      _dom(dom),
      _entries(entries),
      _tasks(),
      myPopulation(1.0),
      myVariance(0.0),
      myThinkTime(0.0),
      myServerStation(0),		/* Reference tasks don't have server stations. */
      _submodel(0),
      _maxPhase(1),
      _lastUtilization(-1.0)		/* Force update 		*/
{
    attributes.initialized      = 0;		/* entity was initialized.	*/
    attributes.closed_model	= 0;		/* Stn in in closed model.     	*/
    attributes.open_model	= 0;		/* Stn is in open model.	*/
    attributes.deterministic    = 0;		/* an entry has det. phase.	*/
    attributes.pure_delay       = 0;		/* Special task of some form.	*/
    attributes.pure_server	= 1;		/* Can use FCFS schedulging.	*/
    attributes.variance         = false;	/* */
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
	attributes.variance = true;
    }
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	(*entry)->configure( nSubmodels );
	_maxPhase = std::max( _maxPhase, (*entry)->maxPhase() );

	if ( (*entry)->hasDeterministicPhases() ) {
	    attributes.deterministic = true;
	}
	if ( !Pragma::variance(Pragma::NO_VARIANCE) && (*entry)->hasVariance() ) {
	    attributes.variance = true;
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
    return for_each( entries().begin(), entries().end(), Sum<Entry,double>( &Entry::throughput ) ).sum();
}



/*
 * Return the total open arrival rate to this server.
 */

double
Entity::openArrivalRate() const
{
    return for_each( entries().begin(), entries().end(), Sum<Entry,double>( &Entry::openArrivalRate ) ).sum();
}



/*
 * Return utilization for this entity.  Compute from the entry utilization.
 */

double
Entity::utilization() const
{		
    return for_each( entries().begin(), entries().end(), Sum<Entry,double>( &Entry::utilization ) ).sum();
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
    return for_each( tasks.begin(), tasks.end(), Sum<Task,double>( &Task::population ) ).sum();
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
 * Set overtaking for this server for clients.
 */

void 
Entity::setOvertaking( const unsigned submodel, const std::set<Task *>& clients )
{
    for ( std::set<Task *>::const_iterator client = clients.begin(); client != clients.end(); ++client ) {
	Overtaking overtaking( *client, this );
	overtaking.compute();
    }
}


/*
 * Return in probability of interlocking.
 */

Probability
Entity::prInterlock( const Task& aClient ) const
{
    const Probability pr = interlock->interlockedFlow( aClient ) / population();
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
Entity::setIdleTime( const double relax,  Submodel * aSubmodel ) 
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
			      isProcessor() ? "processor" : "task", 
			      name().c_str(), copies() );
    }
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
