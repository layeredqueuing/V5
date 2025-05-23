/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk/lqns/task.cc $
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
 * $Id: task.cc 17543 2025-04-16 21:42:50Z greg $
 * ------------------------------------------------------------------------
 */


#include "lqns.h"
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <numeric>
#include <ostream>
#include <sstream>
#include <lqio/dom_actlist.h>
#include <lqio/error.h>
#include <lqio/input.h>
#include <lqio/labels.h>
#include <mva/server.h>
#include <mva/ph2serv.h>
#include <mva/multserv.h>
#include <mva/mva.h>
#include "activity.h"
#include "actlist.h"
#include "call.h"
#include "entry.h"
#include "entrythread.h"
#include "errmsg.h"
#include "group.h"
#include "interlock.h"
#include "flags.h"
#include "model.h"
#include "overtake.h"
#include "pragma.h"
#include "processor.h"
#include "submodel.h"
#include "task.h"
#include "variance.h"

/* ------------------------ Constructors etc. ------------------------- */

/*
 * Create a task.
 */

Task::Task( LQIO::DOM::Task* dom, const Processor * aProc, const Group * aGroup, const std::vector<Entry *>& entries)
    : Entity( dom, entries ),
      _processor(aProc),
      _group(aGroup),
      _activities(),
      _precedences(),
      _maxThreads(1),
      _overlapFactor(1.0),
      _threads(1),
      _customers(),
      _clientChains(),
      _clientStation(),
      _has_forks(false),
      _has_syncs(false),
      _has_quorum(false)
{
    for ( auto entry : entries ) entry->owner( this );
}


/*
 * Clone a task (Not PAN_REPLICATION).
 * Find the replica processor (same DOM/name) and possibly group.
 * Clone the entries/activities.
 */


Task::Task( const Task& src, unsigned int replica )
    : Entity( src, replica ),
      _processor(Processor::find( src._processor->name(), static_cast<unsigned>(std::ceil( static_cast<double>(replica) / static_cast<double>(src._processor->fanIn(this) ) ) ) )),
      _group(nullptr),
      _activities(),
      _precedences(),
      _maxThreads(src._maxThreads),
      _overlapFactor(src._overlapFactor),
      _threads(src._threads),
      _customers(src._customers),
      _clientChains(),
      _clientStation(),
      _has_forks(src._has_forks),
      _has_syncs(src._has_syncs),
      _has_quorum(src._has_quorum)
{

    /* Link to the replica group? */
//    _group = Group::find();

    /* Link in the entries -- entries were created before any task is cloned */
    for ( std::vector<Entry *>::const_iterator entry = src._entries.begin(); entry != src._entries.end(); ++entry  ) {
	Entry * new_entry = Entry::find( (*entry)->name(), replica );
	_entries.push_back( new_entry );
	new_entry->owner( this );
    }

    cloneActivities( src, replica );
}


/*
 * Destructor.
 */

Task::~Task()
{
    std::for_each( _clientStation.begin(), _clientStation.end(), []( Server * server ){ delete server; } );
    std::for_each( precedences().begin(), precedences().end(), []( ActivityList * precedence ){ delete precedence; } );
    std::for_each( activities().begin(), activities().end(), []( Activity * activity ){ delete activity; } );
}


/* 
 * Clone the activities, precedences and relink everything.
 */

void
Task::cloneActivities( const Task& src, unsigned int replica )
{
    /* Clone activities */

    for ( std::vector<Activity *>::const_iterator activity = src._activities.begin(); activity != src._activities.end(); ++activity ) {
	Activity * new_activity = (*activity)->clone( this, replica );
	_activities.push_back( new_activity );

	/* Copy start activities (note: virtual entries are done in the fork/repeat activity list code */
	if ( !(*activity)->isStartActivity() ) continue;
	Entry * entry = Entry::find( (*activity)->entry()->name(), replica );
	if ( entry != nullptr && !entry->isVirtualEntry() ) {
	    entry->setStartActivity( new_activity );
	    new_activity->setEntry( entry );
	}
    }

    /* Do the pre(join)-lists from the list retained by the task, the post(fork)-lists will be done after the corresponding pre-list is completed. */

    for ( std::vector<ActivityList *>::const_iterator precedence = src._precedences.begin(); precedence != src._precedences.end(); ++precedence ) {
	if ( !dynamic_cast<const JoinActivityList *>(*precedence) && !dynamic_cast<const AndOrJoinActivityList *>(*precedence) ) continue;

	/* Clone the join list.  The heavy listing is the ActivityList::ActivityList */
	ActivityList * join_list = (*precedence)->clone( this, replica );
	_precedences.push_back( join_list );

	/* Find the original fork list, then clone it's join list */
	if ( (*precedence)->next() == nullptr ) continue;
	ActivityList * fork_list = (*precedence)->next()->clone( this, replica );
	_precedences.push_back( fork_list );

	/* Connect the clones */
	ActivityList::connect( join_list, fork_list );
    }

    /* Reconnect Fork to Join */
    
    linkForkToJoin();
}

/* ----------------------- Abstract Superclass. ----------------------- */

bool
Task::hasClientChain( const unsigned int n, const unsigned int k ) const
{
    return _clientChains[n].find(k) != _clientChains[n].end();
}



/*
 * Return true if any entry or activity has a think time value.  If
 * so, extendModel() will then create an entry to a thinker device.
 */

bool
Task::hasThinkTime() const
{
    return std::any_of( entries().begin(), entries().end(), std::mem_fn( &Entry::hasThinkTime ) )
	|| std::any_of( activities().begin(), activities().end(), std::mem_fn( &Activity::hasThinkTime ) );
}


/*
 *
 */

bool
Task::check() const
{
    bool rc = Entity::check();

    /* Check prio/scheduling. */

    if ( hasProcessor() && priority() != 0 && !getProcessor()->hasPriorities() ) {
	getDOM()->runtime_error( LQIO::WRN_PRIO_TASK_ON_FIFO_PROC, getProcessor()->name().c_str() );
    }

    /* Check replication */

    const LQIO::DOM::Document* document = getDOM()->getDocument();
    for ( const auto& dst : getDOM()->getFanOuts() ) {
	if ( document->getTaskByName( dst.first ) == nullptr ) {
	    LQIO::runtime_error( LQIO::ERR_NOT_DEFINED, dst.first.c_str() );
	    rc = false;
	}
    }
    for ( const auto& src : getDOM()->getFanIns() ) {
	if ( document->getTaskByName( src.first ) == nullptr ) {
	    LQIO::runtime_error( LQIO::ERR_NOT_DEFINED, src.first.c_str() );
	    rc = false;
	}
    }

    /* Check entries */

    if ( entries().empty() ) {
	getDOM()->runtime_error( LQIO::ERR_TASK_HAS_NO_ENTRIES );
	rc = false;
    }

    rc = std::all_of( entries().begin(),entries().end(), std::mem_fn( &Entry::check ) ) && rc;
    if ( hasActivities() ) {
	if ( std::none_of( entries().begin(),entries().end(), std::mem_fn( &Entry::isActivityEntry ) ) ) {
	    getDOM()->runtime_error( LQIO::ERR_NO_START_ACTIVITIES );
	    rc = false;
	} else {
	    rc = std::all_of( activities().begin(), activities().end(), std::mem_fn( &Activity::check ) )
		&&  std::all_of( precedences().begin(), precedences().end(), std::mem_fn( &ActivityList::check ) )
		&& rc;
	}
    }

    return rc;
}



/*
 * Denote whether this station belongs to the open, closed, or mixed
 * models when performing the MVA solution.
 */

