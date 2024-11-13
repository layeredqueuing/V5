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
 * $Id: instance.cc 17466 2024-11-13 14:17:16Z greg $
 */

/*
 * If this define is SET, then service times will include the first schedule to the processor.
 */

#define COUNT_FIRST_SCHEDULE	1

#include "lqsim.h"
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <lqio/dom_document.h>
#include <lqio/input.h>
#include <lqio/error.h>
#include "activity.h"
#include "entry.h"
#include "errmsg.h"
#include "group.h"
#include "instance.h"
#include "histogram.h"
#include "message.h"
#include "pragma.h"
#include "processor.h"
#include "task.h"

/* see parainout.h */


volatile int client_init_count = 0;	/* Semaphore for -C...*/
std::vector<Instance *> object_tab(MAX_TASKS+1);	/* object table		*/

Instance::Instance( Task * cp, const std::string& task_name, long task_id )
    : r_e_execute(nullptr),
      r_a_execute(nullptr),
      _cp(cp),
      _hold_start_time(0.0),
      _entry(),
      _task_id(task_id),
#if !BUG_289
      _node_id(cp->node_id()),
      _std_port( ps_std_port(task_id) ),
      _reply_port( ps_allocate_port( task_name.c_str(), task_id ) ),	/* reply port id	*/
      _thread_port( ps_allocate_port( cp->name().c_str(), task_id ) ),
      _start_port( cp->has_threads() ? ps_allocate_shared_port( _cp->name().c_str() ) : -1 ),
#endif
      active_threads(0),	/* no threads -- initialize */
      idle_threads(0),
      _phase_start_time(0),
      _current_phase(0),
      lastQuorumEndTime(0),
      _join_done()
{
#if !BUG_289
    assert ( _std_port < MAX_PORTS );
#endif

    _entry.assign( cp->n_entries(), nullptr );
    _join_done.assign( cp->max_activities(), false );
    
    object_tab[task_id] = this;
    if ( static_cast<unsigned long>(task_id) > total_tasks ) {
	total_tasks = task_id;
    }
}

Instance::~Instance() 
{
#if !BUG_289
    if ( _start_port >= 0 ) {
	ps_release_shared_port( _start_port );		/* Buggy */
    }
#endif
}


#if !BUG_289
void
Instance::start( void * )
{
    try {
	object_tab[ps_myself]->run();
    } 
    catch ( const std::runtime_error& e ) {
	fprintf( stderr, "%s: task \"%s\": runtime error: %s\n", LQIO::io_vars.toolname(), object_tab[ps_myself]->name().c_str(), e.what() );
	LQIO::io_vars.error_count += 1;
	deferred_exception = true;
	ps_suspend( ps_myself );
    }
}
#endif


/*
 * Code for performing one server cycle.
 */

void
Instance::client_cycle( Random * distribution )
{
    double think_time = 0;
    if ( distribution != nullptr ) {
	think_time = (*distribution)();
#if !BUG_289
	ps_my_schedule_time = now() + think_time;
	ps_sleep( think_time );
#endif
    }

    /* Force a reschedule IFF we don't sleep */
    if ( _cp->n_entries() == 1 ) {
	server_cycle( _cp->entries()[0], 0, think_time == 0. );
    } else {
	server_cycle( _cp->entries()[static_cast<size_t>(_cp->n_entries()*Random::number())], 0, think_time == 0. );
    }
}



/*
 * Code for performing one server cycle.  Common to srn_client,
 * srn_server and srn_worker.
 *
 *  int entry,				Message received on entry	
 *  int reply_port,			Port to send replies.	
 *  message * msg			Info from client.
 */

void
Instance::server_cycle( Entry * ep, Message * msg, bool reschedule )
{
#if BUG_289
    double start_time = 0.0;
#else
    double start_time 	= ps_my_schedule_time;
#endif
    double delta;
    unsigned p;
    Message * end_msg;

    if ( _cp->_join_start_time == 0.0 ) {
	_cp->_active += 1;
    }
#if !BUG_289
    _cp->r_util.record_offset( _cp->_active, start_time );
#else
    _cp->r_util.record( _cp->_active );
#endif

    _entry[ep->index()] = msg;

    /*
     * Start normal processing.
     */

    if ( msg && msg->reply_port != -1 ) {

	/* Synchronous message -- accumulate queueing time only */

	tar_t * tp = msg->target;
#if BUG_289
	delta = 0.0;
#else
	delta = ps_my_schedule_time - msg->time_stamp;
#endif
	tp->r_delay.record( delta );
	tp->r_delay_sqr.record( square( delta ) );
	timeline_trace( SYNC_INTERACTION_ESTABLISHED, msg->client, msg->intermediate, ep, msg->time_stamp );
    }

    if ( ep->is_activity() ) {
	_current_phase = 0;
	_phase_start_time = start_time;
	ep->_active[0] += 1;

	ep->_phase[0].r_util.record_offset( ep->_active[0], start_time ); 

	run_activities(  ep, ep->_activity, reschedule );

	/* Flush threads */

	flush_threads();

	/* Funky stat recording. */

	delta = now() - _phase_start_time;  
	p = _current_phase;
	Activity * phase = &ep->_phase[p];
	ep->_active[p] -= 1;
	phase->r_util.record( ep->_active[p] );
	phase->r_cycle.record( delta );
	phase->r_cycle_sqr.record( square(delta) );
	if ( phase->_hist_data ) {
	    phase->_hist_data->insert(delta);
	}

	/*tomari, quorum: If we have not encountered a replying activity 
	  after all activities have been executed, then send a reply to the 
	  calling task or client. In a quorum, the last activity in the activity 
	  graph is not necessarily the last one will complete execution.*/
	if ( Pragma::__pragmas->quorum_delayed_calls() && msg && _current_phase == 0 ) {
#if defined(debug_quorum_flag)
	    printf("\ndefault reply port at end of task: msg->reply_port=%d", 
		   msg->reply_port);
	    fflush(stdout);
#endif
#if !BUG_289
	    ps_send( msg->reply_port , 0, (char *)msg, std_port() );
#endif
	}

    } else if ( ep->is_regular() ) {
	for ( p = 0; p < _cp->max_phases(); ++p ) {
	    _current_phase = p;
	    ep->_active[p] += 1;
	    execute_activity( ep, (Activity *)&ep->_phase[p], reschedule );
	    ep->_active[p] -= 1;
	}

    } else {
#if !BUG_289
	ps_kill( task_id() );				/* nothing to do, so abort */
#endif
    }

    delta = now() - start_time;
    ep->r_cycle.record( delta );		/* Entry cycle time.	*/
    _cp->r_cycle.record( delta );		/* Task cycle time.	*/

    end_msg = _entry[ep->index()];
    if ( end_msg ) {
	if ( end_msg->reply_port == -1 ) {
	    _cp->free_message( end_msg );
	} else if ( ep->_join_list == nullptr && _cp->is_sync_server() ) {
	    _cp->getDOM()->runtime_error( LQIO::ERR_REPLY_NOT_GENERATED );
	}
    }

    if ( _cp->_join_start_time == 0.0 ) {
	_cp->_active -= 1;
    }
    _cp->r_util.record( _cp->_active );

#if !BUG_289
    ps_my_schedule_time = now();		/* In case we don't block...	*/
#endif
}


/*----------------------------------------------------------------------*/
/*		     Task class implementations.			*/
/*----------------------------------------------------------------------*/

/*
 * These tasks do the actual "computation" for the simulation 
 */

Real_Instance::Real_Instance( Task * cp, const std::string& task_name ) 
    : Instance( cp, task_name, Real_Instance::create_task( cp, task_name ) )
{
}


