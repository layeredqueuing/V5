/*  -*- c++ -*-
 * $HeadURL$
 * 
 * Everything you wanted to know about a task, but were afraid to ask.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * January 2001
 *
 * ------------------------------------------------------------------------
 * $Id$
 * ------------------------------------------------------------------------
 */

#include "lqn2ps.h"
#include <string>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <cmath>
#if HAVE_FLOAT_H
#include <float.h>
#endif
#include <limits.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include <lqio/labels.h>
#include <lqio/dom_task.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_activity.h>
#include <lqio/dom_actlist.h>
#include <lqio/dom_processor.h>
#include <lqio/dom_group.h>
#include <lqio/dom_document.h>
#include <lqio/srvn_output.h>
#include "errmsg.h"
#include "task.h"
#include "entry.h"
#include "activity.h"
#include "actlist.h"
#include "share.h"
#include "cltn.h"
#include "stack.h"
#include "processor.h"
#include "call.h"
#include "label.h"
#include "model.h"

#if !defined(MAXDOUBLE)
#define MAXDOUBLE FLT_MAX
#endif

set<Task *,ltTask> task;

bool Task::thinkTimePresent             = false;
bool Task::holdingTimePresent           = false;
bool Task::holdingVariancePresent       = false;
Processor * Task::defaultProcessor	= 0;

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNTaskManip {
public:
    SRVNTaskManip( ostream& (*ff)(ostream&, const Task & ), const Task & theTask ) : f(ff), aTask(theTask) {}
private:
    ostream& (*f)( ostream&, const Task& );
    const Task & aTask;

    friend ostream& operator<<(ostream & os, const SRVNTaskManip& m ) { return m.f(os,m.aTask); }
};


static ostream& entries_of_str( ostream& output,  const Task& aTask );
static ostream& task_scheduling_of_str( ostream& output,  const Task & aTask );

static inline SRVNTaskManip entries_of( const Task& aTask ) { return SRVNTaskManip( entries_of_str, aTask ); }
static inline SRVNTaskManip task_scheduling_of( const Task & aTask ) { return SRVNTaskManip( &task_scheduling_of_str, aTask ); }

/* ---------------------- Overloaded Operators ------------------------ */

ostream& operator<<( ostream& output, const Task& self )
{
    switch( Flags::print[OUTPUT_FORMAT].value.i ) {
#if defined(TXT_OUTPUT)
    case FORMAT_TXT:
#endif
#if defined(QNAP_OUTPUT)
    case FORMAT_QNAP:
#endif
    case FORMAT_XML:
	break;
    case FORMAT_SRVN:
    {
	LQIO::SRVN::TaskInput( output, 0 ).print( *dynamic_cast<const LQIO::DOM::Task *>(self.getDOM()) );
    }
	break;
    default:
	self.draw( output );
	break;
    }

    return output;
}

Task::Task( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& entries )
    : Entity( dom, ::task.size()+1 ),
      myProcessor(aProc),
      myShare(aShare),
      myMaxPhase(0),
      maxLevel(0),
      entryWidthPts(0)
{
    entryList = entries;
    setEntryOwner( entryList, this );

    if ( aProc ) {
	ProcessorCall * aCall = new ProcessorCall(this,aProc);
	myCalls << aCall;
	const_cast<Processor *>(aProc)->addDstCall( aCall );
	const_cast<Processor *>(aProc)->addTask( this );
    }

    myNode = Node::newNode( Flags::icon_width, Flags::graphical_output_style == TIMEBENCH_STYLE ? Flags::icon_height : Flags::entry_height );
    myLabel = Label::newLabel();
}



Task::~Task()
{
    entryList.clearContents();
    activityList.deleteContents();
    myCalls.deleteContents();
    precedenceList.deleteContents();
}



/*
 * Reset globals.
 */

void
Task::reset()
{
    thinkTimePresent = false;
    holdingTimePresent = false;
    holdingVariancePresent = false;
    defaultProcessor = 0;
}



/* ------------------------ Instance Methods -------------------------- */

int 
Task::priority() const
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getPriority();
}



/* ------------------------------ Results ----------------------------- */

double
Task::throughput() const 
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getResultThroughput();
}

/* -- */

double
Task::utilization( const unsigned p ) const
{
    assert( 0 < p && p <= LQIO::DOM::Phase::MAX_PHASE );

    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getResultPhasePUtilization(p);
}


/* --- */

double
Task::utilization() const
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getResultUtilization();
}


/* --- */

double 
Task::processorUtilization() const
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getResultProcessorUtilization();
}

/*
 * Rename tasks.
 */

void
Task::rename()
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->rename();
    }

    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;

    for ( unsigned i = 1; anActivity = nextActivity(); ++i ) {
	anActivity->rename();
    }

    Element::rename();
}



void
Task::squishName()
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    Element::squishName();

    while ( anEntry = nextEntry() ) {
	anEntry->squishName();
    }

    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	anActivity->squishName();
    }
}



/*
 * Aggregate activities to activities and/or activities to phases.  If
 * activities are left after aggregation, we will have to recompute
 * their level because there likely is a lot less of them to draw.
 */

Task&
Task::aggregate()
{
    maxLevel = 0;
    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].clearContents();
    }

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->aggregate();
    }

    switch ( Flags::print[AGGREGATION].value.i ) {
    case AGGREGATE_ENTRIES:
	activityList.deleteContents().chop(activityList.size());
	aggregateEntries();
	break;

    case AGGREGATE_ACTIVITIES:
    case AGGREGATE_PHASES:
	activityList.deleteContents().chop(activityList.size());
	break;

    default:
	/* Recompute levels. */
	Sequence<Activity *> nextActivity( activityList );
	Activity * anActivity;
	while ( anActivity = nextActivity() ) {
	    anActivity->resetLevel();
	}
	generate();
	break;
    }

    return *this;
}




/*
 * Sort entries and activities based on when they were visited.
 */

Task const &
Task::sort() const
{
    myCalls.sort( Call::compareSrc );
    Entity::sort();
    return *this;
}


double
Task::getIndex() const
{
    double anIndex = MAXDOUBLE;

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anIndex = min( anIndex, anEntry->getIndex() );
    }

    return anIndex;
}


int
Task::span() const
{
    if ( Flags::print[LAYERING].value.i == LAYERING_GROUP ) {
	Cltn<const Entity *> myServers;
	const int n = servers( myServers );
	if ( n ) return n;		/* Force those making calls to lower levels right */
    }
    return 0;
}


/*
 * Aggregate all entries to this task.
 */

Task&
Task::aggregateEntries()
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    /* Aggregate calls to task */

    for ( unsigned i = 1; i <= paths().size(); ++i ) {
	while ( anEntry = nextEntry() ) {
	    anEntry->aggregateEntries( myPaths[i] );	/* Aggregate based on ref-task chain. */
	}
    }

    return *this;
}

/*
 * Set the chain of this client task to curr_k.  Chains will be set to
 * all servers of this client.  next_k will be changed iff there are
 * forks.
 */

unsigned
Task::setChain( unsigned curr_k, callFunc aFunc  ) const
{
    unsigned next_k = curr_k;

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    /*
     * I will have to find all servers to this client (including the
     * processor)...  because each server will get it's own chain.
     * Threads will complicate matters, bien sur.
     */

    if ( replicas() > 1 ) {
	Cltn<const Entity *> allServers;
	servers( allServers );

	allServers.sort( &Entity::compareCoord );

	Sequence<const Entity *> nextServer( allServers );
	const Entity * aServer;

	while ( aServer = nextServer() ) {
	    if ( !aServer->isSelected() ) continue;
	    while ( anEntry = nextEntry() ) {
		next_k = anEntry->setChain( curr_k, next_k, aServer, aFunc );
	    }
	    curr_k = next_k + 1;
	    next_k = curr_k;
	}

    } else {

	while ( anEntry = nextEntry() ) {
	    next_k = anEntry->setChain( curr_k, next_k, 0, aFunc );
	}
    }
    return next_k;
}


/*
 * Set the server chain k.  We might have more than one processor (-Lclient).
 */

Task&
Task::setServerChain( unsigned k )
{
    Sequence<EntityCall *> nextCall( callList() );
    EntityCall * aCall;
    while ( aCall = nextCall() ) {
	const ProcessorCall * procCall = dynamic_cast<const ProcessorCall *>(aCall);
	if ( procCall ) {
	    const Processor * aProcessor = procCall->dstProcessor();
	    if ( aProcessor->isInteresting() && aProcessor->isSelected() ) {
		const_cast<Processor *>(aProcessor)->setServerChain( k );
	    }
	    continue;
	}
	const TaskCall * taskCall = dynamic_cast<const TaskCall *>(aCall);
	if ( taskCall ) {
	    const Task * aTask = taskCall->dstTask();
	    if ( aTask->isSelected() ) {
		const_cast<Task *>(aTask)->setServerChain( k ).setClientClosedChain( k );
	    }
	}
    }

    Element::setServerChain( k );
    return *this;
}



/*
 * Add a task to the list of tasks for this processor and set local index
 * for MVA.
 */

Task&
Task::addEntry( Entry * anEntry )
{
    entryList << anEntry;
    return *this;
}


Task&
Task::removeEntry( Entry * anEntry )
{
    entryList -= anEntry;
    return *this;
}


/*
 * Add a task to the list of tasks for this processor and set local index
 * for MVA.
 */

Activity *
Task::findActivity( const string& name ) const
{
    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity = 0;

    while ( ( anActivity = nextActivity() ) && anActivity->name() != name );

    return anActivity;
}


