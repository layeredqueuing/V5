/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqn2ps/task.cc $
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
 * $Id: task.cc 15735 2022-06-30 03:18:14Z greg $
 * ------------------------------------------------------------------------
 */

#include "lqn2ps.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include <lqio/dom_activity.h>
#include <lqio/dom_actlist.h>
#include <lqio/dom_document.h>
#include <lqio/dom_entry.h>
#include <lqio/dom_extvar.h>
#include <lqio/dom_group.h>
#include <lqio/dom_processor.h>
#include <lqio/dom_task.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include <lqio/labels.h>
#include <lqio/../../srvn_gram.h>
#include <lqio/srvn_output.h>
#include "activity.h"
#include "actlist.h"
#include "call.h"
#include "entry.h"
#include "errmsg.h"
#include "label.h"
#include "model.h"
#include "pragma.h"
#include "processor.h"
#include "share.h"
#include "task.h"

std::set<Task *,LT<Task> > Task::__tasks;
std::map<std::string,unsigned> Task::__key_table;		/* For squishName 	*/
std::map<std::string,std::string> Task::__symbol_table;		/* For rename		*/
const std::string ReferenceTask::__BCMP_station_name("terminal");	/* No more than 8 characters -- qnap2 limit. */

bool Task::thinkTimePresent             = false;
bool Task::holdingTimePresent           = false;
bool Task::holdingVariancePresent       = false;

const double Task::JLQNDEF_TASK_BOX_SCALING = 1.2;

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

class SRVNTaskManip {
public:
    SRVNTaskManip( std::ostream& (*ff)(std::ostream&, const Task & ), const Task & theTask ) : f(ff), aTask(theTask) {}
private:
    std::ostream& (*f)( std::ostream&, const Task& );
    const Task & aTask;

    friend std::ostream& operator<<(std::ostream & os, const SRVNTaskManip& m ) { return m.f(os,m.aTask); }
};


static std::ostream& entries_of_str( std::ostream& output,  const Task& aTask );
static inline SRVNTaskManip entries_of( const Task& aTask ) { return SRVNTaskManip( entries_of_str, aTask ); }

/* -------------------------- Constructor ----------------------------- */

Task::Task( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const std::vector<Entry *>& entries )
    : Entity( dom, __tasks.size()+1 ),
      _entries(entries),
      _activities(),
      _precedences(),
      _processors(),
      _share(aShare),
      _maxPhase(0),
      _entryWidthInPts(0),
      _key_table(),			/* Squish name - activities 	*/
      _symbol_table()			/* Squish name - activities 	*/
{
    for_each( _entries.begin(), _entries.end(), Exec1<Entry,const Task *>( &Entry::owner, this ) );

    if ( aProc ) {
	_processors.insert(aProc);
	ProcessorCall * aCall = new ProcessorCall(this,aProc);
	_calls.push_back(aCall);
	const_cast<Processor *>(aProc)->addDstCall( aCall );
	const_cast<Processor *>(aProc)->addTask( this );
    }

    _node = Node::newNode( Flags::icon_width, Flags::graphical_output_style == Output_Style::TIMEBENCH ? Flags::icon_height : Flags::entry_height );
    _label = Label::newLabel();
}



Task::~Task()
{
    std::for_each( calls().begin(), calls().end(), Delete<EntityCall *> );
    std::for_each( activities().begin(), activities().end(), Delete<Activity *> );
    std::for_each( precedences().begin(), precedences().end(), Delete<ActivityList *> );
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
}



/* ------------------------ Instance Methods -------------------------- */


bool
Task::hasProcessor( const Processor * processor ) const
{
    return std::find( _processors.begin(), _processors.end(), processor ) != _processors.end();
}


bool
Task::hasPriority() const
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->hasPriority();
}


/*
 * Find the processor allocated to this task.
 */

const Processor *
Task::processor() const
{
    const LQIO::DOM::Processor * dom = dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getProcessor();
    for ( std::set<const Processor *>::const_iterator processor = _processors.begin(); processor != _processors.end(); ++processor ) {
	if ( (*processor)->getDOM() == dom ) return *processor;
    }
    return nullptr;
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

Task&
Task::rename()
{
    const std::string old_name = name();
    std::ostringstream new_name;
    new_name << "t" << elementId();
    const_cast<LQIO::DOM::DocumentObject *>(getDOM())->setName( new_name.str() );
    for_each( activities().begin(), activities().end(), Exec<Element>( &Element::rename ) );

    renameFanInOut( old_name, name() );
    return *this;
}


Task&
Task::squish( std::map<std::string,unsigned>& key_table, std::map<std::string,std::string>& symbol_table )
{
    const std::string old_name = name();
    Element::squish( key_table, symbol_table );
    for_each( activities().begin(), activities().end(), Exec2<Element,std::map<std::string,unsigned>&,std::map<std::string,std::string>&>( &Element::squish, _key_table, _symbol_table ) );

    renameFanInOut( old_name, name() );
    return *this;
}


/* 
 * fix fan in and outs...  Have to find all references to this task the hard way.
 */
    
void
Task::renameFanInOut( const std::string& old_name, const std::string& new_name )
{
    const LQIO::DOM::Document * document = getDOM()->getDocument();		// Find root
    const std::map<std::string,LQIO::DOM::Task*>& tasks = document->getTasks();	// Get all tasks for remap
    for ( std::map<std::string,LQIO::DOM::Task*>::const_iterator t = tasks.begin(); t != tasks.end(); ++t ) {
	const LQIO::DOM::Task * task = t->second;
	renameFanInOut( const_cast<std::map<const std::string,const LQIO::DOM::ExternalVariable *>&>(task->getFanIns()), old_name, new_name );
	renameFanInOut( const_cast<std::map<const std::string,const LQIO::DOM::ExternalVariable *>&>(task->getFanOuts()), old_name, new_name );
    }
}


void
Task::renameFanInOut( std::map<const std::string,const LQIO::DOM::ExternalVariable *>& fan_ins_or_outs, const std::string& old_name, const std::string& new_name )
{
    std::map<const std::string,const LQIO::DOM::ExternalVariable *>::iterator index = fan_ins_or_outs.find( old_name );
    if ( index != fan_ins_or_outs.end() ) {
	const LQIO::DOM::ExternalVariable * value = index->second;
	fan_ins_or_outs.erase( index );						// Remove old item.
	fan_ins_or_outs.insert( std::pair<const std::string,const LQIO::DOM::ExternalVariable *>(new_name, value) );
    }
}



/*
 * Aggregate activities to activities and/or activities to phases.  If
 * activities are left after aggregation, we will have to recompute
 * their level because there likely is a lot less of them to draw.
 * Aggregating to a task merges everything up to an entry.  The actual
 * aggregation is done by submodel in layer.cc
 */

Task&
Task::aggregate()
{
    for_each( entries().begin(), entries().end(), Exec<Entry>( &Entry::aggregate ) );

    switch ( Flags::aggregation() ) {
    case Aggregate::ENTRIES:
    case Aggregate::PHASES:
    case Aggregate::ACTIVITIES:
	for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	    delete *activity;
	}
	for ( std::vector<ActivityList *>::const_iterator precedence = precedences().begin();  precedence != precedences().end(); ++precedence ) {
	    delete *precedence;
	}
	const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(getDOM()))->deleteActivities();
	const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(getDOM()))->deleteActivityLists();
	_activities.clear();
	_layers.clear();
	_precedences.clear();
	break;

    default:
	/* Recompute levels. */
	for_each( activities().begin(), activities().end(), Exec<Activity>( &Activity::resetLevel ) );
	generate();
	break;
    }

    return *this;
}


/*
 * Sort entries and activities based on when they were visited.
 */

Task&
Task::sort()
{
    std::sort( _calls.begin(), _calls.end(), Call::compareSrc );
    Entity::sort();
    return *this;
}


double
Task::getIndex() const
{
    double anIndex = std::numeric_limits<double>::max();

    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	anIndex = std::min( anIndex, (*entry)->getIndex() );
    }

    return anIndex;
}


int
Task::span() const
{
    if ( Flags::layering() == Layering::GROUP ) {
	std::vector<Entity *> myServers;
	return servers( myServers );		/* Force those making calls to lower levels right */
    }
    return 0;
}


/*
 * Set the chain of this client task to curr_k.  Chains will be set to
 * all servers of this client.  next_k will be changed iff there are
 * forks.
 */

unsigned
Task::setChain( unsigned curr_k, callPredicate aFunc  ) const
{
    unsigned next_k = curr_k;

    /*
     * I will have to find all servers to this client (including the
     * processor)...  because each server will get it's own chain.
     * Threads will complicate matters, bien sur.
     */

    if ( isReplicated() ) {
	std::vector<Entity *> servers;
	this->servers( servers );

	std::sort( servers.begin(), servers.end(), &Entity::compareCoord );

	for ( std::vector<Entity *>::const_iterator server = servers.begin(); server != servers.end(); ++server ) {
	    if ( !(*server)->isSelected() ) continue;
	    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
		next_k = (*entry)->setChain( curr_k, next_k, (*server), aFunc );
	    }
	    curr_k = next_k + 1;
	    next_k = curr_k;
	}

    } else {

	for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	    next_k = (*entry)->setChain( curr_k, next_k, 0, aFunc );
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
    for ( std::vector<EntityCall *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	const Processor * aProcessor = dynamic_cast<const Processor *>((*call)->dstEntity());
	if ( aProcessor ) {
	    if ( aProcessor->isInteresting() && aProcessor->isSelected() ) {
		const_cast<Processor *>(aProcessor)->setServerChain( k );
	    }
	    continue;
	}
	const Task * aTask = dynamic_cast<const Task *>((*call)->dstEntity());
	if ( aTask ) {
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
    _entries.push_back(anEntry);
    return *this;
}


Task&
Task::removeEntry( Entry * anEntry )
{
    std::vector<Entry *>::iterator pos = find_if( _entries.begin(), _entries.end(), EQ<Element>(anEntry) );
    if ( pos != _entries.end() ) {
	_entries.erase(pos);
    }
    return *this;
}


/*
 * Add a task to the list of tasks for this processor and set local index
 * for MVA.
 */

Activity *
Task::findActivity( const std::string& name ) const
{
    const std::vector<Activity *>::const_iterator activity = find_if( activities().begin(), activities().end(), EQStr<Activity>( name ) );
    return activity != activities().end() ? *activity : nullptr;
}


Activity *
Task::findOrAddActivity( const LQIO::DOM::Activity * activity )
{
    Activity * anActivity = findActivity( activity->getName() );

    if ( !anActivity ) {
	anActivity = new Activity( this, activity );
	_activities.push_back( anActivity );
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
    std::ostringstream aName;
    aName << srcActivity.name() << "_" << replica;

    return findActivity( aName.str() );
}


/*
 * Find an existing activity which has the same name as srcActivity.  If the activity does not exist, create one and copy it from the src.
 */

Activity *
Task::addActivity( const Activity& srcActivity, const unsigned replica )
{
    std::ostringstream srcName;
    srcName << srcActivity.name() << "_" << replica;

    Activity * dstActivity = findActivity( srcName.str() );
    if ( dstActivity ) return dstActivity;	// throw error?

    LQIO::DOM::Task * dom_task = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(getDOM()));
    LQIO::DOM::Activity * dstDOM = new LQIO::DOM::Activity( *(dynamic_cast<const LQIO::DOM::Activity *>(srcActivity.getDOM())) );
    dstDOM->setName( srcName.str() );
    dom_task->addActivity( dstDOM );
    dstActivity = new Activity( this, dstDOM );
    _activities.push_back( dstActivity );

    /* Clone instance variables */
    
    dstActivity->isSpecified( srcActivity.isSpecified() );
    if ( srcActivity.reachable() ) {
	std::ostringstream aName;
	aName << srcActivity.reachedFrom()->name() << "_" << replica;
	Activity * anActivity = findActivity( aName.str() );
	dstActivity->setReachedFrom( anActivity );
    }

    if ( srcActivity.isStartActivity() ) {
	Entry *dstEntry = Entry::find_replica( srcActivity.rootEntry()->name(), replica );
	dstActivity->rootEntry( dstEntry, nullptr );
	const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(dstEntry->getDOM()))->setStartActivity( dstDOM );
	if (dstEntry->entryTypeOk(LQIO::DOM::Entry::Type::ACTIVITY)) {
	    dstEntry->setStartActivity(dstActivity);
	}
    }

    /* Clone reply list */

    std::vector<Entry *>& dstReplyList = const_cast<std::vector<Entry *>&>(dstActivity->replies());
    for ( std::vector<Entry *>::const_iterator entry = srcActivity.replies().begin(); entry != srcActivity.replies().end(); ++entry ) {
	Entry * dstEntry = Entry::find_replica( (*entry)->name(), replica );
	dstReplyList.push_back( dstEntry );
	dstDOM->getReplyList().push_back(const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>(dstEntry->getDOM())));	/* Need dom for entry, the push that */
    }

    dstActivity->expandActivityCalls( srcActivity, replica );

    return dstActivity;
}
#endif

