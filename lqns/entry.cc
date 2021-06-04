/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/entry.cc $
 *
 * Everything you wanted to know about an entry, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November 1994,
 * January 2005.
 * July 2007.
 *
 * ------------------------------------------------------------------------
 * $Id: entry.cc 14765 2021-06-04 01:04:46Z greg $
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <stdarg.h>
#include <string.h>
#include <lqio/error.h>
#include <mva/fpgoop.h>
#include <mva/prob.h>
#include "actlist.h"
#include "call.h"
#include "entry.h"
#include "entrythread.h"
#include "errmsg.h"
#include "lqns.h"
#include "model.h"
#include "pragma.h"
#include "processor.h"
#include "randomvar.h"
#include "submodel.h"
#include "task.h"
#include "variance.h"

unsigned Entry::totalOpenArrivals   = 0;
unsigned Entry::max_phases	    = 0;

/* ------------------------ Constructors etc. ------------------------- */


Entry::Entry( LQIO::DOM::Entry* dom, unsigned int index, bool global )
    : _dom(dom),
      _phase(dom && dom->getMaximumPhase() > 0 ? dom->getMaximumPhase() : 1 ),
      _total( "total" ),
      _nextOpenWait(0.0),		/* copy for delta computation	*/
      _startActivity(nullptr),
      _entryId(global ? Model::__entry.size()+1 : 0),
      _index(index+1),
      _entryType(LQIO::DOM::Entry::Type::NOT_DEFINED),
      _semaphoreType(dom ? dom->getSemaphoreFlag() : LQIO::DOM::Entry::Semaphore::NONE),
      _calledBy(RequestType::NOT_CALLED),
      _throughput(0.0),
      _throughputBound(0.0),
      _callerList(),
      _interlock(),
      _replica_number(1)		/* This object is not a replica	*/
{
    const size_t size = _phase.size();
    for ( size_t p = 1; p <= size; ++p ) {
	_phase[p].setEntry( this )
	    .setName( name() + "_" + "123"[p-1] )
	    .setPhaseNumber( p );
    }
}


/*
 * Deep copy (except DOM).
 */

Entry::Entry( const Entry& src, unsigned int replica )
    : _dom(src._dom),
      _phase(src._phase.size()),
      _total(src._total._name),
      _nextOpenWait(0.0),
      _startActivity(nullptr),
      _entryId(src._entryId != 0 ? Model::__entry.size()+1 : 0),
      _index(src._index),
      _entryType(src._entryType),
      _semaphoreType(src._semaphoreType),
      _calledBy(src._calledBy),
      _throughput(0.0),
      _throughputBound(0.0),
      _callerList(),
      _interlock(),
      _replica_number(replica)		/* This object is a replica	*/
{
    /* Copy over phases.  Call lists will be done later */
    const size_t size = _phase.size();
    for ( size_t p = 1; p <= size; ++p ) {
	_phase[p].setEntry( this )
	    .setName( src._phase[p].name() )
	    .setPhaseNumber( p )
	    .setDOM( src._phase[p].getDOM() );
    }
}


/*
 * Free storage allocated in wait.  Storage was allocated by layerize.c
 * by calling configure.
 */

Entry::~Entry()
{
}



/*
 * Reset globals.
 */

void
Entry::reset()
{
    totalOpenArrivals	    = 0;
    max_phases		    = 0;
}



/*
 * Compare entry names for equality.
 */

int
Entry::operator==( const Entry& entry ) const
{
    return entryId() == entry.entryId();
}

/* ------------------------ Instance Methods -------------------------- */

double Entry::openArrivalRate() const
{
    if ( hasOpenArrivals() ) {
	try {
	    return getDOM()->getOpenArrivalRateValue();
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "open arrival rate", "entry", name().c_str(), e.what() );
	    throw_bad_parameter();
	}
    }
    return 0.;
}


int Entry::priority() const
{
    if ( getDOM()->hasEntryPriority() ) {
	try {
	    return getDOM()->getEntryPriorityValue();
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "priority", "entry", name().c_str(), e.what() );
	    throw_bad_parameter();
	}
    }
    return 0;
}


/*
 * Check entry data.
 */

bool
Entry::check() const
{
    const double precision = 100000.0;		/* round to nearest 1/precision */
    if ( isStandardEntry() ) {
	std::for_each( _phase.begin(), _phase.end(), Predicate<Phase>( &Phase::check ) );
    } else if ( isActivityEntry() ) {
	if ( !isVirtualEntry() && isCalledUsing( RequestType::RENDEZVOUS ) ) {
	    std::deque<const Activity *> activityStack;
	    Activity::Count_If data( this, &Activity::checkReplies );
	    const double replies = std::floor( getStartActivity()->count_if( activityStack, data ).sum() * precision ) / precision;
	    //tomari: disable to allow a quorum use the default reply which is after all threads completes exection.
	    //(replies == 1 || (replies == 0 && owner->hasQuorum()))
	    //Only tasks have activity entries. 
	    if ( replies != 1.0 && (replies != 0.0 || !dynamic_cast<const Task *>(owner())->hasQuorum()) ) {
		LQIO::solution_error( LQIO::ERR_NON_UNITY_REPLIES, replies, name().c_str() );
	    }
	    assert( activityStack.size() == 0 );
	}
    } else {
	LQIO::solution_error( LQIO::ERR_ENTRY_NOT_SPECIFIED, name().c_str() );
    }

    if ( (isSignalEntry() || isWaitEntry()) && owner()->scheduling() != SCHEDULE_SEMAPHORE ) {
	LQIO::solution_error( LQIO::ERR_NOT_SEMAPHORE_TASK, owner()->name().c_str(), (isSignalEntry() ? "signal" : "wait"), name().c_str() );
    }
    if ( maxPhase() > 1 && owner()->isInfinite() ) {
	LQIO::solution_error( WRN_MULTI_PHASE_INFINITE_SERVER, name().c_str(), owner()->name().c_str(), maxPhase() );
    }

    /* Forwarding probabilities o.k.? */

    double sum = std::accumulate( callList(1).begin(), callList(1).end(), 0.0, Call::add_forwarding );
    if ( sum < 0.0 || 1.0 < sum ) {
	LQIO::solution_error( LQIO::ERR_INVALID_FORWARDING_PROBABILITY, name().c_str(), sum );
    } else if ( sum != 0.0 && owner()->isReferenceTask() ) {
	LQIO::solution_error( LQIO::ERR_REF_TASK_FORWARDING, owner()->name().c_str(), name().c_str() );
    }
    return !LQIO::io_vars.anError();
}



