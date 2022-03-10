/* $Id: para_protos.h 15456 2022-03-09 15:06:35Z greg $ */

/************************************************************************/
/*	para_protos.h - PARASOL library prototype and macro file	*/
/*									*/
/*	Copyright (C) 1993 School of Computer Science, 			*/
/*		Carleton University, Ottawa, Ont., Canada		*/
/*		Written by John Neilson					*/
/*									*/
/*  This library is free software; you can redistribute it and/or	*/
/*  modify it under the terms of the GNU Library General Public		*/
/*  License as published by the Free Software Foundation; either	*/
/*  version 2 of the License, or (at your option) any later version.	*/
/*									*/
/*  This library is distributed in the hope that it will be useful,	*/
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU	*/
/*  Library General Public License for more details.			*/
/*									*/
/*  You should have received a copy of the GNU Library General Public	*/
/*  License along with this library; if not, write to the Free		*/
/*  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.	*/
/*									*/
/*  The author may be reached at neilson@scs.carleton.ca or through	*/
/*  the School of Computer Science at Carleton University.		*/
/*									*/
/*	Created: 09/06/93 (JEN)						*/
/*	Revised: 08/04/94 (JEN) Added protos for ps_receive_last and 	*/
/*				ps_receive random and macro LIFO.	*/
/*		 27/04/94 (JEN) Added protos for ps_idle_cpu,		*/
/*				ps_ready_queue, ps_build_buffer_pool,	*/
/*				ps_free_buffer, ps_get_buffer, and	*/
/*				ps_buffer_size.				*/
/*		 26/07/94 (JEN) Removed packetizing from ps_build_bus	*/
/*				and ps_build_link.			*/
/*		 06/10/94 (JEN) Corrected the NOUSER flag to CUSTOM on 	*/
/*				add_event and remove_event.		*/
/*		 07/12/94 (JEN)	Fixed globals with extern's.		*/
/*		 09/12/94 (JEN) Changed anmes with ps prefix to minimize*/
/*				name clashes.				*/
/*		 27/06/95 (PRM) Added #ifdef __cplusplus code for C++ 	*/
/*				compatability.	Changed the declarations*/
/*				of ps_create and ps_create2.		*/
/*		 15/06/99 (WCS) run_time, ts_flag and ts_report now     */
/*				externally available			*/
/*									*/
/************************************************************************/

#ifndef	_PARA_PROTOS
#define	_PARA_PROTOS

#include <parasol/para_types.h>

/* Functions defined here for compiling on Windows NT.			*/
#if !defined(HAVE_DRAND48)
#include "drand48.h"
#endif

/* #define LQX_DEBUG set to show r_util growth bug */

#define BAD_PARAM(param) bad_param_helper(__FUNCTION__, (param))
#define BAD_CALL(message) bad_call_helper(__FUNCTION__, (message))

extern int	bad_param_helper(

/* Prints a message saying function was called with an invalid value	*/
/* for param.								*/

	const char *function, 			/* function name	*/
	const char *param			/* parameter name	*/
);

/************************************************************************/

extern int	bad_call_helper(

/* Prints a message saying function was called with an invalid value	*/
/* for param.								*/

	const char *function, 			/* function name	*/
	const char *message			/* error message	*/
);


extern	ps_table_t	ps_stat_tab;

/************************************************************************/
/*                 P A R A S O L   M A C R O S	 &   C O D E S		*/
/************************************************************************/

/* 	Macros								*/

#define ps_choice(n)		((long)((n)*drand48()))
#define	ps_exponential(mean)	((double)(-((mean)*log(drand48())))) 
#define	ps_random		(drand48())
#define	ps_uniform(low, high)   ((low) + ((high) - (low))*drand48())

#define	ps_my_name		(ps_htp->name)
#define	ps_my_node		(ps_htp->node)
#define	ps_my_host		(ps_htp->uhost)
#define	ps_my_priority		(ps_htp->upriority)
#define	ps_my_parent		(ps_htp->parent)
#define	ps_my_std_port		(ps_htp->bport)
#define ps_my_schedule_time	(ps_htp->sched_time)
#define ps_my_end_compute_time	(ps_htp->end_compute_time)

#define	ps_myself ((((char *)ps_htp)-ps_task_tab.base)/ps_task_tab.entry_size)

#define	ps_task_name(task)	(ps_task_ptr((task))->name)
#define	ps_task_state(task)	(ps_task_ptr((task))->state)
#define	ps_task_node(task)	(ps_task_ptr((task))->node)
#define	ps_task_host(task)	(ps_task_ptr((task))->uhost)
#define	ps_task_priority(task)	(ps_task_ptr((task))->upriority)
#define	ps_task_parent(task)	(ps_task_ptr((task))->parent)
#define	ps_std_port(task)	(ps_task_ptr((task))->bport)
#define ps_schedule_time(task)	(ps_task_ptr((task))->sched_time)
#define ps_end_compute_time(task)  (ps_task_ptr((task))->end_compute_time) /* tomari quorum */
#define ps_preempted_time(task)	(ps_task_ptr((task))->pt_sum)	/*Added by Tao for task preemption time*/

