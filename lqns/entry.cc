/*  -*- c++ -*-
 * $HeadURL: svn://192.168.2.10/lqn/trunk-V5/lqns/entry.cc $
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
 * $Id: entry.cc 11963 2014-04-10 14:36:42Z greg $
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <string>
#include <stdarg.h>
#include <string.h>
#include <cmath>
#include <algorithm>
#include <lqio/error.h>
#include "errmsg.h"
#include "cltn.h"
#include "stack.h"
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

set<Entry *, ltEntry> entry;

unsigned Entry::totalOpenArrivals   = 0;

unsigned Entry::max_phases	    = 0;

const char * Entry::phaseTypeFlagStr [] = { "Stochastic", "Determin" };


/* ------------------------ Constructors etc. ------------------------- */


Entry::Entry( LQIO::DOM::Entry* aDomEntry, const unsigned id, const unsigned index )
    : openWait(0),
      myDOMEntry(aDomEntry),
      nextOpenWait(0),
      myActivity(0),
      myMaxPhase(0),
      _entryId(id),
      _index(index+1),
      myType(ENTRY_NOT_DEFINED),
      mySemaphoreType(aDomEntry ? aDomEntry->getSemaphoreFlag() : SEMAPHORE_NONE),
      calledFlag(NOT_CALLED),
      myReplies(0),
      myThroughput(0.0),
      myThroughputBound(0.0)
{
    /* Allocate phases */

    phase.grow(MAX_PHASES);

    for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	phase[p].initialize( this, p );
    }
}


/*
 * Free storage allocated in wait.  Storage was allocated by layerize.c
 * by calling configure.
 */

Entry::~Entry()
{
    /* Zero reverse links */
	
    const unsigned n = myCallers.size();
    for ( unsigned i = 1; i <= n; ++i ) {
	myCallers[i] = 0;
    }
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

/*
 * Check entry data.
 */

void
Entry::check() const
{
    if ( isStandardEntry() ) {
		
	/* concordance between c, phase_flag */
		
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    phase[p].check(p);
	}
    } else if ( !isActivityEntry() ) {
	LQIO::solution_error( LQIO::ERR_ENTRY_NOT_SPECIFIED, name() );
    }
	
    if ( (isSignalEntry() || isWaitEntry()) && owner()->scheduling() != SCHEDULE_SEMAPHORE ) {
	LQIO::solution_error( LQIO::ERR_NOT_SEMAPHORE_TASK, owner()->name(), (isSignalEntry() ? "signal" : "wait"), name() );
    }
	
    /* Forwarding probabilities o.k.? */
	
    Sequence<Call *> nextCall( phase[1].callList() );
    Call * aCall;
	
    double sum = 0;
    while ( aCall = nextCall() ) {
	sum += aCall->forward() * aCall->fanOut();
    }
    if ( sum < 0.0 || 1.0 < sum ) {
	LQIO::solution_error( LQIO::ERR_INVALID_FORWARDING_PROBABILITY, name(), sum );
    } else if ( sum != 0.0 && owner()->isReferenceTask() ) {
	LQIO::solution_error( LQIO::ERR_REF_TASK_FORWARDING, owner()->name(), name() );
    }
}



/*
 * Allocate storage for savedWait.  NOTE:  the output file parsing
 * routines cannot deal with variable sized arrays, so fix these
 * at MAX_PHASES.
 */