/*
 * Allocate storage for savedWait.  NOTE:  the output file parsing
 * routines cannot deal with variable sized arrays, so fix these
 * at MAX_PHASES.  Configure for activities is done in Task::configure.
 */

Entry&
Entry::configure( const unsigned nSubmodels )
{
    std::for_each ( _phase.begin(), _phase.end(), Exec1<NullPhase,const unsigned>( &NullPhase::configure, nSubmodels ) );
    _total.configure( nSubmodels );

    const unsigned n_e = Model::__entry.size() + 1;
    if ( n_e != _interlock.size() ) {
	_interlock.resize( n_e );
    }

    initServiceTime();
    return *this;
}



/*
 * Expand replicas (Not PAN_REPLICATION)
 */

Entry&
Entry::expand()
{
    const unsigned replicas = owner()->replicas();
    for ( unsigned int replica = 2; replica <= replicas; ++replica ) {
	Model::__entry.insert( clone( replica ) );
    }
    return *this;
}



/*
 * Expand the replicas of the calls (Not PAN_REPLICATION).  This has
 * to be done separately because all of the replica entries have to
 * exist first (just like Model::prepare)
 */

Entry&
Entry::expandCalls()
{
    std::for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::expandCalls ) );
    return *this;
}



/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.  DirectPath is true if we got here by following the
 * call chain directly.
 */

unsigned
Entry::findChildren( Call::stack& callStack, const bool directPath ) const
{
    unsigned max_depth = callStack.depth();

    if ( isActivityEntry() ) {
	max_depth = std::max( max_depth, _phase[1].findChildren( callStack, directPath ) );    /* Always check because we may have forwarding */
	try {
	    Activity::Children path( callStack, directPath, true );
	    max_depth = std::max( max_depth, getStartActivity()->findChildren( path ) );
	}
	catch ( const activity_cycle& error ) {
	    LQIO::solution_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, owner()->name().c_str(), error.what() );
	}
	catch ( const bad_external_join& error ) {
	    LQIO::solution_error( ERR_EXTERNAL_SYNC, name().c_str(), owner()->name().c_str(), error.what() );
	}
    } else {
	max_depth = std::accumulate( _phase.begin(), _phase.end(), 0, max_two_args<Phase,Call::stack&,bool>( &Phase::findChildren, callStack, directPath ) );
    }

    return max_depth;
}



/*
 * Type 1 throughput bounds.  Reference task think times will limit throughput
 */

Entry&
Entry::initThroughputBound()
{
    const double t = elapsedTime() + owner()->thinkTime();
    if ( t > 0 ) {
	_throughputBound = owner()->copies() / t;
    } else {
	_throughputBound = 0.0;
    }
    setThroughput( _throughputBound );		/* Push bound to entries/phases/activities */
    return *this;
}



/* 
 * Compute overall service time for this entry 
 */

Entry&
Entry::initServiceTime()
{
    if ( isActivityEntry() && !isVirtualEntry() ) {

	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack;
	entryStack.push_back( this );
	Activity::Collect collect( 0, &Activity::collectServiceTime );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();
    }

    _total.setServiceTime( std::accumulate( _phase.begin(), _phase.end(), 0., add_using<Phase>( &Phase::serviceTime ) ) );
    return *this;
}
    


#if PAN_REPLICATION
/*
 * Allocate storage for oldSurgDelay.
 */

Entry&
Entry::initReplication( const unsigned n_chains )
{
    std::for_each ( _phase.begin(), _phase.end(), Exec1<Phase,const unsigned>( &Phase::initReplication, n_chains ) );
    return *this;
}
#endif



Entry&
Entry::resetInterlock()
{
    std::for_each ( _interlock.begin(), _interlock.end(), Exec<InterlockInfo>( &InterlockInfo::reset ) );
    return *this;
}



/*
 * Follow and record a path of phase one calls.  If we are at the head
 * of a path, then all calls from the entry `current' regardless of
 * phase are used.  Otherwise, only phase 1 calls are used.
 */

Entry&
Entry::createInterlock()		/* Called from task -- initialized calls */
{
    Interlock::CollectTable calls;
    initInterlock( calls );
    return *this;
}

Entry&
Entry::initInterlock( Interlock::CollectTable& path ) 
{
    /*
     * Check for cycles in graph.  Return if found.  Cycle catching
     * is done by the other version.  Someday, we might make this more
     * intelligent to compute the final prob. of following the arc.
     */
    if ( path.has_entry( this ) ) return *this;

    path.push_back( this );

    /* Update interlock table */

    const Entry * rootEntry = path.front();

    if ( rootEntry == this ) {
	_interlock[rootEntry->entryId()] = path.calls();
    } else {
	const_cast<Entry *>(rootEntry)->_interlock[entryId()] += path.calls();
	_interlock[rootEntry->entryId()] -= path.calls();
    }

    followInterlock( path );

    path.pop_back();
    return *this;
}




Entry&
Entry::setEntryInformation( LQIO::DOM::Entry * entryInfo )
{
    /* Open arrival stuff. */
    if ( hasOpenArrivals() ) {
	setIsCalledBy( RequestType::OPEN_ARRIVAL );
    }
    return *this;
}



Entry&
Entry::setDOM( unsigned p, LQIO::DOM::Phase* phaseInfo )
{
    if (phaseInfo == nullptr) return *this;
    setMaxPhase(p);
    _phase[p].setDOM(phaseInfo);
    return *this;
}



/*
 * Set the service time for phase `phase' and total over all phases.
 */

Entry&
Entry::addServiceTime( const unsigned p, const double value )
{
    if ( value == 0.0 ) return *this;

    setMaxPhase( p );
    _phase[p].addServiceTime( value );
    _total.setServiceTime( std::accumulate( _phase.begin(), _phase.end(), 0., add_using<Phase>( &Phase::serviceTime ) ) );
    return *this;
}



