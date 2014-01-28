/*  -*- c++ -*-
 * $HeadURL$
 *
 * Everything you wanted to know about a task, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 1994
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */


#include "dim.h"
#include <string>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <string.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include <lqio/labels.h>
#include "cltn.h"
#include "stack.h"
#include "fpgoop.h"
#include "errmsg.h"
#include "task.h"
#include "entry.h"
#include "activity.h"
#include "actlist.h"
#include "variance.h"
#include "server.h"
#include "ph2serv.h"
#include "multserv.h"
#include "processor.h"
#include "group.h"
#include "lqns.h"
#include "pragma.h"
#include "call.h"
#include "submodel.h"
#include "interlock.h"
#include "entrythread.h"
#include <iostream>

set<Task *,ltTask> task;

/* ------------------------ Constructors etc. ------------------------- */

/*
 * Create a task.
 */

Task::Task( LQIO::DOM::Task* domTask, const Processor * aProc, const Group * aGroup, Cltn<Entry *>* entries)
    : Entity( domTask, entries ),
      myDOMTask(domTask),
      myProcessor(aProc),
      myGroup(aGroup),
      myOverlapFactor(1.0),
      maxThreads(1)
{
    if ( entries ) {
	Sequence<Entry *> nextEntry( *entries );
	Entry * anEntry;
	while ( anEntry = nextEntry() ) {
	    anEntry->owner(this);
	}
    }

    myThreads.grow( 1 );

    myThreads[1] = 0;		// Initialize root thread.

}


/*
 * Destructor.
 */

Task::~Task()
{
    myProcessor = 0;			/* Nop - destructor problems.	*/

    myClientStation.deleteContents();
    myPrecedence.deleteContents();
    myActivityList.deleteContents();
    dead_activities.deleteContents();
    entryList.deleteContents();		/* Deletes processor calls */
}


void
Task::reset()
{
}

/* ----------------------- Abstract Superclass. ----------------------- */

/*
 * Return true if any entry or activity has a think time value.  If
 * so, extendModel() will then create an entry to a thinker device.
 */

bool
Task::hasThinkTime() const
{
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	if ( anEntry->hasThinkTime() ) return true;
    }
    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	if ( anActivity->hasThinkTime() ) return true;
    }

    return false;
}


/*
 *
 */

void
Task::check() const
{
    const Processor * aProcessor = processor();
    bool hasActivityEntry = false;

    /* Check prio/scheduling. */

    if ( !schedulingIsOk( validScheduling() ) ) {
	LQIO::solution_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED,
			      scheduling_type_str[(unsigned)domEntity->getSchedulingType()],
			      "task",
			      name() );
	domEntity->setSchedulingType(defaultScheduling());
    }
	
    if ( aProcessor && priority() != 0 && !aProcessor->hasPriorities() ) {
	LQIO::solution_error( LQIO::WRN_PRIO_TASK_ON_FIFO_PROC, name(), aProcessor->name() );
    }

    /* Check entries */

    Sequence<Entry *> nextEntry( entries() );
    const Entry *anEntry;

    while( anEntry = nextEntry() ) {
	anEntry->check();
	if ( anEntry->isActivityEntry() ) {
	    hasActivityEntry = true;
	}
	if ( anEntry->maxPhase() > 1 && isInfinite() ) {
	    LQIO::solution_error( WRN_MULTI_PHASE_INFINITE_SERVER, anEntry->name(), name(), anEntry->maxPhase() );
	}
    }

    if ( hasActivities() ) {
	if ( hasActivityEntry ) {

	    Sequence<Activity *> nextActivity( activities() );
	    const Activity * anActivity;

	    while ( anActivity = nextActivity() ) {
		if ( !anActivity->isSpecified() ) {
		    LQIO::solution_error( LQIO::ERR_ACTIVITY_NOT_SPECIFIED, name(), anActivity->name() );
		} else {
		    anActivity->check();
		}
	    }
	} else {
	    LQIO::solution_error( LQIO::ERR_NO_START_ACTIVITIES, name() );
	}

	Sequence<ActivityList *> nextActivityList(myPrecedence);
	ActivityList * anActivityList;

	while ( anActivityList = nextActivityList() ) {
	    anActivityList->check();
	}
    }
}


/*
 * Denote whether this station belongs to the open, closed, or mixed
 * models when performing the MVA solution.
 */

void
Task::configure( const unsigned nSubmodels )
{
    if ( nEntries() == 0 ) {
	LQIO::solution_error( LQIO::ERR_NO_ENTRIES_DEFINED_FOR_TASK, name() );
	return;
    }

    myClientChains.grow( nSubmodels );	/* Prepare chain vectors	*/
    myClientStation.grow( nSubmodels );	/* Prepare client cltn		*/
    for ( unsigned i = 1; i <= nSubmodels; ++i ) {
	myClientStation[i] = 0;
    }

    Entity::configure( nSubmodels );

    if ( openArrivalRate() > 0.0 ) {
	attributes.open_model = 1;
    }

    /* Configure the threads... */

    for ( unsigned i = 2; i <= myThreads.size(); ++i ) {
	myThreads[i]->index = nEntries() + i - 1;
	myThreads[i]->check();
    }
}



/*
 * Recursively find all children and grand children from `father'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.  directPath is true if we are following the call
 * chain directly.  Otherwise findChildren is being used to bump
 * levels lower (if necessary).
 */

unsigned
Task::findChildren( CallStack& callStack, const bool directPath ) const
{
    unsigned max_depth = Entity::findChildren( callStack, directPath );
    const Entry * dstEntry = callStack.top() ? callStack.top()->dstEntry() : 0;

    /* Chase calls from srcTask. */

    Sequence<Entry *> nextEntry(entries());
    Entry *anEntry;

    while ( anEntry = nextEntry() ) {
	max_depth = max( max_depth, anEntry->findChildren( callStack, directPath && (anEntry == dstEntry) ) );
    }

    return max_depth;
}



/*
 * Initialize the processor for all entries and activities.
 */

void
Task::initProcessor()
{
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->initProcessor();
    }

    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	anActivity->initProcessor();
    }
}


/*
 * Initialize waiting time at my entries.  Also indicate whether the
 * variance calculation should take place.  If any lower level servers
 * have variance, or if I have multiple entries, then we should employ
 * the variance calculation.
 */

Task&
Task::initWait()
{
    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	anActivity->initWait();
    }

    Entity::initWait();

    return *this;
}



/*
 * Derive population based on who calls me.
 */

Task&
Task::initPopulation()
{
    Cltn<const Task *> sources;		/* Cltn of tasks already visited. */

    myPopulation = countCallers( sources );

    if ( isInClosedModel() && ( myPopulation == 0 || !isfinite( myPopulation ) ) ) {
	LQIO::solution_error( ERR_BOGUS_COPIES, myPopulation, name() );
    }
    return *this;
}



/*
 * Initialize throughput bounds.  Must be done after initWait.
 */

Task&
Task::initThroughputBound()
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	anEntry->initThroughputBound();
    }
    return *this;
}



/*
 * Allocate storage for oldSurgDelay (used by Newton Raphson
 * iteration.  This step must be done AFTER we have the chain
 * information.  Note -- arrays must be dimensioned on the LARGEST
 * chain number that goes to a particular client, and NOT on the
 * number of chains that visit that client.
 */

