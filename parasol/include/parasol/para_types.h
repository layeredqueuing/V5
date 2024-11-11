/* $Id: para_types.h 17453 2024-11-10 12:08:26Z greg $ */
/************************************************************************/
/*	para_types.h - PARASOL library typedef file			*/
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
/*	Revised: 21/12/93 (JEN)	Added code field to ps_task_t for run 	*/
/*				off fix.				*/
/*		 27/04/94 (JEN) Added typdefs for buffers and buffer	*/
/*				pools.					*/
/*		 26/07/94 (JEN) Removed psize from ps_bus_t and		*/ 
/*				ps_link_t.				*/
/*		 06/10/94 (JEN) Added stack_limit to ps_task_t.		*/
/*		 09/12/94 (JEN) Changed all typedef names to minimize 	*/
/*				compatibility problems.			*/
/*		 11/05/95 (JEN) Added org_port field to ps_mess_t to	*/
/*				fix a bug in ps_leave_port_set.		*/
/*		 18/05/95 (PRM)	Added mid field to ps_mess_t to support	*/
/*				unique message ids			*/
/*		 25/05/95 (PRM)	Added did field to ps_mess_t and 	*/
/*				ps_task_t to support angio tracing.	*/
/*		 08/06/95 (PRM)	Added rat struct to ps_stat_t union to	*/
/*				support rate statistics.		*/
/*		 07/07/95 (PRM)	Added uhost and upriority field to	*/
/*				ps_task_t for user defined scheduling	*/
/*									*/
/************************************************************************/

#ifndef	_PARA_TYPES
#define	_PARA_TYPES

/* 	Task states and flags						*/

typedef enum ps_task_state_t {
    TASK_FREE,
    TASK_SUSPENDED,
    TASK_READY,
    TASK_HOT,
    TASK_RECEIVING,
    TASK_SLEEPING,
    TASK_SYNC,
    TASK_SYNC_SUSPEND,
    TASK_SYNC_FREE,
    TASK_SPINNING,
    TASK_COMPUTING,
    TASK_BLOCKED
} ps_task_state_t;

/************************************************************************/

typedef struct mctx_st {
	jmp_buf jb;
} mctx_t;

/************************************************************************/

typedef	struct	ps_buf_t {
	long	signature;			/* buffer signature	*/
	long	pool;				/* buffer pool id	*/
	struct 	ps_buf_t	*next;		/* next buffer pointer	*/
} ps_buf_t;

/************************************************************************/

typedef	struct	ps_bus_t {			/* bus struct		*/
	char	*name;				/* bus name		*/
	long	state;				/* bus state		*/
	long	nnodes;				/* # bus nodes		*/
	long	*bnode;				/* bus node array	*/
	double	trate;				/* transmission rate	*/
	long	discipline;			/* queueing discipline	*/
	long	nqueued;			/* # messages queued	*/
	struct 	ps_mess_t	*head;		/* message queue head 	*/
	struct	ps_mess_t	*tail;		/* message queue tail 	*/
	struct	ps_event_t	*ep;		/* bus event pointer	*/
	long	stat;				/* statistics index	*/
} ps_bus_t;

/************************************************************************/

typedef	struct	ps_comm_t {			/* communication struct	*/
	long	blid;				/* link/bus index 	*/
	struct 	ps_comm_t	*next;		/* next pointer		*/
} ps_comm_t;

/************************************************************************/

typedef	struct  ps_cpu_t {			/* cpu processor struct	*/
	long	state;				/* cpu state		*/
	size_t	run_task;			/* running task	index	*/
	long	stat;				/* statistics index	*/
	struct	ps_table_t	*ts_tab;	/* task stats table	*/
	long	scheduler;			/* scheduler task	*/
	long	catcher;			/* catcher task(if any)	*/
	size_t	last_task;			/* last task index	*/
	long 	port_n;				/* # of port		*/
	struct ps_cfs_rq_t *group_rq;		/* pointer of cfs_rq for each group */ 
} ps_cpu_t;
 
/************************************************************************/

typedef struct ps_dye_t {			/* Angio dye struct	*/
	char 	*base_name;			/* base name		*/
	long 	occurence;			/* occurence number	*/
	long	serialno;			/* serial number	*/
} ps_dye_t;

/************************************************************************/

typedef	struct	ps_event_t {			/* PARASOL event struct	*/
	double	time;				/* event time		*/
	long	type;				/* event type code	*/
	long	*gp;				/* generic pointer	*/
	struct	ps_event_t	*next;		/* next event pointer	*/
	struct	ps_event_t	*prior;		/* prior event pointer	*/
} ps_event_t;

/************************************************************************/

typedef	struct ps_link_t {			/* one-way link struct	*/
	char	*name;				/* link name		*/
	long	state;				/* link state		*/
	long	snode;				/* source node index	*/
	long	dnode;				/* destinat'n node index*/
	double	trate;				/* net transmission rate*/
	struct	ps_mess_t	*head;		/* message queue head 	*/
	struct	ps_mess_t	*tail;		/* message queue tail	*/
	long	stat;				/* statistics index	*/
	struct	ps_event_t	*ep;		/* link event pointer	*/
} ps_link_t;