int
Real_Instance::create_task( Task * cp, const std::string& task_name )
{
#if BUG_289
    return 0.0;
#else
    if ( cp->group_id() != -1 ) {
	return ps_create_group( task_name.c_str(), cp->node_id(), ANY_HOST, Instance::start, cp->priority(), cp->group_id() );
    } else {
	return ps_create( task_name.c_str(), cp->node_id(), ANY_HOST, Instance::start, cp->priority() );
    }
#endif
}


/*
 * These tasks coordinate the execution of other tasks.  They are all
 * assigned their own node so that they do not interfere with each
 * other.
 */

Virtual_Instance::Virtual_Instance( Task * cp, const std::string& task_name ) 
    : Instance( cp, task_name, Virtual_Instance::create_task( cp, task_name ) )
{
}

int
Virtual_Instance::create_task( Task * cp, const std::string& task_name )
{

    /*
     * if the task is a cfs task and this task instance have no
     * group, we still create a fcfs node for it.  
     */

#if BUG_289
    return 0;
#else
    const int local_node_id = ps_build_node( task_name.c_str(), 1, 1.0, 0.0, 0, true );
    if ( local_node_id < 0 ) {
	LQIO::runtime_error( ERR_CANNOT_CREATE_X, "processor for task", task_name.c_str() );
	return 0;
    } else {
	return ps_create( task_name.c_str(), local_node_id, 0, Instance::start, cp->priority() );
    }
#endif
}

/*----------------------------------------------------------------------*/
/*		     Task class implementations.			*/
/*----------------------------------------------------------------------*/

/*
 * Open arrival generator task.  They are exactly like client tasks
 * with the exception that they only have one entry.  Most of the work
 * is performed in store_open_arrival_rate in parainout.c.
 */

srn_open_arrivals::srn_open_arrivals( Task * cp, const std::string& task_name )
    : Virtual_Instance( cp, task_name ) 
{
    client_init_count += 1;		/* For -C auto init. 			*/
}



void
srn_open_arrivals::run (void)
{
    timeline_trace( TASK_CREATED );

    /* ---------------------- Main loop --------------------------- */

    for ( ;; ) {				/* Start Client Cycle	*/
	server_cycle( _cp->entries()[0], 0, false );
    }
}



/*
 */

void
srn_sync_server::run()
{
    abort();

    /* spawn activity for each appropriate thread */
}



/*
 * Client or reference task.  They never receive -- only send.
 * Sends occur during phase 2.  Think time is an extra delay
 * component.
 */

srn_client::srn_client( Task * cp, const std::string& task_name  )
    : Real_Instance( cp, task_name ), _think_time( nullptr )
{
    client_init_count += 1;		/* For -C auto init. 			*/
    if ( cp->think_time() > 0.0 ) {
	_think_time = new Exponential( _cp->think_time() );
    }
}


srn_client::~srn_client()
{
    if ( _think_time ) delete _think_time;
}



void
srn_client::run()
{
    timeline_trace( TASK_CREATED );

    /* ---------------------- Main loop --------------------------- */

    for ( ;; ) {				/* Start Client Cycle	*/
	client_cycle( _think_time );
    }
}



/*
 * Server, or non-reference task.  Unique instances only.
 */

void
srn_server::run()
{
    long entry_id;

    timeline_trace( TASK_CREATED );

    /* ---------------------- Main loop --------------------------- */

    for ( ;; ) {
	double start_time = now();

	Message * msg = wait_for_message( entry_id );

	/* If start_time == now(), then we continued right through */
	/* the receive, hence we will want to force a reschedule.   */ 

	server_cycle( Entry::entry_table[entry_id], msg, start_time == now() );
    }
}



/*
 * If we have an "unlimited" number of servers, then we're infinite.
 */

const std::string&
srn_multiserver::type_name() const
{ 
    if ( _max_workers != static_cast<unsigned long>(~0) ) {
	return Task::type_strings.at(Task::Type::MULTI_SERVER); 
    } else {
	return Task::type_strings.at(Task::Type::INFINITE_SERVER); 
    }
}


/*
 * A dispatcher for multi-server type tasks.  The infinite server-type
 * tasks are just like queue-type tasks except that new workers are
 * automagically allocated when needed.  
 */

void
srn_multiserver::run()
{
    timeline_trace( TASK_CREATED );
    unsigned long n_workers = 0;

    for ( ;; ) {

	/* Stuff for server (srn_worker) */

	long worker_port;
	long worker_id;
	double worker_time;
	Message * worker_msg;
	int rc;

	/* Stuff from client */

	long entry_id;

	/* ---- Wait for request ---- */

	Message * msg= wait_for_message( entry_id );

	/* get worker */

#if !BUG_289
	const double time_out = (n_workers < _max_workers) ? IMMEDIATE : NEVER;
	rc = ps_receive( _cp->worker_port(), time_out, &worker_id, &worker_time, (char **)&worker_msg, &worker_port );

	if ( rc == 0 ) {

	    /* Time out -- make a worker */

	    n_workers += 1;
	    Instance * task = new srn_worker( _cp, _cp->name().c_str() );
	    ps_resume( task->task_id() );
	    rc = ps_receive( _cp->worker_port(), NEVER, &worker_id, &worker_time,
			     (char **)&worker_msg, &worker_port );
	} else if ( rc == SYSERR ) {
	    abort();
	}

	timeline_trace( WORKER_DISPATCH );

	/* dispatch */

	ps_send( worker_port, entry_id, (char *)msg, msg->reply_port );
#endif
    }
}



/*
 * A dispatcher for multi-server type tasks.  Queue-type tasks have a fixed
 * size pool.
 */

void
srn_semaphore::run()
{
    timeline_trace( TASK_CREATED );
    Semaphore_Task * cp = dynamic_cast<Semaphore_Task *>(_cp);

    /* Create tokens */

    const bool count_down = dynamic_cast<LQIO::DOM::RWLockTask *>(cp->getDOM()) || dynamic_cast<LQIO::DOM::SemaphoreTask *>(cp->getDOM())->getInitialState() == LQIO::DOM::SemaphoreTask::InitialState::FULL;
    for ( unsigned i = 0; i < cp->multiplicity(); ++i ) {
	Instance * task;
	if ( count_down ) {
	    task = new srn_token( cp, name() );
	} else {
	    task = new srn_token_r( cp, name() );
	}
#if !BUG_289
	ps_resume( task->task_id() );
#endif
    }


    /* Now run the main loop. */

    for ( ;; ) {

	/* Stuff for server (srn_worker) */

	long worker_port;
	long worker_id;
	double worker_time;
	Message * worker_msg;

	/* Stuff from client */

	long entry_id;

	/* ---- Wait for request ---- */

	Message * msg = wait_for_message2( entry_id );

	/* get worker */

#if !BUG_289
	if ( ps_receive( cp->worker_port(), NEVER, &worker_id, &worker_time,
			 (char **)&worker_msg, &worker_port ) == SYSERR ) {
	    abort();
	}

	timeline_trace( WORKER_DISPATCH );

	/* dispatch */

	if (cp->discipline() == SCHEDULE_RWLOCK ) {
	    ps_resend( worker_port, entry_id, msg->time_stamp, (char *)msg, msg->reply_port );	    //keep the time stamp	
	} else {
	    ps_send( worker_port, entry_id, (char *)msg, msg->reply_port );
	}
#endif
    }
}


/*
 * A dispatcher for multi-server type tasks.  In this case, its the
 * signal for a semaphore.  Signalling a semaphore which is not
 * waiting is an error.
 */