Task&
Task::removeActivity( Activity * anActivity )
{
    std::vector<Activity *>::iterator activity = find( _activities.begin(), _activities.end(), anActivity );
    if ( activity != _activities.end() ) {
	_activities.erase( activity );
    }
    return *this;
}


void
Task::addPrecedence( ActivityList * aPrecedence )
{
    _precedences.push_back( aPrecedence );
}


/*
 * Returns the initial depth (0 or 1) if this entity is a root of a
 * call graph.  Returns -1 otherwise.  Used by the topological sorter.
 */

Task::root_level_t
Task::rootLevel() const
{
    root_level_t level = root_level_t::IS_NON_REFERENCE;
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	const request_type callType = (*entry)->requestType();
	switch ( callType ) {

	case request_type::OPEN_ARRIVAL:	/* Root task, but at lower level */
	    level = root_level_t::HAS_OPEN_ARRIVALS;
	    break;

	case request_type::RENDEZVOUS:		/* Non-root task. 		*/
	case request_type::SEND_NO_REPLY:	/* Non-root task. 		*/
	    return root_level_t::IS_NON_REFERENCE;
	    break;
	    
	case request_type::NOT_CALLED:		/* No operation.		*/
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
    return std::any_of( entries().begin(), entries().end(), Predicate1<Entry,const Task *>( &Entry::forwardsTo, aTask ) );
}


/*
 * Return true if this task forwards to another task on this level.
 */

bool
Task::hasForwardingLevel() const
{
    return std::any_of( entries().begin(), entries().end(), ::Predicate<Entry>( &Entry::hasForwardingLevel ) );
}


/*
 * Return true if this task forwards to another task on this level.
 */

bool
Task::isForwardingTarget() const
{
    return std::any_of( entries().begin(), entries().end(), ::Predicate<Entry>( &Entry::isForwardingTarget ) );
}


/*
 * Return true if this task receives rendezvous requests.
 */

bool
Task::isCalled( const request_type callType ) const
{
    return std::any_of( entries().begin(), entries().end(), Predicate1<Entry,const request_type>( &Entry::isCalledBy, callType ) );
}


/*
 * Return the total open arrival rate to this server.
 */

double
Task::openArrivalRate() const
{
    double sum = 0.0;
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	if ( (*entry)->hasOpenArrivalRate() ) {
	    sum += LQIO::DOM::to_double( (*entry)->openArrivalRate() );
	}
    }

    return sum;
}



/*
 * Return true if this task makes any calls to lower level tasks.
 */

bool
Task::hasCalls( const callPredicate aFunc ) const
{
    return std::any_of( entries().begin(), entries().end(), Predicate1<Entry,const callPredicate>( &Entry::hasCalls, aFunc ) )
	|| std::any_of( activities().begin(), activities().end(), Predicate1<Activity,const callPredicate>( &Activity::hasCalls, aFunc ) );
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
    return std::any_of( entries().begin(), entries().end(), ::Predicate<Entry>( &Entry::hasQueueingTime ) )
	|| std::any_of( activities().begin(), activities().end(), ::Predicate<Activity>( &Activity::hasQueueingTime ) );
    return false;
}


/*
 * Return true if this entity is selected.
 * See subclasses for further tests.
 */

bool
Task::isSelectedIndirectly() const
{
    return Entity::isSelectedIndirectly() || std::any_of( processors().begin(), processors().end(), Predicate<Processor>( &Processor::isSelected ) )
	|| std::any_of( entries().begin(), entries().end(), ::Predicate<Entry>( &Entry::isSelectedIndirectly ) )
	|| std::any_of( activities().begin(), activities().end(), ::Predicate<Activity>( &Activity::isSelectedIndirectly ) );
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
    const Processor * processor = this->processor();

    std::vector<Entity *> servers;
    this->servers( servers );

    return servers.size() == 0 && processor != nullptr && processor->nClients() == 1;
}



bool
Task::check() const
{
    bool rc = true;

    /* Check prio/scheduling. */

    const Processor * processor = this->processor();

    if ( processor != nullptr && hasPriority() && !processor->hasPriorities() ) {
	getDOM()->runtime_error( LQIO::WRN_PRIO_TASK_ON_FIFO_PROC, processor->name().c_str() );
    }

    /* Check replication */

    if ( Flags::output_format() == File_Format::PARSEABLE ) {
	LQIO::error_messages.at(ERR_REPLICATION).severity = LQIO::error_severity::WARNING;
	LQIO::error_messages.at(ERR_REPLICATION_PROCESSOR).severity = LQIO::error_severity::WARNING;
    }

    if ( processor != nullptr && processor->isReplicated() ) {
	bool ok = true;
	double srcReplicasValue = 1.0;
	double dstReplicasValue = 1.0;
	const LQIO::DOM::ExternalVariable& dstReplicas = processor->replicas();
	if ( dstReplicas.wasSet() ) {
	    if ( dstReplicas.getValue( dstReplicasValue ) && isReplicated() ) {
		const LQIO::DOM::ExternalVariable& srcReplicas = replicas();
		if ( srcReplicas.wasSet() && srcReplicas.getValue( srcReplicasValue ) ) {
		    const double temp = static_cast<double>(srcReplicasValue) / static_cast<double>(dstReplicasValue);
		    ok = trunc( temp ) == temp;		/* Integer multiple */
		}
	    } else {
		ok = false;
	    }
	}
	if ( !ok ) {
	    LQIO::solution_error( ERR_REPLICATION_PROCESSOR,
				  static_cast<int>(srcReplicasValue), name().c_str(),
				  static_cast<int>(dstReplicasValue), processor->name().c_str() );
	    if ( Flags::output_format() != File_Format::PARSEABLE ) {
		rc = false;
	    }
	}
    }

    /* Check entries */

    const_cast<LQIO::DOM::DocumentObject *>(getDOM())->setSeverity( LQIO::WRN_ENTRY_HAS_NO_REQUESTS, LQIO::error_severity::ERROR );
    if ( scheduling() == SCHEDULE_SEMAPHORE ) {
	if ( nEntries() != 2 ) {
	    getDOM()->runtime_error( LQIO::ERR_TASK_ENTRY_COUNT, nEntries(), N_SEMAPHORE_ENTRIES );
	    rc = false;
	}
	Entry& e1 = *entries().front();
	Entry& e2 = *entries().back();
	if ( !((e1.isSignalEntry() && e2.entrySemaphoreTypeOk(LQIO::DOM::Entry::Semaphore::WAIT))
	       || (e1.isWaitEntry() && e2.entrySemaphoreTypeOk(LQIO::DOM::Entry::Semaphore::SIGNAL))
	       || (e2.isSignalEntry() && e1.entrySemaphoreTypeOk(LQIO::DOM::Entry::Semaphore::WAIT))
	       || (e2.isWaitEntry() && e1.entrySemaphoreTypeOk(LQIO::DOM::Entry::Semaphore::SIGNAL))) ) {
	    getDOM()->solution_error( LQIO::ERR_NO_SEMAPHORE );
	    rc = false;
	} else if ( e1.isCalled() && !e2.isCalled() ) {
	    e2.getDOM()->runtime_error( LQIO::WRN_ENTRY_HAS_NO_REQUESTS );
	    rc = false;
	} else if ( !e1.isCalled() && e2.isCalled() ) {
	    e1.getDOM()->runtime_error( LQIO::WRN_ENTRY_HAS_NO_REQUESTS );
	    rc = false;
	}

    } else if ( scheduling() == SCHEDULE_RWLOCK ) {
	if ( nEntries() != N_RWLOCK_ENTRIES ) {
	    getDOM()->runtime_error( LQIO::ERR_TASK_ENTRY_COUNT, nEntries(), N_RWLOCK_ENTRIES );
	    rc = false;
	}

	std::map<LQIO::DOM::Entry::RWLock,const Entry *> entry;

	for ( std::vector<Entry *>::const_iterator e = entries().begin(); e != entries().end(); ++e ) {
	    const LQIO::DOM::Entry * dom = dynamic_cast<const LQIO::DOM::Entry *>((*e)->getDOM());
	    if ( !entry.insert(std::pair<LQIO::DOM::Entry::RWLock,const Entry *>(dom->getRWLockFlag(),*e)).second ) {
		dom->runtime_error( LQIO::ERR_DUPLICATE_SYMBOL );
	    }
	}

	/* Make sure both or neither entry is called */

	const_cast<LQIO::DOM::DocumentObject *>(getDOM())->setSeverity( LQIO::WRN_ENTRY_HAS_NO_REQUESTS, LQIO::error_severity::ERROR );
	if ( entry.at(LQIO::DOM::Entry::RWLock::READ_UNLOCK)->isCalled() && !entry.at(LQIO::DOM::Entry::RWLock::READ_LOCK)->isCalled() ) {
	    entry.at(LQIO::DOM::Entry::RWLock::READ_LOCK)->getDOM()->runtime_error( LQIO::WRN_ENTRY_HAS_NO_REQUESTS );
	} else if ( !entry.at(LQIO::DOM::Entry::RWLock::READ_UNLOCK)->isCalled() && entry.at(LQIO::DOM::Entry::RWLock::READ_LOCK)->isCalled() ) {
	    entry.at(LQIO::DOM::Entry::RWLock::READ_UNLOCK)->getDOM()->runtime_error( LQIO::WRN_ENTRY_HAS_NO_REQUESTS );
	}
	if ( entry.at(LQIO::DOM::Entry::RWLock::WRITE_UNLOCK)->isCalled() && !entry.at(LQIO::DOM::Entry::RWLock::WRITE_LOCK)->isCalled() ) {
	    entry.at(LQIO::DOM::Entry::RWLock::WRITE_LOCK)->getDOM()->runtime_error( LQIO::WRN_ENTRY_HAS_NO_REQUESTS );
	} else if ( !entry.at(LQIO::DOM::Entry::RWLock::WRITE_UNLOCK)->isCalled() && entry.at(LQIO::DOM::Entry::RWLock::WRITE_LOCK)->isCalled() ) {
	    entry.at(LQIO::DOM::Entry::RWLock::WRITE_UNLOCK)->getDOM()->runtime_error( LQIO::WRN_ENTRY_HAS_NO_REQUESTS );
	}
    }
    const_cast<LQIO::DOM::DocumentObject *>(getDOM())->setSeverity( LQIO::WRN_ENTRY_HAS_NO_REQUESTS, LQIO::error_severity::WARNING );

    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	rc = (*entry)->check() && rc;
	_maxPhase = std::max( _maxPhase, (*entry)->maxPhase() );
    }

    rc = for_each( activities().begin(), activities().end(), AndPredicate<Activity>( &Activity::check ) ).result() && rc;
    return rc;
}




