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
 * $Id: task.cc 15456 2022-03-09 15:06:35Z greg $
 */

#include "lqsim.h"
#include <iostream>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <lqio/labels.h>
#include <lqio/error.h>
#include <lqio/dom_actlist.h>
#include "errmsg.h"
#include "task.h"
#include "instance.h"
#include "activity.h"
#include "processor.h"
#include "group.h"

#define N_SEMAPHORE_ENTRIES 2
#define N_RWLOCK_ENTRIES 4

std::set<Task *, Task::ltTask> Task::__tasks;	/* Task table.	*/

const std::map<const Task::Type,const std::string> Task::type_strings =  {
    { Task::Type::UNDEFINED,              "Undefined" },
    { Task::Type::CLIENT,                 "client" },
    { Task::Type::SERVER,                 "server" },
    { Task::Type::MULTI_SERVER,           "multi" },
    { Task::Type::INFINITE_SERVER,        "infsrv" },
    { Task::Type::SYNCHRONIZATION_SERVER, "sync" },
    { Task::Type::SEMAPHORE,              "semph" },
    { Task::Type::OPEN_ARRIVAL_SOURCE,    "open" },
    { Task::Type::WORKER,                 "worker" },
    { Task::Type::THREAD,                 "thread" },
    { Task::Type::TOKEN,                  "token" },
    { Task::Type::TOKEN_R,                "token_r" },
    { Task::Type::SIGNAL,                 "signal" },
    { Task::Type::RWLOCK,                 "rw_lock" },
    { Task::Type::RWLOCK_SERVER,          "rwlock" },
    { Task::Type::WRITER_TOKEN, 	  "token" }
};


/*
 * n_tasks represents the number of task classes defined in the input
 * file.  total_tasks is the total number of tasks created.  The latter
 * will always be greater than or equal to the former because it includes
 * all "clones" of multi-server tasks.  Use n_tasks as an upper bound to
 * class_tab, and total_tasks as an upper bound to object_tab.
 */

unsigned total_tasks = 0;

Task::Task( const Task::Type type, LQIO::DOM::Task* dom, Processor * processor, Group * a_group )
    : _dom(dom),
      _processor(processor),
      _group_id(-1),
      _compute_func(nullptr),
      _active(0),
      _max_phases(1),
      _act_list(),
      _forks(),
      _joins(),
      _pending_msgs(),
      _join_start_time(0.0),
      _free_msgs(),
      _type(type),
      _entry(),
      _activity(),
      trace_flag(false),
      _hist_data(nullptr),
      r_cycle(),
      r_util(),
      r_group_util(),
      r_loss_prob(),
      _hold_active(0)
{
    if ( processor ) {
	processor->add_task(this);
    }
}


Task::~Task()
{
    std::for_each( _activity.begin(), _activity.end(), Delete<Activity *> );
    std::for_each( _act_list.begin(), _act_list.end(), Delete<ActivityList *> );
    std::for_each( _free_msgs.begin(), _free_msgs.end(), Delete<Message *> );

    if ( _hist_data ) {
	delete _hist_data;
    }
}


/*
 * Configure entries and activities.  Count up the number of
 * activities.  Do final list construction (i.e., tie up loose ends.)
 */

Task&
Task::configure()
{
    /* I need the instance variable "task" set from this point on. */

    double total_calls = for_each( _activity.begin(), _activity.end(), ExecSum<Activity,double>( &Activity::configure ) ).sum();
    for_each( _act_list.begin(), _act_list.end(), Exec<ActivityList>( &ActivityList::configure ) );
    total_calls += for_each( _entry.begin(), _entry.end(), ExecSum<Entry,double>( &Entry::configure ) ).sum();

    if ( total_calls == 0 && is_reference_task() ) {
	LQIO::solution_error( LQIO::WRN_NO_SENDS_FROM_REF_TASK, name() );
    }

    for ( std::vector<Activity *>::const_iterator ap = _activity.begin(); ap != _activity.end(); ++ap ) {
	if ( !(*ap)->is_reachable() ) {
	    LQIO::solution_error( LQIO::WRN_NOT_USED, "Activity", (*ap)->name() );
	} else if ( !(*ap)->is_specified() ) {
	    LQIO::solution_error( LQIO::ERR_ACTIVITY_NOT_SPECIFIED, name(), (*ap)->name() );
	}
    }
    return *this;
}