Task&
Task::configure( const unsigned nSubmodels )
{
    if ( copies() == 1 && scheduling() != SCHEDULE_DELAY && !Pragma::defaultTaskScheduling() ) {
	/* Change scheduling type for fixed rate servers (usually from FCFS to DELAY) */
	getDOM()->setSchedulingType(Pragma::taskScheduling());
    }

    _clientChains.resize( nSubmodels );		/* Prepare chain vectors	*/
    _clientStation.resize( nSubmodels, 0 );	/* Prepare client cltn		*/

    if ( hasActivities() ) {
	for ( auto activity : activities() ) activity->configure( nSubmodels );
	for ( auto precedence : precedences() ) precedence->configure( nSubmodels );
    }
    Entity::configure( nSubmodels );

    if ( hasOpenArrivals() ) {
	setOpenModelServer( true );
    }
    return *this;
}



/*
 * Recursively find all children and grand children from `father'.  As
 * we descend down, we bump the depth.  If our path's cross, we have a
 * loop and abort.  directPath is true if we are following the call
 * chain directly.  Otherwise findChildren is being used to bump
 * levels lower (if necessary).
 */

unsigned
Task::findChildren( Call::stack& callStack, const bool directPath ) const
{
    return std::accumulate( entries().begin(), entries().end(), Entity::findChildren( callStack, directPath ), [&]( unsigned int depth, const Entry * entry )
	{ return std::max( depth, entry->findChildren( callStack, directPath && entry == callStack.back()->dstEntry() ) ); } );
}



/*
 * Initialize the precedence lists (link forks to joins)
 */

Task&
Task::linkForkToJoin()
{
    _has_forks = std::any_of( precedences().begin(), precedences().end(), std::mem_fn(&ActivityList::isFork) );
    _has_syncs = std::any_of( precedences().begin(), precedences().end(), std::mem_fn(&ActivityList::isSync) );
    _has_quorum = std::any_of( precedences().begin(), precedences().end(), std::mem_fn(&ActivityList::hasQuorum) );

    Call::stack callStack;
    Activity::Ancestors ancestors( callStack, true, false );
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end(); ++activity ) {
	if ( !(*activity)->isStartActivity() ) continue;
	try {
	    (*activity)->findChildren( ancestors );
	}
	catch ( const bad_external_join& error ) {
	    error.getDOM()->runtime_error( LQIO::ERR_BAD_PATH_TO_JOIN, (*activity)->name().c_str() );
	}
	catch ( const activity_cycle& error ) {
	    getDOM()->runtime_error( LQIO::ERR_CYCLE_IN_ACTIVITY_GRAPH, error.what() );
	}
    }
    return *this;
}


/*
 * Calculate population levels.  Punt and use the biggest unsigned int for open streams.
 */

void
Task::initializeClient()
{
    unsigned int customers = 0;
    if ( isReferenceTask() ) {
	customers = copies();
    } else if ( hasOpenArrivals() ) {
	customers = std::numeric_limits<unsigned int>::max();
    }
    if ( customers != 0 ) {
	std::deque<const Task *> stack;
	initCustomers( stack, customers );
    }
}


void
Task::reinitializeClient()
{
    unsigned int customers = 0;
    if ( isReferenceTask() ) {
	customers = copies();
    } else if ( hasOpenArrivals() ) {
	customers = std::numeric_limits<unsigned int>::max();
    }
    if ( customers != 0 ) {
	std::deque<const Task *> stack;
	initCustomers( stack, customers );
    }
}

/*+ BUG_425 */
/*
 * Set the number of customers by chain (reference task/open arrival)
 * when this task is used as a client in a submodel.
 */

Task&
Task::initCustomers( std::deque<const Task *>& stack, unsigned int customers )
{
    if ( std::find( stack.begin(), stack.end(), this ) == stack.end() && customers != 0 ) {
	stack.push_back( this );
#if BUG_425
	std::cerr << std::setw( stack.size() * 2 ) << " " << print_name() << "->Task::initCustomers(" << stack.size() << "," << customers << ")" << std::endl;
#endif
	if ( customers == std::numeric_limits<unsigned int>::max() ) {
	    setOpenModelServer( true );
	    /* An infinite server with open arrivals is an open arrival to the processor */
	    if ( isInfinite() ) {
		const_cast<Processor *>(getProcessor())->setOpenModelServer( true );
	    } else {
		setClosedModelClient( true );
		const_cast<Processor *>(getProcessor())->setClosedModelServer( true );
	    }
	} else {
	    setClosedModelClient( true );
	    if ( !isReferenceTask() ) setClosedModelServer( true );
	    const_cast<Processor *>(getProcessor())->setClosedModelServer( true );	// Always.
	}
	/* When I am a client, the number of customers is limited */
	if ( !isInfinite() ) {
	    customers = std::min( customers, copies() );
	}
	_customers[stack.front()] = customers;
	for ( auto entry : entries() ) entry->initCustomers( stack, customers );
#if BUG_425
	std::cerr << std::setw( stack.size() * 2 ) << " " << print_name() << " pop." << std::endl;
#endif
	stack.pop_back();
    }
    return *this;
}
/*- BUG_425 */



/*+
 * BUG_433
 * Initialize waiting time at my entries.  
 */

void
Task::initializeWait( const Submodel& submodel )
{
    Call::Perform g( &Call::Perform::initWait, submodel );
    std::for_each( entries().begin(), entries().end(), Entry::CallsPerform( g ) );
    updateWait( submodel, 1.0 );
}
/*- BUG_433 */


/*
 * Initialize the processor for all entries and activities.
 */

Task&
Task::initializeProcessor()
{
    std::for_each( entries().begin(), entries().end(), std::mem_fn( &Entry::initializeProcessor ) );
    std::for_each( activities().begin(), activities().end(), std::mem_fn( &Phase::initializeProcessor ) );
    return *this;
}


#if PAN_REPLICATION
/*
 * Allocate storage for oldSurgDelay (used by Newton Raphson
 * iteration.  This step must be done AFTER we have the chain
 * information.  Note -- arrays must be dimensioned on the LARGEST
 * chain number that goes to a particular client, and NOT on the
 * number of chains that visit that client.
 */

Task&
Task::setSurrogateDelaySize( size_t n_chains )
{
    for ( auto entry : entries() ) entry->setSurrogateDelaySize( n_chains );
    for ( auto activity : activities() ) activity->setSurrogateDelaySize( n_chains );
    for ( auto precedence : precedences() ) precedence->setSurrogateDelaySize( n_chains );
    return *this;
}
#endif



/*
 * Initialize interlock table.
 */

Task&
Task::createInterlock()
{
    if ( !Pragma::interlock() ) return *this;
    std::for_each ( entries().begin(), entries().end(), std::mem_fn( &Entry::createInterlock ) );
    return *this;
}



/*
 * Initialize populations.
 */

Task&
Task::initThreads()
{
    _maxThreads = 1;
    if ( hasThreads() ) {
	_maxThreads = std::accumulate( entries().begin(), entries().end(), 0, []( unsigned int l, Entry * r ){ return std::max( l, r->concurrentThreads() ); } );
    }
    if ( _maxThreads > nThreads() ) throw std::logic_error( "Task::initThreads" );
    return *this;
}



/*+ BUG_425 */
/*
 * Return the number of customers at this entity.  If it's an infinite
 * server, it's the sum from all reference tasks (or open arrivals,
 * where it is then infinite.)
 */

unsigned int
Task::population() const
{
    double customers = std::accumulate( _customers.begin(), _customers.end(), static_cast<double>(0.0), Task::add_customers() );
    if ( isInfinite() ) {
	return customers;
    } else {
	return std::min( static_cast<double>(copies()), customers );
    }
}


/*
 * For accumulate.  Make sure we don't overflow unsigned ints.  Open arrivals use the upper limit.
 */