#define	ps_task_ptr(id)	((ps_task_t*)(ps_task_tab.base+(id)*ps_task_tab.entry_size))
#define	stat_ptr(id)	((ps_stat_t*)(ps_stat_tab.base+(id)*ps_stat_tab.entry_size)) 
#define ps_declare_glocal(set, type) (ps_allocate_glocal((set), sizeof(type)))
#define ps_glocal(set, index, type) (*((type*)ps_glocal_value((set),(index))))

/*	Queueing discipline codes					*/

#define	FCFS	0			/* first come first served	*/
#define	FIFO	0			/* first in first out		*/
#define	HOL	1			/* head of the line (priority)	*/
#define	NP	1			/* non-preemptive priority	*/
#define	PR	2			/* preemptive resume priority	*/
#define	RAND	3			/* random			*/
#define	LIFO	4			/* last in first out		*/
#define CFS 	5			/* Completely Fair Share*/

/* 	Task states and flags						*/

#define	TASK_FREE		0
#define	TASK_SUSPENDED		1
#define	TASK_READY		2
#define	TASK_HOT		3
#define	TASK_RECEIVING		4
#define	TASK_SLEEPING		5
#define	TASK_SYNC		6
#define	TASK_SYNC_SUSPEND	7
#define	TASK_SYNC_FREE		8
#define	TASK_SPINNING		9
#define	TASK_COMPUTING		10
#define	TASK_BLOCKED		11

/*	Null values							*/

#ifndef	NULL
#define	NULL		(0)
#endif
#define	NULL_NODE	(-1)
#define	NULL_HOST	(-1)
#define	NULL_BUS	(-1)
#define	NULL_LINK	(-1)
#define	NULL_TASK	(-1)
#define	NULL_PORT	(-1)
#define	NULL_LOCK	(-1)

/* 	Miscellaneous Constants						*/

#define	ANY_HOST	(-1)
#define	MAX_PRIORITY	(1000)
#define MIN_PRIORITY	(0)
#define	NEVER		(0.0)
#define	IMMEDIATE	(-1.0)
#define	SYSERR		(-1)
#define	OK		(1)
#ifndef TRUE
#define	TRUE		(1)
#endif
#ifndef	FALSE
#define	FALSE		(0)
#endif
#define	SYSCALL		int
#define	SAMPLE		87264502
#define	VARIABLE	29382731
#define RATE		43928290

/*	Node utilization statistic flags				*/

#define SF_PER_HOST	0x1			/* per host		*/
#define SF_PER_NODE	0x2			/* aggregate per node	*/
#define SF_PER_TASK_HOST 0x4			/* per task cpu		*/
#define SF_PER_TASK_NODE 0x8			/* per task node	*/

/*	Scheduler Notification Message Types				*/

#define SN_READY 	1			/* task is ready	*/
#define SN_IDLE 	2			/* catcher executed	*/
#define SN_USER		10			/* user defined		*/

/*	ps_run_parasol run-time flags					*/

#define RPF_TRACE	0x01			/* Trace flag		*/
#define RPF_STEP	0x02			/* Step(debugger) flag	*/
#define RPF_WARNING	0x04			/* warning flag		*/

/************************************************************************/
/*                 P A R A S O L   G L O B A L S			*/
/************************************************************************/

extern 	ps_task_t	*ps_htp;		/* HOT task pointer	*/
extern 	double		ps_now;			/* current time		*/
extern	ps_table_t	ps_task_tab;		/* task table		*/
extern	long		ts_flag;		/* trace state flag	*/
extern  double		ps_run_time;		/* stop time		*/

/************************************************************************/
/*                 P A R A S O L   P R O T O T Y P E S			*/
/************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/************************************************************************/
/*		Special PARASOL Task Classes				*/
/************************************************************************/

extern	void	ps_genesis(void *);

/************************************************************************/
/*		Task Management Related SYSCALLS			*/
/************************************************************************/

extern	SYSCALL	ps_adjust_priority(

/* Alters the sheduling priority of a specified "task". The target task	*/
/* must be a descendant of the caller or the caller itself.  This call	*/
/* may cause instantaneous task resheduling.				*/

	long	task,				/* task id 		*/
	long	priority			/* task priority	*/
);

/************************************************************************/

extern	SYSCALL ps_awaken(

/* Awakens the specified task if sleeping; otherwise call is ignored.	*/

	long	task				/* task id 		*/
);

/************************************************************************/

extern	SYSCALL	ps_buses(

/* Returns the buses accessible by the specified task.			*/

	long	task,				/* task id		*/
	long	*nbp,				/* # buses pointer	*/
	long	bus_array[]			/* bus array		*/
);

/************************************************************************/

extern	SYSCALL	ps_children(

/* Returns the children (i.e., direct descendants) of the caller.	*/

	long	*ncp,				/* # children pointer	*/
	long	*child_array			/* array of children	*/
);

/************************************************************************/