/*
 * Construct the parasol entity.
 */

Task&
Task::create()
{
    /*
     * Set compute function.  We have a special case for when no
     * processor has been allocated (eg, processor 0).  In this case,
     * tasks compute by "sleeping".
     */

    _compute_func = (!processor() || processor()->is_infinite()) ? ps_sleep : ps_compute;

    /* JOIN Stuff -- All entries are free. */

    trace_flag	= std::regex_match( name(), task_match_pattern );

    if ( debug_flag ){
	(void) fprintf( stddbg, "\n-+++++---- %s task %s", type_name().c_str(), name() );
	if ( _compute_func == ps_sleep ) {
	    (void) fprintf( stddbg, " [delay]" );
	}
	(void) fprintf( stddbg, " ----+++++-\n" );
    }

    /* Compute PDF for all activities for each task. */

    create_instance();
    initialize();

    /* Create "links" where necessary. */

    build_links();

    if ( has_send_no_reply() ) {
	alloc_pool();
    }

    return *this;
}


Task&
Task::initialize()
{
    for_each( _activity.begin(), _activity.end(), Exec<Activity>( &Activity::initialize ) );
    for_each( _entry.begin(), _entry.end(), Exec<Entry>( &Entry::initialize ) );
    for_each( _forks.begin(), _forks.end(), Exec<AndForkActivityList>( &AndForkActivityList::initialize ) );

    /*
     * Allocate statistics for joins.  We do it here because we
     * don't know how many joins we have when we allocate the task
     * class object.
     */

    for ( std::vector<AndJoinActivityList *>::iterator lp = _joins.begin(); lp != _joins.end(); ++lp ) {
	const Activity * src = (*lp)->front();
	const Activity * dst = (*lp)->back();

	(*lp)->r_join.init( SAMPLE, "Join delay %-11.11s %-11.11s ", src->name(), dst->name() );
	(*lp)->r_join_sqr.init( SAMPLE, "Join delay squared %-11.11s %-11.11s ", src->name(), dst->name() );
    }

    /* statistics */
    _active = 0;		/* Reset counts */
    _hold_active = 0;

    r_cycle.init( SAMPLE,        "%s %-11.11s - Cycle Time        ", type_name().c_str(), name() );
    r_util.init( VARIABLE,       "%s %-11.11s - Utilization       ", type_name().c_str(), name() );
    r_group_util.init( VARIABLE, "%s %-11.11s - Group Utilization ", type_name().c_str(), name() );
    return *this;
}

int
Task::node_id() const
{
    return processor() ? processor()->node_id() : 0;
}

void
Task::set_start_activity( LQIO::DOM::Entry* dom )
{
    const char * entry_name = dom->getName().c_str();
    Entry * ep = Entry::find( entry_name );

    if ( !ep ) return;
    if ( !ep->test_and_set( LQIO::DOM::Entry::Type::ACTIVITY ) ) return;

    const LQIO::DOM::Activity * activity_dom = dom->getStartActivity();
    const char * activity_name = activity_dom->getName().c_str();
    Activity * ap = find_activity( activity_name );
    if ( !ap ) {
	LQIO::input_error2( LQIO::ERR_NOT_DEFINED, activity_name );
    } else if ( ep->get_start_activity() != nullptr ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_START_ACTIVITY, entry_name, activity_name );
    } else {
	ep->set_start_activity( ap );
	ap->set_is_start_activity(true);
    }
}

/*
 * add links between tasks to simulate communication delays.
 */

void
Task::build_links()
{
    for ( unsigned j = 0; j < n_entries(); ++j ) {
	for ( std::vector<Activity>::iterator phase = _entry[j]->_phase.begin(); phase != _entry[j]->_phase.end(); ++phase ) {
	    for ( Targets::const_iterator tp = phase->_calls.begin(); tp != phase->_calls.end(); ++tp ) {
		Processor * proc = tp->entry()->task()->processor();
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
		    tar_t& target = const_cast<tar_t&>(*tp);
		    target.set_link( link_tab[h] );
		}
	    }
	}
    }
}



