/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996								*/
/* June 2009								*/
/************************************************************************/

/*
 * Input output processing.
 *
 * $Id$
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <parasol.h>
#include "lqsim.h"
#if defined(HAVE_REGEX_H)
#include <regex.h>
#endif
#include <lqio/input.h>
#include <lqio/labels.h>
#include <lqio/error.h>
#include <lqio/dom_actlist.h>
#include <lqio/dom_extvar.h>
#include "errmsg.h"
#include "task.h"
#include "instance.h"
#include "activity.h"
#include "processor.h"
#include "group.h"

using namespace std;

#define N_SEMAPHORE_ENTRIES 2
#define N_RWLOCK_ENTRIES 4

set <Task *, ltTask> task;	/* Task table.	*/

const char * Task::type_strings[] =
{
    "Undefined",
    "client",
    "server",
    "multi",
    "infsrv",
    "sync",
    "semph",
    "open",
    "worker",
    "thread",
    "token",
    "token_r",
    "signal",
    "rw_lock",
    "rwlock",
    "token_w"
};


/*
 * n_tasks represents the number of task classes defined in the input
 * file.  total_tasks is the total number of tasks created.  The latter
 * will always be greater than or equal to the former because it includes
 * all "clones" of multi-server tasks.  Use n_tasks as an upper bound to
 * class_tab, and total_tasks as an upper bound to object_tab.
 */

unsigned total_tasks = 0;

Task::Task( const task_type type, LQIO::DOM::Task* domTask, Processor * processor, Group * a_group )
    : _dom_task(domTask),
      _processor(processor),
      _group_id(-1),
      _type(type),
      _entry(),
      _activity(),
      _act_list(),
      trace_flag(false),
      join_start_time(0.0),
      compute_func(0),
      entry_status(0),
      free_messages(0),
      _joins(),
      _forks(),
      max_phases(1),
      max_activities(0),
      active(0),
      cpu_active(0),
      hold_active(0),
      _hist_data(0),
      _msg_tab(0)
{
    /*
     * Set compute function.  We have a special case for when no
     * processor has been allocated (eg, processor 0).  In this case,
     * tasks compute by "sleeping".
     */

    compute_func = (!processor || processor->is_infinite()) ? ps_sleep : ps_compute;
    if ( processor ) {
	processor->add_task(this);
    }
}


Task::~Task()
{
    if ( has_activities() ) {
	for ( vector<Activity *>::iterator ap = _activity.begin(); ap != _activity.end(); ++ap ) {
	    delete (*ap);
	}
	
	for ( vector<ActivityList *>::iterator next_actlist = _act_list.begin(); next_actlist != _act_list.end(); ++next_actlist ) {
	    free( *next_actlist );
	}

	_activity.clear();
	_act_list.clear();
	_forks.clear();
	_joins.clear();
    }
    if ( _hist_data ) {
	delete _hist_data;
    }
}


/*
 * Configure entries and activities.  Count up the number of
 * activities.  Do final list construction (i.e., tie up loose ends.)
 */

void
Task::create()
{
    double total_calls = 0;

    /* JOIN Stuff -- All entries are free. */
		
    entry_status = (entry_status_t *)my_malloc( sizeof( struct entry_status_t ) * _entry.size() );
    for ( unsigned i = 0; i < _entry.size(); ++i ) {
	entry_status[i].activity = 0;
	entry_status[i].head = 0;
	entry_status[i].tail = 0;
	_entry[i]->set_owner(this).set_index(i);
    }

#if HAVE_REGCOMP
    trace_flag	= (bool)(task_match_pattern != 0 && regexec( task_match_pattern, (char *)name(), 0, 0, 0 ) != REG_NOMATCH );
#else
    trace_flag = false;
#endif

    if ( debug_flag ){
	(void) fprintf( stddbg, "\n-+++++---- %s task %s", type_name(), name() );
	if ( compute_func == ps_sleep ) {
	    (void) fprintf( stddbg, " [delay]" );
	}
	(void) fprintf( stddbg, " ----+++++-\n" );
    }

    /* Compute PDF for all activities for each task. */

    unsigned int i = 0;
    for ( vector<Activity *>::iterator ap = _activity.begin(); ap != _activity.end(); ++ap, ++i ) {
	(*ap)->index = i;
    }
    max_activities = i;

    create_instance();

    /* I need the instance variable "task" set from this point on. */

    for ( vector<Activity *>::iterator ap = _activity.begin(); ap != _activity.end(); ++ap ) {
	total_calls += (*ap)->configure();
    }

    for ( vector<ActivityList *>::iterator lp = _act_list.begin(); lp != _act_list.end(); ++lp ) {
	configure_list( *lp );
    }

    for ( vector<Entry *>::const_iterator nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	Entry * ep = *nextEntry;
	total_calls += ep->configure();
    }

    if ( total_calls == 0 && is_reference_task() ) { 
	LQIO::solution_error( LQIO::WRN_NO_SENDS_FROM_REF_TASK, name() );
    }

    for ( vector<Activity *>::iterator ap = _activity.begin(); ap != _activity.end(); ++ap ) {
	if ( !(*ap)->is_reachable ) {
	    LQIO::solution_error( LQIO::WRN_NOT_USED, "Activity", (*ap)->name() );
	} else if ( !(*ap)->is_specified() ) {
	    LQIO::solution_error( LQIO::ERR_ACTIVITY_NOT_SPECIFIED, name(), (*ap)->name() );
	}
    }


    /* Create "links" where necessary. */

    build_links();

    if ( has_send_no_reply() ) {
	alloc_pool();
    }


    /*
     * Allocate statistics for joins.  We do it here because we
     * don't know how many joins we have when we allocate the task
     * class object.
     */
	
    for ( vector<ActivityList *>::iterator lp = _forks.begin(); lp != _forks.end(); ++lp ) {
	(*lp)->u.fork.visit_count = 0;
	for ( unsigned j = 0; j < (*lp)->na; ++j ) {
	    if ( (*lp)->u.fork.visit[j] ) {
		(*lp)->u.fork.visit_count += 1;
	    }
	}

    }
    for ( vector<ActivityList *>::iterator lp = _joins.begin(); lp != _joins.end(); ++lp ) {
	const Activity * src = (*lp)->list[0];
	const Activity * dst = (*lp)->list[(*lp)->na-1];

	join_check( (*lp) );	/* check fork-join matching*/
	(*lp)->u.join.r_join.init( SAMPLE, "Join delay %-11.11s %-11.11s ", src->name(), dst->name() );
	(*lp)->u.join.r_join_sqr.init( SAMPLE, "Join delay squared %-11.11s %-11.11s ", src->name(), dst->name() );
    }

    /* statistics */
    active = 0;			/* Reset counts */
    cpu_active = 0;
    hold_active = 0;

    r_cycle.init( SAMPLE,        "%s %-11.11s - Cycle Time        ", type_name(), name() );
    r_util.init( VARIABLE,       "%s %-11.11s - Utilization       ", type_name(), name() );
    r_group_util.init( VARIABLE, "%s %-11.11s - Group Utilization ", type_name(), name() );
}