Task&
Task::initReplication( const unsigned n_chains )
{
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;

    while( anEntry = nextEntry() ) {
	anEntry->initReplication( n_chains );
    }

    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	anActivity->initReplication( n_chains );
    }
    return *this;
}



/*
 * Initialize interlock table.
 */

Task&
Task::initInterlock()
{
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;

    Stack<const Entry *> entryStack( io_vars.n_tasks + 2 );
    while( anEntry = nextEntry() ) {
	InterlockInfo calls(1.0,1.0);

	anEntry->initInterlock( entryStack, calls );
    }
    return *this;
}



/*
 * Initialize populations.
 */

Task&
Task::initThreads()
{
    maxThreads = 1;
    if ( hasThreads() ) {
	Sequence<Entry *> nextEntry( entries() );
	Entry * anEntry;

	while ( anEntry = nextEntry() ) {
	    maxThreads = max( anEntry->concurrentThreads(), maxThreads );
	}
    }
    if ( maxThreads > nThreads() ) throw logic_error( "Task::initThreads" );
    return *this;
}



/*
 * Set the processor for this task.  Setting it twice is probably an
 * error...  We can also remove processors.
 */

Entity&
Task::processor( Processor * aProcessor )
{
    if ( !aProcessor && myProcessor ) {
	aProcessor = const_cast<Processor *>(myProcessor);
	aProcessor->removeTask( this );
	myProcessor = 0;
    } else if ( aProcessor != myProcessor ) {
	if ( myProcessor != 0 ) throw logic_error( "Task::processor" );
	myProcessor = aProcessor;
	aProcessor->addTask( this );
    }
    return *this;
}



/*
 * Locate the destination anEntry in the list of destinations.
 */

Activity *
Task::findActivity( const char * name ) const
{
    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity = 0;

    while ( ( anActivity = nextActivity() ) && strcmp( anActivity->name(), name ) != 0 );

    return anActivity;
}



/*
 * Locate the activity, and if not found create a new one.
 */

Activity *
Task::findOrAddActivity( const char * name )
{
    Activity * anActivity = findActivity( name );

    if ( !anActivity ) {
	anActivity = new Activity( this, name );
	myActivityList << anActivity;
    }

    return anActivity;
}



/*
 * Locate the activity, and if not found create a new one.
 */

Activity *
Task::findOrAddPsuedoActivity( const char * name )
{
    Activity * anActivity = findActivity( name );

    if ( !anActivity ) {
	anActivity = new PsuedoActivity( this, name );
	myActivityList << anActivity;
    }

    return anActivity;
}



void
Task::addPrecedence( ActivityList * aPrecedence )
{
    myPrecedence << aPrecedence;
}

/*
 * Clear replication variables for this pass.
 */

void
Task::resetReplication()
{
    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	anActivity->resetReplication();
    }

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->resetReplication();
    }
}


/*
 * Return one if any of the entries on the receiver is called
 * and zero otherwise.
 */

bool
Task::isCalled() const
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( anEntry->isCalled() ) return true;
    }
    return false;
}



/*
 * Returns the initial depth (0 or 1) if this entity is a root of a
 * call graph.  Returns -1 otherwise.  Used by the topological sorter.
 */

int
Task::rootLevel() const
{
    int level = -1;
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	const requesting_type callType = anEntry->isCalled();
	switch ( callType ) {
	case RENDEZVOUS_REQUEST:
	case SEND_NO_REPLY_REQUEST:
	    return -1;	/* Non-root task. 		*/

	case OPEN_ARRIVAL_REQUEST:
	    level = 1;	/* Root task, but at lower level */
	    break;		/*  */

	case NOT_CALLED:
	    break;		/* No operation.		*/
	}
    }
    return level;
}



/*
 * Count number of calling tasks(!) and return.
 */

unsigned
Task::nClients() const
{
    Cltn<Task *> callingTasks;

    clients( callingTasks );

    return callingTasks.size();
}



/*
 * Count number of Serving tasks(!) and return.
 * May have to take current partition into account.
 */

unsigned
Task::nServers() const
{
    Cltn<Entity *> calledTasks;

    return servers( calledTasks );
}


/*
 * Store all tasks and processors called by this task in `calledTasks'.
 */

unsigned
Task::servers( Cltn<Entity *> & calledTasks ) const
{
    TaskCallList nextCall( this );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	Entity * anEntity = const_cast<Entity *>(aCall->dstTask());
	calledTasks += anEntity;
    }

    return calledTasks.size();
}



/*
 * Store all tasks and processors called by this task in `calledTasks'
 * that are also found in includeOnly.
 */

unsigned
Task::servers(	Cltn<Entity *> & calledTasks, const Cltn<Entity *> & includeOnly ) const
{
    TaskCallList nextCall( this );
    const Call * aCall;
    while ( aCall = nextCall() ) {
	Entity * anEntity = const_cast<Entity *>(aCall->dstTask());

	if ( includeOnly.find( anEntity ) ) {
	    calledTasks += anEntity;
	}
    }

    return calledTasks.size();
}



/*
 * This function locates all unique calling tasks to the receiver.
 * Tasks that are located are added to the collection `reject' so
 * that they are only counted once.  If we hit a multi-server or an
 * infinite server, we locate their sourcing tasks in order to
 * determine the proper population levels.  Multi-servers are treated
 * a little specially in that they limit the number of customers that
 * can be seen.  Note that we need to know replication information
 * in order to get the customer levels right for multiservers.
 *
 * Use doubles to propogate infinity.  We'll abort on the cast to int
 * if we hit infinity.
 */

double
Task::countCallers( Cltn<const Task *> & reject )
{
    double sum = 0;

    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( openArrivalRate() > 0 ) {
	    isInOpenModel( true );
	    if ( isInfinite() ) {
		sum = get_infinity();
	    }
	}

	Sequence<Call *> nextCall( anEntry->callerList() );
	Call * aCall;
	while ( aCall = nextCall() ) {
	    Task * aTask = const_cast<Task *>(aCall->srcTask());

	    if ( aCall->hasRendezvous() ) {

		if ( reject.find( aTask ) ) continue;
		reject << aTask;

		double delta = 0.0;
		if ( aTask->isInfinite() ) {
		    delta = aTask->countCallers( reject );
		    if ( isfinite( delta ) && delta > 0 ) {
			isInClosedModel( true );
			aTask->isInClosedModel( true );
		    } else {
			isInOpenModel( true );
		    }
		} else if ( aTask->isMultiServer() ) {
		    delta = aTask->countCallers( reject );
		    isInClosedModel( true );
		    aTask->isInClosedModel( true );
		} else {
		    delta = static_cast<double>(aTask->copies());
		    isInClosedModel( true );
		    aTask->isInClosedModel( true );
		}
		if ( isfinite( sum ) ) {
		    sum += delta * static_cast<double>(fanIn( aTask ));
		}

	    } else if ( aCall->hasSendNoReply() ) {

		isInOpenModel( true );

	    }
	}
    }

    if ( !isInfinite() && (sum > copies() || isInOpenModel() || !isfinite( sum ) || hasSecondPhase() ) ) {
	sum = static_cast<double>(copies());
    } else if ( isInfinite() && hasSecondPhase() && sum > 0.0 ) {
	sum = 100000;		/* Should be a pragma. */
    }
    return sum;
}



/*
 * Return true is my scheduling == sched.
 */

bool
Task::HOL_Scheduling() const
{
    return scheduling() == SCHEDULE_HOL;
}