/*
 * Set the entry type field.
 */

bool
Entry::setIsCalledBy(const RequestType callType )
{
    if ( _calledBy != RequestType::NOT_CALLED && _calledBy != callType ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, name().c_str() );
	return false;
    } else {
	_calledBy = callType;
	return true;
    }
}


/*
 * mark whether the phase is present or not.
 */

Entry&
Entry::setMaxPhase( const unsigned ph )
{
    const unsigned int max_phase = maxPhase();
    if ( max_phase < ph ) {
	_phase.resize(ph);
	for ( unsigned int p = max_phase + 1; p <= ph; ++p ) {
	    std::string s;
	    s = "0123"[p];
	    _phase[p].setEntry( this )
		.setName( s )
		.configure( _total._wait.size() );
	}
    }
    max_phases = std::max( max_phase, max_phases );		/* Set global value.	*/

    return *this;
}



/*
 * Return 1 if the entry has think time and zero othewise.
 */

bool
Entry::hasVariance() const
{
    if ( isStandardEntry() ) {
	return std::any_of( _phase.begin(), _phase.end(), Predicate<Phase>( &Phase::hasVariance ) );
    } else {
	return true;
    }
}



/*
 * Check entry type.  If entry is NOT defined, the set entry type.
 * Return 1 if types match or entry type not set.
 */

bool
Entry::entryTypeOk( const LQIO::DOM::Entry::Type aType )
{
    if ( _entryType == LQIO::DOM::Entry::Type::NOT_DEFINED ) {
	_entryType = aType;
	return true;
    } else {
	return _entryType == aType;
    }
}



/*
 * Check entry type.  If entry is NOT defined, the set entry type.
 * Return 1 if types match or entry type not set.
 */

bool
Entry::entrySemaphoreTypeOk( const LQIO::DOM::Entry::Semaphore aType )
{
    if ( _semaphoreType == LQIO::DOM::Entry::Semaphore::NONE ) {
	_semaphoreType = aType;
    } else if ( _semaphoreType != aType ) {
	LQIO::input_error2( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name().c_str() );
	return false;
    }
    return true;
}


/*
 * Return the number of concurrent threads callable from this entry.
 */

unsigned
Entry::concurrentThreads() const
{
    if ( !isActivityEntry() ) return 1;

    return getStartActivity()->concurrentThreads( 1 );
}



/*
 * Set the throughput of this entry to value.  If this is an activity entry, then
 * we push the throughput to all activities reachable from this entry.
 * The throughput of an activity is the sum of the throughput from all calling entries.
 * Ergo, we have to push the entry's index to the activities.
 */

Entry&
Entry::saveThroughput( const double value )
{
    setThroughput( value );

    if ( flags.trace_replication || flags.trace_throughput ) {
	std::cout << " Entry::throughput(): Task=" << this->owner()->name() << ", Entry=" << this->name()
	     << ", Throughput=" << _throughput << std::endl;
    }

    if ( isActivityEntry() ) {
	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack;
	entryStack.push_back( this );
	Activity::Collect collect( 0, &Activity::setThroughput );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();
    }
    return *this;
}



/*
 * Set the value of calls to entry `toEntry', `phase'.  Retotal
 * total.
 */

Entry&
Entry::rendezvous( Entry * toEntry, const unsigned p, const LQIO::DOM::Call* callDOMInfo )
{
    if ( callDOMInfo == nullptr ) return *this;
    setMaxPhase( p );

    _phase[p].rendezvous( toEntry, callDOMInfo );
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

double
Entry::rendezvous( const Entry * entry ) const
{
    return std::accumulate( _phase.begin(), _phase.end(), 0., add_using_arg<Phase,const Entry *>( &Phase::rendezvous, entry ) );
}



/*
 * Return number of rendezvous to dstTask.  Used by class ijinfo.
 */

const Entry&
Entry::rendezvous( const Entity *dstTask, VectorMath<double>& calls ) const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	calls[p] += _phase[p].rendezvous( dstTask );
    }
    return *this;
}



/*
 * Set the value of send-no-reply calls to entry `toEntry', `phase'.
 * Retotal.
 */

Entry&
Entry::sendNoReply( Entry * toEntry, const unsigned p, const LQIO::DOM::Call* callDOMInfo )
{
    if ( callDOMInfo == nullptr ) return *this;

    setMaxPhase( p );

    _phase[p].sendNoReply( toEntry, callDOMInfo );
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not returns 0.
 */

double
Entry::sendNoReply( const Entry * entry ) const
{
    return std::accumulate( _phase.begin(), _phase.end(), 0., add_using_arg<Phase,const Entry *>( &Phase::sendNoReply, entry ) );
}



/*
 * Return the sum of all calls from the receiver during it's phase `p'.
 */


double
Entry::sumOfSendNoReply( const unsigned p ) const
{
    const std::set<Call *>& callList = _phase[p].callList();
    return std::accumulate( callList.begin(), callList.end(), 0., add_using<Call>( &Call::sendNoReply ) );
}



/*
 * Store forwarding probability in call list.
 */

Entry&
Entry::forward( Entry * toEntry, const LQIO::DOM::Call* call  )
{
    if ( !call ) return *this;

    setMaxPhase( 1 );
    _phase[1].forward( toEntry, call );
    return *this;
}



/*
 * Set starting activity for this entry.
 */

Entry&
Entry::setStartActivity( Activity * anActivity )
{
    _startActivity = anActivity;
    return *this;
}


/*
 * Reference the value of calls to entry.  The entry must already
 * exist.  If not, returns 0.
 */

double
Entry::processorCalls() const
{
    return std::accumulate( _phase.begin(), _phase.end(), 0., add_using<Phase>( &Phase::processorCalls ) );
}



#if PAN_REPLICATION
/*
 * Clear replication variables for this pass.
 */

Entry&
Entry::resetReplication()
{
    std::for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::resetReplication ) );
    return *this;
}
#endif



/*
 * Compute the coefficient of variation.
 */

double
Entry::computeCV_sqr() const
{
    const double sum_S = std::accumulate( _phase.begin(), _phase.end(), 0., add_using<Phase>( &Phase::elapsedTime ) );

    if ( !std::isfinite( sum_S ) ) {
	return sum_S;
    } else if ( sum_S > 0.0 ) {
	const double sum_V = std::accumulate( _phase.begin(), _phase.end(), 0., add_using<Phase>( &Phase::variance ) );
	return sum_V / square(sum_S);
    } else {
	return 0.0;
    }
}