int
Task::node_id() const
{
    return processor() ? processor()->node_id() : 0;
}

void 
Task::set_start_activity (LQIO::DOM::Entry* theDOMEntry)
{
    const char * entry_name = theDOMEntry->getName().c_str();
    Entry * ep = Entry::find( entry_name );
	
    if ( !ep ) return;
    if ( !ep->test_and_set( LQIO::DOM::Entry::ENTRY_ACTIVITY ) ) return;

    const LQIO::DOM::Activity * domActivity = theDOMEntry->getStartActivity();
    const char * activity_name = domActivity->getName().c_str();
    Activity * ap = find_activity( activity_name );
    if ( !ap ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, activity_name );
    } else if ( ep->activity ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_START_ACTIVITY, entry_name, activity_name );
    } else {
	ep->activity = ap;
	ap->is_start_activity = true;
    }
}

/*
 * add links between tasks to simulate communication delays.
 */

void
Task::build_links()
{
    for ( unsigned j = 0; j < n_entries(); ++j ) {
	for ( unsigned p = 1; p <= MAX_PHASES; ++p ) {
	    vector<tar_t>::iterator tp;
	    for ( tp = _entry[j]->phase[p].tinfo.target.begin(); tp != _entry[j]->phase[p].tinfo.target.end(); ++tp ) {
		Processor * proc = tp->entry->task()->processor();
		if ( proc != processor() && inter_proc_delay > 0.0 ) {
		    const int h = proc->node_id();
		    if ( static_cast<int>(link_tab[h]) == -1 ) {
			char link_name[BUFSIZ];
						
			(void) sprintf( link_name, "%s.%s", name(), proc->name() );

			/*
			 * !!!DANGER!!!!
			 * Stupid parasol insists on massaging the transmission rate,
			 * we have to fudge it here.
			 */

			link_tab[h] = ps_build_link( link_name,
						     processor()->node_id(),
						     h,
						     (double)LINKS_MESSAGE_SIZE * 2 / inter_proc_delay,
						     TRUE );
		    }
		    tp->set_link( link_tab[h] );
		}
	    }
	}
    }
}



bool
Task::has_send_no_reply() const
{
    vector<Entry *>::const_iterator nextEntry;
    for ( nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	const Entry * ep = *nextEntry;
	if ( ep->is_send_no_reply()  ) return true;
    }
    return  false;
}

/* 
 * Create a new activity assigned to a given task and set the information DOM entry for it 
 */

Activity * 
Task::add_activity( LQIO::DOM::Activity * dom_activity )
{
    Activity * activity = find_activity( dom_activity->getName().c_str() );
    if ( activity ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Activity", dom_activity->getName().c_str() );
    } else {
	activity = new Activity( this, dom_activity );
	_activity.push_back( activity );
    }
    return activity;
}




/*
 * Find the activity.  Return error if not found.
 */

Activity *
Task::find_activity( const char * activity_name ) const
{
    vector<Activity *>::const_iterator ap = find_if( _activity.begin(), _activity.end(), eqActivityStr( activity_name ) );
    if ( ap != _activity.end() ) {
	return *ap;
    } else {
	return 0;
    }
}


/*
 * allocate message pool for asynchronous messages.  
 */

void
Task::alloc_pool()
{
    const unsigned size = queue_length() > 0 ? queue_length() : DEFAULT_QUEUE_SIZE;
    if ( !_msg_tab ) {
	_msg_tab = new Message[size];
    }
    for ( unsigned i = 1; i < size; ++i ) {
	_msg_tab[i-1].next = &_msg_tab[i];
    }
    _msg_tab[size-1].next = 0;
    free_messages = _msg_tab;
}


double
Task::throughput() const
{
    switch ( type() ) {
    case Task::SEMAPHORE:  return r_cycle.mean_count() / (Model::block_period() * n_entries()); 		/* Only count for one entry.  */
    case Task::RWLOCK:	   return r_cycle.mean_count() / (Model::block_period() * n_entries()/2); 	/* Only count for two entries.  */
    case Task::SERVER:	   if ( isSynchServer() ) return  r_cycle.mean_count() / (Model::block_period() * n_entries());
	/* Fall through */
    default: 		   return r_cycle.mean_count() / Model::block_period();
    }
}



