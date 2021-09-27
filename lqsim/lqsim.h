/* -*- c++ -*- */
/************************************************************************/
/* Copyright the Real-Time and Distributed Systems Group,		*/
/* Department of Systems and Computer Engineering,			*/
/* Carleton University, Ottawa, Ontario, Canada. K1S 5B6		*/
/* 									*/
/* September 1991.							*/
/************************************************************************/
#ifndef _PARASRVN_H
#define _PARASRVN_H

/*
 * Global vars for setting up simulation.
 *
 * $URL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqsim/lqsim.h $
 *
 * $Id: lqsim.h 14995 2021-09-27 14:01:46Z greg $
 */

#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif
#include <cstdio>
#include <lqio/input.h>
#if	defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>    /* Need def for size_t */
#endif

#define GROUP_SCHEDULING       0

#define	MAX_PHASES	3
#define MAX_TASKS	16384
#define	MAX_PORTS	16384
#define	MAX_LINKS	100
#define	MAX_NODES	32768

#define	MAX_CLASSES	MAX_TASKS
#define	MAX_TARGETS	256
#define MAX_PROC	MAX_TASKS
#define MAX_GROUPS	MAX_TASKS

#define MAX_MESSAGES	16384

#define	LINKS_MESSAGE_SIZE	1


#define EPSILON 0.000001

#if defined(__cplusplus)
extern "C" {
#endif

extern bool debug_flag;			/* Debugging flag set?      	*/
extern bool debug_interactive_stepping;
extern bool global_parse_flag;		/* Parsable output desired? 	*/
extern bool global_xml_flag;	      	/* Output XML results.		*/
extern bool global_rtf_flag;		/* Output in RTF.		*/
extern bool raw_stat_flag;		/* Verbose text output?	    	*/
extern bool verbose_flag;		/* Verbose text output?	    	*/
extern bool no_execute_flag;		/* Run simulation if false	*/
extern bool timeline_flag;		/* Generate output for timeline	*/
extern bool trace_msgbuf_flag;		/* Observe msg buffer operation	*/
extern bool override_print_int;     	/* Override input file.		*/
extern bool reload_flag;		/* Reload results from LQX run.	*/
extern bool restart_flag;		/* Restart and mussing run 	*/
extern bool quorum_delayed_calls;	/* Quorum reply (BUG_311)	*/
extern bool check_stacks;		/* Test for stack overrun	*/
extern int print_interval;		/* Value set by input file.	*/
    
extern unsigned long watched_events;	/* Observe these events.	*/

extern int trace_driver;		/* trace sim. drriver.		*/

extern int scheduling_model;		/* Slice/Natural scheduling.	*/

extern double inter_proc_delay;		/* Inter-processor delay.	*/

extern char * histogram_output_file;	/* File name for histogram data	*/
    
void * my_malloc( size_t size );
void * my_realloc( void * ptr, size_t size );
void report_matherr( FILE * output );

extern int nice_value;
extern bool deferred_exception;		/* Fault detected in thread	*/

extern unsigned link_tab[MAX_NODES];	/* Link table.			*/

/*
 * These items are used with bit-tests.
 *   bit 0 == default/custom
 *   bit 1 == slice/natural
 */

#define SCHEDULE_SLICE		0x0
#define	SCHEDULE_CUSTOM		0x1
#define	SCHEDULE_NATURAL	0x2
#define SCHEDULE_CUSTOM_NATURAL	0x3

#if defined(_AIX)
extern char * strdup( char * str );
#endif

/* For open queues. */
#define DEFAULT_QUEUE_SIZE	1024

typedef enum
{
    ASYNC_INTERACTION_INITIATED,
    SYNC_INTERACTION_INITIATED,
    SYNC_INTERACTION_ESTABLISHED,
    SYNC_INTERACTION_REPLIES,
    SYNC_INTERACTION_COMPLETED,
    SYNC_INTERACTION_ABORTED,
    SYNC_INTERACTION_FORWARDED,
    WORKER_DISPATCH,
    WORKER_IDLE,
    TASK_CREATED,
    TASK_IS_READY,
    TASK_IS_RUNNING,			/* doing message passing... */
    TASK_IS_COMPUTING,			/* executing virtual code... */
    TASK_IS_WAITING,
    THREAD_START,
    THREAD_ENQUEUE_MSG,
    THREAD_DEQUEUE_MSG,
    THREAD_IDLE,
    THREAD_CREATE,
    THREAD_REAP,
    THREAD_STOP,
    ACTIVITY_START,
    ACTIVITY_EXECUTE,
    ACTIVITY_FORK,
    ACTIVITY_JOIN,
    DEQUEUE_READER,
    DEQUEUE_WRITER,
    ENQUEUE_READER,
    ENQUEUE_WRITER
} trace_events;

#define ASYNC_INTERACTION_INITIATED_BIT (1<<ASYNC_INTERACTION_INITIATED)
#define SYNC_INTERACTION_INITIATED_BIT (1<<SYNC_INTERACTION_INITIATED)
#define SYNC_INTERACTION_ESTABLISHED_BIT (1<<SYNC_INTERACTION_ESTABLISHED)
#define SYNC_INTERACTION_REPLIES_BIT (1<<SYNC_INTERACTION_REPLIES)
#define SYNC_INTERACTION_COMPLETED_BIT (1<<SYNC_INTERACTION_COMPLETED)
#define SYNC_INTERACTION_ABORTED_BIT (1<<SYNC_INTERACTION_ABORTED)
#define SYNC_INTERACTION_FORWARDED_BIT (1<<SYNC_INTERACTION_FORWARDED)
#define WORKER_DISPATCH_BIT (1<<WORKER_DISPATCH)
#define WORKER_IDLE_BIT (1<<WORKER_IDLE)
#define TASK_CREATED_BIT (1<<TASK_CREATED)
#define TASK_IS_READY_BIT (1<<TASK_IS_READY)
#define TASK_IS_RUNNING_BIT (1<<TASK_IS_RUNNING)
#define TASK_IS_COMPUTING_BIT (1<<TASK_IS_COMPUTING)
#define TASK_IS_WAITING_BIT (1<<TASK_IS_WAITING)
#define THREAD_START_BIT (1<<THREAD_START)
#define THREAD_ENQUEUE_MSG_BIT (1<<THREAD_ENQUEUE_MSG)
#define THREAD_DEQUEUE_MSG_BIT (1<<THREAD_DEQUEUE_MSG)
#define THREAD_IDLE_BIT (1<<THREAD_IDLE)
#define THREAD_CREATE_BIT (1<<THREAD_CREATE)
#define THREAD_REAP_BIT (1<<THREAD_REAP)
#define THREAD_STOP_BIT (1<<THREAD_STOP)
#define ACTIVITY_START_BIT (1<<ACTIVITY_START)
#define ACTIVITY_EXECUTE_BIT (1<<ACTIVITY_EXECUTE)
#define ACTIVITY_FORK_BIT (1<<ACTIVITY_FORK)
#define ACTIVITY_JOIN_BIT (1<<ACTIVITY_JOIN)

#if defined(__cplusplus)
}
#endif