/*
 * Return the waiting time for all submodels except submodel for phase
 * `p'.  If this is an activity entry, we have to return the chain k
 * component of waiting time.  Note that if submodel == 0, we return
 * the elapsedTime().  For servers in a submodel, submodel == 0; for
 * clients in a submodel, submodel == aSubmodel.number().
 */

/* As a client (submodel != 0) -- don't count join delays! */
/* locate thread k and run wait except */

double
Entry::waitExcept( const unsigned submodel, const unsigned k, const unsigned p ) const
{
    const Task * task = dynamic_cast<const Task *>(owner());
    const unsigned ix = task->threadIndex( submodel, k );

    if ( isStandardEntry() || submodel == 0 || ix <= 1 ) {
	return _phase[p].waitExcept( submodel );			/* Elapsed time is by entry */
    } else {
	//To handle the case of a main thread of control with no fork join.
	return task->waitExcept( ix, submodel, p );
    }
}



#if PAN_REPLICATION
/*
 * Return waiting time.  Normally, we exclude all of chain k, but with
 * replication, we have to include replicas-1 wait for chain k too.
 */

//REPL changes  REP N-R

double
Entry::waitExceptChain( const unsigned submodel, const unsigned k, const unsigned p ) const
{
    const Task * task = dynamic_cast<const Task *>(owner());
    const unsigned ix = task->threadIndex( submodel, k );

    if ( isStandardEntry() || ix <= 1 ) {
	return _phase[p].waitExceptChain( submodel, k );			/* Elapsed time is by entry */
    } else {
	//To handle the case of a main thread of control with no fork join.
	return task->waitExceptChain( ix, submodel, k, p );
    }
}
#endif



/*
 * Return utilization over all phases.
 */

double
Entry::utilization() const
{
    return std::accumulate( _phase.begin(), _phase.end(), 0., add_using<Phase>( &Phase::utilization ) );
}



/*
 * Return probability of visiting.
 */

Probability
Entry::prVisit() const
{
    if ( owner()->isReferenceTask() || owner()->throughput() == 0.0 ) {
	return Probability( 1.0 / owner()->nEntries() );
    } else {
	return Probability( throughput() / owner()->throughput() );
    }
}



/*
 * Find the mean slice time and other important bits of information
 * needed for the markov phase 2 server.  Slice time is averaged for
 * entries composed of activities.
 */

void
Entry::sliceTime( const Entry& dst, Slice_Info slice[], double y_xj[] ) const
{
    slice[0].initialize( owner()->thinkTime() );

    /* Accumulate slice information. */

    y_xj[0] = 0.0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	slice[p].initialize( _phase[p], dst );
	y_xj[p] = slice[p].normalize();
	y_xj[0] += y_xj[p];
    }
}



/*
 * Set up interlocking tables and set up paths for locating remote
 * join points.
 */

const Entry&
Entry::followInterlock( Interlock::CollectTable& path ) const
{
    if ( isActivityEntry() ) {
	getStartActivity()->followInterlock( path );
    } else {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    Interlock::CollectTable branch( path, p > 1 );
	    _phase[p].followInterlock( branch );
	}
    }
    return *this;
}



/*
 * Recursively search from this entry to any entry on myServer.
 * When we pop back up the call stack we add all calling tasks
 * for each arc which calls myServer.  The task adder
 * will ignore duplicates.
 */

bool
Entry::getInterlockedTasks( Interlock::CollectTasks& path ) const
{
    bool found = false;

    if ( path.server() == owner() ) {

	/*
	 * Special case -- we have hit the end of the line.
	 * Indicate that the path being followed is on the path,
	 * but do not add the server itself to the interlocked
	 * task set.
	 */

	return true;
    }

    /*
     * Check all outgoing paths of this task.  If head of path,
     * then any call o.k, otherwise, only phase 1 allowed.
     */

    const bool headOfPath = path.headOfPath();
    const unsigned int max_phase = headOfPath ? maxPhase() : 1;

    path.push_back( this );
    if ( isStandardEntry() ) {
	for ( unsigned p = 1; p <= max_phase; ++p ) {
	    if ( _phase[p].getInterlockedTasks( path ) ) found = true;
	}
    } else if ( isActivityEntry() ) {
	found = getStartActivity()->getInterlockedTasks( path );
    }
    path.pop_back();
    
    if ( found && !headOfPath ) {
	path.insert( owner() );
    }

    return found;
}



/*
 * Return true if this entry belongs to a reference task.
 * This is an error!
 */

bool
Entry::isReferenceTaskEntry() const
{
    return owner() && owner()->isReferenceTask();
}



/*
 * Return true if dst is reachable from any entry of src.
 */

bool
Entry::isInterlocked( const Entry * dstEntry ) const
{
    return _interlock[dstEntry->entryId()].all != 0.0;
}



/*
 * Check for dropped messages.
 */

bool
Entry::checkDroppedCalls() const
{
    bool rc = std::isfinite( openWait() );
    for ( std::set<Call *>::const_iterator call = callerList().begin(); call != callerList().end(); ++call ) {
	rc = rc && ( !(*call)->hasSendNoReply() || std::isfinite( (*call)->wait() ) );
    }
    return rc;
}

