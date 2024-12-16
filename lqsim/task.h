/* -*- c++ -*-
 * Lqsim-parasol task interface.
 *
 * $Id: task.h 17513 2024-12-05 12:52:35Z greg $
 */

/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* May 1996.								*/
/* Nov 2005.								*/
/* Nov 2024.								*/
/************************************************************************/

#ifndef	LQSIM_TASK_H
#define LQSIM_TASK_H

#include <set>
#include <vector>
#include <string>
#include <list>
#include <algorithm>
#include <lqio/dom_task.h>

#include "actlist.h"
#include "processor.h"
#include "result.h"
#if HAVE_PARASOL
#include <parasol/para_internals.h>
#include "message.h"
#else
#include "rendezvous.h"
#endif

class Activity;
class ActivityList;
class Entry;
class Group;
class ParentGroup;
namespace Instance {
    class Instance;
    class Client;
    class Server;
}

class Task {
    friend class Instance::Instance;

    /*
     * Compare to tasks by their name.  Used by the set class to
     * insert items
     */

    struct ltTask
    {
	bool operator()(const Task * p1, const Task * p2) const { return p1->name() < p2->name(); }
    };


    /*
     * Compare a task name to a string.  Used by the find_if (and
     * other algorithm type things).
     */

public:
    static std::set <Task *, ltTask> __tasks;	/* Task table.	*/

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
    static Task * find( const std::string& task_name );
    static Task * add( LQIO::DOM::Task* dom );

private:
    Task( const Task& ) = delete;
    Task& operator=( const Task& ) = delete;

public:
    Task( LQIO::DOM::Task* dom, Processor * processor, Group * group );
    virtual ~Task();

    LQIO::DOM::Task * getDOM() const{ return _dom; }

    virtual double think_time() const { abort(); return 0.0; }			/* Cached.  see create()	*/
    virtual const std::string& name() const { return _dom->getName(); }
    virtual scheduling_type discipline() const { return _dom->getSchedulingType(); }
    virtual unsigned multiplicity() const;					/* Special access!		*/
    virtual int priority() const { return _dom->hasPriority() ? _dom->getPriorityValue() : 0; }

    virtual Type type() const = 0;
    const std::string& type_name() const { return type_strings.at(type()); }

    const std::vector<Entry *>& entries() const { return _entries; }
    const std::vector<Activity *>& activities() const { return _activities; }
    const std::vector<ActivityList *>& precedences() const { return  _precedences; }

    unsigned n_entries() const { return _entries.size(); }
    unsigned max_phases() const { return _max_phases; }
    Task& max_phases( unsigned max_phases ) { _max_phases = std::max( _max_phases, max_phases ); return *this; }

    Instance::Instance * add_task ( const char *task_name, Type type, int cpu_no, Instance::Instance * rip );
    virtual int std_port() const { return -1; }
#if HAVE_PARASOL
    virtual int worker_port() const { return -1; }
    int node_id() const;
#endif
    Processor * processor() const { return _processor; }
    Group * group() const { return _group; }
    Task& setGroup( Group * group ) { _group = group; return *this; }
#if HAVE_PARASOL
    void set_group_id( int group_id ) { _group_id = group_id; }
    int group_id() const { return _group_id; }
    Message * alloc_message();
    void free_message( Message * msg );
#endif

    Activity * find_activity( const std::string& activity_name ) const;

    bool is_infinite() const;
    bool is_multiserver() const { return multiplicity() > 1; }
    bool is_reference_task() const { return type() == Type::CLIENT; }
    virtual bool is_sync_server() const { return false; }
    virtual bool is_async_inf_server() const { return false; }
    bool has_activities() const { return !_activities.empty(); }	/* True if activities present.	*/
    bool has_threads() const { return !_forks.empty(); }
    bool has_think_time() const;
    bool has_lost_messages() const;
    virtual bool derive_utilization() const;

    void set_start_activity( LQIO::DOM::Entry* theDOMEntry );
    Activity * add_activity( LQIO::DOM::Activity * activity );
    unsigned max_activities() const { return _activities.size(); }	/* Max # of activities.		*/
    Task& add_list( ActivityList * list ) { _precedences.push_back( list ); return *this; }
    Task& add_fork( AndForkActivityList * list ) { _forks.push_back( list ); return *this; }
    Task& add_join( AndJoinActivityList * list ) { _joins.push_back( list ); return *this; }