void
Entry::configure( const unsigned nSubmodels, const unsigned max_p )
{
    for ( unsigned p = 1; p <= max_p; ++p ) {
	phase[p].configure( nSubmodels );
    }
	
    total.configure( nSubmodels );
	
    const unsigned n_e = entry.size() + 1;
    if ( n_e != _interlock.size() ) {
	_interlock.resize( n_e );
    }
	
    if ( isActivityEntry() && !isVirtualEntry() ) {
		
	/* Check reply type and set max phase. */
		
	Stack<const Activity *> activityStack( dynamic_cast<const Task *>(owner())->activities().size() ); 
	unsigned next_p = 1;
	double replies = myActivity->aggregate2( this, 1, next_p, 1.0, activityStack, &Activity::aggregateReplies );
	if ( isCalled() == RENDEZVOUS_REQUEST ) {
	    if ( replies == 0.0 ) {
		//tomari: disable to allow a quorum use the default reply which
		//is after all threads completes exection.
		//LQIO::solution_error( ERR_REPLY_NOT_GENERATED, name() );	/* BUG 238 */
	    } else if ( fabs( replies - 1.0 ) > EPSILON ) {
		LQIO::solution_error( LQIO::ERR_NON_UNITY_REPLIES, replies, name() );
	    }
	}
		
	myActivity->configure( nSubmodels, maxPhase() );
		
	/* Compute overall service time for this entry */
		
	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() ); 
	entryStack.push( this );
	next_p = 1;
	myActivity->aggregate( entryStack, 0, 0, 1, next_p, &Activity::aggregateServiceTime );
	entryStack.pop();
		
	double sum  = 0.0;
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    sum += phase[p].serviceTime();
	}
	total.setServiceTime( sum );
    }
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
	max_depth = max( max_depth, phase[1].findChildren( callStack, directPath ) );    /* Always check because we may have forwarding */
	const unsigned size = dynamic_cast<const Task *>(owner())->activities().size();
	Stack<const AndForkActivityList *> forkStack( size ); 	// For matching forks/joins.
	Stack<const Activity *> activityStack( size );		// For checking for cycles.
	const_cast<Entry *>(this)->myReplies = 0.0;
	try {
	    max_depth = max( max_depth, myActivity->findChildren( callStack, directPath, activityStack, forkStack ) );
	}
	catch ( activity_cycle& error ) {
	    LQIO::solution_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, owner()->name(), error.what() );
	    max_depth = max( max_depth, error.depth() );
	}
	catch ( bad_external_join& error ) {
	    LQIO::solution_error( ERR_EXTERNAL_SYNC, name(), owner()->name(), error.what() );
	    max_depth = max( max_depth, error.depth() );
	}
    } else {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    max_depth = max( max_depth, phase[p].findChildren( callStack, directPath ) );
	}
    }

    return max_depth;
}



/*
 * Type 1 throughput bounds.  Reference task think times will limit throughput
 */
		
void
Entry::initThroughputBound()
{
    const double t = elapsedTime() + owner()->thinkTime();
    if ( t > 0 ) {
	myThroughputBound = owner()->copies() / t;
    } else {
	myThroughputBound = 0.0;
    }
    throughput( myThroughputBound );		/* Push bound to entries/phases/activities */
}



/*
 * Allocate storage for oldSurgDelay.
 */

void
Entry::initReplication( const unsigned n_chains )
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	phase[p].initReplication( n_chains );
    }
}



Entry&
Entry::resetInterlock()
{
    for ( unsigned i = 0; i < _interlock.size(); ++i ) {
	_interlock[i].all = 0.;
	_interlock[i].ph1 = 0.;
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
	isCalled( OPEN_ARRIVAL_REQUEST );
    }
    return *this;
}

Entry& 
Entry::setDOM( unsigned ph, LQIO::DOM::Phase* phaseInfo )
{
    if (phaseInfo == NULL) return *this;
    setMaxPhase(ph);
    phase[ph].setDOM(phaseInfo);
    return *this;
}


/*
 * Set the service time for phase `phase' and total over all phases.
 */

Entry&
Entry::addServiceTime( const unsigned ph, const double value )
{
    if ( value == 0.0 ) return *this;

    setMaxPhase( ph );

    phase[ph].addServiceTime( value );
	
    double sum = 0.0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum += phase[p].serviceTime();
    }
    total.setServiceTime( sum );
    return *this;
}


/*
 * Set the entry type field.
 */

bool
Entry::isCalled(const requesting_type callType )
{
    if ( calledFlag != NOT_CALLED && calledFlag != callType ) {
	LQIO::solution_error( LQIO::ERR_OPEN_AND_CLOSED_CLASSES, name() );
	return false;
    } else {
	calledFlag = callType;
	return true;
    }
}