Activity *
Task::findOrAddActivity( const LQIO::DOM::Activity * activity )
{
    Activity * anActivity = findActivity( activity->getName() );

    if ( !anActivity ) {
	anActivity = new Activity( this, activity );
	activityList << anActivity;
    }

    return anActivity;
}



#if defined(REP2FLAT)
/*
 * Find an existing activity which has the same name as srcActivity.  
 */

Activity * 
Task::findActivity( const Activity& srcActivity, const unsigned replica )
{
    ostringstream aName;
    aName << srcActivity.name() << "_" << replica;
    
    return findActivity( aName.str() );
}


/*
 * Find an existing activity which has the same name as srcActivity.  If the activity does not exist, create one and copy it from the src.
 */

Activity * 
Task::addActivity( const Activity& srcActivity, const unsigned replica )
{
    ostringstream aName;
    aName << srcActivity.name() << "_" << replica;
    
    Activity * dstActivity = findActivity( aName.str() );
    if ( dstActivity ) return dstActivity;	// throw error?

    LQIO::DOM::Task * dom_task = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(getDOM()));
    LQIO::DOM::Activity * dstDOM = new LQIO::DOM::Activity( *(dynamic_cast<const LQIO::DOM::Activity *>(srcActivity.getDOM())) );
    dstDOM->setName( aName.str() );
    dom_task->addActivity( dstDOM );
    dstActivity = new Activity( this, dstDOM );
    activityList << dstActivity;

    if ( srcActivity.isStartActivity() ) {
	Entry *dstEntry = Entry::find_replica( srcActivity.rootEntry()->name(), replica );
	const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(dstEntry->getDOM()))->setStartActivity( dstDOM );
	if (dstEntry->entryTypeOk(LQIO::DOM::Entry::ENTRY_ACTIVITY)) {
	    dstEntry->setStartActivity(dstActivity);
	}
    }
    
    const Cltn<const Entry *> *srcReplyList = srcActivity.replyList();
    Cltn<const Entry *> * dstReplyList = dstActivity->replyList();
    if ( srcReplyList ) {
	for ( unsigned i = 1; i <= srcReplyList->size(); ++i ) {
	    const Entry * srcEntry = (*srcReplyList)[i];
	    *dstReplyList << Entry::find_replica( srcEntry->name(), replica );
	}
    }

    dstActivity->expandActivityCalls( srcActivity, replica );

    return dstActivity;
}
#endif

Task&
Task::removeActivity( Activity * anActivity )
{
    activityList -= anActivity;
    return *this;
}


void
Task::addPrecedence( ActivityList * aPrecedence )
{
    precedenceList << aPrecedence;
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

	case OPEN_ARRIVAL_REQUEST:	/* Root task, but at lower level */
	    level = 2;
	    break;

	case RENDEZVOUS_REQUEST:	/* Non-root task. 		*/
	case SEND_NO_REPLY_REQUEST:	/* Non-root task. 		*/
	case NOT_CALLED:		/* No operation.		*/
	    break;
	}
    }
    return level;
}




/*
 * Return true if this task forwards to aTask.
 */

bool
Task::forwardsTo( const Task * aTask ) const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( anEntry->forwardsTo( aTask ) ) return true;
    }
    return false;
}


/*
 * Return true if this task forwards to another task on this level.
 */

bool
Task::hasForwardingLevel() const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( anEntry->hasForwardingLevel() ) return true;
    }
    return false;
}


/*
 * Return true if this task forwards to another task on this level.
 */

bool
Task::isForwardingTarget() const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( anEntry->isForwardingTarget() ) return true;
    }
    return false;
}


/*
 * Return true if this task receives rendezvous requests.
 */

bool
Task::isCalled( const requesting_type callType ) const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( anEntry->isCalled() == callType ) return true;
    }
    return false;
}


/*
 * Return the total open arrival rate to this server.
 */

double
Task::openArrivalRate() const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    double sum = 0.0;
    while ( anEntry = nextEntry() ) {
	if ( anEntry->hasOpenArrivalRate() ) {
	    sum += LQIO::DOM::to_double( anEntry->openArrivalRate() );
	}
    }

    return sum;
}



/*
 * Return true if this task makes any calls to lower level tasks.
 */

bool
Task::hasCalls( const callFunc aFunc ) const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( anEntry->hasCalls( aFunc ) ) {
	    return true;
	}
    }
    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	if ( anActivity->hasCalls( aFunc ) ) return true;
    }
    return false;
}



/*
 * Returns the initial depth (0 or 1) if this entity is a root of a
 * call graph.  Returns -1 otherwise.  Used by the topological sorter.
 */

bool
Task::hasOpenArrivals() const
{
    return openArrivalRate() > 0.;
}



/*
 * Return true if any entry of this task queues on the processor.
 */

bool
Task::hasQueueingTime() const
{
    Sequence<Entry *> nextEntry(entries());
    const Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	if ( anEntry->hasQueueingTime() ) return true;
    }

    Sequence<Activity *> nextActivity(activities());
    const Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	if ( anActivity->hasQueueingTime() ) return true;
    }
    return false;
}


/*
 * Return true if this entity is selected.
 * See subclasses for further tests.
 */

bool
Task::isSelectedIndirectly() const
{
    if ( Entity::isSelectedIndirectly() || processor()->isSelected() ) {
	return true;
    } else {
	Sequence<Entry *>nextEntry( entries() );
	Entry *anEntry;
	while ( anEntry = nextEntry() ) {
	    if ( anEntry->isSelectedIndirectly() ) {
		return true;
	    }
	}
	Sequence<Activity *>nextActivity( activities() );
	Activity *anActivity;
	while ( anActivity = nextActivity() ) {
	    if ( anActivity->isSelectedIndirectly() ) {
		return true;
	    }
	}
    }
    return false;
}


/*
 * Return true if this task can be converted into one which takes open arrivals.
 */

bool
Task::canConvertToOpenArrivals() const
{
    return partial_output() && !isSelected() && !canConvertToReferenceTask();
}


/*
 * If I don't make any calls and my processor is only for me, then return true
 */

bool
Task::isPureServer() const
{
    Cltn<const Entity *> serversCltn;

    servers( serversCltn );
    return serversCltn.size() == 0 && processor()->nTasks() == 1;
}



void
Task::check() const
{
    const Processor * aProcessor = processor();

    /* Check prio/scheduling. */

    if ( aProcessor && priority() != 0 && !aProcessor->hasPriorities() ) {
	LQIO::solution_error( LQIO::WRN_PRIO_TASK_ON_FIFO_PROC, name().c_str(), aProcessor->name().c_str() );
    }

    /* Check entries */

    if ( scheduling() == SCHEDULE_SEMAPHORE ) {
	if ( nEntries() != 2 ) {
	    LQIO::solution_error( LQIO::ERR_ENTRY_COUNT_FOR_TASK, name().c_str(), nEntries(), N_SEMAPHORE_ENTRIES );
	}
	if ( !((entryAt(1)->isSignalEntry() && entryAt(2)->entrySemaphoreTypeOk(SEMAPHORE_WAIT))
	       || (entryAt(1)->isWaitEntry() && entryAt(2)->entrySemaphoreTypeOk(SEMAPHORE_SIGNAL))
	       || (entryAt(2)->isSignalEntry() && entryAt(1)->entrySemaphoreTypeOk(SEMAPHORE_WAIT))
	       || (entryAt(2)->isWaitEntry() && entryAt(1)->entrySemaphoreTypeOk(SEMAPHORE_SIGNAL))) ) {
	    LQIO::solution_error( LQIO::ERR_NO_SEMAPHORE, name().c_str() );
	} else if ( entryAt(1)->isCalled() && !entryAt(2)->isCalled() ) {
	    io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::RUNTIME_ERROR; /* WARNING_ONLY; */
	    LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, entryAt(2)->name().c_str() );
	} else if ( !entryAt(1)->isCalled() && entryAt(2)->isCalled() ) {
	    io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::RUNTIME_ERROR; /* WARNING_ONLY; */
	    LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, entryAt(1)->name().c_str() );
	}
	io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::WARNING_ONLY;		/* Revert to normal */

    } else if ( scheduling() == SCHEDULE_RWLOCK ) {
	if ( nEntries() != N_RWLOCK_ENTRIES ) {
	    LQIO::solution_error( LQIO::ERR_ENTRY_COUNT_FOR_TASK, name().c_str(), nEntries(), N_RWLOCK_ENTRIES );
	}

	const Entry * E[N_RWLOCK_ENTRIES];

	for ( unsigned int i=0;i<N_RWLOCK_ENTRIES;i++){ 
	    E[i]=0; 
	}
	
	for ( unsigned int i=1;i<=N_RWLOCK_ENTRIES;i++){
	    if ( entryAt(i)->is_r_unlock_Entry() ) {
		if ( !E[0] ) { 
		    E[0]=entryAt(i);
		} else { // duplicate entry TYPE error
		    LQIO::solution_error( LQIO::ERR_DUPLICATE_SYMBOL, name().c_str() );
		}
	    } else if (entryAt(i)->is_r_lock_Entry() ) {
		if ( !E[1] ) { 
		    E[1]=entryAt(i); 
		} else { 
		    LQIO::solution_error( LQIO::ERR_DUPLICATE_SYMBOL, name().c_str() );
		}
	    } else if ( entryAt(i)->is_w_unlock_Entry() ) {
		if ( !E[2] ) { 
		    E[2]=entryAt(i); 
		} else { 
		    LQIO::solution_error( LQIO::ERR_DUPLICATE_SYMBOL, name().c_str() );
		}
	    } else if ( entryAt(i)->is_w_lock_Entry() ) {
		if (!E[3]) { 
		    E[3]=entryAt(i); 
		} else { 
		    LQIO::solution_error( LQIO::ERR_DUPLICATE_SYMBOL, name().c_str() );
		}
	    } else { 
		LQIO::solution_error( LQIO::ERR_NO_RWLOCK, name().c_str() );
	    }
	}

	/* Make sure both or neither entry is called */

	if ( E[0]->isCalled() && !E[1]->isCalled() ) {
	    io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::RUNTIME_ERROR; /* WARNING_ONLY; */
	    LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, E[1]->name().c_str() );
	} else if ( !E[0]->isCalled() && E[1]->isCalled() ) {
	    io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::RUNTIME_ERROR; /* WARNING_ONLY; */
	    LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, E[0]->name().c_str() );
	}
	if ( E[2]->isCalled() && !E[3]->isCalled() ) {
	    io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::RUNTIME_ERROR; /* WARNING_ONLY; */
	    LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, E[3]->name().c_str() );
	} else if ( !E[2]->isCalled() && E[3]->isCalled() ) {
	    io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::RUNTIME_ERROR; /* WARNING_ONLY; */
	    LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, E[2]->name().c_str() );
	}
	io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::WARNING_ONLY;		/* Revert to normal */
    }

    Sequence<Entry *> nextEntry( entries() );
    const Entry *anEntry;

    while( anEntry = nextEntry() ) {
	if ( !isReferenceTask() ) {
	    if ( !anEntry->hasOpenArrivalRate() && !anEntry->isCalled() ) {
		LQIO::solution_error( LQIO::WRN_NO_REQUESTS_TO_ENTRY, anEntry->name().c_str() );
	    }
	}
	anEntry->check();
	myMaxPhase = max( myMaxPhase, anEntry->maxPhase() );
    }

    if ( scheduling() == SCHEDULE_SEMAPHORE ) {
	io_vars.error_messages[LQIO::WRN_NO_REQUESTS_TO_ENTRY].severity = LQIO::WARNING_ONLY;
    }

    Sequence<Activity *> nextActivity( activities() );
    const Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	anActivity->check();
    }
}




