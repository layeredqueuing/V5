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
 * $Id: entry.cc 16704 2023-04-30 11:27:18Z greg $
 * ------------------------------------------------------------------------
 */


#include "lqns.h"
#include <cmath>
#include <functional>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <lqio/error.h>
#include <mva/prob.h>
#include <mva/server.h>
#include "actlist.h"
#include "call.h"
#include "entry.h"
#include "entrythread.h"
#include "errmsg.h"
#include "flags.h"
#include "model.h"
#include "option.h"
#include "pragma.h"
#include "processor.h"
#include "randomvar.h"
#include "submodel.h"
#include "task.h"
#include "variance.h"

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
	_phase[p].initialize( name() + "_" + "123"[p-1], p, this );
    }
}


/*
 * Deep copy.
 */

Entry::Entry( const Entry& src, unsigned int replica )
    : _dom(src._dom),
      _phase(src._phase.size()),	/* Don't copy call lists */
      _total(src._total.name()),
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
	_phase[p].initialize( name() + "_" + "123"[p-1], p, this );
	_phase[p].setDOM( src._phase[p].getDOM() );
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
	    getDOM()->throw_invalid_parameter( "open arrival rate", e.what() );
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
	    getDOM()->throw_invalid_parameter( "priority", e.what() );
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

    if ( owner()->isReferenceTask() ) {
	if ( hasOpenArrivals() ) {
	    owner()->getDOM()->input_error( LQIO::ERR_REFERENCE_TASK_OPEN_ARRIVALS, name().c_str() );
	} else if ( isCalled() ) {
	    getDOM()->runtime_error( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, name().c_str() );
	}
    }

    if ( isStandardEntry() ) {
	std::for_each( _phase.begin(), _phase.end(), std::mem_fn( &Phase::check ) );
    } else if ( isActivityEntry() ) {
	if ( !isVirtualEntry() ) {
	    Activity::Count_If data( this, &Activity::checkReplies );

	    std::deque<const Activity *> activityStack;
	    const double replies = std::floor( getStartActivity()->count_if( activityStack, data ).sum() * precision ) / precision;

	    //tomari: disable to allow a quorum use the default reply which is after all threads completes exection.
	    //(replies == 1 || (replies == 0 && owner->hasQuorum()))
	    //Only tasks have activity entries.
	    if ( isCalledUsingRendezvous() && replies != 1.0 && (replies != 0.0 || !dynamic_cast<const Task *>(owner())->hasQuorum()) ) {
		if ( replies == 0 ) {
		    getDOM()->runtime_error( LQIO::ERR_REPLY_NOT_GENERATED );		/* redundant, but more explicit. */
		} else {
		    getDOM()->runtime_error( LQIO::ERR_NON_UNITY_REPLIES, replies );
		}
	    }
	    assert( activityStack.size() == 0 );
	}
    } else {
	getDOM()->runtime_error( LQIO::ERR_NOT_SPECIFIED );
    }

    if ( owner()->scheduling() != SCHEDULE_SEMAPHORE && (isSignalEntry() || isWaitEntry()) ) {
	getDOM()->runtime_error( LQIO::ERR_NOT_SEMAPHORE_TASK, (isSignalEntry() ? "signal" : "wait"), name().c_str() );
    }
    if ( !owner()->isReferenceTask() && !isCalled() ) {
	if ( owner()->scheduling() == SCHEDULE_SEMAPHORE ||  owner()->scheduling() == SCHEDULE_RWLOCK ) {
	    getDOM()->setSeverity( LQIO::WRN_ENTRY_HAS_NO_REQUESTS, LQIO::error_severity::ERROR );
	}
	getDOM()->runtime_error( LQIO::WRN_ENTRY_HAS_NO_REQUESTS );
    }
    if ( maxPhase() > 1 && owner()->isInfinite() ) {
	LQIO::runtime_error( WRN_MULTI_PHASE_INFINITE_SERVER, name().c_str(), owner()->name().c_str(), maxPhase() );
    }

    /* Forwarding probabilities o.k.? */

    double sum = std::accumulate( callList(1).begin(), callList(1).end(), 0.0, Call::add_forwarding );
    if ( sum < 0.0 || 1.0 < sum ) {
	getDOM()->runtime_error( LQIO::ERR_INVALID_FORWARDING_PROBABILITY, sum );
    } else if ( sum != 0.0 && owner()->isReferenceTask() ) {
	getDOM()->runtime_error( LQIO::ERR_REFERENCE_TASK_FORWARDING, name().c_str() );
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
    std::for_each( _phase.begin(), _phase.end(), Exec1<NullPhase,const unsigned>( &NullPhase::configure, nSubmodels ) );
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
    std::for_each( _phase.begin(), _phase.end(), std::mem_fn( &Phase::expandCalls ) );
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
	    getDOM()->runtime_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, error.what() );
	}
	catch ( const bad_external_join& error ) {
	    abort();
	}
    } else {
	max_depth = std::accumulate( _phase.begin(), _phase.end(), max_depth, Entry::max_depth( &Phase::findChildren, callStack, directPath ) );
    }

    return max_depth;
}