/*
 * mark whether the phase is present or not.  
 */

Entry&
Entry::setMaxPhase( const unsigned ph )
{
    myMaxPhase = max( myMaxPhase, ph );
    max_phases = max( myMaxPhase, max_phases );		/* Set global value.	*/

    return *this;
}



/*
 * Return 1 if any phase is deterministic.
 */

bool
Entry::hasDeterministicPhases() const
{
    for ( unsigned p = 1; p <= maxPhase(); p++ ) {
	if ( phaseTypeFlag(p) == PHASE_DETERMINISTIC ) return true;
    }
    return false;
}



/*
 * Return 1 if any phase is not exponential $(C^2_v \not= 1)$.
 */

bool
Entry::hasNonExponentialPhases() const
{
    for ( unsigned p = 1; p <= maxPhase(); p++ ) {
	if ( serviceTime(p) > 0 && CV_sqr(p) != 1.0 ) return true;
    }
    return false;
}


/*
 * Return 1 if the entry has think time and zero othewise.
 */

bool
Entry::hasThinkTime() const
{
    for ( unsigned p = 1; p <= maxPhase(); p++ ) {
	if ( phase[p].hasThinkTime() ) return true;
    }
    return false;
}



/*
 * Return 1 if the entry has think time and zero othewise.
 */

bool
Entry::hasVariance() const
{
    if ( isStandardEntry() ) {
	for ( unsigned p = 1; p <= maxPhase(); p++ ) {
	    if ( phase[p].hasVariance() ) return true;
	}
	return false;
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
    if ( myType == ENTRY_NOT_DEFINED ) {
	myType = aType;
	return true;
    } else {
	return myType == aType;
    }
}



/*
 * Check entry type.  If entry is NOT defined, the set entry type.
 * Return 1 if types match or entry type not set.
 */

bool
Entry::entrySemaphoreTypeOk( const semaphore_entry_type aType )
{
    if ( mySemaphoreType == SEMAPHORE_NONE ) {
	mySemaphoreType = aType;
    } else if ( mySemaphoreType != aType ) {
	LQIO::input_error2( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
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

    return myActivity->concurrentThreads( 1 );
}




/*
 * Set the throughput of this entry to value.  If this is an activity entry, then
 * we push the throughput to all activities reachable from this entry.
 * The throughput of an activity is the sum of the throughput from all calling entries.
 * Ergo, we have to push the entry's index to the activities.
 */

Entry&
Entry::throughput( const double value )
{
    myThroughput = value;

    if ( flags.trace_replication || flags.trace_throughput ) {
	cout <<"Entry::throughput(): Task = "<<this->owner()->name()<<" ,Entry= "<<this->name() 
	     <<" , Throughput="<<myThroughput << endl;
    }

    if ( isActivityEntry() ) {
	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() ); 
	entryStack.push( this );
	unsigned next_p;
	myActivity->aggregate( entryStack, 0, 0, 1, next_p, &Activity::setThroughput );
	entryStack.pop();
    }
    return *this;
}



/*
 * Set the value of calls to entry `toEntry', `phase'.  Retotal
 * total.
 */

Entry&
Entry::rendezvous( Entry * toEntry, const unsigned p, LQIO::DOM::Call* callDOMInfo )
{
    if ( callDOMInfo == NULL ) return *this;
    setMaxPhase( p );

    phase[p].rendezvous( toEntry, callDOMInfo );
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not, returns 0.
 */

double
Entry::rendezvous( const Entry * anEntry ) const
{
    double sum = 0.0;

    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum += phase[p].rendezvous( anEntry );
    }
    return sum;
}



/*
 * Return number of rendezvous to dstTask.  Used by class ijinfo.
 */

void
Entry::rendezvous( const Entity *dstTask, VectorMath<double>& calls ) const
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	calls[p] += phase[p].rendezvous( dstTask );
    }
}