/*
 * Return all clients to this task.
 */

unsigned
Task::referenceTasks( Cltn<const Entity *> &clientsCltn, Element * dst ) const
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	anEntry->referenceTasks( clientsCltn, (dst == this) ? anEntry : dst );	/* Map task to entry if this is the dst */
    }
    return clientsCltn.size();
}



/*
 * Return all clients to this task.
 */

unsigned
Task::clients( Cltn<const Entity *> &clientsCltn, const callFunc aFunc ) const
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	anEntry->clients( clientsCltn, aFunc );
    }
    return clientsCltn.size();
}



/*
 * Locate the destination task in the list of destinations.  Processor
 * calls are lumped into the task call list.  This makes life simpler
 * when we are drawing tasks only.
 */

EntityCall *
Task::findCall( const Entity * anEntity, const callFunc aFunc ) const
{
    Sequence<EntityCall *> nextCall( callList() );
    EntityCall * aCall;

    while ( aCall = nextCall() ) {
	if ( 	(!dynamic_cast<TaskCall *>(aCall) || dynamic_cast<TaskCall *>(aCall)->dstTask() == anEntity)
	     && (!aCall->isProcessorCall() || dynamic_cast<ProcessorCall *>(aCall)->dstProcessor() == anEntity)
	     && (!aFunc || (aCall->*aFunc)() ) ) return aCall;
    }

    return 0;
}



EntityCall *
Task::findOrAddCall( const Task * toTask, const callFunc aFunc )
{
    EntityCall * aCall = findCall( toTask, aFunc );

    if ( !aCall ) {
	aCall = new TaskCall( this, toTask );
	myCalls << aCall;
	const_cast<Task *>(toTask)->addDstCall( aCall );
    }

    return aCall;
}



EntityCall *
Task::findOrAddFwdCall( const Task * toTask )
{
    EntityCall * aCall = findCall( toTask, &GenericCall::isPseudoCall );

    if ( !aCall ) {
	aCall = new PseudoTaskCall( this, toTask );
	myCalls << aCall;
	const_cast<Task *>(toTask)->addDstCall( aCall );
    }

    return aCall;
}



EntityCall *
Task::findOrAddPseudoCall( const Entity * toEntity )
{
    EntityCall * aCall = findCall( toEntity );

    if ( !aCall ) {
	if ( dynamic_cast<const Processor *>(toEntity) ) {
	    const Processor * toProcessor = dynamic_cast<const Processor *>(toEntity);
	    aCall = new PseudoProcessorCall( this, toProcessor );
	    const_cast<Processor *>(toProcessor)->addDstCall( aCall );
	    const_cast<Processor *>(toProcessor)->addTask( this );
	} else {
	    const Task * toTask = dynamic_cast<const Task *>(toEntity);
	    aCall = new PseudoTaskCall( this, toTask );
	    const_cast<Task *>(toTask)->addDstCall( aCall );
	}
	aCall->linestyle( Graphic::DASHED_DOTTED );
	myCalls << aCall;
    }

    return aCall;
}



/*
 * Return all servers to this task.
 */

unsigned
Task::servers( Cltn<const Entity *> &serversCltn ) const
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	Sequence<Call *> nextCall( anEntry->callList() );
	Call * aCall;
	while ( aCall = nextCall() ) {
	    if ( !aCall->hasForwardingLevel() && aCall->isSelected() ) {
		serversCltn += aCall->dstTask();
	    }
	}
    }

    Sequence<Activity *> nextActivity(activities());	/* Bug 623 */
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	Sequence<Call *> nextCall( anActivity->callList() );
	Call * aCall;
	while ( aCall = nextCall() ) {
	    if ( !aCall->hasForwarding() && aCall->isSelected() ) {
		serversCltn += aCall->dstTask();
	    }
	}
    }

    Sequence<EntityCall *> nextCall( callList() );
    EntityCall * aCall;
    while ( aCall = nextCall() ) {
	const ProcessorCall * processorCall = dynamic_cast<ProcessorCall *>(aCall);
	if ( !processorCall ) continue;
	if ( processorCall->dstProcessor()->isSelected() && processorCall->dstProcessor()->isInteresting() ) {
	    serversCltn += processor();
	}
    }

    return serversCltn.size();
}



bool
Task::isInOpenModel( const Cltn<Entity *>& servers ) const
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	Sequence<Call *> nextCall( anEntry->callList() );
	Call * aCall;
	while ( aCall = nextCall() ) {
	    if ( aCall->hasSendNoReply() && servers.find( const_cast<Task *>(aCall->dstTask()) ) ) return true;
	}
    }
    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	Sequence<Call *> nextCall( anActivity->callList() );
	Call * aCall;
	while ( aCall = nextCall() ) {
	    if ( aCall->hasSendNoReply() && servers.find( const_cast<Task *>(aCall->dstTask()) ) ) return true;
	}
    }
    return false;
}


bool
Task::isInClosedModel( const Cltn<Entity *>& servers ) const
{
    Sequence<EntityCall *> nextCall( callList() );
    const EntityCall * aCall;
    while ( aCall = nextCall() ) {
	const ProcessorCall * procCall = dynamic_cast<const ProcessorCall *>(aCall);
	if ( procCall ) {
	    const Processor * aProcessor = procCall->dstProcessor();
	    if ( aProcessor->isInteresting() && servers.find( const_cast<Processor *>(aProcessor) ) ) return true;
	    continue;
	}
	const TaskCall * taskCall = dynamic_cast<const TaskCall *>(aCall);
	if ( taskCall ) {
	    const Task * aTask = taskCall->dstTask();
	    if ( servers.find( const_cast<Task *>(aTask) ) ) return true;
	}
    }

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	Sequence<Call *> nextCall( anEntry->callList() );
	Call * aCall;
	while ( aCall = nextCall() ) {
	    if ( aCall->hasRendezvous() && servers.find( const_cast<Task *>(aCall->dstTask()) ) ) return true;
	}
    }
    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	Sequence<Call *> nextCall( anActivity->callList() );
	Call * aCall;
	while ( aCall = nextCall() ) {
	    if ( aCall->hasRendezvous() && servers.find( const_cast<Task *>(aCall->dstTask()) )) return true;
	}
    }
    return false;
}


/*
 * Recursively find all children and grand children from `father'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
Task::findChildren( CallStack& callStack, const unsigned directPath )
{
    unsigned max_depth = max( callStack.size(), level() );
    const Entry * dstEntry = callStack.top() ? callStack.top()->dstEntry() : 0;

    setLevel( max_depth ).addPath( directPath );

    if ( processor() ) {
	max_depth = max( max_depth + 1, processor()->level() );
	const_cast<Processor *>(processor())->setLevel( max_depth ).addPath( directPath );
    }

    Sequence<Entry *> nextEntry(entries());
    Entry *srcEntry;

    while ( srcEntry = nextEntry() ) {
	max_depth = max( max_depth,
			 srcEntry->findChildren( callStack,
						 ((srcEntry == dstEntry) || srcEntry->hasOpenArrivalRate())
						 ? directPath : 0  ) );
    }

    return max_depth;
}



/*
 * Count the number of calls that match the criteria passed
 */

unsigned
Task::countArcs( const callFunc aFunc ) const
{
    unsigned count = 0;

    Sequence<EntityCall *> nextCall( callList() );
    const EntityCall * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() && (!aFunc || (aCall->*aFunc)()) ) {
	    count += 1;
	}
    }
    return count;
}



/*
 * Count the number of calls that match the criteria passed
 */