const Entry&
Entry::insertDOMResults(double *phaseUtils) const
{
    if ( getReplicaNumber() != 1 ) return *this;		/* NOP */

    double totalPhaseUtil = 0.0;

    /* Write the results into the DOM */
    const double throughput = this->throughput();		/* Used to compute utilization at activity entries */
    _dom->setResultThroughput(throughput)
	.setResultThroughputBound(throughputBound());

    for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	const double u = phase->utilization();
	const unsigned p = phase - _phase.begin();
	totalPhaseUtil += u;
	phaseUtils[p] += u;

	if ( !isActivityEntry() && phase->isPresent() ) {
	    phase->insertDOMResults();
	}
    }

    /* Store the utilization and squared coeff of variation */
    _dom->setResultUtilization(totalPhaseUtil)
	.setResultProcessorUtilization(processorUtilization())
	.setResultSquaredCoeffVariation(computeCV_sqr());

    /* Store activity phase data */
    if (isActivityEntry()) {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    const Phase& phase = _phase[p];
	    const double service_time = phase.elapsedTime();
	    _dom->setResultPhasePServiceTime(p,service_time)
		.setResultPhasePVarianceServiceTime(p,phase.variance())
		.setResultPhasePProcessorWaiting(p,phase.queueingTime())
		.setResultPhasePUtilization(p,service_time * throughput);
	    /*+ BUG 675 */
	    if ( _dom->hasHistogramForPhase( p ) || _dom->hasMaxServiceTimeExceededForPhase( p ) ) {
		NullPhase::insertDOMHistogram( const_cast<LQIO::DOM::Histogram*>(_dom->getHistogramForPhase( p )), phase.elapsedTime(), phase.variance() );
	    }
	    /*- BUG 675 */
	}
    }

    /* Do open arrival rates... */
    if ( hasOpenArrivals() ) {
	_dom->setResultWaitingTime(openWait());
    }
    return *this;
}



/*
 * Debug...
 */

std::ostream&
Entry::printCalls( std::ostream& output, unsigned int submodel ) const
{
    CallInfo calls( *this, LQIO::DOM::Call::Type::RENDEZVOUS );

    for ( std::vector<CallInfoItem>::const_iterator y = calls.begin(); y != calls.end(); ++y ) {
	const Entry& src = *y->srcEntry();
	const Entry& dst = *y->dstEntry();
	if ( submodel != 0 && dst.owner()->submodel() != submodel ) continue;
	output << std::setw(2) << " " << src.name() << "." << src.getReplicaNumber()
	       << " -> " << dst.name() << "." << dst.getReplicaNumber() << std::endl;
    }

    CallInfo fwds( *this, LQIO::DOM::Call::Type::FORWARD );
    for ( std::vector<CallInfoItem>::const_iterator y = calls.begin(); y != calls.end(); ++y ) {
	const Entry& src = *y->srcEntry();
	const Entry& dst = *y->dstEntry();
//	if ( submodel != 0 && dst.owner()->submodel() != submodel ) continue;
	output << std::setw(2) << " " << src.name() << "." << src.getReplicaNumber()
	       << " -> " << dst.name() << "." << dst.getReplicaNumber() << std::endl;
    }
    return output;
}

std::ostream&
Entry::printSubmodelWait( std::ostream& output, unsigned int offset ) const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	if ( offset ) {
	    output << std::setw( offset ) << " ";
	}
	output << std::setw(8-offset) ;
	if ( p == 1 ) {
	    output << name();
	} else {
	    output << " ";
	}
	output << " " << std::setw(1) << p << "  ";
	for ( unsigned j = 1; j <= _phase[p]._wait.size(); ++j ) {
	    output << std::setw(8) << _phase[p]._wait[j];
	}
	output << std::endl;
    }
    return output;
}

/* --------------------------- Dynamic LQX  --------------------------- */

Entry&
Entry::recalculateDynamicValues()
{
    std::for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::recalculateDynamicValues ) );
    _total.setServiceTime( std::accumulate( _phase.begin(), _phase.end(), 0., add_using<Phase>( &Phase::serviceTime ) ) );
    sanityCheckParameters();
    return *this;
}

Entry&
Entry::sanityCheckParameters()
{
    /*
     * After we have finished recalculating we need to make sure once again that any of the dynamic
     * parameters/late-bound parameters are still sane. The ones that could have changed are
     * checked in the following order:
     *
     *   1. Entry Priority (No Constraints)
     *   2. Open Arrival Rate
     *
     */

    /* Make sure the open arrival rate is sane for the setup */
    if ( getDOM() && getDOM()->hasOpenArrivalRate() ) {
	if ( owner()->isReferenceTask() ) {
	    LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_OPEN_ARRIVALS, owner()->name().c_str(), name().c_str() );
	}
    }
    return *this;
}

/* --------------------------- Task Entries --------------------------- */


TaskEntry::TaskEntry( LQIO::DOM::Entry* domEntry, unsigned int index, bool global )
    : Entry(domEntry,index,global),
      _task(nullptr),
      _openWait(0.),
      _nextOpenWait(0.)
{
}

TaskEntry::TaskEntry( const TaskEntry& src, unsigned int replica )
    : Entry(src,replica),
      _task(nullptr),
      _openWait(0.0),
      _nextOpenWait(0.0)
{
    /* Set to replica task */
    _task = Task::find( src._task->name(), replica );
}
    

/*
 * Initialize processor waiting time, variance and priority
 */

TaskEntry&
TaskEntry::initProcessor()
{
    if ( isStandardEntry() ) {
	std::for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::initProcessor ) );
    }
    return *this;
}



/*
 * Set up waiting times for calls to subordinate tasks.
 */

TaskEntry&
TaskEntry::initWait()
{
    std::for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::initWait ) );
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must already
 * exist.  If not, returns 0.
 */

double
TaskEntry::processorCalls( const unsigned p ) const
{
    return _phase[p].processorCalls();
}



/*
 * Return utilization (not including "other" service).
 * For activity entries, serviceTime() was computed in configure().
 */

double
TaskEntry::processorUtilization() const
{
    const Processor * aProc = owner()->getProcessor();
    const double util = std::isfinite( throughput() ) ? throughput() * serviceTime() : 0.0;

    /* Adjust for processor rate */

    if ( aProc ) {
	return util * owner()->replicas() / ( aProc->rate() * aProc->replicas() );
    } else {
	return util;
    }
}



/*
 * Return time spent in the queue for the processor for this entry.
 */

double
TaskEntry::queueingTime( const unsigned p ) const
{
    if ( isStandardEntry() ) {
	return _phase[p].queueingTime();
    } else {
	return 0.0;
    }
}



/*
 * Compute the variance for this entry.
 */