/*+ BUG_425 */
/*
 * Set the number of customers by chain (reference task/open arrival).
 */

Entry&
Entry::initCustomers( std::deque<const Task *>& stack, unsigned int customers )
{
#if BUG_425
    std::cerr << std::setw( stack.size() * 2 ) << " " << ".." << print_name() << "->Entry::initCustomers(" << stack.size() << "," << customers << ")" << std::endl;
#endif
    /* for all phases, activities, chase calls. -- since calls end up at entries... recurse there */
    if ( isActivityEntry() ) {
	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack;
	entryStack.push_back( this );
	Activity::Collect collect( &Activity::collectCustomers, stack, customers );
	getStartActivity()->collect( activityStack, entryStack, collect );
    } else {
	std::for_each( _phase.begin(), _phase.end(), Exec2<Phase,std::deque<const Task *>&,unsigned int>( &Phase::initCustomers, stack, customers ) );
    }
    return *this;
}
/*- BUG_425 */


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
	Activity::Collect collect( &Activity::collectServiceTime );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();
    }

    _total.setServiceTime( std::accumulate( _phase.begin(), _phase.end(), 0., Phase::sum( &Phase::serviceTime ) ) );
    return *this;
}



#if PAN_REPLICATION
/*
 * Allocate storage for oldSurgDelay.
 */

Entry&
Entry::setSurrogateDelaySize( size_t n_chains )
{
    std::for_each( _phase.begin(), _phase.end(), Exec1<Phase,size_t>( &Phase::setSurrogateDelaySize, n_chains ) );
    return *this;
}
#endif



Entry&
Entry::resetInterlock()
{
    std::for_each( _interlock.begin(), _interlock.end(), std::mem_fn( &InterlockInfo::reset ) );
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
    initializeInterlock( calls );
    return *this;
}



void
Entry::initializeInterlock( Interlock::CollectTable& path )
{
    /*
     * Check for cycles in graph.  Return if found.  Cycle catching
     * is done by the other version.  Someday, we might make this more
     * intelligent to compute the final prob. of following the arc.
     */
    if ( path.has_entry( this ) ) return;

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
    _total.setServiceTime( std::accumulate( _phase.begin(), _phase.end(), 0., Phase::sum( &Phase::serviceTime ) ) );
    return *this;
}



/*
 * Set the entry type field.
 */