/*
 * Return all clients to this task.
 */

unsigned
Task::referenceTasks( std::vector<Entity *>& clients, Element * dst ) const
{
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	(*entry)->referenceTasks( clients, (dst == this) ? (*entry) : dst );	/* Map task to entry if this is the dst */
    }
    return clients.size();
}



/*
 * Return all clients to this task.
 */

unsigned
Task::clients( std::vector<Task *>& clients, const callPredicate aFunc ) const
{
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	(*entry)->clients( clients, aFunc );
    }
    return clients.size();
}



/*
 * Locate the destination task in the list of destinations.  Processor
 * calls are lumped into the task call list.  This makes life simpler
 * when we are drawing tasks only.
 */

EntityCall *
Task::findCall( const Entity * anEntity, const callPredicate aFunc ) const
{
    for ( std::vector<EntityCall *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( 	(!dynamic_cast<TaskCall *>(*call) || (*call)->dstEntity() == anEntity)
	     && (!(*call)->isProcessorCall() || (*call)->dstEntity() == anEntity)
	     && (!aFunc || ((*call)->*aFunc)() ) ) return *call;
    }

    return NULL;
}



EntityCall *
Task::findOrAddCall( Task * toTask, const callPredicate aFunc )
{
    EntityCall * aCall = findCall( toTask, aFunc );

    if ( !aCall ) {
	aCall = new TaskCall( this, toTask );
	_calls.push_back( aCall );
	toTask->addDstCall( aCall );
    }

    return aCall;
}



EntityCall *
Task::findOrAddPseudoCall( Entity * toEntity )
{
    EntityCall * aCall = findCall( toEntity );

    if ( !aCall ) {
	if ( dynamic_cast<Processor *>(toEntity) ) {
	    Processor * toProcessor = dynamic_cast<Processor *>(toEntity);
	    aCall = new PseudoProcessorCall( this, toProcessor );
	    toProcessor->addDstCall( aCall );
	    toProcessor->addTask( this );
	} else {
	    Task * toTask = dynamic_cast<Task *>(toEntity);
	    aCall = new PseudoTaskCall( this, toTask );
	    toTask->addDstCall( aCall );
	}
	aCall->linestyle( Graphic::LineStyle::DASHED_DOTTED );
	_calls.push_back( aCall );
    }

    return aCall;
}


#if BUG_270
void
Task::addSrcCall( EntityCall * call )
{
    _calls.push_back( call );
}
#endif


/*
 * Return all servers to this task.
 */

unsigned
Task::servers( std::vector<Entity *> &servers ) const
{
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	for ( std::vector<Call *>::const_iterator call = (*entry)->calls().begin(); call != (*entry)->calls().end(); ++call ) {
	    if ( !(*call)->hasForwardingLevel() && (*call)->isSelected() ) {
		const Task * task = (*call)->dstTask();;
		if ( std::none_of( servers.begin(), servers.end(), EQ<Element>( task ) ) ) {
		    servers.push_back( const_cast<Task *>(task) );
		}
	    }
	}
    }

    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	for ( std::vector<Call *>::const_iterator call = (*activity)->calls().begin(); call != (*activity)->calls().end(); ++call ) {
	    if ( !(*call)->hasForwarding() && (*call)->isSelected() ) {
		const Task * task = (*call)->dstTask();;
		if ( std::none_of( servers.begin(), servers.end(), EQ<Element>( task ) ) ) {
		    servers.push_back( const_cast<Task *>(task) );
		}
	    }
	}
    }

    for ( std::vector<EntityCall *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	const Processor * processor = dynamic_cast<const Processor *>((*call)->dstEntity());
	if ( processor && processor->isSelected() && processor->isInteresting() && std::none_of( servers.begin(), servers.end(), EQ<Element>(processor)) ) {
	    servers.push_back( const_cast<Processor *>(processor) );
	}
    }

    return servers.size();
}



bool
Task::isInOpenModel( const std::vector<Entity *>& servers ) const
{
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	for ( std::vector<Call *>::const_iterator call = (*entry)->calls().begin(); call != (*entry)->calls().end(); ++call ) {
	    if ( (*call)->hasSendNoReply() && std::any_of( servers.begin(), servers.end(), EQ<Element>((*call)->dstTask()) ) ) return true;
	}
    }
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	for ( std::vector<Call *>::const_iterator call = (*activity)->calls().begin(); call != (*activity)->calls().end(); ++call ) {
	    if ( (*call)->hasSendNoReply() && std::any_of( servers.begin(), servers.end(), EQ<Element>((*call)->dstTask()) ) ) return true;
	}
    }
    return false;
}


bool
Task::isInClosedModel( const std::vector<Entity *>& servers ) const
{
    for ( std::vector<EntityCall *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	const Processor * aProcessor = dynamic_cast<const Processor *>((*call)->dstEntity());
	if ( aProcessor ) {
	    if ( aProcessor->isInteresting() && std::any_of( servers.begin(), servers.end(), EQ<Element>(aProcessor) ) ) return true;
	    continue;
	}
	const Task * task = dynamic_cast<const Task *>((*call)->dstEntity());
	if ( task ) {
	    if ( std::any_of( servers.begin(), servers.end(), EQ<Element>((*call)->dstEntity()) ) ) return true;
	}
    }

    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	for ( std::vector<Call *>::const_iterator call = (*entry)->calls().begin(); call != (*entry)->calls().end(); ++call ) {
	    if ( (*call)->hasRendezvous() && std::any_of( servers.begin(), servers.end(), EQ<Element>((*call)->dstTask()) ) ) return true;
	}
    }
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	for ( std::vector<Call *>::const_iterator call = (*activity)->calls().begin(); call != (*activity)->calls().end(); ++call ) {
	    if ( (*call)->hasRendezvous() && std::any_of( servers.begin(), servers.end(), EQ<Element>((*call)->dstTask()) ) ) return true;
	}
    }
    return false;
}


/*
 * Recursively find all children and grand children from `father'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

size_t
Task::findChildren( CallStack& callStack, const unsigned directPath )
{
    const size_t depth = std::max( callStack.size(), level() );
    size_t max_depth = depth;

    setLevel( depth ).addPath( directPath );

    for ( std::set<const Processor *>::const_iterator processor = processors().begin(); processor != processors().end(); ++processor ) {
	const_cast<Processor *>(*processor)->setLevel( std::max( (*processor)->level(), depth + 1 ) ).addPath( directPath );
	max_depth = std::max( max_depth, (*processor)->level() );	
    }

    const Entry * dstEntry = callStack.back() ? callStack.back()->dstEntry() : nullptr;
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	max_depth = std::max( max_depth,
			      (*entry)->findChildren( callStack,
						      (((*entry) == dstEntry) || (*entry)->hasOpenArrivalRate())
						      ? directPath : 0  ) );
    }

    return max_depth;
}



/*
 * Count the number of calls that match the criteria passed
 */

unsigned
Task::countArcs( const callPredicate aFunc ) const
{
    return count_if( calls().begin(), calls().end(), GenericCall::PredicateAndSelected( aFunc ) );
}



/*
 * Count the number of calls that match the criteria passed
 */

double
Task::countCalls( const callPredicate2 aFunc ) const
{
    double count = 0.0;

    for ( std::vector<EntityCall *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	if ( (*call)->isSelected() ) {
	    count += ((*call)->*aFunc)() * (*call)->fanOut();
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
    unsigned count = 1;
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	AndForkActivityList * aList = dynamic_cast<AndForkActivityList *>((*activity)->outputTo());
	if ( aList && aList->size() > 0 ) {
	    count += aList->size();
	}
    }
    return count;
}


std::vector<Activity *>
Task::repliesTo( Entry * anEntry ) const
{
    std::vector<Activity *> aCltn;
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	if ( (*activity)->repliesTo( anEntry ) ) {
	    aCltn.push_back( (*activity));
	}
    }
    return aCltn;
}


bool
Task::canConvertToReferenceTask() const
{
    return Flags::convert_to_reference_task
	&& (submodel_output() || Flags::include_only() != nullptr )
	&& !isSelected()
	&& !hasOpenArrivals()
	&& !isInfinite()
	&& nEntries() == 1
	&& !_processors.empty();
}



#if BUG_270
bool
Task::canPrune() const
{
    if ( maxPhase() > 1 ) return false;	    	/* Just give up... */
    if ( isInfinite() ) return true;		/* No way to queue at me */

    /* 
     * The harder path.  I want to make sure that I can't queue below
     * me.  Find all callers to my servers... if it's just me, then we
     * are good to go.  Note that both the task and it's processor can
     * be multiservers, but the both must have equivalent copies.
     * Processor::nClients() returns the sum of the copies of all the
     * tasks that call it.
     */

    if ( processor()->nClients() != processor()->copiesValue() ) return false;

    /* 
     * This part is trickier, and I may just punt.  Locate everyone
     * who I call, and if they can only be called by me, then we are
     * golden.  I should have aggregated the entries to the task at
     * this point.  canPrune could be recursive?
     */

//    assert( Flags::aggregation() == Aggregate::ENTRIES );
//    std::set<const Task *> callers = std::accumulate( entries().begin(), entries().end(), std::set<const Task *>(), &Entry::collect_callers );
    return calls().size() == 1;
}
#endif



/*
 * This has to be done by class.
 */

void
Task::accumulateDemand( BCMP::Model::Station& station ) const
{
    typedef std::pair<const std::string,BCMP::Model::Station::Class> demand_item;
    typedef std::map<const std::string,BCMP::Model::Station::Class> demand_map;

    demand_map& demands = station.classes();
    const std::pair<demand_map::iterator,bool> result = demands.insert( demand_item( name(), BCMP::Model::Station::Class() ) );	/* null entry */
    result.first->second.accumulate( Task::accumulate_demand( BCMP::Model::Station::Class(), this ) );
}


/* static */ BCMP::Model::Station::Class
Task::accumulate_demand( const BCMP::Model::Station::Class& augend, const Task * task )
{
    return std::accumulate( task->entries().begin(), task->entries().end(), augend, &Entry::accumulate_demand ); 
}