double
Task::throughput_variance() const
{
    switch ( type() ) {
    case Task::SEMAPHORE:  return r_cycle.variance_count() / (square(Model::block_period()) * n_entries());
    case Task::RWLOCK:     return r_cycle.variance_count() / (square(Model::block_period()) * n_entries()/2);
    case Task::SERVER:	   if ( isSynchServer() ) return r_cycle.variance_count() / (square(Model::block_period()) * n_entries());
	/* Fall through */
    default:		   return r_cycle.variance_count() / square(Model::block_period());
    }
}
	
void
Task::reset_stats()
{
    r_util.reset();
    r_cycle.reset();

    for ( vector<Entry *>::const_iterator nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	Entry * ep = *nextEntry;

	ep->r_cycle.reset();
	for ( unsigned p = 1; p <= max_phases; ++p ) {
	    ep->phase[p].reset_stats();
	}

	/* Forwarding */
	    
	if ( ep->is_rendezvous() ) {
	    ep->fwd.reset_stats();
	}
    }

    for ( vector<Activity *>::iterator ap = _activity.begin(); ap != _activity.end(); ++ap ) {
	(*ap)->reset_stats();
    }

    for ( vector<ActivityList *>::iterator lp = _joins.begin(); lp != _joins.end(); ++lp ) {
	(*lp)->u.join.r_join.reset();
	(*lp)->u.join.r_join_sqr.reset();
	if ( (*lp)->u.join._hist_data ) {
	    (*lp)->u.join._hist_data->reset();
	}
    }

    /* Histogram stuff */
 
    if ( _hist_data ) {
	_hist_data->reset();
    }
}


void
Task::accumulate_data()
{
    if ( type() == Task::UNDEFINED ) return;    /* Some tasks don't have statistics */

    r_util.accumulate();
    r_cycle.accumulate();

    for ( vector<Entry *>::const_iterator nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	Entry * ep = *nextEntry;
			
	ep->r_cycle.accumulate();
	for ( unsigned p = 1; p <= max_phases; ++p ) {
	    ep->phase[p].accumulate_data();
	}

	/* Forwarding */

	if ( ep->is_rendezvous() ) {
	    ep->fwd.accumulate();
	}
				
    }

    for ( vector<Activity *>::iterator ap = _activity.begin(); ap != _activity.end(); ++ap ) {
	(*ap)->accumulate_data();
    }

    for ( vector<ActivityList *>::iterator lp = _joins.begin(); lp != _joins.end(); ++lp ) {
	(*lp)->u.join.r_join_sqr.accumulate_variance( (*lp)->u.join.r_join.accumulate() );
	if ( (*lp)->u.join._hist_data ) {
	    (*lp)->u.join._hist_data->accumulate_data();
	}
    }

    /* Histogram stuff */

    if ( _hist_data ) {
	_hist_data->accumulate_data();
    }
}


FILE *
Task::print( FILE * output ) const
{
    r_util.print_raw( output,     "%-6.6s %-11.11s - Utilization", type_name(), name() );
    r_cycle.print_raw( output,    "%-6.6s %-11.11s - Cycle Time ", type_name(), name() );

    for ( vector<Entry *>::const_iterator nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	Entry *ep = *nextEntry;
	unsigned p;
	ep->r_cycle.print_raw( output, "Entry %-11.11s  - Cycle Time      ", ep->name() );

	for ( p = 1; p <= max_phases; ++p ) {
	    ep->phase[p].print_raw_stat( output  );
	}
    }

    for ( vector<Activity *>::const_iterator ap = _activity.begin(); ap != _activity.end(); ++ap ) {
	(*ap)->print_raw_stat( output );
    }

    for ( vector<ActivityList *>::const_iterator lp = _joins.begin(); lp != _joins.end(); ++lp ) {
	(*lp)->u.join.r_join.print_raw( output, "%-6.6s %-11.11s - Join Delay ", type_name(), name() );
	(*lp)->u.join.r_join_sqr.print_raw( output, "%-6.6s %-11.11s - Join DelSqr", type_name(), name() );
    }

    return output;
}

