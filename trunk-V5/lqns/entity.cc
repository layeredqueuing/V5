/* -*- c++ -*-
 * $Id$
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
#include <cmath>
#include <lqio/error.h>
#include <lqio/labels.h>
#include <lqio/dom_extvar.h>
#include "vector.h"
#include "cltn.h"
#include "fpgoop.h"
#include "stack.h"
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

Entity::Entity( LQIO::DOM::Entity* domVersion, Cltn<Entry *>* entries )
    : domEntity(domVersion),
      interlock(0),
      myPopulation(1.0),
      myVariance(0.0),
      myThinkTime(0.0),
      myServerStation(0),		/* Reference tasks don't have server stations. */
      myMaxPhase(1),
      traceFlag(0),
      mySubmodel(0),
      myLastUtilization(-1.0)		/* Force update 		*/
{
    if ( entries != 0 ) {
	entryList = *entries;
    }

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
    /* Release interlocking paths */

    if ( interlock ) {
	delete interlock;
    }
	
    delete myServerStation;
}



/*
 * model max phase for this entity.
 */

void
Entity::configure( const unsigned nSubmodels )
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    if ( copies() <= 0 ) {
	LQIO::solution_error( LQIO::ERR_POSITIVE_INTEGER_EXPECTED, "multiplicity", copies(), "task", name() );
    }
    if ( replicas() <= 0 ) {
	LQIO::solution_error( LQIO::ERR_POSITIVE_INTEGER_EXPECTED, "replicas", replicas(), "task", name() );
    }

    if ( nEntries() > 1 && pragma.entry_variance() ) {
	attributes.variance = true;
    }
    for ( unsigned i = 1; anEntry = nextEntry(); ++i ) {
	anEntry->index = i;
	anEntry->configure( nSubmodels );
	myMaxPhase = max( myMaxPhase, anEntry->maxPhase() );

	if ( anEntry->hasDeterministicPhases() ) {
	    attributes.deterministic = true;
	}
	if ( anEntry->hasVariance() ) {
	    attributes.variance = true;
	}
    }
}



/*
 * Recursively find all children and grand children from `father'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
Entity::findChildren( CallStack& callStack, const bool ) const
{
    unsigned max_depth = max( submodel(), callStack.size() );

    const_cast<Entity *>(this)->setSubmodel( max_depth );
    return max_depth;
}



/*
 * Initialize waiting time at my entries.
 */

Entity&
Entity::initWait()
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while( anEntry = nextEntry() ) {
	anEntry->initWait();
    }

    return *this;
}



/*
 * The default method is to ensure that there is only one copy specified.
 */

Entity&
Entity::copies( const unsigned n )
{
    if ( n != 1 ) {
	throw should_not_implement( "Entity::copies", __FILE__, __LINE__ );
    }
    return *this;
}


/*
 * We need a way to fake out infinity... so if copies is infinite, then we change to an infinite server.
 */

unsigned
Entity::copies() const
{
    const LQIO::DOM::ExternalVariable * dom_copies = domEntity->getCopies(); 
    double value;
    assert(dom_copies->getValue(value) == true);
    if ( isinf( value ) ) return 1;
    if ( value - floor(value) != 0 ) {
	throw domain_error( "Entity::copies" );
    }
    return static_cast<unsigned int>(value);
}


/*
 * The default method is to ensure that there is only one copy specified.
 */

Entity&
Entity::replicas( const unsigned n )
{
    if ( n != 1 ) {
	throw domain_error( "Entity::replicas" );
    }
    return *this;
}


bool 
Entity::isInfinite() const
{
    const LQIO::DOM::ExternalVariable * dom_copies = domEntity->getCopies(); 
    double value;
    assert(dom_copies->getValue(value) == true);
    if ( isinf( value ) ) {
	return true;
    } else { 
	return scheduling() == SCHEDULE_DELAY; 
    }
}



/*
 * Add an entry to the list of entries for this task and set local index
 * for MVA.
 */

Entity&
Entity::addEntry( Entry * anEntry )
{
    entryList << anEntry;
    anEntry->index = entryList.size();
	
    return *this;
}



/*
 * remove an entry from the list of entries for this task and set local index
 * for MVA.
 */

Entity&
Entity::removeEntry( Entry * anEntry )
{
    entryList -= anEntry;
    return *this;
}



/*
 * Return the fan-in to this server from...
 */

unsigned
Entity::fanIn( const Task * aClient ) const
{
    if ( dynamic_cast<LQIO::DOM::Task *>(domEntity) ) {
	return dynamic_cast<LQIO::DOM::Task *>(domEntity)->getFanIn( aClient->name() );
    } else {
	return aClient->replicas() / replicas();
    }
}