double
Task::countCalls( const callFunc2 aFunc ) const
{
    double count = 0.0;

    Sequence<EntityCall *> nextCall( callList() );
    const EntityCall * aCall;

    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() ) {
	    count += (aCall->*aFunc)() * aCall->fanOut();
	}
    }
    return count;
}



/*
 * Try to estimate the number of threads that this task can spawn
 * (it's a gross hack)
 */

unsigned
Task::countThreads() const
{
    if ( hasActivities() ) {
	unsigned count = 1;
	Sequence<Activity *> nextActivity(activities());
	Activity * anActivity;

	while ( anActivity = nextActivity() ) {
	    AndForkActivityList * aList = dynamic_cast<AndForkActivityList *>(anActivity->outputTo());
	    if ( aList && aList->size() > 0 ) {
		count += aList->size();
	    }
	}

	return count;
    } else {   
	return 1;
    }
}


Cltn<Activity *>
Task::repliesTo( Entry * anEntry ) const
{
    Cltn<Activity *> aCltn;
    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;

    while ( anActivity = nextActivity() ) {
	if ( anActivity->repliesTo( anEntry ) ) {
	    aCltn += anActivity;
	}
    }
    return aCltn;
}



/*
 * Compute the service time for a client/server in the queueing network.
 */

double
Task::serviceTimeForQueueingNetwork( const unsigned k, chainTestFunc aFunc ) const
{
    double time = 0.0;
    double sum  = 0.0;
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	if ( (anEntry->*aFunc)( k ) ) {
	    const double tput = anEntry->throughput() ? anEntry->throughput() : 1.0;
	    time += tput * anEntry->serviceTimeForQueueingNetwork();
	    sum  += tput;
	}
    }

    /* Take mean time. */

    if ( sum ) {
	time = time / sum;
    }

    if ( !isSelected() ) {
	time += Flags::have_results ? (LQIO::DOM::to_double(copies()) - utilization()) / throughput() : 0.0;
    }

    return time;
}



/*
 * Clients need their slice time.
 */

double
Task::sliceTimeForQueueingNetwork( const unsigned k, chainTestFunc aFunc ) const
{
    return serviceTimeForQueueingNetwork( k, aFunc ) / (countCalls( &GenericCall::sumOfRendezvous ) + 1.);
}

/*
 * Sort activities (if any).
 */

unsigned
Task::generate()
{
    topologicalSort();

    Sequence<Activity *> nextActivity( activities() );
    Activity * anActivity;

    layers.grow( maxLevel );
    unsigned unused_level = 0;

    while ( anActivity = nextActivity() ) {
	unsigned i = anActivity->level();
	if ( i == 0 || !anActivity->reachable() ) {

	    /* If not reachable (connection error, stick on a layer at the bottom */

	    if ( unused_level == 0 ) {
		if ( graphical_output() ) {
		    layers[maxLevel].height( max( layers[maxLevel].height(), anActivity->height() + 20 ) );
		}
		maxLevel += 1;
		layers.grow( 1 );
		unused_level = maxLevel;
	    }
	    anActivity->level( unused_level );
	    layers[unused_level] << anActivity;

	} else {
	    layers[i] << anActivity;
	}
    }
    return maxLevel;
}



/*
 * Toplogical sort for activities.  It also aggregates activities.
 */

unsigned
Task::topologicalSort()
{
    maxLevel = 1;
    Sequence<Entry *>nextEntry( entries() );
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	Activity * anActivity = anEntry->startActivity();
	if ( !anActivity ) continue;

	const unsigned size = activities().size();
	Stack<const Activity *> activityStack( size );			// For checking for cycles.
	Stack<const AndForkActivityList *> forkStack( size );		// For matching forks and joins.
	try {
	    maxLevel = max( maxLevel, anActivity->findActivityChildren( activityStack, forkStack, anEntry, 0, 1, 1.0  ) );
	}
	catch ( activity_cycle& error ) {
	    maxLevel = max( maxLevel, error.depth()+1 );
	}
	catch ( bad_internal_join& error ) {
	    maxLevel = max( maxLevel, error.depth()+1 );
	}
    }
    return maxLevel;
}


/*
 * Layout the activities (if we have any).
 * moveTo will patch things up.
 */

Task&
Task::format()
{
    double aWidth = 0.0;

    entryList.sort( Entry::compare );

    /* Compute width of task.  Move entries */

    const double ty = myNode->origin.y() + height();

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    entryWidthPts = 0;
    while ( anEntry = nextEntry() ) {
	anEntry->moveTo( entryWidthPts + myNode->origin.x(), ty - anEntry->height() );
	entryWidthPts += anEntry->width() - adjustForSlope( fabs( anEntry->height() ) );
    }
    if ( Flags::graphical_output_style == JLQNDEF_STYLE ) {
	entryWidthPts += Flags::entry_width * 2;
    }


    if ( layers.size() ) {
	for ( unsigned i = 1; i <= layers.size(); ++i ) {
	    layers[i].sort( Activity::compare ).format( 0 );
	}

	const double x = myNode->origin.x() + adjustForSlope( fabs( height() ) ) + Flags::act_x_spacing / 4;

	/* Start from bottom and work up.  Reformat will realign the activities */

	double y = 0.0;
	for ( unsigned i = layers.size(); i > 0; --i ) {
	    layers[i].moveTo( x, myNode->origin.y() + y );
	    y += layers[i].height();  /* grow down */

 	    /* Shift up layer IFF we have to draw stuff below activities */
 	    if ( layers[i].height() > Flags::entry_height ) {
 		layers[i].moveBy( 0.0, layers[i].height() - Flags::entry_height );
 	    }
	}

	y += Flags::icon_height;
	if ( !(queueing_output() && Flags::flatten_submodel) ) {
	    myNode->extent.y( y );
	}

	/* Calculate the space needed for the activities */

	switch( Flags::activity_justification ) {
	case ALIGN_JUSTIFY:
	case DEFAULT_JUSTIFY:
	    aWidth = justifyByEntry();
	    break;
	default:
	    aWidth = justify();
	    break;
	}
	aWidth += adjustForSlope( 2.0 * y );	/* Center icons inside task. */
    }

    /* Modify extent  */

    aWidth = max( max( aWidth, entryWidthPts + adjustForSlope( height() ) ), width() );

    myNode->extent.x( aWidth );

    return *this;
}


/*
 * Layout the entries and activities (if we have any).
 */

Task&
Task::reformat()
{
    /* Move entries */

    entryList.sort( Entry::compare );

    const double x = myNode->origin.x() + adjustForSlope( height() );
    const double y = myNode->origin.y();
    const double offset = adjustForSlope( (height() - fabs(entryAt(1)->height())));
    const double fill = max( ((width() - adjustForSlope( height() )) - entryWidthPts) / (nEntries() + 1.0), 0.0 );

    /* Move entries */

    const double ty = y + height();
    double tx = myNode->origin.x() + offset + fill;

    /* Figure out which sides of the entries to draw */

    if ( fill == 0.0 ) {
	const unsigned int n = nEntries();
	for ( unsigned int i = 1; i <= n; ++i ) {
	    entryAt(i)->drawLeft  = false;
	    entryAt(i)->drawRight = ( i != n || Flags::graphical_output_style == JLQNDEF_STYLE );
	}
    }

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	anEntry->moveTo( tx, ty - anEntry->height() );
	tx += anEntry->width() - adjustForSlope( fabs( anEntry->height() ) ) + fill;
    }

    /* Move activities */

    if ( layers.size() ) {

	double ty = y;

	for ( unsigned i = layers.size(); i > 0; --i ) {
	    layers[i].moveTo( x, ty );
	    ty += layers[i].height();  /* grow down */

 	    if ( layers[i].height() > Flags::entry_height ) {
 		layers[i].moveBy( 0.0, layers[i].height() - Flags::entry_height );
 	    }
	}

	for ( unsigned i = 1; i <= layers.size(); ++i ) {
	    layers[i].sort( Activity::compare ).reformat( 0 );
	}

	switch( Flags::activity_justification ) {
	case ALIGN_JUSTIFY:
	case DEFAULT_JUSTIFY:
	    justifyByEntry();
	    break;
	default:
	    justify();
	    break;
	}
    }


    sort();		/* Reorder arcs */
    moveDst();
    moveSrc();		/* Move arc associated with processor */

    return *this;
}



/*
 * Move activities according to the justification type.
 */

double
Task::justify()
{
    double left  = MAXDOUBLE;
    double right = 0.0;

    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	left  = min( left,  layers[i].x() );
	right = max( right, layers[i].x() + layers[i].width() );
    }

    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	layers[i].justify( right - left, Flags::activity_justification );
    }

    return right - left;
}




/*
 * Move activities so they line up with the entries.
 */