unsigned int
Task::add_customers::add( unsigned int augend, unsigned int addend ) const
{
    if ( augend == std::numeric_limits<unsigned int>::max()
	 || addend == std::numeric_limits<unsigned int>::max()
	 || addend > std::numeric_limits<unsigned int>::max() - augend ) {
	return std::numeric_limits<unsigned int>::max();
    } else {
	return augend + addend;
    }	
};


int
Task::priority() const
{
    try {
	return getDOM()->getPriorityValue();
    }
    catch ( const std::domain_error &e ) {
	getDOM()->throw_invalid_parameter( "priority", e.what() );
    }
    return 0;
}

/*
 * Return the fan-in to this server from...
 */

unsigned
Task::fanIn( const Task * aClient ) const
{
    try {
	return getDOM()->getFanInValue( aClient->name() );
    }
    catch ( const std::domain_error& e ) {
	throw std::domain_error( std::string( "invalid parameter: " ) + e.what() );
    }
    return 1;
}

unsigned
Task::fanOut( const Entity * aServer ) const
{
    try {
	return getDOM()->getFanOutValue( aServer->name() );
    }
    catch ( const std::domain_error& e ) {
	throw std::domain_error( std::string( "invalid parameter: " ) + e.what() );
    }
    return 1;
}


/*
 * Locate the destination anEntry in the list of destinations.
 */

Activity *
Task::findActivity( const std::string& name ) const
{
    const std::vector<Activity *>::const_iterator activity = std::find_if( activities().begin(), activities().end(), [=]( const Activity * a ){ return a->name() == name; } );
    return activity != activities().end() ? *activity : nullptr;
}



/*
 * Locate the activity, and if not found create a new one.
 */

Activity *
Task::findOrAddActivity( const std::string& name )
{
    Activity * anActivity = findActivity( name );

    if ( !anActivity ) {
	anActivity = new Activity( this, name );
	_activities.push_back( anActivity );
    }

    return anActivity;
}



/*
 * Locate the activity, and if not found create a new one.
 */

Activity *
Task::findOrAddPsuedoActivity( const std::string& name )
{
    Activity * anActivity = findActivity( name );

    if ( !anActivity ) {
	anActivity = new PsuedoActivity( this, name );
	_activities.push_back( anActivity );
    }

    return anActivity;
}



void
Task::addPrecedence( ActivityList * precedence )
{
    _precedences.push_back( precedence );
}



#if PAN_REPLICATION
/*
 * Clear replication variables for this pass.
 */

void
Task::clearSurrogateDelay()
{
    std::for_each( entries().begin(), entries().end(), std::mem_fn( &Entry::clearSurrogateDelay ) );
    std::for_each( activities().begin(), activities().end(), std::mem_fn( &Phase::clearSurrogateDelay ) );
}
#endif


/*
 * Return true if any of the entries on the receiver is called
 */

bool
Task::isCalled() const
{
    return std::any_of( entries().begin(), entries().end(), std::mem_fn( &Entry::isCalled ) );
}


bool
Task::hasCalls() const
{
    return std::any_of( entries().begin(), entries().end(), std::mem_fn( &Entry::hasCalls ) )
	|| std::any_of( activities().begin(), activities().end(), std::mem_fn( &Phase::hasCalls ) );
}


/*
 * Return the base replica.
 */

Task*
Task::mapToReplica( size_t i ) const
{
    return find( name(), i );
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
	if ( (*entry)->isCalledUsing( Entry::RequestType::RENDEZVOUS ) || (*entry)->isCalledUsing( Entry::RequestType::SEND_NO_REPLY ) ) {
	    return root_level_t::IS_NON_REFERENCE;	/* Non root task */
	} else if ( (*entry)->isCalledUsing( Entry::RequestType::OPEN_ARRIVAL ) ) {
	    level = root_level_t::HAS_OPEN_ARRIVALS;	/* Root task, but move to lower level */
	}
    }
    return level;
}



/*
 * Expand replicas (Not PAN_REPLICATION)
 */

Task&
Task::expand()
{
    const unsigned int replicas = this->replicas();
    for ( unsigned int replica = 2; replica <= replicas; ++replica ) {
	Task * task = clone( replica );
	Model::__task.insert( task );

	const_cast<Processor *>(task->getProcessor())->addTask( task );
	if ( task->getGroup() ) {
	    const_cast<Group *>(task->getGroup())->addTask( task );
	}
    }
    return *this;
}


/*
 * Expand all calls.  Done are done after all entries have been
 * created and all entries are linked back to tasks.
 */

Task&
Task::expandCalls()
{
    std::for_each( activities().begin(), activities().end(), std::mem_fn( &Phase::expandCalls ) );
    return *this;
}


/*
 * Count number of calling tasks(!) and return.
 */

unsigned
Task::nClients() const
{
    std::set<Task *> callingTasks;
    getClients( callingTasks );

    return callingTasks.size();
}



/*
 * Return all tasks and processors called by this task that are also
 * found in includeOnly.
 */

std::set<Entity *>
Task::getServers( const std::set<Entity *>& includeOnly ) const
{
    std::set<Entity *> servers;
    std::for_each( entries().begin(), entries().end(), Entry::get_servers( servers ) );
    std::for_each( activities().begin(), activities().end(), Phase::get_servers( servers ) );

    std::set<Entity *> result;
    std::set_intersection( servers.begin(), servers.end(), includeOnly.begin(), includeOnly.end(), std::inserter( result, result.begin() ) );
    return result;
}



/*
 * Used by groups.
 */

double
Task::processorUtilization() const
{
    return std::accumulate( entries().begin(), entries().end(),
			    std::accumulate( activities().begin(), activities().end(), 0., []( double sum, const Activity * activity ){ return sum + activity->processorUtilization(); } ),
			    []( double sum, Entry * entry ){ return sum + entry->processorUtilization(); } );
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

    if ( !hasProcessor() || getProcessor()->scheduling() != SCHEDULE_PPR ) return false;

    /*
     * Look for all sourcing tasks.
     */

    std::set<const Task *> sources;

    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	const std::set<Call *>& callerList = (*entry)->callerList();
	for ( std::set<Call *>::const_iterator call = callerList.begin(); call != callerList.end(); ++call ) {
	    sources.insert( (*call)->srcTask() );
	}
    }

    /*
     * If all tasks are on the same processor, then use PPR, otherwise
     * use FIFO
     */

    for ( std::set<const Task *>::const_iterator task = sources.begin(); task != sources.end(); ++task ) {
	if ( (*task)->getProcessor() != getProcessor() ) return false;
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
#if PAN_REPLICATION
    setSurrogateDelaySize( n_chains );
#endif

    Server * station = new Client( nEntries(), n_chains, maxPhase() );

    _clientStation[submodel] = station;
    return station;
}



/*
 * Check results for sanity.
 */

const Entity&
Task::sanityCheck() const
{
    Entity::sanityCheck();

    std::for_each( entries().begin(), entries().end(), []( const Entry * entry ){ if ( !entry->checkDroppedCalls() ) { entry->getDOM()->runtime_error( LQIO::ADV_MESSAGES_DROPPED ); } } );
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
	    return _threads[ix]->thinkTime();
	}
    }
}



/*
 * Compute throughput bounds.  Must be done after initializeWait.
 */

void
Task::computeThroughputBound()
{
    std::for_each( entries().begin(), entries().end(), std::mem_fn( &Entry::computeThroughputBound ) );
}



/*
 * Compute variance at activities before the entries because the
 * entries aggregate the values.
 */

Task&
Task::computeVariance()
{
    std::for_each( activities().begin(), activities().end(), std::mem_fn( &Phase::computeVariance ) );
    Entity::computeVariance();
    return *this;
}



/*
 * Compute change in waiting times for this task.
 */