void
Task::insertDOMResults()
{
    /* Some tasks don't have statistics */

    if ( type() == Task::UNDEFINED ) return;

    double phaseUtil[MAX_PHASES+1];
    double phaseVar[MAX_PHASES+1];
    for ( unsigned p = 0; p < MAX_PHASES+1; p++ ) {
	phaseUtil[p] = 0.0;
	phaseVar[p] = 0.0;
    }

    if ( has_activities() ) {
	for ( vector<Activity *>::iterator ap = _activity.begin(); ap != _activity.end(); ++ap ) {
	    (*ap)->insertDOMResults();
	}

	/* Do the fork/join results here */

	for ( vector<ActivityList *>::iterator lp = _joins.begin(); lp != _joins.end(); ++lp ) {
	    LQIO::DOM::AndJoinActivityList * dom_actlist = dynamic_cast<LQIO::DOM::AndJoinActivityList *>((*lp)->_dom_actlist);
	    if ( dom_actlist ) {
		dom_actlist->setResultJoinDelay((*lp)->u.join.r_join.mean())
		    .setResultVarianceJoinDelay((*lp)->u.join.r_join_sqr.mean());
		
		if ( number_blocks > 1 ) {
		    dom_actlist->setResultJoinDelayVariance( (*lp)->u.join.r_join.variance())
			.setResultVarianceJoinDelayVariance((*lp)->u.join.r_join_sqr.variance());
		}

		if ( (*lp)->u.join._hist_data ) {
		    (*lp)->u.join._hist_data->insertDOMResults();
		}
	    }
	}
    }

    double taskProcUtil = 0.0;		/* Total processor utilization. */
    double taskProcVar = 0.0;
    for ( vector<Entry *>::const_iterator nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	Entry * ep = *nextEntry;
	ep->insertDOMResults();

	for ( unsigned p = 1; p <= max_phases; ++p ) {
	    phaseUtil[p-1] += ep->phase[p].r_util.mean();
	    phaseVar[p-1]  += ep->phase[p].r_util.variance();
	    taskProcUtil   += ep->phase[p].r_cpu_util.mean();
	    taskProcVar    += ep->phase[p].r_cpu_util.variance();
	}
    }

    /* Store totals */
	
    getDOM()->setResultPhaseUtilizations(max_phases,phaseUtil)
	.setResultUtilization(r_util.mean())
	.setResultThroughput(throughput())
	.setResultProcessorUtilization(taskProcUtil);

    if ( number_blocks > 1 ) {
	getDOM()->setResultPhaseUtilizationVariances(max_phases,phaseVar)
	    .setResultUtilizationVariance(r_util.variance())
	    .setResultThroughputVariance(throughput_variance())
	    .setResultProcessorUtilizationVariance(taskProcVar);
    }

    if ( _hist_data ) {
	_hist_data->insertDOMResults();
    }
}

/*----------------------------------------------------------------------*/
/*	 Input processing.  Called from configure() 			*/
/*----------------------------------------------------------------------*/

/*
 * Add a task to the model.
 *
 * NB:	The parser returns a mallocated string for task_name which
 * 	should be freed but once!
 */

Task * 
Task::add( LQIO::DOM::Task* domTask )
{
    /* Recover the old parameter information that used to be passed in */
    const char* task_name = domTask->getName().c_str();
    const LQIO::DOM::Group * domGroup = domTask->getGroup();
    const scheduling_type sched_type = domTask->getSchedulingType();
    const LQIO::DOM::ExternalVariable * n_copies = domTask->getCopies();
    
    if ( !task_name || strlen( task_name ) == 0 ) abort();

    if ( Task::find( task_name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Task", task_name );
    }
    if ( domTask->getReplicas() != 1 ) {
	LQIO::input_error2( ERR_REPLICATION, "task", task_name );
    }

    Task * cp = 0;
    const char* processor_name = domTask->getProcessor()->getName().c_str();
    Processor * processor = Processor::find( processor_name );

    unsigned priority = domTask->getPriority();
    if ( priority != 0 && ( processor->discipline() == SCHEDULE_FIFO
			    || processor->discipline() == SCHEDULE_PS
			    || processor->discipline() == SCHEDULE_RAND ) ) {
	LQIO::input_error2( LQIO::WRN_PRIO_TASK_ON_FIFO_PROC, task_name, processor_name );
    } else if ( priority < MIN_PRIORITY || MAX_PRIORITY < priority ) {
	LQIO::input_error2( WRN_INVALID_PRIORITY, priority, MIN_PRIORITY, MAX_PRIORITY, priority );
	domTask->setPriority(MAX_PRIORITY);
    }

    Group * group = 0;
    if ( !domGroup && processor->discipline() == SCHEDULE_CFS ) {
	LQIO::input_error2( LQIO::ERR_NO_GROUP_SPECIFIED, task_name, processor_name );
    } else if ( domGroup ) {
	group = Group::find( domGroup->getName().c_str() );
	if ( !group ) {
	    LQIO::input_error2( LQIO::ERR_NOT_DEFINED, domGroup->getName().c_str() );
	}
    }

    switch ( sched_type ) {
    case SCHEDULE_BURST:
    case SCHEDULE_UNIFORM:
    case SCHEDULE_CUSTOMER:
	if ( domTask->hasQueueLength() ) {
	    input_error2( LQIO::WRN_QUEUE_LENGTH, task_name );
	}
	cp = new Reference_Task( Task::CLIENT, domTask, processor, group );
	break;

    case SCHEDULE_PPR:
    case SCHEDULE_HOL:
    case SCHEDULE_FIFO:
	if ( domTask->hasThinkTime() ) {
	    input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name );
	}
	Task::task_type a_type;
	if ( !n_copies->wasSet() || to_double( *n_copies ) > 1 ) {
	    a_type = Task::MULTI_SERVER;
	} else if ( to_double( *n_copies ) == 1 ) {
	    a_type = Task::SERVER;
	} else {
	    a_type = Task::INFINITE_SERVER;
	}
	cp = new Server_Task( a_type, domTask, processor, group );
	break;

    case SCHEDULE_DELAY:
	if ( domTask->hasThinkTime() ) {
	    input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name );
	}
	if ( !n_copies->wasSet() || to_double( *n_copies ) != 1 ) {
	    LQIO::input_error2( LQIO::WRN_INFINITE_MULTI_SERVER, "Task", task_name, n_copies );
	}	
	if ( domTask->hasQueueLength() ) {
	    LQIO::input_error2( LQIO::WRN_QUEUE_LENGTH, task_name );
	}
	cp = new Server_Task( Task::INFINITE_SERVER, domTask, processor, group );
	break;

/*+ BUG_164 */
    case SCHEDULE_SEMAPHORE:
	if ( domTask->hasQueueLength()  ) {
	    input_error2( LQIO::WRN_QUEUE_LENGTH, task_name );
	}
	if ( !n_copies->wasSet() || to_double( *n_copies ) != 1 ) {
	    input_error2( LQIO::ERR_INFINITE_TASK, task_name );
	}
 	cp = new Semaphore_Task( Task::SEMAPHORE, domTask, processor, group );
	break;
/*- BUG_164 */
	
/* reader_writer lock */ 

    case SCHEDULE_RWLOCK:
	if ( domTask->hasQueueLength() ) {
	    input_error2( LQIO::WRN_QUEUE_LENGTH, task_name );
	}
	if ( !n_copies->wasSet() || to_double( *n_copies ) == 0 ) {
	    input_error2( LQIO::ERR_RWLOCK_CONCURRENT_READERS, task_name );
	}
 	cp = new ReadWriteLock_Task( Task::RWLOCK, domTask, processor, group );
	break;
/* reader_writer lock*/

    default:
	cp = new Server_Task( Task::SERVER, domTask, processor, group );		/* Punt... */
	input_error2( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_type_str[sched_type], "task", task_name );
	break;
    }

    ::task.insert( cp );

    return cp;
}


