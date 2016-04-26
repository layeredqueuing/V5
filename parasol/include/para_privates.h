/* $Id: para_privates.h 12547 2016-04-05 18:32:45Z greg $ */
/************************************************************************/
/*	para_privates.h - PARASOL library internal header file		*/
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
/*	Created: 06/07/95 (PRM)						*/
/*	History: 10/07/95 (PRM)	Added macros and declarations for 	*/
/*				glocal variable functions.		*/
/*		 16/08/95 (PRM)	Added angio_output definition for use	*/
/*				in enabling/disabling angio output	*/
/*		 15/06/99 (WCS) See changes in code marked with WCS	*/
/*		 		run_time, ts_flag and ts_report now     */
/*				externally available			*/
/*									*/
/************************************************************************/

#ifndef _PARA_PRIVATES
#define _PARA_PRIVATES

#define STACK_TESTING

#ifndef LOCAL
#define	LOCAL static	
#endif /* LOCAL */

/************************************************************************/
/*		Special PARASOL Task Classes				*/
/************************************************************************/

LOCAL	void	block_stats_collector(void *);

/************************************************************************/

LOCAL	void	catcher(void *);

/************************************************************************/

LOCAL	void	port_set_surrogate(void *);

/************************************************************************/

LOCAL	void	reaper(void *);

/************************************************************************/

LOCAL 	void	shared_port_dispatcher(void *);

/************************************************************************/
/*		PARASOL Driver Support Functions			*/
/************************************************************************/

LOCAL 	void 	driver(void);

/* Handles events and advances the global time "now" when appropriate.	*/
/* The debugger is invoked or the simulation is aborted or terminated 	*/
/* as appropriate.							*/ 

/************************************************************************/

LOCAL	void	end_block_handler(

/* Handles END_BLOCK events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

LOCAL	void	end_compute_handler(

/* Handles END_COMPUTE events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

LOCAL	void 	end_quantum_handler(

/* Handles END_QUANTUM events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

LOCAL	void	end_receive_handler(

/* Handles END_RECEIVE (receive t/o) events.				*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

LOCAL	void	end_sleep_handler(

/* Handles END_SLEEP events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

LOCAL	void	end_sync_handler(

/* Handles END_SYNC events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

LOCAL	void	end_trans_handler(

/* Handles END_TRANS events.						*/

	ps_event_t	*ep			/*event pointer		*/
);

/************************************************************************/

LOCAL	void	dq_ready(

/* Dequeues a given ready task from a given node's ready-to-run queue	*/

	ps_node_t	*np,			/* node pointer		*/
	ps_task_t	*tp			/* task pointer		*/
);

/************************************************************************/	

LOCAL	void	find_host(

/* Finds a host cpu for the given task making the task either		*/
/* TASK_COMPUTING TASK_HOT, or TASK_BLOCKED depending on whether there 	*/
/* is remaining cpu time or if the caller is the simulation driver.	*/
/* This may change the state of a running task to TASK_READY if pre-	*/
/* emption is permitted.  If unable to find a suitable host, the state	*/
/* of the input task is made TASK_READY.*/

	ps_task_t	*tp			/* task pointer		*/
);
				
/************************************************************************/

LOCAL	void	find_priority(

/* Finds ready task with priority greater than given priority and makes	*/
/* its state either TASK_COMPUTING, TASK_HOT or TASK_BLOCKED depending	*/
/* on whether there is cpu remaining or the caller is the simulation	*/
/* driver.  If none is found the current task continues as appropriate. */
/* Otherwise, the current task is preempted and made TASK_READY.	*/

	ps_node_t	*np,			/* node pointer		*/
	ps_cpu_t	*hp,			/* host pointer		*/
	long	priority			/* test priority	*/
);

/************************************************************************/
LOCAL	void	find_host_cfs(

/* Finds a host cpu for the given task making the task either		*/
/* TASK_COMPUTING, TASK_HOT, or TASK_BLOCKED depending on whether there	*/
/* is remaining cpu time or if the caller is the simulation driver.	*/
/* This may change the state of a running task to TASK_READY if pre-	*/
/* emption is permitted.  If unable to find a suitable host, the state	*/
/* of the input task is made TASK_READY.*/

	ps_task_t	*tp			/* task pointer		*/
);