extern	SYSCALL	ps_compute(	

/* Retains processor for "delta" (scaled) cpu time. Interruptions are 	*/
/* permitted (i.e., PR or RR scheduling).				*/

	double	delta				/* cpu delta time	*/
);

/************************************************************************/

extern	SYSCALL	ps_create(

/* Creates a named task in the TASK_SUSPENDED state on a specified 	*/
/* "node" and "host" cpu processor. The host may be ANY_HOST.  "Class"	*/
/* refers to the C function serving as the main function of the task.	*/
/* No arguments are passed.  The sheduling priority of the task is 	*/
/* given by "priority".							*/

	const 	char	*name,			/* task name		*/
	long	node,				/* node location index	*/
	long 	host,				/* host cpu id index	*/
	void	(*code)(void *),		/* code pointer		*/
	long	priority			/* task priority	*/
);

/************************************************************************/

extern	SYSCALL	ps_create2(

/* Creates a named task in the TASK_SUSPENDED state on a specified 	*/
/* "node" and "host" cpu processor. The host may be ANY_HOST.  "Class"	*/
/* refers to the C function serving as the main function of the task.	*/
/* No arguments are passed.  The sheduling priority of the task is given*/
/* by "priority". The execution stack is of size "stacksize" bytes.	*/

	const	char	*name,			/* task name		*/
	long	node,				/* node location index	*/
	long 	host,				/* host cpu id index	*/
	void	(*code)(void *),		/* code pointer		*/
	long	priority,			/* task priority	*/
	long 	group,				/* group index		*/
	double	stackscale			/* stack size scaler	*/
);

/************************************************************************/

extern	SYSCALL	ps_create_group(

/* Creates a named task in the TASK_SUSPENDED state on a specified 	*/
/* "node" and "host" cpu processor. The host may be ANY_HOST.  "Class"	*/
/* refers to the C function serving as the main function of the task.	*/
/* No arguments are passed.  The sheduling priority of the task is 	*/
/* given by "priority".							*/

	const 	char	*name,			/* task name		*/
	long	node,				/* node location index	*/
	long 	host,				/* host cpu id index	*/
	void	(*code)(void *),		/* code pointer		*/
	long	priority,			/* task priority	*/
	long 	group				/* group index 		*/
);

/************************************************************************/

extern	SYSCALL	ps_hold(

/* Retains processor for "delta" (scaled) cpu time.  Intended for use in*/
/* dedicated processing where the processor is also retained afterwards.*/
/* This call is suitable and more efficient for parallel system 	*/
/* architectures with one task per processor.				*/

	double	delta				/* cpu time delta	*/
);

/************************************************************************/

extern	SYSCALL	ps_idle_cpu(

/* Returns the number of idle cpu hosts for specified node.		*/

	long	node				/* node index		*/
);

/************************************************************************/

extern	SYSCALL	ps_kill(

/* Kills a task and its descendants freeing resources as it does so.	*/

	long	task				/* task id 		*/
);

/************************************************************************/

extern	SYSCALL	ps_migrate(

/* Migrates a task to a new "node"/"host" processor combo. All locks	*/
/* are lost. The target task must be a descendant of the caller or the 	*/
/* caller itself. As well, a syncing task cannot be migrated. Any 	*/
/* port set surrogate tasks are migrated with the target.		*/
 
	long	task,				/* task id 		*/
	long	node,				/* node id 		*/
	long	host				/* host id 		*/
);

/************************************************************************/

SYSCALL ps_ready_queue(

/* Returns the length as well as the contents of the ready queue	*/

	long	node,
	long	size,
	long	*tasks
);

/************************************************************************/

extern	SYSCALL ps_receive_links(

/* Returns the receive links accessible by the specified task.		*/
 
	long	task,				/* task id		*/
	long	*nlp,				/* # links pointer	*/
	long	link_array[]			/* link array		*/
);

/************************************************************************/

extern	SYSCALL ps_resume(

/* Resumes a task in the TASK_SUSPENDED state allowing it to continue.	*/
/* N.B. Tasks resume execution where they were suspended but without 	*/
/* any lock ownership.							*/

	long	task				/* task id 		*/
);

/************************************************************************/

extern	SYSCALL	ps_send_links(

/* Returns the send links accessible by the specified task.		*/

	long	task,				/* task id		*/
	long	*nlp,				/* # links pointer	*/
	long	link_array[]			/* link array		*/
);

/************************************************************************/

extern	SYSCALL	ps_siblings(

/* Returns the siblings of the caller.					*/

	long	*nsp,				/* # siblings pointer	*/
	long	*sibling_array			/* array of siblings	*/
);

/************************************************************************/

extern	SYSCALL	ps_sleep(

/* Causes the caller to enter a self-induced TASK_SLEEPING state for a	*/
/* specified "duration".  A negative duration is ignored.		*/

	double	duration			/* sleep period		*/
);

/************************************************************************/

extern	SYSCALL	ps_suspend(

/* Suspends a task releasing any locks it may hold.  A task in the 	*/
/* TASK_SYNC state will enter the TASK_SYNC_SUSPEND state; otherwise 	*/
/* tasks become TASK_SUSPENDED. If self suspension is involved, the 	*/
/* scheduler is invoked. 						*/

	long	task				/* task id 		*/
);