bool
Entry::setIsCalledBy(const RequestType callType )
{
    if ( _calledBy != RequestType::NOT_CALLED && _calledBy != callType ) {
	getDOM()->runtime_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES );
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
	_phase.resize( ph );
	for ( unsigned int p = max_phase + 1; p <= ph; ++p ) {
	    _phase[p].initialize( name() + "_" + "123"[p-1], p, this );
	    _phase[p].configure( _total.getWaitSize() );
#if PAN_REPLICATION
	    _phase[p].setSurrogateDelaySize( _phase[1].getSurrogateDelaySize() );
#endif
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
	return std::any_of( _phase.begin(), _phase.end(), std::mem_fn( &Phase::hasVariance ) );
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
 * Set the throughput of this entry to value.  If this is an activity
 * entry, then we push the throughput to all activities reachable from
 * this entry.  The throughput of an activity is the sum of the
 * throughput from all calling entries.  Ergo, we have to push the
 * entry's index to the activities.
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
	Activity::Collect collect( &Activity::setThroughput );
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
    return std::accumulate( _phase.begin(), _phase.end(), 0., add_calls( &Phase::rendezvous, entry ) );
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
    return std::accumulate( _phase.begin(), _phase.end(), 0., add_calls( &Phase::sendNoReply, entry ) );
}



/*
 * Return the sum of all calls from the receiver during it's phase `p'.
 */

double
Entry::sumOfSendNoReply( const unsigned p ) const
{
    const std::set<Call *>& callList = _phase[p].callList();
    return std::accumulate( callList.begin(), callList.end(), 0., Call::sum( &Call::sendNoReply ) );
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
    return std::accumulate( _phase.begin(), _phase.end(), 0., Phase::sum( &Phase::processorCalls ) );
}



#if PAN_REPLICATION
/*
 * Clear replication variables for this pass.
 */

Entry&
Entry::clearSurrogateDelay()
{
    std::for_each( _phase.begin(), _phase.end(), std::mem_fn( &Phase::clearSurrogateDelay ) );
    return *this;
}
#endif



/*
 * Compute the coefficient of variation.
 */

double
Entry::computeCV_sqr() const
{
    const double sum_S = std::accumulate( _phase.begin(), _phase.end(), 0., Phase::sum( &Phase::residenceTime ) );

    if ( !std::isfinite( sum_S ) ) {
	return sum_S;
    } else if ( sum_S > 0.0 ) {
	const double sum_V = std::accumulate( _phase.begin(), _phase.end(), 0., Phase::sum( &Phase::variance ) );
	return sum_V / square(sum_S);
    } else {
	return 0.0;
    }
}



/*
 * Return the waiting time for all submodels except submodel for phase
 * `p'.  If this is an activity entry, we have to return the chain k
 * component of waiting time.  Note that if submodel == 0, we return
 * the residenceTime().  For servers in a submodel, submodel == 0; for
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
 * Return utilization over all phases.  Derived from throughput and residence times.
 */

double
Entry::utilization() const
{
    return std::accumulate( _phase.begin(), _phase.end(), 0., Phase::sum( &Phase::utilization ) );
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


void
Entry::recalculateDynamicValues()
{
    std::for_each( _phase.begin(), _phase.end(), std::mem_fn( &Phase::recalculateDynamicValues ) );
    _total.setServiceTime( std::accumulate( _phase.begin(), _phase.end(), 0., Phase::sum( &Phase::serviceTime ) ) );
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
	    const double service_time = phase.residenceTime();
	    _dom->setResultPhasePServiceTime(p,service_time)
		.setResultPhasePVarianceServiceTime(p,phase.variance())
		.setResultPhasePProcessorWaiting(p,phase.queueingTime())
		.setResultPhasePUtilization(p,service_time * throughput);
	    /*+ BUG 675 */
	    if ( _dom->hasHistogramForPhase( p ) || _dom->hasMaxServiceTimeExceededForPhase( p ) ) {
		NullPhase::insertDOMHistogram( const_cast<LQIO::DOM::Histogram*>(_dom->getHistogramForPhase( p )), phase.residenceTime(), phase.variance() );
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
    CallInfo rnv_calls( *this, LQIO::DOM::Call::Type::RENDEZVOUS );
    std::for_each( rnv_calls.begin(), rnv_calls.end(), print_call( output, submodel, "->" ) );

    CallInfo snr_calls( *this, LQIO::DOM::Call::Type::SEND_NO_REPLY );
    std::for_each( snr_calls.begin(), snr_calls.end(), print_call( output, submodel, "~>" ) );

#if 0
    CallInfo fwds( *this, LQIO::DOM::Call::Type::FORWARD );
    for ( std::vector<CallInfo::Item>::const_iterator y = calls.begin(); y != calls.end(); ++y ) {
	const Entry& src = *y->srcEntry();
	const Entry& dst = *y->dstEntry();
//	if ( submodel != 0 && dst.owner()->submodel() != submodel ) continue;
	output << std::setw(2) << " " << src.name() << "." << src.getReplicaNumber()
	       << " -> " << dst.name() << "." << dst.getReplicaNumber() << std::endl;
    }
#endif
    return output;
}


void
Entry::print_call::operator()( const CallInfo::Item& call ) const
{
    const Entry& src = *call.srcEntry();
    const Entry& dst = *call.dstEntry();
    if ( (_submodel != 0 && dst.owner()->submodel() != _submodel ) || ( src.owner()->isReplica() && dst.owner()->isReplica() ) ) return;
    _output << std::setw(2) << " " << src.print_name() << " " << _arrow << " " << dst.print_name() << std::endl;
}



std::ostream&
Entry::printSubmodelWait( std::ostream& output, unsigned int offset ) const
{
    for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	if ( offset ) {
	    output << std::setw( offset ) << " ";
	}
	output << std::setw(8-offset);
	if ( phase == _phase.begin() ) {
	    std::ostringstream s;
	    output_name( s, *this );
	    output << s.str();
	} else {
	    output << " ";
	}
	output << " " << std::setw(1) << phase->getPhaseNumber() << "  ";
	for ( unsigned j = 1; j <= phase->getWaitSize(); ++j ) {
	    output << std::setw(8) << phase->getWaitTime(j);
	}
	output << std::endl;
    }
    return output;
}


std::string
Entry::fold( const std::string& s1, const Entry * e2 )
{
    std::ostringstream s2;
    s2 << e2->print_name();
    return s1 + "," + s2.str();
}


/* static */ std::ostream&
Entry::output_name( std::ostream& output, const Entry& entry )
{
    output << entry.name();
    if ( entry.owner()->isReplicated() ) {
	output << "." << entry.getReplicaNumber();
    }
    return output;
}

/* ------------------------------ Results ----------------------------- */

/*
 * Save client results.
 */

void
Entry::SaveClientResults::operator()( Entry * entry ) const
{
    const unsigned e = entry->index();
#if PAN_REPLICATION
    if ( _submodel.usePanReplication() ) {

	/*
	 * Get throughput PER CUSTOMER because replication monkeys
	 * with the population levels.  Fix for multiservers.
	 */

	entry->saveThroughput( _submodel.closedModelNormalizedThroughput( _station, e, _k ) * _client.population() );
    } else {
#endif
	entry->saveThroughput( _submodel.closedModelThroughput( _station, e, _k ) );
#if PAN_REPLICATION
    }
#endif
}

void
Entry::SaveServerResults::operator()( Entry * entry ) const
{
    const unsigned e = entry->index();
    double lambda = 0.0;

    if ( _server.isOpenModelServer() ) {
	lambda = _submodel.openModelThroughput( _station, e );		/* BUG_168 */
	entry->saveOpenWait( _station.R( e, 0 ) );
    }

    if ( _server.isClosedModelServer() ) {
	const double tput = _submodel.closedModelThroughput( _station, e );
	if ( std::isfinite( tput ) ) {
	    lambda += tput;
	} else if ( tput < 0.0 ) {
	    throw std::domain_error( "MVASubmodel::saveServerResults" );
	} else {
	    lambda = tput;
	}
    }
    entry->saveThroughput( lambda );
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
 * Initialize processor waiting time, variance and priority.
 * Activities are done by the task.
 */

TaskEntry&
TaskEntry::initProcessor()
{
    if ( isStandardEntry() ) {
	std::for_each( _phase.begin(), _phase.end(), std::mem_fn( &Phase::initProcessor ) );
    }
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
 * Return utilization (not including "other" service).  For activity
 * entries, utilization is computed at the task level.
 */

double
TaskEntry::processorUtilization() const
{
    if ( !isStandardEntry() ) return 0.0;
    return std::accumulate( _phase.begin(), _phase.end(), 0.0, Phase::sum( &Phase::processorUtilization ) );
}



/*
 * Return time spent in the queue for the processor for this entry.
 */

double
TaskEntry::queueingTime( const unsigned p ) const
{
    return isStandardEntry() ? _phase[p].queueingTime() : 0.0;
}



/*
 * Compute the Type 1 throughput bounds.  Reference task think times will limit throughput
 */

void
Entry::computeThroughputBound()
{
    const double t = residenceTime() + owner()->thinkTime();
    if ( t > 0 ) {
	_throughputBound = owner()->copies() / t;
    } else {
	_throughputBound = 0.0;
    }
    saveThroughput( _throughputBound );		/* Push bound to entries/phases/activities */
}



/*
 * Compute the variance for this entry.
 */

TaskEntry&
TaskEntry::computeVariance()
{
    _total.setVariance( 0.0 );
    if ( isActivityEntry() ) {
	std::for_each( _phase.begin(), _phase.end(), Exec1<NullPhase,double>( &Phase::setVariance, 0.0 ) );
	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack; //( dynamic_cast<const Task *>(owner())->activities().size() );
	entryStack.push_back( this );
	Activity::Collect collect( &Activity::collectWait );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();
    } else {
	std::for_each( _phase.begin(), _phase.end(), std::mem_fn( &Phase::computeVariance ) );
    }
    _total.addVariance( std::accumulate( _phase.begin(), _phase.end(), 0., Phase::sum( &Phase::variance ) ) );
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
    const Activity::Collect::Function f = data.collect();
    const unsigned int submodel = data.submodel();

    if ( f == &Activity::collectServiceTime ) {
        setMaxPhase( std::max( maxPhase(), src->maxPhase() ) );
    } else if ( f == &Activity::setThroughput ) {
        setThroughput( src->throughput() * data.rate() );
    } else if ( f == &Activity::collectWait ) {
	if ( submodel == 0 ) {
	    std::for_each( _phase.begin(), _phase.end(), Exec1<NullPhase,double>( &Phase::setVariance, 0.0 ) );
	} else {
	    std::for_each( _phase.begin(), _phase.end(), Exec2<NullPhase,unsigned int,double>( &Phase::setWaitTime, submodel, 0.0 ) );
	}
#if PAN_REPLICATION
    } else if ( f == &Activity::collectReplication ) {
        setMaxPhase( std::max( maxPhase(), src->maxPhase() ) );
        for ( unsigned p = 1; p <= maxPhase(); ++p ) {
            _phase[p].clearSurrogateDelay();
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
	_openWait = under_relax( _openWait, _nextOpenWait, relax );
    }

    /* Scan calls to other task for matches with submodel. */

    if ( isActivityEntry() ) {

	std::for_each( _phase.begin(), _phase.end(), Exec2<NullPhase,unsigned int,double>( &Phase::setWaitTime, submodel, 0.0 ) );

	if ( flags.trace_activities ) {
	    std::cout << "--- AggreateWait for entry " << name() << " ---" << std::endl;
	}
	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack;
	entryStack.push_back( this );
	Activity::Collect collect( &Activity::collectWait, submodel );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();

	if ( Options::Trace::delta_wait( submodel ) || flags.trace_activities ) {
	    std::cout << "--DW--  Entry(with Activities) " << name()
		      << ", submodel " << submodel << std::endl;
	    std::cout << "        Wait=";
	    for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
		std::cout << phase->getWaitTime(submodel) << " ";
	    }
	    std::cout << std::endl;
	}

    } else {

	std::for_each( _phase.begin(), _phase.end(), Exec2<Phase,const Submodel&,double>( &Phase::updateWait, aSubmodel, relax ) );

    }

    _total.setWaitTime( submodel, std::accumulate( _phase.begin(), _phase.end(), 0.0, add_wait( submodel ) ) );

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
	_phase[p].addWaitTime( submodel, addend.mean() );

    } else if ( addend.variance() > 0.0 ) {

	/*
	 * two-phase quorum semantics. If the replying activity is
	 * inside the quorum fork-join, then the difference in
	 * variance when calculating phase 2 variance can be negative.
	 */

	_phase[p].addVariance( addend.variance() );
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
	std::for_each( _phase.begin(), _phase.end(), std::mem_fn( &Phase::clearSurrogateDelay ) );

	std::deque<const Activity *> activityStack;
	std::deque<Entry *> entryStack;
	entryStack.push_back( this );
	Activity::Collect collect( &Activity::collectReplication, aSubmodel.number() );
	getStartActivity()->collect( activityStack, entryStack, collect );
	entryStack.pop_back();

    } else {
	for ( auto& phase : _phase ) delta += phase.updateWaitReplication( aSubmodel );
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
	_phase[p].addSurrogateDelay( addend[p] );
    }
    return *this;
}
#endif



/*
 * For all calls to aServer perform f over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

const Entry&
Entry::callsPerform( Call::Perform& g ) const
{
    g.setPhase( 1 );
    g.setRate( prVisit() );

    if ( isActivityEntry() ) {
	/* since 'rate=prVisit()' is only for call::setvisit;
	 * for a virtual entry, the throughput of its corresponding activity
	 * is used to calculation entry throughput not the throughput of its owner task.
	 * the visit of a call equals rate * rendenzvous() normally;
	 * therefore, rate has to be set to 1.*/
	getStartActivity()->callsPerform( g );
    } else {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    _phase[p].callsPerform( g.setPhase( p ) );
	}
    }
    return *this;
}