/************************************************************************/
LOCAL long find_min_load_host(
/* find a host with a min load */

	ps_node_t	*np,			/* node pointer		*/
	ps_task_t	*tp			/* task pointer		*/
);

/************************************************************************/
LOCAL	void	find_priority_cfs(

/* Finds ready task with priority greater than given priority and makes	*/
/* its state either TASK_COMPUTING, TASK_HOT or TASK_BLOCKED depending	*/
/* on whether there is cpu remaining or the caller is the simulation	*/
/* driver.  If none is found the current task continues as appropriate. */
/* Otherwise, the current task is preempted and made TASK_READY.	*/

	ps_node_t	*np,			/* node pointer		*/
	ps_cpu_t	*hp			/* host pointer		*/
);

/************************************************************************/
LOCAL	void	find_ready(
		
/* Finds and dequeues a ready task making it either TASK_COMPUTING,	*/
/* TASK_HOT, or TASK_BLOCKED depending on whether there is remaining 	*/
/* cpu time or if the caller is the simulation driver.  If no runnable	*/
/* task exists, the node and host states are changed as appropriate and	*/
/* a context switch to the simulation driver is	performed(if necessary).*/

	ps_node_t	*np,			/* node pointer		*/
	ps_cpu_t	*hp			/* host cpu pointer	*/
);

/************************************************************************/
LOCAL	void	find_ready_cfs(
		
/* Finds and dequeues a ready task making it either TASK_COMPUTING,	*/
/* TASK_HOT, or TASK_BLOCKED depending on whether there is remaining 	*/
/* cpu time or if the caller is the simulation driver.  If no runnable	*/
/* task exists, the node and host states are changed as appropriate and	*/
/* a context switch to the simulation driver is	performed(if necessary).*/

	ps_node_t	*np,			/* node pointer		*/
	ps_cpu_t	*hp			/* host cpu pointer	*/

);
/************************************************************************/

LOCAL	void	init_event(void);

/* Initializes the event free list to empty and also the event calendar	*/
/* to empty.								*/

/************************************************************************/

LOCAL	ps_event_t	*next_event(void);

/* Removes the most imminent event from the calendar returning it to 	*/
/* the free list.  It may be examined by the returned pointer, however,	*/
/* its contents are volatile and cannot be trusted for long.  Should	*/
/* the calendar run dry, the null event pointer is returned.		*/

/************************************************************************/

LOCAL	void	ready(

/* Makes the given task TASK_READY and queues it on the appropriate	*/
/* ready-to-run-queue according to its priority if relevant		*/

	ps_task_t	*tp			/* task pointer		*/
);
/************************************************************************/
LOCAL	void	ready_cfs(

/* Makes the given task TASK_READY and queues it on the appropriate	*/
/* ready-to-run-queue according to its priority if relevant		*/
	long 	host,
	ps_task_t	*tp			/* task pointer		*/
);

LOCAL	void	sched(void);

/* Performs a context switch between a cooling hot task and a blocked	*/
/* task (if available).  Otherwise, the switch is to the simulation	*/
/* driver.								*/

/************************************************************************/
/*		PARASOL Dynamic Table Support Functions			*/
/************************************************************************/

LOCAL	long	free_table_entry(

/* Frees a specified dynamic table entry for reuse.			*/

	ps_table_t	*tabp,			/* table pointer	*/
	long	id				/* table entry index	*/	
);

/************************************************************************/

LOCAL	long	get_table_entry(

/* Gets a free dynamic table entry using a rover to search. Grows table */
/* by factor of 2 in size if necessary.					*/

	ps_table_t	*tabp			/* table pointer	*/
);

/************************************************************************/

LOCAL	long	init_table(

/* Initializes a dynamic table of a specified number of entries and a 	*/
/* specified entry size (in bytes).					*/

	ps_table_t	*tabp,			/* table pointer	*/
	long	tab_size,			/* initial table size	*/
	long	entry_size			/* table entry size	*/
);

/************************************************************************/
/*		PARASOL Debugger Support Functions			*/
/************************************************************************/

LOCAL	void	debug(void);					

/* PARASOL debug interpreter.						*/

/************************************************************************/

LOCAL	void	display_breakpoints(void);

/* Gives a summary of all breakpoints that are set.			*/
	
/************************************************************************/

LOCAL void event_report(void);			

/* Reports future calendar events. 					*/


/************************************************************************/

LOCAL void hard_report(void);

/* Reports hardware status.						*/
				
