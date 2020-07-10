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
 * $Id: entry.cc 13676 2020-07-10 15:46:20Z greg $
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <stdarg.h>
#include <string.h>
#include <lqio/error.h>
#include "errmsg.h"
#include "stack.h"
#include "model.h"
#include "entry.h"
#include "fpgoop.h"
#include "activity.h"
#include "actlist.h"
#include "call.h"
#include "task.h"
#include "processor.h"
#include "submodel.h"
#include "lqns.h"
#include "prob.h"
#include "variance.h"
#include "pragma.h"
#include "slice.h"
#include "entrythread.h"
#include "randomvar.h"

unsigned Entry::totalOpenArrivals   = 0;

unsigned Entry::max_phases	    = 0;

const char * Entry::phaseTypeFlagStr [] = { "Stochastic", "Determin" };


/* ------------------------ Constructors etc. ------------------------- */


Entry::Entry( LQIO::DOM::Entry* entryDOM, const unsigned id, const unsigned index )
    : _entryDOM(entryDOM),
      _phase(entryDOM && entryDOM->getMaximumPhase() > 0 ? entryDOM->getMaximumPhase() : 1 ),
      _total(),
      _nextOpenWait(0.0),			/* copy for delta computation	*/
      _startActivity(NULL),
      _entryId(id),
      _index(index+1),
      _entryType(ENTRY_NOT_DEFINED),
      _semaphoreType(entryDOM ? entryDOM->getSemaphoreFlag() : SEMAPHORE_NONE),
      _calledBy(NOT_CALLED),
      _throughput(0.0),
      _throughputBound(0.0),
      _callerList(),
      _interlock()
{
    const size_t size = _phase.size();
    for ( size_t p = 1; p <= size; ++p ) {
	std::string s;
	s = "123"[p-1];
	_phase[p].setEntry( this )
	    .setName( s );
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
Entry::operator==( const Entry& anEntry ) const
{
    return entryId() == anEntry.entryId();
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
    if ( isStandardEntry() ) {
	for_each ( _phase.begin(), _phase.end(), Predicate<Phase>( &Phase::check ) );
    } else if ( !isActivityEntry() ) {
	LQIO::solution_error( LQIO::ERR_ENTRY_NOT_SPECIFIED, name().c_str() );
    }

    if ( (isSignalEntry() || isWaitEntry()) && owner()->scheduling() != SCHEDULE_SEMAPHORE ) {
	LQIO::solution_error( LQIO::ERR_NOT_SEMAPHORE_TASK, owner()->name().c_str(), (isSignalEntry() ? "signal" : "wait"), name().c_str() );
    }

    /* Forwarding probabilities o.k.? */

    double sum = 0;
    for ( std::set<Call *>::const_iterator call = callList(1).begin(); call != callList(1).end(); ++call ) {
	sum += (*call)->forward() * (*call)->fanOut();
    }
    if ( sum < 0.0 || 1.0 < sum ) {
	LQIO::solution_error( LQIO::ERR_INVALID_FORWARDING_PROBABILITY, name().c_str(), sum );
    } else if ( sum != 0.0 && owner()->isReferenceTask() ) {
	LQIO::solution_error( LQIO::ERR_REF_TASK_FORWARDING, owner()->name().c_str(), name().c_str() );
    }
    return !io_vars.anError();
}



/*
 * Allocate storage for savedWait.  NOTE:  the output file parsing
 * routines cannot deal with variable sized arrays, so fix these
 * at MAX_PHASES.
 */

Entry&
Entry::configure( const unsigned nSubmodels )
{
    for_each ( _phase.begin(), _phase.end(), Exec1<NullPhase,const unsigned>( &NullPhase::configure, nSubmodels ) );
    _total.configure( nSubmodels );

    const unsigned n_e = Model::__entry.size() + 1;
    if ( n_e != _interlock.size() ) {
	_interlock.resize( n_e );
    }

    if ( isActivityEntry() && !isVirtualEntry() ) {

	/* Check reply type and set max phase. */

	Stack<const Activity *> activityStack( dynamic_cast<const Task *>(owner())->activities().size() );
	unsigned next_p = 1;
	double replies = _startActivity->aggregate2( this, 1, next_p, 1.0, activityStack, &Activity::aggregateReplies );
	if ( isCalledUsing( RENDEZVOUS_REQUEST ) ) {
	    if ( replies == 0.0 ) {
		//tomari: disable to allow a quorum use the default reply which
		//is after all threads completes exection.
		//LQIO::solution_error( ERR_REPLY_NOT_GENERATED, name().c_str() );	/* BUG 238 */
	    } else if ( fabs( replies - 1.0 ) > EPSILON ) {
		LQIO::solution_error( LQIO::ERR_NON_UNITY_REPLIES, replies, name().c_str() );
	    }
	}

	_startActivity->configure( nSubmodels );

	/* Compute overall service time for this entry */

	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() );
	entryStack.push( this );
	next_p = 1;
	_startActivity->aggregate( entryStack, 0, 0, 1, next_p, &Activity::aggregateServiceTime );
	entryStack.pop();

	_total.setServiceTime( for_each( _phase.begin(), _phase.end(), Sum<Phase,double>( &Phase::serviceTime ) ).sum() );
    }
    return *this;
}




/*
 * Recursively find all children and grand children from `this'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.  DirectPath is true if we got here by following the
 * call chain directly.
 */

unsigned
Entry::findChildren( CallStack& callStack, const bool directPath ) const
{
    unsigned max_depth = callStack.size();

    if ( isActivityEntry() ) {
	max_depth = max( max_depth, _phase[1].findChildren( callStack, directPath ) );    /* Always check because we may have forwarding */
	const unsigned size = dynamic_cast<const Task *>(owner())->activities().size();
	Stack<const AndForkActivityList *> forkStack( size ); 	// For matching forks/joins.
	Stack<const Activity *> activityStack( size );		// For checking for cycles.
	try {
	    max_depth = max( max_depth, _startActivity->findChildren( callStack, directPath, activityStack, forkStack ) );
	}
	catch ( const activity_cycle& error ) {
	    LQIO::solution_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, owner()->name().c_str(), error.what() );
	    max_depth = max( max_depth, error.depth() );
	}
	catch ( const bad_external_join& error ) {
	    LQIO::solution_error( ERR_EXTERNAL_SYNC, name().c_str(), owner()->name().c_str(), error.what() );
	    max_depth = max( max_depth, error.depth() );
	}
    } else {
	for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	    max_depth = max( max_depth, phase->findChildren( callStack, directPath ) );
	}
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
 * Allocate storage for oldSurgDelay.
 */

Entry&
Entry::initReplication( const unsigned n_chains )
{
    for_each ( _phase.begin(), _phase.end(), Exec1<Phase,const unsigned>( &Phase::initReplication, n_chains ) );
    return *this;
}



Entry&
Entry::resetInterlock()
{
    for ( Vector<InterlockInfo>::iterator i = _interlock.begin(); i != _interlock.end(); ++i ) {
	i->all = 0.;
	i->ph1 = 0.;
    }
    return *this;
}



/*
 * Follow and record a path of phase one calls.  If we are at the head
 * of a path, then all calls from the entry `current' regardless of
 * phase are used.  Otherwise, only phase 1 calls are used.
 */

unsigned
Entry::initInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls )
{
    /*
     * Check for cycles in graph.  Return if found.  Cycle catching
     * is done by the other version.  Someday, we might make this more
     * intelligent to compute the final prob. of following the arc.
     */

    if ( entryStack.find( this ) ) {
	return entryStack.size() + 1;
    }

    entryStack.push( this );

    /* Update interlock table */

    const Entry * rootEntry = entryStack.bottom();

    if ( rootEntry == this ) {
	_interlock[rootEntry->entryId()] = globalCalls;
    } else {
	const_cast<Entry *>(rootEntry)->_interlock[entryId()] += globalCalls;
	_interlock[rootEntry->entryId()] -= globalCalls;
    }

    const unsigned max_depth = followInterlock( entryStack, globalCalls );

    entryStack.pop();
    return max_depth;
}