/*
 * Set the value of send-no-reply calls to entry `toEntry', `phase'.
 * Retotal.
 */

Entry&
Entry::sendNoReply( Entry * toEntry, const unsigned p, LQIO::DOM::Call* callDOMInfo )
{
    if ( callDOMInfo == NULL ) return *this;

    setMaxPhase( p );

    phase[p].sendNoReply( toEntry, callDOMInfo );
    return *this;
}



/*
 * Reference the value of calls to entry.  The entry must
 * already exist.  If not returns 0.
 */

double
Entry::sendNoReply( const Entry * anEntry ) const
{
    return sendNoReply( anEntry, 0 );
}



/*
 * Return the sum of all calls from the receiver during it's phase `p'.
 */


double
Entry::sumOfSendNoReply( const unsigned p ) const
{
    Sequence<Call *> nextCall(phase[p].callList());
    const Call * toCall;

    double sum = 0.0;
    while ( toCall = nextCall() ) {
	sum += toCall->sendNoReply();
    }
    return sum;
}



/*
 * Store forwarding probability in call list.
 */

Entry&
Entry::forward( Entry * toEntry, LQIO::DOM::Call* call  )
{
    if ( !call ) return *this;

    setMaxPhase( 1 );
    phase[1].forward( toEntry, call );
    return *this;
}






/*
 * Set starting activity for this entry.
 */

Entry&
Entry::setStartActivity( Activity * anActivity )
{
    myActivity = anActivity;
    anActivity->setRootEntry( this );
    myMaxPhase = 1;
    return *this;
}


/*
 * Reference the value of calls to entry.  The entry must already
 * exist.  If not, returns 0.
 */

double
Entry::processorCalls() const
{
    double sum = 0.0;

    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum += processorCalls( p );
    }
    return sum;
}



/*
 * Clear replication variables for this pass.
 */

void
Entry::resetReplication()
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	phase[p].resetReplication();
    }
}



/*
 * Compute the coefficient of variation.
 */

double
Entry::computeCV_sqr() const
{
    double sum_S = 0.0;
    double sum_V = 0.0;

    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum_S += elapsedTime(p);
	sum_V += variance(p);
    }

    if ( !isfinite( sum_S ) ) {
	return sum_S;
    } else if ( sum_S > 0.0 ) {
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
	return phase[p].waitExcept( submodel );			/* Elapsed time is by entry */
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
	return phase[p].waitExceptChain( submodel, k );			/* Elapsed time is by entry */
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
    double sum = 0.0;

    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	sum += utilization(p);
    }
    return sum;
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
	slice[p].initialize( phase[p], dst );
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
	max_depth = max( myActivity->followInterlock( entryStack, globalCalls, 1 ), max_depth );
    } else {
	for ( unsigned p = 1; p <= myMaxPhase; ++p ) {
	    max_depth = max( phase[p].followInterlock( entryStack, globalCalls, p ), max_depth );
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
			    Cltn<const Entity *>& interlockedTasks ) const
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
	    if ( phase[p].getInterlockedTasks( entryStack, dstServer, interlockedTasks, last_phase ) ) {
		found = true;
	    }
	}
    } else if ( isActivityEntry() ) {
	found = myActivity->getInterlockedTasks( entryStack, dstServer, interlockedTasks, last_phase );
    }
    entryStack.pop();

    if ( found && !headOfPath ) {
	interlockedTasks += owner();
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
    bool rc = isfinite( openWait );

    Sequence<Call *> nextCall( callerList() );
    Call * aCall;

    while ( aCall = nextCall() ) {
	rc = rc && ( !aCall->hasSendNoReply() || isfinite( aCall->wait() ) );
    }
    return rc;
}