bool
Task::has_send_no_reply() const
{
    return std::any_of( _entry.begin(), _entry.end(), Predicate<Entry>( &Entry::is_send_no_reply ) );
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
    std::vector<Activity *>::const_iterator ap = find_if( _activity.begin(), _activity.end(), eqActivityStr( activity_name ) );
    if ( ap != _activity.end() ) {
	return *ap;
    } else {
	return nullptr;
    }
}


/*
 * allocate message pool for asynchronous messages.
 */

void
Task::alloc_pool()
{
    unsigned size = DEFAULT_QUEUE_SIZE;
    if ( getDOM()->hasQueueLength() ) {
	try {
	    size = getDOM()->getQueueLengthValue();
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "pool size", "task", name(), e.what() );
	    throw_bad_parameter();
	}
    }
    for ( unsigned int i = 0; i < size; ++i ) {
	_free_msgs.push_back( new Message );
    }
}


Message *
Task::alloc_message()
{
    Message * msg = nullptr;
    if ( _free_msgs.size() > 0 ) {
	msg = _free_msgs.front();
	_free_msgs.pop_front();
    }
    return msg;
}

void
Task::free_message( Message * msg )
{
    /* Async message -- acummulate queuing + service (M/G/m model) */

    double delta = ps_now - msg->time_stamp;
    tar_t *tp = msg->target;
    ps_record_stat( tp->r_delay.raw, delta );
    ps_record_stat( tp->r_delay_sqr.raw, square( delta ) );
    _free_msgs.push_back( msg );
}

double
Task::throughput() const
{
    switch ( type() ) {
    case Task::Type::SEMAPHORE: return r_cycle.mean_count() / (Model::block_period() * n_entries()); 	/* Only count for one entry.  */
    case Task::Type::RWLOCK:	return r_cycle.mean_count() / (Model::block_period() * n_entries()/2); 	/* Only count for two entries.  */
    case Task::Type::SERVER:	if ( is_sync_server() ) return  r_cycle.mean_count() / (Model::block_period() * n_entries());
	/* Fall through */
    default: 		   	return r_cycle.mean_count() / Model::block_period();
    }
}



double
Task::throughput_variance() const
{
    switch ( type() ) {
    case Task::Type::SEMAPHORE: return r_cycle.variance_count() / (square(Model::block_period()) * n_entries());
    case Task::Type::RWLOCK:    return r_cycle.variance_count() / (square(Model::block_period()) * n_entries()/2);
    case Task::Type::SERVER:	if ( is_sync_server() ) return r_cycle.variance_count() / (square(Model::block_period()) * n_entries());
	/* Fall through */
    default:		   	return r_cycle.variance_count() / square(Model::block_period());
    }
}

Task&
Task::reset_stats()
{
    r_util.reset();
    r_cycle.reset();

    std::for_each( _entry.begin(), _entry.end(), Exec<Entry>( &Entry::reset_stats ) );
    std::for_each( _activity.begin(), _activity.end(), Exec<Activity>( &Activity::reset_stats ) );
    std::for_each( _joins.begin(), _joins.end(), Exec<AndJoinActivityList>( &AndJoinActivityList::reset_stats ) );

    /* Histogram stuff */

    if ( _hist_data ) {
	_hist_data->reset();
    }
    return *this;
}


Task&
Task::accumulate_data()
{
    if ( type() == Task::Type::UNDEFINED ) return *this;    /* Some tasks don't have statistics */

    r_util.accumulate();
    r_cycle.accumulate();

    std::for_each( _entry.begin(), _entry.end(), Exec<Entry>( &Entry::accumulate_data ) );
    std::for_each( _activity.begin(), _activity.end(), Exec<Activity>( &Activity::accumulate_data ) );
    std::for_each( _joins.begin(), _joins.end(), Exec<AndJoinActivityList>( &AndJoinActivityList::accumulate_data ) );

    /* Histogram stuff */

    if ( _hist_data ) {
	_hist_data->accumulate_data();
    }
    return *this;
}


