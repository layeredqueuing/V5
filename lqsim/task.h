/* -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqsim/task.h $
 * Global vars for simulation.
 *
 * $Id: task.h 15293 2021-12-28 22:12:27Z greg $
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
struct ActivityList;
class srn_client;

#define	PRIORITY_OFFSET	10

typedef SYSCALL (*syscall_func_ptr)( double );
typedef void (*void_func_ptr)( void );
typedef double (*join_delay_func_ptr)( const result_t * );
typedef void * processor_class;

typedef double (*hold_func_ptr)( const Task *, const unsigned );

class Task {
    friend Activity * find_or_create_activity ( const void * task, const char * activity_name );
    friend class Instance;

public:
    /* Update service_routine in task.c when changing this enum */
    enum class Type {
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
    };

    static const std::map<const Type,const std::string> type_strings;

public:
    static Task * find( const char * task_name );
    static Task * add( LQIO::DOM::Task* domTask );

private:
    Task( const Task& );
    Task& operator=( const Task& );

public:
    Task( const Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );
    virtual ~Task();

    LQIO::DOM::Task * getDOM() const{ return _dom; }

    virtual const char * name() const { return _dom->getName().c_str(); }
    virtual scheduling_type discipline() const { return _dom->getSchedulingType(); }
    virtual unsigned multiplicity() const;					/* Special access!		*/
    virtual int priority() const;

    Type type() const { return _type; }
    const std::string& type_name() const { return type_strings.at(_type); }

    unsigned n_entries() const { return _entry.size(); }
    unsigned max_phases() const { return _max_phases; }
    Task& max_phases( unsigned max_phases ) { _max_phases = std::max( _max_phases, max_phases ); return *this; }

    Instance * add_task ( const char *task_name, Type type, int cpu_no, Instance * rip );
    virtual int std_port() const { return -1; }
    virtual int worker_port() const { return -1; }
    int node_id() const;
    Processor * processor() const { return _processor; }
    int group_id() const { return _group_id; }
    Task& set_group_id( int group_id ) { _group_id = group_id; return *this; }
    Message * alloc_message();
    void free_message( Message * msg );

    Activity * find_activity( const char * activity_name ) const;

    bool is_infinite() const;
    bool is_multiserver() const { return multiplicity() > 1; }
    bool is_reference_task() const { return type() == Type::CLIENT; }
    virtual bool is_sync_server() const { return false; }
    bool has_activities() const { return _activity.size() > 0; }	/* True if activities present.	*/
    bool has_threads() const { return _forks.size() > 0; }
    virtual bool derive_utilization() const;
    bool has_lost_messages() const;

    void set_start_activity( LQIO::DOM::Entry* theDOMEntry );
    Activity * add_activity( LQIO::DOM::Activity * activity );
    unsigned max_activities() const { return _activity.size(); }	/* Max # of activities.		*/
    void add_fork( ActivityList * list ) { _forks.push_back( list ); }
    void add_join( ActivityList * list ) { _joins.push_back( list ); }

    virtual Task& configure();		/* Called after recalulateDynamicVariables but before construct */
    virtual Task& construct();
    Task& initialize();			/* Called after construct() and start()	*/
    virtual bool start() = 0;
    virtual Task& kill() = 0;

    virtual Task& reset_stats();
    virtual Task& accumulate_data();
    virtual Task& insertDOMResults();
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
    LQIO::DOM::Task* _dom;			/* Stores all of the data.      */

    Processor * _processor;			/* node			        */
    int _group_id;				/* group  			*/
    syscall_func_ptr _compute_func;		/* function to use to "compute"	*/
    unsigned _active;				/* Number of active instances.	*/
    unsigned _max_phases;			/* Max # phases, this task.	*/

    std::vector<ActivityList *> _forks;		/* List of forks for this task	*/
    std::vector<ActivityList *> _joins; 	/* List of joins for this task	*/
    std::list<Message *> _pending_msgs;		/* Messages blocked by join.	*/
    double _join_start_time;			/* non-zero if in sync-join	*/

    std::list<Message *> _free_msgs;		/* Pool of messages 		*/

protected:
    Type _type;

public:
    std::vector<Entry *> _entry;		/* entry array		        */
    std::vector<Activity *>_activity;		/* List of activities.		*/
    std::vector<ActivityList *> _act_list;	/* activity list array 		*/
    unsigned _hold_active;			/* Number of active instances.	*/

    bool trace_flag;				/* True if task is to be traced	*/

    Histogram * _hist_data;            		/* Structure which stores histogram data for this task */
    result_t r_cycle;				/* Cycle time.		        */
    result_t r_util;				/* Utilization.		        */
    result_t r_group_util;			/* group Utilization.		*/
    result_t r_loss_prob;			/* Asynch message loss prob.	*/
};


