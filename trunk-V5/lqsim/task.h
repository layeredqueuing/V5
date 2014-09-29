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
/************************************************************************/

#ifndef	TASK_H
#define TASK_H

#include <set>
#include <vector>
#include <string>
#include <cstdio>
#include <lqio/dom_task.h>
#include <parasol.h>

#include "result.h"
#include "entry.h"
#include "message.h"

class Task;
class Processor;
class Group;
class ParentGroup;
class Instance;
class Activity;
struct activity_list_t;
class srn_client;

#define	PRIORITY_OFFSET	10

typedef SYSCALL (*syscall_func_ptr)( double );
typedef void (*void_func_ptr)( void );
typedef double (*join_delay_func_ptr)( const result_t * );
typedef void * processor_class;

typedef struct entry_status_t {
    struct activity_list_t * activity;
    class Message * head;
    class Message * tail;
} entry_status_t;

typedef double (*hold_func_ptr)( const Task *, const unsigned );

class Task {
    friend Activity * find_or_create_activity ( const void * task, const char * activity_name );
    
public:
    /* Update service_routine in task.c when changing this enum */
    typedef enum {
	UNDEFINED,
	CLIENT,
	SERVER,
	MULTI_SERVER,
	INFINITE_SERVER,
	SYNCHRONIZATION_SERVER,
	SEMAPHORE,
	OPEN_ARRIVAL_SOURCE,
	WORKER,
	THREAD,
	TOKEN,
	TOKEN_R,
	SIGNAL,
	RWLOCK,			/* RWLOCK TASK CLASS	*/
	RWLOCK_SERVER,		/* RWLOCK SERVER TOKEN	*/
	WRITER_TOKEN
    } task_type;

    static const char * type_strings[];
    
public:
    static Task * find( const char * task_name );
    static Task * add( LQIO::DOM::Task* domTask );

private:
    Task( const Task& );
    Task& operator=( const Task& );
    
public:
    Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );
    virtual ~Task();

    LQIO::DOM::Task * getDOM() const{ return _dom_task; }
    
    virtual const char * name() const { return _dom_task->getName().c_str(); }
    virtual scheduling_type discipline() const { return _dom_task->getSchedulingType(); }
    virtual unsigned multiplicity() const;					/* Special access!		*/
    virtual int priority() const { return _dom_task->getPriority(); }		/* priority		        */
    virtual void * task_element() const { return const_cast<void *>(_dom_task->getXMLDOMElement()); }

    task_type type() const { return _type; }
    const char * type_name() const { return type_strings[static_cast<int>(_type)]; }

    unsigned n_entries() const { return _entry.size(); }

    Instance * add_task ( const char *task_name, task_type type, int cpu_no, Instance * rip );
    virtual int std_port() const { return -1; }
    virtual int worker_port() const { return -1; }
    int node_id() const;
    Processor * processor() const { return _processor; }
    int group_id() const { return _group_id; }
    Task& set_group_id( int group_id ) { _group_id = group_id; return *this; } 

    Activity * find_activity( const char * activity_name ) const;

    bool is_infinite() const;
    bool is_multiserver() const { return multiplicity() > 1; }
    bool is_reference_task() const { return type() == CLIENT; }
    virtual bool isSynchServer() const { return false; }
    bool has_activities() const { return _activity.size() > 0; }	/* True if activities present.	*/
    bool has_threads() const { return _forks.size() > 0; }
    virtual bool derive_utilization() const;
    bool has_lost_messages() const;

    void set_start_activity( LQIO::DOM::Entry* theDOMEntry );
    Activity * add_activity( LQIO::DOM::Activity * activity );

    virtual void create();
    virtual bool start() = 0;
    virtual void kill() = 0;

    virtual void reset_stats();
    virtual void accumulate_data();
    virtual void insertDOMResults();
    virtual FILE * print( FILE * ) const;

protected:
    virtual void create_instance() = 0;

private:
    bool has_send_no_reply() const;

    void build_links();
    void alloc_pool();
    
    double throughput() const;
    double throughput_variance() const;
    
private:
    LQIO::DOM::Task* _dom_task;		/* Stores all of the data.      */

    Processor * _processor;		/* node			        */
    int _group_id;			/* group  			*/

protected:
    task_type _type;

public:
    vector<Entry *> _entry;		/* entry array		        */
    vector<Activity *>_activity;	/* List of activities.		*/
    vector<ActivityList *> _act_list;	/* activity list array 		*/
    
    bool trace_flag;			/* True if task is to be traced	*/
    double join_start_time;		/* non-zero if in sync-join	*/
    syscall_func_ptr compute_func;	/* function to use to "compute"	*/

    struct entry_status_t * entry_status;	/* Entry is busy.	*/
    Message * free_messages;		/* Pool of messages 		*/
    vector<ActivityList *> _joins; 	/* List of joins for this task	*/
    vector<ActivityList *> _forks;
    unsigned max_phases;		/* Max # phases, this task.	*/
    unsigned max_activities;		/* Max # of activities.		*/
    unsigned active;			/* Number of active instances.	*/
    unsigned hold_active;		/* Number of active instances.	*/
    Histogram * _hist_data;            	/* Structure which stores histogram data for this task */
    result_t r_cycle;			/* Cycle time.		        */
    result_t r_util;			/* Utilization.		        */
    result_t r_group_util;		/* group Utilization.		*/
    result_t r_loss_prob;		/* Asynch message loss prob.	*/