/************************************************************************/

LOCAL void soft_report(void);

/* Reports software status.						*/

/************************************************************************/

LOCAL	void	set_task_state(

/* Sets the state of the given task to the given state.  Also checks if	*/
/* there was a breakpolong set on that task's changing state and if so	*/
/* calls debug().							*/

	ps_task_t	*tp,
	ps_task_state_t	state
);

/************************************************************************/

LOCAL	void	task_breakpoints(void);

/* Allow user to set and delete task state breakpoints.			*/
	
/************************************************************************/
/*		PARASOL Angio Tracing Support Functions			*/
/************************************************************************/

LOCAL	long	create_dye(

/* Creates a new dye with the given base name				*/

	const	char	*base_name		/* dye name		*/
);

/************************************************************************/

LOCAL	long	derive_dye(

/* Creates a new dye which is a derivative of the original dye, with	*/
/* the same base name and occurence number, but a new serial number.	*/

	long		did
);

/************************************************************************/

LOCAL	void	end_trace(

/* Ends a single angio trace by logging a wEnd event and freeing the 	*/
/* dye.									*/

	ps_task_t	*tp
);

/************************************************************************/

LOCAL	void 	free_dye(

/* Frees the memory associated with the given dye.			*/

	long		did
);

/************************************************************************/

LOCAL 	long	get_occurence_number(

/* Returns a unique occurence number for the given base name by keeping	*/
/* track of all base names along with their occurence numbers. 		*/
/* new_base_name is filled in with a pointer to the shared occurence 	*/
/* name.								*/

	const	char	*base_name, 
	char 		**new_base_name
);

/************************************************************************/

LOCAL	void	log_angio_event(

/* Logs an angio event to the angio trace output file (angio_file).	*/

	ps_task_t	*tp,
	ps_dye_t	*dye,
	const	char	*event
);

/************************************************************************/

LOCAL	void	start_trace(

/* Starts an angio trace by assigning the given dye to the given task 	*/
/* and logging a wBegin event.						*/

	ps_task_t	*tp,
	long		did
);

/************************************************************************/
/*		PARASOL Glocal variable support functions		*/
/************************************************************************/

LOCAL	void 	glocal_init_stuff(

/*	Initializes the ps_env_tab structure and grows the table if 	*/
/*	necessary.							*/

	long		env			/* environment id	*/
);

/************************************************************************/
/*		PARASOL Miscellaneous Support Functions			*/
/************************************************************************/

LOCAL	void	adjust_priority(

/* Alters the sheduling priority of a specified "task". No error 	*/
/* checking is performed, so priority may be > MAX_PRIORITY or <	*/
/* MIN_PRIORITY. This call may cause instantaneous task resheduling.	*/

	long	task,				/* task id		*/
	long	priority			/* new priority		*/
);

/************************************************************************/

LOCAL	long 	ancestor(				

/* Tests if the caller is an ancestor of the given task.  Returns TRUE	*/
/* if the simulation driver is the caller or if the caller is the given	*/
/* task.								*/

	ps_task_t	*tp			/* task pointer		*/
);

/************************************************************************/

LOCAL	void 	dq_lock(

/* Dequeues the task pointed to by "tp" from the lock on which it is	*/
/* spinning.								*/

	ps_task_t	*tp			/* task pointer		*/
);

/************************************************************************/

LOCAL	void	free_mess(

/* Free message envelope.						*/

	ps_mess_t	*mp			/* message pointer	*/
);

/************************************************************************/

LOCAL 	void 	free_pair(

/* Stuffs a freed tp pair onto the tpflist.				*/

	ps_tp_pair_t *pp			/* tp pair pointer	*/
);

/************************************************************************/

LOCAL	long	get_dss(

/* Returns a reasonable (I hope) default stack size for the machine we 	*/
/* are running on.  Stack size is truncated so that it is a multiple of	*/
/* sizeof(double) in order to avoid alignment problems.			*/

	long	t_flag
);

/************************************************************************/

LOCAL	ps_mess_t	*get_mess(void);

/* Get message envelope.						*/

/************************************************************************/

LOCAL 	ps_tp_pair_t *get_pair(void);

/* Returns a pointer to a tp pair using tpflist if possible		*/

/************************************************************************/

LOCAL	void	init_locks(void);

/* Initializes the lock table.						*/

/************************************************************************/