template <class Type> struct Exec
{
    typedef Type& (Type::*funcPtr)();
    Exec<Type>( funcPtr f ) : _f(f) {};
    void operator()( Type * object ) const { (object->*_f)(); }
    void operator()( Type& object ) const { (object.*_f)(); }
private:
    funcPtr _f;
};

template <class Type> struct ConstExec
{
    typedef const Type& (Type::*funcPtr)() const;
    ConstExec<Type>( const funcPtr f ) : _f(f) {};
    void operator()( const Type * object ) const { (object->*_f)(); }
    void operator()( const Type& object ) const { (object.*_f)(); }
private:
    const funcPtr _f;
};
    
template <class Type1, class Type2> struct Exec1
{
    typedef Type1& (Type1::*funcPtr)( Type2 x );
    Exec1<Type1,Type2>( funcPtr f, Type2 x ) : _f(f), _x(x) {}
    void operator()( Type1 * object ) const { (object->*_f)( _x ); }
    void operator()( Type1& object ) const { (object.*_f)( _x ); }
private:
    funcPtr _f;
    Type2 _x;
};

template <class Type1, class Type2> struct ConstExec1
{
    typedef const Type1& (Type1::*funcPtr)( Type2 x ) const;
    ConstExec1<Type1,Type2>( const funcPtr f, Type2 x ) : _f(f), _x(x) {}
    void operator()( Type1 * object ) const { (object->*_f)( _x ); }
    void operator()( Type1& object ) const { (object.*_f)( _x ); }
private:
    const funcPtr _f;
    Type2 _x;
};

template <class Type1, class Type2 > struct ExecSum
{
    typedef Type2 (Type1::*funcPtr)();
    ExecSum<Type1,Type2>( funcPtr f ) : _f(f), _sum(0.) {};
    void operator()( Type1 * object ) { _sum += (object->*_f)(); }
    void operator()( Type1& object ) { _sum += (object.*_f)(); }
    Type2 sum() const { return _sum; }
private:
    const funcPtr _f;
    Type2 _sum;
};
    
template <class Type1, class Type2 > struct ConstExecSum
{
    typedef Type2 (Type1::*funcPtr)() const;
    ConstExecSum<Type1,Type2>( funcPtr f ) : _f(f), _sum(0.) {};
    void operator()( Type1 * object ) { _sum += (object->*_f)(); }
    void operator()( Type1& object ) { _sum += (object.*_f)(); }
    Type2 sum() const { return _sum; }
private:
    const funcPtr _f;
    Type2 _sum;
};
    
template <class Type> struct Predicate
{
    typedef bool (Type::*predicate)() const;
    Predicate<Type>( const predicate p ) : _p(p) {};
    bool operator()( const Type * object ) const { return (object->*_p)(); }
    bool operator()( const Type& object ) const { return (object.*_p)(); }
private:
    const predicate _p;
};

static inline void throw_bad_parameter() { throw std::domain_error( "invalid parameter" ); }
#endif