/*
 * Return true if I need to use SCHEDULE_PPR.
 * We use a heuristic: if all the clients and the server share
 * the same processor, we use PPR scheduling, otherwise we
 * use FIFO scheduling.
 */

bool
Task::PPR_Scheduling() const
{
    if ( scheduling() == SCHEDULE_PPR ) return true;

    const Processor * aProcessor = processor();

    if ( !aProcessor || aProcessor->scheduling() != SCHEDULE_PPR ) return false;

    /*
     * Look for all sourcing tasks.
     */

    Cltn<const Task *> sources;

    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	Sequence<Call *> nextCall( anEntry->callerList() );
	Call * aCall;
	while ( aCall = nextCall() ) {
	    sources += aCall->srcTask();
	}
    }

    /*
     * If all tasks are on the same processor, then use PPR, otherwise
     * use FIFO
     */

    Sequence<const Task *> nextTask( sources );
    const Task * aTask;
    while ( aTask = nextTask() ) {
	if ( aTask->processor() != aProcessor ) return false;
    }
    return true;
}



/*
 * Client creation is relatively straight forward.  We simply create a
 * Infinite_Server and let-er rip. More sophistication can be added later if we
 * have a collection of clients running on a unique processor (then create a
 * FCFS_Server).
 */

Server *
Task::makeClient( const unsigned n_chains, const unsigned submodel )
{
    initReplication( n_chains );

    Server * aStation = new Client( nEntries(), n_chains, maxPhase() );

    myClientStation[submodel] = aStation;
    return aStation;
}




/*
 * Check results for sanity.
 */


void
Task::sanityCheck() const
{
    Entity::sanityCheck();

    bool rc = true;

    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	rc = rc && anEntry->checkDroppedCalls();
    }
    if ( !rc ) {
	LQIO::solution_error( LQIO::ADV_MESSAGES_DROPPED, name() );
    }
}



/*
 * Set the visit ratios from ...
 */

const Task&
Task::callsPerform( callFunc aFunc, const unsigned submodel ) const
{
    const ChainVector& aChain = myClientChains[submodel];

    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;

    unsigned ix = 1;
    while ( ix <= aChain.size() ) {
	unsigned k = aChain[ix++];

	while ( anEntry = nextEntry() ) {
	    anEntry->callsPerform( aFunc, submodel, k );
	}
	for ( unsigned j = 2; j <= myThreads.size(); ++j ) {
	    k = aChain[ix++];
	    myThreads[j]->callsPerform( aFunc, submodel, k );

	}
    }
    return *this;
}



/*
 * Set the visit ratios from ...
 */

const Task&
Task::openCallsPerform( callFunc aFunc, const unsigned submodel ) const
{
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->callsPerform( aFunc, submodel );
    }
    for ( unsigned j = 2; j <= myThreads.size(); ++j ) {
	myThreads[j]->callsPerform( aFunc, submodel );
    }
    return *this;
}



/*
 * Return idle time.  Compensate for threads.
 */
double
Task::thinkTime( const unsigned submodel, const unsigned k ) const
{
    if ( submodel == 0 || !hasThreads() ) {
	return Entity::thinkTime();
    } else {
	const unsigned ix = threadIndex( submodel, k );
	if ( ix == 1 ) {
	    return Entity::thinkTime();
	} else {
	    return myThreads[ix]->thinkTime();
	}
    }
}



Task&
Task::computeVariance()
{
    /* compute variance at activities before the entries because the entries aggregate the values. */

    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	anActivity->computeVariance();
    }

    Entity::computeVariance();
    return *this;
}



/*
 * Compute change in waiting times for this task.
 */

Task&
Task::updateWait( const Submodel& aSubmodel, const double relax )
{
    /* Do updateWait for each activity first. */

    if ( hasActivities() ) {
	Sequence<Activity *> nextActivity( activities() );
	Activity * anActivity;

	while ( anActivity = nextActivity() ) {
	    anActivity->updateWait( aSubmodel, relax );
	}
    }

    /* Entry updateWait for activity entries will update waiting times. */

    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	anEntry->updateWait( aSubmodel, relax );
    }

    /* Now recompute thread idle times */

    for ( unsigned i = 2; i <= myThreads.size(); ++i ) {
	myThreads[i]->setIdleTime( relax );
    }

    return *this;
}



/*
 * Compute change in waiting times for this task.
 */

double
Task::updateWaitReplication( const Submodel& aSubmodel, unsigned & n_delta )
{
    double delta = 0.0;

    /* Do updateWait for each activity first. */

    if ( hasActivities() ) {
	Sequence<Activity *> nextActivity( activities() );
	Activity * anActivity;

	while ( anActivity = nextActivity() ) {
	    delta   += anActivity->updateWaitReplication( aSubmodel );
	    n_delta += 1;
	}
    }

    /* Entry updateWait for activity entries will update waiting times. */

    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	delta += anEntry->updateWaitReplication( aSubmodel, n_delta );
    }

    return delta;
}


/*
 * Dynamic Updates / Late Finalization
 * In order to integrate LQX's support for model changes we need to
 * have a way of re-calculating what used to be static for all
 * dynamically editable values
 */

void
Task::recalculateDynamicValues()
{
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	anEntry->recalculateDynamicValues();
    }
}

/*----------------------------------------------------------------------*/
/*                       Threads (subthreads)                           */
/*----------------------------------------------------------------------*/

/*
 * Return the number Threads for this task.
 */

unsigned
Task::nThreads() const
{
    return max( 1, myThreads.size() );
}



/*
 * Return the thread index for chain k.  See Submodel::makeChains().
 */

unsigned
Task::threadIndex( const unsigned submodel, const unsigned k ) const
{
    if ( k == 0 ) {
	return 0;
    } else if ( nThreads() == 1 ) {
	return 1;
    } else {
	unsigned ix = myClientChains[submodel].find( k );
	if ( replicas() > 1 ) {
	    return (ix - 1) % nThreads() + 1;
	} else {
	    return ix;
	}
    }
}



/*
 * Return the waiting time for all submodels except submodel for phase
 * `p'.  If this is an activity entry, we have to return the chain k
 * component of waiting time.  Note that if submodel == 0, we return
 * the elapsedTime().  For servers in a submodel, submodel == 0; for
 * clients in a submodel, submodel == aSubmodel.number().
 */

double
Task::waitExcept( const unsigned ix, const unsigned submodel, const unsigned p ) const
{
    return myThreads[ix]->waitExcept( submodel, 0, p );		// k is ignored anyway...
}



/*
 * Return the waiting time for all submodels except submodel for phase
 * `p'.  If this is an activity entry, we have to return the chain k
 * component of waiting time.  Note that if submodel == 0, we return
 * the elapsedTime().  For servers in a submodel, submodel == 0; for
 * clients in a submodel, submodel == aSubmodel.number().
 */

double
Task::waitExceptChain( const unsigned ix, const unsigned submodel, const unsigned k, const unsigned p ) const
{
    return myThreads[ix]->waitExceptChain( submodel, k, p );
}



/*
 * Go through all the chains for the client and generate overlap factors.
 * See (8) [Mak].
 */

void
Task::forkOverlapFactor( const Submodel& aSubmodel, VectorMath<double>* of ) const
{
    const ChainVector& chain( myClientChains[aSubmodel.number()] );
    const unsigned n = chain.size();

    for ( unsigned i = 1; i <= n; ++i ) {
	for ( unsigned j = 1; j <= n; ++j ) {
	    of[chain[i]][chain[j]] = overlapFactor( i, j );
	}
    }

    if ( flags.trace_forks ) {
	printOverlapTable( cout, chain, of );
    }
}