    virtual Task& configure();		/* Called after recalulateDynamicVariables but before create */
    virtual Task& create();
    Task& initialize();			/* Called after create() and start()	*/
    virtual bool run() = 0;
    virtual Task& stop() = 0;

    virtual Task& reset_stats();
    virtual Task& accumulate_data();
    virtual Task& insertDOMResults();
    virtual std::ostream& print( std::ostream& ) const;

protected:
    virtual void create_instance() = 0;

private:
    bool has_send_no_reply() const;

#if HAVE_PARASOL
    void alloc_pool();
#endif

    double throughput() const;
    double throughput_variance() const;

private:
    LQIO::DOM::Task* _dom;			/* Stores all of the data.      */
    Processor * _processor;			/* node			        */
    Group * _group;				/* group  			*/
#if HAVE_PARASOL
    int _group_id;
#endif
    Processor::compute_fptr _compute_func;	/* function to use to "compute"	*/
    unsigned _active;				/* Number of active instances.	*/
    unsigned _max_phases;			/* Max # phases, this task.	*/

    std::vector<Entry *> _entries;		/* entry array		        */
    std::vector<Activity *>_activities;		/* List of activities.		*/
    std::vector<ActivityList *> _precedences;	/* activity list array 		*/
    std::vector<AndForkActivityList *> _forks;	/* List of forks for this task	*/
    std::vector<AndJoinActivityList *> _joins; 	/* List of joins for this task	*/

#if HAVE_PARASOL
    std::list<Message *> _pending_msgs;		/* Messages blocked by join.	*/
    std::list<Message *> _free_msgs;		/* Pool of messages 		*/
#endif

public:
    bool trace_flag;				/* True if task is to be traced	*/

    Histogram * _hist_data;            		/* Histogram data for this task */
    SampleResult r_cycle;			/* Cycle time.		        */
    VariableResult r_util;			/* Utilization.		        */
    VariableResult r_group_util;		/* group Utilization.		*/

    unsigned _hold_active;			/* Number of active instances.	*/
    double _join_start_time;			/* non-zero if in sync-join	*/
};


class Reference_Task : public Task
{
private:
    Reference_Task( const Reference_Task& ) = delete;
    Reference_Task& operator=( const Reference_Task& ) = delete;

public:
    Reference_Task( LQIO::DOM::Task* dom, Processor * processor, Group * group );
    virtual ~Reference_Task();

    virtual Type type() const { return Task::Type::CLIENT; }
    virtual double think_time() const { return _think_time; }			/* Cached.  see create()	*/

    virtual bool run();
    virtual Reference_Task& stop();

protected:
    virtual void create_instance();

private:
    double _think_time;				/* Cached copy of think time.	*/
    std::vector<Instance::Client *> _clients;	/* task id's of clients		*/
};

class Server_Task : public Task
{
public:
    Server_Task( LQIO::DOM::Task* dom, Processor * processor, Group * group );
    virtual ~Server_Task();

    virtual Type type() const { return Task::Type::SERVER; }
    virtual int std_port() const;

    void set_synchronization_server();
    virtual bool is_sync_server() const { return _sync_server; }
    virtual bool run();
    virtual Server_Task& stop();

protected:
    virtual void create_instance();

protected:
    Instance::Instance * _server;		/* task id of main inst	        */
    bool _sync_server;				/* True if we sync here		*/

private:
#if !HAVE_PARASOL
    Rendezvous::Single_Server<int,int> _rendezvous;
#endif
};

class Multi_Server_Task : public Task
{
public:
    Multi_Server_Task( LQIO::DOM::Task* dom, Processor * processor, Group * group );
    virtual ~Multi_Server_Task();

    virtual Type type() const { return Task::Type::MULTI_SERVER; }
    virtual int std_port() const;

#if HAVE_PARASOL
    virtual int worker_port() const { return _worker_port; }
#endif

    virtual bool run();
    virtual Multi_Server_Task& stop();

protected:
    virtual void create_instance();

protected:
    Instance::Instance * _server;		/* task id of main inst	        */

private:
#if HAVE_PARASOL
    int _worker_port;				/* Port for workers to send to.	*/
#else
    Rendezvous::Multi_Server<int,int> _rendezvous;
    std::vector<Instance::Client *> _servers;	/* task id's of clients		*/
#endif
};

class Infinite_Server_Task : public Task
{
public:
    Infinite_Server_Task( LQIO::DOM::Task* dom, Processor * processor, Group * group );
    virtual ~Infinite_Server_Task();

    virtual Type type() const { return Task::Type::MULTI_SERVER; }
    virtual int std_port() const;