TaskEntry&
TaskEntry::computeVariance()
{
    _total._variance = 0.0;
    if ( isActivityEntry() ) {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    _phase[p]._variance = 0.0;
	}

	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack; //( dynamic_cast<const Task *>(owner())->activities().size() );
	entryStack.push_back( this );
	Activity::Collect collect( 0, &Activity::collectWait );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();
	_total._variance += std::accumulate( _phase.begin(), _phase.end(), 0., add_using<Phase>( &Phase::variance ) );
    } else {
	_total._variance += std::for_each( _phase.begin(), _phase.end(), ExecSum<Phase,double>( &Phase::computeVariance ) ).sum();
    }
    if ( flags.trace_variance != 0 && (dynamic_cast<TaskEntry *>(this) != nullptr) ) {
	std::cout << "Variance(" << name() << ",p) ";
	for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	    std::cout << ( phase == _phase.begin() ? " = " : ", " ) << phase->variance();
	}
	std::cout << std::endl;
    }
    return *this;
}



/*
 * Common code for aggregation: reset entry information.
 */

void
Entry::set( const Entry * src, const Activity::Collect& data )
{
    const Activity::Function f = data.collect();
    const unsigned int submodel = data.submodel();

    if ( f == &Activity::collectServiceTime ) {
        setMaxPhase( std::max( maxPhase(), src->maxPhase() ) );
    } else if ( f == &Activity::setThroughput ) {
        setThroughput( src->throughput() * data.rate() );
    } else if ( f == &Activity::collectWait ) {
        for ( unsigned p = 1; p <= maxPhase(); ++p ) {
            if ( submodel == 0 ) {
                _phase[p]._variance = 0.0;
            } else {
                _phase[p]._wait[submodel] = 0.0;
            }
        }
#if PAN_REPLICATION
    } else if ( f == &Activity::collectReplication ) {
        for ( unsigned p = 1; p <= maxPhase(); ++p ) {
            _phase[p]._surrogateDelay.resize( src->_phase[p]._surrogateDelay.size() );
            _phase[p].resetReplication();
        }
#endif
    }
}

/*
 * Calculate total wait for a particular submodel and save.  Return
 * the difference between this pass and the previous.
 */

TaskEntry&
TaskEntry::updateWait( const Submodel& aSubmodel, const double relax )
{
    const unsigned submodel = aSubmodel.number();
    if ( submodel == 0 ) throw std::logic_error( "TaskEntry::updateWait" );

    /* Open arrivals first... */

    if ( _nextOpenWait > 0.0 ) {
	under_relax( _openWait, _nextOpenWait, relax );
    }

    /* Scan calls to other task for matches with submodel. */

    if ( isActivityEntry() ) {

	std::for_each( _phase.begin(), _phase.end(), clear_wait(submodel) );

	if ( flags.trace_activities ) {
	    std::cout << "--- AggreateWait for entry " << name() << " ---" << std::endl;
	}
	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack;
	entryStack.push_back( this );
	Activity::Collect collect( submodel, &Activity::collectWait );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();

	if ( flags.trace_delta_wait || flags.trace_activities ) {
	    std::cout << "--DW--  Entry(with Activities) " << name()
		 << ", submodel " << submodel << std::endl;
	    std::cout << "        Wait=";
	    for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
		std::cout << phase->_wait[submodel] << " ";
	    }
	    std::cout << std::endl;
	}

    } else {

	std::for_each( _phase.begin(), _phase.end(), Exec2<Phase,const Submodel&,double>( &Phase::updateWait, aSubmodel, relax ) );

    }

    _total._wait[submodel] = std::accumulate( _phase.begin(), _phase.end(), 0.0, add_wait( submodel ) );

    return *this;
}



/*
 * If submodel != 0, then we have the mean time for the submodel.
 * Overwise, we have the variance.  Called from actlist for handing
 *  "repeat", and "and", and "or" Forks.
 */

Entry&
Entry::aggregate( const unsigned submodel, const unsigned p, const Exponential& addend )
{
    if ( submodel ) {
	_phase[p]._wait[submodel] += addend.mean();

    } else if ( addend.variance() > 0.0 ) {

	/*
	 * two-phase quorum semantics. If the replying activity is
	 * inside the quorum fork-join, then the difference in
	 * variance when calculating phase 2 variance can be negative.
	 */

	_phase[p]._variance += addend.variance();
    }

    if (flags.trace_quorum) {
	std::cout << std::endl << "Entry::aggregate(): submodel=" << submodel <<", entry " << name() << std::endl;
	std::cout <<"    addend.mean()=" << addend.mean() <<", addend.variance()="<<addend.variance()<< std::endl;
    }

    return *this;
}



#if PAN_REPLICATION
/* BUG_1
 * Calculate total wait for a particular submodel and save.  Return
 * the difference between this pass and the previous.
 */

double
TaskEntry::updateWaitReplication( const Submodel& aSubmodel, unsigned & n_delta )
{
    double delta = 0.0;
    if ( isActivityEntry() ) {
	std::for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::resetReplication ) );

	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack;
	entryStack.push_back( this );
	Activity::Collect collect( aSubmodel.number(), &Activity::collectReplication );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();

    } else {
	delta = std::for_each( _phase.begin(), _phase.end(), ExecSum1<Phase,double,const Submodel&>( &Phase::updateWaitReplication, aSubmodel )).sum();
	n_delta += _phase.size();
    }
    return delta;
}
#endif



#if PAN_REPLICATION
/*
 */

Entry&
Entry::aggregateReplication( const Vector< VectorMath<double> >& addend )
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	_phase[p]._surrogateDelay += addend[p];
    }
    return *this;
}
#endif



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

const Entry&
Entry::callsPerform( callFunc aFunc, const unsigned submodel, const unsigned k ) const
{
    const double rate = prVisit();

    if ( isActivityEntry() ) {
	/* since 'rate=prVisit()' is only for call::setvisit;
	 * for a virtual entry, the throughput of its corresponding activity
	 * is used to calculation entry throughput not the throughput of its owner task.
	 * the visit of a call equals rate * rendenzvous() normally;
	 * therefore, rate has to be set to 1.*/
	getStartActivity()->callsPerform( Phase::CallExec( this, submodel, k, 1, aFunc, rate ) );
    } else {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    _phase[p].callsPerform( Phase::CallExec( this, submodel, k, p, aFunc, rate ) );
	}
    }
    return *this;
}

/* -------------------------- Device Entries -------------------------- */

/*
 * Make a processor Entry.
 */