/************************************************************************/

extern	SYSCALL	ps_sync(	

/* Retains processor for "delta" (scaled) cpu time w/o interruption.	*/
/* PR and RR sheduling are postponed until delta time expires.		*/

	double	delta				/* cpu time delta	*/
);

/************************************************************************/
/* 		Message Passing Related SYSCALLS			*/
/************************************************************************/

extern	SYSCALL	ps_allocate_port(	

/* Allocates a named port to a specified existing task. 		*/

	const	char 	*name,			/* port name		*/
	long	task				/* task id 		*/
);

/************************************************************************/

extern	SYSCALL ps_allocate_port_set(

/* Allocates a named port set (port) to the calling task.		*/

	const	char	*name			/* port set name	*/
);

/************************************************************************/

extern	SYSCALL	ps_allocate_shared_port(

/* Allocates a named port as a shared port. The port is owned by a 	*/
/* shared port dispatcher task sired by the caller.			*/

	const	char	*name			/* shared port name	*/
);

/************************************************************************/

extern	SYSCALL ps_broadcast(
		
/* Globally broadcasts a message without use of a bus or a link.	*/

	long	type,				/* message type		*/
	char	*text,				/* message text	pointer	*/
	long	ack_port			/* acknowledge port	*/
);

/************************************************************************/

extern	SYSCALL ps_buffer_size(			

/* Returns the size of a buffer if it is a member of a pool.		*/

	char	*bp				/* buffer pointer	*/
);

/************************************************************************/

extern	SYSCALL	ps_build_buffer_pool(

/* Creates an empty buffer pool having buffers of the specified size.	*/

	long	size				/* buffer size		*/
);

/************************************************************************/

extern	SYSCALL ps_bus_send(

/* Sends a message via "bus" to "port". The message type, logical size, */
/* text, and acknowledge port are passed in the remaining arguments.	*/

	long	bus,				/* bus used		*/
	long	port,				/* target port		*/
	long	type,				/* message type		*/
	long 	size,				/* message size (bytes)	*/
	char	*text,				/* message text pointer	*/
	long	ack_port			/* acknowledge port	*/
);

/************************************************************************/

extern	SYSCALL	ps_free_buffer(

/* Frees a buffer, returning it to the appropriate buffer pool.		*/

	char	*bp				/* buffer pointer	*/
);

/************************************************************************/

extern 	char	*ps_get_buffer(

/* Gets a buffer from the specified buffer pool.			*/

	long	pool				/* buffer pool id	*/
);

/************************************************************************/

extern	SYSCALL	ps_join_port_set(

/* Adds an existing owned port to a specified port set.			*/

	long	port_set,			/* port set id		*/
	long	port				/* added port id	*/
);

/************************************************************************/

extern	SYSCALL	ps_leave_port_set(

/* Removes a port from the specified port set & returns it to caller.	*/

	long	port_set,			/* port set id		*/
	long	port				/* removed port		*/
);

/************************************************************************/

extern	SYSCALL	ps_link_send(

/* Sends a message via "link" to "port". The message type, logical size,*/
/* text, and acknowledge port are passed in the remaining arguments.	*/

	long	link,				/* link used		*/
	long	port,				/* target port		*/
	long	type,				/* message type		*/
	long	size,				/* message size (bytes)	*/
	char	*text,				/* message text	pointer	*/
	long	ack_port			/* acknowledge port	*/
);

/************************************************************************/

extern	SYSCALL ps_localcast(		

/* Broadcasts a message to all tasks on caller's node without use of a 	*/
/* bus or a link.							*/

	long	type,				/* message type		*/
	char	*text,				/* message text	pointer	*/
	long	ack_port			/* acknowledge port	*/
);

/************************************************************************/

extern	SYSCALL ps_multicast(	

/* Broadcasts a message to descendants only without use of a bus or a 	*/
/* link.								*/

	long	type,				/* message type		*/
	char	*text,				/* message text	pointer	*/
	long	ack_port			/* acknowledge port	*/
);

/************************************************************************/

extern	SYSCALL	ps_my_ports(	

/* Returns ports owned by caller excluding those in port sets. 		*/

	long	*pcountp,			/* port count pointer	*/
	long	port_array[]			/* port array		*/
);

/************************************************************************/

extern	SYSCALL	ps_owner(

/* Returns the task index of the owner of the specified port.		*/

	long	port				/* port index		*/
);

/************************************************************************/

extern	SYSCALL	ps_pass_port( 		

/* Transfers a "port" to another "task".  Caller must be port owner or 	*/
/* an ancestor of the owner.  All existing queued messages are retained.*/
/* "Port set" ports cannot be passed.					*/

	long	port,				/* port id		*/
	long	task				/* task id 		*/
);

/************************************************************************/