void
Entry::insertDOMResults(double *phaseUtils) const
{
    double totalPhaseUtil = 0.0;
	
    /* Write the results into the DOM */
    myDOMEntry->resetResultFlags();
    myDOMEntry->setResultThroughput(throughput())
	.setResultThroughputBound(throughputBound());
	
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	totalPhaseUtil += utilization(p);
	phaseUtils[p-1] += utilization(p);
		
	if ( !isActivityEntry() && phaseIsPresent(p) ) {
	    phase[p].insertDOMResults();
	}
    }		
	
    /* Store the utilization and squared coeff of variation */
    myDOMEntry->setResultUtilization(totalPhaseUtil)
	.setResultProcessorUtilization(processorUtilization())
	.setResultSquaredCoeffVariation(computeCV_sqr());
	
    /* Store activity phase data */
    if (isActivityEntry()) {	
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    myDOMEntry->setResultPhasePServiceTime(p,elapsedTime(p))
		.setResultPhasePVarianceServiceTime(p,variance(p))
		.setResultPhasePProcessorWaiting(p,queueingTime(p));
	    /*+ BUG 675 */
	    if ( myDOMEntry->hasHistogramForPhase( p ) ) {
		NullPhase::insertDOMHistogram( const_cast<LQIO::DOM::Histogram*>(myDOMEntry->getHistogramForPhase( p )), elapsedTime(p), variance(p) );
	    }
	    /*- BUG 675 */
	}
    }
	
    /* Do open arrival rates... */
    if (openArrivalRate() != 0.0) {
	myDOMEntry->setResultOpenWaitTime(openWait);
    }
	
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
	for ( unsigned j = 1; j <= phase[p].myWait.size(); ++j ) {
	    output << setw(8) << phase[p].myWait[j];
	}
	output << endl;
    }
    return output;
}

/* --------------------------- Dynamic LQX  --------------------------- */

void Entry::recalculateDynamicValues()
{
    double sum = 0.0;
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	phase[p].recalculateDynamicValues();
	sum += phase[p].serviceTime();
    }
	
    total.setServiceTime( sum );
    sanityCheckParameters();
}

void Entry::sanityCheckParameters()
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
    if ( myDOMEntry && myDOMEntry->hasOpenArrivalRate() ) {
	if ( this->owner()->isReferenceTask() ) {
	    LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_OPEN_ARRIVALS, this->owner()->name(), this->name() );
	}
    }
}

/* --------------------------- Task Entries --------------------------- */

/*
 * Set the entry owner to aTask.
 */

Entry&
TaskEntry::owner( const Entity * aTask )
{
    myTask = aTask;
    return *this;
}


/*
 * Initialize processor waiting time, variance and priority
 */

void
TaskEntry::initProcessor()
{
    if ( isStandardEntry() ) {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    phase[p].initProcessor();
	}
    }
}



/*
 * Set up waiting times for calls to subordinate tasks.  
 */

void
TaskEntry::initWait()
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	phase[p].initWait();
    }
}



/*
 * Reference the value of calls to entry.  The entry must already
 * exist.  If not, returns 0.
 */

double
TaskEntry::processorCalls( const unsigned p ) const
{
    return phase[p].processorCalls();
}



/*
 * Return utilization (not including "other" service).
 * For activity entries, serviceTime() was computed in configure(). 
 */

double
TaskEntry::processorUtilization() const
{
    const Processor * aProc = owner()->processor();
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

	return phase[p].queueingTime();

    } else {

	return 0.0;

    }
}



/*
 * Compute the variance for this entry.
 */

