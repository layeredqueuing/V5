/* -*- c++ -*-
 * Logic executed for task behaviour in simulation.
 * 
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * May 1996.
 * Nov 2005.
 * June 2009.
 * December 2024.
 * ------------------------------------------------------------------------
 * $Id: instance.h 17510 2024-12-04 16:03:30Z greg $
 * ------------------------------------------------------------------------
 */

#ifndef	INSTANCE_H
#define INSTANCE_H

#include <vector>
#include <thread>
#include "lqsim.h"
#include "task.h"

class Entry;
class Activity;
class Message;
class Random;

extern volatile int client_init_count;		/* Semaphore for -C...*/

namespace Instance
{
    class Instance
    {
    private:
	Instance( const Instance& ) = delete;
	Instance& operator=( const Instance& ) = delete;

    protected:
#if HAVE_PARASOL
	static void start( void * );
#endif
	static void random_shuffle_reply( std::vector<const Entry *>& array );
	static void random_shuffle_activity( std::vector<Activity *>& array );

    protected:

    public:
	Instance( Task * task, const std::string& task_name, long task_id );
	virtual ~Instance() = 0;
    
#if HAVE_PARASOL
	long task_id() const { return _task_id; }
	long node_id() const { return _node_id; }
	long std_port() const { return _std_port; }
	long reply_port() const { return _reply_port; }
	long start_port() const { return _start_port; }
	long thread_port() const { return _thread_port; }
#else
	bool set_thread( std::thread* thread ) { _thread = thread; return thread != nullptr; }
	std::thread * get_thread() { return _thread; }
	std::thread::id task_id() const { return _thread->get_id(); }
#endif
	Processor * processor() const { return _processor; }

	const std::string& name() const { return _cp->name(); }
	int priority() const { return _cp->priority(); }
	virtual const std::string& type_name() const = 0;
	virtual void run() = 0;
	virtual int parentPort() const{ return -1;}
	virtual void setParent(int parentPort) {}
	void timeline_trace( const trace_events event, ... );
    
    protected:
	void client_cycle( Random * );
	void server_cycle(  Entry *, Message * msg, bool reschedule );
	Message * wait_for_message( long& entry_id );
	Message * wait_for_message2( long& entry_id );
	void run_activities( Entry * ep, Activity * ap, bool reschedule );

	void timeline_quit();

    private:
	virtual Instance * root_ptr() { return this; }

	void execute_activity( Entry * ep, Activity * ap, bool& reschedule );
	bool all_activities_done( const Activity * ap );
	Activity * next_activity( Entry * ep, Activity * ap_in, bool reschedule );
	void spawn_activities( const long entry_id, ActivityList * fork_list );
	void wait_for_threads( AndForkActivityList * fork_list, double * thread_K_outOf_N_end_compute_time );
	void flush_threads();
	int thread_wait( double time_out, char ** msg, const bool flush, double * thread_end_compute_time );

	void compute( Activity * ap, Activity * sp );
	void do_forwarding( Message * msg, const Entry * ep );

    public:
	Result * r_e_execute;		/* For preemption (state).	*/
	Result * r_a_execute;		/* For preemption (state).	*/

    protected:
	Task * _cp;			/* Pointer to class.	        */
	double _hold_start_time;	/* For semaphores		*/

    private:
	Processor * _processor;
	std::vector<Message *> _entry;	/* Msg at local entry i		*/

#if HAVE_PARASOL
	const long _task_id;		/* Parasol Task id.		*/
	const long _node_id;		/* Parasol Node id.		*/
	const long _std_port;		/* Main port.			*/
	const long _reply_port;		/* reply port id		*/
	const long _thread_port;	/* Messages from threads.	*/
	long _start_port;		/* Messages to threads.		*/
#else
	std::thread* _thread;
#endif

	int active_threads;		/* Threads that are running.	*/
	int idle_threads;		/* Threads that are waiting.	*/
	double _phase_start_time;	/* For activities		*/
	int _current_phase;		/* Phase 			*/
	double lastQuorumEndTime;
	std::vector<bool> _join_done;	/* */
    };

/* 
 * Class for all tasks actually running on a processor and consuming time.
 */

    class Real_Instance : public Instance
    {
    protected:
	Real_Instance( Task * cp, const std::string& task_name );

    private:
	int create_task( Task * cp, const std::string& task_name );
    };


/* 
 * Class for all tasks used to coordinate the simulation.  The DO NOT
 * consume time, rather they are used to generate traffic and
 * coordinate other "worker" tasks.
 */

    class Virtual_Instance : public Instance
    {
    protected:
	Virtual_Instance( Task * cp, const std::string& task_name );