Entry&
Entry::setEntryInformation( LQIO::DOM::Entry * entryInfo )
{
    /* Open arrival stuff. */
    if ( hasOpenArrivals() ) {
	setIsCalledBy( OPEN_ARRIVAL_REQUEST );
    }
    return *this;
}

Entry&
Entry::setDOM( unsigned p, LQIO::DOM::Phase* phaseInfo )
{
    if (phaseInfo == NULL) return *this;
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
    _total.setServiceTime( for_each( _phase.begin(), _phase.end(), Sum<Phase,double>( &Phase::serviceTime ) ).sum() );
    return *this;
}


/*
 * Set the entry type field.
 */

bool
Entry::setIsCalledBy(const requesting_type callType )
{
    if ( _calledBy != NOT_CALLED && _calledBy != callType ) {
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
		.configure( _total.myWait.size() );
	}
    }
    max_phases = max( max_phase, max_phases );		/* Set global value.	*/

    return *this;
}



/*
 * Return 1 if the entry has think time and zero othewise.
 */

bool
Entry::hasVariance() const
{
    if ( isStandardEntry() ) {
	return find_if( _phase.begin(), _phase.end(), Predicate<Phase>( &Phase::hasVariance ) ) != _phase.end();
    } else {
	return true;
    }
}



/*
 * Check entry type.  If entry is NOT defined, the set entry type.
 * Return 1 if types match or entry type not set.
 */