LOCAL	void	init_semaphores(void);

/* Initializes the semaphore table.					*/

/************************************************************************/

LOCAL	long	lookup_port_set(

/* Finds the port set belonging to the given task which uses the	*/
/* given surrogate.							*/

	long task,
	long surrogate
);

/************************************************************************/

LOCAL	void	no_trace_rep(void);

/* This is representative of an application without tracing on		*/

/************************************************************************/

LOCAL	long	other_err_helper(

/* Prints a message saying that something was wrong in a call to a	*/
/* function.								*/

	const char *function,			/* function name	*/
	const char *message			/* error message	*/
);

/************************************************************************/

LOCAL	long	port_receive(

/* Receives a message according to port queueing discipline - i.e., 	*/
/* FIFO, LIFO, or RAND.							*/

	long	discipline,			/* queueing discipline	*/
	long	port,				/* port id index	*/
	double	time_out,			/* receive timeout	*/
	long	*typep,				/* type pointer		*/
	double	*tsp,				/* time stamp pointer	*/
	char	**texth,			/* text handle		*/
	long	*app,				/* acknowledge port ptr	*/
	long	*opp,				/* original port ptr	*/
	long	*mid,				/* message id ptr	*/
	long	*did				/* dye id ptr		*/
);

/************************************************************************/

LOCAL 	long	port_send(

/* Sends a message to "port" without use of a bus or a link.  The type,	*/
/* timestamp, text, the acknowledge and the "original" ports are passed */
/* in the remaining arguments. 						*/

	long	port,				/* target port		*/
	long	type,				/* message type		*/
	double	ts,				/* message timestamp	*/
	char	*text,				/* message text pointer	*/
	long	ack_port,			/* acknowledge port	*/
	long	oport,				/* original port	*/
	long	mid,				/* message id		*/
	long	did				/* dye id		*/
);

/************************************************************************/

LOCAL	long	relative(

/* Tests if given task is a relative of the target task.  A relative is	*/
/* defined here as either an ancestor, a sibling, or a sibling of an	*/
/* ancestor.								*/

	ps_task_t	*tp,			/* given task pointer	*/
	ps_task_t	*ttp			/* target task pointer	*/
);

/************************************************************************/

LOCAL	void release_locks(

/* Releases the locks held by the specified task (if a descendant).    	*/

	ps_task_t	*tp			/* task pointer		*/
);

/************************************************************************/

LOCAL 	void 	release_ports(

/* Releases all ports owned by the task pointed to by "tp". 		*/

	ps_task_t	*tp			/* task pointer		*/
);

/************************************************************************/

LOCAL	void	set_run_task(

/* Sets the run_task member of hp to the task id of the task specified	*/
/* by tp (hp->run_task = tid(tp)). The rest of the code is for 		*/
/* gathering statistics on the usage of each CPU on a per task basis.	*/

	ps_node_t	*np,
	ps_cpu_t	*hp,
 	ps_task_t	*tp
);

/************************************************************************/

LOCAL	int	stat_compare(

/* This is a callback function that is passed in to quicksort for 	*/
/* sorting the statistics in alphabetical order by name.		*/

	ps_stat_t 	*s1, 
	ps_stat_t 	*s2
);

#ifdef STACK_TESTING

/************************************************************************/

void	test_all_stacks(void);

/* Tests the amount of stack space used on all stacks			*/

/************************************************************************/

LOCAL	void	test_stack(

/* Tests the amount of stack space used on all stacks			*/

	long	task				/* task id		*/
);

#endif /*STACK_TESTING*/

/************************************************************************/

LOCAL	void	trace_rep(void);

/* This is representative of the PARASOL tracing function ts_report	*/
 
#if !HAVE_SIGALTSTACK || _WIN32 || _WIN64
/************************************************************************/

LOCAL	void	wrapper(

/* Wraps PARASOL tasks for startup and default termination.		*/

	mctx_t * newc,				/* new context buffer	*/
	mctx_t * oldc				/* old context buffer	*/
);
#endif


/************************************************************************/

LOCAL	long	sp_tester(
	long	*sp_dirp,			/* sp direction pointer	*/
	long	*sp_indp,			/* sp index pointer	*/
	long	*sp_deltap			/* sp delta pointer	*/
);

/************************************************************************/