double
Task::justifyByEntry()
{
    const unsigned MAX_LEVEL = layers.size();
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;
    double width = 0;

    /*
     * Find all activities reachable from this entry...
     * and stick them into the sublayer
     */

    for ( double x = left() + adjustForSlope( fabs( height() ) ) + Flags::act_x_spacing / 4.; anEntry = nextEntry(); x += Flags::print[X_SPACING].value.f ) {
	double right = 0.0;
	double left  = MAXDOUBLE;
	Activity * startActivity = anEntry->startActivity();
	if ( !startActivity ) continue;

	/* Sublayer will only contain activities reachable from current entry */

	Vector2<ActivityLayer> sublayer;
	sublayer.grow(MAX_LEVEL);

	for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
	    Activity * anActivity;
	    Sequence<Activity *> nextActivity( layers[i].activities() );
	    while ( anActivity = nextActivity() ) {
		if ( anActivity->reachedFrom() == startActivity ) {
		    sublayer[i] << anActivity;
		}
	    }
	}

	/* Now, move all activities for this entry together */

	for ( unsigned i = MAX_LEVEL; i > 0; --i ) {
	    if ( layers[i].size() == 0 ) continue;
	    sublayer[i].reformat( 0 );
	    right = max( right, sublayer[i].x() + sublayer[i].width() );
	    left  = min( left,  sublayer[i].x() );
	}

	/* Justify the current "slice", then move it to its column */

	for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
	    sublayer[i].justify( right - left, Flags::activity_justification ).moveBy( x, 0 );
	}

	if ( Flags::activity_justification == ALIGN_JUSTIFY ) {
	    double shift = 0;
	    for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
		sublayer[i].alignActivities();
		shift = max( x - sublayer[i].x(), shift );	// If we've moved left too far, we'll have to shift everything.
	    }
	    for ( unsigned i = 1; i <= MAX_LEVEL; ++i ) {
		if ( shift > 0 ) {
		    sublayer[i].moveBy( shift, 0 );
		}
		right = max( right, sublayer[i].x() + sublayer[i].width() - x );	// Don't forget to subtract x!
	    }
	}

	/* The next column starts here. */

	x += right;
	width = x;
    }

    return width - (left() + adjustForSlope( fabs( height() ) ) );
//    return width - left();
}



double
Task::alignActivities()
{
    double minLeft  = MAXDOUBLE;
    double maxRight = 0;

    for ( unsigned i = 1; i <= layers.size(); ++i ) {
	if ( !layers[i].size() ) continue;
	layers[i].alignActivities();
	minLeft  = min( minLeft, layers[i].x() );
	maxRight = max( maxRight, layers[i].x() + layers[i].width() );

    }
    return maxRight;
}


/*
 * Move the entity, it's entries and activities.  Don't recompute everything.
 */

Task&
Task::moveBy( const double dx, const double dy )
{
    myNode->moveBy( dx, dy );
    myLabel->moveBy( dx, dy );

    /* Move entries */

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	anEntry->moveBy( dx, dy );
    }

    /* Move activities */

    if ( layers.size() ) {
	for ( unsigned i = 1; i <= layers.size(); ++i ) {
	    layers[i].moveBy( dx, dy );
	}
	/* Second pass is needed because info from above and below is used. */
	for ( unsigned i = layers.size(); i >= 1; --i ) {
	    layers[i].moveBy( 0.0, 0.0 );
	}
    }

    sort();			/* Reorder arcs */
    moveDst();			/* Move arcs calling me.	      */
    moveSrc();			/* Move arc associated with processor */

    return *this;
}


/*
 * Move the entity, it's entries and activities.  Recompute everything.
 */

Task&
Task::moveTo( const double x, const double y )
{
    myNode->moveTo( x, y );

    reformat();

    if ( Flags::print[AGGREGATION].value.i == AGGREGATE_ENTRIES ) {
	myLabel->moveTo( bottomCenter() ).moveBy( 0, height() / 2 );
    } else if ( !queueing_output() ) {

	/* Move Label -- do after X extent recalculated */

	if ( Flags::graphical_output_style == JLQNDEF_STYLE ) {
	    myLabel->moveTo( topRight() ).moveBy( -Flags::entry_width, -entryAt(1)->height()/2 );
	} else if ( layers.size() ) {
	    myLabel->moveTo( topCenter() ).moveBy( 0, -entryAt(1)->height() - 10 );
	} else {
	    myLabel->moveTo( bottomCenter() ).moveBy( 0, height() / 5 );
	}
    } else {
	myLabel->moveTo( bottomCenter() ).moveBy( 0, height() / 5 );
    }

    return *this;
}


/*
 * Move the arc associated with the processor.  Offset the point
 * based on the location of the processor.
 * Invoked by the processor and by Task::moveTo().
 */

Task&
Task::moveSrc()
{
    if ( Flags::graphical_output_style == JLQNDEF_STYLE ) {
	Point aPoint = bottomRight();
	aPoint.moveBy( -Flags::entry_width, 0 );
	callList()[1]->moveSrc( aPoint );
    } else if ( callList().size() == 1 && dynamic_cast<ProcessorCall *>(callList()[1]) ) {
	Point aPoint = bottomCenter();
	double diff = aPoint.x() - processor()->center().x();
	if ( diff > 0 && fabs( diff ) > width() / 4 ) {
	    aPoint.moveBy( -width() / 4, 0 );
	} else if ( diff < 0 && fabs( diff ) > width() / 4 ) {
	    aPoint.moveBy( width() / 4, 0 );
	}
	callList()[1]->moveSrc( aPoint );
    } else {
	const int nFwd = countArcs( &GenericCall::hasForwardingLevel );
	Sequence<EntityCall *> nextCall( callList() );
	Point aPoint = bottomLeft();
	const double delta = width() / static_cast<double>(countArcs() + 1 - nFwd);
	EntityCall * srcCall;

	while ( srcCall = nextCall() ) {
	    if ( srcCall->isSelected()  && !srcCall->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		srcCall->moveSrc( aPoint );
	    }
	}
    }
    return *this;
}


/*
 * Move all arcs I sink.  This is only really applicable iff we're doing
 * -Ztasks-only.
 */

Task&
Task::moveDst()
{
    Sequence<GenericCall *> nextRefr( callerList() );
    GenericCall * dstCall;
    Point aPoint = myNode->topLeft();

    if ( Flags::print_forwarding_by_depth ) {
	const double delta = width() / static_cast<double>(countCallers() + 1);

	/* Draw other incomming arcs. */

	while ( dstCall = nextRefr() ) {
	    if ( dstCall->isSelected() ) {
		aPoint.moveBy( delta, 0 );
		dstCall->moveDst( aPoint );
	    }
	}
    } else {
	/*
	 * We add the outgoing forwarding arcs to the incomming side of the entry,
	 * so adjust the counts as necessary.
	 */

	const int nFwd = countArcs( &GenericCall::hasForwardingLevel );
	const double delta = width() / static_cast<double>(countCallers() + 1 + nFwd );
	const double fy = Flags::print[Y_SPACING].value.f / 2.0 + top();

	/* Draw incomming forwarding arcs first. */

	Point fwdPoint( left(), fy );
	while ( dstCall = nextRefr() ) {
	    if ( dstCall->isSelected() && dstCall->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		dstCall->moveDst( aPoint ).movePenultimate( fwdPoint );
		fwdPoint.moveBy( delta, 0 );
	    }
	}

	/* Draw other incomming arcs. */

	while ( dstCall = nextRefr() ) {
	    if ( dstCall->isSelected() && !dstCall->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		if ( dstCall->hasForwarding() ) {
		    dstCall->movePenultimate( aPoint ).moveSecond( aPoint );
		}
		dstCall->moveDst( aPoint );
	    }
	}

	/* Draw outgoing forwarding arcs */

	fwdPoint.moveTo( right(), fy );
	Sequence<EntityCall *> nextCall( callList() );
	EntityCall * srcCall;
	while ( srcCall = nextCall() ) {
	    if ( srcCall->isSelected() && srcCall->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		srcCall->moveSrc( aPoint ).moveSecond( fwdPoint );
		fwdPoint.moveBy( delta, 0 );
	    }
	}
    }

    return *this;
}


/*
 * Move the arc associated with the processor.  Offset the point
 * based on the location of the processor.
 * Invoked by the processor and by Task::moveTo().
 */

Task&
Task::moveSrcBy( const double dx, const double dy )
{
    Sequence<EntityCall *> nextCall( callList() );
    EntityCall * srcCall;

    while ( srcCall = nextCall() ) {
	if ( srcCall->isSelected() ) {
	    srcCall->moveSrcBy( dx, dy );
	}
    }
    return *this;
}



/*
 * Label the node.
 */

Entity&
Task::label()
{
    if ( queueing_output() ) {
	bool print_goop = false;
	if ( Flags::print[INPUT_PARAMETERS].value.b ) {
	    labelQueueingNetwork( &Entry::labelQueueingNetworkVisits );
	    print_goop = true;
	}
	if ( Flags::have_results && Flags::print[WAITING].value.b ) {
	    labelQueueingNetwork( &Entry::labelQueueingNetworkWaiting );
	    print_goop = true;
	}
	if ( print_goop ) {
	    myLabel->newLine();
	}
    }
    Entity::label();
    if ( Flags::print[INPUT_PARAMETERS].value.b ) {
	if ( queueing_output() ) {
	    if ( !isSelected() ) {
		const double Z = Flags::have_results ? (LQIO::DOM::to_double(copies()) - utilization()) / throughput() : 0.0;
		if ( Z > 0.0 ) {
		    myLabel->newLine() << " Z = " << Z;
		}
	    }
	    labelQueueingNetwork( &Entry::labelQueueingNetworkService );
	} else {
	    if ( Flags::print[AGGREGATION].value.i == AGGREGATE_ENTRIES && Flags::print[PRINT_AGGREGATE].value.b ) {
		myLabel->newLine() << " [" << print_service_time( *entryAt(1) ) << ']';
	    }
	    if ( hasThinkTime()  ) {
		*myLabel << " Z=" << dynamic_cast<ReferenceTask *>(this)->thinkTime();
	    }
	}

    }
    if ( Flags::have_results ) {
 	bool print_goop = false;
	if ( Flags::print[TASK_THROUGHPUT].value.b ) {
	    myLabel->newLine();
	    if ( throughput() == 0.0 && Flags::print[COLOUR].value.i != COLOUR_OFF ) {
		myLabel->colour( Graphic::RED );
	    }
	    *myLabel << begin_math( &Label::lambda ) << "=" << throughput();
	    print_goop = true;
	}
	if ( Flags::print[TASK_UTILIZATION].value.b ) {
	    if ( print_goop ) {
		*myLabel << ',';
	    } else {
		myLabel->newLine() << begin_math();
		print_goop = true;
	    }
	    *myLabel << _mu() << "=" << utilization();
	    if ( hasBogusUtilization() && Flags::print[COLOUR].value.i != COLOUR_OFF ) {
		myLabel->colour(Graphic::RED);
	    }
	}
	if ( print_goop ) {
	    *myLabel << end_math();
	}
    }

    /* Now label entries. */

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->label();
    }

    /* And the outgoing arcs, if any */

    Sequence<EntityCall *> nextCall(myCalls);
    EntityCall * aCall;
    while ( aCall = nextCall() ) {
	aCall->label();
    }

    for ( unsigned i = layers.size(); i > 0; --i ) {
	layers[i].label();
    }

    return *this;
}