FILE *
Task::print( FILE * output ) const
{
    r_util.print_raw( output,     "%-6.6s %-11.11s - Utilization", type_name().c_str(), name() );
    r_cycle.print_raw( output,    "%-6.6s %-11.11s - Cycle Time ", type_name().c_str(), name() );

    for ( std::vector<Entry *>::const_iterator entry = _entry.begin(); entry != _entry.end(); ++entry ) {
	(*entry)->r_cycle.print_raw( output, "Entry %-11.11s  - Cycle Time      ", (*entry)->name() );

	for_each( (*entry)->_phase.begin(), (*entry)->_phase.end(), ConstExec1<Activity,FILE *>( &Activity::print_raw_stat, output ) );
    }

    for_each( _activity.begin(), _activity.end(), ConstExec1<Activity,FILE *>( &Activity::print_raw_stat, output ) );

    for ( std::vector<AndJoinActivityList *>::const_iterator lp = _joins.begin(); lp != _joins.end(); ++lp ) {
	(*lp)->r_join.print_raw( output, "%-6.6s %-11.11s - Join Delay ", type_name().c_str(), name() );
	(*lp)->r_join_sqr.print_raw( output, "%-6.6s %-11.11s - Join DelSqr", type_name().c_str(), name() );
    }

    return output;
}

Task&
Task::insertDOMResults()
{
    /* Some tasks don't have statistics */

    if ( type() == Task::Type::UNDEFINED ) return *this;

    double phaseUtil[MAX_PHASES];
    double phaseVar[MAX_PHASES];
    for ( unsigned p = 0; p < MAX_PHASES; p++ ) {
	phaseUtil[p] = 0.0;
	phaseVar[p] = 0.0;
    }

    if ( has_activities() ) {
	for_each( _activity.begin(), _activity.end(), Exec<Activity>( &Activity::insertDOMResults ) );
	for_each( _joins.begin(), _joins.end(), Exec<AndJoinActivityList>( &AndJoinActivityList::insertDOMResults ) );
    }

    double taskProcUtil = 0.0;		/* Total processor utilization. */
    double taskProcVar = 0.0;
    for ( std::vector<Entry *>::const_iterator nextEntry = _entry.begin(); nextEntry != _entry.end(); ++nextEntry ) {
	Entry * ep = *nextEntry;
	ep->insertDOMResults();

	for ( unsigned p = 0; p < max_phases(); ++p ) {
	    phaseUtil[p] += ep->_phase[p].r_util.mean();
	    phaseVar[p]  += ep->_phase[p].r_util.variance();
	    taskProcUtil += ep->_phase[p].r_cpu_util.mean();
	    taskProcVar  += ep->_phase[p].r_cpu_util.variance();
	}
    }

    /* Store totals */

    getDOM()->setResultPhaseUtilizations(max_phases(),phaseUtil)
	.setResultUtilization(r_util.mean())
	.setResultThroughput(throughput())
	.setResultProcessorUtilization(taskProcUtil);

    if ( number_blocks > 1 ) {
	getDOM()->setResultPhaseUtilizationVariances(max_phases(),phaseVar)
	    .setResultUtilizationVariance(r_util.variance())
	    .setResultThroughputVariance(throughput_variance())
	    .setResultProcessorUtilizationVariance(taskProcVar);
    }

    if ( _hist_data ) {
	_hist_data->insertDOMResults();
    }
    return *this;
}

/*----------------------------------------------------------------------*/
/*	 Input processing.  Called from configure() 			*/
/*----------------------------------------------------------------------*/

/*
 * Add a task to the model.
 */