bool
Entry::entryTypeOk( const entry_type aType )
{
    if ( _entryType == ENTRY_NOT_DEFINED ) {
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
Entry::entrySemaphoreTypeOk( const semaphore_entry_type aType )
{
    if ( _semaphoreType == SEMAPHORE_NONE ) {
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

    return _startActivity->concurrentThreads( 1 );
}




/*
 * Set the throughput of this entry to value.  If this is an activity entry, then
 * we push the throughput to all activities reachable from this entry.
 * The throughput of an activity is the sum of the throughput from all calling entries.
 * Ergo, we have to push the entry's index to the activities.
 */

Entry&
Entry::setThroughput( const double value )
{
    _throughput = value;

    if ( flags.trace_replication || flags.trace_throughput ) {
	cout <<"Entry::throughput(): Task = "<<this->owner()->name()<<" ,Entry= "<<this->name()
	     <<" , Throughput="<<_throughput << endl;
    }

    if ( isActivityEntry() ) {
	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() );
	entryStack.push( this );
	unsigned next_p;
	_startActivity->aggregate( entryStack, 0, 0, 1, next_p, &Activity::setThroughput );
	entryStack.pop();
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
    if ( callDOMInfo == NULL ) return *this;
    setMaxPhase( p );

    _phase[p].rendezvous( toEntry, callDOMInfo );
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

double
Entry::rendezvous( const Entry * anEntry ) const
{
    return for_each( _phase.begin(), _phase.end(), Sum1<Phase,double,const Entry *>( &Phase::rendezvous, anEntry ) ).sum();
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
    if ( callDOMInfo == NULL ) return *this;

    setMaxPhase( p );

    _phase[p].sendNoReply( toEntry, callDOMInfo );
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not returns 0.
 */

double
Entry::sendNoReply( const Entry * anEntry ) const
{
    return for_each( _phase.begin(), _phase.end(), Sum1<Phase,double,const Entry *>( &Phase::sendNoReply, anEntry ) ).sum();
}



/*
 * Return the sum of all calls from the receiver during it's phase `p'.
 */


double
Entry::sumOfSendNoReply( const unsigned p ) const
{
    const std::set<Call *>& callList = _phase[p].callList();
    return for_each( callList.begin(), callList.end(), Sum<Call,double>( &Call::sendNoReply ) ).sum();
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
    anActivity->setEntry( this );
    return *this;
}


/*
 * Reference the value of calls to entry.  The entry must already
 * exist.  If not, returns 0.
 */

double
Entry::processorCalls() const
{
    return for_each( _phase.begin(), _phase.end(), Sum<Phase,double>( &Phase::processorCalls ) ).sum();
}



/*
 * Clear replication variables for this pass.
 */

Entry&
Entry::resetReplication()
{
    for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::resetReplication ) );
    return *this;
}



/*
 * Compute the coefficient of variation.
 */

double
Entry::computeCV_sqr() const
{
    const double sum_S = for_each( _phase.begin(), _phase.end(), Sum<Phase,double>( &Phase::elapsedTime ) ).sum();

    if ( !isfinite( sum_S ) ) {
	return sum_S;
    } else if ( sum_S > 0.0 ) {
	const double sum_V = for_each( _phase.begin(), _phase.end(), Sum<Phase,double>( &Phase::variance ) ).sum();
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
    const Task * aTask = dynamic_cast<const Task *>(owner());
    const unsigned ix = aTask->threadIndex( submodel, k );

    if ( isStandardEntry() || submodel == 0 || ix <= 1 ) {
	return _phase[p].waitExcept( submodel );			/* Elapsed time is by entry */
    } else {
	//To handle the case of a main thread of control with no fork join.
	return aTask->waitExcept( ix, submodel, p );
    }
}


/*
 * Return waiting time.  Normally, we exclude all of chain k, but with
 * replication, we have to include replicas-1 wait for chain k too.
 */

//REPL changes  REP N-R

double
Entry::waitExceptChain( const unsigned submodel, const unsigned k, const unsigned p ) const
{
    const Task * aTask = dynamic_cast<const Task *>(owner());
    const unsigned ix = aTask->threadIndex( submodel, k );

    if ( isStandardEntry() || ix <= 1 ) {
	return _phase[p].waitExceptChain( submodel, k );			/* Elapsed time is by entry */
    } else {
	//To handle the case of a main thread of control with no fork join.
	return aTask->waitExceptChain( ix, submodel, k, p );
    }
}



/*
 * Return utilization over all phases.
 */

double
Entry::utilization() const
{
    return for_each( _phase.begin(), _phase.end(), Sum<Phase,double>( &Phase::utilization ) ).sum();
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

unsigned
Entry::followInterlock( Stack<const Entry *>& entryStack, const InterlockInfo& globalCalls )
{
    unsigned max_depth = entryStack.size();

    if ( isActivityEntry() ) {
	max_depth = max( _startActivity->followInterlock( entryStack, globalCalls, 1 ), max_depth );
    } else {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    max_depth = max( _phase[p].followInterlock( entryStack, globalCalls, p ), max_depth );
	}
    }
    return max_depth;
}



/*
 * Recursively search from this entry to any entry on myServer.
 * When we pop back up the call stack we add all calling tasks
 * for each arc which calls myServer.  The task adder
 * will ignore duplicates.
 */

bool
Entry::getInterlockedTasks( Stack<const Entry *>& entryStack, const Entity * dstServer,
			    std::set<const Entity *>& interlockedTasks ) const
{
    bool found = false;

    if ( dstServer == owner() ) {

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

    const bool headOfPath = entryStack.size() == 0;
    const unsigned last_phase = headOfPath ? maxPhase() : 1;

    entryStack.push( this );
    if ( isStandardEntry() ) {
	for ( unsigned p = 1; p <= last_phase; ++p ) {
	    if ( _phase[p].getInterlockedTasks( entryStack, dstServer, interlockedTasks, last_phase ) ) {
		found = true;
	    }
	}
    } else if ( isActivityEntry() ) {
	found = _startActivity->getInterlockedTasks( entryStack, dstServer, interlockedTasks, last_phase );
    }
    entryStack.pop();

    if ( found && !headOfPath ) {
	interlockedTasks.insert( owner() );
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
    bool rc = isfinite( openWait() );
    for ( std::set<Call *>::const_iterator call = callerList().begin(); call != callerList().end(); ++call ) {
	rc = rc && ( !(*call)->hasSendNoReply() || isfinite( (*call)->wait() ) );
    }
    return rc;
}

const Entry&
Entry::insertDOMResults(double *phaseUtils) const
{
    double totalPhaseUtil = 0.0;

    /* Write the results into the DOM */
    const double throughput = this->throughput();		/* Used to compute utilization at activity entries */
    _entryDOM->setResultThroughput(throughput)
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
    _entryDOM->setResultUtilization(totalPhaseUtil)
	.setResultProcessorUtilization(processorUtilization())
	.setResultSquaredCoeffVariation(computeCV_sqr());

    /* Store activity phase data */
    if (isActivityEntry()) {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    const Phase& phase = _phase[p];
	    const double service_time = phase.elapsedTime();
	    _entryDOM->setResultPhasePServiceTime(p,service_time)
		.setResultPhasePVarianceServiceTime(p,phase.variance())
		.setResultPhasePProcessorWaiting(p,phase.queueingTime())
		.setResultPhasePUtilization(p,service_time * throughput);
	    /*+ BUG 675 */
	    if ( _entryDOM->hasHistogramForPhase( p ) || _entryDOM->hasMaxServiceTimeExceededForPhase( p ) ) {
		NullPhase::insertDOMHistogram( const_cast<LQIO::DOM::Histogram*>(_entryDOM->getHistogramForPhase( p )), phase.elapsedTime(), phase.variance() );
	    }
	    /*- BUG 675 */
	}
    }

    /* Do open arrival rates... */
    if ( hasOpenArrivals() ) {
	_entryDOM->setResultWaitingTime(openWait());
    }
    return *this;
}



/*
 * Debug...
 */

ostream&
Entry::printSubmodelWait( ostream& output, const unsigned offset ) const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	if ( offset ) {
	    output << setw( offset ) << " ";
	}
	output << setw(8-offset) ;
	if ( p == 1 ) {
	    output << name();
	} else {
	    output << " ";
	}
	output << " " << setw(1) << p << "  ";
	for ( unsigned j = 1; j <= _phase[p].myWait.size(); ++j ) {
	    output << setw(8) << _phase[p].myWait[j];
	}
	output << endl;
    }
    return output;
}

/* --------------------------- Dynamic LQX  --------------------------- */

Entry&
Entry::recalculateDynamicValues()
{
    for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::recalculateDynamicValues ) );
    _total.setServiceTime( for_each( _phase.begin(), _phase.end(), Sum<Phase,double>( &Phase::serviceTime ) ).sum() );
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
    if ( _entryDOM && _entryDOM->hasOpenArrivalRate() ) {
	if ( owner()->isReferenceTask() ) {
	    LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_OPEN_ARRIVALS, owner()->name().c_str(), name().c_str() );
	}
    }
    return *this;
}