/*
 * Stick labels in for queueing network.
 */

const Task&
Task::labelQueueingNetwork( entryLabelFunc aFunc ) const
{
    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	(anEntry->*aFunc)( *myLabel );
    }
    return *this;
}



/*
 * Move the entity and it's entries.
 */

Task&
Task::scaleBy( const double sx, const double sy )
{
    Entity::scaleBy( sx, sy );

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->scaleBy( sx, sy );
    }

    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	anActivity->scaleBy( sx, sy );
    }

    Sequence<ActivityList *> nextPrecedence(precedences());
    ActivityList * aPrecedence;
    while ( aPrecedence = nextPrecedence() ) {
	aPrecedence->scaleBy( sx, sy );
    }

    Sequence<EntityCall *> nextCall( callList() );
    EntityCall * aCall;
    while ( aCall = nextCall() ) {
	aCall->scaleBy( sx, sy );
    }

    return *this;
}



/*
 * Move the entity and it's entries.
 */

Task&
Task::depth( const unsigned depth )
{
    Entity::depth( depth );

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->depth( depth );
    }

    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	anActivity->depth( depth );
    }

    Sequence<ActivityList *> nextPrecedence(precedences());
    ActivityList * aPrecedence;
    while ( aPrecedence = nextPrecedence() ) {
	aPrecedence->depth( depth-1 );
    }
    Sequence<EntityCall *> nextCall( callList() );
    EntityCall * aCall;
    while ( aCall = nextCall() ) {
	aCall->depth( depth );
    }

    return *this;
}



/*
 * Move the entity and it's entries.
 */

Task&
Task::translateY( const double dy )
{
    Entity::translateY( dy );

    Sequence<Entry *> nextEntry(entries());
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	anEntry->translateY( dy );
    }

    Sequence<Activity *> nextActivity(activities());
    Activity * anActivity;
    while ( anActivity = nextActivity() ) {
	anActivity->translateY( dy );
    }


    Sequence<ActivityList *> nextPrecedence(precedences());
    ActivityList * aPrecedence;
    while ( aPrecedence = nextPrecedence() ) {
	aPrecedence->translateY( dy );
    }

    Sequence<EntityCall *> nextCall( callList() );
    EntityCall * aCall;
    while ( aCall = nextCall() ) {
	aCall->translateY( dy );
    }

    return *this;
}

#if defined(REP2FLAT)
Task& 
Task::removeReplication() 
{
    Entity::removeReplication();

    LQIO::DOM::Task * dom = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(getDOM()));
    for ( std::map<const std::string, unsigned int>::const_iterator fi = dom->getFanIns().begin(); fi != dom->getFanIns().end(); ++fi ) {
	dom->setFanIn( fi->first, 1 );
    }
    for ( std::map<const std::string, unsigned int>::const_iterator fo = dom->getFanOuts().begin(); fo != dom->getFanOuts().end(); ++fo ) {
	dom->setFanOut( fo->first, 1 );
    }

    return *this;
}


Task *
Task::expandTask( int replica ) const
{
    /* Get a pointer to the replicated processor */

    const Processor *aProcessor = Processor::find_replica( processor()->name(), replica );

    ostringstream aName;
    aName << name() << "_" << replica;
    set<Task *,ltTask>::const_iterator nextTask = find_if( task.begin(), task.end(), eqTaskStr( aName.str() ) );
    if ( nextTask != task.end() ) {
	string msg = "Task::expandTask(): cannot add symbol ";
	msg += aName.str();
	throw runtime_error( msg );
    }
    Task * aTask = clone( replica, aName.str(), aProcessor, share() );
    task.insert( aTask );

    const std::vector<LQIO::DOM::Entry *>& dom_entries = dynamic_cast<const LQIO::DOM::Task *>(aTask->getDOM())->getEntryList();

    Sequence<Entry *> nextEntry( aTask->entries() );
    Entry * anEntry;
    while ( anEntry = nextEntry() ) {
	LQIO::DOM::Entry * dom_entry = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(anEntry->getDOM()));
	const_cast<std::vector<LQIO::DOM::Entry *>*>(&dom_entries)->push_back( dom_entry );        /* Add to task. */
    }

    /* Handle group if necessary */

    if ( hasActivities() ) {
	aTask->expandActivities( *this, replica );
    }

    return aTask;
}


const Cltn<Entry *>& 
Task::groupEntries( int replica, Cltn<Entry *>& newEntryList  ) const
{
    Sequence<Entry *> nextEntry( entries() );
    Entry * anEntry;

    while ( anEntry = nextEntry() ) {
	newEntryList << Entry::find_replica( anEntry->name(), replica );
    }

    return newEntryList;
}



Task&
Task::expandActivities( const Task& src, int replica ) 
{
    /* clone the activities */
    LQIO::DOM::Task * task_dom = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(getDOM()));

    const Activity *srcActivity;
    Sequence<Activity *> nextActivity(src.activities());
    while ( srcActivity = nextActivity() ) {
	addActivity(*srcActivity, replica);
    }

    /* Now reconnect the precedences.  Do the join list from the task.  We have to connect the fork list. */

    const ActivityList *srcPrecedence;
    Sequence<ActivityList *> nextPrecedence(src.precedences());
    while ( srcPrecedence = nextPrecedence() ) {
	if ( !dynamic_cast<const JoinActivityList *>(srcPrecedence) &&  !dynamic_cast<const AndOrJoinActivityList *>(srcPrecedence) ) continue;
	
	/* Ok we have the join list. */

	ActivityList * pre_list = srcPrecedence->clone();
	LQIO::DOM::ActivityList * pre_list_dom = const_cast<LQIO::DOM::ActivityList *>(pre_list->getDOM());
	task_dom->addActivityList( pre_list_dom );

	/* Now reconnect the activities. A little kludgey because we have a list in one case and not the other. */

	Cltn<Activity *> pre_act_list;
	if (dynamic_cast<const ForkJoinActivityList *>(srcPrecedence)) {
	    pre_act_list = dynamic_cast<const ForkJoinActivityList *>(srcPrecedence)->getMyActivityList();
	} else if (dynamic_cast<const SequentialActivityList *>(srcPrecedence)) {
	    pre_act_list += dynamic_cast<const SequentialActivityList *>(srcPrecedence)->getMyActivity();
	}

	Sequence<Activity *> nextActivity(pre_act_list);
	while ( srcActivity = nextActivity() ) {
	    Activity * pre_activity = findActivity(*srcActivity, replica);
	    LQIO::DOM::Activity * pre_activity_dom = const_cast<LQIO::DOM::Activity *>(dynamic_cast<const LQIO::DOM::Activity *>(pre_activity->getDOM()));
	    pre_activity->outputTo( pre_list );
	    pre_activity_dom->outputTo( pre_list_dom );
	    pre_list->add( pre_activity );
	    pre_list_dom->add( pre_activity_dom );
	}

	const ActivityList *dstPrecedence = srcPrecedence->next();

	if ( dstPrecedence ) {

	    /* Here is the fork list */

	    ActivityList * post_list = dstPrecedence->clone();
	    LQIO::DOM::ActivityList * post_list_dom = const_cast<LQIO::DOM::ActivityList *>(post_list->getDOM());
	    task_dom->addActivityList( post_list_dom );
	    ActivityList::act_connect( pre_list, post_list );
	    pre_list_dom->setNext( post_list_dom );
	    post_list_dom->setPrevious( pre_list_dom );

	    /* Now reconnect the activities. A little kludgey because we have a list in one case and not the other. */

	    Cltn<Activity *> post_act_list;
	    if (dynamic_cast<const ForkJoinActivityList *>(dstPrecedence)) {
		post_act_list = dynamic_cast<const ForkJoinActivityList *>(dstPrecedence)->getMyActivityList();
	    } else if (dynamic_cast<const RepeatActivityList *>(dstPrecedence)) {
		post_act_list = dynamic_cast<const RepeatActivityList *>(dstPrecedence)->getMyActivityList();
		post_act_list += dynamic_cast<const RepeatActivityList *>(dstPrecedence)->getMyActivity();
	    } else if (dynamic_cast<const SequentialActivityList *>(dstPrecedence)) {
		post_act_list += dynamic_cast<const SequentialActivityList *>(dstPrecedence)->getMyActivity();
	    }

	    Sequence<Activity *> nextActivity(post_act_list);
	    while ( srcActivity = nextActivity() ) {
		Activity * post_activity = findActivity(*srcActivity, replica);
		LQIO::DOM::Activity * post_activity_dom = const_cast<LQIO::DOM::Activity *>(dynamic_cast<const LQIO::DOM::Activity *>(post_activity->getDOM()));
		post_activity->inputFrom( post_list );
		post_activity_dom->inputFrom( post_list_dom );
		post_list->add( post_activity );
		
		/* A little tricky here.  We need to copy over whatever parameter there was. */
		
		const LQIO::DOM::ActivityList * dstPrecedenceDOM = dstPrecedence->getDOM();
		LQIO::DOM::ExternalVariable * parameter = dstPrecedenceDOM->getParameter( dynamic_cast<const LQIO::DOM::Activity *>(srcActivity->getDOM()) );
		post_list_dom->add( post_activity_dom, parameter );
	    }

	}
    }
    return *this;
}