/*
 * Compute and the overlap factor probability.  See Mak.  Need to make
 * multi-server version somwhat smarter.
 */

double
Task::overlapFactor( const unsigned i, const unsigned j ) const
{
    double theta = 0.0;


    if ( i > nThreads() || j > nThreads() ) { 		/* BUG_1 */

	/* i and j belong to different replicas -- ergo they always overlap. */

	return 1.0;

    } else if ( i == 1 || j == 1 || i == j
		|| myThreads[i]->isAncestorOf( myThreads[j] )
		|| myThreads[i]->isDescendentOf( myThreads[j] ) ) {

	/*
	 * Thread 1 is special (i.e., the thread of the
	 * entry/task, and by definition does not overlap any
	 * sub-threads within a task.  Further, threads which
	 * are descendents of other threads don't overlap
	 * either.
	 */

	if ( population() == 1 ) {
	    theta = 0.0;
	} else {
	    theta = 1.0;
	}

    } else {
	Probability pr = 0.0;

	/* Constant propogation */

	const double x_i = myThreads[i]->estimateCDF().mean();
	const double x_j = myThreads[j]->estimateCDF().mean();

	/* ---- Overlap probabilities ---- */

	if ( x_i == 0.0 || x_j == 0.0 ) {		/* Degeneate case */
	    pr = 0.0;

	} else if ( myThreads[i]->isSiblingOf(  myThreads[j] ) ) {
	    pr = 1.0;

	} else {					/* Partial overlap */

	    const Exponential start_i = myThreads[i]->startTime();
	    const Exponential start_j = myThreads[j]->startTime();
	    const Exponential end_i   = start_i + *myThreads[i];
	    const Exponential end_j   = start_j + *myThreads[j];

	    pr = ( 1.0 - ( Pr_A_lt_B( end_j, start_i ) +
			   Pr_A_lt_B( end_i, start_j ) ) );	/* Eqn 6, (8) */
	}

	if ( pr > 0.0 ) {

	    /* ---- Overlap interval ---- */

	    double d_ij = min( *myThreads[i], *myThreads[j] );

	    if ( pragma.getThreads() != MAK_LUNDSTROM_THREADS && myThreads[i]->elapsedTime() != 0 ) {
		d_ij = d_ij *  (myThreads[j]->elapsedTime() / myThreads[i]->elapsedTime());			// tomari quorum BUG 257
	    }

	    /* ---- Final overal factor ---- */

	    theta = pr * d_ij / x_i;

	    if ( pragma.getThreads() == HYPER_THREADS ) {
		theta *= (myThreads[j]->elapsedTime() / myThreads[i]->elapsedTime());
		const double inflation = myThreads[j]->elapsedTime() * throughput();
		if ( inflation ) {
		    theta /= inflation;
		}
	    }

	    /* Compensate for multiservers */

	    if ( population() > 1 ) {
		theta = (theta + (population() - 1.0)) / population();
	    }

	} else {
	    theta = 0.0;
	}
    }

    return theta * myOverlapFactor;	/* Scale to parent value */
}

/*----------------------------------------------------------------------*/
/*                           Synchronization                            */
/*----------------------------------------------------------------------*/

bool
Task::hasForks() const
{
    return myThreads.size() > 1;
}


/*
 * Return true if two entries synchronize on a join (i.e., and external join).
 */

bool
Task::hasSynchs() const
{
    return false;
}



/*
 * Go through all the chains for the client and generate overlap factors.
 * See (8) [Mak].
 */

void
Task::joinOverlapFactor( const Submodel& aSubmodel, VectorMath<double>* of ) const
{
#if 0
    Sequence<AndJoinActivityList *> nextJoin( myJoin );
    AndJoinActivityList * aJoin;
    while ( aJoin = nextJoin() ) {
	aJoin->overlapFactor( aSubmodel, of );
    }
#endif
}



#if HAVE_LIBGSL
//to model the delayed threads in a task with a quorum join.
//quorumAndJoinList->orgForkList;
//will be converted to:
//quorumAndJoinList->newAndForkList;
//newAndJoinList->finalAndForkList;
//The activities of orgForkList are added to newAndForkList and newAndJoinList.
int
Task::expandQuorumGraph()
{
    int anError = false;
    Cltn<ActivityList *> joinLists;
    Activity * localQuorumDelayActivity;
    Activity * finalActivity;
    Activity * anActivity;
    ActivityList * nextJoinList;
    int quorumListNumber = 0;

    //Get Join Lists
    Sequence<Activity *> nextActivity(activities());
    for (  ; (anActivity = nextActivity()) && (anActivity->outputTo());   ) {
	joinLists.findOrAdd(anActivity->outputTo());
    }
    // cout <<"\njoinLists.size () = " << joinLists.size() << endl;
    Sequence<ActivityList *> nextList(joinLists);

    while (nextJoinList = nextList() ) {
	AndForkActivityList *  newAndForkList = NULL;
	AndJoinActivityList *  newAndJoinList = NULL;
	AndJoinActivityList *  quorumAndJoinList = NULL;
	ActivityList * orgForkList = NULL;

	quorumAndJoinList = dynamic_cast<AndJoinActivityList *>(nextJoinList);
	if (!quorumAndJoinList) { continue; }

        if ( quorumAndJoinList->hasQuorum() ) {

	    //check that there is no replying activity inside he quorum
	    //fork-join list since we cannot solve for a quorum with a
	    //two-phase semantics.
////////////////////////////////////////
	    Cltn<Activity *>   cltnQuorumJoinActivities = dynamic_cast<ForkJoinActivityList *>(quorumAndJoinList)->getMyActivityList();
	    Sequence<Activity *> nextQuorumJoinActivity(cltnQuorumJoinActivities);
	    while (anActivity=nextQuorumJoinActivity()) {
		Cltn<Entry *> * aQuorumReplyList = anActivity->replyList();
		if (aQuorumReplyList) {
		    cout <<"\nTask::expandQuorumGraph(): Error detected in input file.";
		    cout <<" A quorum join list cannot have a replying activity." << endl;
		    cout << "This is not implemented. No output will be generated." << endl;
		    exit(0);
		}
		//act_add_reply_list ( this, finalActivityName, aReplyList );
		// replyActivity->replyListReset();
	    }

////////////////////////////////////////////////
	    quorumListNumber++;
	    quorumAndJoinList->quorumListNum(quorumListNumber);

	    char localQuorumDelayActivityName[32];
	    sprintf( localQuorumDelayActivityName, "localQmDelay_%d", quorumListNumber);
	    localQuorumDelayActivity =  findOrAddPsuedoActivity(localQuorumDelayActivityName);

	    if (localQuorumDelayActivity) {
		localQuorumDelayActivity->remoteQuorumDelay.active(true);
		char remoteQuorumDelayName[32];
		sprintf( remoteQuorumDelayName, "remoteQmDelay_%d", quorumListNumber);
		localQuorumDelayActivity->remoteQuorumDelay.nameSet(remoteQuorumDelayName);
	    } else {
		cout <<"\nTask::expandQuorumGraph(): Error, could not create localQuorumDelay activity" << endl;
		abort();
	    }
	    char finalActivityName[32];
	    sprintf( finalActivityName, "final_%d", quorumListNumber);
	    finalActivity =  findOrAddPsuedoActivity(finalActivityName);

	    Cltn<Activity *> cltnJoinActivities;
	    if ( dynamic_cast<ForkJoinActivityList *>(quorumAndJoinList)) {
		cltnJoinActivities = dynamic_cast<ForkJoinActivityList *>(quorumAndJoinList)->getMyActivityList();
	    }

	    //to force the local delay (sumTotal of localQuorumDelayActivity) to use a gamma distribution fitting.
	    //localQuorumDelayActivity->phaseTypeFlag(PHASE_DETERMINISTIC);

	    localQuorumDelayActivity->localQuorumDelay(true);

	    store_activity_service_time ( localQuorumDelayActivity->name(), 0 );
	    store_activity_service_time ( finalActivityName, 0 );

	    newAndJoinList = dynamic_cast<AndJoinActivityList *>(localQuorumDelayActivity->act_and_join_list( newAndJoinList, 0 ));
	    if ( newAndJoinList == NULL ) throw logic_error( "Task::expandQuorumGraph" );
	    newAndForkList= dynamic_cast<AndForkActivityList *> (localQuorumDelayActivity->act_and_fork_list( newAndForkList, 0 ));
	    if ( newAndForkList == NULL ) throw logic_error( "Task::expandQuorumGraph" );
	    orgForkList =quorumAndJoinList->next();

	    if (orgForkList) {
		Cltn<Activity *> cltnForkActivities;
		if ( dynamic_cast<ForkJoinActivityList *>(orgForkList)) {
		    cltnForkActivities = dynamic_cast<ForkJoinActivityList *>(orgForkList)->getMyActivityList();
		} else if ( dynamic_cast<SequentialActivityList *>(orgForkList)) {
		    Activity * replyActivity= dynamic_cast<SequentialActivityList *>(orgForkList)->getMyActivity();
		    cltnForkActivities += replyActivity;

		    Sequence<Activity *> nextForkActivity(cltnForkActivities);

		    while (anActivity=nextForkActivity()) {
			anActivity->resetInputOutputLists();
			newAndJoinList = dynamic_cast<AndJoinActivityList *>( anActivity->act_and_join_list( newAndJoinList, 0 ));
			newAndForkList = dynamic_cast<AndForkActivityList *>( anActivity->act_and_fork_list( newAndForkList, 0 ));
		    }

		    act_connect ( quorumAndJoinList, newAndForkList );
		    ForkActivityList * finalAndForkList = dynamic_cast<ForkActivityList *>(finalActivity->act_fork_item( 0 ));
		    act_connect ( newAndJoinList, finalAndForkList );
		}
	    }
	}
    }
    return !anError;
}
#endif