/* --------------------------- Task Entries --------------------------- */

/*
 * Initialize processor waiting time, variance and priority
 */

TaskEntry&
TaskEntry::initProcessor()
{
    if ( isStandardEntry() ) {
	for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::initProcessor ) );
    }
    return *this;
}



/*
 * Set up waiting times for calls to subordinate tasks.
 */

TaskEntry&
TaskEntry::initWait()
{
    for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::initWait ) );
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
    const double util = isfinite( throughput() ) ? throughput() * serviceTime() : 0.0;

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
    _total.myVariance = 0.0;
    if ( isActivityEntry() ) {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    _phase[p].myVariance = 0.0;
	}

	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() );
	entryStack.push( const_cast<TaskEntry *>(this) );
	unsigned next_p;
	_startActivity->aggregate( entryStack, 0, 0, 1, next_p, &Activity::aggregateWait );
	entryStack.pop();
	_total.myVariance += for_each( _phase.begin(), _phase.end(), Sum<Phase,double>( &Phase::variance ) ).sum();
    } else {
	_total.myVariance += for_each( _phase.begin(), _phase.end(), ExecSum<Phase,double>( &Phase::computeVariance ) ).sum();
    }
    if ( flags.trace_variance && dynamic_cast<TaskEntry *>(this) != NULL ) {
	cout << "Variance(" << name() << ",p) ";
	for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	    cout << ( phase == _phase.begin() ? " = " : ", " ) << phase->variance();
	}
	cout << endl;
    }
    return *this;
}