#if !HAVE_SIGALTSTACK || _WIN32 || _WIN64
LOCAL	void	sp_winder(
	long	x				/* recursion index	*/
);
#else
LOCAL	long	sp_winder();
#endif

/************************************************************************/
/*		PARASOL Red Black Tree Support Functions			*/
/************************************************************************/

LOCAL  	rb_node 	*init_rb_node(void);

LOCAL	void  init_cfs_rq(ps_cfs_rq_t * rq,long node,double sched_time,sched_info *si);

LOCAL void assign_cfs_rq(ps_cfs_rq_t * rq1,ps_cfs_rq_t * rq2);

LOCAL void init_sched_info(sched_info * si, long task,ps_cfs_rq_t * own_rq,ps_cfs_rq_t * rq,double weight);

LOCAL struct rb_node * insert_rb_node( struct  ps_cfs_rq_t * rq,double key);

LOCAL void delete_rb_node(struct  ps_cfs_rq_t * rq, struct  rb_node * n);

LOCAL struct rb_node * successor(struct rb_node *n) ;

LOCAL struct rb_node *find_rb_node(struct ps_cfs_rq_t *rq, double key) ;

void print_rb_node(struct  rb_node *n );

void print_rb_tree(struct  rb_node *n);

/*void rb_set_data(struct rb_node *n,struct sched_info *si,double key); */

LOCAL void rb_set_data(struct rb_node *n,struct rb_node * p);

/************************************************************************/
/*		PARASOL CFS scheduler Support Functions			*/
/************************************************************************/



/* find the leftmost node in the ps_cfs_rq_t		*/

LOCAL sched_info * 	find_fair_si(struct ps_cfs_rq_t *rq);

/************************************************************************/

LOCAL 	long	find_fair_task(struct ps_cfs_rq_t *rq) ;
/* find the leftmost node in the ps_cfs_rq_t		*/

/************************************************************************/
LOCAL 	long	find_fair_task(struct ps_cfs_rq_t *rq) ;
/************************************************************************/
LOCAL 	long 	find_next_task(ps_task_t *tp);
/************************************************************************/
SYSCALL		find_next_fair(struct ps_cfs_rq_t *rq) ;

/************************************************************************/

/*LOCAL bool is_cfsrq_empty(struct ps_cfs_rq_t * rq); */
void print_si(sched_info *si);
void print_rq(ps_cfs_rq_t *rq);
/************************************************************************/
double get_quantum(ps_node_t *np,long host,double delta);

void 	enqueue_cfs_task(

/*Enqueue ready task into a cfs_rq */

	ps_task_t 	* tp
);
 void dq_cfs_si(sched_info *si);
 void enque_cfs_si(sched_info * si);

/************************************************************************/
/* Dequeue ready task from a cfs_rq */
void  	dq_cfs_task(	ps_task_t 	* tp);

/************************************************************************/
SYSCALL check_fair(ps_task_t *run_tp,ps_task_t *ready_tp);

/************************************************************************/
void update_run_task(ps_task_t * tp);

/* update the sched_info of the run task. this function will be called	*/
/* at end of running.							*/
	

/************************************************************************/
void update_ready_task(ps_task_t *tp);

/* update the sched_info of a ready task.		*/
	
 void update_ready(sched_info * si);
/************************************************************************/
void update_sleep_task(ps_task_t *tp);

/* update the sched_info of a sleeping task.		*/
	

/************************************************************************/
 void update_cfs_curr(ps_cfs_rq_t * cfs_rq);

/************************************************************************/
/* set this cfs task to run */
LOCAL void set_cfs_task_run(ps_task_t *tp);

/************************************************************************/

/* cooling this cfs task */
LOCAL void cooling_cfs_task(ps_task_t *tp);

void cap_handler(ps_task_t *tp);
/************************************************************************/
/*		P A R A S O L   M A C R O S   &   C O D E S		*/
/************************************************************************/

/*	Macros								*/

#define	nid(np)	((((char *)(np))-ps_node_tab.base)/ps_node_tab.entry_size)
#define	hid(np, hp) (((ps_cpu_t *)hp) - (ps_cpu_t *)(((ps_node_t *)np)->cpu))
#define	bid(bp)	((((char *)(bp))-ps_bus_tab.base)/ps_bus_tab.entry_size)
#define	lid(lp)	((((char *)(lp))-ps_link_tab.base)/ps_link_tab.entry_size)
#define	pid(pp)	((((char *)(pp))-ps_port_tab.base)/ps_port_tab.entry_size)
#define	tid(tp)	((((char *)(tp))-ps_task_tab.base)/ps_task_tab.entry_size)
#define	sid(sip)	((((char *)(sip))-ps_sched_info_tab.base)/ps_sched_info_tab.entry_size)