DeviceEntry::DeviceEntry( LQIO::DOM::Entry* dom, Processor * processor )
    : Entry( dom, processor->nEntries() ), _processor(processor)
{
    processor->addEntry( this );
}


DeviceEntry::DeviceEntry( const DeviceEntry& src, unsigned int replica )
    : Entry( src, replica ),
      _processor(nullptr)
{
    /* !!! See task.cc to find the replica processor */
}


DeviceEntry::~DeviceEntry()
{
}

/*
 * Initialize processor waiting time, variance and priority
 */

DeviceEntry&
DeviceEntry::initProcessor()
{
    throw should_not_implement( "DeviceEntry::initProcessor", __FILE__, __LINE__ );
    return *this;
}



/*
 * Initialize savedWait fields.
 */

DeviceEntry&
DeviceEntry::initWait()
{
    const unsigned i  = owner()->submodel();
    const double time = _phase[1].serviceTime();

    _phase[1]._wait[i] = time;
    _total._wait[i] = time;
    return *this;
}


/*
 * Initialize variance for the processor entry.  This value doesn't change during solution,
 * so computeVariance() at the entry level is a NOP.  However, we still need to initialize it.
 */

DeviceEntry&
DeviceEntry::initVariance()
{
    _phase[1].initVariance();
    return *this;
}


/*
 * Set the entry owner to task.
 */

DeviceEntry&
DeviceEntry::owner( const Entity * )
{
    throw should_not_implement( "DeviceEntry::owner", __FILE__, __LINE__ );
    return *this;
}


/*
 * Set the service time for phase `phase' for the device.  Device entries have only one phase.
 */

DeviceEntry&
DeviceEntry::setServiceTime( const double service_time )
{
    LQIO::DOM::Phase* phaseDom = _dom->getPhase(1);
    phaseDom->setServiceTime(new LQIO::DOM::ConstantExternalVariable(service_time/dynamic_cast<const Processor *>(_processor)->rate()));
    setDOM(1, phaseDom );
    return *this;
}


DeviceEntry&
DeviceEntry::setCV_sqr( const double cv_square )
{
    LQIO::DOM::Phase* phaseDom = _dom->getPhase(1);
    phaseDom->setCoeffOfVariationSquaredValue(cv_square);
    return *this;
}


DeviceEntry&
DeviceEntry::setPriority( const int priority )
{
    _dom->setEntryPriority( new LQIO::DOM::ConstantExternalVariable( priority ) );
    return *this;
}


/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

double
DeviceEntry::processorCalls( const unsigned ) const
{
    throw should_not_implement( "DeviceEntry::processorCalls", __FILE__, __LINE__ );
    return 0.0;
}



/*
 * Return the waiting time for group `g' phase `p'.
 */

DeviceEntry&
DeviceEntry::updateWait( const Submodel&, const double )
{
    throw should_not_implement( "DeviceEntry::updateWait", __FILE__, __LINE__ );
    return *this;
}



#if PAN_REPLICATION
/*
 * Return the waiting time for group `g' phase `p'.
 */

double
DeviceEntry::updateWaitReplication( const Submodel&, unsigned&  )
{
    throw should_not_implement( "DeviceEntry::updateWaitReplication", __FILE__, __LINE__ );
    return 0.0;
}
#endif



/*
 * Return utilization (not including "other" service).
 */

double
DeviceEntry::processorUtilization() const
{
    const Processor * aProc = dynamic_cast<const Processor *>(owner());
    const double util = throughput() * serviceTime();

    if ( aProc ) {
	return util / aProc->rate();
    } else {
	return util;
    }
}



/*
 * Return time spent in the queue for the processor for this entry.
 */

double
DeviceEntry::queueingTime( const unsigned ) const
{
    throw should_not_implement( "DeviceEntry::queueingTime", __FILE__, __LINE__ );
    return 0.0;
}

/*
 * Create a fake DOM Entry as a place holder.
 */

VirtualEntry::VirtualEntry( const Activity * anActivity )
    : TaskEntry( new LQIO::DOM::Entry(anActivity->getDOM()->getDocument(), anActivity->name().c_str()),
		 anActivity->owner()->nEntries(), false )	/* Don't create an global entry id */
{
    owner( anActivity->owner() );
}


/*
 * Since we make it, we delete it.
 */

VirtualEntry::~VirtualEntry()
{
}

static bool
map_entry_name( const std::string& entry_name, Entry * & outEntry, bool receiver, const LQIO::DOM::Entry::Type aType = LQIO::DOM::Entry::Type::NOT_DEFINED )
{
    bool rc = true;
    outEntry = Entry::find( entry_name );

    if ( !outEntry ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry_name.c_str() );
	rc = false;
    } else if ( receiver && outEntry->isReferenceTaskEntry() ) {
	LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, outEntry->owner()->name().c_str(), entry_name.c_str() );
	rc = false;
    } else if ( aType != LQIO::DOM::Entry::Type::NOT_DEFINED && !outEntry->entryTypeOk( aType ) ) {
	LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, entry_name.c_str() );
    }

    return rc;
}

/* ----------------------- Accumulate Printing ------------------------ */

/*
 * Initialize record.
 */

CallInfoItem::CallInfoItem( const Entry * src, const Entry * dst )
    : phase(MAX_PHASES), source( src ), destination( dst )
{
    if ( src == nullptr || dst == nullptr ) throw std::logic_error( "CallInfoItem::CallInfoItem" );
}



/*
 *
 */

bool
CallInfoItem::hasRendezvous() const
{
    for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	if ( phase[p] && phase[p]->hasRendezvous() ) return true;
    }
    return false;
}


bool
CallInfoItem::hasSendNoReply() const
{
    for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	if ( phase[p] && phase[p]->hasSendNoReply() ) return true;
    }
    return false;
}


bool
CallInfoItem::hasForwarding() const
{
    return phase[1] && phase[1]->hasForwarding();
}


/*
 * Does this call item call another task?
 */

bool
CallInfoItem::isTaskCall() const
{
    for ( unsigned p = 1; p <= srcEntry()->maxPhase(); ++p ) {
	if ( phase[p] && !phase[p]->isProcessorCall() ) return true;
    }
    return false;
}


/*
 * Does this call item call a processor?
 */