/*
 * Calculate total wait for a particular submodel and save.  Return
 * the difference between this pass and the previous.
 */

TaskEntry&
TaskEntry::updateWait( const Submodel& aSubmodel, const double relax )
{
    const unsigned submodel = aSubmodel.number();
    if ( submodel == 0 ) throw logic_error( "TaskEntry::updateWait" );

    /* Open arrivals first... */

    if ( _nextOpenWait > 0.0 ) {
	under_relax( _openWait, _nextOpenWait, relax );
    }

    /* Scan calls to other task for matches with submodel. */

    _total.myWait[submodel] = 0.0;

    if ( isActivityEntry() ) {

	for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	    phase->myWait[submodel] = 0.0;
	}

	if ( flags.trace_activities ) {
	    cout << "--- AggreateWait for entry " << name() << " ---" << endl;
	}
	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() );
	entryStack.push( this );
	unsigned next_p;
	_startActivity->aggregate( entryStack, 0, submodel, 1, next_p, &Activity::aggregateWait );

	for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	    _total.myWait[submodel] += phase->myWait[submodel];
	}
	if ( flags.trace_delta_wait || flags.trace_activities ) {
	    cout << "--DW--  Entry(with Activities) " << name()
		 << ", submodel " << submodel << endl;
	    cout << "        Wait=";
	    for ( Vector<Phase>::const_iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
		cout << phase->myWait[submodel] << " ";
	    }
	    cout << endl;
	}

    } else {

	for ( Vector<Phase>::iterator phase = _phase.begin(); phase != _phase.end(); ++phase ) {
	    phase->updateWait( aSubmodel, relax );

	    if ( !phase->isPresent() && phase->myWait[submodel] > 0.0 ) {
		throw logic_error( "TaskEntry::updateWait" );
	    }

	    _total.myWait[submodel] += phase->myWait[submodel];
	}
    }

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
	_phase[p].myWait[submodel] += addend.mean();
    } else if  (addend.variance() > 0.0 ) { //two-phase quorum semantics. If the replying activity
	//is inside the quorum fork-join, then the difference in variance when calculating
	//phase 2 variance can be negative.

	_phase[p].myVariance += addend.variance();
    }

    if (flags.trace_quorum) {
	cout << "\nEntry::aggregate(): submodel=" << submodel <<", entry " << name() << endl;
	cout <<" addend.mean()=" << addend.mean() <<", addend.variance()="<<addend.variance()<< endl;
    }

    return *this;
}



