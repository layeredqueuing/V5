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

/*
 * If this define is SET, then service times will include the first schedule to the processor.
 */

#define COUNT_FIRST_SCHEDULE	1

#include <iostream>
#include <iomanip>
#include <sstream>
#include <parasol.h>
#include "lqsim.h"
#include <assert.h>
#include <string.h>
#include <lqio/input.h>
#include <lqio/error.h>
#include "processor.h"
#include "group.h"
#include "errmsg.h"
#include "task.h"
#include "instance.h"
#include "activity.h"
#include "message.h"
#include "pragma.h"

/* see parainout.h */


int client_init_count = 0;		/* Semaphore for -C...*/
Instance * object_tab[MAX_TASKS];	/* object table		*/

Instance::Instance( Task * cp, const char * task_name, long task_id )
    : r_e_execute(-1),
      r_a_execute(-1),
      _cp(cp),
      hold_start_time(0.0),
      _entry(0),
      _task_id(task_id),
      _node_id(cp->node_id()),
      _reply_port( ps_allocate_port( task_name, task_id ) ),	/* reply port id	*/
      _thread_port( ps_allocate_port( cp->name(), task_id ) ),
      _start_port( cp->has_threads() ? ps_allocate_shared_port( _cp->name() ) : -1 ),
      active_threads(0),	/* no threads -- initialize */
      idle_threads(0),
      phase_start_time(0),
      _current_phase(1),
      lastQuorumEndTime(0),
      join_done(0)
{

    /* Allocate message space */
	
    _entry = new Message * [_cp->n_entries()];

    /* Allocate space for join counts. */
	
    if ( _cp->max_activities ) {
	join_done = static_cast<bool *>(calloc( _cp->max_activities, sizeof( bool ) ));
    } else {
	join_done = 0;
    }

    object_tab[task_id] = this;
    if ( task_id > total_tasks ) {
	total_tasks = task_id;
    }
}

Instance::~Instance() 
{
    delete [] _entry;
    free( join_done );
    if ( _start_port >= 0 ) {
	ps_release_shared_port( _start_port );		/* Buggy */
    }
}


void
Instance::start( void * )
{
    try {
	object_tab[ps_myself]->run();
    } 
    catch ( runtime_error& e ) {
	fprintf( stderr, "%s: task \"%s\": %s\n", io_vars.lq_toolname, object_tab[ps_myself]->name(), e.what() );
	io_vars.anError = true;
	ps_suspend( ps_myself );
    }
}


long
Instance::std_port() const
{
    return ps_std_port(task_id());
}


/*
 * Code for performing one server cycle.
 */