#define	node_ptr(id)	((ps_node_t*)(ps_node_tab.base+(id)*ps_node_tab.entry_size))
#define	group_ptr(id)	((ps_group_t*)(ps_group_tab.base+(id)*ps_group_tab.entry_size))
#define sched_info_ptr(id)	((sched_info*)(ps_sched_info_tab.base+(id)*ps_sched_info_tab.entry_size))
#define	bus_ptr(id)	((ps_bus_t*)(ps_bus_tab.base+(id)*ps_bus_tab.entry_size))
#define	link_ptr(id)	((ps_link_t*)(ps_link_tab.base+(id)*ps_link_tab.entry_size))
#define	para_stat_ptr(id)	((long*)(ps_para_stat_tab.base+(id)*ps_para_stat_tab.entry_size)) 
#define	lock_ptr(id)	((ps_lock_t*)(ps_lock_tab.base+(id)*ps_lock_tab.entry_size))
#define	sema_ptr(id)	((ps_sema_t*)(ps_sema_tab.base+(id)*ps_sema_tab.entry_size))
#define	port_ptr(id)	((ps_port_t*)(ps_port_tab.base+(id)*ps_port_tab.entry_size))
#define pool_ptr(id)	((ps_pool_t*)(ps_pool_tab.base+(id)*ps_pool_tab.entry_size))
#define ts_stat_ptr(ts_tab,id) ((long*)((ts_tab)->base+(id)*(ts_tab)->entry_size))
#define	env_ptr(id)	((ps_env_t*)(ps_env_tab.base+(id)*ps_env_tab.entry_size))
#define	var_ptr(ep,id)	((ps_var_t*)(ep->var_tab.base+(id)*ep->var_tab.entry_size))
#define	dye_ptr(id)	((ps_dye_t*)(ps_dye_tab.base+(id)*ps_dye_tab.entry_size))

#ifndef __GNUC__
#define __FUNCTION__ "unknown"
#endif /* NO__FUNCTION__ */

#define OTHER_ERR(err) other_err_helper(__FUNCTION__, (err))

#define SET_TASK_STATE(tp, newstate) \
	if(ntask_bps) set_task_state((tp), (newstate)); \
	else (tp)->state = (newstate);

#define bus_delay	(mp->size/bp->trate)
#define	link_delay	(mp->size/lp->trate)

#if !_WIN32 && !_WIN64
#if !defined(HAVE__SETJMP) 
#define	_setjmp		setjmp
#endif
#if !defined(HAVE__LONGJMP) 
#define	_longjmp	longjmp
#endif
#endif

#if	!HAVE_SIGALTSTACK && !_WIN32 && !_WIN64
#define mctx_save(mctx) _setjmp((mctx)->jb) 		/* save machine context */ 
#define mctx_restore(mctx) _longjmp((mctx)->jb, 1)	/* restore machine context */
#define ctxsw(old,new)	if(!_setjmp((old)->jb)) _longjmp((new)->jb, 1)
#else
#define mctx_save(mctx) setjmp((mctx)->jb) 		/* save machine context */ 
#define mctx_restore(mctx) longjmp((mctx)->jb, 1)	/* restore machine context */
#define ctxsw(old,new)	if(!setjmp((old)->jb)) longjmp((new)->jb, 1)
#endif

/*	CPU states and flags						*/

#define	CPU_IDLE	0		
#define	CPU_BUSY	1
#define	CPU_DOWN	2

/*	BUS states and flags						*/

#define	BUS_IDLE	0
#define	BUS_BUSY	1
#define	BUS_DOWN	2

/*	LINK states and flags						*/

#define	LINK_IDLE	0
#define	LINK_BUSY	1
#define	LINK_DOWN	2

/*	Event type codes						*/