/* BUG_1
 * Calculate total wait for a particular submodel and save.  Return
 * the difference between this pass and the previous.
 */

double
TaskEntry::updateWaitReplication( const Submodel& aSubmodel, unsigned & n_delta )
{
    double delta = 0.0;
    if ( isActivityEntry() ) {
	for_each( _phase.begin(), _phase.end(), Exec<Phase>( &Phase::resetReplication ) );

	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() );
	entryStack.push( this );
	unsigned next_p;
	_startActivity->aggregate( entryStack, 0, aSubmodel.number(), 1, next_p, &Activity::aggregateReplication );

    } else {
	delta = for_each( _phase.begin(), _phase.end(), ExecSum1<Phase,double,const Submodel&>( &Phase::updateWaitReplication, aSubmodel )).sum();
	n_delta += _phase.size();
    }
    return delta;
}



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



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

const Entry&
Entry::callsPerform( callFunc aFunc, const unsigned submodel, const unsigned k ) const
{
    const double rate = prVisit();

    Stack<const Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() );
    entryStack.push( this );

    if ( isActivityEntry() ) {
	/* since 'rate=prVisit()' is only for call::setvisit;
	 * for a virtual entry, the throughput of its corresponding activity
	 * is used to calculation entry throughput not the throughput of its owner task.
	 * the visit of a call equals rate * rendenzvous() normally;
	 * therefore, rate has to be set to 1.*/
	_startActivity->callsPerform( entryStack, 0, submodel, k, 1, aFunc, rate );
    } else {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    _phase[p].callsPerform( entryStack, 0, submodel, k, p, aFunc, rate );
	}
    }
    entryStack.pop();
    return *this;
}

/* -------------------------- Device Entries -------------------------- */

/*
 * Make a processor Entry.
 */

DeviceEntry::DeviceEntry( LQIO::DOM::Entry* domEntry, const unsigned id, Processor * aProc )
    : Entry(domEntry,id,aProc->nEntries()), myProcessor(aProc)
{
    aProc->addEntry( this );
}