/*
 * Located the task id.
 */

Task *
Task::find( const char * task_name )
{
    set<Task *,ltTask>::const_iterator nextTask = find_if( ::task.begin(), ::task.end(), eqTaskStr( task_name ) );
    if ( nextTask == ::task.end() ) {
	return 0;
    } else {
	return *nextTask;
    }
}


/*
 * We need a way to fake out infinity... so if copies is infinite,
 * then we change to an infinite server.
 */

unsigned
Task::multiplicity() const
{
    const LQIO::DOM::ExternalVariable * dom_copies = getDOM()->getCopies(); 
    double value;
    assert(dom_copies->getValue(value) == true);
    if ( isinf( value ) ) return 1;
    assert( value - floor(value) == 0 );
    return static_cast<unsigned int>(value);
}



bool 
Task::is_infinite() const
{
    const LQIO::DOM::ExternalVariable * dom_copies = getDOM()->getCopies(); 
    double value;
    assert(dom_copies->getValue(value) == true);
    if ( isinf( value ) ) {
	return true;
    } else { 
	return discipline() == SCHEDULE_DELAY; 
    }
}


bool
Task::derive_utilization() const
{ 
    return processor()->derive_utilization(); 
}


bool
Task::has_lost_messages() const
{
    for ( vector<Entry *>::const_iterator nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	if ( (*nextEntry)->has_lost_messages() ) return true;
    }
    return false;
}

/* ------------------------------------------------------------------------ */

Reference_Task::Reference_Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup )
    : Task( type, domTask, aProc, aGroup ), _think_time(0.0)
{
}


void
Reference_Task::create_instance()
{
    if ( n_entries() != 1 ) {
	solution_error( LQIO::WRN_TOO_MANY_ENTRIES_FOR_REF_TASK, name() );
    } 
    if ( getDOM()->hasThinkTime() ) {
	_think_time = getDOM()->getThinkTimeValue();
    }
    _task_list.clear();
    for ( unsigned i = 0; i < multiplicity(); ++i ) {
	_task_list.push_back( new srn_client( this, name() ) );
    }
}


bool
Reference_Task::start()
{
    for ( vector<srn_client *>::const_iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	srn_client * task = *t;
	if ( ps_resume( task->task_id() ) != OK ) return false;
    }
    return true;
}


void
Reference_Task::kill()
{
    for ( vector<srn_client *>::const_iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	srn_client * task = *t;
	ps_kill( task->task_id() );
    }
}

/* ------------------------------------------------------------------------ */

Server_Task::Server_Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup )
    : Task( type, domTask, aProc, aGroup ),
      _task(0),
      _worker_port(-1),
      _sync_server(false)
{
}

int
Server_Task::std_port() const
{
    return ps_std_port(_task->task_id());
}

void
Server_Task::set_synchronization_server() 
{
    assert( type() == SERVER && _task );		/* we must be a server and must have run. */
    _sync_server = true;
}


/*
 * Define task type at run time.  It may change because of LQX
 * execution.  Simple servers are more efficient than multi-servers
 * with 1 worker.
 */

void
Server_Task::create_instance()
{
    if ( is_infinite() ) {
	_task = new srn_multiserver( this, name(), ~0 );
	_worker_port = ps_allocate_port( name(), _task->task_id() );
	_type = Task::INFINITE_SERVER;
    } else if ( is_multiserver() ) {
	_task = new srn_multiserver( this, name(), multiplicity() );
	_worker_port = ps_allocate_port( name(), _task->task_id() );
	_type = Task::MULTI_SERVER;
    } else {
	_task = new srn_server( this, name() );
	_worker_port = -1;
	_type = Task::SERVER;
    }
}


bool
Server_Task::start()
{
    return ps_resume( _task->task_id() ) == OK;
}


void
Server_Task::kill()
{
    ps_suspend( _task->task_id() );
    ps_kill( _task->task_id() );
    _task = 0;
    _worker_port = -1;
}

/* ------------------------------------------------------------------------ */

Semaphore_Task::Semaphore_Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup )
    : Server_Task( type, domTask, aProc, aGroup ),
      _signal_task(0),
      _signal_port(-1)
{
}