LQIO::DOM::Task *
Task::cloneDOM( const string& aName, LQIO::DOM::Processor * dom_processor ) const
{
    LQIO::DOM::Task * dom_task = new LQIO::DOM::Task( *dynamic_cast<const LQIO::DOM::Task*>(getDOM()) );

    dom_task->setName( aName );
    dom_task->setProcessor( dom_processor );
    const_cast<LQIO::DOM::Document *>(getDOM()->getDocument())->addTaskEntity( dom_task );	    /* Reconnect all of the dom stuff. */
    dom_processor->addTask( dom_task );

    return dom_task;
}
#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                 */
/* ------------------------------------------------------------------------ */

/*
 * Draw the SRVN model object.
 */

ostream&
Task::draw( ostream& output ) const
{
    ostringstream aComment;
    aComment << "Task " << name()
	     << task_scheduling_of( *this )
	     << entries_of( *this );
    if ( processor() ) {
	aComment << " " << processor()->name();
    }
#if defined(BUG_375)
    aComment << " span=" << span() << ", index=" << index();
#endif
    myNode->comment( output, aComment.str() );
    myNode->fillColour( colour() );
    if ( Flags::print[COLOUR].value.i == COLOUR_OFF ) {
	myNode->penColour( Graphic::DEFAULT_COLOUR );			// No colour.
    } else if ( throughput() == 0.0 ) {
	myNode->penColour( Graphic::RED );
    } else if ( colour() == Graphic::GREY_10 ) {
	myNode->penColour( Graphic::BLACK );
    } else {
	myNode->penColour( colour() );
    }

    Point points[4];
    const double dx = adjustForSlope( fabs(height()) );
    points[0] = topLeft().moveBy( dx, 0 );
    points[1] = bottomLeft();
    points[2] = bottomRight().moveBy( -dx, 0 );
    points[3] = topRight();

    if ( isMultiServer() || isInfinite() || isReplicated() ) {
	Point copies[4];
	const double delta = -2.0 * Model::scaling() * myNode->direction();
	for ( int i = 0; i < 4; ++i ) {
	    copies[i] = points[i];
	    copies[i].moveBy( 2.0 * Model::scaling(), delta );
	}
	const int aDepth = myNode->depth();
	myNode->depth( aDepth + 1 );
	myNode->polygon( output, 4, copies );
	myNode->depth( aDepth );
    }
    myNode->polygon( output, 4, points );

    myLabel->backgroundColour( colour() ).comment( output, aComment.str() );
    output << *myLabel;

    if ( Flags::print[AGGREGATION].value.i != AGGREGATE_ENTRIES ) {

	/* Print out entries */

	Sequence<Entry *> nextEntry(entries());
	Entry * anEntry;

	while ( anEntry = nextEntry() ) {
	    output << *anEntry;
	}

	if ( hasActivities() ) {
	    Sequence<Activity *> nextActivity(activities());
	    Activity * anActivity;

	    while ( anActivity = nextActivity() ) {
		output << *anActivity;
	    }
	}

    }

    Sequence<EntityCall *> nextCall( callList() );
    EntityCall * aCall;
    while ( aCall = nextCall() ) {
	if ( aCall->isSelected() ) {
	    output << *aCall;
	}
    }

    return output;
}

/*
 * Draw the queueing model object.
 */

ostream&
Task::drawClient( ostream& output, const bool is_in_open_model, const bool is_in_closed_model ) const
{
    string aComment;
    aComment += "========== ";
    aComment += name();
    aComment += " ==========";
    myNode->comment( output, aComment );
    myNode->penColour( colour() == Graphic::GREY_10 ? Graphic::BLACK : colour() ).fillColour( colour() );

    myLabel->moveTo( bottomCenter() ).justification( LEFT_JUSTIFY );
    if ( is_in_open_model && is_in_closed_model ) {
	Point aPoint = bottomCenter();
	aPoint.moveBy( radius() * -3.0, 0 );
	myNode->multi_server( output, aPoint, radius() );
	aPoint = bottomCenter().moveBy( radius() * 1.5, 0 );
	myNode->open_source( output, aPoint, radius() );
	myLabel->moveBy( radius() * 1, radius() * 3 * myNode->direction() );
    } else if ( is_in_open_model ) {
	myNode->open_source( output, bottomCenter(), radius() );
	myLabel->moveBy( radius() * 1, radius() * myNode->direction() );
    } else {
	myNode->multi_server( output, bottomCenter(), radius() );
	myLabel->moveBy( radius() * 1, radius() * 3 * myNode->direction() );
    }
    output << *myLabel;
    return output;
}


#if defined(QNAP_OUTPUT)
/*
 * QNAP queueing network.
 */

ostream&
Task::printQNAPClient( ostream& output, const bool is_in_open_model, const bool is_in_closed_model, const bool multi_class ) const
{
    output << indent(0) << "name = " << name() << ";" << endl;
    if ( is_in_closed_model ) {
	output << indent(0) << "type = infinite;" << endl;
	for ( unsigned i = 1; i <= myClientClosedChains.size(); ++i ) {
	    output << indent(0) << "service" << closed_chain( *this, multi_class, i )
		   << " = exp(" << sliceTimeForQueueingNetwork( myClientClosedChains[i], &Element::hasClientClosedChain )
		   << ");" << endl;
	    output << indent(0) << "init" << closed_chain( *this, multi_class, i ) << " = " << copies() << ";" << endl;
	}
    } else if ( is_in_open_model ) {
	output << indent(0) << "type = source;" << endl;
    }
    printQNAPRequests( output, is_in_open_model, is_in_closed_model, multi_class );
    return output;
}



/*
 * QNAP queueing network.
 * Modify me for BUG_182.
 */

ostream&
Task::printQNAPRequests( ostream& output, const bool is_in_open_model, const bool is_in_closed_model, const bool multi_class ) const
{
    Sequence<EntityCall *> nextCall( myCalls );

    if ( is_in_closed_model ) {
	printQNAPRequests( output, nextCall, multi_class, &closed_chain, &GenericCall::sumOfRendezvous );
    }
    if ( is_in_open_model ) {
	printQNAPRequests( output, nextCall, multi_class, &open_chain, &GenericCall::sumOfSendNoReply );
    }
    return output;
}


ostream&
Task::printQNAPRequests( ostream& output, Sequence<EntityCall *> &nextCall,
			 const bool multi_class, QNAP_Element_func chain_func, callFunc2 call_func ) const
{
    bool first = true;
    EntityCall * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() ) {
	    continue;
	} else if ( first ) {
	    output << indent(0) << "transit" << (*chain_func)( *this, multi_class, 1 ) << " = ";
	    first = false;
	} else {
	    output << "," << endl << indent(0) << "    ";		/* Start a new line.  */
	}
	output << aCall->dstName();
	if ( aCall->fanOut() > 1 ) {
	    output << "(1 step 1 until " << aCall->fanOut() << "), "
		   << qnap_visits( *aCall )
		   << "(1 step 1 until " << aCall->fanOut() << ")";
	} else if ( aCall->isProcessorCall() ) {
	    output << "," << countCalls( &GenericCall::sumOfRendezvous ) + 1.0;
	} else {
	    output << "," << (aCall->*call_func)();
	}
    }
    if ( !first ) {
	output << ";" << endl;
    }
    return output;
}
#endif


#if defined(PMIF_OUTPUT)
/*
 * PMIF queueing network.
 */

ostream&
Task::printPMIFClient( ostream& output ) const
{
    output << indent(+1) << "<ClosedWorkload WorkloadName=\"" << closed_chain( *this, true, 1 )
	   << "\" NumberOfJobs=\"" << copies()
	   << "\" TimeUnits=\"" << "sec"
	   << "\" ThinkTime=\"" << sliceTimeForQueueingNetwork( myClientClosedChains[1], &Element::hasClientClosedChain )
	   << "\" ThinkDevice=\"" << name()
	   << "\">" << endl;
    printPMIFRequests( output );
    output << indent(-1) << "</ClosedWorkload>" << endl;
    return output;
}



/*
 * PMIF queueing network.
 */

ostream&
Task::printPMIFArcs( ostream& output ) const
{
    Sequence<EntityCall *> nextCall( myCalls );
    EntityCall * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() ) continue;
	output << indent(0) << "<Arc FromNode=\"" << aCall->srcName()
	       << "\" ToNode=\"" << aCall->dstName() << "\"/>" << endl;
	output << indent(0) << "<Arc FromNode=\"" << aCall->dstName()
	       << "\" ToNode=\"" << aCall->srcName() << "\"/>" << endl;
    }
    return output;
}


ostream&
Task::printPMIFRequests( ostream& output ) const
{
    const double nCalls = countCalls( &GenericCall::sumOfRendezvous );
    if ( nCalls == 0.0 ) return output;

    Sequence<EntityCall *> nextCall( myCalls );
    EntityCall * aCall;

    while ( aCall = nextCall() ) {
	if ( !aCall->isSelected() || !aCall->hasRendezvous() ) continue;
	output << indent(0) << "<Transit To=\"" << aCall->dstName()
	       << "\" Probability=\"" << aCall->sumOfRendezvous() / nCalls
	       << "\"/>" << endl;
    }
    return output;
}
#endif