void
Instance::client_cycle( const double think_time )
{
    if ( think_time > 0.0 ) {
	const double delay = ps_exponential( think_time );
	ps_my_schedule_time = ps_now + delay;
	ps_sleep( delay );
    }

    /* Force a reschedule IFF we don't sleep */
    if ( _cp->n_entries() == 1 ) {
	server_cycle( _cp->_entry[0], 0, think_time == 0. );
    } else {
	server_cycle( _cp->_entry[ps_choice( _cp->n_entries() )], 0, think_time == 0. );
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
Instance::server_cycle ( Entry * ep, Message * msg, bool reschedule )
{
    double start_time 	= ps_my_schedule_time;
    double delta;
    unsigned p;
    Message * end_msg;

    if ( _cp->join_start_time == 0.0 ) {
	_cp->active += 1;
    }
    ps_record_stat2( _cp->r_util.raw, _cp->active, start_time );

    assert( ep->local_id < _cp->n_entries() );
    _entry[ep->local_id] = msg;

    /*
     * Start normal processing.
     */

    if ( msg && msg->reply_port != -1 ) {

	/* Synchronous message -- accumulate queueing time only */

	tar_t * tp = msg->source;
	delta = ps_my_schedule_time - msg->time_stamp;
	ps_record_stat( tp->r_delay.raw, delta );
	ps_record_stat( tp->r_delay_sqr.raw, square( delta ) );
	timeline_trace( SYNC_INTERACTION_ESTABLISHED, msg->client, msg->intermediate, ep, msg->time_stamp );
    }

    if ( ep->is_activity() ) {
	_current_phase = 1;
	phase_start_time = start_time;
	ep->active[1] += 1;

	ps_record_stat2( ep->phase[1].r_util.raw, ep->active[1], start_time ); 

   
	run_activities(  ep, ep->activity, reschedule );

	/* Flush threads */

	flush_threads();

	/* Funky stat recording. */

	delta = ps_now - phase_start_time;  
	p = _current_phase;
	ep->active[p] -= 1;
	ps_record_stat( ep->phase[p].r_util.raw, ep->active[p] );
	ps_record_stat( ep->phase[p].r_cycle.raw, delta );
	ps_record_stat( ep->phase[p].r_cycle_sqr.raw, square(delta) );
	if ( ep->phase[p]._hist_data ) {
	    ep->phase[p]._hist_data->insert(delta);
	}

	/*tomari, quorum: If we have not encountered a replying activity 
	  after all activities have been executed, then send a reply to the 
	  calling task or client. In a quorum, the last activity in the activity 
	  graph is not necessarily the last one will complete execution.*/
	if (pragma.quorum_delayed_calls() && msg && _current_phase==1 ) {
#if defined(debug_quorum_flag)
	    printf("\ndefault reply port at end of task: msg->reply_port=%d", 
		   msg->reply_port);
	    fflush(stdout);
#endif
	    ps_send( msg->reply_port , 0, (char *)msg, ps_my_std_port );
	}

    } else if ( ep->is_regular() ) {
	for ( p = 1; p <= _cp->max_phases; ++p ) {
	    _current_phase = p;
	    ep->active[p] += 1;
	    execute_activity( ep, (Activity *)&ep->phase[p], reschedule );
	    ep->active[p] -= 1;
	}

    } else {
	abort();
    }

    delta = ps_now - start_time;
    ps_record_stat( ep->r_cycle.raw, delta );		/* Entry cycle time.	*/
    ps_record_stat( _cp->r_cycle.raw, delta );		/* Task cycle time.	*/

    end_msg = _entry[ep->local_id];
    if ( end_msg ) {
	if ( end_msg->reply_port == -1 ) {

	    /* Async message -- acummulate queuing + service (M/G/m model) */

	    Message::free( end_msg );		/* Async message to dispose of.	*/
	} else if ( _cp->entry_status[ep->local_id].activity == 0 && _cp->isSynchServer() ) {
	    LQIO::solution_error( LQIO::ERR_REPLY_NOT_GENERATED, ep->name() );
	}
    }

    if ( _cp->join_start_time == 0.0 ) {
	_cp->active -= 1;
    }
    ps_record_stat( _cp->r_util.raw, _cp->active );

    ps_my_schedule_time = ps_now;		/* In case we don't block...	*/
}

/*----------------------------------------------------------------------*/
/*		     Task class implementations.			*/
/*----------------------------------------------------------------------*/

/*
 * These tasks do the actual "computation" for the simulation 
 */

Real_Instance::Real_Instance( Task * cp, const char * task_name ) 
    : Instance( cp, task_name, Real_Instance::create_task( cp, task_name ) )
{
}


int
Real_Instance::create_task( Task * cp, const char * task_name )
{
    if ( cp->group_id() != -1 ) {
	return ps_create_group( task_name, cp->node_id(), ANY_HOST, Instance::start, cp->priority(), cp->group_id() );
    } else {
	return ps_create( task_name, cp->node_id(), ANY_HOST, Instance::start, cp->priority() );
    }
}


/*
 * These tasks coordinate the execution of other tasks.  They are all
 * assigned their own node so that they do not interfere with each
 * other.
 */

Virtual_Instance::Virtual_Instance( Task * cp, const char * task_name ) 
    : Instance( cp, task_name, Virtual_Instance::create_task( cp, task_name ) )
{
}

int
Virtual_Instance::create_task( Task * cp, const char * task_name )
{

    /*
     * if the task is a cfs task and this task instance have no
     * group, we still create a fcfs node for it.  
     */

    const int local_node_id = ps_build_node( task_name, 1, 1.0, 0.0, 0, true );
    if ( local_node_id < 0 ) {
	LQIO::solution_error( ERR_CANNOT_CREATE_X, "processor for task", task_name );
	return 0;
    } else {
	return ps_create( task_name, local_node_id, 0, Instance::start, cp->priority() );
    }
}

/*----------------------------------------------------------------------*/
/*		     Task class implementations.			*/
/*----------------------------------------------------------------------*/

/*
 * Open arrival generator task.  They are exactly like client tasks
 * with the exception that they only have one entry.  Most of the work
 * is performed in store_open_arrival_rate in parainout.c.
 */

srn_open_arrivals::srn_open_arrivals( Task * cp, const char * a_name )
    : Virtual_Instance( cp, a_name ) 
{
    client_init_count += 1;		/* For -C auto init. 			*/
}



void
srn_open_arrivals::run (void)
{
    timeline_trace( TASK_CREATED );

    if ( Model::initial_loops() > 0 ) {
	for ( unsigned int i = 0; i < Model::initial_loops(); ++i ) {
	    server_cycle( _cp->_entry[0], 0, false );
	}
	client_init_count -= 1;
	if ( client_init_count == 0 ) {
	    /* Recompute blocking statistics */
	    Model::set_block_period( ps_now * 100 );
	    ps_awaken( Model::genesis_task_id() );
	}
    }

    /* ---------------------- Main loop --------------------------- */

    for ( ;; ) {				/* Start Client Cycle	*/
	server_cycle( _cp->_entry[0], 0, false );
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

srn_client::srn_client( Task * cp, const char * a_name  )
    : Real_Instance( cp, a_name ) 
{
    client_init_count += 1;		/* For -C auto init. 			*/
}



void
srn_client::run()
{
    timeline_trace( TASK_CREATED );

    const double think_time = dynamic_cast<Reference_Task *>(_cp)->think_time();

    if ( Model::initial_loops() > 0 ) {
	unsigned limit = static_cast<int>(static_cast<double>(Model::initial_loops()) / _cp->multiplicity());
	for ( unsigned i = 0; i < limit; ++i ) {
	    client_cycle( think_time );
	}
	client_init_count -= 1;
	if ( client_init_count == 0 ) {
	    /* Recompute blocking statistics */
	    Model::set_block_period( ps_now * 100 );
	    ps_awaken( Model::genesis_task_id() );
	}
    }

    /* ---------------------- Main loop --------------------------- */

    for ( ;; ) {				/* Start Client Cycle	*/
	client_cycle( think_time );
    }
}



/*
 * Server, or non-reference task.  Unique instances only.
 */

void
srn_server::run()
{
    Message * msg;
    long entry_id;

    timeline_trace( TASK_CREATED );

    /* ---------------------- Main loop --------------------------- */

    for ( ;; ) {
	double start_time = ps_now;

	wait_for_message( entry_id, &msg );

	/* If start_time == ps_now, then we continued right through */
	/* the receive, hence we will want to force a reschedule.   */ 

	server_cycle( Entry::entry_table[entry_id], msg, start_time == ps_now );
    }
}



/*
 * If we have an "unlimited" number of servers, then we're infinite.
 */

const char * 
srn_multiserver::type_name() const
{ 
    if ( _max_workers != static_cast<unsigned long>(~0) ) {
	return Task::type_strings[Task::MULTI_SERVER]; 
    } else {
	return Task::type_strings[Task::INFINITE_SERVER]; 
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
	Message * msg;		/* Message.			*/

	/* ---- Wait for request ---- */

	wait_for_message( entry_id, &msg );

	/* get worker */

	const double time_out = (n_workers < _max_workers) ? IMMEDIATE : NEVER;
	rc = ps_receive( _cp->worker_port(), time_out, &worker_id, &worker_time, (char **)&worker_msg, &worker_port );

	if ( rc == 0 ) {

	    /* Time out -- make a worker */

	    n_workers += 1;
	    Instance * task = new srn_worker( _cp, _cp->name() );
	    ps_resume( task->task_id() );
	    rc = ps_receive( _cp->worker_port(), NEVER, &worker_id, &worker_time,
			     (char **)&worker_msg, &worker_port );
	} else if ( rc == SYSERR ) {
	    abort();
	}

	timeline_trace( WORKER_DISPATCH );

	/* dispatch */

	ps_send( worker_port, entry_id, (char *)msg, msg->reply_port );
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

    const bool count_down = dynamic_cast<LQIO::DOM::RWLockTask *>(cp->getDOM()) || dynamic_cast<LQIO::DOM::SemaphoreTask *>(cp->getDOM())->getInitialState() == LQIO::DOM::SemaphoreTask::INITIALLY_FULL;
    for ( unsigned i = 0; i < cp->multiplicity(); ++i ) {
	Instance * task;
	if ( count_down ) {
	    task = new srn_token( cp, name() );
	} else {
	    task = new srn_token_r( cp, name() );
	}
	ps_resume( task->task_id() );
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
	Message * msg;		/* Message.			*/

	/* ---- Wait for request ---- */

	wait_for_message2( entry_id, &msg );

	/* get worker */

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
    Message * msg;		/* Message.			*/
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

	wait_for_message2( entry_id, &msg );
	ps_sleep(0);		/* Force context switch */

	/* ---- get worker (should be queued) --- */

	int rc = ps_receive( cp->signal_port(), IMMEDIATE, &worker_id, &worker_time, (char **)&worker_msg, &worker_port );
	if ( rc == 0 ) {
	    LQIO::solution_error( ERR_SIGNAL_NO_WAIT, cp->name() );
	    continue;
	} else if ( rc == SYSERR ) {
	    abort();
	}

	timeline_trace( WORKER_DISPATCH );

	/* dispatch */

	ps_send( worker_port, entry_id, (char *)msg, msg->reply_port );
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
	long reply_port;		/* reply port			*/
	Message * msg;		/* Time stamp info from client	*/
	double start_time = ps_now;

	if ( ps_send( _cp->worker_port(), -Task::WORKER, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;
	}

	/* If start_time == ps_now, then we continued right through */
	/* the receive, hence we will want to force a reschedule.   */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == ps_now );

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
	double start_time = ps_now;

	/* Send to the wait task first and do the wait processing. */

	if ( ps_send( cp->worker_port(), -Task::WORKER, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	if(cp->discipline()==SCHEDULE_RWLOCK){
	
	    if(time_stamp!=ps_now){
		const double delta = ps_now - time_stamp;
		ps_record_stat( dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_wait.raw, delta );
		ps_record_stat( dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_wait_sqr.raw, square( delta ) );
	    }

	}

	hold_start_time = ps_now;			/* Time we were "waited". */
	cp->hold_active += 1;
	ps_record_stat( cp->r_hold_util.raw, cp->hold_active );

	/* Send to the signal task next and do the signal processing. */

	if ( ps_send( cp->signal_port(), -Task::WORKER, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	/* Wait procesing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == ps_now );

	timeline_trace( TASK_IS_WAITING, 1 );

	/* Now wait for signal */

	ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );


	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	/* Signal processing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == ps_now );

	/* All done, wait for "wait" */

	cp->hold_active -= 1;
	const double delta = ps_now - hold_start_time;

	if(cp->discipline()==SCHEDULE_RWLOCK){

	    ps_record_stat( dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_hold.raw, delta );
	    ps_record_stat( dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_hold_util.raw, cp->hold_active );
	    ps_record_stat( dynamic_cast<ReadWriteLock_Task *>(_cp)->r_reader_hold_sqr.raw, square( delta ) );

	}else{
			
	    ps_record_stat( cp->r_hold_util.raw, cp->hold_active );
	    ps_record_stat( cp->r_hold.raw, delta );
	    ps_record_stat( cp->r_hold_sqr.raw, square( delta ) );
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

    hold_start_time = ps_now;		/* Time we were "waited". */
    cp->hold_active += 1;
    timeline_trace( TASK_CREATED );

    for ( ;; ) {
	double time_stamp;	/* time message sent.		*/
	long reply_port;	/* reply port			*/
	double start_time = ps_now;

	/* Send to the signal task first */

	if ( ps_send( cp->signal_port(), -Task::WORKER, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	cp->hold_active -= 1;
	double delta = ps_now - hold_start_time;
	ps_record_stat( cp->r_hold.raw, delta );
	ps_record_stat( cp->r_hold_sqr.raw, square( delta ) );
	ps_record_stat( cp->r_hold_util.raw, cp->hold_active );
	if ( cp->_hist_data ) {
	    cp->_hist_data->insert( delta );
	}

	/* Send to the queue (wait) task next and do the signal processing. */

	if ( ps_send( cp->worker_port(), -Task::WORKER, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	/* Signal procesing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == ps_now );

	timeline_trace( TASK_IS_WAITING, 1 );

	/* Now wait for wait */

	ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	cp->hold_active += 1;
	hold_start_time = ps_now;
	ps_record_stat( cp->r_hold_util.raw, cp->hold_active );

	/* Wait processing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == ps_now );

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
    Message * msg;
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
	//double start_time = ps_now;

	wait_for_message( entry_id, &msg );

	/* dispatch request to reader queue or worker token*/

	ep = Entry::entry_table[entry_id]; 

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
		  delta = ps_now - time_stamp;
		  ps_record_stat( cp->r_writer_wait.raw, delta );
		  ps_record_stat( cp->r_writer_wait_sqr.raw, square( delta ) );
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
		LQIO::solution_error( ERR_SIGNAL_NO_WAIT, cp->name() );
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

		/*	delta = ps_now - time_stamp;
			ps_record_stat( cp->r_writer_wait.raw, delta );
			ps_record_stat( cp->r_writer_wait_sqr.raw, square( delta ) );*/
	    }
	    else if (readers>0){
		/* some readers are waiting in the queue. */

		while (readers-activeReaders>0){
				
		    ps_receive(cp->readerQ_port(), IMMEDIATE, &entry_id1, &time_stamp, (char **)&msg1, &reply_port);
		    if ( ps_resend( cp->reader()->std_port(), entry_id1, time_stamp, (char *)msg1, reply_port ) != OK ) {	abort();	}
			
		    activeReaders++;
		    timeline_trace( DEQUEUE_READER, 1 );
		    /*
		      delta = ps_now - time_stamp;
		      ps_record_stat( cp->r_reader_wait.raw, delta );
		      ps_record_stat( cp->r_reader_wait_sqr.raw, square( delta ) );
		    */
		} 
	    }

	}
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
	double start_time = ps_now;

	/* Send to the wait task first and do the wait processing. */

	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );
	timeline_trace( TASK_IS_READY, 1 );

	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}
	
	if(time_stamp!=ps_now){
	    const double delta = ps_now - time_stamp;
	    ps_record_stat( cp->r_writer_wait.raw, delta );
	    ps_record_stat( cp->r_writer_wait_sqr.raw, square( delta ) );
	}
	
        hold_start_time = ps_now;			/* Time we were "waited". */
	cp->hold_active += 1;
	ps_record_stat( cp->r_writer_hold_util.raw, cp->hold_active );

	/* Send to the signal task next and do the signal processing. */

	if ( ps_send( cp->signal_port2(), -Task::WRITER_TOKEN, (char *)0, ps_my_std_port ) != OK ) {
	    abort();
	}

	/* writer procesing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == ps_now );

	timeline_trace( TASK_IS_WAITING, 1 );

	/* Now wait for unlocking signal */

	ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)&msg, &reply_port );

	timeline_trace( TASK_IS_READY, 1 );
	
	if ( msg ) {
	    msg->reply_port = reply_port;		/* reply to original client */
	}

	/* Signal processing */

	server_cycle( Entry::entry_table[entry_id], msg, start_time == ps_now );

	/* All done, wait for "wait" */

	cp->hold_active -= 1;
	ps_record_stat( cp->r_writer_hold_util.raw, cp->hold_active );

	const double delta = ps_now - hold_start_time;
	ps_record_stat( cp->r_writer_hold.raw, delta );
	ps_record_stat( cp->r_writer_hold_sqr.raw, square( delta ) );

	timeline_trace( WORKER_IDLE );
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
    Instance * pp 	= object_tab[ps_task_parent(ps_myself)];

    timeline_trace( TASK_CREATED );

    for ( ;; ) {
	double time_stamp;		/* Time stamp.		*/
	long entry_id;			/* entry id		*/
	long reply_port;			/* reply port		*/
	Entry * ep;

	if ( ps_send( pp->thread_port(), -Task::THREAD, (char *)ap, ps_my_std_port ) != OK ) {
	    abort();
	}

	timeline_trace( TASK_IS_WAITING, 1 );

	ps_receive_shared( pp->start_port(), NEVER, &entry_id, &time_stamp, (char **)&ap, &reply_port );

	timeline_trace( TASK_IS_READY, 1 );
	timeline_trace( THREAD_START, ap );

	ep = Entry::entry_table[entry_id];
	run_activities( ep, ap, 0 );

	timeline_trace( THREAD_STOP, ap, root_ptr() );
    }
}



/*
 * Main line port manipulation.  Stuff for blocking ports and priority queueing will
 * have to go here.
 */


/*----------------------------------------------------------------------*/
/*		       Processor Utilities.				*/
/*----------------------------------------------------------------------*/

void
Instance::wait_for_message( long& entry_id, Message **msg )
{
    double time_stamp;		/* time message sent.		*/
    long reply_port;		/* reply port			*/
    int rc;

    timeline_trace( TASK_IS_WAITING, 1 );
    ps_my_schedule_time = ps_now;		/* In case we don't block...	*/

    for ( ;; ) {
	unsigned e;
	if ( _cp->discipline() == SCHEDULE_HOL ) {
	    rc = ps_receive_priority( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)msg, &reply_port );
	} else {
	    rc = ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)msg, &reply_port );
	}

	if ( rc == SYSERR ) {
	    abort();
	}

	if ( *msg ) {
	    (*msg)->reply_port = reply_port;
	    (*msg)->time_stamp = time_stamp;
	}

	/* Is entry busy? !JOIN! */

	e = Entry::entry_table[entry_id]->local_id;
	if ( _cp->entry_status[e].activity ) {

	    /* queue message */

	    if ( _cp->entry_status[e].head ) {
		_cp->entry_status[e].tail->next = (* msg);
	    } else {
		_cp->entry_status[e].head = (* msg);
	    }
	    _cp->entry_status[e].tail = (* msg);
	    (* msg)->next = 0;
	} else {
	    break;
	}
    }

    timeline_trace( TASK_IS_READY, 1 );
}

void
Instance::wait_for_message2( long& entry_id, Message **msg )
{
    double time_stamp;		/* time message sent.		*/
    long reply_port;		/* reply port			*/
    int rc;

    timeline_trace( TASK_IS_WAITING, 1 );
    ps_my_schedule_time = ps_now;		/* In case we don't block...	*/

    for ( ;; ) {

	rc = ps_receive( ps_my_std_port, NEVER, &entry_id, &time_stamp, (char **)msg, &reply_port );

	if ( rc == SYSERR ) {
	    abort();
	}
	
	if ( *msg ) {
	    (*msg)->reply_port = reply_port;
	    (*msg)->time_stamp = time_stamp;
	}

	break;
    }

    timeline_trace( TASK_IS_READY, 1 );	
	
}

/*
 * Compute, taking into account the coefficient of variation.  ap is
 * the current activity.  sp is the associated entry.
 */

double
Instance::compute ( Activity * ap, Activity * pp )
{
    double time = 0.0;

    if ( ap->has_service_time() ) {

	time = ap->get_slice_time();
	timeline_trace( TASK_IS_COMPUTING, time );

	r_a_execute = ap->r_cpu_util.raw;

	ap->cpu_active += 1;
	ps_record_stat( ap->r_cpu_util.raw,  ap->cpu_active );	/* Phase P execution.	*/
	if ( ap != pp ) {
	    r_e_execute = pp->r_cpu_util.raw;
	    pp->cpu_active += 1;
	    ps_record_stat( pp->r_cpu_util.raw, pp->cpu_active );	/* CPU util by phase */
	}

	(*_cp->compute_func)( time );

	ap->cpu_active -= 1;
	ps_record_stat( ap->r_cpu_util.raw,  pp->cpu_active );	/* Phase P execution.	*/
	if ( ap != pp ) {
	    pp->cpu_active -= 1;
	    ps_record_stat( pp->r_cpu_util.raw, pp->cpu_active );
	}
	r_a_execute = -1;
	r_e_execute = -1;

    }

    if ( _cp->compute_func == ps_sleep || !ap->has_service_time() ) {
	ps_my_end_compute_time = ps_now;	/* Won't call the end_compute handler, ergo, set here */
    }
    ps_my_schedule_time = ps_now;
    ps_record_stat( ap->r_service.raw, time );

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
Instance::do_forwarding ( Message * msg, const Entry * ep )
{
    if ( !msg ) {
	return;				/* No operation.	*/

    } else if ( msg->reply_port == -1 ) {

	Message::free( msg );		/* Async message.  Free it. */

    } else {
	const Targets *fp = &ep->fwd;	/* forwarding pointer	*/
	unsigned int i = 0;		/* loop index		*/
	unsigned int j = 0;		/* loop index (det ph.)	*/
	tar_t * tp = fp->entry_to_send_to( i, j );

	if ( !tp ) {

	    /* Reply to sender	*/

	    long reply_port  = msg->reply_port;	/* Local copy	*/

	    timeline_trace( SYNC_INTERACTION_REPLIES, ep, msg->client );
	    ps_send( reply_port, 0, (char *)msg->init( ep, 0 ), ps_my_std_port );

	} else {

	    timeline_trace( SYNC_INTERACTION_FORWARDED, ep, msg->client, tp->entry );

	    msg->time_stamp   = ps_now; 		/* Tag send time.	*/
	    msg->intermediate = ep;
	    msg->source       = tp;

	    ps_send( fp->target[i].entry->port,	/* Forward request.	*/
		     fp->target[i].entry->entry_id, (char *)msg, msg->reply_port );
	}
    }
}


/*
 * Randomly shuffle items.
 */

void
Instance::random_shuffle_reply( vector<const Entry *>& array )
{
    const unsigned n = array.size();
    for ( unsigned i = n; i >= 1; --i ) {
	const unsigned k = static_cast<unsigned>(drand48() * i);
	if ( i-1 != k ) {
	    const Entry * temp = array[k];
	    array[k] = array[i-1];
	    array[i-1] = temp;
	}
    }
}


/*
 * Randomly shuffle items.
 */

void
Instance::random_shuffle_activity( Activity ** array, unsigned n )
{
    unsigned i;
    for ( i = n; i >= 1; --i ) {
	unsigned k = static_cast<unsigned>(drand48() * i);
	if ( i-1 != k ) {
	    Activity * temp = array[k];
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
    double start_time = ps_my_schedule_time;
    double slices = 0.0;
    int phase = _current_phase;
    int count = ap->tinfo.size() > 0 || ap->has_service_time() || ap->think_time(); /* !!! warning !!! */
    double delta;

    timeline_trace( ACTIVITY_START, ap );

    ap->active += count;
    ps_record_stat2( ap->r_util.raw, ap->active, start_time );		/* Activity utilization.*/

    /*
     * Delay for "think time".  
     */

    if ( ap->think_time() > 0.0 ) {
	double think_time = ps_exponential( ap->think_time() );
	ps_my_schedule_time = ps_now + think_time;
	ps_sleep( think_time );
    } 

    /*
     * Now do service.
     */

    if ( ap->tinfo.size() > 0 || ap->has_service_time() ) {
	double sends = 0.0;
	unsigned int i = 0;		/* loop index			*/
	unsigned int j = 0;		/* loop index (det ph.)		*/

	if ( reschedule ) {
	    Processor::reschedule( this );
	}

	delta = ps_now - ps_my_schedule_time;
	ps_record_stat( ap->r_proc_delay.raw, delta );			/* Delay for schedul.	*/
	ps_record_stat( ap->r_proc_delay_sqr.raw, delta * delta );	/* Delay for schedul.	*/
	ap->pt_prewaiting = delta;			/*Added by Tao*/

	if ( ap != &ep->phase[phase] ) {
	    ps_record_stat( ep->phase[phase].r_proc_delay.raw, delta );
	    ps_record_stat( ep->phase[phase].r_proc_delay_sqr.raw, delta * delta );
	    ep->phase[phase].pt_prewaiting = delta;	/*Added by Tao*/
	}

	timeline_trace( ACTIVITY_EXECUTE, ap );

	for ( ;; ) {

	    compute( ap, &ep->phase[phase] );
	    slices += 1.0;

	    tar_t * tp = ap->tinfo.entry_to_send_to( i, j );

	    if ( !tp ) break;

	    sends += 1.0;
	    if ( tp->reply() ) {
		tp->send_synchronous( ep, _cp->priority(), reply_port() );

		delta = ps_now - ps_my_schedule_time;

		ps_add_stat( ap->r_proc_delay.raw, delta );
		ps_add_stat( ap->r_proc_delay_sqr.raw, (delta + ap->pt_prewaiting) * (delta + ap->pt_prewaiting) -  ap->pt_prewaiting * ap->pt_prewaiting);
		ap->pt_prewaiting += delta;
		if ( ap != &ep->phase[phase] ) {
		    ps_add_stat( ep->phase[phase].r_proc_delay.raw, delta );
		    ps_add_stat( ep->phase[phase].r_proc_delay_sqr.raw, (delta + ep->phase[phase].pt_prewaiting) * (delta + ep->phase[phase].pt_prewaiting) -  ep->phase[phase].pt_prewaiting * ep->phase[phase].pt_prewaiting);
		    ep->phase[phase].pt_prewaiting += delta;
		}
		/*End here*/

	    } else {
		tp->send_asynchronous( ep, _cp->priority() );
		if ( pragma.reschedule_on_async_send() ) {
		    Processor::reschedule( this );
		}
	    }
	} /* end for loop */

	ps_record_stat( ap->r_sends.raw, sends );
	ps_record_stat( ap->r_slices.raw, slices );
	if ( ap != &ep->phase[phase] ) {
	    ps_record_stat( ep->phase[phase].r_sends.raw, sends );
	    ps_record_stat( ep->phase[phase].r_slices.raw, slices );
	}

	reschedule = true;
    }

    if ( count ) {
	delta = ps_now - start_time;				/* Bug 232 */

	ps_record_stat( ap->r_cycle.raw, delta );		/* Entry cycle time.	*/
	ps_record_stat( ap->r_cycle_sqr.raw, square(delta) );	/* Entry cycle time.	*/
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
	    assert( reply_ep->local_id < _cp->n_entries() );
	    Message * msg = root_ptr()->_entry[reply_ep->local_id];
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

		if ( msg->reply_port != -1 ) {
		    Instance * dest_ip = object_tab[ps_owner(msg->reply_port)];
		    if ( dest_ip && node_id() == dest_ip->node_id() && ps_random >= 0.5 ) {
		        reschedule = false;
		    }
		}

		/* ---------- */

		do_forwarding( msg, reply_ep );	/* Clears msg! */

		root_ptr()->_entry[reply_ep->local_id] = 0;

		/* Possible phase change.  Record phase cycle time and utilization */

		if ( phase == 1 && ap != &reply_ep->phase[phase] && ep == reply_ep ) {
		    assert( ep->active[1] );
		    delta = ps_now - root_ptr()->phase_start_time;
		    ps_record_stat( ep->phase[1].r_cycle.raw, delta );
		    ps_record_stat( ep->phase[1].r_cycle_sqr.raw, square(delta) );
		    if ( ep->phase[1]._hist_data ) {
			ep->phase[1]._hist_data->insert( delta );
		    }

		    ep->active[1] -= 1;
		    ps_record_stat( ep->phase[1].r_util.raw, ep->active[1] );
		    ep->active[2] += 1;
		    ps_record_stat( ep->phase[2].r_util.raw, ep->active[2] );

		    root_ptr()->phase_start_time = ps_now;
		    root_ptr()->_current_phase = 2;
		    phase = 2;
		}

	    } else {
		LQIO::solution_error( ERR_REPLY_NOT_FOUND, ap->name(), reply_ep->name() );
	    }

	}
    }

    ap->active -= count;
    ps_record_stat( ap->r_util.raw, ap->active );

    /*Add the preemption time to the waiting time if available. Tao*/

    if (ps_preempted_time (task_id()) > 0.0) {   
	ps_add_stat( ap->r_proc_delay.raw, ps_preempted_time (task_id()) );
	ps_add_stat( ap->r_proc_delay_sqr.raw, (ps_preempted_time (task_id()) + ap->pt_prewaiting) * (ps_preempted_time (task_id()) + ap->pt_prewaiting) -  ap->pt_prewaiting * ap->pt_prewaiting);

#if 0
	if ( ap != &ep->phase[phase] ) {
	    ps_add_stat( ep->phase[phase].r_proc_delay.raw, ps_preempted_time (task_id()) );
	    ps_add_stat( ep->phase[phase].r_proc_delay_sqr.raw, (ps_preempted_time (task_id()) + ep->phase[phase].pt_prewaiting) * (ps_preempted_time (task_id()) + ep->phase[phase].pt_prewaiting) -  ep->phase[phase].pt_prewaiting * ep->phase[phase].pt_prewaiting);

	}
#endif

	ps_task_ptr(task_id())->pt_tag = 0;
	ps_task_ptr(task_id())->pt_sum = 0.0;
	ps_task_ptr(task_id())->pt_last = 0.0;

    }
}



/*
 * locate next node in the activity list.
 * May have to do joins and all that good stuff.
 */

Activity * 
Instance::next_activity( Entry * ep, Activity * ap_in, bool reschedule )
{
    Activity * ap_out = 0;
    activity_list_t * fork_list = 0;

    if ( ap_in->_output != 0 ) {

	/*
	 * If there is an input list and that input list is of
	 * type join then I have to do some work....
	 */

	activity_list_t * join_list = ap_in->_output;

	switch ( join_list->type ) {
	case ACT_AND_JOIN_LIST:
	    if ( _cp->isSynchServer() && join_list->u.join.join_type == JOIN_SYNCHRONIZATION ) {
		if ( root_ptr()->all_activities_done( ap_in ) ) {
		    unsigned e;
		    double delta = ps_now - _cp->join_start_time;
		    ps_record_stat( join_list->u.join.r_join.raw, delta );
		    ps_record_stat( join_list->u.join.r_join_sqr.raw, square( delta ) );
					  
		    _cp->join_start_time = 0.0;

		    /* Release all entries blocked */
					
		    for ( e = 0; e < _cp->n_entries(); ++e ) {
			if ( _cp->entry_status[e].activity == join_list ) {

			    /* Re-enqueue messages waiting !JOIN! */

			    while ( _cp->entry_status[e].head ) {
				Message * msg = _cp->entry_status[e].head;
				_cp->entry_status[e].head = msg->next;
				msg->next = 0;

				ps_send( ps_my_std_port, _cp->_entry[e]->entry_id, (char *)msg, msg->reply_port );
			    }
			    _cp->entry_status[e].tail = 0;

			    /* Mark entry as ready to accept messages */

			    _cp->entry_status[e].activity = 0;
			}
		    }
		} else {
		    /* Mark entry busy */
		    _cp->entry_status[ep->local_id].activity = join_list;
		    _cp->join_start_time = ps_now;
		    return 0;	/* Do not execute output list. */
		}
	    } else {
		/* tomari:quorum,  Histogram binning needs to be done here. */
		return 0;	/* Thread complete. */
	    }
	    break;

	case ACT_JOIN_LIST:
	case ACT_OR_JOIN_LIST:
	    break;

	default:
	    abort();
	}
	fork_list = join_list->u.join.next;
    }

again_1:
    if ( fork_list ) {

	/*
	 * Now I can do the fork_list list.  And forks are the biggest headache.
	 */

	unsigned i;
	double fork_start;
	double exit_value;
	double sum;
	activity_list_t * join_list = 0;
	double thread_K_outOf_N_end_compute_time = 0;

	switch ( fork_list->type ) {
	case ACT_AND_FORK_LIST:

 	    fork_start = ps_now;

	    /* launch threads */

	    if ( fork_list->na == 0 ) {
		abort();
	    }

	    timeline_trace( ACTIVITY_FORK, fork_list->list[0], this );
	    random_shuffle_activity( fork_list->list, fork_list->na );
				  
	    spawn_activities( ep->entry_id, fork_list );

	    /* Wait for all threads to complete, then we're done. */

	    wait_for_threads( fork_list, &thread_K_outOf_N_end_compute_time );

	    lastQuorumEndTime = thread_K_outOf_N_end_compute_time;

	    join_list = fork_list->u.fork.join;
	    if ( join_list ) {
		const double delta = thread_K_outOf_N_end_compute_time - fork_start; 

		ps_record_stat( join_list->u.join.r_join.raw, delta );
		ps_record_stat( join_list->u.join.r_join_sqr.raw, square( delta ) );
		if ( join_list->u.join._hist_data ) {
		    join_list->u.join._hist_data->insert( delta );
		}
		fork_list = join_list->u.join.next;
		timeline_trace( ACTIVITY_JOIN, fork_list->list[0], fork_list );

	    } else {
		fork_list = 0;
	    }
	    ps_my_end_compute_time = ps_now;				/* BUG 321 */
	    ps_my_schedule_time    = ps_now;				/* BUG 259 */
	    goto again_1;

	case ACT_OR_FORK_LIST:
	    if ( fork_list->na == 0 ) {
		abort();
	    }
	    exit_value = ps_random;
	    sum = 0.0;
	    for ( i = 0; i < fork_list->na; ++i ) {
		sum += fork_list->u.fork.prob[i];
		if ( sum > exit_value ) break;
	    }
	    ap_out = fork_list->list[i];
	    break;

	case ACT_FORK_LIST:
	    if ( fork_list->na == 1 ) {
		ap_out = fork_list->list[0];
	    } else if ( fork_list->na != 0 ) {
		abort();
	    }
	    break;

	case ACT_LOOP_LIST:
	again_2:
	    ps_my_end_compute_time = ps_now;	/* BUG 321 */
	    exit_value = ps_random * (fork_list->u.loop.total + 1.0);
	    sum = 0;
	    for ( i = 0; i < fork_list->na; ++i ) {
		sum += fork_list->u.loop.count[i];
		if ( sum > exit_value ) {
		    run_activities( ep, fork_list->list[i], reschedule );
		    goto again_2;
		}
	    }
	    ap_out = fork_list->u.loop.endlist;
	    break;


	default:
	    abort();
	    ap_out = 0;
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
Instance::spawn_activities( const long entry_id, activity_list_t * fork_list )
{
    ps_my_schedule_time = ps_now;		/* In case we don't block...	*/

    for ( unsigned i = 0; i < fork_list->na; ++i ) {
	char * msg = 0;
	Activity * ap = fork_list->list[i];

	/* Request for service arrives. */

	assert( idle_threads >= 0 );
	assert( ap );

	/* Reap any pending threads if possible */

	while ( thread_wait( IMMEDIATE, (char **)&msg, false, NULL ) ) {
	    timeline_trace( THREAD_REAP, msg );
	}

	/*  Think about if there is overflow then more and more theads will be created. */

	if ( idle_threads == 0 ) {

	    /* No worker available -- Allocate a new task... */

	    char * nextThreadName = (char *)malloc( strlen( ap->name() ) + 32 );
	    sprintf( nextThreadName, "%s_thread_%d",ap->name(),  active_threads + 1);
	    Instance * task = new srn_thread( _cp, nextThreadName, root_ptr() );
	    const int id = task->task_id();
	    timeline_trace( THREAD_CREATE, id );
	    ps_resume( id  );
	    active_threads += 1;
	    thread_wait( NEVER, &msg, false, NULL );

	}

	active_threads += 1;		 
	idle_threads   -= 1;

	if ( ps_send( start_port(), entry_id, (char *)ap, std_port() ) != OK ) {
	    abort();
	}
    }
}



/*
 * Wait for all forks to finish for the case of AND-Join. In the case of Quorum, just wait for K 
 * out of N threads.
 */

void
Instance::flush_threads()
{
    while ( active_threads > 0  ) {
	Activity * ap;

	thread_wait( NEVER, (char **)&ap, true, NULL );
    }
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
	    ps_record_stat( replyMsg->r_afterQuorumThreadWait.raw, *thread_end_compute_time - lastQuorumEndTime);
	}
    }

    return rc;
}


/*
 * Locate all activities that are in this join set.  If all
 * done, then clear and return true, otherwise, return false.
 */

bool
Instance::all_activities_done( const Activity * ap )
{
    activity_list_t * join_list = ap->_output;
    unsigned i = 0;

    join_done[ap->index] = true;

    for ( i = 0; i < join_list->na; ++i ) {
	if ( !join_done[join_list->list[i]->index] ) {
	    return false;
	}
    }

    /* All activities tagged -- zap and return */

    for ( i = 0; i < join_list->na; ++i ) {
	join_done[join_list->list[i]->index] = false;	/* !!! */
    }
    return true;
}



void
Instance::wait_for_threads( activity_list_t * fork_list, double * thread_K_outOf_N_end_compute_time )
{
    Activity * ap;			/* Time stamp info from client	*/
    int count = fork_list->u.fork.visit_count;
    activity_list_t * join_list = fork_list->u.fork.join;
    const int N = count;
    const int K = (join_list && join_list->u.join.quorumCount > 0) ? join_list->u.join.quorumCount : count;

    if ( count == 0 ) return;

    timeline_trace( TASK_IS_WAITING, 1 );

    ps_my_schedule_time = ps_now;	/* In case we don't block...	*/

    while ( count > (N - K) ) {

	thread_wait( NEVER, (char **)&ap, false, thread_K_outOf_N_end_compute_time );

	/* Only acknowledge active threads */
 
	for ( unsigned i = 0; i < fork_list->na; ++i ) {
	    if ( fork_list->u.fork.visit[i] && fork_list->list[i] == ap ) {
		count -= 1;
		break;
	    }
	}
    }

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
	activity_list_t * alp;
	int an_int;
	Instance * root;

	if ( ((1 << event) & watched_events ) == 0 ) {
	    return;
	}

	if ( trace_driver ) {
	    (void) fprintf( stddbg, "\nTime* %8g T %s(%ld): ", ps_now, _cp->name(), task_id() );
	} else {
	    (void) fprintf( stddbg, "%8g %8s %8s(%2ld): ", ps_now, type_name(), _cp->name(), task_id() );
	}

	switch ( event ) {
	case TASK_CREATED:
	    (void) fprintf( stddbg, "%s created.", type_name() );
	    break;

	case ASYNC_INTERACTION_INITIATED:
	    from_entry = va_arg( args, Entry * );
	    to_entry   = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Sending SNR to task %s, entry %s",
			    from_entry->name(),
			    to_entry->task()->name(),
			    to_entry->name() );
	    break;

	case SYNC_INTERACTION_INITIATED:
	    from_entry = va_arg( args, Entry * );
	    to_entry   = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Sending RNV to task %s, entry %s",
			    from_entry->name(),
			    to_entry->task()->name(),
			    to_entry->name() );
	    break;

	case SYNC_INTERACTION_ESTABLISHED:
	    from_entry = va_arg( args, Entry * );
	    int_entry  = va_arg( args, Entry * );
	    to_entry   = va_arg( args, Entry * );
	    time       = va_arg( args, double );
	    (void) fprintf( stddbg, "Entry %s -- Received msg from task %s, entry %s (t=%g)",
			    to_entry->name(),
			    from_entry->task()->name(), 
			    from_entry->name(),
			    time );
	    if ( int_entry ) {
		(void) fprintf( stddbg, ", forwarded via task %s, entry %s",
				int_entry->task()->name(), 
				int_entry->name() );
	    }
	    break;

	case SYNC_INTERACTION_REPLIES:
	    to_entry   = va_arg( args, Entry * );
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Reply sent to task %s, entry %s",
			    to_entry->name(),
			    from_entry->task()->name(), 
			    from_entry->name() );
	    break;

	case SYNC_INTERACTION_COMPLETED:
	    to_entry   = va_arg( args, Entry * );
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Reply received from task %s, entry %s",
			    to_entry->name(),
			    from_entry->task()->name(),
			    from_entry->name() );

	    break;

	case SYNC_INTERACTION_FORWARDED:
	    int_entry  = va_arg( args, Entry * );
	    from_entry = va_arg( args, Entry * );
	    to_entry   = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Message from task %s, entry %s forwarded to task %s, entry %s",
			    int_entry->name(),
			    from_entry->task()->name(), 
			    from_entry->name(),
			    to_entry->task()->name(),
			    to_entry->name() );
	    break;


	case SYNC_INTERACTION_ABORTED:
	    to_entry   = va_arg( args, Entry * );
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Reply tossed for task %s, entry %s",
			    to_entry->name(),
			    from_entry->task()->name(), 
			    from_entry->name() );
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
	    (void) fprintf( stddbg, "Thread start, activity %s.", ap->name() );
	    break;

	case THREAD_STOP:
	    ap   = va_arg( args, Activity * );
	    root = va_arg( args, Instance * );
	    (void) fprintf( stddbg, "Thread stop, activity %s", ap->name() );
	    if ( root ) {
		(void) fprintf( stddbg, ", root is %s(%ld).", 
				root->_cp->name(), root->task_id() );
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
	    (void) fprintf( stddbg, "Entry %s -- Message ENqueued.", from_entry->name() );
	    break;

	case THREAD_DEQUEUE_MSG:
	    from_entry = va_arg( args, Entry * );
	    (void) fprintf( stddbg, "Entry %s -- Message DEqueued.", from_entry->name() );
	    break;

	case THREAD_REAP:
	    ap = va_arg( args, Activity * );
	    if ( ap ) {
		(void) fprintf( stddbg, "Reaped %s.", ap->name() );
	    } else {
		(void) fprintf( stddbg, "Reaped unknown." );
	    }
	    break;

	case ACTIVITY_START:
	    ap = va_arg( args, Activity * );
	    (void) fprintf( stddbg, "Start activity %s.", ap->name() );
	    break;

	case ACTIVITY_EXECUTE:
	    ap = va_arg( args, Activity * );
	    (void) fprintf( stddbg, "Execute activity %s.", ap->name() );
	    break;

	case ACTIVITY_FORK:
	    ap    = va_arg( args, Activity * );
	    root  = va_arg( args, Instance * );
	    (void) fprintf( stddbg, "activity %s --FORK--", ap->name()  );
	    if ( root ) {
		(void) fprintf( stddbg, ", root is %s(%ld).", 
				root->_cp->name(), root->task_id() );
	    } else {
		(void) fprintf( stddbg, ", root is NULL." );
	    }
	    break;

	case ACTIVITY_JOIN:
	    ap    = va_arg( args, Activity * );
	    alp   = va_arg( args, activity_list_t * );
	    (void) fprintf( stddbg, "activity %s --JOIN--, %ld", ap->name(), (unsigned long)alp );
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
	switch ( event ) {

	case TASK_CREATED:
	    (void) fprintf( stddbg, "%d %12.3f %s(%02ld) %#04x %#04lx\n",
			    event,
			    ps_now * 1000,
			    _cp->name(),
			    ps_myself,
			    0,
			    ps_myself );
	    break;

	case TASK_IS_WAITING:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %d\n", event, ps_now * 1000, ps_myself, va_arg( args, int ) );
	    break;

	case TASK_IS_READY:
	case TASK_IS_RUNNING:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %d\n", event, ps_now * 1000, ps_myself, va_arg( args, int ) );
	    break;

	case SYNC_INTERACTION_INITIATED:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %#04lx\n", event, ps_now * 1000, ps_myself, va_arg( args, long ) );
	    break;

	    /*	case SYNC_INTERACTION_FAILED: */
	case SYNC_INTERACTION_COMPLETED:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %#04lx\n", event, ps_now * 1000, ps_myself, reinterpret_cast<long>(va_arg( args, long *)) );
	    timeline_trace( TASK_IS_READY,   1 );
	    break;

	case SYNC_INTERACTION_ESTABLISHED:
	    (void) fprintf( stddbg, "%d %12.3f %#04lx %#04lx\n", event, ps_now * 1000, va_arg( args, long ), ps_myself );
	    break;
	}
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
    (void) fprintf( stddbg, "%d %12.3f\n", QUIT_DISPLAY, ps_now * 1000 );
#endif
}