/************************************************************************/

typedef	struct	ps_lock_t {			/* lock struct		*/
	long	state;				/* lock state		*/
	size_t	owner;				/* lock owner index	*/
	long	queue;				/* lock queue           */
	long	count;				/* lock queue count	*/
	long	next;				/* next lock index	*/
} ps_lock_t;

/************************************************************************/

typedef	struct	ps_mess_t {			/* message struct	*/
	long	state;				/* message state	*/
	long	sender;				/* sender index		*/
	long	port;				/* port index		*/
	long	ack_port;			/* ack port index	*/
	long	org_port;			/* original port index	*/
	long	c_code;				/* comm media code	*/
	long	blid;				/* bus/link index	*/
	long 	type;				/* message type code	*/
	long	size;				/* message size (bytes)	*/
	double	ts;				/* message time-stamp	*/
	char	*text;				/* message text pointer	*/
	long	mid;				/* Unique message id    */
	long	did;				/* dye id		*/
	long	pri;				/* Message priority	*/
	struct	ps_mess_t	*next;		/* next message pointer	*/
} ps_mess_t;

/************************************************************************/

typedef	struct 	ps_node_t {			/* processor node struct*/
	char	*name;				/* node name		*/
	long	ncpu;				/* # of cpu's		*/
	long	nfree;				/* # of free cpu's	*/
	ps_cpu_t	*cpu;			/* array of cpu's	*/
	long	rtrq;				/* ready to run queue	*/
	double	speed;				/* cpu speed factor	*/
	double	quantum;			/* time slice quantum	*/
	long	discipline;			/* queueing discipline	*/
 	ps_comm_t	*sl_list;		/* send link list	*/
	ps_comm_t	*rl_list;		/* receive link list	*/
	ps_comm_t	*bus_list;		/* bus list		*/
	long		stat;			/* usage stat		*/
	struct ps_table_t	*ts_tab;	/* per task usage stats	*/
	long		sf;			/* stats flags		*/
	double	build_time;			/* construction time	*/

	/* the following for cfs scheduler*/
	long  ngroup;				/* # of groups	*/
	struct ps_cfs_rq_t *host_rq;		/* the pointer of cfs_rq */
	struct ps_cfs_rq_t *group_rq;		/* pointer of cfs_rq for each group */ 
} ps_node_t;

/************************************************************************/

typedef	struct	ps_pool_t {
	long	size;				/* size of user buffers	*/
	struct	ps_buf_t	*flist;		/* free list of buffers	*/
} ps_pool_t;

/************************************************************************/

typedef struct  ps_port_t {			/* port struct		*/
	char	*name;				/* port name		*/
	long	state;				/* port state		*/
	long	nmess;				/* # of queued messages	*/
	struct	ps_mess_t	*first;		/* first message pointer*/
	struct	ps_mess_t	*last;		/* last message pointer	*/
	long	owner;				/* port owner index	*/
	struct 	ps_tp_pair_t *tplist;		/* list of tp pairs	*/
	long	next;				/* next port index	*/
} ps_port_t;

/************************************************************************/

typedef	struct	ps_tp_pair_t {			/* task port pair struct*/
	long	task;				/* task index		*/
	long	port;				/* port	index		*/
	struct 	ps_tp_pair_t *next;		/* next pointer		*/
} ps_tp_pair_t;

/************************************************************************/

typedef	struct	ps_sema_t {			/* semaphore struct	*/
	long	count;				/* semaphore counter	*/
	ps_tp_pair_t 	*queue;			/* semaphore queue	*/
} ps_sema_t;

/************************************************************************/

typedef	struct	ps_stat_t {			/* statistics struct	*/
	char	*name;				/* statistic name	*/
	double	resid;				/* non-rounding resid	*/
	union {
	    struct {
		long	count;			/* sample count		*/
		double	sum;			/* sample sum		*/
	    } sam;
	    struct {
		double	start;			/* start time		*/
		double	old_value;		/* previous value	*/
		double	old_time;		/* previous time	*/
		double	integral;		/* variable integral	*/
	    } var;
	    struct {
		double	start;			/* start time		*/
		long	count;			/* # observations	*/
	    } rat;
	} values;
	long	type;				/* statistic type	*/
} ps_stat_t;

/************************************************************************/

typedef struct 	ps_table_t {			/* dynamic table struct	*/
	long	signature;			/* table signature	*/
	long	tab_size;			/* # of table entries	*/
	long	entry_size;			/* table entry size	*/
	long	used;				/* # of entries used	*/
	long	rover;				/* rover index		*/
	char	*tab;				/* table pointer	*/
	char	*base;				/* table base		*/
} ps_table_t;

/************************************************************************/