/*
 * Normally, chains have think times.  JMVA represents this as the service time at the reference station
 */

void
Task::create_chain::operator()( const Task * task ) const
{
    if ( task->isInClosedModel(_servers) ) {
	/* Think time for a task is the class think time. */
	const LQIO::DOM::ExternalVariable * copies = task->isMultiServer() ? &task->copies() : &Element::ONE;
	_model.insertClosedChain( task->name(), copies, &Element::ZERO );
    }
    if ( task->isInOpenModel(_servers) ) {
	_model.insertOpenChain( task->name(), &Element::ZERO );
    }
}


/*
 * Create a terminal station.  Insert total visits into clients and
 * set service time for class if the processor has been removed.
 * Include task think time.
 */

void
Task::create_customers::operator()( const Task * task )
{
    /* If processor is missing, use service time here.  "class" may have to generalize to entry */

    const ReferenceTask * client = dynamic_cast<const ReferenceTask *>(task);
    const LQIO::DOM::ExternalVariable * service_time = nullptr;
    if ( client != nullptr ) {
	service_time = addExternalVariables( client->entries().front()->thinkTime(), &client->thinkTime() );
    }
    if ( task->processor() == nullptr ) {
	// for all entries s += prVisit(e) * e->serviceTime ??
	service_time = addExternalVariables( service_time, task->entries().at(0)->serviceTime() );
    }
    BCMP::Model::Station::Class demand( &Element::ONE, service_time );	/* One visit */

    const std::string name = task->name();
    _terminals.insertClass( name, demand );

    /* Find all observations for this chain - it will become a result_var for the termianls station */

    const LQIO::Spex::obs_var_tab_t& observations = LQIO::Spex::observations();
    for ( LQIO::Spex::obs_var_tab_t::const_iterator obs = observations.begin(); obs != observations.end(); ++obs ) {
	if ( obs->first != task->getDOM() ) continue;
	switch ( obs->second.getKey() ) {
	case KEY_THROUGHPUT:	_terminals.classAt(name).insertResultVariable( BCMP::Model::Result::Type::THROUGHPUT, obs->second.getVariableName() ); break;
	case KEY_UTILIZATION:	_terminals.classAt(name).insertResultVariable( BCMP::Model::Result::Type::UTILIZATION, obs->second.getVariableName() ); break;
	}
    }
}

/*
 * Sort activities (if any).
 */

unsigned
Task::generate()
{
    unsigned int maxLevel = topologicalSort();

    _layers.resize( maxLevel + 1 );

    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	if ( (*activity)->reachable() ) {
	    _layers.at((*activity)->level()) += (*activity);
	}
    }
    return maxLevel;
}



/*
 * Toplogical sort for activities.  It also aggregates activities.
 */

size_t
Task::topologicalSort()
{
    size_t maxLevel = 0;
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	Activity * anActivity = (*entry)->startActivity();
	if ( !anActivity ) continue;

	std::deque<const Activity *> activityStack;		// For checking for cycles.
	std::deque<const AndForkActivityList *> forkStack;	// For matching forks and joins.
	try {
	    maxLevel = std::max( maxLevel, anActivity->findActivityChildren( activityStack, forkStack, (*entry), 0, 1, 1.0  ) );
	}
	catch ( const Activity::cycle_error& error ) {
	    maxLevel = std::max( maxLevel, error.depth()+1 );
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

    std::sort( _entries.begin(), _entries.end(), Entry::compare );

    /* Compute width of task.  Move entries */

    const double ty = _node->height() + height();
    _entryWidthInPts = 0;
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	(*entry)->moveTo( _entryWidthInPts + _node->left(), ty - (*entry)->height() );
	_entryWidthInPts += (*entry)->width() - adjustForSlope( fabs( (*entry)->height() ) );
    }
    if ( Flags::graphical_output_style == Output_Style::JLQNDEF ) {
	_entryWidthInPts += Flags::entry_width * JLQNDEF_TASK_BOX_SCALING;
    }


    if ( _layers.size() ) {
	for ( std::vector<ActivityLayer>::iterator layer = _layers.begin(); layer != _layers.end(); ++layer ) {
	    layer->sort( Activity::compare ).format( 0 );
	}

	const double x = _node->left() + adjustForSlope( fabs( height() ) ) + Flags::act_x_spacing / 4;

	/* Start from bottom and work up.  Reformat will realign the activities */

	double y = _node->bottom();
	for ( std::vector<ActivityLayer>::reverse_iterator layer = _layers.rbegin(); layer != _layers.rend(); ++layer ) {
	    layer->moveTo( x, y );
	    y += layer->height();  /* grow down */

 	    /* Shift up layer IFF we have to draw stuff below activities */
 	    if ( layer->height() > Flags::entry_height ) {
 		layer->moveBy( 0.0, layer->height() - Flags::entry_height );
 	    }
	}

	y += Flags::icon_height;
	if ( !(queueing_output() && Flags::flatten_submodel) ) {
	    _node->setHeight( y );
	}

	/* Calculate the space needed for the activities */

	switch( Flags::activity_justification ) {
	case Justification::ALIGN:
	case Justification::DEFAULT:
	    aWidth = justifyByEntry();
	    break;
	default:
	    aWidth = justify();
	    break;
	}
	aWidth += adjustForSlope( 2.0 * y );	/* Center icons inside task. */
    }

    /* Modify extent  */

    _node->setWidth( std::max( std::max( aWidth, _entryWidthInPts + adjustForSlope( height() ) ), width() ) );

    return *this;
}


/*
 * Layout the entries and activities (if we have any).
 */

Task&
Task::reformat()
{
    std::sort( _entries.begin(), _entries.end(), Entry::compare );

    const double x = _node->left() + adjustForSlope( height() );
    double y = _node->bottom();
    const double offset = adjustForSlope( (height() - fabs(entries().front()->height())));
    const double fill = std::max( ((width() - adjustForSlope( height() )) - _entryWidthInPts) / (nEntries() + 1.0), 0.0 );

    /* Figure out which sides of the entries to draw */

    if ( fill == 0.0 ) {
	for ( std::vector<Entry *>::const_reverse_iterator entry = entries().rbegin(); entry != entries().rend(); ++entry ) {
	    (*entry)->drawLeft  = false;
	    (*entry)->drawRight = ( entry != entries().rbegin() || Flags::graphical_output_style == Output_Style::JLQNDEF );
	}
    }

    /* Move entries */

    const double ty = y + height();
    double tx = _node->left() + offset + fill;
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	(*entry)->moveTo( tx, ty - (*entry)->height() );
	tx += (*entry)->width() - adjustForSlope( fabs( (*entry)->height() ) ) + fill;
    }

    /* Move activities */

    if ( _layers.size() ) {

	for ( std::vector<ActivityLayer>::reverse_iterator layer = _layers.rbegin(); layer != _layers.rend(); ++layer ) {
	    layer->moveTo( x, y );
	    y += layer->height();  /* grow down */

 	    if ( layer->height() > Flags::entry_height ) {
 		layer->moveBy( 0.0, layer->height() - Flags::entry_height );
 	    }
	}

	for ( std::vector<ActivityLayer>::iterator layer = _layers.begin(); layer != _layers.end(); ++layer ) {
	    layer->sort( Activity::compare ).reformat( 0 );
	}

	switch( Flags::activity_justification ) {
	case Justification::ALIGN:
	case Justification::DEFAULT:
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
    double left  = std::numeric_limits<double>::max();
    double right = 0.0;

    for ( std::vector<ActivityLayer>::iterator layer = _layers.begin(); layer != _layers.end(); ++layer ) {
	left  = std::min( left,  layer->x() );
	right = std::max( right, layer->x() + layer->width() );
    }

    for ( std::vector<ActivityLayer>::iterator layer = _layers.begin(); layer != _layers.end(); ++layer ) {
	layer->justify( right - left, Flags::activity_justification );
    }

    return right - left;
}




/*
 * Move activities so they line up with the entries.
 */

double
Task::justifyByEntry()
{
    const unsigned MAX_LEVEL = _layers.size();
    double width = 0;

    /*
     * Find all activities reachable from this entry...
     * and stick them into the sublayer
     */

    double x = left() + adjustForSlope( fabs( height() ) ) + Flags::act_x_spacing / 4.;
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	double right = 0.0;
	double left  = std::numeric_limits<double>::max();
	Activity * startActivity = (*entry)->startActivity();
	if ( !startActivity ) continue;

	/* Sublayer will only contain activities reachable from current entry */

	std::vector<ActivityLayer> sublayer(MAX_LEVEL);

	int i = 0;
	for ( std::vector<ActivityLayer>::iterator layer = _layers.begin(); layer != _layers.end(); ++layer, ++i ) {
	    if ( !*layer ) continue;
	    for ( std::vector<Activity *>::const_iterator activity = layer->activities().begin(); activity != layer->activities().end(); ++activity ) {
		if ( (*activity)->reachedFrom() == startActivity ) {
		    sublayer.at(i) += (*activity);
		}
	    }
	}

	/* Now, move all activities for this entry together */

	i = _layers.size();
	for ( std::vector<ActivityLayer>::reverse_iterator layer = _layers.rbegin(); layer != _layers.rend(); ++layer ) {
	    i -= 1;
	    if ( !*layer ) continue;
	    sublayer.at(i).reformat( 0 );
	    right = std::max( right, sublayer[i].x() + sublayer[i].width() );
	    left  = std::min( left,  sublayer[i].x() );
	}

	/* Justify the current "slice", then move it to its column */

	for ( unsigned i = 0; i < MAX_LEVEL; ++i ) {
	    sublayer[i].justify( right - left, Flags::activity_justification ).moveBy( x, 0 );
	}

	if ( Flags::activity_justification == Justification::ALIGN ) {
	    double shift = 0;
	    for ( unsigned i = 0; i < MAX_LEVEL; ++i ) {
		sublayer[i].alignActivities();
		shift = std::max( x - sublayer[i].x(), shift );	// If we've moved left too far, we'll have to shift everything.
	    }
	    for ( unsigned i = 0; i < MAX_LEVEL; ++i ) {
		if ( shift > 0 ) {
		    sublayer[i].moveBy( shift, 0 );
		}
		right = std::max( right, sublayer[i].x() + sublayer[i].width() - x );	// Don't forget to subtract x!
	    }
	}

	/* The next column starts here. */

	x += right;
	width = x;
	x += Flags::x_spacing();
    }

    return width - (left() + adjustForSlope( fabs( height() ) ) );
//    return width - left();
}



double
Task::alignActivities()
{
    double minLeft  = std::numeric_limits<double>::max();
    double maxRight = 0;

    for ( std::vector<ActivityLayer>::iterator layer = _layers.begin(); layer != _layers.end(); ++layer ) {
	if ( !*layer ) continue;
	layer->alignActivities();
	minLeft  = std::min( minLeft, layer->x() );
	maxRight = std::max( maxRight, layer->x() + layer->width() );

    }
    return maxRight;
}


/*
 * Move the entity, it's entries and activities.  Don't recompute everything.
 */