Task&
Task::updateWait( const Submodel& submodel, const double relax )
{
    /* Do updateWait for each activity first. */

    for ( auto activity : activities() ) activity->updateWait( submodel, relax );

    /* Entry updateWait for activity entries will update waiting times. */

    for ( auto entry : entries() ) entry->updateWait( submodel, relax );

    /* Now recompute thread idle times */

    for ( Vector<Thread *>::const_iterator thread = std::next(threads().begin()); thread != threads().end(); ++thread ) (*thread)->setSubmodelThinkTime( relax );
    return *this;
}



#if PAN_REPLICATION
/*
 * Compute change in waiting times for this task.
 */

double
Task::updateWaitReplication( const Submodel& submodel, unsigned & n_delta )
{
    double delta = 0.0;

    /* Do updateWait for each activity first. */

    for ( auto activity : activities() ) delta += activity->updateWaitReplication( submodel );
    n_delta += activities().size();

    /* Entry updateWait for activity entries will update waiting times. */

    for ( auto entry : entries() ) delta += entry->updateWaitReplication( submodel, n_delta );

    return delta;
}
#endif


/*
 * Dynamic Updates / Late Finalization
 * In order to integrate LQX's support for model changes we need to
 * have a way of re-calculating what used to be static for all
 * dynamically editable values
 */

void
Task::recalculateDynamicValues()
{
    std::for_each( entries().begin(), entries().end(), std::mem_fn( &Entry::recalculateDynamicValues ) );
}


double
Task::bottleneckStrength() const
{
    /* find out who I call */
    return 0;
}

/* ----------------------------- Save Results ----------------------------- */

/*
 * Get and save the waiting time results for all servers to this client
 */

void
Task::saveClientResults( const MVASubmodel& submodel, const Server& station, unsigned int chain ) 
{
    closedCallsPerform( Call::Perform( &Call::Perform::saveWait, submodel ) );
    openCallsPerform( Call::Perform( &Call::Perform::saveOpen, submodel ) );

    if ( !isReferenceTask() || isCalled() ) return;
    
    /* Other results (only useful for references tasks). */

    if ( isClosedModelClient() ) {
	std::for_each( entries().begin(), entries().end(), Entry::SaveClientResults( submodel, station, chain, *this ) );
    }
    setUtilization( computeUtilization( submodel, station ) );
}



/*
 * Set the visit ratios.  Chains can result from both replication and from
 * threads.  _thread[0] is the main entry.
 */

const Task&
Task::closedCallsPerform( Call::Perform g ) const
{
    const ChainVector& chains = _clientChains[g.submodel()];
    unsigned i = 1;

    while ( i <= chains.size() ) {
	g.setChain( chains[i] );
	std::for_each( entries().begin(), entries().end(), Entry::CallsPerform( g ) );		// regular entries
	i = std::for_each( std::next(threads().begin()), threads().end(), Entry::CallsPerformWithChain( g, chains, i + 1 ) ).index();	// threads (fork-join)
    }
    return *this;
}



/*
 * Set the visit ratios from ...
 */

const Task&
Task::openCallsPerform( Call::Perform g ) const
{
    std::for_each( entries().begin(), entries().end(), Entry::CallsPerform( g ) );
    std::for_each( std::next(threads().begin()), threads().end(), Entry::CallsPerform( g ) );
    return *this;
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
    return std::max( (size_t)1, threads().size() );
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
	/* Note: Chain vector starts at one, not zero. */
	const size_t ix = _clientChains[submodel].find( k ) - _clientChains[submodel].begin();
	if ( replicas() > 1 ) {
	    return ix % nThreads() + 1;
	} else {
	    return ix + 1;
	}
    }
}



/*
 * Return the waiting time for all submodels except submodel for phase
 * `p'.  If this is an activity entry, we have to return the chain k
 * component of waiting time.  Note that if submodel == 0, we return
 * the residenceTime().  For servers in a submodel, submodel == 0; for
 * clients in a submodel, submodel == aSubmodel.number().
 */

double
Task::waitExcept( const unsigned ix, const unsigned submodel, const unsigned p ) const
{
    return _threads[ix]->waitExcept( submodel, 0, p );		// k is ignored anyway...
}



#if PAN_REPLICATION
/*
 * Return the waiting time for all submodels except submodel for phase
 * `p'.  If this is an activity entry, we have to return the chain k
 * component of waiting time.  Note that if submodel == 0, we return
 * the residenceTime().  For servers in a submodel, submodel == 0; for
 * clients in a submodel, submodel == aSubmodel.number().
 */

double
Task::waitExceptChain( const unsigned ix, const unsigned submodel, const unsigned k, const unsigned p ) const
{
    return _threads[ix]->waitExceptChain( submodel, k, p );
}
#endif



/*
 * Go through all the chains for the client and generate overlap factors.
 * See (8) [Mak].
 */