void
Entry::CallsPerformWithChain::operator()( Entry * object )
{
    _g.setChain( _chains[_i] );
    object->callsPerform( _g );
    _i += 1;
}

/* -------------------------- Device Entries -------------------------- */

/*
 * Make a processor Entry.
 */

DeviceEntry::DeviceEntry( LQIO::DOM::Entry* dom, Processor * processor )
    : Entry( dom, processor->nEntries() ), _processor(processor)
{
    entryTypeOk( LQIO::DOM::Entry::Type::STANDARD );
    setIsCalledBy( RequestType::RENDEZVOUS );
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
    throw LQIO::should_not_implement( "DeviceEntry::initProcessor" );
    return *this;
}



/*
 * Initialize savedWait fields.
 */

DeviceEntry&
DeviceEntry::initWait()
{
    const unsigned submodel  = owner()->submodel();
    const double time = _phase[1].serviceTime();

    _phase[1].setWaitTime( submodel, time );
    _total.setWaitTime( submodel, time );
    for ( std::set<Call *>::iterator call = callerList().begin(); call != callerList().end(); ++call ) {
	(*call)->setWait( time );
    }
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
    throw LQIO::should_not_implement( "DeviceEntry::owner" );
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
    throw LQIO::should_not_implement( "DeviceEntry::processorCalls" );
    return 0.0;
}



