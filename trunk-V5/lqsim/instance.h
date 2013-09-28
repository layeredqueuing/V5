/* -*- c++ -*-
 * $HeadURL$
 * Global vars for simulation.
 *
 * $Id$
 */

/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996.								*/
/* Nov 2005.								*/
/* June 2009.								*/
/************************************************************************/

#ifndef	INSTANCE_H
#define INSTANCE_H

#include "task.h"

class Entry;
class Activity;
class Message;


extern int client_init_count;		/* Semaphore for -C...*/

class  Instance
{
private:
    Instance( const Instance& );
    Instance& operator=( const Instance& );

protected:
    static void start( void * = 0 );
    static void random_shuffle_reply( vector<const Entry *>& array );
    static void random_shuffle_activity( Activity ** array, unsigned n );

protected:
    Instance( Task * a_task, const char * task_name, long task_id );

public:
    virtual ~Instance() = 0;
    
    long task_id() const { return _task_id; }
    long node_id() const { return _node_id; }
    long std_port() const;
    long reply_port() const { return _reply_port; }
    long start_port() const { return _start_port; }
    long thread_port() const { return _thread_port; }

    const char * name() const { return _cp->name(); }
    int priority() const { return _cp->priority(); }
    virtual const char * type_name() const = 0;
    virtual void run() = 0;

    void timeline_trace( const trace_events event, ... );
    
protected:
    void client_cycle( const double think_time );
    void server_cycle(  Entry *, Message * msg, bool reschedule );
    void wait_for_message( long& entry_id, Message **msg );
	void wait_for_message2( long& entry_id, Message **msg );
    void run_activities( Entry * ep, Activity * ap, bool reschedule );

    void timeline_quit();

private:
    virtual Instance * root_ptr() { return this; }

    void execute_activity( Entry * ep, Activity * ap, bool& reschedule );
    bool all_activities_done( const Activity * ap );
    Activity * next_activity( Entry * ep, Activity * ap_in, bool reschedule );
    void spawn_activities( const long entry_id, activity_list_t * fork_list );
    void wait_for_threads( activity_list_t * fork_list, double * thread_K_outOf_N_end_compute_time );
    void flush_threads();
    int thread_wait( double time_out, char ** msg, const bool flush, double * thread_end_compute_time );

    double compute ( Activity * ap, Activity * sp );
    void send_synchronous( const Entry *ep, tar_t * tp, Message * msg );
    void send_asynchronous( const Entry * ep, tar_t * tp, Message * msg );
    void do_forwarding ( Message * msg, const Entry * ep );

public:
    int r_e_execute;			/* For preemption (state).	*/
    int r_a_execute;			/* For preemption (state).	*/

protected:
    Task * _cp;				/* Pointer to class.	        */
    double hold_start_time;		/* For semaphores		*/

private:
    Message ** _entry;			/* Msg at local entry i		*/

    const long _task_id;		/* Parasol Task id.		*/
    const long _node_id;		/* Parasol Node id.		*/
    const long _reply_port;		/* reply port id		*/
    const long _thread_port;		/* Messages from threads.	*/
    long _start_port;			/* Messages to threads.		*/

    int active_threads;			/* Threads that are running.	*/
    int idle_threads;			/* Threads that are waiting.	*/
    double phase_start_time;		/* For activities		*/
    int _current_phase;			/* Phase 			*/
    double lastQuorumEndTime;
    bool * join_done;			/* */
};

/* 
 * Class for all tasks actually running on a processor and consuming time.
 */

class Real_Instance : public Instance
{
protected:
    Real_Instance( Task * cp, const char * task_name );

private:
    int create_task( Task * cp, const char * task_name );
};


/* 
 * Class for all tasks used to coordinate the simulation.  The DO NOT
 * consume time, rather they are used to generate traffic and
 * coordinate other "worker" tasks.
 */

class Virtual_Instance : public Instance
{
protected:
    Virtual_Instance( Task * cp, const char * task_name );

private:
    int create_task( Task * cp, const char * task_name );
};


class srn_client : public Real_Instance
{
    /* CLIENT 		*/
public:
    srn_client( Task * cp, const char * task_name  );