/*
 * Set the service time for the activity.
 */

void
Task::store_activity_service_time ( const char * activity_name, const double service_time )
{
    Activity * anActivity = findOrAddActivity( activity_name );
    anActivity->isSpecified( true );
    anActivity->setServiceTime( service_time );
}

/*----------------------------------------------------------------------*/
/*                                Output                                */
/*----------------------------------------------------------------------*/

/*
 * Count up the number of calls made by this task (regardless of phase).
 */

unsigned
Task::countCallList( unsigned callType ) const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    unsigned count = 0;
    while ( anEntry = nextEntry() ) {
	CallInfo callList( anEntry, callType );
	count += callList.size();
    }

    Sequence<Activity *> nextActivity(activities());
    const Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	count += anActivity->countCallList( callType );
    }
    return count;
}



/*
 * Count up the number of calls made by this task (regardless of phase).
 */

unsigned
Task::countJoinList() const
{
//    #warning fix me!
    return 0;
}


void
Task::insertDOMResults(void) const
{
    Sequence<Entry *> nextEntry( entries() );
    Entry *anEntry;
    double totalTaskUtil   = 0.0;
    double totalThroughput = 0.0;
    double totalProcUtil   = 0.0;
    double totalPhaseUtils[MAX_PHASES];
    double resultPhaseData[MAX_PHASES];

    for ( unsigned p = 0; p < MAX_PHASES; ++p ) {
	totalPhaseUtils[p] = 0.0;
	resultPhaseData[p] = 0.0;
    }

    if (hasActivities()) {
	Sequence<Activity *> nextActivity(activities());
	Activity *anActivity;

	while (anActivity = nextActivity()) {
	    anActivity->insertDOMResults();
	}

	Sequence<ActivityList *> nextActivityList(myPrecedence);
	ActivityList * anActivityList;

	while ( anActivityList = nextActivityList() ) {
	    anActivityList->insertDOMResults();
	}
    }

    for ( int count = 0; anEntry = nextEntry(); ++count ) {
	anEntry->computeVariance();
	anEntry->insertDOMResults(&totalPhaseUtils[0]);

	totalProcUtil += anEntry->processorUtilization();
	totalThroughput += anEntry->throughput();
    }

    /* Store totals */

    for ( unsigned p = 0; p < maxPhase(); ++p ) {
	totalTaskUtil += totalPhaseUtils[p];
	resultPhaseData[p] = totalPhaseUtils[p];
    }

    /* Place all of the totals into the DOM task itself */
    myDOMTask->setResultPhaseUtilizations(maxPhase(), resultPhaseData);
    myDOMTask->setResultUtilization(totalTaskUtil);
    myDOMTask->setResultThroughput(totalThroughput);
    myDOMTask->setResultProcessorUtilization(totalProcUtil);
}


/* --------------------------- Debugging. --------------------------- */

/*
 * Debug - print out waiting by submodel.
 */

ostream&
Task::printSubmodelWait( ostream& output ) const
{
    Sequence<Entry *> nextEntry( entries() );
    const Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	anEntry->printSubmodelWait( output, 0 );
    }

    for ( unsigned i = 2; i <= myThreads.size(); ++i ) {
	myThreads[i]->printSubmodelWait( output, 2 );
    }
    return output;
}


/*
 * Print chains for this client.
 */

ostream&
Task::printClientChains( ostream& output, const unsigned submodel ) const
{
    output << "Chains:" << myClientChains[submodel] << endl;
    return output;
}


/*
 * Tracing: Print out the overlap table.
 */

ostream&
Task::printOverlapTable( ostream& output, const ChainVector& chain, const VectorMath<double>* of ) const
{

    //To handle the case of a main thread of control with no fork join.
    unsigned n = nThreads(); // unsigned n = chain.size();


    unsigned i;
    unsigned j;
    int precision = output.precision(3);
    ios_base::fmtflags flags = output.setf( ios::left, ios::adjustfield );
    output.setf( ios::left, ios::adjustfield );

    /* Overlap table.  */
    // if (myThreads[4]->name() ) {

//cout << "NAME IS : " << myThreads[4]->name() << endl;
    //}
    output << "-- Fork Overlap Table -- " << endl << name() << endl << setw(6) << " ";
    for ( i = 2; i <= n ; ++i ) {

	output << setw(6) << myThreads[i]->name();
    }
    output << endl;



    for ( i = 2; i <= n; ++i ) {
	output << setw(6) << myThreads[i]->name();
	for ( j = 1; j <= n; ++j ) {
	    output << setw(6) << of[chain[i]][chain[j]];
	}
	output << endl;
    }
    output << endl << endl;

    output.flags( flags );
    output.precision( precision );
    return output;
}



/*
 * Tracing -- Print out the joins delays.
 */