#warning fix me
unsigned
Entity::fanOut( const Entity * aServer ) const
{
    if ( dynamic_cast<LQIO::DOM::Task *>(domEntity) ) {
	return dynamic_cast<LQIO::DOM::Task *>(domEntity)->getFanOut( aServer->name() );
    } else {
	return replicas() / aServer->replicas();
    }
}

/*
 * Return the aggregate throughput of this entity.
 */

double
Entity::throughput() const
{		
    double sum = 0.0;

    Sequence<Entry *> nextEntry( entries() );
    const Entry * anEntry;
	
    while ( anEntry = nextEntry() ) {
	sum += anEntry->throughput();
    }
    return sum;
}



/*
 * Return the total open arrival rate to this server.
 */

double
Entity::openArrivalRate() const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    double sum = 0.0;
    while ( anEntry = nextEntry() ) {
	sum += anEntry->openArrivalRate();
    }

    return sum;
}



/*
 * Return utilization for this entity.  Compute from the entry utilization.
 */

double
Entity::utilization() const
{		
    double sum = 0.0;

    Sequence<Entry *> nextEntry( entries() );
    const Entry * anEntry;
	
    while ( anEntry = nextEntry() ) {
	sum += anEntry->utilization();
    }
    return sum;
}



/*
 * Find all tasks that call this task and add them to the argument.
 * Return the number of calling tasks ADDED!
 */

unsigned 
Entity::clients( Cltn<Task *> & callingTasks ) const
{
    unsigned startSize = callingTasks.size();

    Sequence<Entry *> nextEntry( entries() );
    const Entry * anEntry;
	
    while ( anEntry = nextEntry() ) {

	Sequence<Call *> nextCall( anEntry->callerList() );
	const Call * aCall;
	while ( aCall = nextCall() ) {
	    if ( !aCall->hasForwarding() && aCall->srcTask()->isUsed() ) {
		callingTasks += const_cast<Task *>(aCall->srcTask());
	    }
	}
    }

    return callingTasks.size() - startSize;
}



/*
 * Return true if phased server are used.
 */

bool
Entity::markovOvertaking() const
{
    return (bool)( hasSecondPhase()
		   && !isInfinite()
		   && pragma.getOvertaking() == MARKOV_OVERTAKING);
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
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;
    
    while ( anEntry = nextEntry() ) {
	anEntry->computeVariance();
    }

    return *this;
}



/*
 * Set overtaking for this server for clients.
 */

void 
Entity::setOvertaking( const unsigned submodel, const Cltn<Task *>& clients )
{
    Sequence<Task *> nextClient( clients );
    Task * aClient;
    while ( aClient = nextClient() ) {
	Overtaking overtaking( aClient, this );
	overtaking.compute();
    }
}


/*
 * Update utilization for this entity and return
 */

double
Entity::deltaUtilization() const
{
    double delta = utilization() - myLastUtilization;
    myLastUtilization = utilization();
    return delta;
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

void
Entity::sanityCheck() const
{
    if ( !isInfinite() && utilization() > copies() * 1.05 ) {
	LQIO::solution_error( ADV_INVALID_UTILIZATION, utilization(), 
			      isProcessor() ? "processor" : "task", 
			      name(), copies() );
    }
}


/*
 * Print chains for this client.
 */

ostream&
Entity::printServerChains( ostream& output ) const
{
    output << "Chains:" << myServerChains << endl;
    return output;
}


ostream&
Entity::printOvertaking( ostream& output ) const
{
    for ( set<Task *,ltTask>::const_iterator nextClient = task.begin(); nextClient != task.end(); ++nextClient ) {
	const Task * aClient = *nextClient;
	Overtaking overtaking( aClient, this );
	output << overtaking;
    }
    return output;
}

static ostream&
entity_info_str( ostream& output, const Entity& aServer )
{
    if ( aServer.serverStation() ) {
	string aString = "(";
	aString += aServer.serverStation()->typeStr();
	aString += ")";
	output << aString;
    }
    return output;
}


static ostream&
server_chains_of_str( ostream& output, const Entity& aServer )
{
    aServer.printServerChains( output );
    return output;
}


SRVNEntityManip
print_server_chains( const Entity& anEntity )
{
    return SRVNEntityManip( server_chains_of_str, anEntity);
}


SRVNEntityManip
print_info( const Entity& anEntity )
{
    return SRVNEntityManip( entity_info_str, anEntity );
}