Task&
Task::moveBy( const double dx, const double dy )
{
    _node->moveBy( dx, dy );
    _label->moveBy( dx, dy );
    for_each( entries().begin(), entries().end(), ExecXY<Element>( &Entry::moveBy, dx, dy ) );    		/* Move entries */
    for_each( _layers.begin(), _layers.end(), ExecXY<ActivityLayer>( &ActivityLayer::moveBy, dx, dy ) );	/* Move activities */
    for_each( _layers.rbegin(), _layers.rend(), ExecXY<ActivityLayer>( &ActivityLayer::moveBy, 0, 0 ) );	/* clean up.*/

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
    _node->moveTo( x, y );

    reformat();

    if ( Flags::aggregation() == Aggregate::ENTRIES ) {
	_label->moveTo( bottomCenter() ).moveBy( 0, height() / 2 );
    } else if ( !queueing_output() ) {

	/* Move Label -- do after X extent recalculated */

	if ( Flags::graphical_output_style == Output_Style::JLQNDEF ) {
	    _label->moveTo( topRight() ).moveBy( -(Flags::entry_width * JLQNDEF_TASK_BOX_SCALING * 0.5), -entries().front()->height()/2 );
	} else if ( _layers.size() ) {
	    _label->moveTo( topCenter() ).moveBy( 0, -entries().front()->height() - 10 );
	} else {
	    _label->moveTo( bottomCenter() ).moveBy( 0, height() / 5 );
	}
    } else {
	_label->moveTo( bottomCenter() ).moveBy( 0, height() / 5 );
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
    if ( Flags::graphical_output_style == Output_Style::JLQNDEF ) {
	Point aPoint = bottomRight();
	aPoint.moveBy( -(Flags::entry_width * JLQNDEF_TASK_BOX_SCALING)/2.0, 0 );
	calls().front()->moveSrc( aPoint );
    } else if ( calls().size() == 1 && dynamic_cast<ProcessorCall *>(calls().front()) ) {
	Point aPoint = bottomCenter();
	const Processor * processor = this->processor();
	double diff = aPoint.x() - processor->center().x();
	if ( diff > 0 && fabs( diff ) > width() / 4 ) {
	    aPoint.moveBy( -width() / 4, 0 );
	} else if ( diff < 0 && fabs( diff ) > width() / 4 ) {
	    aPoint.moveBy( width() / 4, 0 );
	}
	calls().front()->moveSrc( aPoint );
    } else {
	const int nFwd = countArcs( &GenericCall::hasForwardingLevel );
	Point aPoint = bottomLeft();
	const double delta = width() / static_cast<double>(countArcs() + 1 - nFwd);

	for ( std::vector<EntityCall *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	    if ( (*call)->isSelected()  && !(*call)->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		(*call)->moveSrc( aPoint );
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
    Point aPoint = _node->topLeft();

    if ( Flags::print_forwarding_by_depth ) {
	const double delta = width() / static_cast<double>(countCallers() + 1);

	/* Draw other incomming arcs. */

	for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	    if ( (*call)->isSelected() ) {
		aPoint.moveBy( delta, 0 );
		(*call)->moveDst( aPoint );
	    }
	}
    } else {
	/*
	 * We add the outgoing forwarding arcs to the incomming side of the entry,
	 * so adjust the counts as necessary.
	 */

	const int nFwd = countArcs( &GenericCall::hasForwardingLevel );
	const double delta = width() / static_cast<double>(countCallers() + 1 + nFwd );
	const double fy = Flags::y_spacing() / 2.0 + top();

	/* Draw incomming forwarding arcs first. */

	Point fwdPoint( left(), fy );
	for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	    if ( (*call)->isSelected() && (*call)->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		(*call)->moveDst( aPoint ).movePenultimate( fwdPoint );
		fwdPoint.moveBy( delta, 0 );
	    }
	}

	/* Draw other incomming arcs.  Note that secondPoint() and penultimatePoint() don't work here because   */
	/* Arc should only have two points (so the source/destination may get clobbered)			*/

	for ( std::vector<GenericCall *>::const_iterator call = callers().begin(); call != callers().end(); ++call ) {
	    if ( (*call)->isSelected() && !(*call)->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		if ( (*call)->hasForwarding() && (*call)->nPoints() == 4 ) {
		    (*call)->pointAt(1) = aPoint;
		    (*call)->pointAt(2) = aPoint;
		}
		(*call)->moveDst( aPoint );
	    }
	}

	/* Draw outgoing forwarding arcs */

	fwdPoint.moveTo( right(), fy );
	for ( std::vector<EntityCall *>::const_iterator call = calls().begin(); call != calls().end(); ++call ) {
	    if ( (*call)->isSelected() && (*call)->hasForwardingLevel() ) {
		aPoint.moveBy( delta, 0 );
		(*call)->moveSrc( aPoint ).moveSecond( fwdPoint );
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
    for_each( calls().begin(), calls().end(), ExecXY<GenericCall>( &GenericCall::moveSrcBy, dx, dy ) );
    return *this;
}


Graphic::Colour
Task::colour() const
{
    switch ( Flags::colouring() ) {
    case Colouring::DIFFERENCES:
	return colourForDifference( throughput() );

    default:
	return Entity::colour();
    }
}


/*
 * Label the node.
 */

Task&
Task::label()
{
    if ( queueing_output() ) {
	bool print_goop = false;
	if ( Flags::print_input_parameters() ) {
	    labelQueueingNetwork( &Entry::labelQueueingNetworkVisits );
	    print_goop = true;
	}
	if ( Flags::have_results && Flags::print[WAITING].opts.value.b ) {
	    labelQueueingNetwork( &Entry::labelQueueingNetworkWaiting );
	    print_goop = true;
	}
	if ( print_goop ) {
	    _label->newLine();
	}
    }
    Entity::label();
    if ( !queueing_output() ) {
	_label->justification( Flags::label_justification );
    }
    if ( Flags::print_input_parameters() ) {
	if ( queueing_output() ) {
	    if ( !isSelected() ) {
		const double Z = Flags::have_results ? (copiesValue() - utilization()) / throughput() : 0.0;
		if ( Z > 0.0 ) {
		    _label->newLine() << " Z = " << Z;
		}
	    }
	    labelQueueingNetwork( &Entry::labelQueueingNetworkService );
	} else {
	    if ( Flags::aggregation() == Aggregate::ENTRIES && Flags::print[PRINT_AGGREGATE].opts.value.b ) {
		_label->newLine() << " [" << service_time_of( *entries().front() ) << ']';
	    }
	    if ( hasThinkTime()  ) {
		*_label << " Z=" << dynamic_cast<ReferenceTask *>(this)->thinkTime();
	    }
	}

    }
    if ( Flags::have_results ) {
 	bool print_goop = false;
	if ( Flags::print[TASK_THROUGHPUT].opts.value.b ) {
	    _label->newLine();
	    if ( throughput() == 0.0 && Flags::colouring() != Colouring::NONE ) {
		_label->colour( Graphic::Colour::RED );
	    }
	    *_label << begin_math( &Label::lambda ) << "=" << opt_pct(throughput());
	    print_goop = true;
	}
	if ( Flags::print[TASK_UTILIZATION].opts.value.b ) {
	    if ( print_goop ) {
		*_label << ',';
	    } else {
		_label->newLine() << begin_math();
		print_goop = true;
	    }
	    *_label << _rho() << "=" << opt_pct(utilization());
	    if ( hasBogusUtilization() && Flags::colouring() != Colouring::NONE ) {
		_label->colour(Graphic::Colour::RED);
	    }
	}
	if ( print_goop ) {
	    *_label << end_math();
	}
    }

    for_each( entries().begin(), entries().end(), Exec<Element>( &Element::label ) );       	/* Now label entries. */
    for_each( activities().begin(), activities().end(), Exec<Activity>( &Activity::label ) );	/* And activities... */
    for_each( calls().begin(), calls().end(), Exec<GenericCall>( &GenericCall::label ) );   	/* And the outgoing arcs, if any */
    for_each( precedences().begin(), precedences().end(), Exec<ActivityList>( &ActivityList::label ) );
    for_each( _layers.rbegin(), _layers.rend(), Exec<ActivityLayer>( &ActivityLayer::label ) );
    return *this;
}



/*
 * Clients in the LQN model represent a class, but clients in the BCMP model are represented by 
 * a single multi-class station (termainals). 
 */

Task&
Task::labelBCMPModel( const BCMP::Model::Station::Class::map_t& demand, const std::string& class_name )
{
    *_label << name();
    _label->newLine();
    if ( isMultiServer() ) {		/* copies() will be NULL if not */
	*_label << "[" << copies() << "]";
    } else {
	*_label << "[1]";
    }
    const BCMP::Model::Station::Class& reference = demand.at(class_name);
    _label->newLine();
    *_label << "(" << *reference.visits() << "," << *reference.service_time() << ")";
    return *this;
}



/*
 * Stick labels in for queueing network.
 */

Task&
Task::labelQueueingNetwork( entryLabelFunc aFunc )
{
    for_each( _entries.begin(), _entries.end(), Exec1<Entry,Label&>( aFunc, *_label ) );
    return *this;
}



/*
 * Move the entity and it's entries.
 */

Task&
Task::scaleBy( const double sx, const double sy )
{
    Entity::scaleBy( sx, sy );
    for_each( entries().begin(), entries().end(), ExecXY<Element>( &Element::scaleBy, sx, sy ) );
    for_each( activities().begin(), activities().end(), ExecXY<Element>( &Element::scaleBy, sx, sy ) );
    for_each( precedences().begin(), precedences().end(), ExecXY<ActivityList>( &ActivityList::scaleBy, sx, sy ) ) ;
    for_each( calls().begin(), calls().end(), ExecXY<GenericCall>( &GenericCall::scaleBy, sx, sy ) );
    return *this;
}



/*
 * Move the entity and it's entries.
 */

Task&
Task::depth( const unsigned depth )
{
    Entity::depth( depth );
    for_each( entries().begin(), entries().end(), Exec1<Element,unsigned int>( &Element::depth, depth ) );
    for_each( activities().begin(), activities().end(), Exec1<Element,unsigned int>( &Element::depth, depth ) );
    for_each( precedences().begin(), precedences().end(), Exec1<ActivityList,unsigned int>( &ActivityList::depth, depth ) );
    for_each( calls().begin(), calls().end(), Exec1<GenericCall,unsigned int>( &GenericCall::depth, depth ) );
    return *this;
}



/*
 * Move the entity and it's entries.
 */

Task&
Task::translateY( const double dy )
{
    Entity::translateY( dy );
    for_each( entries().begin(), entries().end(), Exec1<Element,double>( &Element::translateY, dy ) );
    for_each( activities().begin(), activities().end(), Exec1<Element,double>( &Element::translateY, dy ) );
    for_each( precedences().begin(), precedences().end(), Exec1<ActivityList,double>( &ActivityList::translateY, dy ) );
    for_each( calls().begin(), calls().end(), Exec1<GenericCall,double>( &GenericCall::translateY, dy ) );
    return *this;
}

/* -------------------------------------------------------------------- */
/* Converion to BCMP model.						*/
/* -------------------------------------------------------------------- */

#if BUG_270
Task&
Task::relink()
{
    if ( !canPrune() ) return *this;
    for_each( entries().begin(), entries().end(), Exec1<Entry,const std::vector<EntityCall *>&>( &Entry::linkToClients, calls() ) );
    for_each( entries().begin(), entries().end(), Exec<Entry>( &Entry::unlinkFromServers ) );
    unlinkFromProcessor();
    Model::__zombies.push_back( this );
    /* Observation variables ??? */
    return *this;
}



Task&
Task::unlinkFromProcessor()
{
    Processor * processor = const_cast<Processor *>(this->processor());
    processor->removeTask( this );
    for ( std::vector<EntityCall *>::iterator call = _calls.begin(); call != _calls.end(); ++call ) {
	if ( dynamic_cast<ProcessorCall *>(*call) && dynamic_cast<ProcessorCall *>(*call)->dstEntity() == processor ) {
	    processor->removeDstCall( *call );
	    break;
	}
    }
    _processors.erase( processor );
    Model::__zombies.push_back( processor );
    return *this;
}


/*
 * Find all calls from this task going to a common server, then merge
 */

Task&
Task::mergeCalls()
{
#if BUG_270_DEBUG
    std::cout << "Task::mergeCalls() for " << name() << std::endl;
#endif
    /* At this point, they are all processor calls */
    std::multimap<const Entity *,EntityCall *> merge;
    for ( std::vector<EntityCall *>::const_iterator call = _calls.begin(); call != _calls.end(); ++call ) {
	merge.insert( std::pair<const Entity *,EntityCall *>( (*call)->dstEntity(), *call ) );
    }

    std::vector<EntityCall *> new_calls;
    merge_iter upper;
    for ( merge_iter lower = merge.begin(); lower != merge.end(); lower = upper ) {
	const Entity * server = lower->first;
	upper = merge.upper_bound( server ); 
	const LQIO::DOM::ExternalVariable * visits       = std::accumulate( lower, upper, static_cast<const LQIO::DOM::ExternalVariable *>(nullptr), &Task::sum_rendezvous );
	/* Accumlating visits appears to be correct, but service time is NOT... I need demand, then normalize to visits */
	const LQIO::DOM::ExternalVariable * demand 	 = std::accumulate( lower, upper, static_cast<const LQIO::DOM::ExternalVariable *>(nullptr), &Task::sum_demand );
	const LQIO::DOM::ExternalVariable * service_time = Entity::divideExternalVariables( demand, visits );
#if BUG_270_DEBUG
	size_t count = merge.count( server );
	std::cout << "  To " << server->name() << ", count=" << count
		  << ", visits=";
	if ( visits != nullptr ) {
	    std::cout << *visits;
	}
	std::cout << ", service time=";
	if ( service_time != nullptr ) {
	    std::cout << *service_time << std::endl;
	} else {
	    std::cout << "0.000" << std::endl;		// I need to get rid of this.
	}
#endif
	if ( visits == 0 ) continue;
	/* create a new call with the number of vists and the service time / number of visits */
	if ( server->isProcessor() ) {
	    ProcessorCall * call;
	    call = new ProcessorCall( this, dynamic_cast<const Processor *>(server) );
	    call->rendezvous( visits );
//	    call->setServiceTime( Enttiy::divideExternalVariables( service_time, visits ) );
	    call->setServiceTime( service_time );
	    new_calls.push_back( call );
	} else {
	    abort();
	}
    }

#if 0
    _calls = new_calls;    /* copy over the new call list */
#endif
    return *this;
}


const LQIO::DOM::ExternalVariable *
Task::sum_rendezvous( const LQIO::DOM::ExternalVariable * augend, const merge_pair& addend ) 
{
    return Entity::addExternalVariables( augend, addend.second->sumOfRendezvous() );
}


const LQIO::DOM::ExternalVariable *
Task::sum_demand( const LQIO::DOM::ExternalVariable * augend, const merge_pair& addend )
{
    const ProcessorCall * call = dynamic_cast<const ProcessorCall *>(addend.second);
    if ( call == nullptr || call->srcEntry() == nullptr ) return augend;
    // !!! Fix this!!
    const LQIO::DOM::ExternalVariable * demand = Entity::multiplyExternalVariables( call->sumOfRendezvous(), call->srcEntry()->serviceTime() );
    return Entity::addExternalVariables( augend, demand );
}
#endif

/* -------------------------------------------------------------------- */
/* Replication								*/
/* -------------------------------------------------------------------- */

#if defined(REP2FLAT)
Task&
Task::removeReplication()
{
    Entity::removeReplication();

    LQIO::DOM::Task * dom = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(getDOM()));
    for ( std::map<const std::string, const LQIO::DOM::ExternalVariable *>::const_iterator fi = dom->getFanIns().begin(); fi != dom->getFanIns().end(); ++fi ) {
	dom->setFanIn( fi->first, &Element::ONE );
    }
    for ( std::map<const std::string, const LQIO::DOM::ExternalVariable *>::const_iterator fo = dom->getFanOuts().begin(); fo != dom->getFanOuts().end(); ++fo ) {
	dom->setFanOut( fo->first, &Element::ONE );
    }

    return *this;
}


Task&
Task::expand()
{
    const unsigned int replicas = replicasValue();
    const Processor * processor = this->processor();
    const unsigned int procFanOut = replicas / processor->replicasValue();
    
    for ( unsigned int replica = 1; replica <= replicas; ++replica ) {

	/* Get a pointer to the replicated processor */

	const unsigned int proc_replica = static_cast<unsigned int>(static_cast<double>(replica-1) / static_cast<double>(procFanOut)) + 1;
	const Processor *aProcessor = Processor::find_replica( processor->name(), proc_replica );

	std::ostringstream replica_name;
	replica_name << name() << "_" << replica;
	if ( find_if( __tasks.begin(), __tasks.end(), eqTaskStr( replica_name.str() ) )!= __tasks.end() ) {
	    std::string msg = "Task::expand(): cannot add symbol ";
	    msg += replica_name.str();
	    throw std::runtime_error( msg );
	}
	Task * new_task = clone( replica, replica_name.str(), aProcessor, share() );
	new_task->myPaths = myPaths;		// Bad hack?
	new_task->setLevel( level() );
	__tasks.insert( new_task );

	std::vector<LQIO::DOM::Entry *>& dom_entries = const_cast<std::vector<LQIO::DOM::Entry *>&>( dynamic_cast<const LQIO::DOM::Task *>(new_task->getDOM())->getEntryList() );
	dom_entries.clear();
	for ( std::vector<Entry *>::const_iterator entry = new_task->entries().begin(); entry != new_task->entries().end(); ++entry ) {
	    LQIO::DOM::Entry * dom_entry = const_cast<LQIO::DOM::Entry *>(dynamic_cast<const LQIO::DOM::Entry *>((*entry)->getDOM()));
	    dom_entries.push_back( dom_entry );        /* Add to task. */
	}

	/* Handle group if necessary */

	if ( hasActivities() ) {
	    new_task->expandActivities( *this, replica );
	}

	/* Patch up observations */

	if ( replica == 1 ) {
	    cloneObservations( getDOM(), new_task->getDOM() );
	}
    }
    return *this;
}


const std::vector<Entry *>&
Task::groupEntries( int replica, std::vector<Entry *>& newEntryList  ) const
{
    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	newEntryList.push_back( Entry::find_replica( (*entry)->name(), replica ) );
    }
    return newEntryList;
}



Task&
Task::expandActivities( const Task& src, int replica )
{
    /* clone the activities */
    LQIO::DOM::Task * task_dom = const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>(getDOM()));

    for ( std::vector<Activity *>::const_iterator activity = src.activities().begin(); activity != src.activities().end(); ++activity ) {
	addActivity(**activity, replica);
    }

    /* Now reconnect the precedences.  Do the join list from the task.  We have to connect the fork list. */

    for ( std::vector<ActivityList *>::const_iterator precedence = src.precedences().begin(); precedence != src.precedences().end(); ++precedence ) {
	if ( !dynamic_cast<const JoinActivityList *>((*precedence))  && !dynamic_cast<const AndOrJoinActivityList *>((*precedence)) ) continue;

	/* Ok we have the join list. */

	ActivityList * pre_list = (*precedence)->clone();
	pre_list->setOwner( this );
	addPrecedence( pre_list );
	LQIO::DOM::ActivityList * pre_list_dom = const_cast<LQIO::DOM::ActivityList *>(pre_list->getDOM());
	task_dom->addActivityList( pre_list_dom );
	pre_list_dom->setTask( task_dom );

	/* Now reconnect the activities. A little kludgey because we have a list in one case and not the other. */

	std::vector<Activity *> pre_act_list;
	if (dynamic_cast<const ForkJoinActivityList *>((*precedence))) {
	    pre_act_list = dynamic_cast<const ForkJoinActivityList *>((*precedence))->activityList();
	} else if (dynamic_cast<const SequentialActivityList *>((*precedence))) {
	    pre_act_list.push_back( dynamic_cast<const SequentialActivityList *>((*precedence))->getMyActivity());
	}

	for ( std::vector<Activity *>::const_iterator activity = pre_act_list.begin(); activity != pre_act_list.end(); ++activity ) {
	    Activity * pre_activity = findActivity(*(*activity), replica);
	    LQIO::DOM::Activity * pre_activity_dom = const_cast<LQIO::DOM::Activity *>(dynamic_cast<const LQIO::DOM::Activity *>(pre_activity->getDOM()));
	    pre_activity->outputTo( pre_list );
	    pre_activity_dom->outputTo( pre_list_dom );
	    pre_list->add( pre_activity );
	    if ( pre_list_dom != nullptr ) pre_list_dom->add( pre_activity_dom );
	}

	const ActivityList *dstPrecedence = (*precedence)->next();

	if ( dstPrecedence ) {

	    /* Here is the fork list */

	    ActivityList * post_list = dstPrecedence->clone();
	    post_list->setOwner( this );
	    addPrecedence( post_list );
	    LQIO::DOM::ActivityList * post_list_dom = const_cast<LQIO::DOM::ActivityList *>(post_list->getDOM());
	    post_list_dom->setTask( task_dom );
	    task_dom->addActivityList( post_list_dom );
	    ActivityList::act_connect( pre_list, post_list );
	    pre_list_dom->setNext( post_list_dom );
	    post_list_dom->setPrevious( pre_list_dom );

	    /* Now reconnect the activities. A little kludgey because we have a list in one case and not the other. */

	    std::vector<Activity *> post_act_list;
	    if (dynamic_cast<const ForkJoinActivityList *>(dstPrecedence)) {
		post_act_list = dynamic_cast<const ForkJoinActivityList *>(dstPrecedence)->activityList();
	    } else if (dynamic_cast<const RepeatActivityList *>(dstPrecedence)) {
		post_act_list = dynamic_cast<const RepeatActivityList *>(dstPrecedence)->activityList();
//		post_act_list.push_back(dynamic_cast<const RepeatActivityList *>(dstPrecedence)->getMyActivity());
	    } else if (dynamic_cast<const SequentialActivityList *>(dstPrecedence)) {
		post_act_list.push_back(dynamic_cast<const SequentialActivityList *>(dstPrecedence)->getMyActivity());
	    }

	    for ( std::vector<Activity *>::const_iterator activity = post_act_list.begin(); activity != post_act_list.end(); ++activity ) {
		Activity * post_activity = findActivity(*(*activity), replica);
		LQIO::DOM::Activity * post_activity_dom = const_cast<LQIO::DOM::Activity *>(dynamic_cast<const LQIO::DOM::Activity *>(post_activity->getDOM()));
		post_activity->inputFrom( post_list );
		post_activity_dom->inputFrom( post_list_dom );
		post_list->add( post_activity );

		/* A little tricky here.  We need to copy over whatever parameter there was. */

		const LQIO::DOM::ActivityList * dstPrecedenceDOM = dstPrecedence->getDOM();
		const LQIO::DOM::ExternalVariable * parameter = dstPrecedenceDOM->getParameter( dynamic_cast<const LQIO::DOM::Activity *>((*activity)->getDOM()) );
		post_list_dom->add( post_activity_dom, parameter );
	    }

	}
    }
    return *this;
}



LQIO::DOM::Task *
Task::cloneDOM( const std::string& aName, LQIO::DOM::Processor * dom_processor ) const
{
    LQIO::DOM::Task * dom_task = new LQIO::DOM::Task( *dynamic_cast<const LQIO::DOM::Task*>(getDOM()) );

    dom_task->setName( aName );
    dom_task->setProcessor( dom_processor );
    const_cast<LQIO::DOM::Document *>(getDOM()->getDocument())->addTaskEntity( dom_task );	    /* Reconnect all of the dom stuff. */
    dom_processor->addTask( dom_task );

    return dom_task;
}


static struct {
    set_function first;
    get_function second;
} task_mean[] = { 
// static std::pair<set_function,get_function> task_mean[] = {
    { &LQIO::DOM::DocumentObject::setResultProcessorUtilization, &LQIO::DOM::DocumentObject::getResultProcessorUtilization },
    { &LQIO::DOM::DocumentObject::setResultThroughput, &LQIO::DOM::DocumentObject::getResultThroughput },
    { &LQIO::DOM::DocumentObject::setResultUtilization, &LQIO::DOM::DocumentObject::getResultUtilization },
    { NULL, NULL }
};

static struct {
    set_function first;
    get_function second;
} task_variance[] = { 
//static std::pair<set_function,get_function> task_variance[] = {
    { &LQIO::DOM::DocumentObject::setResultProcessorUtilizationVariance, &LQIO::DOM::DocumentObject::getResultProcessorUtilizationVariance },
    { &LQIO::DOM::DocumentObject::setResultThroughputVariance, &LQIO::DOM::DocumentObject::getResultThroughputVariance },
    { &LQIO::DOM::DocumentObject::setResultUtilizationVariance, &LQIO::DOM::DocumentObject::getResultUtilizationVariance },
    { NULL, NULL }
};


/*
 * Rename XXX_1 to XXX and reinsert the task and its dom into their associated arrays.   XXX_2 and up will be discarded.
 */

Task&
Task::replicateTask( LQIO::DOM::DocumentObject ** root )
{
    unsigned int replica = 0;
    std::string root_name = baseReplicaName( replica );
    LQIO::DOM::Task * task = dynamic_cast<LQIO::DOM::Task *>(*root);
    if ( replica == 1 ) {
	*root = const_cast<LQIO::DOM::DocumentObject *>(getDOM());
	task = dynamic_cast<LQIO::DOM::Task *>(*root);
	std::pair<std::set<Task *>::iterator,bool> rc = __tasks.insert( this );
	if ( !rc.second ) throw std::runtime_error( "Duplicate task" );
	task->setName( root_name );
	const_cast<LQIO::DOM::Processor *>(task->getProcessor())->addTask( task );		/* Add back (for XML output) */
	/* Group too?? */
	const_cast<LQIO::DOM::Document *>((*root)->getDocument())->addTaskEntity( task );	/* Reconnect all of the dom stuff. */
    } else if ( task->getReplicasValue() < replica ) {
	task->setReplicasValue( replica );
	for ( unsigned int i = 0; task_mean[i].first != NULL; ++i ) {
	    update_mean( task, task_mean[i].first, getDOM(), task_mean[i].second, replica );
	    update_variance( task, task_variance[i].first, getDOM(), task_variance[i].second );
	}
    }

    for ( std::vector<Activity *>::const_iterator a = activities().begin(); a != activities().end(); ++a ) {
	const LQIO::DOM::Activity * src = dynamic_cast<const LQIO::DOM::Activity *>((*a)->getDOM());		/* The replica 1, 2,... dom */

	/* 
	 * Strip the _n from the replica name.  After pass 1 (replica==1), the "roots" activity
	 * won't have the _n either, so it can be found 
	 */
	
	std::string& name = const_cast<std::string&>(src->getName());
	size_t pos = name.rfind( '_' );
	name = name.substr( 0, pos );
	LQIO::DOM::Activity * activity = const_cast<const LQIO::DOM::Task *>(task)->getActivity(name);
	(*a)->replicateActivity( activity, replica );
    }
    return *this;
}


Task&
Task::replicateCall() 
{
    unsigned int replica = 0;
    std::string root_name = baseReplicaName( replica );
    if ( replica == 1 ) {
	for_each( activities().begin(), activities().end(), Exec<Activity>( &Activity::replicateCall ) );
    }
    return *this;
}

/*
 * Called AFTER replicas are set in all NEW model elements.
 */

/* static */ void
Task::updateFanInOut()
{
    for ( std::set<Task *>::const_iterator src = __tasks.begin(); src != __tasks.end(); ++src ) {
	const LQIO::DOM::Task * src_dom = dynamic_cast<const LQIO::DOM::Task *>((*src)->getDOM());
	const std::vector<Entry *>& entries = (*src)->entries();
	for_each( entries.begin(), entries.end(), UpdateFanInOut( *const_cast<LQIO::DOM::Task *>(src_dom)) );
	const std::vector<Activity *>& activities = (*src)->activities();
	for_each( activities.begin(), activities.end(), UpdateFanInOut( *const_cast<LQIO::DOM::Task *>(src_dom)) );
    }
}

void Task::UpdateFanInOut::operator()( Entry * entry ) const { updateFanInOut( entry->calls() ); }
void Task::UpdateFanInOut::operator()( Activity * activity ) const { updateFanInOut( activity->calls() ); }

void 
Task::UpdateFanInOut::updateFanInOut( const std::vector<Call *>& calls ) const
{
    const unsigned src_replicas = _src.getReplicasValue();
    for ( std::vector<Call *>::const_iterator call = calls.begin(); call != calls.end(); ++call ) {
	LQIO::DOM::Task& dst = *const_cast<LQIO::DOM::Task *>(dynamic_cast<const LQIO::DOM::Task *>((*call)->dstTask()->getDOM()));
	const unsigned dst_replicas = dst.getReplicasValue();
	if ( dst_replicas > src_replicas ) {
	    double fan_out = static_cast<double>(dst_replicas) / static_cast<double>(src_replicas);
	    if ( fan_out == rint( fan_out ) ) {
		_src.setFanOutValue( dst.getName(), static_cast<unsigned>(fan_out) );
	    } else {
		_src.setFanOutValue( dst.getName(), dst_replicas );
		dst.setFanInValue( _src.getName(), src_replicas );
	    }
	} else if ( src_replicas > dst_replicas ) {
	    double fan_in = static_cast<double>(src_replicas) / static_cast<double>(dst_replicas);
	    if ( fan_in == rint( fan_in ) ) {
		dst.setFanInValue( _src.getName(), static_cast<unsigned>(fan_in) );
	    } else {
		_src.setFanOutValue( dst.getName(), dst_replicas );
		dst.setFanInValue( _src.getName(), src_replicas );
	    }
	}
    }
}
#endif

/* ------------------------------------------------------------------------ */
/*                                  Output                                 */
/* ------------------------------------------------------------------------ */

/*
 * Draw the SRVN model object.
 */

const Task&
Task::draw( std::ostream& output ) const
{
    /* see lqiolib/src/srvn_gram.y:task_sched_flag */
    static const std::map<const scheduling_type,const char> task_scheduling = {
	{ SCHEDULE_BURST,       'b' },
	{ SCHEDULE_CUSTOMER,	'r' },
	{ SCHEDULE_DELAY,       'n' },
	{ SCHEDULE_FIFO,	'n' },
	{ SCHEDULE_HOL,	        'h' },
	{ SCHEDULE_POLL,        'P' },
	{ SCHEDULE_PPR,	        'p' },
	{ SCHEDULE_RWLOCK,      'W' },
	{ SCHEDULE_UNIFORM,     'u' }
    };

    std::ostringstream aComment;
    aComment << "Task " << name()
	     << task_scheduling.at( scheduling() )
	     << " " << entries_of( *this );
    if ( processor() ) {
	aComment << " " << processor()->name();
    }
#if defined(BUG_375)
    aComment << " span=" << span() << ", index=" << index();
#endif
    _node->comment( output, aComment.str() );
    _node->fillColour( colour() );
    if ( Flags::colouring() == Colouring::NONE ) {
	_node->penColour( Graphic::Colour::DEFAULT );			// No colour.
    } else if ( Flags::have_results && throughput() == 0.0 ) {
	_node->penColour( Graphic::Colour::RED );
    } else if ( colour() == Graphic::Colour::GREY_10 ) {
	_node->penColour( Graphic::Colour::BLACK );
    } else {
	_node->penColour( colour() );
    }

    std::vector<Point> points(4);
    const double dx = adjustForSlope( fabs(height()) );
    points[0] = topLeft().moveBy( dx, 0 );
    points[1] = topRight();
    points[2] = bottomRight().moveBy( -dx, 0 );
    points[3] = bottomLeft();

    if ( isMultiServer() || isInfinite() || isReplicated() ) {
	std::vector<Point> copies = points;
	const double delta = -2.0 * Model::scaling() * _node->direction();
	for_each( copies.begin(), copies.end(), ExecXY<Point>( &Point::moveBy, 2.0 * Model::scaling(), delta ) );
	const int aDepth = _node->depth();
	_node->depth( aDepth + 1 );
	_node->polygon( output, copies );
	_node->depth( aDepth );
    }
    if ( Flags::graphical_output_style == Output_Style::JLQNDEF ) {
	const double shift = width() - (Flags::entry_width * JLQNDEF_TASK_BOX_SCALING * Model::scaling());
	points[0].moveBy( shift, 0 );
	points[3].moveBy( shift, 0 );
	if ( Flags::colouring() == Colouring::NONE ) {
	    _node->fillColour( Graphic::Colour::GREY_10 );
	}
    }
    _node->polygon( output, points );

    _label->backgroundColour( colour() ).comment( output, aComment.str() );
    output << *_label;

    if ( Flags::aggregation() != Aggregate::ENTRIES ) {
	for_each( entries().begin(), entries().end(), ConstExec1<Element,std::ostream&>( &Element::draw, output ) );
	for_each( activities().begin(), activities().end(), ConstExec1<Element,std::ostream&>( &Element::draw, output ) );
	for_each( precedences().begin(), precedences().end(), ConstExec1<ActivityList,std::ostream&>( &ActivityList::draw, output ) );
    }

    for_each( calls().begin(), calls().end(), ConstExec1<GenericCall,std::ostream&>( &GenericCall::draw, output ) );

    return *this;
}

/* ------------------------------------------------------------------------ */
/*                           Draw the queuieng network                      */
/* ------------------------------------------------------------------------ */

/*
 * Draw the queueing model object.
 */

std::ostream&
Task::drawClient( std::ostream& output, const bool is_in_open_model, const bool is_in_closed_model ) const
{
    std::string aComment;
    aComment += "========== ";
    aComment += name();
    aComment += " ==========";
    _node->comment( output, aComment );
    _node->penColour( colour() == Graphic::Colour::GREY_10 ? Graphic::Colour::BLACK : colour() ).fillColour( colour() );

    _label->moveTo( bottomCenter() )
	.justification( Justification::LEFT );
    if ( is_in_open_model && is_in_closed_model ) {
	Point aPoint = bottomCenter();
	aPoint.moveBy( radius() * -3.0, 0 );
	_node->multi_server( output, aPoint, radius() );
	aPoint = bottomCenter().moveBy( radius() * 1.0, 0.0 );
	_node->open_source( output, aPoint, radius() );
	_label->moveBy( radius() * 0.5, radius() * 4.0 * _node->direction() );
    } else if ( is_in_open_model ) {
	_node->open_source( output, bottomCenter(), radius() );
	_label->moveBy( radius() * 0.5, radius() * _node->direction() );
    } else {
	_node->multi_server( output, bottomCenter(), radius() );
	_label->moveBy( radius() * 0.5, radius() * 4.0 * _node->direction() );
    }
    output << *_label;
    return output;
}

/* ------------------------- Reference Tasks -------------------------- */

ReferenceTask::ReferenceTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const std::vector<Entry *>& entries )
    : Task( dom, aProc, aShare, entries )
{
}

ReferenceTask *
ReferenceTask::clone( unsigned int replica, const std::string& aName, const Processor * aProcessor, const Share * aShare ) const
{
    LQIO::DOM::Task * dom_task = cloneDOM( aName, const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(aProcessor->getDOM()) ) );

//	.setGroup( aShare->getDOM() );

    std::vector<Entry *> entries;
    return new ReferenceTask( dom_task, aProcessor, aShare, groupEntries( replica, entries ) );
}