void
TaskEntry::computeVariance() 
{
    total.myVariance = 0.0;
    if ( isActivityEntry() ) {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    phase[p].myVariance = 0.0;
	}

	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() ); 
	entryStack.push( const_cast<TaskEntry *>(this) );
	unsigned next_p;
	myActivity->aggregate( entryStack, 0, 0, 1, next_p, &Activity::aggregateWait );
	entryStack.pop();

	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    total.myVariance += phase[p].variance();
	}
    } else {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    total.myVariance += phase[p].computeVariance();
	}
    }
    if ( flags.trace_variance && dynamic_cast<TaskEntry *>(this) ) {
	cout << "Variance(" << name() << ",p) ";
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    cout << ( p == 1 ? " = " : ", " ) << variance(p);
	}
	cout << endl;
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
    if ( submodel == 0 ) throw logic_error( "TaskEntry::updateWait" );

    /* Open arrivals first... */

    if ( nextOpenWait > 0.0 ) {
	under_relax( openWait, nextOpenWait, relax );
    }

    /* Scan calls to other task for matches with submodel. */
	
    total.myWait[submodel] = 0.0;

    if ( isActivityEntry() ) {

	for ( unsigned p = 1; p <= 2; ++p ) {
	    phase[p].myWait[submodel] = 0.0;
	}

	if ( flags.trace_activities ) {
	    cout << "--- AggreateWait for entry " << name() << " ---" << endl;
	}
	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() ); 
	entryStack.push( this );
	unsigned next_p;
	myActivity->aggregate( entryStack, 0, submodel, 1, next_p, &Activity::aggregateWait );

	for ( unsigned p = 1; p <= 2; ++p ) {
	    total.myWait[submodel] += phase[p].myWait[submodel];
	}
	if ( flags.trace_delta_wait || flags.trace_activities ) {
	    cout << "--DW--  Entry(with Activities) " << name() 
		 << ", submodel " << submodel << endl;
	    cout << "        Wait=";
	    for ( unsigned p = 1; p <= 2; ++p ) {
		cout << phase[p].myWait[submodel] << " ";
	    }
	    cout << endl;
	}
	    
    } else {

	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    phase[p].updateWait( aSubmodel, relax );

	    if ( !phaseIsPresent(p) && phase[p].myWait[submodel] > 0.0 ) {
		throw logic_error( "TaskEntry::updateWait" );
	    }

	    total.myWait[submodel] += phase[p].myWait[submodel];
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
	
	phase[p].myWait[submodel] += addend.mean();
    }
    else if  (addend.variance() > 0.0 ) { //two-phase quorum semantics. If the replying activity
	//is inside the quorum fork-join, then the difference in variance when calculating 
	//phase 2 variance can be negative.
    
	phase[p].myVariance += addend.variance();
   
	//phase[p].myVariance += addend.variance();

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

	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    phase[p].mySurrogateDelay = 0.0;
	}

	Stack<Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() ); 
	entryStack.push( this );
	unsigned next_p;
	myActivity->aggregate( entryStack, 0, aSubmodel.number(), 1, next_p, &Activity::aggregateReplication );

    } else {

	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    
	    delta   += phase[p].updateWaitReplication( aSubmodel );
		 
	    n_delta += 1;
	}
    }
    return delta;
}



/*
 */

Entry&
Entry::aggregateReplication( const Vector< VectorMath<double> >& addend )
{
    for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	phase[p].mySurrogateDelay += addend[p];
    }
    return *this;
}



/*
 * For all calls to aServer perform aFunc over the chains in nextChain.
 * See Call::setVisits and Call::saveWait
 */

void
Entry::callsPerform( callFunc aFunc, const unsigned submodel, const unsigned k ) const
{
    const double rate = prVisit();

    Stack<const Entry *> entryStack( dynamic_cast<const Task *>(owner())->activities().size() ); 
    entryStack.push( this );

    if ( isActivityEntry() ) {
	myActivity->callsPerform( entryStack, 0, submodel, k, 1, aFunc, rate );
    } else {
	for ( unsigned p = 1; p <= maxPhase(); ++p ) {
	    phase[p].callsPerform( entryStack, 0, submodel, k, p, aFunc, rate );
	}
    }
    entryStack.pop();
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
    LQIO::DOM::Phase* phaseDom = myDOMEntry->getPhase(1);
    const LQIO::DOM::ExternalVariable* serviceTime = phaseDom->getServiceTime();
    if ( serviceTime ) delete const_cast<LQIO::DOM::ExternalVariable *>(serviceTime);
    const LQIO::DOM::ExternalVariable* cv_square   = phaseDom->getCoeffOfVariationSquared();
    if ( cv_square ) delete const_cast<LQIO::DOM::ExternalVariable *>(cv_square);
    const LQIO::DOM::ExternalVariable* priority    = myDOMEntry->getEntryPriority();
    if ( priority ) delete const_cast<LQIO::DOM::ExternalVariable *>(priority);
    delete myDOMEntry;
    myDOMEntry = 0;
}