Task *
Task::add( LQIO::DOM::Task* domTask )
{
    /* Recover the old parameter information that used to be passed in */
    const char* task_name = domTask->getName().c_str();
    const LQIO::DOM::Group * domGroup = domTask->getGroup();
    const scheduling_type sched_type = domTask->getSchedulingType();

    if ( !task_name || strlen( task_name ) == 0 ) abort();

    if ( Task::find( task_name ) ) {
	LQIO::input_error2( LQIO::ERR_DUPLICATE_SYMBOL, "Task", task_name );
    }
    if ( domTask->hasReplicas() ) {
	LQIO::input_error2( ERR_REPLICATION, "task", task_name );
    }

    Task * cp = 0;
    const char* processor_name = domTask->getProcessor()->getName().c_str();
    Processor * processor = Processor::find( processor_name );

    if ( !LQIO::DOM::Common_IO::is_default_value( domTask->getPriority(), 0 ) && ( processor->discipline() == SCHEDULE_FIFO
										   || processor->discipline() == SCHEDULE_PS
										   || processor->discipline() == SCHEDULE_RAND ) ) {
	LQIO::input_error2( LQIO::WRN_PRIO_TASK_ON_FIFO_PROC, task_name, processor_name );
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
	if ( domTask->isInfinite() ) {
	    input_error2( LQIO::ERR_REFERENCE_TASK_IS_INFINITE, task_name );
	}
	cp = new Reference_Task( Task::Type::CLIENT, domTask, processor, group );
	break;

    case SCHEDULE_PPR:
    case SCHEDULE_HOL:
    case SCHEDULE_FIFO:
	if ( domTask->hasThinkTime() ) {
	    input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name );
	}
	Task::Type a_type;

	if ( domTask->isInfinite() ) {
	    a_type = Task::Type::INFINITE_SERVER;
	} else if ( domTask->isMultiserver() ) {
	    a_type = Task::Type::MULTI_SERVER;
	} else {
	    a_type = Task::Type::SERVER;
	}
	cp = new Server_Task( a_type, domTask, processor, group );
	break;

    case SCHEDULE_DELAY:
	if ( domTask->hasThinkTime() ) {
	    input_error2( LQIO::ERR_NON_REF_THINK_TIME, task_name );
	}
	if ( domTask->isMultiserver() ) {
	    LQIO::input_error2( LQIO::WRN_INFINITE_MULTI_SERVER, "Task", task_name, domTask->getCopiesValue() );
	}
	if ( domTask->hasQueueLength() ) {
	    LQIO::input_error2( LQIO::WRN_QUEUE_LENGTH, task_name );
	}
	cp = new Server_Task( Task::Type::INFINITE_SERVER, domTask, processor, group );
	break;

/*+ BUG_164 */
    case SCHEDULE_SEMAPHORE:
	if ( domTask->hasQueueLength()  ) {
	    input_error2( LQIO::WRN_QUEUE_LENGTH, task_name );
	}
	if ( domTask->isInfinite() ) {
	    input_error2( LQIO::ERR_INFINITE_TASK, task_name );
	}
 	cp = new Semaphore_Task( Task::Type::SEMAPHORE, domTask, processor, group );
	break;
/*- BUG_164 */

/* reader_writer lock */

    case SCHEDULE_RWLOCK:
	if ( domTask->hasQueueLength() ) {
	    input_error2( LQIO::WRN_QUEUE_LENGTH, task_name );
	}
	if ( domTask->isInfinite() ) {
	    input_error2( LQIO::ERR_INFINITE_TASK, task_name );
	}
 	cp = new ReadWriteLock_Task( Task::Type::RWLOCK, domTask, processor, group );
	break;
/* reader_writer lock*/

    default:
	cp = new Server_Task( Task::Type::SERVER, domTask, processor, group );		/* Punt... */
	input_error2( LQIO::WRN_SCHEDULING_NOT_SUPPORTED, scheduling_label[sched_type].str, "task", task_name );
	break;
    }

    Task::__tasks.insert( cp );

    return cp;
}


/*
 * Located the task id.
 */

Task *
Task::find( const char * task_name )
{
    std::set<Task *>::const_iterator nextTask = find_if( Task::__tasks.begin(), Task::__tasks.end(), eqTaskStr( task_name ) );
    if ( nextTask == Task::__tasks.end() ) {
	return nullptr;
    } else {
	return *nextTask;
    }
}


/*
 * We need a way to fake out infinity... so if copies is infinite,
 * then we change to one.  Reference tasks can never be infinite.
 */

unsigned
Task::multiplicity() const
{
    unsigned int value = 1;
    if ( !getDOM()->isInfinite() || is_reference_task() ) {
	try {
	    value = getDOM()->getCopiesValue();
	}
	catch ( const std::domain_error& e ) {
	    solution_error( LQIO::ERR_INVALID_PARAMETER, "multiplicity", "task", name(), e.what() );
	    throw_bad_parameter();
	}
    }
    return value;
}


int
Task::priority() const
{
    try {
	return getDOM()->getPriorityValue();
    }
    catch ( const std::domain_error &e ) {
	LQIO::solution_error( LQIO::ERR_INVALID_PARAMETER, "priority", "task", name(), e.what() );
	throw_bad_parameter();
    }
    return 0;
}

bool
Task::is_infinite() const
{
    return getDOM()->isInfinite();
}


/*
 * Return true if any entry or activity has a think time value.  If
 * so, Model::extend() will then create an entry to a thinker device.
 */