ostream&
Task::printJoinDelay( ostream& output ) const
{
//#warning fix me!
#if 0
    Sequence<AndJoinActivityList *> nextJoin( myJoin );
    AndJoinActivityList * aJoin;

    while ( aJoin = nextJoin() ) {
	aJoin->printDelay( output );
	output << endl;
    }
#endif

    return output;
}

/* ------------------------- Reference Tasks. ------------------------- */

ReferenceTask::ReferenceTask( LQIO::DOM::Task* domTask, const Processor * aProc, const Group * aGroup, Cltn<Entry *> *aCltn )
    : Task( domTask, aProc, aGroup, aCltn )
{
}


unsigned
ReferenceTask::copies() const
{
    const LQIO::DOM::ExternalVariable * dom_copies = domEntity->getCopies();
    double value;
    assert(dom_copies->getValue(value) == true);
    if ( isinf( value ) ) {
	LQIO::solution_error( LQIO::ERR_REFERENCE_TASK_IS_INFINITE, name() );
	return 1;
    } else if ( value - floor(value) != 0 ) {
	throw domain_error( "ReferencerTask::copies" );
    }
    return static_cast<unsigned int>(value);
}



/*
 * Dynamic Updates / Late Finalization
 * In order to integrate LQX's support for model changes we need to
 * have a way of re-calculating what used to be static for all
 * dynamically editable values
 */

void
ReferenceTask::recalculateDynamicValues()
{
    Task::recalculateDynamicValues();

    const LQIO::DOM::ExternalVariable * dom_thinkTime = dynamic_cast<LQIO::DOM::Task *>(domEntity)->getThinkTime();
    double value = 0.;
    if ( dom_thinkTime ) {
	assert(dom_thinkTime->getValue(value) == true);
    } 
    myThinkTime = value;
}


/*
 * Valid reference task?
 */

void
ReferenceTask::check() const
{
    Task::check();

    if ( nEntries() != 1 ) {
	LQIO::solution_error( LQIO::WRN_TOO_MANY_ENTRIES_FOR_REF_TASK, name() );
    }
    if ( queueLength() > 0 ) {
	LQIO::solution_error( LQIO::WRN_QUEUE_LENGTH, name() );
    }

    Sequence<Entry *> nextEntry( entries() );
    const Entry *anEntry;

    while( anEntry = nextEntry() ) {
	if ( anEntry->isCalled()  ) {
	    LQIO::solution_error( LQIO::ERR_REFERENCE_TASK_IS_RECEIVER, name(), anEntry->name() );
	}
    }
}


/*
 * Set population.
 */

double
ReferenceTask::countCallers( Cltn<const Task *> &reject )
{
    return static_cast<double>(copies());
}

/*
 * Reference tasks are always "direct paths" for the purposes of forwarding.
 */

unsigned
ReferenceTask::findChildren( CallStack& callStack, const bool ) const
{
    unsigned max_depth = Entity::findChildren( callStack, true );

    /* Chase calls from srcTask. */

    Sequence<Entry *> nextEntry(entries());
    Entry *anEntry;

    while ( anEntry = nextEntry() ) {
	max_depth = max( max_depth, anEntry->findChildren( callStack, true ) );
    }

    return max_depth;
}


/*
 * Reference tasks cannot be servers by definition.
 */

Server *
ReferenceTask::makeServer( const unsigned )
{
    throw should_not_implement( "ReferenceTask::makeServer", __FILE__, __LINE__ );
    return 0;
}

/* -------------------------- Simple Servers. ------------------------- */


void
ServerTask::check() const
{
    Task::check();

    if ( scheduling() == SCHEDULE_DELAY && copies() != 1 ) {
	LQIO::solution_error( LQIO::WRN_INFINITE_MULTI_SERVER, "Task", name(), copies() );
    }

    Sequence<Entry *> nextEntry( entries() );
    const Entry *anEntry;

    while( anEntry = nextEntry() ) {
	if ( anEntry->openArrivalRate() > 0.0 ) {
	    Entry::totalOpenArrivals += 1;
	} else if ( !anEntry->isCalled() ) {
	    LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, anEntry->name() );
	}
    }
}


/*
 * Count up the number of callers to this task which is essential if the
 * Infinite server is used as a client in a lower level model.  The
 * multiplicity is ignore otherwise.
 */

void
ServerTask::configure( const unsigned nSubmodels )
{
    Task::configure( nSubmodels );

    /* Tag this task as special if necessary. */

    const Processor * aProcessor = processor();
    if ( isInfinite() && aProcessor->isInfinite() ) {
	isPureDelay( true );
    }
}


/*
 * Return true if the population is infinite (i.e., an open source)
 */

bool
ServerTask::hasInfinitePopulation() const
{
    return isInfinite() && !isfinite(population());
}



/*
 * Indicate whether the variance calculation should take place.
 */

bool
ServerTask::hasVariance() const
{
    return  !isInfinite() && !isMultiServer() && attributes.variance != 0;
}



/*
 * Return the scheduling type allowed for this object.  Overridden by
 * subclasses if the scheduling type can be something other than FIFO.
 */

scheduling_type
ServerTask::defaultScheduling() const
{
    if ( isInfinite() ) {
	return SCHEDULE_DELAY;
    } else {
	return SCHEDULE_FIFO;
    }
}


/*
 * Return the scheduling type allowed for this object.  Overridden by
 * subclasses if the scheduling type can be something other than FIFO.
 */

unsigned
ServerTask::validScheduling() const
{
    if ( isInfinite() ) {
	return (unsigned)-1;		/* All types allowed */
    } else if ( isMultiServer() ) {
	return SCHED_PS_BIT|SCHED_FIFO_BIT;
    } else {
	return SCHED_FIFO_BIT|SCHED_HOL_BIT|SCHED_PPR_BIT;
    }
}


/*
 * Create (or recreate) a server.  If we're called a a second+ time,
 * and the station type changes, then we change the underlying
 * station.  We only return a station when we create one.
 */