/*
 * Initialize processor waiting time, variance and priority
 */

void
DeviceEntry::initProcessor()
{
    throw should_not_implement( "DeviceEntry::initProcessor", __FILE__, __LINE__ );
}



/*
 * Initialize savedWait fields.
 */

void
DeviceEntry::initWait()
{
    const unsigned i  = owner()->submodel();
    const double time = serviceTime(1);

    phase[1].myWait[i] = time;
    total.myWait[i] = time;
}


/*
 * Initialize variance for the processor entry.  This value doesn't change during solution,
 * so computeVariance() at the entry level is a NOP.  However, we still need to initialize it.
 */

void
DeviceEntry::initVariance()
{
    phase[1].initVariance();
}


/*
 * Set the entry owner to aTask.
 */

Entry&
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
    LQIO::DOM::Phase* phaseDom = myDOMEntry->getPhase(1);
    phaseDom->setServiceTime(new LQIO::DOM::ConstantExternalVariable(service_time/dynamic_cast<const Processor *>(myProcessor)->rate()));
    setDOM(1, phaseDom );
    return *this;
}


DeviceEntry&
DeviceEntry::setCV_sqr( const double cv_square )
{
    LQIO::DOM::Phase* phaseDom = myDOMEntry->getPhase(1);
    phaseDom->setCoeffOfVariationSquaredValue(cv_square);
    return *this;
}


DeviceEntry&
DeviceEntry::setPriority( const int priority )
{
    myDOMEntry->setEntryPriority( new LQIO::DOM::ConstantExternalVariable( priority ) );
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
    : TaskEntry( new LQIO::DOM::Entry(anActivity->getDOM()->getDocument(), anActivity->name(), NULL), 0, anActivity->owner()->nEntries() )
{
    owner( anActivity->owner() );
}


/*
 * Since we make it, we delete it.
 */

VirtualEntry::~VirtualEntry()
{
    delete myDOMEntry;
    myDOMEntry = 0;
}


/*
 * Set starting activity for this entry.
 */

Entry&
VirtualEntry::setStartActivity( Activity * anActivity )
{
    myActivity = anActivity;
    myMaxPhase = 1;
    return *this;
}

static bool
map_entry_name( const char * entry_name, Entry * & outEntry, bool receiver, const entry_type aType = ENTRY_NOT_DEFINED )
{
    bool rc = true;
    outEntry   = Entry::find( entry_name );
	
    if ( !outEntry ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, entry_name );
	rc = false;
    } else if ( receiver && outEntry->isReferenceTaskEntry() ) {
	LQIO::input_error2( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, outEntry->owner()->name(), entry_name );
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
 * Clear record.
 */

CallInfoItem::~CallInfoItem()
{
    for ( unsigned p = 0; p <= MAX_PHASES; ++p ) {
	phase[p] = 0;
    }
}

/*
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

CallInfo::CallInfo( const Entry * anEntry, const unsigned callType )
    : index(1)
{
    if ( !anEntry->isStandardEntry() ) return;

    for ( unsigned p = 1; p <= anEntry->maxPhase(); ++p ) {
	Sequence<Call *> nextCall( anEntry->callList( p ) );
	const Call * aCall;

	while ( aCall = nextCall() ) {
	    if ( aCall->isProcessorCall() || aCall->dstEntry()->owner()->isProcessor() ) continue;

	    if ( (    (callType & Call::SEND_NO_REPLY_CALL) && aCall->hasSendNoReply() )
		 || ( (callType & Call::FORWARDED_CALL) && aCall->isForwardedCall() )
//		 || ( (callType & Call::FORWARDED_CALL) && aCall->hasForwarding() )
		 || ( (callType & Call::RENDEZVOUS_CALL) && aCall->hasRendezvous() && !aCall->isForwardedCall() ) 
		 || ( (callType & Call::OVERTAKING_CALL) && aCall->hasOvertaking() && !aCall->isForwardedCall() ) 
		) {

		Sequence<CallInfoItem *> nextItem( itemCltn );
		CallInfoItem * item;

		while ( (item = nextItem()) && item->dstEntry() != aCall->dstEntry() ) ;	/* Look for matching dst. */

		if ( !item ) {
		    item = new CallInfoItem( anEntry, aCall->dstEntry() );		// !change me!
		    itemCltn << item;
		} else if ( item->phase[p] ) {
		    if ( item->phase[p]->isForwardedCall() && aCall->hasRendezvous() ) {
			item->phase[p] = aCall;	/* Drop forward -- keep rnv */
			continue;
		    } else if ( item->phase[p]->hasRendezvous() && aCall->isForwardedCall() ) {
			continue;
		    } else {
			LQIO::internal_error( __FILE__, __LINE__, "CallInfo::CallInfo" );
		    }
		}
			
		item->phase[p] = aCall;
	    }
	}
    }
}