bool
Task::has_think_time() const
{
    return std::any_of( _entry.begin(), _entry.end(), Predicate<Entry>( &Entry::has_think_time ) )
	|| std::any_of( _activity.begin(), _activity.end(), Predicate<Activity>( &Activity::has_think_time ) );
}



bool
Task::has_lost_messages() const
{
    return std::any_of( _entry.begin(), _entry.end(), Predicate<Entry>( &Entry::has_lost_messages ) );
}



bool
Task::derive_utilization() const
{
    return processor()->derive_utilization();
}

/* ------------------------------------------------------------------------ */

Reference_Task::Reference_Task( const Task::Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup )
    : Task( type, domTask, aProc, aGroup ), _think_time(0.0), _task_list()
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
    for ( std::vector<srn_client *>::const_iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	if ( ps_resume( (*t)->task_id() ) != OK ) return false;
    }
    return true;
}


Reference_Task&
Reference_Task::kill()
{
    for ( std::vector<srn_client *>::const_iterator t = _task_list.begin(); t != _task_list.end(); ++t ) {
	ps_kill( (*t)->task_id() );
    }
    return *this;
}

/* ------------------------------------------------------------------------ */

Server_Task::Server_Task( const Task::Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup )
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
	_type = Task::Type::INFINITE_SERVER;
    } else if ( is_multiserver() ) {
	_task = new srn_multiserver( this, name(), multiplicity() );
	_worker_port = ps_allocate_port( name(), _task->task_id() );
	_type = Task::Type::MULTI_SERVER;
    } else {
	_task = new srn_server( this, name() );
	_worker_port = -1;
	_type = Task::Type::SERVER;
    }
}


bool
Server_Task::start()
{
    return ps_resume( _task->task_id() ) == OK;
}


Server_Task&
Server_Task::kill()
{
    ps_suspend( _task->task_id() );
    ps_kill( _task->task_id() );
    _task = 0;
    _worker_port = -1;
    return *this;
}

/* ------------------------------------------------------------------------ */

Semaphore_Task::Semaphore_Task( const Task::Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup )
    : Server_Task( type, domTask, aProc, aGroup ),
      r_hold(),
      r_hold_sqr(),
      r_hold_util(),
      _signal_task(0),
      _signal_port(-1)
{
}


Semaphore_Task&
Semaphore_Task::create()
{
    Task::create();

    r_hold.init( SAMPLE,         "%s %-11.11s - Hold Time         ", type_name().c_str(), name() );
    r_hold_sqr.init( SAMPLE,     "%s %-11.11s - Hold Time Sq      ", type_name().c_str(), name() );
    r_hold_util.init( VARIABLE,  "%s %-11.11s - Hold Utilization  ", type_name().c_str(), name() );
    return *this;
}