extern	SYSCALL ps_receive(

/* Receives a message from an owned "port". If several messages are 	*/
/* present, the one waiting the longest is selected. If a positive 	*/
/* "time_out" is specified the caller will unblock receiving a timeout 	*/
/* message after the time out delay has elapsed.  Time_out values of 	*/
/* NEVER mean that the caller blocks until a message is received while 	*/
/* IMMEDIATE values mean that no blocking occurs even if no messages are*/
/* available.  The remaining arguments return via pointers the message 	*/
/* type, time-stamp, text pointer, and acknowledge port.		*/

	long	port,				/* port id index	*/
	double	time_out,			/* receive timeout	*/
	long	*typep,				/* type pointer		*/
	double	*tsp,				/* time stamp pointer	*/
	char	**texth,			/* text handle		*/
	long	*app				/* acknowledge port ptr	*/
);

/************************************************************************/

extern 	SYSCALL ps_receive_last(

/* Receives a message from an owned "port". If several messages are	*/
/* present, the latest one is selected. If a positive "time_out" is	*/
/* specified the caller will unblock receiving a timeout message after	*/
/* the time out delay has elapsed.  Time_out values of NEVER mean that	*/
/* the caller blocks until a message is received while IMMEDIATE values	*/
/* mean that no blocking occurs even if no messages are available.  The	*/
/* remaining arguments return via pointers the message type, time-stamp,*/
/* text pointer, and acknowledge port.					*/

	long	port,				/* port id index	*/
	double	time_out,			/* receive timeout	*/
	long	*typep,				/* type pointer		*/
	double	*tsp,				/* time stamp pointer	*/
	char	**texth,			/* text handle		*/
	long	*app				/* acknowledge port ptr	*/
);

/************************************************************************/

SYSCALL ps_receive_random(

/* Receives a message from an owned "port". If several messages are	*/
/* present, one is selected at random. If a positive "time_out" is	*/
/* specified the caller will unblock receiving a timeout message after	*/
/* the time out delay has elapsed.  Time_out values of NEVER mean that	*/
/* the caller blocks until a message is received while IMMEDIATE values	*/
/* mean that no blocking occurs even if no messages are available.  The	*/
/* remaining arguments return via pointers the message type, time-stamp,*/
/* text pointer, and acknowledge port.					*/

	long	port,				/* port id index	*/
	double	time_out,			/* receive timeout	*/
	long	*typep,				/* type pointer		*/
	double	*tsp,				/* time stamp pointer	*/
	char	**texth,			/* text handle		*/
	long	*app				/* acknowledge port ptr	*/
);

/************************************************************************/

SYSCALL ps_receive_priority(

/* Receives a message from an owned "port". If several messages are	*/
/* present, the one with highest priority is selected. If a positive 	*/
/* "time_out" is specified the caller will unblock receiving a timeout 	*/
/* message after the time out delay has elapsed.  Time_out values of 	*/
/* NEVER mean that the caller blocks until a message is received while  */
/* IMMEDIATE values mean that no blocking occurs even if no messages 	*/
/* are available.  The remaining arguments return via pointers the 	*/
/* message type, time-stamp, text pointer, and acknowledge port.	*/

	long	port,				/* port id index	*/
	double	time_out,			/* receive timeout	*/
	long	*typep,				/* type pointer		*/
	double	*tsp,				/* time stamp pointer	*/
	char	**texth,			/* text handle		*/
	long	*app				/* acknowledge port ptr	*/
);

/************************************************************************/

extern	SYSCALL	ps_receive_shared(

/* Receives a message from a shared "port". If a positive "time_out" is	*/
/* specified the caller will unblock receiving a timeout message after	*/
/* the time out delay has elapsed.  Time_out values of NEVER mean that	*/
/* the caller blocks until a message is received. The remaining 	*/
/* arguments return via pointers the message type, time-stamp, text 	*/
/* pointer, and acknowledge port.					*/

	long	port,				/* port id index	*/
	double	time_out,			/* receive timeout	*/
	long	*typep,				/* type pointer		*/
	double	*tsp,				/* time stamp pointer	*/
	char	**texth,			/* text handle		*/
	long	*app				/* acknowledge port ptr	*/
);

/************************************************************************/

extern	SYSCALL	ps_release_port(		

/* Deallocates a specified "port" from caller. All queued messages are 	*/
/* lost with possible memory leak if caller was to free text string.	*/
/* If port is a "port set" port, any surrogates are killed along with 	*/
/* all ports and messages within the set.				*/

	long	port				/* port id		*/
);

/************************************************************************/

extern	SYSCALL	ps_release_shared_port(

/* Deallocates the specified shared "port" by killing its dispatcher 	*/
/* host. To be successful the caller must be the task which allocated	*/
/* the shared port or an ancestor of it. All queued messages are lost	*/
/* with a possible memory leak if the recipient was to free text string.*/

	long	port				/* shared port id	*/
);

/************************************************************************/

extern	SYSCALL	ps_resend(

/* Sends a message to "port" without use of a bus or a link.  The type,	*/
/* timestamp, text, and acknowledge port are passed in the remaining 	*/
/* arguments.	Typically used for redirecting a received message and	*/
/* preserving its timestamp.						*/

	long	port,				/* target port		*/
	long	type,				/* message type		*/
	double	ts,				/* message timestamp	*/
	char	*text,				/* message text pointer	*/
	long	ack_port			/* acknowledge port	*/
);