bool
ReferenceTask::hasThinkTime() const
{
    return dynamic_cast<const LQIO::DOM::Task *>(getDOM())->hasThinkTime();
}

const LQIO::DOM::ExternalVariable&
ReferenceTask::thinkTime() const
{
    return *dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getThinkTime();
}


/*
 * Utilization for reference tasks should be one, unless there is a
 * think time, then it should be < 1.
 */

bool
ReferenceTask::hasBogusUtilization() const
{
    if ( !Flags::have_results ) return false;

    const double u = utilization() / copiesValue();
    const double z = dynamic_cast<const LQIO::DOM::Task *>(getDOM())->getThinkTimeValue();
    return (z == 0. && u < 0.99) || 1.01 < u;
}


/*
 * Reference tasks are always fully utilized, but never a performance
 * problem, so always draw them black.
 */

Graphic::Colour
ReferenceTask::colour() const
{
    switch ( Flags::colouring() ) {
    case Colouring::SERVER_TYPE:
	return Graphic::Colour::RED;

    case Colouring::RESULTS:
	if ( hasBogusUtilization() ) {
	    return Entity::colour();	/* Punt to superclass */
	} else {
	    return Graphic::Colour::DEFAULT;
	}
	break;

    default:
	return Task::colour();
    }
}


/*
 * Recursively find all children and grand children from `father'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.
 */