void
Task::forkOverlapFactor( const Submodel& submodel ) const
{
    Vector<double>* of = submodel.getOverlapFactor();
    const ChainVector& chain( _clientChains[submodel.number()] );
    const unsigned n = chain.size();

    for ( unsigned i = 1; i <= n; ++i ) {
	for ( unsigned j = 1; j <= n; ++j ) {
	    of[chain[i]][chain[j]] = overlapFactor( i, j );
	}
    }

    if ( flags.trace_forks ) {
	printOverlapTable( std::cout, chain, of );
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
		|| _threads[i]->isAncestorOf( _threads[j] )
		|| _threads[i]->isDescendentOf( _threads[j] ) ) {

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

	const double x_i = _threads[i]->estimateCDF().mean();
	const double x_j = _threads[j]->estimateCDF().mean();

	/* ---- Overlap probabilities ---- */

	if ( x_i == 0.0 || x_j == 0.0 ) {		/* Degeneate case */
	    pr = 0.0;

	} else if ( _threads[i]->isSiblingOf(  _threads[j] ) ) {
	    pr = 1.0;

	} else {					/* Partial overlap */

	    const Exponential start_i = _threads[i]->startTime();
	    const Exponential start_j = _threads[j]->startTime();
	    const Exponential end_i   = start_i + *_threads[i];
	    const Exponential end_j   = start_j + *_threads[j];

	    pr = ( 1.0 - ( Pr_A_lt_B( end_j, start_i ) +
			   Pr_A_lt_B( end_i, start_j ) ) );	/* Eqn 6, (8) */
	}

	if ( pr > 0.0 ) {

	    /* ---- Overlap interval ---- */

	    double d_ij = min( *_threads[i], *_threads[j] );

	    if ( !Pragma::threads(Pragma::Threads::MAK_LUNDSTROM) && _threads[i]->residenceTime() != 0 ) {
		d_ij = d_ij *  (_threads[j]->residenceTime() / _threads[i]->residenceTime());			// tomari quorum BUG 257
	    }

	    /* ---- Final overal factor ---- */

	    theta = pr * d_ij / x_i;

	    if ( Pragma::threads(Pragma::Threads::HYPER) ) {
		theta *= (_threads[j]->residenceTime() / _threads[i]->residenceTime());
		const double inflation = _threads[j]->residenceTime() * throughput();
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

    return theta * _overlapFactor;	/* Scale to parent value */
}

/*----------------------------------------------------------------------*/
/*                           Synchronization                            */
/*----------------------------------------------------------------------*/

/*
 * Go through all the chains for the client and generate overlap factors.
 * See (8) [Mak].
 */

void
Task::joinOverlapFactor( const Submodel& aSubmodel ) const
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
    std::set<ActivityList *> joinLists;
    Activity * localQuorumDelayActivity;
    Activity * finalActivity;
    int quorumListNumber = 0;

    //Get Join Lists
    for ( std::vector<Activity *>::const_iterator activity = activities().begin(); activity != activities().end() && (*activity)->nextJoin() ; ++activity ) {
	joinLists.insert((*activity)->nextJoin());
    }
    // cout <<"\njoinLists.size () = " << joinLists.size() << endl;

    for ( std::set<ActivityList *>::const_iterator join_list = joinLists.begin(); join_list != joinLists.end(); ++join_list ) {
	AndJoinActivityList *  quorumAndJoinList = dynamic_cast<AndJoinActivityList *>(*join_list);
	if (!quorumAndJoinList) { continue; }

	AndForkActivityList *  newAndForkList = nullptr;
	AndJoinActivityList *  newAndJoinList = nullptr;

        if ( quorumAndJoinList->hasQuorum() ) {

	    //check that there is no replying activity inside he quorum
	    //fork-join list since we cannot solve for a quorum with a
	    //two-phase semantics.

	    const std::vector<const Activity *>& cltnQuorumJoinActivities = quorumAndJoinList->activityList();
	    for ( std::vector<const Activity *>::const_iterator activity = cltnQuorumJoinActivities.begin(); activity != cltnQuorumJoinActivities.end(); ++activity ) {
		const std::set<const Entry *>& aQuorumReplyList = (*activity)->replyList();
		if (!aQuorumReplyList.empty()) {
		    std::cout <<"\nTask::expandQuorumGraph(): Error detected in input file.";
		    std::cout <<" A quorum join list cannot have a replying activity." << std::endl;
		    std::cout << "This is not implemented. No output will be generated." << std::endl;
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
		localQuorumDelayActivity->_remote_quorum_delay.active(true);
		char _remote_quorum_delayName[32];
		sprintf( remoteQuorumDelayName, "remoteQmDelay_%d", quorumListNumber);
		localQuorumDelayActivity->_remote_quorum_delay.nameSet(remoteQuorumDelayName);
	    } else {
		std::cout <<"\nTask::expandQuorumGraph(): Error, could not create localQuorumDelay activity" << std::endl;
		abort();
	    }
	    char finalActivityName[32];
	    sprintf( finalActivityName, "final_%d", quorumListNumber);
	    finalActivity =  findOrAddPsuedoActivity(finalActivityName);

	    //to force the local delay (sumTotal of localQuorumDelayActivity) to use a gamma distribution fitting.
	    //localQuorumDelayActivity->phaseTypeFlag(PHASE_DETERMINISTIC);

	    localQuorumDelayActivity->localQuorumDelay(true);

	    localQuorumDelayActivity->setServiceTime(0.);
//	    store_activity_service_time ( localQuorumDelayActivity->name(), 0 );
	    finalActivity->setServiceTime(0.);
//	    store_activity_service_time ( finalActivityName, 0 );

	    newAndJoinList = dynamic_cast<AndJoinActivityList *>(localQuorumDelayActivity->act_and_join_list( newAndJoinList, 0 ));
	    if ( newAndJoinList == nullptr ) throw std::logic_error( "Task::expandQuorumGraph" );
	    newAndForkList= dynamic_cast<AndForkActivityList *> (localQuorumDelayActivity->act_and_fork_list( newAndForkList, 0 ));
	    if ( newAndForkList == nullptr ) throw std::logic_error( "Task::expandQuorumGraph" );

#if 0
#warning dead code and probably broken... graph changes
	    ActivityList * orgForkList = quorumAndJoinList->next();
	    if (orgForkList) {
		Vector<Activity *> cltnForkActivities;
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

		    ActivityList::connect( quorumAndJoinList, newAndForkList );
		    ForkActivityList * finalAndForkList = dynamic_cast<ForkActivityList *>(finalActivity->act_fork_item( 0 ));
		    ActivityList::connect( newAndJoinList, finalAndForkList );
		}
	    }
#endif
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
 * Save all task results.
 */

const Task&
Task::insertDOMResults(void) const
{
    if ( getReplicaNumber() != 1 ) return *this;		/* NOP */

    Entity::insertDOMResults();
    
    if ( hasActivities() ) {
	std::for_each( activities().begin(), activities().end(), std::mem_fn( &Phase::insertDOMResults ) );
	std::for_each( precedences().begin(), precedences().end(), std::mem_fn( &ActivityList::insertDOMResults ) );
    }

    double totalPhaseUtils[MAX_PHASES];
    double resultPhaseData[MAX_PHASES];

    for ( unsigned p = 0; p < MAX_PHASES; ++p ) {
	totalPhaseUtils[p] = 0.0;
	resultPhaseData[p] = 0.0;
    }

    for ( std::vector<Entry *>::const_iterator entry = entries().begin(); entry != entries().end(); ++entry ) {
	(*entry)->computeVariance()
	    .insertDOMResults(&totalPhaseUtils[0]);
    }

    /* Store totals */

    double totalTaskUtil   = 0.0;
    for ( unsigned p = 0; p < maxPhase(); ++p ) {
	totalTaskUtil += totalPhaseUtils[p];
	resultPhaseData[p] = totalPhaseUtils[p];
    }

    /* Place all of the totals into the DOM task itself */
    getDOM()->setResultPhaseUtilizations(maxPhase(), resultPhaseData);
    getDOM()->setResultUtilization(totalTaskUtil);
    getDOM()->setResultThroughput( throughput() );
    getDOM()->setResultProcessorUtilization( processorUtilization() );
    getDOM()->setResultBottleneckStrength(0);
    return *this;
}


/* --------------------------- Debugging. --------------------------- */

/*
 * Debug - print out waiting by submodel.
 */

std::ostream&
Task::printSubmodelWait( std::ostream& output ) const
{
    for ( const auto entry : entries() ) entry->printSubmodelWait( output, 0 );
    if ( flags.trace_virtual_entry ) {
	for ( const auto precedence : precedences() ) precedence->printSubmodelWait( output, 2 );
    } else {
	for ( Vector<Thread *>::const_iterator thread = std::next(threads().begin()); thread != threads().end(); ++thread ) (*thread)->printSubmodelWait( output, 0 );
    }
    return output;
}


/*
 * Print chains for this client.
 */

std::ostream&
Task::printClientChains( std::ostream& output, const unsigned submodel ) const
{
    output << "Chains:" << _clientChains[submodel] << std::endl;
    return output;
}


/*
 * Tracing: Print out the overlap table.
 */

std::ostream&
Task::printOverlapTable( std::ostream& output, const ChainVector& chain, const Vector<double>* of ) const
{

    //To handle the case of a main thread of control with no fork join.
    unsigned n = nThreads(); // unsigned n = chain.size();


    unsigned i;
    unsigned j;
    int precision = output.precision(3);
    std::ios_base::fmtflags flags = output.setf( std::ios::left, std::ios::adjustfield );
    output.setf( std::ios::left, std::ios::adjustfield );

    /* Overlap table.  */

    output << "-- Fork Overlap Table -- " << std::endl << name() << std::endl << std::setw(6) << " ";
    for ( i = 2; i <= n ; ++i ) {
	output << std::setw(6) << _threads[i]->name();
    }
    output << std::endl;

    for ( i = 2; i <= n; ++i ) {
	output << std::setw(6) << _threads[i]->name();
	for ( j = 1; j <= n; ++j ) {
	    output << std::setw(6) << of[chain[i]][chain[j]];
	}
	output << std::endl;
    }
    output << std::endl << std::endl;

    output.flags( flags );
    output.precision( precision );
    return output;
}



/*
 * Tracing -- Print out the joins delays.
 */

std::ostream&
Task::printJoinDelay( std::ostream& output ) const
{
    for ( const auto precedence : precedences() ) precedence->printJoinDelay( output );
    return output;
}

/* ------------------------- Reference Tasks. ------------------------- */

ReferenceTask::ReferenceTask( LQIO::DOM::Task* dom, const Processor * processor, const Group * group, const std::vector<Entry *>& entries )
    : Task( dom, processor, group, entries )
{
    setClosedModelClient( true );
}




Task *
ReferenceTask::clone( unsigned int replica )
{
    return new ReferenceTask( *this, replica  );
}


unsigned
ReferenceTask::copies() const
{
    try {
	return getDOM()->getCopiesValue();
    }
    catch ( const std::domain_error &e ) {
	getDOM()->throw_invalid_parameter( "multiplicity", e.what() );
    }
    return 1;
}



/*
 * Reference tasks are never servers, but still need to be initialized
 * like one (throughputs and what have you.)  See Entity::initServer()
 */

void
ReferenceTask::initializeClient()
{
    Task::initializeClient();
    initializeServer();
}



/*
 */

void
ReferenceTask::reinitializeClient()
{
    Task::reinitializeClient();
    reinitializeServer();
}




void
ReferenceTask::setSubmodel( const unsigned submodel )
{
    assert( submodel == 1 );
    Entity::setSubmodel( submodel );
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

    try {
	_thinkTime = dynamic_cast<LQIO::DOM::Task *>(getDOM())->getThinkTimeValue();
    }
    catch ( const std::domain_error& e ) {
	getDOM()->throw_invalid_parameter( "think time", e.what() );
    }
}


/*
 * Valid reference task?
 */

bool
ReferenceTask::check() const
{
    Task::check();

    if ( nEntries() != 1 ) {
	LQIO::DOM::DocumentObject::setSeverity(LQIO::ERR_TASK_ENTRY_COUNT, LQIO::error_severity::WARNING );
	getDOM()->runtime_error( LQIO::ERR_TASK_ENTRY_COUNT, nEntries(), 1 );
    }
    if ( getDOM()->hasQueueLength() ) {
	getDOM()->runtime_error( LQIO::ERR_NOT_SUPPORTED, "queue length" );
    }
    if ( !hasCalls() ) {
	getDOM()->runtime_error( LQIO::WRN_NOT_USED );
    }
    
    const double sum = std::accumulate( entries().begin(), entries().end(), 0.0, []( double l, const Entry * r ){ return l + r->prVisit(); } );
    if ( sum < 1.0 - EPSILON || 1.0 + EPSILON < sum ) {
	getDOM()->runtime_error( LQIO::ERR_INVALID_VISIT_PROBABILITY, sum );
    }

    return true;
}


/*
 * Reference tasks are always "direct paths" for the purposes of forwarding.
 */

unsigned
ReferenceTask::findChildren( Call::stack& callStack, const bool ) const
{
    return std::accumulate( entries().begin(), entries().end(), Entity::findChildren( callStack, true ), find_max_depth( callStack ) );
}


unsigned int
ReferenceTask::find_max_depth::operator()( unsigned int depth, const Entry * entry )
{
    return std::max( depth, entry->findChildren( _callStack, true ) );
}


/*
 * Reference tasks cannot be servers by definition.
 */

Server *
ReferenceTask::makeServer( const unsigned )
{
    throw LQIO::should_not_implement( "ReferenceTask::makeServer" );
    return nullptr;
}



/*
 * Check utilization.  It's too hard to do if there's think time because it should be < 1
 */

const Task&
ReferenceTask::sanityCheck() const
{
    const double u = utilization() / copies();
    if ( (!(hasThinkTime() || thinkTime() > 0.) && u < 0.99) || 1.01 < u ) {
	LQIO::runtime_error( ADV_INVALID_UTILIZATION, getDOM()->getTypeName(), name().c_str(), copies(), utilization() );
    }
    return *this;
}

/* -------------------------- Simple Servers. ------------------------- */

Task *
ServerTask::clone( unsigned int replica )
{
    return new ServerTask( *this, replica );
}


unsigned int
ServerTask::queueLength() const
{
    try {
	return getDOM()->getQueueLengthValue();
    }
    catch ( const std::domain_error& e ) {
	getDOM()->throw_invalid_parameter( "queue length", e.what() );
    }
    return 0;
}

bool
ServerTask::check() const
{
    bool rc = Task::check();

    if ( scheduling() == SCHEDULE_DELAY && copies() != 1 ) {
	getDOM()->runtime_error( LQIO::WRN_INFINITE_MULTI_SERVER, copies() );
	getDOM()->setCopiesValue(1);
    }
    if ( std::any_of( entries().begin(), entries().end(), std::mem_fn( &Entry::hasVisitProbability ) ) ) {
	getDOM()->runtime_error( LQIO::WRN_TASK_HAS_VISIT_PROBABILITY );
    }
    if ( isInfinite() && (std::any_of( entries().begin(), entries().end(), []( const Entry * entry ){ return entry->isCalledUsing( Entry::RequestType::SEND_NO_REPLY ); } )
			  || std::any_of( entries().begin(), entries().end(), []( const Entry * entry ) { return entry->isCalledUsing( Entry::RequestType::OPEN_ARRIVAL ); } ) ) ) {
	getDOM()->runtime_error( LQIO::WRN_INFINITE_SERVER_OPEN_ARRIVALS );
    }

    return rc;
}


/*
 * Return true if the population is infinite (i.e., an open source)
 */

bool
ServerTask::hasInfinitePopulation() const
{
    return isInfinite() && population() == std::numeric_limits<unsigned int>::max();
}



/*
 * Indicate whether the variance calculation should take place.
 */

bool
ServerTask::hasVariance() const
{
    return !isInfinite()
	&& !isMultiServer()
	&& !Pragma::forceMultiserver( Pragma::ForceMultiserver::TASKS )
	&& Entity::hasVariance();
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

bool
ServerTask::schedulingIsOK() const
{
    return isInfinite() && scheduling() == SCHEDULE_DELAY
	|| !isMultiServer() && scheduling() == SCHEDULE_HOL
	|| scheduling() == SCHEDULE_FIFO
	|| scheduling() == SCHEDULE_LIFO
	|| scheduling() == SCHEDULE_RAND;
}


/*
 * Create (or recreate) a server.  If we're called a a second+ time,
 * and the station type changes, then we change the underlying
 * station.  We only return a station when we create one.
 *
 * Note: one can't take the address of a constructor in C++ so we use
 * switch statements.
 */

Server *
ServerTask::makeServer( const unsigned nChains )
{
    if ( isInfinite() ) {

	/* ---------------- Infinite Servers ---------------- */

	if ( dynamic_cast<Infinite_Server *>(_station) ) return nullptr;
	_station = new Infinite_Server( nEntries(), nChains, maxPhase() );

    } else if ( isMultiServer() || Pragma::forceMultiserver( Pragma::ForceMultiserver::TASKS ) ) {

	/* ---------------- Multi Servers ---------------- */

	if ( !hasSecondPhase() || Pragma::overtaking( Pragma::Overtaking::NONE ) ) {

	    switch ( Pragma::multiserver() ) {
	    default:
	    case Pragma::Multiserver::DEFAULT:
		if ( (copies() < 128 && nChains <= 5) || isOpenModelServer() ) {
		    if ( dynamic_cast<Conway_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies() ) return nullptr;
		    _station = new Conway_Multi_Server( copies(), nEntries(), nChains, maxPhase());
		} else {
		    if ( dynamic_cast<Rolia_Multi_Server *>(_station) && _station->mu() == copies() ) return nullptr;
		    _station = new Rolia_Multi_Server(  copies(), nEntries(), nChains, maxPhase());
		}
		break;

	    case Pragma::Multiserver::BRUELL:
		if ( dynamic_cast<Bruell_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Bruell_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::CONWAY:
		if ( dynamic_cast<Conway_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies() ) return nullptr;
		_station = new Conway_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::REISER:
		if ( dynamic_cast<Reiser_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies() ) return nullptr;
		_station = new Reiser_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::REISER_PS:
		if ( dynamic_cast<Reiser_PS_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies() ) return nullptr;
		_station = new Reiser_PS_Multi_Server( copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::ROLIA:
		if ( dynamic_cast<Rolia_Multi_Server *>(_station) && _station->mu() == copies() ) return nullptr;
		_station = new Rolia_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::ROLIA_PS:
		if ( dynamic_cast<Rolia_PS_Multi_Server *>(_station) && _station->mu() == copies() ) return nullptr;
		_station = new Rolia_PS_Multi_Server(  copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::SCHMIDT:
		if ( dynamic_cast<Schmidt_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Schmidt_Multi_Server(   copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::SURI:
		if ( dynamic_cast<Suri_Multi_Server *>(_station)  && _station->mu() == copies()) return nullptr;
		_station = new Suri_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::ZHOU:
		if ( dynamic_cast<Zhou_Multi_Server *>(_station)  && _station->mu() == copies()) return nullptr;
		_station = new Zhou_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;
	    }

	} else if ( markovOvertaking() ) {

	    switch ( Pragma::multiserver() ) {
	    default:
	    case Pragma::Multiserver::DEFAULT:
	    case Pragma::Multiserver::CONWAY:
	    case Pragma::Multiserver::SCHMIDT:
		if ( dynamic_cast<Markov_Phased_Conway_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Markov_Phased_Conway_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::REISER:
		if ( dynamic_cast<Markov_Phased_Reiser_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Markov_Phased_Reiser_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::ROLIA:
		if ( dynamic_cast<Markov_Phased_Rolia_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies()) return nullptr;
		_station = new Markov_Phased_Rolia_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::REISER_PS:
	    case Pragma::Multiserver::ROLIA_PS:
		if ( dynamic_cast<Markov_Phased_Rolia_PS_Multi_Server *>(_station) && _station->mu() == copies()) return nullptr;
		_station = new Markov_Phased_Rolia_PS_Multi_Server(  copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::SURI:
		if ( dynamic_cast<Markov_Phased_Suri_Multi_Server *>(_station) && _station->mu() == copies()) return nullptr;
		throw LQIO::not_implemented( "Task::makeServer" );
		_station = new Markov_Phased_Suri_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::ZHOU:
		if ( dynamic_cast<Markov_Phased_Zhou_Multi_Server *>(_station) && _station->mu() == copies()) return nullptr;
		_station = new Markov_Phased_Zhou_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;
	    }

	} else {

	    switch ( Pragma::multiserver() ) {
	    default:
	    case Pragma::Multiserver::DEFAULT:
	    case Pragma::Multiserver::CONWAY:
	    case Pragma::Multiserver::SCHMIDT:
		if ( dynamic_cast<Phased_Conway_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies() ) return nullptr;
		_station = new Phased_Conway_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::REISER:
	    case Pragma::Multiserver::REISER_PS:
		if ( dynamic_cast<Phased_Reiser_Multi_Server *>(_station) && _station->getMarginalProbabilitiesSize() == copies() ) return nullptr;
		_station = new Phased_Reiser_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;


	    case Pragma::Multiserver::ROLIA:
		if ( dynamic_cast<Phased_Rolia_Multi_Server *>(_station) && _station->mu() == copies() ) return nullptr;
		_station = new Phased_Rolia_Multi_Server(     copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::ROLIA_PS:
		if ( dynamic_cast<Phased_Rolia_PS_Multi_Server *>(_station) && _station->mu() == copies() ) return nullptr;
		_station = new Phased_Rolia_PS_Multi_Server(  copies(), nEntries(), nChains, maxPhase());
		break;

	    case Pragma::Multiserver::ZHOU:
		if ( dynamic_cast<Phased_Zhou_Multi_Server *>(_station)  && _station->mu() == copies()) return nullptr;
		_station = new Phased_Zhou_Multi_Server(    copies(), nEntries(), nChains, maxPhase());
		break;

	    }
	}

    } else if ( hasVariance() ) {

	/* ---------------- Simple Servers ---------------- */

	/* Stations with variance calculation used.	*/

	if ( !hasSecondPhase() || Pragma::overtaking( Pragma::Overtaking::NONE ) ) {
	    if ( HOL_Scheduling() ) {
		if ( dynamic_cast<HOL_HVFCFS_Server *>(_station) ) return nullptr;
		_station = new HOL_HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else if ( PPR_Scheduling() ) {
		if ( dynamic_cast<PR_HVFCFS_Server *>(_station) ) return nullptr;
		_station = new PR_HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<HVFCFS_Server *>(_station) ) return nullptr;
		_station = new HVFCFS_Server( nEntries(), nChains, maxPhase() );
	    }
	} else switch( Pragma::overtaking() ) {
	    case Pragma::Overtaking::ROLIA:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_HVFCFS_Rolia_Phased_Server *>(_station) ) return nullptr;
		    _station = new HOL_HVFCFS_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_HVFCFS_Rolia_Phased_Server *>(_station) ) return nullptr;
		    _station = new PR_HVFCFS_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<HVFCFS_Rolia_Phased_Server *>(_station) ) return nullptr;
		    _station = new HVFCFS_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    case Pragma::Overtaking::SIMPLE:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_HVFCFS_Simple_Phased_Server *>(_station) ) return nullptr;
		    _station = new HOL_HVFCFS_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_HVFCFS_Simple_Phased_Server *>(_station) ) return nullptr;
		    _station = new PR_HVFCFS_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<HVFCFS_Simple_Phased_Server *>(_station) ) return nullptr;
		    _station = new HVFCFS_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    case Pragma::Overtaking::MARKOV:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_HVFCFS_Markov_Phased_Server *>(_station) ) return nullptr;
		    _station = new HOL_HVFCFS_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_HVFCFS_Markov_Phased_Server *>(_station) ) return nullptr;
		    _station = new PR_HVFCFS_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<HVFCFS_Markov_Phased_Server *>(_station) ) return nullptr;
		    _station = new HVFCFS_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    default:
		LQIO::internal_error( __FILE__, __LINE__, "ServerTask::makeServer" );
		break;

	    }

    } else {

	/* Stations withOUT variance.			*/

	if ( !hasSecondPhase() || Pragma::overtaking( Pragma::Overtaking::NONE ) ) {
	    if ( HOL_Scheduling() ) {
		if ( dynamic_cast<HOL_FCFS_Server *>(_station) ) return nullptr;
		_station = new HOL_FCFS_Server( nEntries(), nChains, maxPhase() );
	    } else if ( PPR_Scheduling() ) {
		if ( dynamic_cast<PR_FCFS_Server *>(_station) ) return nullptr;
		_station = new PR_FCFS_Server( nEntries(), nChains, maxPhase() );
	    } else {
		if ( dynamic_cast<FCFS_Server *>(_station) ) return nullptr;
		_station = new FCFS_Server( nEntries(), nChains, maxPhase() );
	    }

	} else switch( Pragma::overtaking() ) {
	    case Pragma::Overtaking::ROLIA:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_Rolia_Phased_Server *>(_station) ) return nullptr;
		    _station = new HOL_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_Rolia_Phased_Server *>(_station) ) return nullptr;
		    _station = new PR_Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<Rolia_Phased_Server *>(_station) ) return nullptr;
		    _station = new Rolia_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    case Pragma::Overtaking::SIMPLE:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_Simple_Phased_Server *>(_station) ) return nullptr;
		    _station = new HOL_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_Simple_Phased_Server *>(_station) ) return nullptr;
		    _station = new PR_Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<Simple_Phased_Server *>(_station) ) return nullptr;
		    _station = new Simple_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    case Pragma::Overtaking::MARKOV:
		if ( HOL_Scheduling() ) {
		    if ( dynamic_cast<HOL_Markov_Phased_Server *>(_station) ) return nullptr;
		    _station = new HOL_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		} else if ( PPR_Scheduling() ){
		    if ( dynamic_cast<PR_Markov_Phased_Server *>(_station) ) return nullptr;
		    _station = new PR_Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		} else {
		    if ( dynamic_cast<Markov_Phased_Server *>(_station) ) return nullptr;
		    _station = new Markov_Phased_Server( nEntries(), nChains, maxPhase() );
		}
		break;

	    default:
		LQIO::internal_error( __FILE__, __LINE__, "ServerTask::makeServer" );
		break;
	    }
    }

    return _station;
}