#define	END_SYNC	0			/* sync timeout		*/
#define	END_COMPUTE	1			/* end of cpu service	*/
#define	END_QUANTUM	2			/* end of quantum	*/
#define	END_TRANS	3			/* end of transmission	*/
#define	END_SLEEP	4			/* end of sleep		*/
#define	END_RECEIVE	5			/* receive timeout	*/
#define	END_BLOCK	6			/* end task blockage	*/
#define	LINK_FAILURE	7			/* link failure		*/
#define	LINK_REPAIR	8			/* end of link failure	*/
#define	BUS_FAILURE	9			/* bus failure		*/
#define	BUS_REPAIR	10			/* end of bus failure	*/
#define	NODE_FAILURE	11			/* node failure		*/
#define	NODE_REPAIR	12			/* end of node failure	*/
#define	USER_EVENT	13			/* user defined event(s)*/
#define	CALENDAR	1000			/* calendar head & tail	*/

/*	Lock states and flags						*/

#define	UNLOCKED	0
#define	LOCKED		1

/* 	Port states and flags						*/

#define	PORT_FREE	0	
#define	PORT_USED	1
#define	PORT_SET	2
#define	PORT_SHARED	3

/*	Message type codes						*/

#define	PRIMARY		0
#define	REPLY		1
#define	ACK_TIMEOUT	0x80000000
#define	SP_REQUEST	0x80000001
#define SP_CANCEL	0x80000002

/*	Communication media codes					*/

#define	MAGIC	0
#define	BUS	1
#define	LINK	2

/* red black tree		*/

#define RB_RED          0
#define RB_BLACK        1

#define NIL &leaf           /* all leafs are sentinels */


/*	Definitions 							*/

#define	SYSCALL		int
#define	LOCAL		static

#define	MAX_DOUBLE	(HUGE_VAL)
#define MAX_STRING_LEN		50

#define	DEFAULT_MAX_NODES	10
#define	DEFAULT_MAX_GROUPS	10
#define	DEFAULT_MAX_BUSES	10
#define	DEFAULT_MAX_LINKS	10
#define	DEFAULT_MAX_STATS	20		
#define	DEFAULT_MAX_TASKS	10
#define	DEFAULT_MAX_LOCKS	10
#define	DEFAULT_MAX_SEMAPHORES	20
#define	DEFAULT_MAX_PORTS	20
#define	DEFAULT_MAX_POOLS	5
#define DEFAULT_MAX_VARS	5
#define DEFAULT_MAX_ENVS	5
#define DEFAULT_MAX_DYES	2

#define	DRIVER		(-1)
#define	NULL_EVENT	(-1)
#define	NULL_NODE	(-1)
#define	NULL_HOST	(-1)
#define	NULL_BUS	(-1)
#define	NULL_LINK	(-1)
#define	NULL_STAT	(-1)			
#define	NULL_TASK	(-1)
#define	NULL_LOCK	(-1)
#define	NULL_PORT	(-1)
#define	NULL_MESS	(-1)

#define	DRIVER_PTR	((ps_task_t *) 4)
#define	NULL_EVENT_PTR	((ps_event_t *) 0)
#define	NULL_NODE_PTR	((ps_node_t *)0)
#define	NULL_HOST_PTR	((ps_cpu_t *) 0)
#define	NULL_COMM_PTR	((ps_comm_t *) 0)
#define	NULL_BUS_PTR	((ps_bus_t *) 0)
#define	NULL_LINK_PTR	((ps_link_t *) 0)
#define	NULL_TASK_PTR	((ps_task_t *) 0)
#define	NULL_LOCK_PTR	((ps_lock_t *) 0)
#define	NULL_PORT_PTR	((ps_port_t *) 0)
#define	NULL_PAIR_PTR	((ps_tp_pair_t *) 0)
#define	NULL_MESS_PTR	((ps_mess_t *) 0)
#define NULL_BUF_PTR	((ps_buf_t *) 0)
#define NULL_CFSRQ_PTR	((ps_cfs_rq_t *) 0)
#define NULL_SCHED_PTR	((sched_info *) 0)

#define	TABLE		29776723		/* table signature	*/
#define	SUICIDE		(-321341452)		/* suicide signature	*/
#define BUFFER		90829098		/* buffer signature	*/

#define TASK_DELTA	50			
#define SUBSCRIBER_DELTA 20

/************************************************************************/
/*	P A R A S O L   T A B L E S   &   V A R I A B L E S		*/
/************************************************************************/

LOCAL	ps_table_t	ps_node_tab;		/* node table		*/
LOCAL	ps_table_t	ps_group_tab;		/* group table		*/
LOCAL	ps_table_t	ps_bus_tab;		/* bus table		*/
LOCAL	ps_table_t	ps_link_tab;		/* link table		*/
	ps_table_t	ps_stat_tab;		/* statistics table	*/