/************************************************************************/

extern	SYSCALL	ps_send(

/* Sends a message to "port" without use of a bus or a link.  The type,	*/
/* text, and acknowledge port are passed in the remaining arguments.	*/

	long	port,				/* target port		*/
	long	type,				/* message type		*/
	char	*text,				/* message text pointer	*/
	long	ack_port			/* acknowledge port	*/
);

/************************************************************************/

extern	SYSCALL	ps_send_priority(

/* Sends a message to "port" without use of a bus or a link.  The type,	*/
/* text, acknowledge port and priority are passed in the remaining arguments.	*/
/* Priority messages must be received with ps_receive_priority.		*/

	long	port,				/* target port		*/
	long	type,				/* message type		*/
	char	*text,				/* message text pointer	*/
	long	ack_port,			/* acknowledge port	*/
	long	priority			/* priority of message	*/
);

/************************************************************************/
/* 		Synchronization Related SYSCALLS			*/
/************************************************************************/

extern	SYSCALL	ps_lock(

/* Acquires the specified "lock" waiting by spinning if necessary.	*/

	long	lock				/* lock id index	*/
);

/************************************************************************/

extern	SYSCALL	ps_reset_semaphore(

/* Resets a specified semaphore to a count value clearing task queue.	*/

	long	sid,				/* semaphore id		*/
	long	counter				/* semaphore counter	*/
);

/************************************************************************/

extern	SYSCALL ps_signal_semaphore(

/* Signals a specified semaphore possibly resuming a queued task.	*/

	long	sid				/* semaphore id		*/
);

/************************************************************************/

extern	SYSCALL	ps_unlock(

/* Releases the specified "lock" if held by the caller or one of its 	*/
/* descendants.								*/

	long	lock				/* lock id index	*/
);

/************************************************************************/

extern	SYSCALL	ps_wait_semaphore(

/* Waits on a specified semaphore if its count value is <= 0.		*/

	long	sid				/* semaphore id		*/
);

/************************************************************************/
/* 		Statistics Related SYSCALLS				*/
/************************************************************************/

extern	SYSCALL	ps_block_stats(	

/* Collects and reports blocked statistics by blocking the simulation 	*/
/* run into "nb" blocks after allowing for transients to die in "delay".*/

	long	nb,				/* number of blocks	*/
	double	delay				/* transient delay	*/
);

/************************************************************************/

extern	SYSCALL	ps_get_stat(

/* Returns the mean and either the number of observations (SAMPLE) or	*/
/* the observation period (VARIABLE) of the specified statistic.	*/

	long	stat,				/* statistics index	*/
	double	*meanp,				/* mean pointer		*/
	double	*osp				/* other stat pointer	*/
);

/************************************************************************/

extern	SYSCALL	ps_open_stat(

/* Opens & initializes a statistic. 					*/

	const	char	*name,			/* statistic name	*/
	long	type				/* type of statistic	*/
);

/************************************************************************/

SYSCALL	ps_record_rate_stat(

/* Records a statistic sample or value.					*/

	long	stat				/* statistic index	*/
);

/************************************************************************/

extern long ps_get_node_stat_index(
    
/* Return the index to the internal node utilization statistic. */

	long node_id				/* Node id 		*/
);	

/************************************************************************/

#if !defined(__WINNT__) && !defined(__CYGWIN__)
static inline
#endif
SYSCALL  ps_record_stat(
/* Records a statistic sample or value.					*/

	long	stat,				/* statistics index	*/
	double	value				/* sample | value	*/
)
#if !defined(__WINNT__) && !defined(__CYGWIN__)
{
	ps_stat_t	*sp;			/* statistics pointer	*/
	double 	temp;				/* temporary		*/
	double	delta;				/* time delta		*/

	if(stat < 0 || stat >= ps_stat_tab.used)
	  /*	return(BAD_PARAM("stat")); */
	        return(SYSERR);

	sp = stat_ptr(stat);

	switch( sp->type ) {
	
	case SAMPLE:	
		(sp->values.sam.count)++;
		sp->resid += value;
		temp = sp->values.sam.sum + sp->resid;
		sp->resid += (sp->values.sam.sum - temp);
		sp->values.sam.sum = temp;
		break;

	case VARIABLE:

		if((delta = ps_now - sp->values.var.old_time) == 0.0) {
			sp->values.var.old_value = value;
			return(OK);
		}
		sp->resid += (delta * sp->values.var.old_value);
		temp = sp->values.var.integral + sp->resid;
		sp->resid += (sp->values.var.integral - temp);
		sp->values.var.integral = temp; 
		sp->values.var.old_time = ps_now;
		sp->values.var.old_value = value;
		break;

	default:
		
	  /* return(BAD_CALL("Only works for VARIABLE and SAMPLE statistics")); */
	     return(SYSERR);
	}

	return(OK);
}
#endif
;