    private:
	int create_task( Task * cp, const std::string& task_name );
    };


    class Client : public Real_Instance
    {
	/* CLIENT 		*/
    public:
	Client( Task * cp, const std::string& task_name  );
	~Client();
    
	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::CLIENT); }
	void run();

    private:
	Random * _think_time;		/* Distribution generator	*/
    };


    class Server : public Real_Instance
    {
	/* SERVER 		*/
    public:
	Server( Task * cp, const std::string& task_name )
	    : Real_Instance( cp, task_name ) {_parent_port=-1;}

	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::SERVER); }
	void run();
	int parentPort() const{ return _parent_port;}
	void setParent(int parentPort) {_parent_port=parentPort; }
    private:
	int _parent_port;
    };


#if HAVE_PARASOL
    class Multiserver : public Virtual_Instance
    {
	/* INFINITE_SERVER 	*/
    public:
	Multiserver( Task * cp, const std::string& task_name, unsigned long max_workers )
	    : Virtual_Instance( cp, task_name ), _max_workers(max_workers) {_parent_port=-1;}

	virtual const std::string& type_name() const;
	void run();
	int parentPort() const{ return _parent_port;}
	void setParent(int parentPort) {_parent_port=parentPort; }
    private:
	int _parent_port;
	const unsigned long _max_workers;
    };
#endif


    class Sync_Server : public Real_Instance
    {
	/* SYNCHRONIZATION_SERV */
    public:
	Sync_Server( Task * cp, const std::string& task_name )
	    : Real_Instance( cp, task_name ) {}

	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::SYNCHRONIZATION_SERVER); }
	void run();
    };


    class Semaphore : public Virtual_Instance
    {
	/* MULTI_SERVER 	*/
    public:
	Semaphore( Task * cp, const std::string& task_name )
	    : Virtual_Instance( cp, task_name ) {}

	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::SEMAPHORE); }
	void run();

    private:
    };


    class Token : public Real_Instance
    {
	/* SEMAPHORE		*/		/* BUG_164 */
    public:
	Token( Task * cp, const std::string& task_name )
	    : Real_Instance( cp, task_name ) {}

	const std::string& type_name() const { return Task::type_strings.at(Task::Type::TOKEN); }
	void run();
    };


    class Token_r : public Real_Instance
    {
	/* SEMAPHORE		*/		/* BUG_164 */
    public:
	Token_r( Task * cp, const std::string& task_name )
	    : Real_Instance( cp, task_name ) {}

	const std::string& type_name() const { return Task::type_strings.at(Task::Type::TOKEN_R); }
	void run();
    };


    class Writer_Token : public Real_Instance
    {
	/* WORKER_TOKEN		*/		/* BUG_164 */
    public:
	Writer_Token( Task * cp, const std::string& task_name )
	    : Real_Instance( cp, task_name ) {}

	const std::string& type_name() const { return Task::type_strings.at(Task::Type::WRITER_TOKEN); }
	void run();
    };

    class RWLock_Server : public Virtual_Instance
    {
	/* RWLOCK 	*/
    public:
	RWLock_Server( Task * cp, const std::string& task_name )
	    : Virtual_Instance( cp, task_name ) {}

	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::RWLOCK_SERVER); }
	void run();
    };


    class Open_Arrivals : public Virtual_Instance
    {
	/* OPEN_ARRIVAL_SOURCE 	*/
    public:
	Open_Arrivals( Task * cp, const std::string& task_name );

	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::OPEN_ARRIVAL_SOURCE); }
	void run();
    };


    class Worker : public Real_Instance
    {
	/* WORKER 		*/
    public:
	Worker( Task * cp, const std::string& task_name )
	    : Real_Instance( cp, task_name ) {}

	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::WORKER); }
	void run();
    };


    class Thread : public Real_Instance
    {
	/* THREAD		*/
    public:
	Thread( Task * cp, const std::string& task_name, Instance * rip )
	    : Real_Instance( cp, task_name ), _root_ptr(rip) {}

	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::THREAD); }
	void run();

    private:
	virtual Instance * root_ptr() { return _root_ptr; }

	Instance * _root_ptr;		/* Pointer to root instance.	*/
    };


    class Signal : public Virtual_Instance {
	/* SIGNAL		*/
    public:
	Signal( Task * cp, const std::string& task_name )
	    : Virtual_Instance( cp, task_name ) {}

	virtual const std::string& type_name() const { return Task::type_strings.at(Task::Type::SIGNAL); }
	void run();
    };
}
#if HAVE_PARASOL
extern std::vector<Instance::Instance *> object_tab;	/* object table		*/
#endif
#endif