    virtual const char * type_name() const { return Task::type_strings[Task::CLIENT]; }
    void run();
};


class srn_server : public Real_Instance
{
    /* SERVER 		*/
public:
    srn_server( Task * cp, const char * task_name )
	: Real_Instance( cp, task_name ) {}

    virtual const char * type_name() const { return Task::type_strings[Task::SERVER]; }
    void run();
};


class srn_multiserver : public Virtual_Instance
{
    /* INFINITE_SERVER 	*/
public:
    srn_multiserver( Task * cp, const char * task_name, unsigned long max_workers )
	: Virtual_Instance( cp, task_name ), _max_workers(max_workers) {}

    virtual const char * type_name() const;
    void run();

private:
    const unsigned long _max_workers;
};


class srn_sync_server : public Real_Instance
{
    /* SYNCHRONIZATION_SERV */
public:
    srn_sync_server( Task * cp, const char * task_name )
	: Real_Instance( cp, task_name ) {}

    virtual const char * type_name() const { return Task::type_strings[Task::SYNCHRONIZATION_SERVER]; }
    void run();
};


class srn_semaphore : public Virtual_Instance
{
    /* MULTI_SERVER 	*/
public:
    srn_semaphore( Task * cp, const char * task_name )
	: Virtual_Instance( cp, task_name ) {}

    virtual const char * type_name() const { return Task::type_strings[Task::SEMAPHORE]; }
    void run();

private:
};


class srn_token : public Real_Instance
{
    /* SEMAPHORE		*/		/* BUG_164 */
public:
    srn_token( Task * cp, const char * task_name )
	: Real_Instance( cp, task_name ) {}

    const char * type_name() const { return Task::type_strings[Task::TOKEN]; }
    void run();
};


class srn_token_r : public Real_Instance
{
    /* SEMAPHORE		*/		/* BUG_164 */
public:
    srn_token_r( Task * cp, const char * task_name )
	: Real_Instance( cp, task_name ) {}

    const char * type_name() const { return Task::type_strings[Task::TOKEN_R]; }
    void run();
};


class srn_writer_token : public Real_Instance
{
    /* WORKER_TOKEN		*/		/* BUG_164 */
public:
    srn_writer_token( Task * cp, const char * task_name )
	: Real_Instance( cp, task_name ) {}

    const char * type_name() const { return Task::type_strings[Task::WRITER_TOKEN]; }
    void run();
};

class srn_rwlock_server : public Virtual_Instance
{
    /* RWLOCK 	*/
public:
    srn_rwlock_server( Task * cp, const char * task_name )
	: Virtual_Instance( cp, task_name ) {}

    virtual const char * type_name() const { return Task::type_strings[Task::RWLOCK_SERVER]; }
    void run();
};


class srn_open_arrivals : public Virtual_Instance
{
    /* OPEN_ARRIVAL_SOURCE 	*/
public:
    srn_open_arrivals( Task * cp, const char * task_name );

    virtual const char * type_name() const { return Task::type_strings[Task::OPEN_ARRIVAL_SOURCE]; }
    void run();
};


class srn_worker : public Real_Instance
{
    /* WORKER 		*/
public:
    srn_worker( Task * cp, const char * task_name )
	: Real_Instance( cp, task_name ) {}

    virtual const char * type_name() const { return Task::type_strings[Task::WORKER]; }
    void run();
};


class srn_thread : public Real_Instance
{
    /* THREAD		*/
public:
    srn_thread( Task * cp, const char * task_name, Instance * rip )
	: Real_Instance( cp, task_name ), _root_ptr(rip) {}

    virtual const char * type_name() const { return Task::type_strings[Task::THREAD]; }
    void run();

private:
    virtual Instance * root_ptr() { return _root_ptr; }

    Instance * _root_ptr;		/* Pointer to root instance.	*/
};


class srn_signal : public Virtual_Instance {
    /* SIGNAL		*/
public:
    srn_signal( Task * cp, const char * task_name )
	: Virtual_Instance( cp, task_name ) {}

    virtual const char * type_name() const { return Task::type_strings[Task::SIGNAL]; }
    void run();
};

extern Instance *object_tab[MAX_TASKS];	/* object table		*/
#endif