/************************************************************************/

#if !defined(__WINNT__) && !defined(__CYGWIN__)
static inline
#endif
SYSCALL	ps_record_stat2(

/* Records a statistic sample or value.					*/

	long	stat,				/* statistic index	*/
	double	value,				/* sample | value	*/
	double  start				/* Start time.		*/
)
#if !defined(__WINNT__) && !defined(__CYGWIN__)
{
	ps_stat_t	*sp;			/* statistics pointer	*/
	double 	temp;				/* temporary		*/
	double	delta;				/* time delta		*/

	if(stat < 0 || stat >= ps_stat_tab.used)
	  /*	return(BAD_PARAM("stat")); */
	  return(SYSERR);

	switch((sp = stat_ptr(stat))->type) {
	
	case SAMPLE:	
		(sp->values.sam.count)++;
		sp->resid += value;
		temp = sp->values.sam.sum + sp->resid;
		sp->resid += (sp->values.sam.sum - temp);
		sp->values.sam.sum = temp;
		break;

	case VARIABLE:

		if((delta = start - sp->values.var.old_time) == 0.0) {
			sp->values.var.old_value = value;
			return(OK);
		}
		sp->resid += (delta * sp->values.var.old_value);
		temp = sp->values.var.integral + sp->resid;
		sp->resid += (sp->values.var.integral - temp);
		sp->values.var.integral = temp; 
		sp->values.var.old_time = start;
		sp->values.var.old_value = value;
		break;

	default:
		
	  /* return(BAD_CALL("Only works for VARIABLE and SAMPLE statistics")); */
	  return(SYSERR);
	}

	return(OK);
}
#endif
;

/************************************************************************/

extern	SYSCALL	ps_reset_stat(

/* Resets (zeroes) a specified statistic.				*/

	long	stat				/* statistics index	*/
);


/************************************************************************/

extern SYSCALL	ps_reset_all_stats(void);

/* Resets (zeroes) all statistics.					*/

/************************************************************************/

extern	void	ps_stats(void);

/* Reports all statistics in tabular form.				*/

/************************************************************************/

extern SYSCALL	ps_add_stat(

/* Adds a number to a statistic sample 	*/

	long	stat,				/* statistic index	*/
	double	value				/* sample | value	*/
);



/************************************************************************/
/* 		Miscellaneous SYSCALLS					*/
/************************************************************************/

/************************************************************************/

extern	void	ps_abort(

/* Writes PARASOL abort message to stderr and then aborts.		*/

	const	char	*string			/* abort string		*/
);

/************************************************************************/

extern	SYSCALL ps_build_bus(

/* Constructs a named bus connecting two or more specified nodes with a */
/* given packet size "packet_size", transmission rate "trans_rate", and */
/* queueing discipline "discipline".  Queueing may be FCFS or random.  	*/
/* Utilization statistics are collected if "sf" is set.			*/

	const	char	*name,			/* bus name		*/
	long	ncount,				/* node count		*/
	long	*node_array,			/* array of nodes 	*/
	double	trans_rate,			/* transmission rate	*/
	long	discipline,			/* queueing discipline	*/
	long	sf				/* statistics flag	*/
);

/************************************************************************/

extern	SYSCALL ps_build_link(

/* Constructs a one-way link between "source" and "destination" nodes 	*/
/* with a specified packet size "packet_size" and transmission rate	*/
/* "trans_rate".  Queueing is strictly FCFS.  Utilization statistics	*/
/* are collected if "sf" is set.					*/

	const	char	*name,			/* link name		*/
	long	source,				/* source node id 	*/
	long	destination,			/* destination node id	*/
	double	trans_rate,			/* transmission rate	*/
	long	sf				/* statistics flag	*/
);

/************************************************************************/

extern SYSCALL ps_build_node(

/* Constructs a named node with "ncpu" processors with a speed factor	*/
/* "speed" which scales the durations of TASK_COMPUTING, TASK_SYNC and 	*/
/* TASK_BLOCKED states. The queueing discipline is specified by 	*/
/* "quantum" & "discipline" ( a non-zero quantum specifies the time 	*/
/* slice for Round Robin scheduling).  Individual processor utilization */
/* statistics are collected if "sf" is set.				*/

	const	char	*name,			/* node name		*/
	long	ncpu,				/* node size - # cpu's	*/
	double	speed,				/* cpu speed factor	*/
	double	quantum,			/* quantum size		*/
	long	discipline,			/* queueing discipline	*/
	long	sl				/* statistics level	*/
);	

/************************************************************************/