typedef	struct  ps_task_t {			/* task struct		*/
	char	*name;				/* task name		*/
	ps_task_state_t state;			/* task state		*/
	long	node;				/* node location	*/
	long	uhost;				/* user cpu index	*/
	long	host;				/* cpu index		*/
	struct	ps_cpu_t	*hp;		/* host pointer		*/
	double	*stack_base;			/* stack base		*/
	double	*stack_limit;			/* stack limit		*/
	mctx_t	context;			/* task context		*/
	void	(* code)(void *);		/* task code pointer	*/
	long	upriority;			/* user task priority	*/
	long	priority;			/* task priority	*/
	long	next;				/* next task index	*/
	long	parent;				/* task parent index	*/
	long	son;				/* task son index	*/
	long	sibling;			/* task sibling index	*/
	long	port_list;			/* port list 		*/
	long	bport;				/* broadcast port index	*/
	long	blind_port;			/* blind port index	*/
	long	wport;				/* waiting port index	*/
	long	lock_list;			/* lock list 		*/
	long	spin_lock;			/* spin lock index	*/
	struct	ps_event_t	*tep;		/* task event pointer	*/
	double	rct;				/* remaining cpu time	*/
	struct	ps_event_t	*qep;		/* quantum event pointer*/
	long	qx;				/* quantum expired flag	*/
	struct	ps_event_t	*rtoep;		/* receive t/o event ptr*/
	long	did;				/* dye id		*/
	long	tsn;				/* Task serial number	*/
	double	sched_time;			/* Task schedule time	*/
 	double	end_compute_time;		/* Task end compute time */
	long	tbp;				/* break polong info	*/
	
	/*The following fields are used to calculate the preemption time of a task. Tao*/
	long	pt_tag;				/*preemption tag. 1: preempted, 0: not preempted.*/
	double	pt_last;			/*the latest time that is preempted*/
	double	pt_sum;				/*the total preemption time*/

	/* The followings are for cfs scheduler*/
	long 	group;				/* group index	*/
	long 	group_id;			/* order of group */
	struct sched_info	*si;		/* contain the cfs schedule information of the task */
	 
} ps_task_t;

/************************************************************************/

typedef struct	ps_var_t {
	long	used;				/* is entry used?	*/
	long	width;				/* Width of each elem	*/
	void	*data;				/* Array of elements	*/
} ps_var_t;

/************************************************************************/

typedef struct	ps_env_t {
	long	used;				/* is entry used?	*/
	long	ntasks;				/* size of indices array*/
	long	nsubscribers;			/* # of subscribers	*/
	long	nallocated;			/* Allocated vars	*/	
	long	*indices;			/* indices array	*/
	struct	ps_table_t	var_tab;	/* vars table		*/
} ps_env_t;

/************************************************************************/

typedef	struct  ps_cfs_rq_t {		/* cfs run queue struct		*/
	long	node;				/* node location	*/
	long 	nready;			/* # of ready to run task */
	struct rb_node * root;
	struct rb_node * leftmost;

	/* time parameters */
	double fair;			/* fair key*/
	double exec_time;		/*the exec_time*/
	double wait_to_run;		/* the wait time */
	double sched_time;		/* Task schedule time	*/
    /* double underrun; */
    /* double overrun; */

	struct sched_info *si;		/* contain the cfs schedule information of the cfs-rq
						if the rq is owned by a group	*/
	long 	load;
	struct sched_info * curr;
} ps_cfs_rq_t;
/************************************************************************/

typedef	struct  ps_group_t {			/* group struct		*/
	char	*name;				/* group name		*/
	long	node;				/* node location	*/
	long	group_id2;		/* the order of group in the node */
	double	share;			/* share value of the group	*/
	long 	cap;			/* cap flag*/

	long	ntask;			/*  # of task in this group	*/
	long 	* task;				/* array of task	*/
	long	stat;			/* usage stat		*/
	struct ps_table_t	*ts_tab;	/* per task usage stats	*/
	long		sf;			/* stats flags		*/
	
    /* sched_info *curr; */
    /* ps_cfs_rq_t * rq; */

} ps_group_t;
/************************************************************************/

typedef	struct  sched_info {		

	long on_rq ; 	/*  1 means this si in the cfs_rq,otherwise is 0. */
	long task;
	/*struct rb_node * n; */
	struct rb_node *rbnode;		/* pointer of the rbnode  */
	struct ps_cfs_rq_t * own_rq ; 	/* the cfs_rq owned by this group ; */
	struct ps_cfs_rq_t * rq; 	/*  */
	struct sched_info * parent;	/* a pointer to the si in upper rq  */

	/* time parameters */
	double fair;
	double key;
	double q;
	double exec_time; 
	double sched_time;
	double sleep_time;
	double weight;


} sched_info;
/************************************************************************/

typedef	struct  rb_node {			/* red black tree node struct		*/

	long 	color;
    	struct rb_node *rb_left;
	struct rb_node *rb_right;
	struct rb_node *rb_parent;
	double key;
	struct sched_info	*si;

}rb_node;

/************************************************************************/


#endif	/* _PARA_TYPES */