size_t
ReferenceTask::findChildren( CallStack& callStack, const unsigned directPath )
{
    const size_t depth = std::max( callStack.size(), level() );
    size_t max_depth = depth;

    setLevel( depth ).addPath( directPath );

    for ( std::set<const Processor *>::const_iterator processor = processors().begin(); processor != processors().end(); ++processor ) {
	const_cast<Processor *>(*processor)->setLevel( std::max( (*processor)->level(), depth + 1 ) ).addPath( directPath );
	max_depth = std::max( max_depth, (*processor)->level() );	
    }

    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	max_depth = std::max( max_depth, (*entry)->findChildren( callStack, directPath ) );
    }

    return max_depth;
}


#if BUG_270
Task&
ReferenceTask::relink()
{
    if ( processor()->isInteresting() ) return *this;		/* don't do these! */
    unlinkFromProcessor();
    Model::__zombies.push_back( const_cast<Processor *>(processor()) );
    return *this;
}
#endif


/*
 * This has to be done by class.
 */

void
ReferenceTask::accumulateDemand( BCMP::Model::Station& station ) const
{
    typedef std::pair<const std::string,BCMP::Model::Station::Class> demand_item;
    typedef std::map<const std::string,BCMP::Model::Station::Class> demand_map;
    BCMP::Model::Station::Class demand;
    if ( hasThinkTime() ) {
	demand.setServiceTime( thinkTime().clone() );
    }

    demand_map& demands = const_cast<demand_map&>(station.classes());
    const std::pair<demand_map::iterator,bool> result = demands.insert( demand_item( name(), demand ) );
    result.first->second.accumulate( Task::accumulate_demand( BCMP::Model::Station::Class(), this ) );
}