extern SYSCALL ps_build_node2(

/* Constructs a named node with "ncpu" processors with a speed factor	*/
/* "speed" which scales the durations of TASK_COMPUTING, TASK_SYNC and 	*/
/* TASK_BLOCKED states. The quantum duration is specified by quantum, 	*/
/* and the user defined scheduling task is specified by "scheduler"	*/  

	const	char	*name, 
	long	ncpu, 
	double	speed, 
 	void 	(*scheduler)(void *),
	long	sl
);
/************************************************************************/
SYSCALL ps_build_node_cfs( 

/* Constructs a cfs_rq for the node whose discipline is CFS.		*/
/*  If ngroup>0, construct a group cfs_rq for each group 		*/ 

	long	node				/* node size - # cpu's	*/
);
/************************************************************************/
/* WCS - sep, 2008 - Added , build a group */ 
SYSCALL ps_build_group( 

	const	char	*name,			/* group name		*/
	double	group_share,			/* group share		*/
	long	node,				/* node id		*/
	long 	cap				/* cap flag		*/

);
/************************************************************************/

extern	SYSCALL	ps_curr_priority (

/* Returns the current priority of the cpu on the host.		*/

	long	node,				/* node id index	*/
	long	host				/* host id index	*/
);

/************************************************************************/

extern	SYSCALL	ps_headroom(void);

/* Checks the headroom remaining on the stack of the caller.		*/

/************************************************************************/

extern	double	ps_erlang(

/* Generates a variate from an Erlang distribution			*/

	double	mean,				/* distribution mean	*/
	long	kernel				/* erlang kernel	*/
);

/************************************************************************/

extern SYSCALL	ps_run_parasol( 

/* Runs PARASOL using supplied parameters and flags			*/

	double	duration,			/* simulation duration	*/
	long	seed,				/* random number seed	*/
	long	flags				/* run-time flags	*/
);

/************************************************************************/

SYSCALL	ps_schedule(

/* For use by user defined scheduling functions only!!! schedules the	*/
/* specified task to run on the specified host (immediately).		*/

	long	task,
	long	host
);

/************************************************************************/
/*		Angio Tracing Syscalls  				*/
/************************************************************************/

SYSCALL ps_enable_angio_tracing (

/*	Enables angio tracing, and opens the specified angio trace file	*/
/*	This function should only be called once, BEFORE any calls to	*/
/*	ps_create or ps_create2.					*/

	const	char	*trace_filename		/* Trace file name	*/
);

/************************************************************************/

SYSCALL ps_inject_trace_name (

/*	Injects a new dye into the calling task.			*/

	const	char	*name			/* base trace name	*/
);
 

/************************************************************************/

SYSCALL ps_task_cycle_begin (void);

/*	Logs a wCycle event.						*/
 
/************************************************************************/

SYSCALL ps_toggle_angio_output(

/* Allows the user to turn logging of angio trace events on or off	*/

	long	value				/* flag value		*/
);

/************************************************************************/

SYSCALL ps_log_user_event (

/*	Logs an event specified by the user				*/

	const	char	*event
);

/************************************************************************/
/*		Glocal Variable related SYSCALLs			*/
/************************************************************************/

SYSCALL ps_allocate_glocal (

/*	Allocates a glocal variable width bytes in size in the		*/
/*	environment with identifier env.  All subscribers to that	*/
/*	environment will get their own copy of the variable.		*/

	long 	env,				/* environment id	*/
	long	width				/* data size in bytes	*/
);

/************************************************************************/

SYSCALL	ps_share_glocal (

/*	Allows the caller to share a copy of all the glocal variables  	*/
/*	in the environment with identifier env with the given task	*/

	long 		env,			/* environment id	*/
	long		task			/* task 		*/
);

/************************************************************************/

SYSCALL	ps_subscribe_glocal (

/*	Gives the caller his own copy of the variable with identifier 	*/
/*	gloc in the environment with identifier env.			*/

	long 		env			/* environment id	*/
);

/************************************************************************/

void *ps_glocal_value (

/*	Returns a pointer to the calling task's copy of the glocal	*/
/*	variable with identifier gloc in the environment env. We may	*/
/*	want to consider implementing this as a macro once everything	*/
/*	is debugged, for obvious performance reasons.			*/

	long		env,			/* environment id	*/	
	long		gloc			/* variable id		*/
);

/************************************************************************/
/*		PARASOL Debugger Support Functions			*/
/************************************************************************/

void	ts_report(

/* PARASOL trace state reporter.					*/

	ps_task_t	*tp,			/* task pointer		*/
	const	char	*sp			/* string pointer	*/
);
	
/************************************************************************/
#ifdef	CUSTOM
extern	ps_event_t	*add_event(

/* Gets an event struct from the free list and sets its fields.  This	*/
/* event is then merged into the calendar according to event time and	*/
/* a pointer to it is returned. 					*/

	double	time,				/* event time		*/
	long	type,				/* primary event code	*/
	long	*gp				/* generic pointer	*/
);
#endif	
/************************************************************************/
#ifdef	CUSTOM
extern	void	remove_event(

/* Removes an event from the calendar and adds it to the free list.  It	*/
/* may be examined using its pointer however its contents are valid 	*/
/* only until the struct is reused by an add_event call. If the event	*/
/* pointer is null, nothing happens.  Guard code ensures that the event	*/
/* is in a doubly linked list and aborts the simulation if not.		*/

	ps_event_t	*ep			/* event pointer	*/
);
#endif	

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif	/* _PARA_PROTOS */