CallInfo::~CallInfo()
{ 
    itemCltn.deleteContents(); 
}


CallInfoItem *
CallInfo::operator()()
{
    if ( index <= itemCltn.sz ) {
	return itemCltn.ia[index++];
    } else {
	index = 1;
	return 0;
    }
}

/*----------------------------------------------------------------------*/
/*       Input processing.  Called from load.cc::prepareModel()         */
/*----------------------------------------------------------------------*/

Entry *
Entry::create(LQIO::DOM::Entry* domEntry, unsigned int index )
{
    const char* entry_name = domEntry->getName().c_str();
	
    set<Entry *,ltEntry>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( entry_name ) );
    if ( nextEntry != entry.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Entry", entry_name );
	return 0;
    } else {
	Entry * anEntry = new TaskEntry( domEntry, entry.size() + 1, index );
	entry.insert( anEntry );
	
	/* Make sure that the entry type is set properly for all entries */
	if (anEntry->entryTypeOk(static_cast<const entry_type>(domEntry->getEntryType())) == false) {
	    LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, domEntry->getName().c_str() );
	}
		
	/* Set field width for entry names. */
		
	return anEntry;
    }
}

void 
Entry::add_call( const unsigned p, LQIO::DOM::Call* domCall )
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
	LQIO::input_error2( LQIO::ERR_MIXED_ENTRY_TYPES, name() );
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
	LQIO::input_error2( LQIO::ERR_REF_TASK_FORWARDING, owner()->name(), name() );
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
    Activity* activity = newTask->findActivity(theDOMEntry->getStartActivity()->getName().c_str());
    Entry* realEntry = NULL;
	
    map_entry_name( theDOMEntry->getName().c_str(), realEntry, false, ACTIVITY_ENTRY );
    realEntry->setStartActivity(activity);
    activity->setRootEntry(realEntry);
}

/* ---------------------------------------------------------------------- */

/*
 * Find the entry and return it.  
 */

/* static */ Entry *
Entry::find( const string& entry_name )
{
    std::set<Entry *,ltEntry>::const_iterator nextEntry = find_if( entry.begin(), entry.end(), eqEntryStr( entry_name ) );
    if ( nextEntry == entry.end() ) {
	return 0;
    } else {
	return *nextEntry;
    }
}


static ostream&
entries_of_str( ostream& output, const Cltn<Entry *>& entryList )
{
    Sequence<Entry *> nextEntry(entryList);
    const Entry * anEntry;
    string aString;

    for ( unsigned i = 0; anEntry = nextEntry(); ++i ) {
	if ( i > 0 ) {
	    aString += ", ";
	}
	aString += anEntry->name();
    }
    output << aString;
    return output;
}


SRVNEntryListManip
print_entries( const Cltn<Entry *>& entryList )
{
    return SRVNEntryListManip( entries_of_str, entryList );
}