void
Semaphore_Task::create_instance()
{
    if ( n_entries() != N_SEMAPHORE_ENTRIES ) {
	LQIO::solution_error( LQIO::ERR_ENTRY_COUNT_FOR_TASK, name(), n_entries(), N_SEMAPHORE_ENTRIES );
    }
    if ( _entry[0]->is_signal() ) {
	if ( !_entry[1]->test_and_set_semaphore( LQIO::DOM::Entry::Semaphore::WAIT ) ) {
	    LQIO::solution_error( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
	}
	if ( !_entry[1]->test_and_set_recv( Entry::Type::RENDEZVOUS ) ) {
	    LQIO::solution_error( LQIO::ERR_ASYNC_REQUEST_TO_WAIT, _entry[1]->name() );
	}
    } else if ( _entry[0]->is_wait() ) {
	if ( !_entry[1]->test_and_set_semaphore( LQIO::DOM::Entry::Semaphore::SIGNAL ) ) {
	    LQIO::solution_error( LQIO::ERR_MIXED_SEMAPHORE_ENTRY_TYPES, name() );
	}
	if ( !_entry[0]->test_and_set_recv( Entry::Type::RENDEZVOUS ) ) {
	    LQIO::solution_error( LQIO::ERR_ASYNC_REQUEST_TO_WAIT, _entry[0]->name() );
	}
    } else {
	LQIO::solution_error( LQIO::ERR_NO_SEMAPHORE, name() );
	std::cerr << "entry names: " << _entry[0]->name() << ", " << _entry[1]->name() << std::endl;
    }
    if ( !_hist_data && getDOM()->hasHistogram() ) {
	_hist_data = new Histogram( getDOM()->getHistogram() );
    }

    std::string buf = name();
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

Semaphore_Task&
Semaphore_Task::kill()
{
    Server_Task::kill();
    ps_kill( _signal_task->task_id() );
    _signal_port  = -1;
    return *this;
}

Semaphore_Task&
Semaphore_Task::reset_stats()
{
    Task::reset_stats();

    r_hold.reset();
    r_hold_sqr.reset();
    r_hold_util.reset();
    return *this;
}

Semaphore_Task&
Semaphore_Task::accumulate_data()
{
    Task::accumulate_data();

    r_hold_sqr.accumulate_variance( r_hold.accumulate() );
    r_hold_util.accumulate();
    return *this;
}

Semaphore_Task&
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
    return *this;
}



FILE *
Semaphore_Task::print( FILE * output ) const
{
    Task::print( output );
    r_hold.print_raw( output,      "%-6.6s %-11.11s - Hold Time  ", type_name().c_str(), name() );
    r_hold_sqr.print_raw( output,  "%-6.6s %-11.11s - Hold Sqr   ", type_name().c_str(), name() );
    r_hold_util.print_raw( output, "%-6.6s %-11.11s - Hold Util  ", type_name().c_str(), name() );

    return output;
}

/* ------------------------------------------------------------------------ */

ReadWriteLock_Task::ReadWriteLock_Task( const Task::Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup )
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
	    std::cerr << "entry names: " << _entry[0]->name() << ", " << _entry[1]->name() <<", " << _entry[2]->name()<< ", " << _entry[3]->name() << std::endl;
	}
    }
    //test reader lock entry
    if ( !_entry[E[1]]->test_and_set_rwlock( LQIO::DOM::Entry::RWLock::READ_LOCK ) ) {
	LQIO::solution_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    }
    if ( !_entry[E[1]]->test_and_set_recv( Entry::Type::RENDEZVOUS ) ) {
	LQIO::solution_error( LQIO::ERR_ASYNC_REQUEST_TO_WAIT, _entry[E[1]]->name() );
    }

    //test reader unlock entry
    if ( !_entry[E[0]]->test_and_set_rwlock( LQIO::DOM::Entry::RWLock::READ_UNLOCK ) ) {
	LQIO::solution_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    }

    //test writer lock entry
    if ( !_entry[E[3]]->test_and_set_rwlock( LQIO::DOM::Entry::RWLock::WRITE_LOCK ) ) {
	LQIO::solution_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    }
    if ( !_entry[E[3]]->test_and_set_recv( Entry::Type::RENDEZVOUS ) ) {
	LQIO::solution_error( LQIO::ERR_ASYNC_REQUEST_TO_WAIT, _entry[E[3]]->name() );
    }

    //test writer unlock entry
    if ( !_entry[E[2]]->test_and_set_rwlock( LQIO::DOM::Entry::RWLock::WRITE_UNLOCK ) ) {
	LQIO::solution_error( LQIO::ERR_MIXED_RWLOCK_ENTRY_TYPES, name() );
    }



    std::string buf = name();
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



ReadWriteLock_Task&
ReadWriteLock_Task::create()
{
    Semaphore_Task::create();

    r_reader_hold.init( SAMPLE,         "%s %-11.11s - Reader Hold Time         ", type_name().c_str(), name() );
    r_reader_hold_sqr.init( SAMPLE,     "%s %-11.11s - Reader Hold Time sq      ", type_name().c_str(), name() );
    r_reader_wait.init( SAMPLE,         "%s %-11.11s - Reader Blocked Time      ", type_name().c_str(), name() );
    r_reader_wait_sqr.init( SAMPLE,     "%s %-11.11s - Reader Blocked Time sq   ", type_name().c_str(), name() );
    r_reader_hold_util.init( VARIABLE,  "%s %-11.11s - Reader Hold Utilization  ", type_name().c_str(), name() );
    r_writer_hold.init( SAMPLE,         "%s %-11.11s - Writer Hold Time         ", type_name().c_str(), name() );
    r_writer_hold_sqr.init( SAMPLE,     "%s %-11.11s - Writer Hold Time sq      ", type_name().c_str(), name() );
    r_writer_wait.init( SAMPLE,         "%s %-11.11s - Writer Blocked Time      ", type_name().c_str(), name() );
    r_writer_wait_sqr.init( SAMPLE,     "%s %-11.11s - Writer Blocked Time sq   ", type_name().c_str(), name() );
    r_writer_hold_util.init( VARIABLE,  "%s %-11.11s - Writer Hold Utilization  ", type_name().c_str(), name() );
    return *this;
}