/*
 * Return the waiting time for group `g' phase `p'.
 */

DeviceEntry&
DeviceEntry::updateWait( const Submodel&, const double )
{
    throw LQIO::should_not_implement( "DeviceEntry::updateWait" );
    return *this;
}



#if PAN_REPLICATION
/*
 * Return the waiting time for group `g' phase `p'.
 */

double
DeviceEntry::updateWaitReplication( const Submodel&, unsigned&  )
{
    throw LQIO::should_not_implement( "DeviceEntry::updateWaitReplication" );
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
    throw LQIO::should_not_implement( "DeviceEntry::queueingTime" );
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
map_entry_name( const std::string& entry_name, Entry * & entry, bool receiver, const LQIO::DOM::Entry::Type type = LQIO::DOM::Entry::Type::NOT_DEFINED )
{
    bool rc = true;
    entry = Entry::find( entry_name );

    if ( !entry ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry_name.c_str() );
	rc = false;
    } else if ( receiver && entry->owner()->isReferenceTask() ) {
	entry->owner()->getDOM()->input_error( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, entry_name.c_str() );
	rc = false;
    } else if ( type != LQIO::DOM::Entry::Type::NOT_DEFINED && !entry->entryTypeOk( type ) ) {
	entry->getDOM()->input_error( LQIO::ERR_MIXED_ENTRY_TYPES );
    }

    return rc;
}

/*----------------------------------------------------------------------*/
/*       Input processing.  Called from load.cc::prepareModel()         */
/*----------------------------------------------------------------------*/

Entry *
Entry::create(LQIO::DOM::Entry* dom, unsigned int index )
{
    const std::string& entry_name = dom->getName();

    if ( Entry::find( entry_name ) != nullptr ) {
	dom->runtime_error( LQIO::ERR_DUPLICATE_SYMBOL, dom->getTypeName() );
	return nullptr;
    } else {
	Entry * entry = new TaskEntry( dom, index );
	Model::__entry.insert( entry );

	/* Make sure that the entry type is set properly for all entries */
	if (entry->entryTypeOk(static_cast<const LQIO::DOM::Entry::Type>(dom->getEntryType())) == false) {
	    dom->runtime_error( LQIO::ERR_MIXED_ENTRY_TYPES );
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
	getDOM()->runtime_error( LQIO::ERR_MIXED_ENTRY_TYPES );
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
	owner()->getDOM()->runtime_error( LQIO::ERR_REFERENCE_TASK_FORWARDING, name().c_str() );
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