/* --------------------------- Server Tasks --------------------------- */

ServerTask::ServerTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const std::vector<Entry *>& entries )
    : Task( dom, aProc, aShare, entries )
{
    if ( scheduling() != SCHEDULE_DELAY ) {
	if ( isMultiServer() ) {
	    if ( Pragma::forceInfinite( Pragma::ForceInfinite::MULTISERVERS ) ) {
		const_cast<LQIO::DOM::Task *>(dom)->setSchedulingType(SCHEDULE_DELAY);
	    }
	} else {
	    if ( Pragma::forceInfinite( Pragma::ForceInfinite::FIXED_RATE ) ) {
		const_cast<LQIO::DOM::Task *>(dom)->setSchedulingType(SCHEDULE_DELAY);
	    } else if ( !Pragma::defaultTaskScheduling() ) {
		/* Change scheduling type for fixed rate servers (usually from FCFS to DELAY) */
		const_cast<LQIO::DOM::Task *>(dom)->setSchedulingType(Pragma::taskScheduling());
	    }
	}
    }
}


ServerTask*
ServerTask::clone( unsigned int replica, const std::string& aName, const Processor * aProcessor, const Share * aShare ) const
{
    LQIO::DOM::Task * dom_task = cloneDOM( aName, const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(aProcessor->getDOM()) ) );

//	.setGroup( aShare->getDOM() );

    std::vector<Entry *> entries;
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
	&& (submodel_output() || Flags::include_only() != nullptr )
	&& !isSelected()
	&& !hasOpenArrivals()
	&& !isInfinite()
	&& nEntries() == 1
	&& !processors().empty();
}

/*+ BUG_164 */
/* -------------------------- Semaphore Tasks ------------------------- */

SemaphoreTask::SemaphoreTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const std::vector<Entry *>& entries )
    : Task( dom, aProc, aShare, entries )
{
}


SemaphoreTask*
SemaphoreTask::clone( unsigned int replica, const std::string& aName, const Processor * aProcessor, const Share * aShare ) const
{
    LQIO::DOM::Task * dom_task = cloneDOM( aName, const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(aProcessor->getDOM()) ) );

    std::vector<Entry *> entries;
    return new SemaphoreTask( dom_task, aProcessor, aShare, groupEntries( replica, entries ) );
}


/* -------------------------- RWLOCK Tasks ------------------------- */

RWLockTask::RWLockTask( const LQIO::DOM::Task* dom, const Processor * aProc, const Share * aShare, const std::vector<Entry *>& entries )
    : Task( dom, aProc, aShare, entries )
{
}


RWLockTask*
RWLockTask::clone( unsigned int replica, const std::string& aName, const Processor * aProcessor, const Share * aShare ) const
{
    LQIO::DOM::Task * dom_task = cloneDOM( aName, const_cast<LQIO::DOM::Processor *>(dynamic_cast<const LQIO::DOM::Processor *>(aProcessor->getDOM()) ) );

    std::vector<Entry *> entries;
    return new RWLockTask( dom_task, aProcessor, aShare, groupEntries( replica, entries ) );
}


/*----------------------------------------------------------------------*/
/*		 	   Called from yyarse.  			*/
/*----------------------------------------------------------------------*/

/*
 * Add a task to the model.  Called by the parser.
 */

Task *
Task::create( const LQIO::DOM::Task* task_dom, std::vector<Entry *>& entries )
{
    /* Recover the old parameter information that used to be passed in */
    const char* task_name = task_dom->getName().c_str();
    const LQIO::DOM::Group * group_dom = task_dom->getGroup();
    const scheduling_type sched_type = task_dom->getSchedulingType();

    if ( !task_name || strlen( task_name ) == 0 ) abort();

    if ( entries.size() == 0 ) {
	task_dom->runtime_error( LQIO::ERR_TASK_HAS_NO_ENTRIES );
	return nullptr;
    }
    if ( std::any_of( __tasks.begin(), __tasks.end(), eqTaskStr( task_name ) ) ) {
	task_dom->input_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return nullptr;
    }

    const std::string& processor_name = task_dom->getProcessor()->getName();
    Processor * processor = Processor::find( processor_name );
    if ( !processor ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, processor_name.c_str() );
    }

    const Share * share = 0;
    if ( !group_dom && processor->scheduling() == SCHEDULE_CFS ) {
	LQIO::input_error2( LQIO::ERR_NO_GROUP_SPECIFIED, task_name, processor_name.c_str() );
    } else if ( group_dom ) {
	std::set<Share *>::const_iterator nextShare = find_if( processor->shares().begin(), processor->shares().end(), EQStr<Share>( group_dom->getName() ) );
	if ( nextShare == processor->shares().end() ) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, group_dom->getName().c_str() );
	} else {
	    share = *nextShare;
	}
    }

    /* Pick-a-task */

    Task * task = nullptr;
    switch ( sched_type ) {
    case SCHEDULE_UNIFORM:
    case SCHEDULE_BURST:
    case SCHEDULE_CUSTOMER:
	task = new ReferenceTask( task_dom, processor, share, entries );
	break;

    case SCHEDULE_FIFO:
    case SCHEDULE_PPR:
    case SCHEDULE_HOL:
    case SCHEDULE_DELAY:
	task = new ServerTask( task_dom, processor, share, entries );
	break;

    case SCHEDULE_SEMAPHORE:
	if ( entries.size() != 2 ) {
	    task_dom->runtime_error( LQIO::ERR_TASK_ENTRY_COUNT, entries.size(), 2 );
	}
	task = new SemaphoreTask( task_dom, processor, share, entries );
	break;

	case SCHEDULE_RWLOCK:
	if ( entries.size() != N_RWLOCK_ENTRIES ) {
	    task_dom->runtime_error( LQIO::ERR_TASK_ENTRY_COUNT, entries.size(), N_RWLOCK_ENTRIES );
	}
	task = new RWLockTask( task_dom, processor, share, entries );
	break;

    default:
	task_dom->runtime_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label[(unsigned)sched_type].str );
	task = new ServerTask( task_dom, processor, share, entries );
	break;
    }

    __tasks.insert( task );
    return task;
}

/* ---------------------------------------------------------------------- */

static std::ostream&
entries_of_str( std::ostream& output, const Task& aTask )
{
    for ( std::vector<Entry *>::const_reverse_iterator entry = aTask.entries().rbegin(); entry != aTask.entries().rend(); ++entry ) {
	if ( (*entry)->pathTest() ) {
	    output << " " << (*entry)->name();
	}
    }
    output << " -1";
    return output;
}