void
srn_signal::run()
{
    long entry_id;
    Semaphore_Task * cp = dynamic_cast<Semaphore_Task *>(_cp);

    timeline_trace( TASK_CREATED );

    for ( ;; ) {

	/* Stuff for server (srn_worker) */

	long worker_port;
	long worker_id;
	double worker_time;
	Message * worker_msg;

	/* ---- Wait for request ---- */

	Message * msg = wait_for_message2( entry_id );
#if !BUG_289
	ps_sleep(0);		/* Force context switch */

	/* ---- get worker (should be queued) --- */

	int rc = ps_receive( cp->signal_port(), IMMEDIATE, &worker_id, &worker_time, (char **)&worker_msg, &worker_port );
	if ( rc == 0 ) {
	    LQIO::runtime_error( ERR_SIGNAL_NO_WAIT, cp->name().c_str() );
	    continue;
	} else if ( rc == SYSERR ) {
	    abort();
	}

	timeline_trace( WORKER_DISPATCH );

	/* dispatch */

	ps_send( worker_port, entry_id, (char *)msg, msg->reply_port );
#endif
    }
}


/* ----------------------------------------------------------------------*/

/*
 * Worker -- a non-reference task.  This type of task is an instance of
 * a multi-server.  They are dispatched from srn_queue tasks.
 */

void
srn_worker::run()
{
    timeline_trace( TASK_CREATED );

    for ( ;; ) {
	double time_stamp;	/* time message sent.		*/
	long entry_id;		/* entry id			*/
	long reply_port;	/* reply port			*/
	Message * msg;		/* Time stamp info from client	*/
	double start_time = now();

#if !BUG_289
	if ( ps_send( _cp->worker_port(), -1, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}
	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );
#endif

	if ( msg ) {
	    msg->reply_port = reply_port;
	}

	/* If start_time == now(), then we continued right through */
	/* the receive, hence we will want to force a reschedule.   */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == now() );

	timeline_trace( WORKER_IDLE );
    }
}



/* ----------------------------------------------------------------------*/

/*+ BUG_164
 * Semaphore -- a non-reference task.  This type of task is an
 * instance of a semaphore token.  They are dispatched from srn_queue
 * tasks.
 */