/* ------------------------- Semaphore Servers. ----------------------- */

Task *
SemaphoreTask::clone( unsigned int replica )
{
    return new SemaphoreTask( *this, replica );
}


bool
SemaphoreTask::check() const
{
    bool rc = true;
    if ( nEntries() != 2 ) {
	LQIO::DOM::DocumentObject::setSeverity(LQIO::ERR_TASK_ENTRY_COUNT, LQIO::error_severity::ERROR );
	getDOM()->runtime_error( LQIO::ERR_TASK_ENTRY_COUNT, nEntries(), N_SEMAPHORE_ENTRIES );
	rc = false;
    } else if ( !((entryAt(1)->isSignalEntry() && entryAt(2)->entrySemaphoreTypeOk(LQIO::DOM::Entry::Semaphore::WAIT))
	   || (entryAt(1)->isWaitEntry() && entryAt(2)->entrySemaphoreTypeOk(LQIO::DOM::Entry::Semaphore::SIGNAL))
	   || (entryAt(2)->isSignalEntry() && entryAt(1)->entrySemaphoreTypeOk(LQIO::DOM::Entry::Semaphore::WAIT))
	   || (entryAt(2)->isWaitEntry() && entryAt(1)->entrySemaphoreTypeOk(LQIO::DOM::Entry::Semaphore::SIGNAL))) ) {
	getDOM()->runtime_error( LQIO::ERR_NO_SEMAPHORE );
	rc = false;
    }

    return rc;
}