private:
    Message * _msg_tab;			/* Message pool			*/
};


class Reference_Task : public Task
{
public:
    Reference_Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );

    virtual double think_time() const { return _think_time; }			/* Cached.  see create()	*/

    virtual bool start();
    virtual void kill();

protected:    
    virtual void create_instance();

private:
    double _think_time;			/* Cached copy of think time.	*/	
    vector<srn_client *> _task_list;	/* task id's of clients		*/
};

class Server_Task : public Task
{
public:
    Server_Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );

    virtual int std_port() const;

    void set_synchronization_server();
    virtual bool isSynchServer() const { return _sync_server; }
    virtual int worker_port() const { return _worker_port; }

    virtual bool start();
    virtual void kill();

protected:    
    virtual void create_instance();

protected:
    Instance * _task;			/* task id of main inst	        */
    int _worker_port;			/* Port for workers to send to.	*/
    bool _sync_server;			/* True if we sync here		*/
};

class Semaphore_Task : public Server_Task
{
public:
    Semaphore_Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );

    int signal_port() const { return _signal_port; }
    Instance * signal_task() const { return _signal_task; }

    virtual void create();
    virtual bool start();
    virtual void kill();

    virtual void reset_stats();
    virtual void accumulate_data();
    virtual void insertDOMResults();

    virtual FILE * print( FILE * ) const;

protected:    
    virtual void create_instance();

public:
    result_t r_hold;			/* Service time.		*/
    result_t r_hold_sqr;		/* Service time.		*/
    result_t r_hold_util;

protected:
    Instance * _signal_task;		/* 				*/
    int _signal_port;			/* Port for signals to send to.	*/
};

class ReadWriteLock_Task : public Semaphore_Task
{
public:
    ReadWriteLock_Task( const task_type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );

    Instance * writer() const { return _writer; }
    Instance * reader() const { return _reader; }

    int writerQ_port() const { return _writerQ_port; }	/* for RWLOCK */
    int readerQ_port() const { return _readerQ_port; }	
    int signal_port2() const { return _signal_port2; }

    virtual void create();
    virtual bool start();
    virtual void kill();

    virtual void reset_stats();
    virtual void accumulate_data();
    virtual void insertDOMResults();

    virtual FILE * print( FILE * ) const;

protected:    
    virtual void create_instance();

public:
    result_t r_reader_hold;		/* Reader holding time		*/
    result_t r_reader_hold_sqr;		
    result_t r_reader_wait;		/* Reader blocked time		*/
    result_t r_reader_wait_sqr;		
    result_t r_reader_hold_util;

    result_t r_writer_hold;		/* writer holding time		*/
    result_t r_writer_hold_sqr;		
    result_t r_writer_wait;		/* writer blocked time		*/
    result_t r_writer_wait_sqr;	
    result_t r_writer_hold_util;
    
private:
    Instance * _reader;			/* task id for readers' queue   */
    Instance * _writer;	  	        /* task id for writer_token	*/   

    int _signal_port2;			/* Signal Port for writer_token	*/  
    int _writerQ_port;			/* Port for writer message queue*/
    int _readerQ_port;			/* Port for reader message queue*/

};


class Pseudo_Task : public Task
{
public:
    Pseudo_Task( const char * name ) : Task( Task::OPEN_ARRIVAL_SOURCE, 0, 0, 0 ), _name(name) {}

    virtual const char * name() const { return _name.c_str(); }
    virtual scheduling_type discipline() const { return SCHEDULE_DELAY; }
    virtual unsigned multiplicity() const { return 1; }			/* Special access!		*/
    virtual int priority() const { return 0; }				/* priority		        */
    virtual double think_time() const { return 0.0; }			/* Think time for ref. tasks.	*/
    virtual void * task_element() const { return 0; }
    virtual bool derive_utilization() const { return true; }

    virtual void insertDOMResults();

    virtual bool start();
    virtual void kill();

protected:    
    virtual void create_instance();

private:
    const string _name;
    Instance * _task;			/* task id of main inst	        */
};


typedef double (*hold_func_ptr)( const Task *, const unsigned );

extern unsigned total_tasks;

/* ------------------------------------------------------------------------ */
/*
 * Compare to tasks by their name.  Used by the set class to insert items
 */

struct ltTask
{
    bool operator()(const Task * p1, const Task * p2) const { return strcmp( p1->name(), p2->name() ) < 0; }
};


/*
 * Compare a task name to a string.  Used by the find_if (and other algorithm type things).
 */

struct eqTaskStr 
{
    eqTaskStr( const char * s ) : _s(s) {}
    bool operator()(const Task * p1 ) const { return strcmp( p1->name(), _s ) == 0; }

private:
    const char * _s;
};

struct eqTaskPtr
{
    eqTaskPtr( const Task * p ) : _p(p) {}
    bool operator()(const Task * p ) const { return p == _p; }

private:
    const Task * _p;
};

extern set <Task *, ltTask> task;	/* Task table.	*/
#endif