bool
CallInfoItem::isProcessorCall() const
{
    for ( unsigned p = 1; p <= srcEntry()->maxPhase(); ++p ) {
	if ( phase[p] && phase[p]->isProcessorCall() ) return true;
    }
    return false;
}



/*
 * Locate all calls generated by entry regardless of phase.
 * Create a collection so that the () operator can step over it.
 */

CallInfo::CallInfo( const Entry& entry, LQIO::DOM::Call::Type callType )
    : _calls()
{
    if ( !entry.isStandardEntry() ) return;

    for ( unsigned p = 1; p <= entry.maxPhase(); ++p ) {
	const std::set<Call *>& callList = entry.callList( p );
	for ( std::set<Call *>::const_iterator call = callList.begin(); call != callList.end(); ++call ) {
	    if ( (*call)->isProcessorCall() || (*call)->dstEntry()->owner()->isProcessor() ) continue;

	    if ( (    (callType == LQIO::DOM::Call::Type::SEND_NO_REPLY) && (*call)->hasSendNoReply() )
		 || ( (callType == LQIO::DOM::Call::Type::FORWARD) && (*call)->isForwardedCall() )
		 || ( (callType == LQIO::DOM::Call::Type::RENDEZVOUS) && (*call)->hasRendezvous() && !(*call)->isForwardedCall() )
		) {

		std::vector<CallInfoItem>::iterator item = std::find_if( _calls.begin(), _calls.end(), compare( (*call)->dstEntry() ) );
		if ( item == _calls.end() ) {
		    _calls.emplace_back( CallInfoItem( &entry, (*call)->dstEntry() ) );
		    _calls.back().phase[p] = (*call);
		} else if ( item->phase[p] ) {
		    if ( item->phase[p]->isForwardedCall() && (*call)->hasRendezvous() ) {
			item->phase[p] = (*call);	/* Drop forward -- keep rnv */
			continue;
		    } else if ( item->phase[p]->hasRendezvous() && (*call)->isForwardedCall() ) {
			continue;
		    } else {
			LQIO::internal_error( __FILE__, __LINE__, "CallInfo::CallInfo" );
		    }
		} else {
		    item->phase[p] = (*call);
		}
	    }
	}
    }
}

/*----------------------------------------------------------------------*/
/*       Input processing.  Called from load.cc::prepareModel()         */
/*----------------------------------------------------------------------*/

Entry *
Entry::create(LQIO::DOM::Entry* dom, unsigned int index )
{
    const std::string& entry_name = dom->getName();

    if ( Entry::find( entry_name ) != nullptr ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Entry", entry_name.c_str() );
	return nullptr;
    } else {
	Entry * entry = new TaskEntry( dom, index );
	Model::__entry.insert( entry );

	/* Make sure that the entry type is set properly for all entries */
	if (entry->entryTypeOk(static_cast<const LQIO::DOM::Entry::Type>(dom->getEntryType())) == false) {
	    LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, entry_name.c_str() );
	}

	/* Set field width for entry names. */

	return entry;
    }
}

void
Entry::add_call( const unsigned p, const LQIO::DOM::Call* domCall )
{
    /* Make sure this is one of the supported call types */
    if (domCall->getCallType() != LQIO::DOM::Call::Type::SEND_NO_REPLY &&
	domCall->getCallType() != LQIO::DOM::Call::Type::RENDEZVOUS ) {
	abort();
    }

    LQIO::DOM::Entry* toDOMEntry = const_cast<LQIO::DOM::Entry*>(domCall->getDestinationEntry());
    const std::string& to_entry_name = toDOMEntry->getName();

    /* Internal Entry references */
    Entry * toEntry;

    /* Begin by mapping the entry names to their entry types */
    if ( !entryTypeOk(LQIO::DOM::Entry::Type::STANDARD) ) {
	LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, name().c_str() );
    } else if ( map_entry_name( to_entry_name, toEntry, true ) ) {
	if ( domCall->getCallType() == LQIO::DOM::Call::Type::RENDEZVOUS) {
	    rendezvous( toEntry, p, domCall );
	} else if ( domCall->getCallType() == LQIO::DOM::Call::Type::SEND_NO_REPLY ) {
	    sendNoReply( toEntry, p, domCall );
	}
    }
}



Entry&
Entry::setForwardingInformation( Entry* toEntry, LQIO::DOM::Call * call )
{
    /* Do some checks for sanity */
    if ( owner()->isReferenceTask() ) {
	LQIO::input_error2( LQIO::ERR_REF_TASK_FORWARDING, owner()->name().c_str(), name().c_str() );
    } else if ( forward( toEntry ) > 0.0 ) {
	LQIO::input_error2( LQIO::WRN_MULTIPLE_SPECIFICATION );
    } else {
	forward( toEntry, call );
    }
    return *this;
}


void
set_start_activity (Task* task, LQIO::DOM::Entry* entry_DOM)
{
    Activity* activity = task->findActivity(entry_DOM->getStartActivity()->getName());
    Entry* entry = nullptr;

    map_entry_name( entry_DOM->getName(), entry, false, LQIO::DOM::Entry::Type::ACTIVITY );
    entry->setStartActivity(activity);
    activity->setEntry(entry);
}

/* ---------------------------------------------------------------------- */

bool
Entry::equals::operator()( const Entry * entry ) const
{
    return entry->name() == _name && entry->getReplicaNumber() == _replica;
}


/*
 * Find the entry and return it.
 */

/* static */ Entry *
Entry::find( const std::string& name, unsigned int replica )
{
    std::set<Entry *>::const_iterator entry = std::find_if( Model::__entry.begin(), Model::__entry.end(), EqualsReplica<Entry>( name, replica ) );
    return ( entry != Model::__entry.end() ) ? *entry : nullptr;
}


void
Entry::get_clients::operator()( const Entry * entry ) const
{
    _clients = std::accumulate( entry->callerList().begin(), entry->callerList().end(), _clients, Call::add_client );
}


/*
 * Return all tasks and processors called by this entry.
 */

void
Entry::get_servers::operator()( const Entry * entry ) const
{
    if ( entry->isActivityEntry() ) return;
    std::for_each( entry->_phase.begin(), entry->_phase.end(), Phase::get_servers( _servers ) );
}