SemaphoreTask&
SemaphoreTask::configure( const unsigned )
{
    throw LQIO::not_implemented( "SemaphoreTask::configure" );
    return *this;
}

Server *
SemaphoreTask::makeServer( const unsigned )
{
    throw LQIO::not_implemented( "SemaphoreTask::configure" );
}

/* ----------------------- External functions. ------------------------ */

/*
 * Add a task to the model.  Called by the parser.
 */

Task*
Task::create( LQIO::DOM::Task* dom, const std::vector<Entry *>& entries )
{
    if ( dom == nullptr || dom->getName().empty() ) abort();
    const std::string& task_name = dom->getName();

    if ( Task::find( task_name ) ) {
	dom->runtime_error( LQIO::ERR_DUPLICATE_SYMBOL );
	return nullptr;
    } else if ( entries.empty() ) {
	dom->runtime_error( LQIO::ERR_TASK_HAS_NO_ENTRIES );
	return nullptr;
    }

    const std::string& processor_name = dom->getProcessor()->getName();
    Processor * processor = Processor::find( processor_name );

    if ( !processor ) {
	LQIO::input_error( LQIO::ERR_NOT_DEFINED, processor_name.c_str() );
	return nullptr;
    }

    const LQIO::DOM::Group * group_dom = dom->getGroup();
    Group * group = nullptr;
    if ( !group_dom && processor->scheduling() == SCHEDULE_CFS ) {
	dom->runtime_error( LQIO::ERR_NO_GROUP_SPECIFIED, processor_name.c_str() );
    } else if ( group_dom ) {
	const std::string& group_name = group_dom->getName();
	group = Group::find( group_name );
	if ( !group ) {
	    LQIO::input_error( LQIO::ERR_NOT_DEFINED, group_name.c_str() );
	}
    }

    /* Pick-a-task */

    Task * task = nullptr;

    const scheduling_type sched_type = dom->getSchedulingType();
    if ( sched_type != SCHEDULE_CUSTOMER && dom->hasThinkTime () ) {
	dom->runtime_error( LQIO::ERR_NON_REF_THINK_TIME );
    }

    switch ( sched_type ) {

	/* ---------- Client tasks ---------- */
    case SCHEDULE_BURST:
    case SCHEDULE_UNIFORM:
	dom->runtime_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label.at(sched_type).str.c_str() );
	/* Fall through */
    case SCHEDULE_CUSTOMER:
	task = new ReferenceTask( dom, processor, group, entries );
	break;

	/* ---------- Generic Server tasks ---------- */
    case SCHEDULE_DELAY:
    case SCHEDULE_FIFO:
    case SCHEDULE_PPR:
    case SCHEDULE_HOL:
	task = new ServerTask( dom, processor, group, entries );
	break;

	/* ---------- Special Cases ---------- */
	/*+ BUG_164 */
    case SCHEDULE_SEMAPHORE:
	if ( entries.size() != N_SEMAPHORE_ENTRIES ) {
	    dom->runtime_error( LQIO::ERR_TASK_ENTRY_COUNT, entries.size(), N_SEMAPHORE_ENTRIES );
	}
	LQIO::input_error( LQIO::ERR_NOT_SUPPORTED, "Semaphore tasks" );
	//	task = new SemaphoreTask( task_name, n_copies, replications, processor, entries, priority );
	//	break;
	/* fall through for now */
	/*- BUG_164 */

    default:
	dom->runtime_error( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label.at(sched_type).str.c_str() );
	dom->setSchedulingType(SCHEDULE_FIFO);
	task = new ServerTask( dom, processor, group, entries );
	break;
    }

    Model::__task.insert( task );		/* Insert into map */
    processor->addTask( task );
    if ( group ) {
	group->addTask( task );
    }
    return task;
}