void
srn_token::run()
{
    Message * msg;		/* Time stamp info from client	*/
    long entry_id;		/* entry id			*/
    Semaphore_Task * cp = dynamic_cast<Semaphore_Task *>(_cp);

    timeline_trace( TASK_CREATED );

    for ( ;; ) {
	double time_stamp;	/* time message sent.		*/
	long reply_port;	/* reply port			*/
	double start_time = now();

	/* Send to the wait task first and do the wait processing. */

#if !BUG_289
	if ( ps_send( cp->worker_port(), -1, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	if(cp->discipline()==SCHEDULE_RWLOCK){
	
	    if(time_stamp!=now()){
		const double delta = now() - time_stamp;
		dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_wait.record( delta );
		dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_wait_sqr.record( square( delta ) );
	    }

	}

	_hold_start_time = now();			/* Time we were "waited". */
	cp->_hold_active += 1;
	cp->r_hold_util.record( cp->_hold_active );

	/* Send to the signal task next and do the signal processing. */

	if ( ps_send( cp->signal_port(), -1, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	/* Wait procesing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == now() );

	timeline_trace( TASK_IS_WAITING, 1 );

	/* Now wait for signal */

	ps_receive( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );
#endif

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	/* Signal processing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == now() );

	/* All done, wait for "wait" */

	cp->_hold_active -= 1;
	const double delta = now() - _hold_start_time;

	if(cp->discipline()==SCHEDULE_RWLOCK){

	    dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_hold.record( delta );
	    dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_hold_util.record( cp->_hold_active );
	    dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_hold_sqr.record( square( delta ) );

	}else{
			
	    cp->r_hold_util.record( cp->_hold_active );
	    cp->r_hold.record( delta );
	    cp->r_hold_sqr.record( square( delta ) );
	}

	if ( cp->_hist_data ) {
	    cp->_hist_data->insert( delta );
	}

	timeline_trace( WORKER_IDLE );
    }
}


/*
 * Semaphore -- a non-reference task.  This type of task is an
 * instance of a semaphore token.  They are dispatched from srn_queue
 * tasks.
 */

void
srn_token_r::run()
{
    Message * msg;		/* Time stamp info from client	*/
    long entry_id;		/* entry id			*/
    Semaphore_Task * cp = dynamic_cast<Semaphore_Task *>(_cp);

    _hold_start_time = now();		/* Time we were "waited". */
    cp->_hold_active += 1;
    timeline_trace( TASK_CREATED );

    for ( ;; ) {
	double time_stamp;	/* time message sent.		*/
	long reply_port;	/* reply port			*/
	double start_time = now();

	/* Send to the signal task first */

#if !BUG_289
	if ( ps_send( cp->signal_port(), -1, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	cp->_hold_active -= 1;
	double delta = now() - _hold_start_time;
	cp->r_hold.record( delta );
	cp->r_hold_sqr.record( square( delta ) );
	cp->r_hold_util.record( cp->_hold_active );
	if ( cp->_hist_data ) {
	    cp->_hist_data->insert( delta );
	}

	/* Send to the queue (wait) task next and do the signal processing. */

	if ( ps_send( cp->worker_port(), -1, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}
#endif
	/* Signal procesing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == now() );

	timeline_trace( TASK_IS_WAITING, 1 );

	/* Now wait for wait */

#if !BUG_289
	ps_receive( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
#endif
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	cp->_hold_active += 1;
	_hold_start_time = now();
	cp->r_hold_util.record( cp->_hold_active );

	/* Wait processing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == now() );

	/* All done, wait for "signal" */

	timeline_trace( WORKER_IDLE );
    }
}
/*- BUG_164 */


/******************************************************/
/*  RWLOCK  */
/*
 * rwlock , non-reference task.  Unique instances only.
 */
void
srn_rwlock_server::run()
{
    long entry_id;
    Message * msg1;
    long entry_id1;
    double time_stamp;	/* time message sent.		*/
    long reply_port;	/* reply port			*/
    ReadWriteLock_Task * cp = dynamic_cast<ReadWriteLock_Task *>(_cp);


    Entry *ep;
    int writers, readers, activeReaders;
    writers= readers= activeReaders=0;

    timeline_trace( TASK_CREATED );

    /* ---------------------- Main loop --------------------------- */

    for ( ;; ) {
	//double start_time = now();

	Message * msg = wait_for_message( entry_id );

	/* dispatch request to reader queue or worker token*/

	ep = Entry::entry_table[entry_id]; 

#if !BUG_289
	if (ep->is_r_lock() ) {
	    /*	lock reader request*/
	    readers++;
	    if (writers>0){
		/* there is at least a writer waiting or writing;
		   enqueue the reader request into internal readerQ;*/
		if ( ps_resend( cp->readerQ_port(), entry_id, msg->time_stamp, (char *)msg, msg->reply_port ) != OK ) {	abort();}

		timeline_trace( ENQUEUE_READER, 1 );
	    }else{
		/* no writer(s) waiting, send request to reader queue task */

		if ( ps_resend( cp->reader()->std_port(), entry_id, msg->time_stamp, (char *)msg, msg->reply_port ) != OK ) {	abort();}
		activeReaders++;
	    }

	}else if (ep->is_r_unlock() ) {
	    /* unlock reader request; */

	    if ( ps_send( cp->signal_task()->std_port(), entry_id, (char *)msg, msg->reply_port ) != OK ) {	abort(); }

	    activeReaders--;
	    readers--;

	    if (activeReaders==0  &&  writers>0){
		/* the last reader finished and signal to writers;  dequeue a writer; */

		ps_receive(cp->writerQ_port(), IMMEDIATE, &entry_id1, &time_stamp, (char **)&msg1, &reply_port);
		if ( ps_resend( cp->writer()->std_port(), entry_id1, time_stamp, (char *)msg1, reply_port ) != OK ) { abort();	}
		timeline_trace( DEQUEUE_WRITER, 1 );
		/*
		  delta = now() - time_stamp;
		  cp->r_writer_wait.record( delta );
		  cp->r_writer_wait_sqr.record( square( delta ) );
		*/
	    }
	}else if (ep->is_w_lock() ) {
	    /* writer lock request;*/

	    writers ++;
	    if (writers>1 || activeReaders>0){
		/* there is an active writer or some active readers
		   enqueue the writer request */
			
		if ( ps_resend( cp->writerQ_port(), entry_id, msg->time_stamp, (char *)msg, msg->reply_port ) != OK ) {	abort();}
		timeline_trace( ENQUEUE_WRITER, 1 );
	    }else{
		/* dispatch writer request;*/

		if ( ps_resend( cp->writer()->std_port(), entry_id, msg->time_stamp, (char *)msg, msg->reply_port ) != OK ) {  abort(); 	}
	    }
	}else  {
	    /*  unlock writer  request;
		writer_token task receive both writer lock request and unlock signal at std_port,
		in order to make sure the unlock writer signal is arrive after the lock writer request, 
		and before the next the lock writer request. */
	    long writer_id;

	    int rc = ps_receive(cp->signal_port2(), IMMEDIATE, &writer_id,  &time_stamp, (char **)&msg1, &reply_port);
		
	    if ( rc == 0 ) {
		LQIO::runtime_error( ERR_SIGNAL_NO_WAIT, cp->name().c_str() );
		continue;

	    } else if ( rc == SYSERR ) {     abort();  }

	    /*  signal writer_token; */
	    if ( ps_resend( cp->writer()->std_port(), entry_id, msg->time_stamp, (char *)msg, msg->reply_port ) != OK ) { abort();	}
			
	    writers --;
	    if (writers>0){

		/* dequeue another writer locking request */
		ps_receive(cp->writerQ_port(), IMMEDIATE, &entry_id1, &time_stamp, (char **)&msg1, &reply_port);
		if ( ps_resend( cp->writer()->std_port(), entry_id1, time_stamp, (char *)msg1, reply_port ) != OK ) {  	abort();	}
		timeline_trace( DEQUEUE_WRITER, 1 );

		/*	delta = now() - time_stamp;
			cp->r_writer_wait.record( delta );
			cp->r_writer_wait_sqr.record( square( delta ) );*/
	    }
	    else if (readers>0){
		/* some readers are waiting in the queue. */

		while (readers-activeReaders>0){
				
		    ps_receive(cp->readerQ_port(), IMMEDIATE, &entry_id1, &time_stamp, (char **)&msg1, &reply_port);
		    if ( ps_resend( cp->reader()->std_port(), entry_id1, time_stamp, (char *)msg1, reply_port ) != OK ) {	abort();	}
			
		    activeReaders++;
		    timeline_trace( DEQUEUE_READER, 1 );
		    /*
		      delta = now() - time_stamp;
		      cp->r_reader_wait.record( delta );
		      cp->r_reader_wait_sqr.record( square( delta ) );
		    */
		} 
	    }
	}
#endif
    }
}


/*
 * Rwlock -- a non-reference task.  This type of task is an
 * instance of a writer_token.  It is are dispatched from srn_rwlock_server
 * tasks.
 */

void
srn_writer_token::run()
{
    Message * msg;		/* Time stamp info from client	*/
    long entry_id;		/* entry id			*/
    ReadWriteLock_Task * cp = dynamic_cast<ReadWriteLock_Task *>(_cp);

    timeline_trace( TASK_CREATED );

    for ( ;; ) {

	double time_stamp;	/* time message sent.		*/
	long reply_port;	/* reply port			*/
	double start_time = now();

	/* Send to the wait task first and do the wait processing. */

	timeline_trace( TASK_IS_WAITING, 1 );

#if !BUG_289
	ps_receive( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}
	
	if(time_stamp!=now()){
	    const double delta = now() - time_stamp;
	    cp->r_writer_wait.record( delta );
	    cp->r_writer_wait_sqr.record( square( delta ) );
	}
	
        _hold_start_time = now();			/* Time we were "waited". */
	cp->_hold_active += 1;
	cp->r_writer_hold_util.record( cp->_hold_active );

	/* Send to the signal task next and do the signal processing. */

	if ( ps_send( cp->signal_port2(), -1, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	/* writer procesing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == now() );

	timeline_trace( TASK_IS_WAITING, 1 );

	/* Now wait for unlocking signal */

	ps_receive( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );

	timeline_trace( TASK_IS_READY, 1 );
	
	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	/* Signal processing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == now() );

	/* All done, wait for "wait" */

	cp->_hold_active -= 1;
	const double delta = now() - _hold_start_time;
	cp->r_writer_hold_util.record( cp->_hold_active );
	cp->r_writer_hold.record( delta );
	cp->r_writer_hold_sqr.record( square( delta ) );

	timeline_trace( WORKER_IDLE );
#endif
    }
}    
/*  -RWLOCK  */

/*
 * Thread -- This type of task is an instance of a task with activites
 * that fork.  They are dispatched from any other type of task.
 */

void
srn_thread::run()
{
    Activity * ap 	= 0;
#if !BUG_289
    Instance * pp 	= object_tab[ps_task_parent(ps_myself)];
#endif

    timeline_trace( TASK_CREATED );

    for ( ;; ) {
	double time_stamp;		/* Time stamp.		*/
	long entry_id;			/* entry id		*/
	long reply_port;		/* reply port		*/
	Entry * ep;

#if !BUG_289
	if ( ps_send( pp->thread_port(), -1, (char *)ap, ps_my_std_port ) != OK ) {
	    abort();
	}

	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive_shared( pp->start_port(), NEVER, &entry_id, &time_stamp, (char **)&ap, &reply_port );

#endif
	timeline_trace( TASK_IS_READY, 1 );
	timeline_trace( THREAD_START, ap );

	ep = Entry::entry_table[entry_id];
	run_activities( ep, ap, 0 );

	timeline_trace( THREAD_STOP, ap, root_ptr() );
    }
}

/*----------------------------------------------------------------------*/
/*		       Processor Utilities.				*/
/*----------------------------------------------------------------------*/

/*
 * Main line port manipulation.  Stuff for blocking ports and priority queueing will
 * have to go here.
 */

Message *
Instance::wait_for_message( long& entry_id )
{
    Message *msg = nullptr;

#if !BUG_289
    ps_my_schedule_time = now();		/* In case we don't block...	*/
#endif

    /* Handle any pending messages if possible */
    
    for ( std::list<Message *>::iterator i = _cp->_pending_msgs.begin(); i != _cp->_pending_msgs.end(); ++i ) {
	msg = *i;					/* This is returned!			*/
	Entry * ep = msg->target->entry();
	if ( ep->_join_list == nullptr ) {			/* Entry is available to receive.	*/
	    entry_id = ep->entry_id();			/* This is returned!			*/
	    _cp->_pending_msgs.erase(i);		/* Remove Message from queue		*/
	    return msg;					/* All done.				*/
	}
    }

    /* Handle any new messages */
    
    timeline_trace( TASK_IS_WAITING, 1 );
    for ( ;; ) {
	double time_stamp;		/* time message sent.		*/
	long reply_port;		/* reply port			*/

#if !BUG_289
	if ( _cp->discipline() == SCHEDULE_HOL ) {
	    assert( ps_receive_priority( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port ) != SYSERR );
	} else {
	    if ( parentPort() > 0 ) {
		ps_send( parentPort(), entry_id, (char *)msg, ps_my_std_port );
	    }
	    assert( ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port ) != SYSERR );
	}
#endif
	msg->reply_port = reply_port;
	msg->time_stamp = time_stamp;

	/* Is entry busy? !JOIN! */

	const Entry * ep = Entry::entry_table[entry_id];
	if ( ep->_join_list == nullptr ) break;		/* Entry available - done!		*/

	_cp->_pending_msgs.push_back( msg ); 	    	/* queue message and try again.		*/
    }
    timeline_trace( TASK_IS_READY, 1 );
    return msg;
}

Message *
Instance::wait_for_message2( long& entry_id )
{
    Message * msg = nullptr;	/* Message from client		*/
    double time_stamp;		/* time message sent.		*/
    long reply_port;		/* reply port			*/

    timeline_trace( TASK_IS_WAITING, 1 );
#if !BUG_289
    ps_my_schedule_time = now();		/* In case we don't block...	*/

    assert( ps_receive( std_port(), NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port ) != SYSERR );
#endif
	
    if ( msg ) {
	msg->reply_port = reply_port;
	msg->time_stamp = time_stamp;
    }

    timeline_trace( TASK_IS_READY, 1 );
    return msg;
	
}

/*
 * Compute, taking into account the coefficient of variation.  ap is
 * the current activity.  sp is the associated entry.
 */

double
Instance::compute ( Activity * ap, Activity * pp )
{
    double time = 0.0;

    if ( ap->_slice_time != nullptr ) {

	time = ap->get_slice_time();
	timeline_trace( TASK_IS_COMPUTING, time );

	r_a_execute = &ap->r_cpu_util;

	ap->_cpu_active += 1;
	ap->r_cpu_util.record( ap->_cpu_active );	/* Phase P execution.	*/
	if ( ap != pp ) {
	    r_e_execute = &pp->r_cpu_util;
	    pp->_cpu_active += 1;
	    pp->r_cpu_util.record( pp->_cpu_active );	/* CPU util by phase */
	}

#if !BUG_289
	(*_cp->_compute_func)( time );
#endif

	ap->_cpu_active -= 1;
	ap->r_cpu_util.record(  pp->_cpu_active );	/* Phase P execution.	*/
	if ( ap != pp ) {
	    pp->_cpu_active -= 1;
	    pp->r_cpu_util.record( pp->_cpu_active );
	}
	r_a_execute = nullptr;
	r_e_execute = nullptr;

    }

#if !BUG_289
    if ( _cp->_compute_func == ps_sleep || !ap->has_service_time() ) {
	ps_my_end_compute_time = now();	/* Won't call the end_compute handler, ergo, set here */
    }
    ps_my_schedule_time = now();
#endif
    ap->r_service.record( time );

    return time;
}

/*----------------------------------------------------------------------*/
/*		       Miscellaneous Utilities.				*/
/*----------------------------------------------------------------------*/


/*
 * Either reply to or forward the request.  NOTE: If the task is of type THREAD, the
 * msg will always be freed on thread completion.  Therefore, we have to copy
 */

void
Instance::do_forwarding( Message * msg, const Entry * ep )
{
    if ( !msg ) {
	return;				/* No operation.	*/

    } else if ( msg->reply_port == -1 ) {

	_cp->free_message( msg );

    } else {
	const Targets& fwd = ep->_fwd;	/* forwarding pointer	*/
	unsigned int i = 0;		/* loop index		*/
	unsigned int j = 0;		/* loop index (det ph.)	*/
	tar_t * tp = fwd.entry_to_send_to( i, j );

#if !BUG_289
	if ( !tp ) {

	    /* Reply to sender	*/

	    long reply_port  = msg->reply_port;	/* Local copy	*/

	    timeline_trace( SYNC_INTERACTION_REPLIES, ep, msg->client );
	    ps_send( reply_port, 0, (char *)msg->init( ep, nullptr ), ps_my_std_port );

	} else {
				
	    timeline_trace( SYNC_INTERACTION_FORWARDED, ep, msg->client, tp->entry() );

	    msg->time_stamp   = now(); 		/* Tag send time.	*/
	    msg->intermediate = ep;
	    msg->target       = tp;

	    ps_send( fwd[i].entry()->get_port(),	/* Forward request.	*/
		     fwd[i].entry()->entry_id(), (char *)msg, msg->reply_port );
	}
#endif
    }
}


/*
 * Randomly shuffle items.
 */

void
Instance::random_shuffle_reply( std::vector<const Entry *>& array )
{
    const size_t n = array.size();
    for ( size_t i = n; i >= 1; --i ) {
	const size_t k = static_cast<size_t>(Random::number() * i);
	if ( i-1 != k ) {
	    const Entry * temp = array[k];
	    array[k] = array[i-1];
	    array[i-1] = temp;
	}
    }
}

/*----------------------------------------------------------------------*/
/*                         Activity sequencing.                         */
/*----------------------------------------------------------------------*/


/*
 * Sequence through activity list.
 */

void
Instance::run_activities( Entry * ep, Activity * ap, bool reschedule )
{
    while ( ap ) {
	execute_activity( ep, ap, reschedule );
	ap = next_activity( ep, ap, reschedule );
    }
}



/*
 * Run an activity.
 */

void
Instance::execute_activity( Entry * ep, Activity * ap, bool& reschedule )
{
#if BUG_289
    double start_time = 0.;
#else
    double start_time = ps_my_schedule_time;
#endif
    double slices = 0.0;
    unsigned int p = _current_phase;
    int count = ap->is_specified();
    double delta;
    Activity * phase = &ep->_phase.at(p);

    timeline_trace( ACTIVITY_START, ap );

    ap->_active += count;
    ap->r_util.record_offset( ap->_active, start_time );		/* Activity utilization.*/

    /*
     * Delay for "think time".  
     */

#if !BUG_289
    if ( ap->has_think_time() ) {
	const double think_time = ap->get_think_time();
	ps_my_schedule_time = now() + think_time;
	ps_sleep( think_time );
    }
#endif

    /*
     * Now do service.
     */

    if ( ap->has_calls() > 0 || ap->_slice_time != nullptr ) {
	double sends = 0.0;
	unsigned int i = 0;		/* loop index			*/
	unsigned int j = 0;		/* loop index (det ph.)		*/

	if ( reschedule ) {
	    Processor::reschedule( this );
	}

#if !BUG_289
	delta = now() - ps_my_schedule_time;
#endif
	ap->r_proc_delay.record( delta );		/* Delay for schedul.	*/
	ap->r_proc_delay_sqr.record( square( delta ) );	/* Delay for schedul.	*/
	ap->_prewaiting = delta;			/*Added by Tao*/

	if ( ap != phase ) {
	    phase->r_proc_delay.record( delta );
	    phase->r_proc_delay_sqr.record( square( delta ) );
	    phase->_prewaiting = delta;			/*Added by Tao*/
	}

	timeline_trace( ACTIVITY_EXECUTE, ap );

	for ( ;; ) {

	    compute( ap, phase );
	    slices += 1.0;
	    tar_t * tp = ap->_calls.entry_to_send_to( i, j );

	    if ( !tp ) break;

	    sends += 1.0;
#if !BUG_289
	    if ( tp->reply() ) {
	      //if(ep->name()=="retrievePage") printf("reply port=%ld\n", reply_port());
		tp->send_synchronous( ep, _cp->priority(), reply_port() );
		delta = now() - ps_my_schedule_time;

		ap->r_proc_delay.add( delta );
		ap->r_proc_delay_sqr.add( square(delta + ap->_prewaiting) -  square(ap->_prewaiting) );
		ap->_prewaiting += delta;
		if ( ap != phase ) {
		    phase->r_proc_delay.add( delta );
		    phase->r_proc_delay_sqr.add( square(delta + phase->_prewaiting) -  square(phase->_prewaiting * phase->_prewaiting) );
		    phase->_prewaiting += delta;
		}
		/*End here*/

	    } else {
		tp->send_asynchronous( ep, _cp->priority() );
		if ( Pragma::__pragmas->reschedule_on_async_send() ) {
		    Processor::reschedule( this );
		}
	    }
#endif
	} /* end for loop */

	ap->r_sends.record( sends );
	ap->r_slices.record( slices );
	if ( ap != phase ) {
	    phase->r_sends.record( sends );
	    phase->r_slices.record( slices );
	}

	reschedule = true;
    }

    if ( count ) {
	delta = now() - start_time;				/* Bug 232 */

	ap->r_cycle.record( delta );		/* Entry cycle time.	*/
	ap->r_cycle_sqr.record( square(delta) );	/* Entry cycle time.	*/
	if ( ap->_hist_data ) {
	    ap->_hist_data->insert( delta );
	}
    }

    const unsigned size = ap->_reply.size();
    if ( size ) {					/* reply or forward.	*/

	/* Reply to all possible */

 	random_shuffle_reply( ap->_reply );
	for ( unsigned int i = 0; i < size; ++i ) {
	    const Entry * reply_ep = ap->_reply[i];
	    assert( reply_ep->index() < _cp->n_entries() );
	    Message * msg = root_ptr()->_entry[reply_ep->index()];
	    if ( msg ) {
			 
		/*
		 * 		 !!! KLUDGE ALERT !!!
		 *
		 * The scheduler is deterministic, but we want random
		 * behaviour when the reply takes place.  So, 50% of the
		 * time, hiccup to allow the competing task to run first.
		 * I hate simulations.  Reschedule is set at the end of a
		 * phase/activity, so clear it half the time.
		 */

#if !BUG_289
		if ( msg->reply_port != -1 ) {
		    Instance * dest_ip = object_tab[ps_owner(msg->reply_port)];
		    if ( dest_ip && node_id() == dest_ip->node_id() && Random::number() >= 0.5 ) {
		        reschedule = false;
		    }
		}
#endif
		/* ---------- */

		do_forwarding( msg, reply_ep );	/* Clears msg! */

		root_ptr()->_entry[reply_ep->index()] = 0;

		/* Possible phase change.  Record phase cycle time and utilization */

		if ( p == 0 && ap != &reply_ep->_phase[p] && ep == reply_ep ) {
		    assert( ep->_active[0] );
		    delta = now() - root_ptr()->_phase_start_time;
		    ep->_phase[0].r_cycle.record( delta );
		    ep->_phase[0].r_cycle_sqr.record( square(delta) );
		    if ( ep->_phase[0]._hist_data ) {
			ep->_phase[0]._hist_data->insert( delta );
		    }

		    ep->_active[0] -= 1;
		    ep->_phase[0].r_util.record( ep->_active[0] );
		    ep->_active[1] += 1;
		    ep->_phase[1].r_util.record( ep->_active[1] );

		    root_ptr()->_phase_start_time = now();
		    root_ptr()->_current_phase = 1;
		    p = 1;
		}

	    } else {
		LQIO::runtime_error( ERR_REPLY_NOT_FOUND, ap->name().c_str(), reply_ep->name().c_str() );
	    }

	}
    }

    ap->_active -= count;
    ap->r_util.record( ap->_active );

    /*Add the preemption time to the waiting time if available. Tao*/

#if !BUG_289
    if (ps_preempted_time (task_id()) > 0.0) {   
	ap->r_proc_delay.add( ps_preempted_time(task_id()) );
	ap->r_proc_delay_sqr.add( (ps_preempted_time(task_id()) + ap->_prewaiting) * (ps_preempted_time (task_id()) + ap->_prewaiting) -  square(ap->_prewaiting) );

#if 0
	if ( ap != &ep->phase[phase] ) {
	    phase->r_proc_delay.add( ps_preempted_time (task_id()) );
	    phase->r_proc_delay_sqr.add( (ps_preempted_time (task_id()) + phase->_prewaiting) * (ps_preempted_time (task_id()) + phase->_prewaiting) -  square(phase->_prewaiting) );
	}
#endif

	ps_task_ptr(task_id())->pt_tag = 0;
	ps_task_ptr(task_id())->pt_sum = 0.0;
	ps_task_ptr(task_id())->pt_last = 0.0;

    }
#endif
}



/*
 * locate next node in the activity list.
 * May have to do joins and all that good stuff.
 */

Activity * 
Instance::next_activity( Entry * ep, Activity * ap_in, bool reschedule )
{
    Activity * ap_out = nullptr;
    InputActivityList * fork_list = nullptr;

    if ( ap_in->_output != 0 ) {

	/*
	 * If there is an input list and that input list is of
	 * type join then I have to do some work....
	 */

	AndJoinActivityList * join_list = dynamic_cast<AndJoinActivityList *>(ap_in->_output);

	if ( join_list != nullptr ) {
	    if ( _cp->is_sync_server() && join_list->join_type_is( AndJoinActivityList::Join::SYNCHRONIZATION ) ) {
		if ( root_ptr()->all_activities_done( ap_in ) ) {
		    double delta = now() - _cp->_join_start_time;
		    join_list->r_join.record( delta );
		    join_list->r_join_sqr.record( square( delta ) );
					  
		    _cp->_join_start_time = 0.0;

		    /* Mark entry as ready to accept messages.  Wait_for_message will take the 	*/
		    /* first pending message to this entry if any are present.			*/
		    
		    std::for_each( _cp->entries().begin(), _cp->entries().end(), [=]( Entry * entry ){ if ( entry->_join_list == join_list ) entry->_join_list = nullptr; } );
		} else {
		    /* Mark entry busy */
		    ep->_join_list = join_list;
		    _cp->_join_start_time = now();
		    return nullptr;	/* Do not execute output list. */
		}
	    } else {
		/* tomari:quorum,  Histogram binning needs to be done here. */
		return nullptr;	/* Thread complete. */
	    }
	}
	fork_list = ap_in->_output->get_next();
    }

	/*
	 * Now I can do the fork_list list.  And forks are the biggest headache.
	 */


again_1:
    if ( fork_list ) {
	if ( fork_list->get_type() == ActivityList::Type::AND_FORK_LIST ) {
	    AndForkActivityList * and_fork_list = dynamic_cast<AndForkActivityList *>(fork_list);
	    const double fork_start = now();
	    double thread_K_outOf_N_end_compute_time = 0;

	    /* launch threads */

	    assert( and_fork_list->size() > 0 );

	    timeline_trace( ACTIVITY_FORK, and_fork_list->front(), this );
	    and_fork_list->shuffle();
				  
	    spawn_activities( ep->entry_id(), and_fork_list );

	    /* Wait for all threads to complete, then we're done. */

	    wait_for_threads( and_fork_list, &thread_K_outOf_N_end_compute_time );

	    lastQuorumEndTime = thread_K_outOf_N_end_compute_time;

	    AndJoinActivityList * join_list = dynamic_cast<AndJoinActivityList *>(and_fork_list->get_join());
	    if ( join_list ) {
		const double delta = thread_K_outOf_N_end_compute_time - fork_start; 

		join_list->r_join.record( delta );
		join_list->r_join_sqr.record( square( delta ) );
		if ( join_list->_hist_data ) {
		    join_list->_hist_data->insert( delta );
		}
		fork_list = join_list->get_next();
		timeline_trace( ACTIVITY_JOIN, fork_list->front(), fork_list );

	    } else {
		fork_list = nullptr;
	    }
#if !BUG_289
	    ps_my_end_compute_time = now();				/* BUG 321 */
	    ps_my_schedule_time    = now();				/* BUG 259 */
#endif
	    goto again_1;

	} else if ( fork_list->get_type() == ActivityList::Type::OR_FORK_LIST ) {

	    assert ( fork_list->size() > 0 );
	    const double exit_value = Random::number();
	    double sum = 0.0;
	    size_t i = 0;
	    for ( i = 0; i < fork_list->size(); ++i ) {
		sum += static_cast<OrForkActivityList *>(fork_list)->get_prob_at(i);
		if ( sum > exit_value ) break;
	    }
	    ap_out = fork_list->at(i);

	} else if ( fork_list->get_type() == ActivityList::Type::FORK_LIST ) {
	    assert( fork_list->size() <= 1 );
	    if ( fork_list->size() == 1 ) {
		ap_out = fork_list->front();
	    }

	} else {
	again_2:
	    LoopActivityList * loop_list = dynamic_cast<LoopActivityList *>(fork_list);
#if !BUG_289
	    ps_my_end_compute_time = now();	/* BUG 321 */
#endif
	    const double exit_value = Random::number() * (loop_list->get_total() + 1.0);
	    double sum = 0;
	    for ( ActivityList::const_iterator i  = loop_list->begin(); i < loop_list->end(); ++i ) {
		sum += loop_list->get_count_at(i-loop_list->begin());
		if ( sum > exit_value ) {
		    run_activities( ep, *i, reschedule );
		    goto again_2;
		}
	    }
	    ap_out = loop_list->get_exit();
	}
    }

    return ap_out;
}



/*
 * Start up an activity...
 * if thread waiting 
 * start thread 
 * else create thread 
 * queue activity for run 
 */

void
Instance::spawn_activities( const long entry_id, ActivityList * fork_list )
{
#if !BUG_289
    ps_my_schedule_time = now();		/* In case we don't block...	*/
#endif

    for ( ActivityList::const_iterator i = fork_list->begin(); i != fork_list->end(); ++i ) {
	char * msg = 0;
	Activity * ap = (*i);

	/* Request for service arrives. */

	assert( idle_threads >= 0 );
	assert( ap );

	/* Reap any pending threads if possible */

#if !BUG_289
	while ( thread_wait( IMMEDIATE, (char **)&msg, false, nullptr ) ) {
	    timeline_trace( THREAD_REAP, msg );
	}
#endif

	/*  Think about if there is overflow then more and more theads will be created. */

	if ( idle_threads == 0 ) {

	    /* No worker available -- Allocate a new task... */

	    const std::string nextThreadName = ap->name() + "_thread_" + std::to_string( active_threads + 1 );
	    Instance * task = new srn_thread( _cp, nextThreadName, root_ptr() );
	    const int id = task->task_id();
	    timeline_trace( THREAD_CREATE, id );
#if !BUG_289
	    ps_resume( id  );
	    active_threads += 1;
	    thread_wait( NEVER, &msg, false, nullptr );
#endif

	}

	active_threads += 1;		 
	idle_threads   -= 1;

#if !BUG_289
	if ( ps_send( start_port(), entry_id, (char *)ap, std_port() ) != OK ) {
	    abort();
	}
#endif
    }
}



/*
 * Wait for all forks to finish for the case of AND-Join. In the case of Quorum, just wait for K 
 * out of N threads.
 */

void
Instance::flush_threads()
{
#if !BUG_289
    while ( active_threads > 0  ) {
	Activity * ap;

	thread_wait( NEVER, (char **)&ap, true, nullptr );
    }
#endif
}



/*
 * Wait for a thread.
 */

int
Instance::thread_wait( double time_out, char ** msg, const bool flush, double * thread_end_compute_time )
{
    double time_stamp;			/* time message sent.		*/
    long entry_id;
    long reply_port;
    int rc; 

    /* tomari quorum */

#if !BUG_289
    rc = ps_receive( thread_port(), time_out, &entry_id, &time_stamp, msg, &reply_port );
    if ( rc == SYSERR ) {
	abort();
    } else if ( rc != 0 ) {
	idle_threads   += 1;
	active_threads -= 1;	
    }

    if ( thread_end_compute_time ) {
	*thread_end_compute_time = ps_end_compute_time( ps_owner( reply_port ) ); 

	if ( flush ) { /* flush_thread() call */
	    Activity * replyMsg = (Activity *)(*msg);
	    replyMsg->r_afterQuorumThreadWait.record(*thread_end_compute_time - lastQuorumEndTime);
	}
    }
#endif

    return rc;
}


/*
 * Locate all activities that are in this join set.  If all
 * done, then clear and return true, otherwise, return false.
 */

bool
Instance::all_activities_done( const Activity * ap )
{
    OutputActivityList * join_list = ap->_output;

    _join_done[ap->index()] = true;

    for ( ActivityList::const_iterator i = join_list->begin(); i < join_list->end(); ++i ) {
	if ( !_join_done[(*i)->index()] ) {
	    return false;
	}
    }

    /* All activities tagged -- zap and return */

    for ( ActivityList::const_iterator i = join_list->begin(); i < join_list->end(); ++i ) {
	_join_done[(*i)->index()] = false;	/* !!! */
    }
    return true;
}



void
Instance::wait_for_threads( AndForkActivityList * fork_list, double* thread_K_outOf_N_end_compute_time )
{
    Activity * ap;			/* Time stamp info from client	*/
    int count = fork_list->get_visits();
    AndJoinActivityList * join_list = dynamic_cast<AndJoinActivityList *>(fork_list->get_join());
    const int N = count;
    const int K = (join_list && join_list->get_quorum_count() > 0) ? join_list->get_quorum_count() : count;

    if ( count == 0 ) return;

    timeline_trace( TASK_IS_WAITING, 1 );

#if !BUG_289
    ps_my_schedule_time = now();	/* In case we don't block...	*/

    while ( count > (N - K) ) {

	thread_wait( NEVER, (char **)&ap, false, thread_K_outOf_N_end_compute_time );

	/* Only acknowledge active threads */
 
	for ( size_t i = 0; i < fork_list->size(); ++i ) {
	    if ( fork_list->at(i) && fork_list->at(i) == ap ) {
		count -= 1;
		break;
	    }
	}
    }
#endif
    timeline_trace( TASK_IS_READY, 1 );
}

/*----------------------------------------------------------------------*/
/* Timeline Stuff.							*/
/*----------------------------------------------------------------------*/

/*
 * Emit timeline tracing info.
 */

void
Instance::timeline_trace( const trace_events event, ... )
{
    va_list args;

    va_start( args, event );

    if ( _cp->trace_flag && !timeline_flag ) {
	double time;			/* Args for va-arg */
	Entry * from_entry;
	Entry * int_entry;
	Entry * to_entry;
	Activity * ap;
	ActivityList * alp;
	int an_int;
	Instance * root;

	if ( ((1 << event) & watched_events ) == 0 ) {
	    return;
	}

	if ( trace_driver ) {
	    (void) fprintf( stddbg, "\nTime* %8g T %s(%ld): ", now(), _cp->name().c_str(), task_id() );
	} else {
	    (void) fprintf( stddbg, "%8g %8s %8s(%2ld): ", now(), type_name().c_str(), _cp->name().c_str(), task_id() );
	}

	switch ( event ) {
	case TASK_CREATED:
	    (void) fprintf( stddbg, "%s created.", type_name().c_str() );
	    break;

	case ASYNC_INTERACTION_INITIATED:
	    from_entry = va_arg( args, Entry * );
	    to_entry   = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Sending SNR to task %s, entry %s",
			    from_entry->name().c_str(),
			    to_entry->task()->name().c_str(),
			    to_entry->name().c_str() );
	    break;

	case SYNC_INTERACTION_INITIATED:
	    from_entry = va_arg( args, Entry * );
	    to_entry   = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Sending RNV to task %s, entry %s",
			    from_entry->name().c_str(),
			    to_entry->task()->name().c_str(),
			    to_entry->name().c_str() );
	    break;

	case SYNC_INTERACTION_ESTABLISHED:
	    from_entry = va_arg( args, Entry * );
	    int_entry  = va_arg( args, Entry * );
	    to_entry   = va_arg( args, Entry * );
	    time       = va_arg( args, double );
	    (void) fprintf( stddbg, "Entry %s -- Received msg from task %s, entry %s (t=%g)",
			    to_entry->name().c_str(),
			    from_entry->task()->name().c_str(), 
			    from_entry->name().c_str(),
			    time );
	    if ( int_entry ) {
		(void) fprintf( stddbg, ", forwarded via task %s, entry %s",
				int_entry->task()->name().c_str(), 
				int_entry->name().c_str() );
	    }
	    break;

	case SYNC_INTERACTION_REPLIES:
	    to_entry   = va_arg( args, Entry * );
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Reply sent to task %s, entry %s",
			    to_entry->name().c_str(),
			    from_entry->task()->name().c_str(), 
			    from_entry->name().c_str() );
	    break;

	case SYNC_INTERACTION_COMPLETED:
	    to_entry   = va_arg( args, Entry * );
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Reply received from task %s, entry %s",
			    to_entry->name().c_str(),
			    from_entry->task()->name().c_str(),
			    from_entry->name().c_str() );

	    break;

	case SYNC_INTERACTION_FORWARDED:
	    int_entry  = va_arg( args, Entry * );
	    from_entry = va_arg( args, Entry * );
	    to_entry   = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Message from task %s, entry %s forwarded to task %s, entry %s",
			    int_entry->name().c_str(),
			    from_entry->task()->name().c_str(), 
			    from_entry->name().c_str(),
			    to_entry->task()->name().c_str(),
			    to_entry->name().c_str() );
	    break;


	case SYNC_INTERACTION_ABORTED:
	    to_entry   = va_arg( args, Entry * );
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Reply tossed for task %s, entry %s",
			    to_entry->name().c_str(),
			    from_entry->task()->name().c_str(), 
			    from_entry->name().c_str() );
	    break;

	case TASK_IS_READY:
	    (void) fprintf( stddbg, "Ready." );
	    break;

	case TASK_IS_RUNNING:
	    (void) fprintf( stddbg, "Running." );
	    break;

	case TASK_IS_COMPUTING:
	    time = va_arg( args, double );
	    (void) fprintf( stddbg, "Computing [%g]", time );
	    break;

	case TASK_IS_WAITING:
	    (void) fprintf( stddbg, "Waiting." );
	    break;

	case WORKER_DISPATCH:
	    (void) fprintf( stddbg, "Dispatch..." );
	    break;

	case WORKER_IDLE:
	    (void) fprintf( stddbg, "...Idle" );
	    break;


	case THREAD_START:
	    ap = va_arg( args, Activity * );
	    (void) fprintf( stddbg, "Thread start, activity %s.", ap->name().c_str() );
	    break;

	case THREAD_STOP:
	    ap   = va_arg( args, Activity * );
	    root = va_arg( args, Instance * );
	    (void) fprintf( stddbg, "Thread stop, activity %s", ap->name().c_str() );
	    if ( root ) {
		(void) fprintf( stddbg, ", root is %s(%ld).", 
				root->_cp->name().c_str(), root->task_id() );
	    } else {
		(void) fprintf( stddbg, ", root is null??" );
	    }
	    break;

	case THREAD_CREATE:
	    an_int = va_arg( args, int );
	    (void) fprintf( stddbg, "Thread %d created.", an_int );
	    break;

	case THREAD_IDLE:
	    an_int = va_arg( args, int );
	    (void) fprintf( stddbg, "Thread idle. count = %d", an_int );
	    break;

	case THREAD_ENQUEUE_MSG:
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Message ENqueued.", from_entry->name().c_str() );
	    break;

	case THREAD_DEQUEUE_MSG:
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Message DEqueued.", from_entry->name().c_str() );
	    break;

	case THREAD_REAP:
	    ap = va_arg( args, Activity * );
	    if ( ap ) {
		(void) fprintf( stddbg, "Reaped %s.", ap->name().c_str() );
	    } else {
		(void) fprintf( stddbg, "Reaped unknown." );
	    }
	    break;

	case ACTIVITY_START:
	    ap = va_arg( args, Activity * );
	    (void) fprintf( stddbg, "Start activity %s.", ap->name().c_str() );
	    break;

	case ACTIVITY_EXECUTE:
	    ap = va_arg( args, Activity * );
	    (void) fprintf( stddbg, "Execute activity %s.", ap->name().c_str() );
	    break;

	case ACTIVITY_FORK:
	    ap    = va_arg( args, Activity * );
	    root  = va_arg( args, Instance * );
	    (void) fprintf( stddbg, "activity %s --FORK--", ap->name().c_str()  );
	    if ( root ) {
		(void) fprintf( stddbg, ", root is %s(%ld).", 
				root->_cp->name().c_str(), root->task_id() );
	    } else {
		(void) fprintf( stddbg, ", root is NULL." );
	    }
	    break;

	case ACTIVITY_JOIN:
	    ap    = va_arg( args, Activity * );
	    alp   = va_arg( args, ActivityList * );
	    (void) fprintf( stddbg, "activity %s --JOIN--, %ld", ap->name().c_str(), reinterpret_cast<size_t>(alp) );
	    break;

	case ENQUEUE_READER:
	    (void) fprintf( stddbg, "Enqueue Reader." );
	    break;

	case ENQUEUE_WRITER:
	    (void) fprintf( stddbg, "Enqueue Writer." );
	    break;

	case DEQUEUE_READER:
	    (void) fprintf( stddbg, "Dequeue Reader." );
	    break;

	case DEQUEUE_WRITER:
	    (void) fprintf( stddbg, "Dequeue Writer." );
	    break;

	default:
	    (void) fprintf( stddbg, "unkown event -- %d.", (int)event );
	    break;
	}

	if ( trace_driver ) {
	    fflush( stddbg );
	} else {
	    fprintf( stddbg, "\n" );
	} 

    } else if ( timeline_flag ) {
#if !BUG_289
	switch ( event ) {

	case TASK_CREATED:
	    (void) fprintf( stddbg, "%d %12.3f %s(%02ld) %#04x %#04lx\n",
			    event,
			    now() * 1000,
			    _cp->name().c_str(),
			    ps_myself,
			    0,
			    ps_myself );
	    break;

	case TASK_IS_WAITING:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %d\n", event, now() * 1000, ps_myself, va_arg( args, int ) );
	    break;

	case TASK_IS_READY:
	case TASK_IS_RUNNING:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %d\n", event, now() * 1000, ps_myself, va_arg( args, int ) );
	    break;

	case SYNC_INTERACTION_INITIATED:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %#04lx\n", event, now() * 1000, ps_myself, va_arg( args, long ) );
	    break;

	    /*	case SYNC_INTERACTION_FAILED: */
	case SYNC_INTERACTION_COMPLETED:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %#04lx\n", event, now() * 1000, ps_myself, reinterpret_cast<size_t>(va_arg( args, long *)) );
	    timeline_trace( TASK_IS_READY,   1 );
	    break;

	case SYNC_INTERACTION_ESTABLISHED:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %#04lx\n", event, now() * 1000, va_arg( args, long ), ps_myself );
	    break;
	}
#endif
    }
    va_end( args );
}


/*
 * All done.  Inform timeline.
 */

void
Instance::timeline_quit ()
{
#ifdef	NOTDEF
    (void) fprintf( stddbg, "%d %12.3f\n", QUIT_DISPLAY, now() * 1000 );
#endif
}