class Reference_Task : public Task
{
private:
    Reference_Task( const Reference_Task& ) = delete;
    Reference_Task& operator=( const Reference_Task& ) = delete;

public:
    Reference_Task( const Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );

    virtual double think_time() const { return _think_time; }			/* Cached.  see create()	*/

    virtual bool start();
    virtual Reference_Task& kill();

protected:
    virtual void create_instance();

private:
    double _think_time;				/* Cached copy of think time.	*/
    std::vector<srn_client *> _task_list;	/* task id's of clients		*/
};

class Server_Task : public Task
{
public:
    Server_Task( const Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );

    virtual int std_port() const;

    void set_synchronization_server();
    virtual bool is_sync_server() const { return _sync_server; }
    virtual int worker_port() const { return _worker_port; }

    virtual bool start();
    virtual Server_Task& kill();

protected:
    virtual void create_instance();

protected:
    Instance * _task;				/* task id of main inst	        */
    int _worker_port;				/* Port for workers to send to.	*/
    bool _sync_server;				/* True if we sync here		*/
};

class Semaphore_Task : public Server_Task
{
public:
    Semaphore_Task( const Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );

    int signal_port() const { return _signal_port; }
    Instance * signal_task() const { return _signal_task; }

    virtual Semaphore_Task& construct();
    virtual bool start();
    virtual Semaphore_Task& kill();

    virtual Semaphore_Task& reset_stats();
    virtual Semaphore_Task& accumulate_data();
    virtual Semaphore_Task& insertDOMResults();

    virtual FILE * print( FILE * ) const;

protected:
    virtual void create_instance();

public:
    result_t r_hold;				/* Service time.		*/
    result_t r_hold_sqr;			/* Service time.		*/
    result_t r_hold_util;

protected:
    Instance * _signal_task;			/* 				*/
    int _signal_port;				/* Port for signals to send to.	*/
};

class ReadWriteLock_Task : public Semaphore_Task
{
public:
    ReadWriteLock_Task( const Type type, LQIO::DOM::Task* domTask, Processor * aProc, Group * aGroup );

    Instance * writer() const { return _writer; }
    Instance * reader() const { return _reader; }

    int writerQ_port() const { return _writerQ_port; }	/* for RWLOCK */
    int readerQ_port() const { return _readerQ_port; }
    int signal_port2() const { return _signal_port2; }

    virtual ReadWriteLock_Task& construct();
    virtual bool start();
    virtual ReadWriteLock_Task& kill();

    virtual ReadWriteLock_Task& reset_stats();
    virtual ReadWriteLock_Task& accumulate_data();
    virtual ReadWriteLock_Task& insertDOMResults();

    virtual FILE * print( FILE * ) const;

protected:
    virtual void create_instance();

public:
    result_t r_reader_hold;			/* Reader holding time		*/
    result_t r_reader_hold_sqr;
    result_t r_reader_wait;			/* Reader blocked time		*/
    result_t r_reader_wait_sqr;
    result_t r_reader_hold_util;

    result_t r_writer_hold;			/* writer holding time		*/
    result_t r_writer_hold_sqr;
    result_t r_writer_wait;			/* writer blocked time		*/
    result_t r_writer_wait_sqr;
    result_t r_writer_hold_util;

private:
    Instance * _reader;				/* task id for readers' queue   */
    Instance * _writer;	  	        	/* task id for writer_token	*/

    int _signal_port2;				/* Signal Port for writer_token	*/
    int _writerQ_port;				/* Port for writer message queue*/
    int _readerQ_port;				/* Port for reader message queue*/

};


class Pseudo_Task : public Task
{
public:
    Pseudo_Task( const char * name ) : Task( Type::OPEN_ARRIVAL_SOURCE, nullptr, nullptr, nullptr ), _name(name) {}

    virtual const char * name() const { return _name.c_str(); }
    virtual scheduling_type discipline() const { return SCHEDULE_DELAY; }
    virtual unsigned multiplicity() const { return 1; }			/* Special access!		*/
    virtual int priority() const { return 0; }				/* priority		        */
    virtual double think_time() const { return 0.0; }			/* Think time for ref. tasks.	*/
    virtual void * task_element() const { return 0; }
    virtual bool derive_utilization() const { return true; }

    virtual Pseudo_Task& insertDOMResults();

    virtual bool start();
    virtual Pseudo_Task& kill();

protected:
    virtual void create_instance();

private:
    const std::string _name;
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

extern std::set <Task *, ltTask> task;	/* Task table.	*/
#endif