/* static */ Task *
Task::find( const std::string& name, unsigned int replica )
{
    std::set<Task *>::const_iterator task = std::find_if( Model::__task.begin(), Model::__task.end(), EqualsReplica<Task>( name, replica ) );
    return ( task != Model::__task.end() ) ? *task : nullptr;
}

/*----------------------------------------------------------------------*/
/*                               Printing                               */
/*----------------------------------------------------------------------*/

/*
 * Generic printer.
 */

std::ostream&
Task::print( std::ostream& output ) const
{
    std::ios_base::fmtflags oldFlags = output.setf( std::ios::left, std::ios::adjustfield );

    std::ostringstream ss;
    ss << print_name();
    output << std::setw(10) << ss.str()
	   << " " << std::setw(15) << print_info()
	   << " " << std::setw(9)  << print_type();
    ss.str("");
    if ( hasProcessor() ) {
	ss << getProcessor()->print_name();
    } else {
	ss << "--";
    }
    output << " " << std::setw(10) << ss.str()
	   << " " << std::setw(3)  << priority();
    if ( isReferenceTask() && thinkTime() > 0.0 ) {
	output << " " << thinkTime();
    }
    output << " " << print_entries();
    if ( activities().size() > 0 ) {
	output << " : " << print_activities();
    }

    /* Bonus information about stations -- derived by solver */

    if ( nThreads() > 1 ) {
	output << " {" << nThreads() << " threads}";
    }

    output.flags(oldFlags);
    return output;
}

/* -------------------------------------------------------------------- */
/* Funky Formatting functions for inline with <<.			*/
/* -------------------------------------------------------------------- */

/* static */ std::ostream&
Task::output_activities( std::ostream& output, const Task& task )
{
    const std::vector<Activity *>& activities = task.activities();
    output << std::accumulate( std::next(activities.begin()), activities.end(), activities.front()->name(), &Activity::fold );
    return output;
}