bool
ReadWriteLock_Task::start()
{
    return Semaphore_Task::start()		/* Starts _signal_task */
	&& ps_resume( _reader->task_id() )
	&& ps_resume( _writer->task_id() );
}


ReadWriteLock_Task&
ReadWriteLock_Task::kill()
{
    ps_suspend( _reader->task_id() );
    ps_suspend( _writer->task_id() );
    ps_suspend( _signal_task->task_id() );

    ps_kill( _reader->task_id() );
    ps_kill( _writer->task_id() );
    ps_kill( _signal_task->task_id() );

    Semaphore_Task::kill();
    return *this;
}


ReadWriteLock_Task&
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
    return *this;
}


ReadWriteLock_Task&
ReadWriteLock_Task::accumulate_data()
{
    Semaphore_Task::accumulate_data();

    r_reader_hold_sqr.accumulate_variance( r_reader_hold.accumulate() );
    r_writer_hold_sqr.accumulate_variance( r_writer_hold.accumulate() );
    r_reader_wait_sqr.accumulate_variance( r_reader_wait.accumulate() );
    r_writer_wait_sqr.accumulate_variance( r_writer_wait.accumulate() );
    r_reader_hold_util.accumulate();
    r_writer_hold_util.accumulate();
    return *this;
}


ReadWriteLock_Task&
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
    return *this;
}


FILE *
ReadWriteLock_Task::print( FILE * output ) const
{
    Semaphore_Task::print( output );

    r_reader_hold.print_raw( output,      "%-6.6s %-11.11s - Reader Hold Time    ", type_name().c_str(), name() );
    r_reader_hold_sqr.print_raw( output,  "%-6.6s %-11.11s - Reader Hold Sqr     ", type_name().c_str(), name() );
    r_reader_wait.print_raw( output,      "%-6.6s %-11.11s - Reader Blocked Time ", type_name().c_str(), name() );
    r_reader_wait_sqr.print_raw( output,  "%-6.6s %-11.11s - Reader Blocked Sqr  ", type_name().c_str(), name() );
    r_reader_hold_util.print_raw( output, "%-6.6s %-11.11s - Reader Hold Util    ", type_name().c_str(), name() );

    r_writer_hold.print_raw( output,      "%-6.6s %-11.11s - Writer Hold Time    ", type_name().c_str(), name() );
    r_writer_hold_sqr.print_raw( output,  "%-6.6s %-11.11s - Writer Hold Sqr     ", type_name().c_str(), name() );
    r_writer_wait.print_raw( output,      "%-6.6s %-11.11s - Writer Blocked Time ", type_name().c_str(), name() );
    r_writer_wait_sqr.print_raw( output,  "%-6.6s %-11.11s - Writer Blocked Sqr  ", type_name().c_str(), name() );
    r_writer_hold_util.print_raw( output, "%-6.6s %-11.11s - Writer Hold Util    ", type_name().c_str(), name() );

    return output;
}

/* ------------------------------------------------------------------------ */

/*
 * Pseudo Tasks are used to source open arrivals.  The DOM's of the
 * pseudo entries of this pseudo task are the DOM's of the
 * corresponding entries with open arrivals.
 */

Pseudo_Task&
Pseudo_Task::insertDOMResults()
{
    if ( type() != Task::Type::OPEN_ARRIVAL_SOURCE ) return *this;

    /* Waiting times for open arrivals */

    for_each( _entry.begin(), _entry.end(), Exec<Entry>( &Entry::insertDOMResults ) );
    return *this;
}


void
Pseudo_Task::create_instance()
{
    if ( type() != Task::Type::OPEN_ARRIVAL_SOURCE ) return;

    _task = new srn_open_arrivals( this, name() );	/* Create a fake task.			*/
}


bool
Pseudo_Task::start()
{
    return ps_resume( _task->task_id() ) == OK;
}


Pseudo_Task&
Pseudo_Task::kill()
{
    ps_kill( _task->task_id() );
    _task = 0;
    return *this;
}