LOCAL	ps_table_t	ps_para_stat_tab;	/* para stats table	*/
LOCAL	ps_table_t	ps_lock_tab;		/* lock table		*/
LOCAL	ps_table_t	ps_sema_tab;		/* semaphore table	*/
LOCAL	ps_table_t	ps_port_tab;		/* port table		*/
LOCAL	ps_table_t 	ps_pool_tab;		/* buffer pool table	*/
LOCAL	ps_table_t	ps_env_tab;		/* environment table	*/
LOCAL	ps_table_t	ps_sched_info_tab;	/* sched_info table	*/

LOCAL	double	t_tab[2][34] = 
		       {12.706,4.303,3.182,2.776,2.571,2.447,2.365,2.306,
			2.262,2.228,2.201,2.179,2.160,2.145,2.131,2.120,
			2.110,2.101,2.093,2.086,2.080,2.074,2.069,2.064,
			2.060,2.056,2.052,2.048,2.045,2.042,2.021,2.000,
			1.980,1.960,63.657,9.925,5.841,4.604,4.032,3.707,
			3.499,3.355,3.250,3.169,3.106,3.055,3.012,2.977,
			2.947,2.921,2.898,2.878,2.861,2.845,2.831,2.819,
			2.807,2.797,2.787,2.779,2.771,2.763,2.756,2.750, 
			2.704,2.660,2.617,2.576};

LOCAL const char *task_state_names[] = {
	"FREE",
	"SUSPENDED",
	"READY",
	"HOT",
	"RECEIVING",
	"SLEEPING",
	"SYNC",
	"SYNC_SUSPEND",
	"SYNC_FREE",
	"SPINNING",
	"COMPUTING",
	"BLOCKED",
	NULL };

LOCAL rb_node leaf = { RB_BLACK, NIL, NIL, NIL, 0.0,NULL_SCHED_PTR};	

/* WCS - 12 June 1999 - Moved.  See get_dss and ts_report. 		*/
extern long ps_trsct;				/* trace_rep stack ctest*/

LOCAL	ps_event_t	calendar[2];		/* future event list	*/
LOCAL	ps_event_t	*event_fl;		/* event free list	*/
LOCAL	mctx_t	d_context;			/* driver context	*/
LOCAL	long	step_flag;			/* single step flag	*/
LOCAL	long	break_flag;			/* break polong flag	*/
LOCAL	double	break_time;			/* break polong time	*/
LOCAL	long	reaper_port;			/* grim reaper port	*/
LOCAL	ps_mess_t *next_mess = NULL_MESS_PTR;	/* free mess pointer	*/
LOCAL 	ps_tp_pair_t *tpflist = NULL_PAIR_PTR;	/* tp pair free list	*/
LOCAL 	long	sp_dir;				/* stack direction flag	*/
#if !HAVE_SIGALTSTACK || _WIN32 || _WIN64
LOCAL	long	sp_ind;				/* stack jmp_buf index	*/
LOCAL	jmp_buf	sp_jb[3];			/* sp test jmp buf array*/
LOCAL	long	sp_yadd[3];			/* sp test address array*/
#endif
LOCAL	long	sp_delta;			/* sp growth delta	*/
LOCAL	double	bs_time;			/* block statistic time	*/
LOCAL	long	qxflag = 0;			/* quantum expired flag	*/
LOCAL	long	next_mid;			/* Message serial number*/
#ifdef STACK_TESTING
LOCAL	long	max_stack;			/* Maximum stack used	*/
#endif /* STACK_TESTING */
LOCAL	ps_table_t	ps_dye_tab;		/* dye table		*/
LOCAL	long	async_count;			/* Async comm. count	*/
LOCAL	long	task_count;			/* Task creation count	*/
LOCAL	long	angio_flag;			/* angio trace flag	*/
LOCAL	FILE	*angio_file;			/* trace output file	*/
LOCAL	long	angio_output;			/* angio output flag	*/
LOCAL	long	sp_dss;				/* default stack size	*/
LOCAL	long	w_flag;				/* warning flag		*/
LOCAL	long	ntask_bps;			/* # of task breakpoints*/



/************************************************************************/

#endif /* _PARA_PRIVATES */