DeviceEntry::~DeviceEntry()
{
    LQIO::DOM::Phase* phaseDom = _entryDOM->getPhase(1);
    const LQIO::DOM::ExternalVariable* serviceTime = phaseDom->getServiceTime();
    if ( serviceTime ) delete const_cast<LQIO::DOM::ExternalVariable *>(serviceTime);
    const LQIO::DOM::ExternalVariable* cv_square   = phaseDom->getCoeffOfVariationSquared();
    if ( cv_square ) delete const_cast<LQIO::DOM::ExternalVariable *>(cv_square);
    const LQIO::DOM::ExternalVariable* priority    = _entryDOM->getEntryPriority();
    if ( priority ) delete const_cast<LQIO::DOM::ExternalVariable *>(priority);
    delete _entryDOM;
    _entryDOM = 0;
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

    _phase[1].myWait[i] = time;
    _total.myWait[i] = time;
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
 * Set the entry owner to aTask.
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
    LQIO::DOM::Phase* phaseDom = _entryDOM->getPhase(1);
    phaseDom->setServiceTime(new LQIO::DOM::ConstantExternalVariable(service_time/dynamic_cast<const Processor *>(myProcessor)->rate()));
    setDOM(1, phaseDom );
    return *this;
}


DeviceEntry&
DeviceEntry::setCV_sqr( const double cv_square )
{
    LQIO::DOM::Phase* phaseDom = _entryDOM->getPhase(1);
    phaseDom->setCoeffOfVariationSquaredValue(cv_square);
    return *this;
}


DeviceEntry&
DeviceEntry::setPriority( const int priority )
{
    _entryDOM->setEntryPriority( new LQIO::DOM::ConstantExternalVariable( priority ) );
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


/*
 * Return the waiting time for group `g' phase `p'.
 */

double
DeviceEntry::updateWaitReplication( const Submodel&, unsigned&  )
{
    throw should_not_implement( "DeviceEntry::updateWaitReplication", __FILE__, __LINE__ );
    return 0.0;
}


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
    : TaskEntry( new LQIO::DOM::Entry(anActivity->getDOM()->getDocument(), anActivity->name().c_str()), 0, anActivity->owner()->nEntries() )
{
    owner( anActivity->owner() );
}


/*
 * Since we make it, we delete it.
 */

VirtualEntry::~VirtualEntry()
{
    delete _entryDOM;
    _entryDOM = 0;
}


/*
 * Set starting activity for this entry.
 */

Entry&
VirtualEntry::setStartActivity( Activity * anActivity )
{
    _startActivity = anActivity;
    return *this;
}

static bool
map_entry_name( const char * entry_name, Entry * & outEntry, bool receiver, const entry_type aType = ENTRY_NOT_DEFINED )
{
    bool rc = true;
    outEntry = Entry::find( entry_name );

    if ( !outEntry ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry_name );
	rc = false;
    } else if ( receiver && outEntry->isReferenceTaskEntry() ) {
	LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, outEntry->owner()->name().c_str(), entry_name );
	rc = false;
    } else if ( aType != ENTRY_NOT_DEFINED && !outEntry->entryTypeOk( aType ) ) {
	LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, entry_name );
    }

    return rc;
}

/* ----------------------- Accumulate Printing ------------------------ */

/*
 * Initialize record.
 */

CallInfoItem::CallInfoItem( const Entry * src, const Entry * dst )
    : source( src ), destination( dst )
{
    if ( src == 0 || dst == 0 ) throw logic_error( "CallInfoItem::CallInfoItem" );

    for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	phase[p] = 0;
    }
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
    if ( phase[1] && phase[1]->hasForwarding() ) {
	return true;
    } else {
	return false;
    }
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
 * Locate all calls generated by anEntry regardless of phase.
 * Create a collection so that the () operator can step over it.
 */