/* ------------------------- Reference Tasks -------------------------- */

ReferenceTask::ReferenceTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& entries )
    : Task( dom, aProc, aShare, entries )
{
}


ReferenceTask *
ReferenceTask::clone( unsigned int replica, const string& aName, const Processor * aProcessor, const Share * aShare ) const
{
    LQIO::DOM::Task * dom_task = cloneDOM( aName, const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(aProcessor->getDOM()) ) );

//	.setGroup( aShare->getDOM() );

    Cltn<Entry *> entries;
    return new ReferenceTask( dom_task, aProcessor, aShare, groupEntries( replica, entries ) );
}


bool
ReferenceTask::hasThinkTime() const 
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->hasThinkTime();
}

LQIO::DOM::ExternalVariable& 
ReferenceTask::thinkTime() const
{
    return *dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getThinkTime();
}


/*
 * Reference tasks are always fully utilized, but never a performance
 * problem, so always draw them black.
 */

Graphic::colour_type
ReferenceTask::colour() const
{
    switch ( Flags::print[COLOUR].value.i ) {
    case COLOUR_SERVER_TYPE:
	return Graphic::RED;

    case COLOUR_RESULTS:
	return processor()->colour();
	break;

    default:
	return Entity::colour();
    } 
}


/*
 * Recursively find all children and grand children from `father'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

unsigned
ReferenceTask::findChildren( CallStack& callStack, const unsigned directPath )
{
    unsigned max_depth = max( callStack.size(), level() );

    setLevel( max_depth ).addPath( directPath );

    if ( processor() ) {
	max_depth = max( max_depth + 1, processor()->level() );
	const_cast<Processor *>(processor())->setLevel( max_depth ).addPath( directPath );
    }

    Sequence<Entry *> nextEntry(entries());
    Entry *srcEntry;

    while ( srcEntry = nextEntry() ) {
	max_depth = max( max_depth, srcEntry->findChildren( callStack, directPath ) );
    }

    return max_depth;
}


bool
Task::canConvertToReferenceTask() const
{
    return Flags::convert_to_reference_task
      && (submodel_output()
#if HAVE_REGEX_T
	  || Flags::print[INCLUDE_ONLY].value.r
#endif
      )
      && !isSelected()
      && !hasOpenArrivals()
      && !isInfinite()
      && nEntries() == 1
      && processor();
}

/* --------------------------- Server Tasks --------------------------- */

ServerTask::ServerTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& entries )
    : Task( dom, aProc, aShare, entries )
{
}


ServerTask*
ServerTask::clone( unsigned int replica, const string& aName, const Processor * aProcessor, const Share * aShare ) const
{
    LQIO::DOM::Task * dom_task = cloneDOM( aName, const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(aProcessor->getDOM()) ) );

//	.setGroup( aShare->getDOM() );

    Cltn<Entry *> entries;
    return new ServerTask( dom_task, aProcessor, aShare, groupEntries( replica, entries ) );
}


/*
 * If we are converting a model and simplifying, in some instances we
 * can convert a server to a reference task.
 */

bool
ServerTask::canConvertToReferenceTask() const
{
    return Flags::convert_to_reference_task
      && (submodel_output()
#if HAVE_REGEX_T
	  || Flags::print[INCLUDE_ONLY].value.r
#endif
	  )
      && !isSelected()
      && !hasOpenArrivals()
      && !isInfinite()
      && nEntries() == 1
      && processor();
}

/*+ BUG_164 */
/* -------------------------- Semaphore Tasks ------------------------- */

SemaphoreTask::SemaphoreTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& entries )
    : Task( dom, aProc, aShare, entries )
{
}


SemaphoreTask*
SemaphoreTask::clone( unsigned int replica, const string& aName, const Processor * aProcessor, const Share * aShare ) const
{
    LQIO::DOM::Task * dom_task = cloneDOM( aName, const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(aProcessor->getDOM()) ) );

    Cltn<Entry *> entries;
    return new SemaphoreTask( dom_task, aProcessor, aShare, groupEntries( replica, entries ) );
}


/* -------------------------- RWLOCK Tasks ------------------------- */

RWLockTask::RWLockTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const Cltn<Entry *>& entries )
    : Task( dom, aProc, aShare, entries )
{
}


RWLockTask*
RWLockTask::clone( unsigned int replica, const string& aName, const Processor * aProcessor, const Share * aShare ) const
{
    LQIO::DOM::Task * dom_task = cloneDOM( aName, const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(aProcessor->getDOM()) ) );

    Cltn<Entry *> entries;
    return new RWLockTask( dom_task, aProcessor, aShare, groupEntries( replica, entries ) );
}


/*----------------------------------------------------------------------*/
/*		 	   Called from yyarse.  			*/
/*----------------------------------------------------------------------*/

/*
 * Add a task to the model.  Called by the parser.
 */

Task *
Task::create( const LQIO::DOM::Task* domTask, Cltn<Entry *>& entries )
{
    /* Recover the old parameter information that used to be passed in */
    const char* task_name = domTask->getName().c_str();
    const LQIO::DOM::Group * domGroup = domTask->getGroup();
    const scheduling_type sched_type = domTask->getSchedulingType();
    
    if ( !task_name || strlen( task_name ) == 0 ) abort();

    if ( entries.size() == 0 ) {
	LQIO::input_error2( LQIO::ERR_NO_ENTRIES_DEFINED_FOR_TASK, task_name );
	return 0;
    }
    if ( find_if( task.begin(), task.end(), eqTaskStr( task_name ) ) != task.end() ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Task", task_name );
	return 0;
    }

    const string& processor_name = domTask->getProcessor()->getName();
    Processor * aProcessor = Processor::find( processor_name );
    if ( !aProcessor ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, processor_name.c_str() );
    }

    const Share * aShare = 0;
    if ( !domGroup && aProcessor->scheduling() == SCHEDULE_CFS ) {
	LQIO::input_error2( LQIO::ERR_NO_GROUP_SPECIFIED, task_name, processor_name.c_str() );
    } else if ( domGroup ) {
	set<Share *,ltShare>::const_iterator nextShare = find_if( aProcessor->shares().begin(), aProcessor->shares().end(), eqShareStr( domGroup->getName() ) );
	if ( nextShare == aProcessor->shares().end() ) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domGroup->getName().c_str() );
	} else {
	    aShare = *nextShare;
	}
    }

    /* Pick-a-task */

    Task * aTask;
    switch ( sched_type ) {
    case SCHEDULE_UNIFORM:
    case SCHEDULE_BURST:
    case SCHEDULE_CUSTOMER:
	aTask = new ReferenceTask( domTask, aProcessor, aShare, entries );
	break;

    case SCHEDULE_FIFO:
    case SCHEDULE_PPR:
    case SCHEDULE_HOL:
    case SCHEDULE_DELAY:
	aTask = new ServerTask( domTask, aProcessor, aShare, entries );
	break;

    case SCHEDULE_SEMAPHORE:
	if ( entries.size() != 2 ) {
	    ::LQIO::input_error2( LQIO::ERR_ENTRY_COUNT_FOR_TASK, task_name, entries.size(), 2 );
	}
	aTask = new SemaphoreTask( domTask, aProcessor, aShare, entries );
	break;
		
	case SCHEDULE_RWLOCK:
	if ( entries.size() != N_RWLOCK_ENTRIES ) {
	    ::LQIO::input_error2( LQIO::ERR_ENTRY_COUNT_FOR_TASK, task_name, entries.size(), N_RWLOCK_ENTRIES );
	}
	aTask = new RWLockTask( domTask, aProcessor, aShare, entries );
	break;

    default:
	LQIO::input_error2( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_type_str[(unsigned)sched_type], "task", task_name );
	aTask = new ServerTask( domTask, aProcessor, aShare, entries );
	break;
    }

    task.insert( aTask );
    return aTask;
}

/* ---------------------------------------------------------------------- */

bool 
ltTask::operator()(const Task * t1, const Task * t2) const 
{ 
    return t1->name() < t2->name(); 
}

static ostream&
entries_of_str( ostream& output, const Task& aTask )
{
    BackwardsSequence<Entry *> nextEntry( aTask.entries() );
    const Entry * anEntry;
    while( anEntry = nextEntry() ) {
	if ( anEntry->pathTest() ) {
	    output << " " << anEntry->name();
	}
    }
    output << " -1";
    return output;
}

/*
 * Print out of scheduling flag field.  See ../lqio input.h
 */

static ostream&
task_scheduling_of_str( ostream& output, const Task & aTask )
{
    output << ' ';
    switch ( aTask.scheduling() ) {
    case SCHEDULE_BURST:     output << 'b'; break;
    case SCHEDULE_CUSTOMER:  output << 'r'; break;
    case SCHEDULE_DELAY:     output << 'n'; break;
    case SCHEDULE_FIFO:	     output << 'n'; break;
    case SCHEDULE_HOL:	     output << 'h'; break;
    case SCHEDULE_POLL:      output << 'P'; break;
    case SCHEDULE_PPR:	     output << 'p'; break;
    case SCHEDULE_RWLOCK:    output << 'W'; break;
    case SCHEDULE_SEMAPHORE: 
    case SCHEDULE_SEMAPHORE_R: 
	if ( dynamic_cast<const LQIO::DOM::SemaphoreTask *>(aTask.getDOM())->getInitialState() == LQIO::DOM::SemaphoreTask::INITIALLY_EMPTY ) {
	    output << 'Z'; break;
	} else {
	    output << 'S'; break;
	}
	break;
    case SCHEDULE_UNIFORM:   output << 'u'; break;
    default:	   	     output << '?'; break;
    }
    return output;
}