Server *
ServerTask::makeServer( const unsigned nChains )
{
    Server * oldStation = myServerStation;

    if ( isInfinite() ) {

	/* ---------------- Infinite Servers ---------------- */

	if ( dynamic_cast<Infinite_Server *>(myServerStation) ) return 0;
	myServerStation = new Infinite_Server( nEntries(), nChains, maxPhase() );

    } else if ( isMultiServer() ) {

	/* ---------------- Multi Servers ---------------- */

	if ( !hasSecondPhase() || pragma.getOvertaking() == NO_OVERTAKING ) {

	    switch ( pragma.getMultiserver() ) {
	    default:
	    case DEFAULT_MULTISERVER:
		if ( (copies() < 40 || nChains <= 3) || isInOpenModel() ) {
		    if ( dynamic_cast<Conway_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies() ) return 0;
		    myServerStation = new Conway_Multi_Server( copies(), nEntries(), nChains, maxPhase());
		} else {
		    if ( dynamic_cast<Rolia_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies() ) return 0;
		    myServerStation = new Rolia_Multi_Server(  copies(), nEntries(), nChains, maxPhase());
		}
		break;

	    case CONWAY_MULTISERVER:
		if ( dynamic_cast<Conway_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies() ) return 0;
		myServerStation = new Conway_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case REISER_MULTISERVER:
		if ( dynamic_cast<Reiser_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies() ) return 0;
		myServerStation = new Reiser_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case REISER_PS_MULTISERVER:
		if ( dynamic_cast<Reiser_PS_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies() ) return 0;
		myServerStation = new Reiser_PS_Multi_Server( copies(), nEntries(), nChains, maxPhase());
		break;

	    case ROLIA_MULTISERVER:
		if ( dynamic_cast<Rolia_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Rolia_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;

	    case ROLIA_PS_MULTISERVER:
		if ( dynamic_cast<Rolia_PS_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Rolia_PS_Multi_Server(  copies(), nEntries(), nChains, maxPhase());
		break;

	    case BRUELL_MULTISERVER:
		if ( dynamic_cast<Bruell_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Bruell_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case SCHMIDT_MULTISERVER:
		if ( dynamic_cast<Schmidt_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Schmidt_Multi_Server(   copies(), nEntries(), nChains, maxPhase());
		break;

	    case SURI_MULTISERVER:
		if ( dynamic_cast<Suri_Multi_Server *>(myServerStation)  && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Suri_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;
	    }

	} else if ( markovOvertaking() ) {

	    switch ( pragma.getMultiserver() ) {
	    default:
	    case DEFAULT_MULTISERVER:
	    case CONWAY_MULTISERVER:
	    case SCHMIDT_MULTISERVER:
		if ( dynamic_cast<Markov_Phased_Conway_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Markov_Phased_Conway_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case REISER_MULTISERVER:
		if ( dynamic_cast<Markov_Phased_Reiser_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Markov_Phased_Reiser_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case ROLIA_MULTISERVER:
		if ( dynamic_cast<Markov_Phased_Rolia_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Markov_Phased_Rolia_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;

	    case REISER_PS_MULTISERVER:
	    case ROLIA_PS_MULTISERVER:
		if ( dynamic_cast<Markov_Phased_Rolia_PS_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Markov_Phased_Rolia_PS_Multi_Server(  copies(), nEntries(), nChains, maxPhase());
		break;

	    case SURI_MULTISERVER:
		if ( dynamic_cast<Markov_Phased_Suri_Multi_Server *>(myServerStation) && myServerStation->marginalProbabilitiesSize() == copies()) return 0;
		myServerStation = new Markov_Phased_Suri_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;

	    }

	} else {

	    switch ( pragma.getMultiserver() ) {
	    default:
	    case DEFAULT_MULTISERVER:
	    case CONWAY_MULTISERVER:
	    case SCHMIDT_MULTISERVER:
		if ( dynamic_cast<Phased_Conway_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Phased_Conway_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case REISER_MULTISERVER:
	    case REISER_PS_MULTISERVER:
		if ( dynamic_cast<Phased_Reiser_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Phased_Reiser_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;


	    case ROLIA_MULTISERVER:
		if ( dynamic_cast<Phased_Rolia_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Phased_Rolia_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;

	    case ROLIA_PS_MULTISERVER:
		if ( dynamic_cast<Phased_Rolia_PS_Multi_Server *>(myServerStation) ) return 0;
		myServerStation = new Phased_Rolia_PS_Multi_Server(  copies(), nEntries(), nChains, maxPhase());
		break;

	    }
	}

    } else if ( hasVariance() ) {

	/* ---------------- Simple Servers ---------------- */

	/* Stations with variance calculation used.	*/

	if ( !hasSecondPhase() || pragma.getOvertaking() == NO_OVERTAKING ) {
	    if ( HOL_Scheduling() ) {
		if ( dynamic_cast<HOL_HVFCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new HOL_HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else if ( PPR_Scheduling() ) {
		if ( dynamic_cast<PR_HVFCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new PR_HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<HVFCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    }
	} else switch( pragma.getOvertaking() ) {
	    case ROLIA_OVERTAKING:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_HVFCFS_Rolia_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HOL_HVFCFS_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_HVFCFS_Rolia_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new PR_HVFCFS_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<HVFCFS_Rolia_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HVFCFS_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    case SIMPLE_OVERTAKING:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_HVFCFS_Simple_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HOL_HVFCFS_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_HVFCFS_Simple_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new PR_HVFCFS_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<HVFCFS_Simple_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HVFCFS_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    case MARKOV_OVERTAKING:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_HVFCFS_Markov_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HOL_HVFCFS_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_HVFCFS_Markov_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new PR_HVFCFS_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<HVFCFS_Markov_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HVFCFS_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    default:
		LQIO::internal_error( __FILE__, __LINE__, "ServerTask::makeServer" );
		break;

	    }

    } else {

	/* Stations withOUT variance.			*/

	if ( !hasSecondPhase() || pragma.getOvertaking() == NO_OVERTAKING ) {
	    if ( HOL_Scheduling() ) {
		if ( dynamic_cast<HOL_FCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new HOL_FCFS_Server( nEntries(), nChains, maxPhase() );
	    } else if ( PPR_Scheduling() ) {
		if ( dynamic_cast<PR_FCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new PR_FCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<FCFS_Server *>(myServerStation) ) return 0;
		myServerStation = new FCFS_Server( nEntries(), nChains, maxPhase() );
	    }

	} else switch( pragma.getOvertaking() ) {
	    case ROLIA_OVERTAKING:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_Rolia_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HOL_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_Rolia_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new PR_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<Rolia_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    case SIMPLE_OVERTAKING:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_Simple_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HOL_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_Simple_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new PR_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<Simple_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    case MARKOV_OVERTAKING:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_Markov_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new HOL_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_Markov_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new PR_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<Markov_Phased_Server *>(myServerStation) ) return 0;
		    myServerStation = new Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    default:
		LQIO::internal_error( __FILE__, __LINE__, "ServerTask::makeServer" );
		break;
	    }
    }

    if ( oldStation ) delete oldStation;

    return myServerStation;
}

/* ------------------------- Semaphore Servers. ----------------------- */

void
SemaphoreTask::check() const
{
    if ( nEntries() == 2 ) {
	if ( !((entryAt(1)->isSignalEntry() && entryAt(2)->entrySemaphoreTypeOk(SEMAPHORE_WAIT))
	       || (entryAt(1)->isWaitEntry() && entryAt(2)->entrySemaphoreTypeOk(SEMAPHORE_SIGNAL))
	       || (entryAt(2)->isSignalEntry() && entryAt(1)->entrySemaphoreTypeOk(SEMAPHORE_WAIT))
	       || (entryAt(2)->isWaitEntry() && entryAt(1)->entrySemaphoreTypeOk(SEMAPHORE_SIGNAL))) ) {
	    LQIO::solution_error( LQIO::ERR_NO_SEMAPHORE, name() );
	}
    } else {
	LQIO::solution_error( LQIO::ERR_ENTRY_COUNT_FOR_TASK, name(), nEntries(), N_SEMAPHORE_ENTRIES );
    }

    Sequence<Entry *> nextEntry( entries() );
    const Entry *anEntry;

    io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::WARNING_ONLY;
    while( anEntry = nextEntry() ) {
	if ( anEntry->openArrivalRate() > 0.0 ) {
	    Entry::totalOpenArrivals += 1;
	} else if ( !anEntry->isCalled() ) {
	    LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, anEntry->name() );
	}
    }
}


void
SemaphoreTask::configure( const unsigned )
{
    throw not_implemented( "SemaphoreTask::configure", __FILE__, __LINE__ );
}

Server *
SemaphoreTask::makeServer( const unsigned )
{
    throw not_implemented( "SemaphoreTask::configure", __FILE__, __LINE__ );
}

/*----------------------------------------------------------------------*/
/*               A Sequence of all calls from this task                 */
/*----------------------------------------------------------------------*/

/*
 * Locate all calls generated by aTask regardless of phase.
 * Create a collection so that the () operator can step over it.
 */

TaskCallList::TaskCallList( const Task * aTask )
    : index(1)
{
    Sequence<Entry *> nextEntry( aTask->entries() );
    const Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	if ( anEntry->isActivityEntry() ) continue;
	for ( unsigned p = 1; p <= anEntry->maxPhase(); ++p ) {
	    callCltn += anEntry->callList(p);
	    if ( anEntry->processorCall( p ) ) {
		callCltn += anEntry->processorCall( p );
	    }
	}
    }

    Sequence<Activity *> nextActivity( aTask->activities() );
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	callCltn += anActivity->callList();
	if ( anActivity->processorCall() ) {
	    callCltn += anActivity->processorCall();
	}
    }
}



/*
 * De-reference calls prior to callCltn destruction (may not be
 * necessary, but why take chances?
 */

TaskCallList::~TaskCallList()
{
    callCltn.clearContents();
}


/*
 * Return next item in callCltn at each call.
 */

Call *
TaskCallList::operator()()
{
    if ( index <= callCltn.sz ) {
	return callCltn.ia[index++];
    } else {
	index = 1;
	return 0;
    }
}

/* ----------------------- External functions. ------------------------ */

/*
 * Add a task to the model.  Called by the parser.
 */

Task*
Task::create( LQIO::DOM::Task* domTask, Cltn<Entry *> * entries )
{
    /* Recover the old parameter information that used to be passed in */
    const char* task_name = domTask->getName().c_str();
    const LQIO::DOM::Group * group = domTask->getGroup();
    const scheduling_type sched_type = domTask->getSchedulingType();

    if ( !task_name || strlen( task_name ) == 0 ) abort();

    if ( !entries ) {
	LQIO::input_error2( LQIO::ERR_NO_ENTRIES_DEFINED_FOR_TASK, task_name );
	return NULL;
    } else if ( find_if( task.begin(), task.end(), eq_task_name( task_name ) ) != task.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Task", task_name );
	return NULL;
    }

    if ( sched_type != SCHEDULE_CUSTOMER && domTask->hasThinkTime () ) {
	LQIO::input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name );
    }

    const char* processor_name = domTask->getProcessor()->getName().c_str();
    Processor * aProcessor = Processor::find( processor_name );

    if ( !aProcessor ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, processor_name );
	return NULL;
    }

    Group * aGroup = 0;
    if ( group ) {
	aGroup = Group::find( group->getName().c_str() );
	if ( !aGroup ) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, group->getName().c_str() );
	}
    }

    /* Pick-a-task */

    Task * aTask;

    switch ( sched_type ) {

    /* ---------- Client tasks ---------- */
    case SCHEDULE_BURST:
    case SCHEDULE_UNIFORM:
	LQIO::input_error2( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_type_str[static_cast<unsigned>(sched_type)], "task", task_name );
	/* Fall through */
    case SCHEDULE_CUSTOMER:
	aTask = new ReferenceTask( domTask, aProcessor, aGroup, entries );
	break;

    /* ---------- Generic Server tasks ---------- */
    case SCHEDULE_DELAY:
    case SCHEDULE_FIFO:
    case SCHEDULE_PPR:
    case SCHEDULE_HOL:
	aTask = new ServerTask( domTask, aProcessor, aGroup, entries );
	break;

	/* ---------- Special Cases ---------- */
	/*+ BUG_164 */
    case SCHEDULE_SEMAPHORE:
	if ( entries->size() != N_SEMAPHORE_ENTRIES ) {
	    LQIO::input_error2( LQIO::ERR_ENTRY_COUNT_FOR_TASK, task_name, entries->size(), N_SEMAPHORE_ENTRIES );
	}
	LQIO::input_error2( LQIO::ERR_NOT_SUPPORTED, "Semaphore tasks" );
	//	aTask = new SemaphoreTask( task_name, n_copies, replications, aProcessor, aGroup, entries, priority );
	//	break;
	/* fall through for now */
	/*- BUG_164 */

    default:
	LQIO::input_error2( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_type_str[static_cast<unsigned>(sched_type)], "task", task_name );
	domTask->setSchedulingType(SCHEDULE_FIFO);
	aTask = new ServerTask( domTask, aProcessor, aGroup, entries );
	break;
    }

    if ( flags.trace_task && strcmp( flags.trace_task, task_name ) == 0 ) {
	aTask->trace( true );
    }

    task.insert( aTask );		/* Insert into map */
    aProcessor->addTask( aTask );

    return aTask;
}

bool
ltTask::operator()(const Task * t1, const Task * t2) const
{
    return strcmp( t1->name(), t2->name() ) < 0;
}

/*----------------------------------------------------------------------*/
/*                               Printing                               */
/*----------------------------------------------------------------------*/

/*
 * Generic printer.
 */

ostream&
Task::print( ostream& output ) const
{
    ios_base::fmtflags oldFlags = output.setf( ios::left, ios::adjustfield );

    output << setw(8) << name() 
	   << " " << setw(9) << task_type(*this) 
	   << " " << setw(5) << replicas() 
	   << " " << setw(8) << (myProcessor ? myProcessor->name() : "--") 
	   << " " << setw(3) << priority();
    if ( isReferenceTask() && thinkTime() > 0.0 ) {
	output << " " << thinkTime();
    }
    output << " " << print_entries( entries() );
    if ( activities().size() > 0 ) {
	output << " : " << print_activities( *this );
    }

    /* Bonus information about stations -- derived by solver */

    output << " " << print_info( *this );
    if ( nThreads() > 1 ) {
	output << " {" << nThreads() << " threads}";
    }

    output.flags(oldFlags);
    return output;
}

/* ---------------------------------------------------------------------- */

bool
eq_task_name::operator()( const Task * t ) const
{
    return strcmp( t->name(), _s ) == 0;
}

static ostream&
activities_of_str( ostream& output, const Task& aTask )
{
    Sequence<Activity *> nextActivity(aTask.activities());
    const Activity * anActivity;
    string aString;

    for ( unsigned i = 0; anActivity = nextActivity(); ++i ) {
	if ( i > 0 ) {
	    aString += ", ";
	}
	aString += anActivity->name();
    }
    output << aString;
    return output;
}

SRVNTaskManip
print_activities( const Task& aTask )
{
    return SRVNTaskManip( activities_of_str, aTask );
}


static ostream&
task_type_of( ostream& output, const Task& aTask )
{
    char buf[12];
    const unsigned n      = aTask.copies();

    if ( aTask.scheduling() == SCHEDULE_CUSTOMER ) {
	sprintf( buf, "ref(%d)", n );
    } else if ( aTask.isInfinite() ) {
	sprintf( buf, "inf" );
    } else if ( n > 1 ) {
	sprintf( buf, "mult(%d)", n );
    } else {
	sprintf( buf, "serv" );
    }
    output << buf;
    return output;
}

static ostream&
client_chains_of_str( ostream& output, const Task& aClient, const unsigned aSubmodel )
{
    aClient.printClientChains( output, aSubmodel );
    return output;
}


SRVNTaskIntManip
print_client_chains( const Task& aTask, const unsigned aSubmodel )
{
    return SRVNTaskIntManip( client_chains_of_str, aTask, aSubmodel );
}


SRVNTaskManip
task_type( const Task& aTask )
{
    return SRVNTaskManip( task_type_of, aTask );
}