void
Semaphore_Task::create()
{
    Task::create();

    r_hold.init( SAMPLE,         "%s %-11.11s - Hold Time         ", type_name(), name() );
    r_hold_sqr.init( SAMPLE,     "%s %-11.11s - Hold Time Sq      ", type_name(), name() );
    r_hold_util.init( VARIABLE,  "%s %-11.11s - Hold Utilization  ", type_name(), name() );
}

void
Semaphore_Task::create_instance()
{
    if ( n_entries() != N_SEMAPHORE_ENTRIES ) {
	LQIO::solution_error( LQIO::ERR_ENTRY_COUNT_FOR_TASK, name(), n_entries(), N_SEMAPHORE_ENTRIES );
    }
    if ( _entry[0]->is_signal() ) {
	if ( !_entry[1]->test_and_set_semaphore( SEMAPHORE_WAIT ) ) {
	    LQIO::solution_error( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
	}
	if ( !_entry[1]->test_and_set_recv( Entry::RECEIVE_RENDEZVOUS ) ) {
	    LQIO::solution_error( LQIO::ERR_ASYNC_REQUEST_TO_WAIT, _entry[1]->name() );
	}
    } else if ( _entry[0]->is_wait() ) {
	if ( !_entry[1]->test_and_set_semaphore( SEMAPHORE_SIGNAL ) ) {
	    LQIO::solution_error( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
	}
	if ( !_entry[0]->test_and_set_recv( Entry::RECEIVE_RENDEZVOUS ) ) {
	    LQIO::solution_error( LQIO::ERR_ASYNC_REQUEST_TO_WAIT, _entry[0]->name() );
	}
    } else {
	LQIO::solution_error( LQIO::ERR_NO_SEMAPHORE, name() );
	cerr << "entry names: " << _entry[0]->name() << ", " << _entry[1]->name() << endl;
    }
    if ( !_hist_data && getDOM()->hasHistogram() ) {
	_hist_data = new Histogram( getDOM()->getHistogram() );
    }

    string buf = name();
    buf += "-wait";
    /* entry for waiting request - send to token. */
    _task = new srn_semaphore( this, buf.c_str() );
    _worker_port = ps_allocate_port( buf.c_str(), _task->task_id() );

    /* Entry for signal request */
    buf = name();
    buf += "-signal";
    _signal_task = new srn_signal( this,  buf.c_str() );
    _signal_port = ps_allocate_port( buf.c_str(), _signal_task->task_id() );
}


bool
Semaphore_Task::start()
{
    if ( !Server_Task::start() ) return false;
    return ps_resume( _signal_task->task_id() ) == OK;
}

void
Semaphore_Task::kill()
{
    Server_Task::kill();
    ps_kill( _signal_task->task_id() );
    _signal_port  = -1;
}

void
Semaphore_Task::reset_stats()
{
    Task::reset_stats();

    r_hold.reset();
    r_hold_sqr.reset();
    r_hold_util.reset();
}

void
Semaphore_Task::accumulate_data()
{
    Task::accumulate_data();

    r_hold_sqr.accumulate_variance( r_hold.accumulate() );
    r_hold_util.accumulate();
}

void
Semaphore_Task::insertDOMResults()
{
    Task::insertDOMResults();

    LQIO::DOM::SemaphoreTask * dom = dynamic_cast<LQIO::DOM::SemaphoreTask *>(getDOM());

    dom->setResultHoldingTime(r_hold.mean());
    dom->setResultHoldingTimeVariance(r_hold.variance());
    dom->setResultVarianceHoldingTime(r_hold_sqr.mean());
    dom->setResultVarianceHoldingTimeVariance(r_hold_sqr.variance());
    dom->setResultHoldingUtilization(r_hold_util.mean());
    dom->setResultHoldingUtilizationVariance(r_hold_util.variance());
}



FILE *
Semaphore_Task::print( FILE * output ) const
{
    Task::print( output );
    r_hold.print_raw( output,      "%-6.6s %-11.11s - Hold Time  ", type_name(), name() );
    r_hold_sqr.print_raw( output,  "%-6.6s %-11.11s - Hold Sqr   ", type_name(), name() );
    r_hold_util.print_raw( output, "%-6.6s %-11.11s - Hold Util  ", type_name(), name() );

    return output;
}

/* ------------------------------------------------------------------------ */

ReadWriteLock_Task::ReadWriteLock_Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup )
    : Semaphore_Task( type, domTask, aProc, aGroup ),
      _reader(0),
      _writer(0),
      _signal_port2(-1),
      _writerQ_port(-1),
      _readerQ_port(-1)
{
}

void
ReadWriteLock_Task::create_instance()
{
    if ( n_entries() != N_RWLOCK_ENTRIES ) {
	LQIO::solution_error( LQIO::ERR_ENTRY_COUNT_FOR_TASK, name(), n_entries(), N_RWLOCK_ENTRIES );
    }
	
    int E[N_RWLOCK_ENTRIES];
    for (int i=0;i<N_RWLOCK_ENTRIES;i++){ E[i]=-1; }
	
    for (int i=0;i<N_RWLOCK_ENTRIES;i++){
	if ( _entry[i]->is_r_unlock() ) {
	    if (E[0]== -1) { E[0]=i; }
	    else{ // duplicate entry TYPE error
		LQIO::solution_error( LQIO::ERR_DUPLICATE_SYMBOL, name() );
	    }
	} else if ( _entry[i]->is_r_lock() ) {
	    if (E[1]== -1) { E[1]=i; }
	    else{ 
		LQIO::solution_error( LQIO::ERR_DUPLICATE_SYMBOL, name() );
	    }
	} else if ( _entry[i]->is_w_unlock() ) {
	    if (E[2]== -1) { E[2]=i; }
	    else{ 
		LQIO::solution_error( LQIO::ERR_DUPLICATE_SYMBOL, name() );
	    }
	} else if ( _entry[i]->is_w_lock() ) {
	    if (E[3]== -1) { E[3]=i; }
	    else{ 
		LQIO::solution_error( LQIO::ERR_DUPLICATE_SYMBOL, name() );
	    }
	} else { 
	    LQIO::solution_error( LQIO::ERR_NO_RWLOCK, name() );
	    cerr << "entry names: " << _entry[0]->name() << ", " << _entry[1]->name() <<", " << _entry[2]->name()<< ", " << _entry[3]->name()<< endl;
	}
    }
    //test reader lock entry
    if ( !_entry[E[1]]->test_and_set_rwlock( RWLOCK_R_LOCK ) ) {
	LQIO::solution_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    }
    if ( !_entry[E[1]]->test_and_set_recv( Entry::RECEIVE_RENDEZVOUS ) ) {
	LQIO::solution_error( LQIO::ERR_ASYNC_REQUEST_TO_WAIT, _entry[E[1]]->name() );
    }
	 
    //test reader unlock entry
    if ( !_entry[E[0]]->test_and_set_rwlock( RWLOCK_R_UNLOCK ) ) {
	LQIO::solution_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    }

    //test writer lock entry
    if ( !_entry[E[3]]->test_and_set_rwlock( RWLOCK_W_LOCK ) ) {
	LQIO::solution_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    }
    if ( !_entry[E[3]]->test_and_set_recv( Entry::RECEIVE_RENDEZVOUS ) ) {
	LQIO::solution_error( LQIO::ERR_ASYNC_REQUEST_TO_WAIT, _entry[E[3]]->name() );
    }

    //test writer unlock entry
    if ( !_entry[E[2]]->test_and_set_rwlock( RWLOCK_W_UNLOCK ) ) {
	LQIO::solution_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    }

 

    string buf = name();
    buf += "-rwlock-server";
    /*  waiting for request - send to reader_queue or writer_token. */
    /*  srn_rwlock_server should not be blocked, even if number of concurrent */
    /*	readers is greater than the maximum number of the reader lock.*/
    _task = new srn_rwlock_server( this, buf.c_str() );
    _readerQ_port = ps_allocate_port( buf.c_str(), _task->task_id() );
    _writerQ_port = ps_allocate_port( buf.c_str(), _task->task_id() );
    _signal_port2 = ps_allocate_port( buf.c_str(), _task->task_id() );

    buf = name();
    buf += "-reader-queue";
    /* entry for waiting lock reader request - send to token. */
    _reader = new srn_semaphore( this, buf.c_str() );
    _worker_port = ps_allocate_port( buf.c_str(), _reader->task_id() );

    /* Entry for signal request */
    buf = name();
    buf += "-reader-signal";
    _signal_task = new srn_signal( this,  buf.c_str() );
    _signal_port = ps_allocate_port( buf.c_str(), _signal_task->task_id() );

    /*  writer token*/
    buf = name();
    buf += "-writer-token";
    _writer = new srn_writer_token( this, buf.c_str() );
}


void
ReadWriteLock_Task::create()
{
    Semaphore_Task::create();

    r_reader_hold.init( SAMPLE,         "%s %-11.11s - Reader Hold Time         ", type_name(), name() );
    r_reader_hold_sqr.init( SAMPLE,     "%s %-11.11s - Reader Hold Time sq      ", type_name(), name() );
    r_reader_wait.init( SAMPLE,         "%s %-11.11s - Reader Blocked Time      ", type_name(), name() );
    r_reader_wait_sqr.init( SAMPLE,     "%s %-11.11s - Reader Blocked Time sq   ", type_name(), name() );
    r_reader_hold_util.init( VARIABLE,  "%s %-11.11s - Reader Hold Utilization  ", type_name(), name() );
    r_writer_hold.init( SAMPLE,         "%s %-11.11s - Writer Hold Time         ", type_name(), name() );
    r_writer_hold_sqr.init( SAMPLE,     "%s %-11.11s - Writer Hold Time sq      ", type_name(), name() );
    r_writer_wait.init( SAMPLE,         "%s %-11.11s - Writer Blocked Time      ", type_name(), name() );
    r_writer_wait_sqr.init( SAMPLE,     "%s %-11.11s - Writer Blocked Time sq   ", type_name(), name() );
    r_writer_hold_util.init( VARIABLE,  "%s %-11.11s - Writer Hold Utilization  ", type_name(), name() );
}

bool
ReadWriteLock_Task::start()
{
    return Semaphore_Task::start()		/* Starts _signal_task */
	&& ps_resume( _reader->task_id() ) 
	&& ps_resume( _writer->task_id() );
}


void
ReadWriteLock_Task::kill()
{
    ps_suspend( _reader->task_id() );
    ps_suspend( _writer->task_id() );
    ps_suspend( _signal_task->task_id() );

    ps_kill( _reader->task_id() );
    ps_kill( _writer->task_id() );
    ps_kill( _signal_task->task_id() );

    Semaphore_Task::kill();
}


void
ReadWriteLock_Task::reset_stats()
{
    Semaphore_Task::reset_stats();

    r_reader_hold.reset();
    r_reader_hold_sqr.reset();
    r_reader_hold_util.reset();
    r_reader_wait.reset();
    r_reader_wait_sqr.reset();	
    r_writer_hold.reset();
    r_writer_hold_sqr.reset();
    r_writer_wait.reset();
    r_writer_wait_sqr.reset();
    r_writer_hold_util.reset();
}


void
ReadWriteLock_Task::accumulate_data()
{
    Semaphore_Task::accumulate_data();

    r_reader_hold_sqr.accumulate_variance( r_reader_hold.accumulate() );
    r_writer_hold_sqr.accumulate_variance( r_writer_hold.accumulate() );
    r_reader_wait_sqr.accumulate_variance( r_reader_wait.accumulate() );
    r_writer_wait_sqr.accumulate_variance( r_writer_wait.accumulate() );
    r_reader_hold_util.accumulate();
    r_writer_hold_util.accumulate();
}


void
ReadWriteLock_Task::insertDOMResults()
{
    Task::insertDOMResults();

    LQIO::DOM::RWLockTask * dom = dynamic_cast<LQIO::DOM::RWLockTask *>(getDOM());

    dom->setResultReaderHoldingTime( r_reader_hold.mean() ) ;
    dom->setResultWriterHoldingTime( r_writer_hold.mean() ) ;
    dom->setResultReaderHoldingTimeVariance( r_reader_hold.variance() ) ;
    dom->setResultWriterHoldingTimeVariance( r_writer_hold.variance() ) ;
    dom->setResultVarianceReaderHoldingTime( r_reader_hold_sqr.mean() ) ;
    dom->setResultVarianceWriterHoldingTime( r_writer_hold_sqr.mean() ) ;
    dom->setResultVarianceReaderHoldingTimeVariance( r_reader_hold_sqr.variance() ) ;
    dom->setResultVarianceWriterHoldingTimeVariance( r_writer_hold_sqr.variance() ) ;
    dom->setResultReaderBlockedTime( r_reader_wait.mean() ) ;
    dom->setResultWriterBlockedTime( r_writer_wait.mean() ) ;
    dom->setResultReaderBlockedTimeVariance( r_reader_wait.variance() ) ;
    dom->setResultWriterBlockedTimeVariance( r_writer_wait.variance() ) ;
    dom->setResultVarianceReaderBlockedTime( r_reader_wait_sqr.mean() ) ;
    dom->setResultVarianceWriterBlockedTime( r_writer_wait_sqr.mean() ) ;
    dom->setResultVarianceReaderBlockedTimeVariance( r_reader_wait_sqr.variance() ) ;
    dom->setResultVarianceWriterBlockedTimeVariance( r_writer_wait_sqr.variance() ) ;

    dom->setResultReaderHoldingUtilization( r_reader_hold_util.mean() ) ;
    dom->setResultReaderHoldingUtilizationVariance( r_reader_hold_util.variance()) ;
    dom->setResultWriterHoldingUtilization( r_writer_hold_util.mean() ) ;
    dom->setResultWriterHoldingUtilizationVariance( r_writer_hold_util.variance()) ;
}


FILE *
ReadWriteLock_Task::print( FILE * output ) const
{
    Semaphore_Task::print( output );

    r_reader_hold.print_raw( output,      "%-6.6s %-11.11s - Reader Hold Time    ", type_name(), name() );
    r_reader_hold_sqr.print_raw( output,  "%-6.6s %-11.11s - Reader Hold Sqr     ", type_name(), name() );
    r_reader_wait.print_raw( output,      "%-6.6s %-11.11s - Reader Blocked Time ", type_name(), name() );
    r_reader_wait_sqr.print_raw( output,  "%-6.6s %-11.11s - Reader Blocked Sqr  ", type_name(), name() );
    r_reader_hold_util.print_raw( output, "%-6.6s %-11.11s - Reader Hold Util    ", type_name(), name() );

    r_writer_hold.print_raw( output,      "%-6.6s %-11.11s - Writer Hold Time    ", type_name(), name() );
    r_writer_hold_sqr.print_raw( output,  "%-6.6s %-11.11s - Writer Hold Sqr     ", type_name(), name() );
    r_writer_wait.print_raw( output,      "%-6.6s %-11.11s - Writer Blocked Time ", type_name(), name() );
    r_writer_wait_sqr.print_raw( output,  "%-6.6s %-11.11s - Writer Blocked Sqr  ", type_name(), name() );
    r_writer_hold_util.print_raw( output, "%-6.6s %-11.11s - Writer Hold Util    ", type_name(), name() );

    return output;
}

/* ------------------------------------------------------------------------ */
void
Pseudo_Task::insertDOMResults()
{
    if ( type() != Task::OPEN_ARRIVAL_SOURCE ) return;

    /* Waiting times for open arrivals */

    for ( vector<Entry *>::const_iterator nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	for ( vector<tar_t>::iterator tp = (*nextEntry)->phase[1].tinfo.target.begin(); tp != (*nextEntry)->phase[1].tinfo.target.end(); ++tp ) {
	    Entry * ep = tp->entry;
	    LQIO::DOM::Entry * entry_dom = ep->get_DOM();
	    entry_dom->setResultOpenWaitTime((*tp).mean_delay());
	    if ( number_blocks > 1 ) {
		entry_dom->setResultOpenWaitTimeVariance((*tp).variance_delay());
	    }
	}
    }
}


void
Pseudo_Task::create_instance()
{
    if ( type() != Task::OPEN_ARRIVAL_SOURCE ) return;

    _task = new srn_open_arrivals( this, name() );	/* Create a fake task.			*/
}


bool
Pseudo_Task::start()
{
    return ps_resume( _task->task_id() ) == OK;
}


void
Pseudo_Task::kill()
{
    ps_kill( _task->task_id() );
    _task = 0;
}