    virtual bool is_async_inf_server() const;
    virtual int worker_port() const { return _worker_port; }

    virtual bool run();
    virtual Infinite_Server_Task& stop();

protected:
    virtual void create_instance();

protected:
    Instance::Instance * _server;		/* task id of main inst	        */

private:
    int _worker_port;				/* Port for workers to send to.	*/
#if !HAVE_PARASOL
    Rendezvous::Multi_Server<int,int> _rendezvous;
#endif
};

class Semaphore_Task : public Server_Task
{
public:
    Semaphore_Task( LQIO::DOM::Task* dom, Processor * processor, Group * group );

    virtual Type type() const { return Task::Type::SEMAPHORE; }
    virtual int worker_port() const { return _worker_port; }
    int signal_port() const { return _signal_port; }
    Instance::Instance * signal_task() const { return _signal_task; }

    virtual bool run();
    virtual Semaphore_Task& stop();

    virtual Semaphore_Task& reset_stats();
    virtual Semaphore_Task& accumulate_data();
    virtual Semaphore_Task& insertDOMResults();

    virtual std::ostream& print( std::ostream& ) const;

protected:
    virtual void create_instance();

public:
    SampleResult r_hold;			/* Service time.		*/
    SampleResult r_hold_sqr;			/* Service time.		*/
    VariableResult r_hold_util;

protected:
    Instance::Instance * _signal_task;		/* 				*/
    int _worker_port;
    int _signal_port;				/* Port for signals to send to.	*/

private:
    static const unsigned int N_SEMAPHORE_ENTRIES = 2;
};

class ReadWriteLock_Task : public Semaphore_Task
{
public:
    ReadWriteLock_Task( LQIO::DOM::Task* dom, Processor * processor, Group * group );

    virtual Type type() const { return Task::Type::RWLOCK; }
    Instance::Instance * writer() const { return _writer; }
    Instance::Instance * reader() const { return _reader; }

    int writerQ_port() const { return _writerQ_port; }	/* for RWLOCK */
    int readerQ_port() const { return _readerQ_port; }
    int signal_port2() const { return _signal_port2; }

    virtual bool run();
    virtual ReadWriteLock_Task& stop();

    virtual ReadWriteLock_Task& reset_stats();
    virtual ReadWriteLock_Task& accumulate_data();
    virtual ReadWriteLock_Task& insertDOMResults();

    virtual std::ostream& print( std::ostream& ) const;

protected:
    virtual void create_instance();

public:
    SampleResult r_reader_hold;			/* Reader holding time		*/
    SampleResult r_reader_hold_sqr;
    SampleResult r_reader_wait;			/* Reader blocked time		*/
    SampleResult r_reader_wait_sqr;
    VariableResult r_reader_hold_util;

    SampleResult r_writer_hold;			/* writer holding time		*/
    SampleResult r_writer_hold_sqr;
    SampleResult r_writer_wait;			/* writer blocked time		*/
    SampleResult r_writer_wait_sqr;
    VariableResult r_writer_hold_util;

private:
    Instance::Instance * _reader;		/* task id for readers' queue   */
    Instance::Instance * _writer;	  	/* task id for writer_token	*/

    int _signal_port2;				/* Signal Port for writer_token	*/
    int _writerQ_port;				/* Port for writer message queue*/
    int _readerQ_port;				/* Port for reader message queue*/

    static const unsigned int N_RWLOCK_ENTRIES = 4;
};


class Pseudo_Task : public Task
{
public:
    Pseudo_Task( const std::string& name ) : Task( nullptr, nullptr, nullptr ), _name(name) {}

    virtual Type type() const { return Task::Type::OPEN_ARRIVAL_SOURCE; }
    virtual const std::string& name() const { return _name; }
    virtual scheduling_type discipline() const { return SCHEDULE_DELAY; }
    virtual unsigned multiplicity() const { return 1; }			/* Special access!		*/
    virtual int priority() const { return 0; }				/* priority		        */
    virtual double think_time() const { return 0.0; }			/* Think time for ref. tasks.	*/
    virtual void * task_element() const { return 0; }
    virtual bool derive_utilization() const { return true; }

    virtual Pseudo_Task& insertDOMResults();

    virtual bool run();
    virtual Pseudo_Task& stop();

protected:
    virtual void create_instance();

private:
    const std::string _name;
    Instance::Instance * _task;			/* task id of main inst	        */
};

extern unsigned total_tasks;
#endif