CallInfo::CallInfo( const Entry& anEntry, const unsigned callType )
    : _calls()
{
    if ( !anEntry.isStandardEntry() ) return;

    for ( unsigned p = 1; p <= anEntry.maxPhase(); ++p ) {
	const std::set<Call *>& callList = anEntry.callList( p );
	for ( std::set<Call *>::const_iterator call = callList.begin(); call != callList.end(); ++call ) {
	    if ( (*call)->isProcessorCall() || (*call)->dstEntry()->owner()->isProcessor() ) continue;

	    if ( (    (callType & Call::SEND_NO_REPLY_CALL) && (*call)->hasSendNoReply() )
		 || ( (callType & Call::FORWARDED_CALL) && (*call)->isForwardedCall() )
//		 || ( (callType & Call::FORWARDED_CALL) && (*call)->hasForwarding() )
		 || ( (callType & Call::RENDEZVOUS_CALL) && (*call)->hasRendezvous() && !(*call)->isForwardedCall() )
		 || ( (callType & Call::OVERTAKING_CALL) && (*call)->hasOvertaking() && !(*call)->isForwardedCall() )
		) {

		std::vector<CallInfoItem>::iterator item = find_if( _calls.begin(), _calls.end(), compare( (*call)->dstEntry() ) );
		if ( item == _calls.end() ) {
		    _calls.push_back( CallInfoItem( &anEntry, (*call)->dstEntry() ) );
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
Entry::create(LQIO::DOM::Entry* domEntry, unsigned int index )
{
    const char* entry_name = domEntry->getName().c_str();

    std::set<Entry *>::const_iterator nextEntry = find_if( Model::__entry.begin(), Model::__entry.end(), EQStr<Entry>( entry_name ) );
    if ( nextEntry != Model::__entry.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Entry", entry_name );
	return 0;
    } else {
	Entry * anEntry = new TaskEntry( domEntry, Model::__entry.size() + 1, index );
	Model::__entry.insert( anEntry );

	/* Make sure that the entry type is set properly for all entries */
	if (anEntry->entryTypeOk(static_cast<const entry_type>(domEntry->getEntryType())) == false) {
	    LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, entry_name );
	}

	/* Set field width for entry names. */

	return anEntry;
    }
}

void
Entry::add_call( const unsigned p, const LQIO::DOM::Call* domCall )
{
    /* Make sure this is one of the supported call types */
    if (domCall->getCallType() != LQIO::DOM::Call::SEND_NO_REPLY &&
	domCall->getCallType() != LQIO::DOM::Call::RENDEZVOUS &&
	domCall->getCallType() != LQIO::DOM::Call::QUASI_RENDEZVOUS) {
	abort();
    }

    LQIO::DOM::Entry* toDOMEntry = const_cast<LQIO::DOM::Entry*>(domCall->getDestinationEntry());
    const char* to_entry_name = toDOMEntry->getName().c_str();

    /* Internal Entry references */
    Entry * toEntry;

    /* Begin by mapping the entry names to their entry types */
    if ( !entryTypeOk(STANDARD_ENTRY) ) {
	LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, name().c_str() );
    } else if ( map_entry_name( to_entry_name, toEntry, true ) ) {
	if ( domCall->getCallType() == LQIO::DOM::Call::RENDEZVOUS) {
	    rendezvous( toEntry, p, domCall );
	} else if ( domCall->getCallType() == LQIO::DOM::Call::SEND_NO_REPLY ) {
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
set_start_activity (Task* newTask, LQIO::DOM::Entry* theDOMEntry)
{
    Activity* activity = newTask->findActivity(theDOMEntry->getStartActivity()->getName());
    Entry* realEntry = NULL;

    map_entry_name( theDOMEntry->getName().c_str(), realEntry, false, ACTIVITY_ENTRY );
    realEntry->setStartActivity(activity);
    activity->setEntry(realEntry);
}

/* ---------------------------------------------------------------------- */

/*
 * Find the entry and return it.
 */

/* static */ Entry *
Entry::find( const string& entry_name )
{
    std::set<Entry *>::const_iterator nextEntry = find_if( Model::__entry.begin(), Model::__entry.end(), EQStr<Entry>( entry_name ) );
    if ( nextEntry == Model::__entry.end() ) {
	return 0;
    } else {
	return *nextEntry;
    }
}
