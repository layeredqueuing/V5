/* $Id: para_library.c 15895 2022-09-23 17:21:55Z greg $ */

/************************************************************************/
/*	para_library.c - PARASOL library source file			*/
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
/*	Created: 10/06/93 (JEN)						*/
/*	Revised: 21/12/93 (JEN)	Fixed memory leak in ps_release_port.	*/
/*				Added internal wrapper and modified	*/
/*				ps_create2 to prevent user tasks from	*/
/*				"running off the end".			*/
/*		 06/04/94 (JEN) Fixed find_priority to cause context 	*/
/*				switches to occur if previous HOT task	*/
/*				is preempted by a COMPUTING task or if	*/
/*				it is preempted by new HOT task - bugs	*/
/*				appeared with ps_adjust_priority calls.	*/
/*		 08/04/94 (JEN) Improved ps_receive and added calls	*/
/*				ps_receive_last and ps_receive_random.	*/
/*		 27/04/94 (JEN) Added ps_idle_cpu, ps_ready_queue, 	*/
/*				ps_build_buffer_pool, ps_free_buffer,	*/
/*				ps_get_buffer, and ps_buffer_size.	*/
/*		 19/07/94 (JEN) Fixed problems with ps_block_stat.	*/
/*		 26/07/94 (JEN) Removed packetizing from ps_build_bus	*/
/*				and ps_build_link.			*/
/*		 06/10/94 (JEN) Added sp_tester and sp_winder and 	*/
/*				modified ps_run_parasol, ps_create2, 	*/
/*				and wrapper for portability.  Also	*/
/*				modified ps_headroom for upward growing	*/
/*				stacks.					*/
/*		 07/10/94 (JEN) Modified ps_create, ps_create2, 	*/
/*				sp_tester, and ps_run_parasol for 	*/
/*				auto sized stacks.			*/
/*		 12/10/94 (JEN) Fixed guard code in ps_awaken.	Also	*/
/*				fixed ps_headroom for upward growing	*/
/*				stacks.					*/
/*		 06/12/94 (JEN) Added global declarations to avoid 	*/
/*				multiple definition warnings.		*/
/*		 09/12/94 (JEN) Changed names with ps prefix to minimize*/
/*				name clashes.				*/
/*		 08/02/95 (JEN) Modified buffer pool code to prevent	*/
/*				misalignment problems with structured	*/
/*				buffers.				*/
/*		 14/02/95 (JEN) Modified ready, find_host, find_priority*/
/*				end_quantum_handler, end_sync_handler 	*/
/*				for PR queueing.			*/
/*		 16/02/95 (JEN) Fixed ps_migrate for null move. Changed	*/
/*				find_priority and find_ready for mods to*/
/*				hid macro.				*/
/*		 11/05/95 (JEN) Fixed port_set_surrogate to detect and	*/
/*				commit suicide if port set port is gone */
/*				and use port_send to relay message.	*/
/*		 		Modified port_receive to return original*/
/*				port and ps_receive, ps_receive_last, 	*/
/*				and ps_receive_random to accomodate new	*/
/*				port_receive.  Mods to ps_leave_port_set*/
/*				to shift messages back to original port	*/
/*				and drop ps_kill of dead surrogate.	*/
/*				Added port_send.			*/
/*		 18/05/95 (PRM) added file level variable next_mid to	*/
/*				implement message serial numbers. 	*/
/*				Changed ps_send and ps_receive family	*/
/*				to prlong serial numbers when tracing.	*/
/*				Made PARASOL task classes invisible to	*/
/*				users by making changes to ts_report.	*/
/*				Made messaging via port sets and shared	*/
/*				ports transparent by maintaining the	*/
/*				message id. Fixed a bug with handling	*/
/*				timeouts in shared_port_dispatcher.	*/
/*				timeouts. Added mid parameter to 	*/
/*				port_send/receive to implement transpar-*/
/*				ent messaging. Added lookup_port_set	*/
/*				to help with tracing			*/
/*		 24/05/95 (PRM)	Unrolled breadth recursion in relative()*/
/*				in order to minimize chance of stack	*/
/*				overflow with fat task creation trees.	*/
/*				Modified ancestor() so that in most	*/
/*				cases it will run faster.		*/
/*		 25/05/95 (PRM)	Added support for angio tracing includ-	*/
/*				ing support functions and SYSCALLs.	*/
/*				Modified most other SYSCALLs to log	*/
/*				appropriate angio events. Added	5 new	*/
/*				globals.				*/
/*		 30/05/95 (PRM)	Modified ps_create2 to cook task names	*/
/*				if angio tracing is enabled. Put guard	*/
/*				code in ps_inject_trace_name to check 	*/
/*				for invalid trace names. Added did param*/
/*				to port_send/receive, stopped angio	*/
/*				trace output for system class tasks.	*/
/*		 08/06/95 (PRM)	Added guard code to ps_reset_semaphore	*/
/*				to disallow negative values.		*/
/*		 08/06/95 (PRM)	Implemented RATE statistics including	*/
/*				the addition of ps_record_rate_stat.	*/
/*				Fixed ps_send so that messages sent from*/
/*				A port set owner to a surrogate's std	*/
/*				port are not seen in the trace.		*/
/*		 16/06/95 (PRM)	Added code for testing all stacks. This */
/*				includes all code included in #ifdef	*/
/*				STACK_TESTING/#endif pairs.		*/
/*		 21/06/95 (PRM)	Fixed problem with ps_migrate when	*/
/*				a computing task migrates between nodes	*/
/*				with different speeds. Fixed problem 	*/
/*				with ps_suspend and tasks in the 	*/
/*				TASK_BLOCKED state.			*/
/*		 22/06/95 (PRM) Fixed bug in find_priority. Fixed bug	*/
/*				in the port_send reporting factility	*/
/*		 27/06/95 (PRM)	Changed the `class' parameters in 	*/
/*				ps_create and ps_create2 to `code' for	*/
/*				C++ compatability.			*/
/*		 07/07/95 (PRM)	Implemented user defined scheduling, 	*/
/*				changes include new SYSCALLs ps_schedule*/
/*				ps_ready_queue2 and ps_build_node2, 	*/
/*				changes to find_host, find_priority	*/
/*				find_ready and ready, as well as the 	*/
/*				system task class catcher.		*/
/*		 09/07/95 (PRM)	Added SYSCALLs and support functions for*/
/*				for implementing glocal variables.	*/
/*		 26/07/95 (PRM) Modified ps_create2 to take a stack 	*/
/*				scaling factor rather than a stack size.*/
/*		 15/06/99 (WCS) Please see bug fixes scattered through  */
/*				the code and marked with WCS comments.  */
/*		 		(ps_)run_time, ts_flag and ts_report    */
/*				now externally available.		*/
/*				Also merged in Greg Franks messages	*/
/*				with priorities and sched_time stuff	*/
/*									*/
/************************************************************************/

#include <parasol/para_internals.h>
#include <parasol/para_privates.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include <setjmp.h>

#define TEMP_STR_SIZE	128

#if defined(DEBUG)
static void 	print_event( const char *, ps_event_t *ep );			/* event pointer	*/
#endif
    

/************************************************************************/
/*                 P A R A S O L   G L O B A L S			*/
/************************************************************************/

ps_task_t	*ps_htp;			/* HOT task pointer	*/
double		ps_now;				/* current time		*/
ps_table_t	ps_task_tab;			/* task table		*/
long		ts_flag;			/* trace state flag	*/
double		ps_run_time;			/* stop time		*/

/* WCS - 30 Aug 1997 - Added.  See get_dss and ts_report. */
long		ps_trsct;			/* trace_rep stack ctest*/
unsigned        first_run = TRUE;

/************************************************************************/
/*	P A R A S O L   L I B R A R Y   D I R E C T O R Y		*/
/*									*/
/*	Task Management Related SYSCALLs:				*/
/*		ps_adjust_priority, ps_awaken, ps_buses, ps_children,	*/
/*		ps_compute, ps_create, ps_create2, ps_hold, ps_idle_cpu,*/
/*		ps_kill, ps_migrate, ps_ready_queue, ps_receive_links, 	*/
/*		ps_resume, ps_send_links, ps_siblings, ps_sleep, 	*/
/*		ps_suspend, ps_sync					*/
/*									*/
/*	Message Passing Related SYSCALLs:				*/
/*		ps_allocate_port, ps_allocate_port_set, 		*/
/*		ps_allocate_shared_port, ps_broadcast, ps_buffer_size,	*/
/*		ps_build_buffer_pool, ps_bus_send, ps_free_buffer,	*/
/*		ps_get_buffer, ps_join_port_set, ps_leave_port_set,	*/
/*		ps_link_send, ps_localcast, ps_multicast, ps_my_ports,	*/
/*		ps_owner, ps_pass_port, ps_receive, ps_receive_last,	*/
/*		ps_receive_random, ps_receive_shared, ps_release_port,	*/
/*		ps_release_shared_port, ps_resend, ps_send		*/
/*									*/
/*	Sychronization Related SYSCALLs:				*/
/*		ps_lock, ps_reset_semaphore, ps_signal_semaphore,	*/
/*		ps_unlock, ps_wait_semaphore				*/
/*									*/
/*	Statistics Related SYSCALLs:					*/
/*		ps_block_stats, ps_get_stat, ps_open_stat, 		*/
/*		ps_record_stat, ps_reset_stat, ps_reset_all_stats,	*/
/*		ps_stats						*/
/*									*/
/*	Miscellaneous SYSCALL						*/
/*		ps_build_bus, ps_build_link, ps_build_node, ps_erlang,	*/
/*		ps_headroom, ps_run_parasol, ps_abort, ps_curr_priority	*/
/*									*/
/*	Angio Tracing SYSCALLS:						*/
/*		ps_inject_trace_name, ps_task_cycle, ps_user_event	*/
/*		ps_toggle_angio_output					*/
/*									*/
/*	Glocal Variable SYSCALLS:					*/
/*		ps_allocate_glocal, ps_share_glocal, ps_subscribe_glocal*/
/*		ps_glocal_value						*/
/*									*/
/*	Special PARASOL Task Classes:					*/
/*		port_set_surrogate, reaper, shared_port_dispatcher	*/
/*									*/
/*	Driver Support Functions:					*/
/*		main, bus_failure_handler, bus_repair_handler, driver,	*/
/*		end_block_handler, end_compute_handler, 		*/
/*		end_quantum_handler, end_receive_handler,		*/
/*		end_sleep_handler, end_sync_handler, end_trans_handler,	*/
/*		link_failure_handler, link_repair_handler, 		*/
/*		node_failure_handler, node_repair_handler, 		*/
/*		user_event_handler					*/
/*									*/
/*	Scheduler Support Functions:					*/
/*		add_event, ctxsw, dq_ready, find_host, find_priority,	*/
/*		find_ready, init_event, next_event, private_priority,	*/
/*		ready, remove_event, sched				*/
/*									*/
/*	Dynamic Table Support Functions:				*/
/*		free_table_entry, get_table_entry, init_table		*/
/*		 							*/
/*	Debugger Support Functions:					*/
/*		debug, event_report, hard_report, padstr, soft_report,	*/
/*		ts_report						*/
/*									*/
/*	Angio Tracing Support Functions:				*/
/*		create_dye, derive_dye, free_dye, start_trace,		*/
/*		end_trace, log_angio_event, get_occurence_number	*/
/*									*/
/*	Glocal Variable Support Functions:				*/
/*		glocal_init_stuff					*/
/*									*/
/*	Miscellaneous Support Functions:				*/
/*		ps_abort, ancestor, bad_param_helper, dq_lock,   	*/
/*		free_mess, free_pair, get_mess, get_pair, init_locks, 	*/
/*		init_semaphores, port_receive, port_send, relative, 	*/
/*		release_locks, release_ports, warning, wrapper		*/
/*									*/
/************************************************************************/

/************************************************************************/
/*	P A R A S O L   L I B R A R Y   F U N C T I O N S		*/
/************************************************************************/

/************************************************************************/
/*		Task Management Related SYSCALLS			*/
/************************************************************************/

SYSCALL	ps_adjust_priority(

/* Alters the sheduling priority of a specified "task". The target task	*/
/* must be a descendant of the caller or the caller itself.  This call	*/
/* may cause instantaneous task resheduling.				*/

	long	task,				/* task index		*/
	long	priority			/* task priority	*/
)
{
	ps_task_t	*tp;			/* task pointer		*/

	if(task >= ps_task_tab.tab_size || task < 0)
		return(BAD_PARAM("task"));
	if((tp = ps_task_ptr(task))->state == TASK_FREE)
		return(BAD_PARAM("task"));
	if(!ancestor(tp))
		return(BAD_PARAM("task"));
	if(priority < MIN_PRIORITY || priority > MAX_PRIORITY)
		return(BAD_PARAM("priority"));

	tp->upriority = priority;
	adjust_priority (task, priority);
	return OK;
}

/************************************************************************/

SYSCALL ps_awaken(

/* Awakens the specified task if sleeping; otherwise call is ignored.	*/

	long	task				/* task index		*/
) 
{
	ps_task_t	*tp;			/* task pointer		*/

	if(task >= ps_task_tab.tab_size || task < 0)
		return(BAD_PARAM("task"));
	if((tp = ps_task_ptr(task))->state == TASK_FREE)
		return(BAD_PARAM("task"));
	if(tp->state != TASK_SLEEPING)
		return(OK);
	if (angio_flag) 
		log_angio_event(tp, dye_ptr(tp->did), "wAwaken");
	remove_event(tp->tep);
	tp->tep = NULL_EVENT_PTR;

	/* if for CFS task,update sleep task 	 */
	if((node_ptr(tp->node))->discipline==PS_CFS){
		update_sleep_task(tp);
	}
	find_host(tp);
	return(OK);
}

/************************************************************************/

SYSCALL	ps_buses(

/* Returns the buses accessible by the specified "task".		*/

	long	task,				/* task id		*/
	long	*nbp,				/* # buses pointer	*/
	long	bus_array[]			/* bus array		*/
)
{
	ps_comm_t	*cp;			/* comm pointer		*/
	ps_task_t	*tp;			/* task pointer		*/

	if(task >= ps_task_tab.tab_size || task < 0)
		return(BAD_PARAM("task"));
	if((tp = ps_task_ptr(task))->state == TASK_FREE)
		return(BAD_PARAM("task"));

	*nbp = 0;
	cp = node_ptr(tp->node)->bus_list;
	while(cp != NULL_COMM_PTR) {
		bus_array[*nbp] = cp->blid;
		(*nbp)++;
		cp = cp->next;
	}
	return(OK);
}

/************************************************************************/

SYSCALL	ps_children(

/* Returns the children (i.e., direct descendants) of the caller.	*/

	long	*ncp,				/* # children pointer	*/
	long	*child_array			/* array of children	*/
)
{
	ps_task_t	*tp;			/* task pointer		*/

	*ncp = 0;
	if(ps_htp == DRIVER_PTR)
		return(OK);
	if(ps_htp->son != NULL_TASK) {
		tp = ps_task_ptr(ps_htp->son);
		(*ncp)++;
		*child_array++ = ps_htp->son;
		while(tp->sibling != NULL_TASK) {
			(*ncp)++;
			*child_array++ = tp->sibling;
			tp = ps_task_ptr(tp->sibling);
		}
	}
	return(OK);
}

/************************************************************************/

SYSCALL	ps_compute(	

/* Retains processor for "delta" (scaled) cpu time. Interruptions are 	*/
/* permitted (i.e., PR or RR scheduling).				*/

	double	delta				/* cpu time delta	*/
)
{
	char	string[40];
	ps_node_t *np;

	if(delta < 0.0)
		return(BAD_PARAM("delta"));
	np=node_ptr(ps_htp->node);
	if(np->discipline==PS_CFS)
		ps_htp->si->q=get_quantum(np,hid(np,ps_htp->hp),delta);
	ps_htp->tep = add_event(ps_now + delta / np->speed, 
		END_COMPUTE, (long *) ps_myself);
	SET_TASK_STATE(ps_htp, TASK_COMPUTING);
	if (angio_flag) {
		sprintf (string, "wUse %g", delta);
		ps_log_user_event (string);
	}
	sched();

	return(OK);
}

/************************************************************************/

SYSCALL	ps_create(

/* Creates a named task in the TASK_SUSPENDED state on a specified 	*/
/* "node" and "host" cpu processor. The host may be ANY_HOST.  "Class"	*/
/* refers to the C function serving as the main function of the task.	*/
/* No arguments are passed.  The sheduling priority of the task is given*/
/* by "priority". 							*/

	const 	char	*name,			/* task name		*/
	long	node,				/* node location index	*/
	long 	host,				/* host cpu id index	*/
	void	(*code)(void *),		/* code pointer		*/
	long	priority			/* task priority	*/
)
{	
	return(ps_create2(name, node, host, code, priority, -1, 1.0));
}

/************************************************************************/

SYSCALL	ps_create_group(

/* Creates a named task in the TASK_SUSPENDED state on a specified 	*/
/* "node" and "host" cpu processor. The host may be ANY_HOST.  "Class"	*/
/* refers to the C function serving as the main function of the task.	*/
/* No arguments are passed.  The sheduling priority of the task is given*/
/* by "priority". 							*/

	const 	char	*name,			/* task name		*/
	long	node,				/* node location index	*/
	long 	host,				/* host cpu id index	*/
	void	(*code)(void *),		/* code pointer		*/
	long	priority,			/* task priority	*/
	long	group				/* group index		*/
)
{	
	return(ps_create2(name, node, host, code, priority, group, 1.0));
}

/************************************************************************/
SYSCALL	ps_create2(

/* Creates a named task in the TASK_SUSPENDED state on a specified 	*/
/* "node" and "host" cpu processor. The host may be ANY_HOST.  "Class"	*/
/* refers to the C function serving as the main function of the task.	*/
/* No arguments are passed.  The sheduling priority of the task is given*/
/* by "priority". The execution stack is of size "stacksize" bytes.	*/
/* "group" specifies which group the task belongs to(if applicable).	*/
	const 	char	*name,			/* task name		*/
	long	node,				/* node location index	*/
	long 	host,				/* host cpu id index	*/
	void	(*code)(void *),		/* code pointer		*/
	long	priority,			/* task priority	*/
	long	group,				/* group index		*/
	double	stackscale			/* task size in bytes	*/
)
{
	long	stacksize;			/* stack size		*/
	long	myid=0;				/* calling task id	*/
	long	i;				/* loop index		*/
	long	port;				/* port id index	*/
	long	task;				/* task id index	*/
	ps_node_t 	*np;			/* node pointer		*/
	ps_task_t	*tp;			/* task pointer		*/
	long	adjustment;			/* stack base adjuster	*/
	sched_info 	*si;			/* sched_info pointer	*/
#if	!HAVE_SIGALTSTACK || _WIN32 || _WIN64
	long	*ctxt;				/* context pointer	*/
	long	dss;				/* default stack size	*/
#endif

	if( node >= ps_node_tab.used || node < 0 )
		return(BAD_PARAM("node"));
	if(host >= node_ptr(node)->ncpu || host < ANY_HOST)
		return(BAD_PARAM("host"));
	if(priority > MAX_PRIORITY || priority < MIN_PRIORITY)
		return(BAD_PARAM("priority"));
	if((group!=-1 && group < 0) || group >node_ptr(node)->ngroup)
		return(BAD_PARAM("group"));
	if(stackscale <= 0.)
	    	return(BAD_PARAM("stackscale"));
	if(ps_htp != DRIVER_PTR)
		myid = ps_myself;
	if((task = get_table_entry(&ps_task_tab)) == SYSERR)
		return(OTHER_ERR("growing task table"));

#if	HAVE_SIGALTSTACK && !_WIN32 && !_WIN64
	stacksize = (long)(stackscale * (double)0x10000) + MINSIGSTKSZ;	/* punt... */
#else
	stacksize = (long)(stackscale * (double)sp_dss) + 0x1000;
	stacksize -= stacksize % sizeof(double);

/* 	Two step test necessary because of gcc bug!			*/

	dss = 4*sp_delta*sizeof(double);
	if(stacksize < dss)
		return(BAD_PARAM("stackscale"));

#ifdef STACK_TESTING
	stacksize *= 5;
#endif /*STACK_TESTING*/
#endif

/*	Must fix ps_htp if table grew and not driver.			*/

	if(ps_htp != DRIVER_PTR)
		ps_htp = ps_task_ptr(myid);

	tp = ps_task_ptr(task);
	if(!(tp->name = (char *) malloc((unsigned)(strlen(name) + 1))))
		ps_abort("Insufficient memory");
	sprintf(tp->name, "%s", name);

/*	Have to cook task names if angio tracing is on.			*/

	if (angio_flag)
		for (i = 0; tp->name[i]; i++)
			if (isspace(tp->name[i]))
				tp->name[i] = '_';

	tp->state = TASK_SUSPENDED;
	tp->node = node;
	tp->port_list = NULL_PORT;
	tp->code = code;
	if(ts_flag)
		ts_report(tp, "created (suspended)");
	if((port = ps_allocate_port("Broadcast", task)) < 0) {
		ps_kill(task);
		return(OTHER_ERR("allocating standard port"));
	}

	tp->host = tp->uhost = host;
	tp->hp = NULL_HOST_PTR;
	if(!(tp->stack_base = (double *) malloc((unsigned)stacksize)))
		ps_abort("Insufficient memory");
	if((adjustment = ((size_t)tp->stack_base % sizeof(double)))) {
		adjustment = sizeof(double) - adjustment;
		stacksize -= sizeof(double);
	}
	tp->stack_limit = tp->stack_base + stacksize/sizeof(double);
#ifdef STACK_TESTING
	memset (tp->stack_base, 0x55, stacksize);
#endif /*STACK_TESTING*/

#if	HAVE_SIGALTSTACK && !_WIN32 && !_WIN64
	mctx_create( &tp->context, tp->code, 0, tp->stack_base, stacksize );
#else
	if(ps_htp == DRIVER_PTR) {
		if(!mctx_save(&d_context))
			wrapper(&tp->context, &d_context);
	}
	else {
	    if(!mctx_save(&ps_htp->context)) 
			wrapper(&tp->context, &ps_htp->context);
	}
	ctxt = (long *) &tp->context;
	if(sp_dir > 0)
		ctxt[sp_ind] = (size_t)tp->stack_base + adjustment + 0x100;
	else
		/* WCS - 20 Aug 1997 - Added: << - 0x100 >> in consideration of assumed   */
		/* setjmp nonzero return value, actually provided by some longjmp call.   */
		/* Will this be adequate for all architectures?  << - sizeof(long) >>      */
		/* seems to work for NeXTSTEP on Intel.  Where does the return value fall */
		/* with respect to the env argument?  Why is 0x100 used when sp_dir > 0?  */
		ctxt[sp_ind] = (size_t)tp->stack_base + adjustment + stacksize - 0x100;
#endif

	tp->code = code;
	tp->priority = tp->upriority = priority;
	tp->next = NULL_TASK;
	if(ps_htp == DRIVER_PTR)
		tp->parent = DRIVER;
	else
		tp->parent = ps_myself;
	tp->son = NULL_TASK;
	if(ps_htp != DRIVER_PTR) {
		tp->sibling = ps_htp->son;
		ps_htp->son = task;
	}
	else
		tp->sibling = NULL_TASK;
	tp->bport = port;
	tp->blind_port = NULL_PORT;
	tp->wport = NULL_PORT;
	tp->lock_list = NULL_LOCK;
	tp->spin_lock = NULL_LOCK;
	tp->tep = tp->qep = tp->rtoep = NULL_EVENT_PTR;
	tp->rct = 0.0;
	tp->qx = FALSE;
	tp->tsn = task_count++;

	if (group>=0)
		tp->group=group_ptr(group)->group_id2;
	else
		tp->group=-1;
	tp->group_id=group;
	tp->si=NULL_SCHED_PTR;

	/*for cfs scheduler */
	np = node_ptr(tp->node);
	if (np->discipline==PS_CFS) {
		if( group < 0) {
			ps_kill(task);
			return(OTHER_ERR("creating task with no group"));	/* Can't run non group task on cfs node. */
		}

	    	/*tp->group=group; */

		if(!(si = (sched_info *) malloc(sizeof(sched_info))))
			ps_abort("Insufficient memory");

		init_sched_info(si, task,NULL_CFSRQ_PTR,NULL_CFSRQ_PTR, 1); /*weight=1 */
		tp->si=si;

	} 	/* end cfs	*/

	tp->sched_time = ps_now;
	tp->tbp = -2;
	if (task < 2 || angio_flag) {
		if (ps_htp == DRIVER_PTR)
			tp->did = create_dye ("Initial");
		else
			tp->did = derive_dye (ps_task_ptr(tp->parent)->did);
	}
	if (angio_flag)
		log_angio_event (tp, dye_ptr(tp->did), "wBegin");

	return(task);
}
	
/************************************************************************/

long stack_corruption_tester (

/*	Returns the amount of stack space used by the function fn.	*/
/*	Warning: The globals sp_delta, sp_ind and sp_dir must be set 	*/
/*	before calling this function.					*/

	void 	(*fn)(void)			/* function to test	*/
)
{
#if	!HAVE_SIGALTSTACK || _WIN32 || _WIN64
	char 		*stack;			/* stack base		*/
	long		bytes;			/* bytes used		*/
	long		stacksize;		/* bytes allocated	*/
	long		i;			/* loop index		*/
	long		*ctxt;			/* 			*/
	jmp_buf		newc;			/* New context		*/
	static jmp_buf	oldc;			/* current context	*/
	static void	(*func)(void);		/* global storage	*/
	long		adjustment;		/* stack ptr. adjustment*/
	
	stacksize = 2048 * sp_delta * sizeof(double) / sizeof(double);	
	if(!(stack = (char *) malloc((unsigned)stacksize)))
		ps_abort("Insufficient memory");
	if((adjustment = (size_t)stack % sizeof(double)) != 0) {
		adjustment = sizeof(double) - adjustment;
		stacksize -= sizeof(double);
	}
	
	memset (stack, 0x55, stacksize);	/* initialize stack 	*/
	
#if !_WIN32 && !_WIN64
	if(!_setjmp(oldc)) {			/* save old context	*/
 		if(!_setjmp(newc)) {		/* setup new context	*/
			ctxt = (long *)newc;
			if(sp_dir > 0)
				ctxt[sp_ind] = (long)(stack + 0x100);
			else
				ctxt[sp_ind] = (long)(stack + adjustment 
				    + stacksize - 0x100);
			func = fn;
 			_longjmp (newc, 1);	/* enter new context	*/
 		}
		else {				/* we're in new context	*/
			(*func)();
			_longjmp (oldc, 1);	/* back to old context	*/
		}
	}
#else
	if(!setjmp(oldc)) {			/* save old context	*/
 		if(!setjmp(newc)) {		/* setup new context	*/
			ctxt = (long *)newc;
			if(sp_dir > 0)
				ctxt[sp_ind] = (size_t)(stack + 0x100);
			else
				ctxt[sp_ind] = (size_t)(stack + adjustment 
				    + stacksize - 0x100);
			func = fn;
 			longjmp (newc, 1);	/* enter new context	*/
 		}
		else {				/* we're in new context	*/
			(*func)();
			longjmp (oldc, 1);	/* back to old context	*/
		}
	}
#endif
	if (sp_dir > 0) {			/* start at the top	*/
		for (i = stacksize - 1; i >= 0; i--)
			if (stack[i] != 0x55) break;
		bytes = i - 0x100;
	}
	else {
		for (i = 0; i < stacksize; i++)	/* start at the bottom	*/
			if (stack[i] != 0x55) break;
		bytes = stacksize - i - 0x100;
	}
	free (stack);
	
	return bytes; 
#else
	return 0;
#endif
}

/************************************************************************/


SYSCALL	ps_hold(

/* Retains processor for "delta" (scaled) cpu time.  Intended for use in*/
/* dedicated processing where the processor is also retained afterwards.*/
/* This call is suitable and more efficient for parallel system 	*/
/* architectures with one task per processor.				*/

	double	delta				/* cpu time delta	*/
)
{
	char	string[40];

	if(delta < 0.0)
		return(BAD_PARAM("delta"));

	ps_htp->tep = add_event(ps_now + delta / node_ptr(ps_htp->node)->speed, 
		END_BLOCK, (long *)ps_myself);
	SET_TASK_STATE(ps_htp, TASK_BLOCKED);
	if (angio_flag) {
		sprintf (string, "wUse %g", delta);
		ps_log_user_event (string);
	}

	sched();

	return(OK);
}

/************************************************************************/

SYSCALL	ps_idle_cpu(

/* Returns the number of idle cpu hosts for specified node.		*/

	long	node				/* node index		*/
)
{
	ps_node_t	*np;			/* node pointer		*/

	if(node < 0 || node >= ps_node_tab.used)
		return(BAD_PARAM("node"));
	else
		return((np = node_ptr(node))->nfree);
}

/************************************************************************/

SYSCALL	ps_kill(

/* Kills a task and its descendants freeing resources as it does so.	*/

	long	task				/* task index		*/
)
{
	ps_task_t	*tp, *btp, *ctp;	/* task pointers	*/
	long	old_state;			/* old task state	*/
	ps_node_t	*np;			/* node pointer		*/
	ps_cpu_t	*hp;			/* host cpu pointer	*/
	sched_info	*si;
	long	sib;				/* sibling index	*/
	long	temp;				/* temporary		*/
	void	release_ports();		/* port killer		*/

	if(task >= ps_task_tab.tab_size || task < 2) {
		return(BAD_PARAM("task"));
	} else if((tp = ps_task_ptr(task))->state == TASK_FREE) {
		return(BAD_PARAM("task"));
	} else if(!ancestor(tp)) {
		return(BAD_PARAM("task"));
	}

/*	Use grim reaper for suicides to prevent memory leak.		*/

	if(tp == ps_htp) {
		if(port_send(reaper_port, SUICIDE, ps_now, "", task, 0, 0, 0) 
		    == SYSERR)
			ps_abort("Failed to send suicide request to reaper");
		ps_suspend(task);
	}

	old_state = tp->state;
	if(tp->tbp != -2)
		ntask_bps--;
	np = node_ptr(tp->node);
	hp = tp->hp;
	if(ts_flag)
		ts_report(tp, "dead");

/*	Release resources including: descendants, name, task events, 	*/
/*	locks & ports held.						*/

	if((tp->state != TASK_SYNC) && (tp->state != TASK_SYNC_FREE) &&
	    (tp->state != TASK_SYNC_SUSPEND)) {

/*	Kill descendants recursively					*/

		if(tp->son != NULL_TASK) {
			sib = ps_task_ptr(tp->son)->sibling;
			while(sib != NULL_TASK) {
				temp = ps_task_ptr(sib)->sibling;
				ps_kill(sib);
				sib = temp;
			}
			ps_kill(tp->son);
		}

/*	Cleanup own material						*/

		remove_event(tp->tep);
		remove_event(tp->qep);
		remove_event(tp->rtoep);		
		tp->qep = tp->tep = tp->rtoep = NULL_EVENT_PTR;
		SET_TASK_STATE(tp, TASK_FREE);
 		free(tp->name);
		release_locks(tp);
		release_ports(tp);
		free_table_entry(&ps_task_tab, task);
	

/*	Fix parent's son tree						*/

		if(tp->parent != DRIVER) {
			btp = ps_task_ptr(tp->parent);
			ctp = ps_task_ptr(btp->son);
			while(ctp != tp) {
				btp = ctp;
				ctp = ps_task_ptr(ctp->sibling);
			}
			if(btp == ps_task_ptr(tp->parent))
				btp->son = ctp->sibling;
			else
				btp->sibling = ctp->sibling;
		}
	}

/*	Clean up angio trace						*/
	if (angio_flag) {
		log_angio_event(tp, dye_ptr(tp->did), "wEnd");
		free_dye (tp->did);
	}

/*	if task is a cfs task, dq and free its si.*/
	switch(old_state) {
	
	case TASK_SYNC:
	case TASK_SYNC_SUSPEND:
		SET_TASK_STATE(tp, TASK_SYNC_FREE);

	case TASK_SYNC_FREE:
		break;

	case TASK_READY:
		dq_ready(np, tp);
	 
	case TASK_SLEEPING:
	case TASK_RECEIVING:
	case TASK_SUSPENDED:
	        if((si=tp->si))
			free(si);
#ifdef STACK_TESTING
		test_stack(task);
#endif /* STACK_TESTING */
		free(tp->stack_base);
		break;

	case TASK_SPINNING:
		dq_lock(tp);

	case TASK_BLOCKED:
	case TASK_COMPUTING:
	        if((si=tp->si)){
			if(si->on_rq)
				dq_cfs_task(tp);
			free(si);
		}
#ifdef STACK_TESTING
		test_stack(task);
#endif /* STACK_TESTING */
		free(tp->stack_base);
		find_ready(np, hp);	
		break;
	}
	
	return(OK);
}

/************************************************************************/

SYSCALL	ps_migrate(

/* Migrates a task to a new "node"/"host" processor combo. All locks	*/
/* are lost. The target task must be a descendant of the caller or the 	*/
/* caller itself. As well, a syncing task cannot be migrated. Any 	*/
/* port set surrogate tasks are migrated with the target.		*/
 
	long	task,				/* task id index	*/
	long	node,				/* node id index	*/
	long	host				/* host id index	*/
)
{
	ps_task_t	*tp;			/* task pointer		*/
	ps_node_t	*np, *onp;		/* node pointers	*/
	ps_cpu_t	*ohp;			/* old host pointer	*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/
	ps_port_t	*pp;			/* port pointer		*/
	long	port;				/* port index		*/
	ps_tp_pair_t *pairp;			/* tp pair pointer	*/

	if(task >= ps_task_tab.tab_size || task < 2)
		return(BAD_PARAM("task"));
	if(!ancestor(tp = ps_task_ptr(task)))
		return(BAD_PARAM("task"));
	if(node < 0 || node >= ps_node_tab.used)
		return(BAD_PARAM("node"));
	np = node_ptr(node);
	if(host != ANY_HOST && ((host < 0) || (host >= np->ncpu)))
		return(BAD_PARAM("host"));
	if(tp->state == TASK_FREE ||
           tp->state == TASK_SYNC ||
	   tp->state == TASK_SYNC_FREE ||
	   tp->state == TASK_SYNC_SUSPEND)
		return(BAD_CALL("Task syncing, free, or suspended"));
	if(node == tp->node && host == tp->host)
		return(OK);

	onp = node_ptr(tp->node);
	ohp = tp->hp;

	if(ts_flag) {
		sprintf(string, "migrating to node %ld", node);
		ts_report(tp, string);
	}

	tp->node = node;
	tp->host = tp->uhost = host;
	tp->hp = NULL_HOST_PTR;
	remove_event(tp->qep);
	tp->qep = NULL_EVENT_PTR;
	release_locks(tp);

/*	Migrate all port set surrogates too				*/

	port = tp->port_list;
	while(port != NULL_PORT) {
		pairp = (pp = port_ptr(port))->tplist;
		while(pairp) {
			ps_migrate(pairp->task, node, host);
			pairp = pairp->next;
		}
		port = pp->next;
	}

	switch(tp->state) {

	case TASK_READY:
		dq_ready(onp, tp);
		find_host(tp);

	case TASK_SUSPENDED:
	case TASK_RECEIVING:
	case TASK_SLEEPING:
		break;

	case TASK_COMPUTING:
 		tp->rct = (tp->tep->time - ps_now) * onp->speed / np->speed;

	case TASK_BLOCKED:
		remove_event(tp->tep);
		find_ready(onp, ohp);
		find_host(tp);
		break;

	case TASK_SPINNING:
		dq_lock(tp);

	case TASK_HOT:
		find_ready(onp, ohp);
		find_host(tp);
		break;
	}
	return(OK);
}

/************************************************************************/

SYSCALL ps_ready_queue(

/* Returns the length as well as the contents of the ready queue. Tasks	*/
/* such as the scheduler and the catchers on user defined nodes are not	*/
/* included.								*/

	long	node,
	long	size,
	long	*tasks
)
{
	long	count;				/* queue length counter	*/
	long	task;				/* task index		*/

	if(node < 0 || node >= ps_node_tab.used)
		return(BAD_PARAM("node"));

	count = 0;
	task = node_ptr(node)->rtrq;
	while(task != NULL_TASK) {
		if (ps_task_ptr(task)->priority <= MAX_PRIORITY) {
			if (count < size)
				tasks[count] = task;
			count++;
		}
		task = ps_task_ptr(task)->next;
	}
	return(count);	
}

/************************************************************************/

SYSCALL ps_receive_links(

/* Returns the receive links accessible by the specified "task".	*/
 
	long	task,				/* task index		*/
	long	*nlp,				/* # links pointer	*/
	long	link_array[]			/* link array		*/
)
{
	ps_comm_t	*cp;			/* comm pointer		*/
	ps_task_t	*tp;			/* task pointer		*/

	if(task >= ps_task_tab.tab_size || task < 0)
		return(BAD_PARAM("task"));
	if((tp = ps_task_ptr(task))->state == TASK_FREE)
		return(BAD_PARAM("task"));

	*nlp = 0;
	cp = node_ptr(tp->node)->rl_list;
	while(cp != NULL_COMM_PTR) {
		link_array[*nlp] = cp->blid;
		(*nlp)++;
		cp = cp->next;
	}
	return(OK);
}

/************************************************************************/

SYSCALL ps_resume(

/* Resumes a task in the TASK_SUSPENDED state allowing it to continue.	*/
/* N.B. Tasks resume execution where they were suspended but without any*/
/* lock ownership.							*/

	long	task				/* task id index	*/
)
{
	ps_task_t	*tp;			/* task pointers	*/

	if(task >= ps_task_tab.tab_size || task < 0)
		return(BAD_PARAM("task"));

	tp = ps_task_ptr(task);
	switch(tp->state) {

	case 	TASK_SUSPENDED:
		if (angio_flag) 
			log_angio_event(tp, dye_ptr(tp->did), "wAwaken");
		find_host(tp);
		return(OK);

	case	TASK_SYNC_SUSPEND:
	        if (angio_flag)
			log_angio_event(tp, dye_ptr(tp->did), "wAwaken");
		SET_TASK_STATE(tp, TASK_SYNC);
		return(OK);

	default:
	        return(BAD_CALL("Task is not suspended"));
	}
}
		
/************************************************************************/

SYSCALL	ps_send_links(

/* Returns the send links accessible by the specified "task".		*/

	long	task,				/* task index		*/
	long	*nlp,				/* # links pointer	*/
	long	link_array[]			/* link array		*/
)
{
	ps_comm_t	*cp;			/* comm pointer		*/
	ps_task_t	*tp;			/* task pointer		*/

	if(task >= ps_task_tab.tab_size || task < 0)
		return(BAD_PARAM("task"));
	if((tp = ps_task_ptr(task))->state == TASK_FREE)
		return(BAD_PARAM("task"));

	*nlp = 0;
	cp = node_ptr(tp->node)->sl_list;
	while(cp != NULL_COMM_PTR) {
		link_array[*nlp] = cp->blid;
		(*nlp)++;
		cp = cp->next;
	}
	return(OK);
}

/************************************************************************/

SYSCALL	ps_siblings(

/* Returns the siblings of the caller.					*/

	long	*nsp,				/* # siblings pointer	*/
	long	*sibling_array			/* sibling array	*/
)
{
	long	task;				/* task index		*/

	*nsp = 0;
	if((task = ps_my_parent) == 0)
		return(OK);
	task = ps_task_ptr(task)->son;
	while(task != NULL_TASK) {
		if(task != ps_myself) {
			(*nsp)++;
			*sibling_array++ = task;
		}
		task = ps_task_ptr(task)->sibling;
	}
	return(OK);
}

/************************************************************************/

SYSCALL	ps_sleep(

/* Causes the caller to enter a self-induced TASK_SLEEPING state for a	*/
/* specified "duration".  A non-positive duration is ignored.		*/

	double	duration			/* sleep period		*/
)
{
	ps_node_t	*np;			/* node pointer		*/
	ps_cpu_t	*hp;			/* host cpu pointer	*/
	char		string[40];		/* Output buffer	*/

	np = node_ptr(ps_htp->node);

	if(duration <= 0.0) {
		if((np->rtrq == NULL_TASK ) ||
		   ((np->discipline == PS_PR) && (ps_htp->priority != 
		      ps_task_ptr(np->rtrq)->priority)) ||
		   ((np->discipline == PS_HOL) && (ps_htp->priority >
		      ps_task_ptr(np->rtrq)->priority)))		
			return(OK);
		duration = 0.0;
	}
	hp = ps_htp->hp;
	if(np->discipline == PS_CFS){
		update_run_task(ps_htp);
		dq_cfs_task(ps_htp);
		/* cooling the cfs task */
		cooling_cfs_task(ps_htp);
		/* current task is preempted*/
	}
	

	ps_htp->hp = NULL_HOST_PTR;
	SET_TASK_STATE(ps_htp, TASK_SLEEPING);
	if(ts_flag)
		ts_report(ps_htp, "sleeping");
	if (angio_flag) {
		sprintf (string, "wDelay %g", duration);
		ps_log_user_event (string);
	}
	remove_event(ps_htp->qep);
	ps_htp->qep = NULL_EVENT_PTR;
	ps_htp->tep = add_event(ps_now+duration, END_SLEEP, (long *)ps_myself);
	find_ready(np, hp);

	return(OK);
}

/************************************************************************/

SYSCALL	ps_suspend(

/* Suspends a task releasing any locks it may hold.  A task in the 	*/
/* TASK_SYNC state will enter the TASK_SYNC_SUSPEND state; otherwise 	*/
/* tasks become TASK_SUSPENDED. If self suspension is involved, the 	*/
/* scheduler is invoked. 						*/

	long	task				/* task index		*/
)
{
	ps_task_t 	*tp;			/* task pointer		*/
	ps_node_t	*np;			/* node pointer		*/
	ps_cpu_t	*hp;			/* host pointer		*/
	long	old_state;			/* old state of task	*/

	if(task >= ps_task_tab.tab_size || task < 1)
		return(BAD_PARAM("task"));
	switch(old_state = (tp = ps_task_ptr(task))->state) {

	case TASK_SYNC:
		SET_TASK_STATE(tp, TASK_SYNC_SUSPEND);
 
	case TASK_SYNC_SUSPEND:
		return(OK);

	case TASK_SYNC_FREE:
	case TASK_FREE:
		return(BAD_PARAM("task"));

	default:
		SET_TASK_STATE(tp, TASK_SUSPENDED);
		if(ts_flag)
			ts_report(tp, "suspended");
		if(angio_flag)
			log_angio_event(tp, dye_ptr(tp->did), "wDelay FOREVER");
		remove_event(tp->qep);
		tp->qep = NULL_EVENT_PTR;
		release_locks(tp);
		np = node_ptr(tp->node);
		hp = tp->hp;
		tp->hp = NULL_HOST_PTR;

		switch(old_state) {

		case TASK_READY:
			dq_ready(np, tp);
			break;

		case TASK_SPINNING:
			dq_lock(tp);

		case TASK_BLOCKED:
		case TASK_COMPUTING:
			remove_event(tp->tep);
			tp->tep = NULL_EVENT_PTR;
		
		case TASK_HOT:

			if(np->discipline == PS_CFS){
				update_run_task(tp);
				dq_cfs_task(tp);
				/* cooling the cfs task */
				cooling_cfs_task(tp);
		
			}

			find_ready(np, hp);
			break;

		case TASK_RECEIVING:
		case TASK_SLEEPING:
			remove_event(tp->tep);
			tp->tep = NULL_EVENT_PTR;
			break;
		}
	}
	return(OK);
}
 

/************************************************************************/

SYSCALL	ps_sync(	

/* Retains processor for "delta" (scaled) cpu time w/o interruption.	*/
/* PR and RR sheduling are postponed until delta time expires.		*/

	double	delta				/* cpu time delta	*/
)
{
	char	string[40];

	if(delta < 0.0)
		return(BAD_PARAM("delta"));

/* WCS - 15 June 1999 - added */
	if(ps_my_node == 0) 
		return(BAD_CALL("ps_sync on node 0 not recommended -- block stats may not be reported"));

	ps_htp->tep = add_event(ps_now + delta / node_ptr(ps_htp->node)->speed, 
		END_SYNC, (long *)ps_myself);
	SET_TASK_STATE(ps_htp, TASK_SYNC);
	if (angio_flag) {
		sprintf (string, "wSync %g", delta);
		ps_log_user_event (string);
	}
	sched();

	return(OK);
}

	
/************************************************************************/
/*		Message Passing Related SYSCALLS			*/
/************************************************************************/

SYSCALL	ps_allocate_port(	

/* Allocates a named port to a specified existing "task". 		*/

	const	char 	*name,			/* port name		*/
	long	task				/* task index		*/
)
{
	long	port;				/* port id index	*/
	ps_port_t	*pp;			/* port pointer		*/
	ps_task_t	*tp;			/* task pointer		*/

	if(task >= ps_task_tab.tab_size || task < 0)
		return(BAD_PARAM("task"));
	if((tp = ps_task_ptr(task))->state == TASK_FREE)
		return(BAD_PARAM("task"));
	if((port = get_table_entry(&ps_port_tab)) == SYSERR)
		return(OTHER_ERR("growing port table"));

	(pp = port_ptr(port))->state = PORT_USED;
	if(!(pp->name = (char *) malloc(strlen(name) + 1)))
		ps_abort("Insufficient memory");
	sprintf(pp->name, "%s", name);
	pp->next = tp->port_list;
	tp->port_list = port;
	pp->first = pp->last = NULL_MESS_PTR;
	pp->nmess = 0;
	pp->owner = task;
	pp->tplist = (ps_tp_pair_t *) NULL;
	
	return(port);
}

/************************************************************************/

SYSCALL ps_allocate_port_set(

/* Allocates a named port set (port) to the calling task.		*/

	const	char	*name			/* port set name	*/
)
{
	long	port;				/* port set port	*/
	ps_port_t	*pp;			/* port pointer		*/

	if((port = get_table_entry(&ps_port_tab)) == SYSERR)
		return(OTHER_ERR("growing port table"));

	(pp = port_ptr(port))->state = PORT_SET;
	if(!(pp->name = (char *) malloc(strlen(name) + 1)))
		ps_abort("Insufficient memory");
	sprintf(pp->name, "%s", name);
	pp->next = ps_htp->port_list;
	ps_htp->port_list = port;
	pp->first = pp->last = NULL_MESS_PTR;
	pp->nmess = 0;
	pp->owner = ps_myself;
	pp->tplist = (ps_tp_pair_t *) NULL;
	
	return(port);
}

/************************************************************************/

SYSCALL	ps_allocate_shared_port(

/* Allocates a named port as a shared port. The port is owned by a 	*/
/* shared port dispatcher task sired by the caller.			*/

	const	char	*name			/* shared port name	*/
)
{

	char	string[TEMP_STR_SIZE];		/* temp string		*/
	long	dispatcher;			/* dispatcher task id	*/
	long	spid;				/* shared port id	*/
	sprintf(string, "%s - Shared Port Dispatcher", ps_htp->name);
	if(ps_resume(dispatcher = ps_create(string, ps_htp->node, ps_htp->host, 
	    shared_port_dispatcher, MAX_PRIORITY)) == SYSERR)
		return(OTHER_ERR("creating dispatcher"));

	spid = ps_std_port(dispatcher);
	port_ptr(spid)->state = PORT_SHARED;
	return(spid);
}

/************************************************************************/

SYSCALL ps_broadcast(
		
/* Globally broadcasts a message without use of a bus or a link.	*/

	long	type,				/* message type		*/
	char	*text,				/* message text	pointer	*/
	long	ack_port			/* acknowledge port	*/
)
{
	long	i;

	for(i = 1; i < ps_task_tab.tab_size; i++) 
		if(i != ps_myself && ps_task_ptr(i)->state != TASK_FREE)
			ps_send(ps_std_port(i), type, text, ack_port);
	return(OK);
}

/************************************************************************/

SYSCALL ps_buffer_size(			

/* Returns the size of a buffer if it is a member of a pool.		*/

	char	*bp				/* buffer pointer	*/
)
{
	ps_buf_t	*ibp;			/* internal buffer ptr	*/

	if(((size_t)bp)%4)
		return(BAD_PARAM("bp"));
	ibp = (ps_buf_t *) (bp - 2*sizeof(double));
	if(ibp->signature != BUFFER)
		return(BAD_PARAM("bp"));
	return(pool_ptr(ibp->pool)->size);
}

/************************************************************************/

SYSCALL	ps_build_buffer_pool(

/* Creates an empty buffer pool having buffers of the specified size.	*/

	long	size				/* buffer size		*/
)
{
	long	pool;				/* buffer pool id	*/
	ps_pool_t	*pp;			/* pool pointer		*/

	if(size < 1)
		return(BAD_PARAM("size"));
	if((pool = get_table_entry(&ps_pool_tab)) == SYSERR)
		return(OTHER_ERR("growing pool table"));

	pp = pool_ptr(pool);
	pp->size = size;
	pp->flist = NULL_BUF_PTR;
	return(pool);
}

/************************************************************************/

SYSCALL ps_bus_send(

/* Sends a message via "bus" to "port". The message type, logical size, */
/* text, and acknowledge port are passed in the remaining arguments.	*/

	long	bus,				/* bus used		*/
	long	port,				/* target port		*/
	long	type,				/* message type		*/
	long 	size,				/* message size (bytes)	*/
	char	*text,				/* message text pointer	*/
	long	ack_port			/* acknowledge port	*/
)
{
	long	sf, df;				/* flags		*/
	long	i;				/* loop index		*/
	long	nskip;				/* # messages skipped	*/
	ps_mess_t	*mp, *cmp;		/* message pointers	*/
	ps_mess_t	*bmp=0; 
	ps_bus_t	*bp;			/* bus pointer		*/
	ps_port_t	*pp;			/* port pointer		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/

	if(bus < 0 || bus >= ps_bus_tab.used) 
		return(BAD_PARAM("bus"));
	bp = bus_ptr(bus);
	if(port < 1 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state == PORT_FREE)
		return(BAD_PARAM("port"));
	sf = df = FALSE;
	for(i = 0; i < bp->nnodes; i++) {
		if(ps_htp->node == bp->bnode[i])
			sf = TRUE;
		if(ps_task_ptr(port_ptr(port)->owner)->node == bp->bnode[i])
			df = TRUE;
	}
	if(!df)
		return(BAD_CALL("owner of \"port\" not connected to bus"));
	if(!sf)
		return(BAD_CALL("Sender not connected to bus"));
	if(size < 0)
		return(BAD_PARAM("size"));

	mp = get_mess();
	if(bp->discipline == PS_RAND) {
		if(bp->nqueued) {
			nskip = (long) (drand48()*bp->nqueued) + 1;
			cmp = bp->head;
			for(i = 0; i < nskip; i++) {
				bmp = cmp;
				cmp = cmp->next;
			}
			bmp->next = mp;
			mp->next = cmp;
		}
		else {
			bp->head = mp;
			mp->next = NULL_MESS_PTR;
		}

	}
	else {
		if(bp->head == NULL_MESS_PTR)
			bp->head = mp;
		else
			bp->tail->next = mp;
		bp->tail = mp;
		mp->next = NULL_MESS_PTR;
	}
	(bp->nqueued)++;

	mp->sender = ps_myself;
	mp->port = mp->org_port = port;
	mp->ack_port = ack_port;
	mp->type = type;
	mp->size = size;
	mp->text = text;
	mp->c_code = BUS;
	mp->blid = bus;
	mp->ts = ps_now;
	mp->mid = next_mid++;
	if (angio_flag) {
		mp->did = derive_dye (ps_htp->did);
		log_angio_event (ps_htp, dye_ptr(mp->did), "wBegin");
	}
	if(ts_flag) {
		if (pp->state == PORT_SHARED)
			sprintf(string, "sending message %ld to shared port %ld via bus %ld",
				mp->mid, port, bus);
		else if (ps_task_ptr(pp->owner)->code == port_set_surrogate)
			sprintf(string, "sending message %ld to task %ld via port %ld and bus %ld",
				mp->mid, ps_task_ptr(pp->owner)->parent, port, bus);
		else  if (pp->owner != 0)
			sprintf(string, "sending message %ld to task %ld via port %ld and bus %ld",
				mp->mid, pp->owner, port, bus);
		ts_report(ps_htp, string);
	}
	if(bp->state == BUS_IDLE) {
		bp->state = BUS_BUSY;
		if(bp->stat != NULL_STAT)
			ps_record_stat(bp->stat, 1.0);
		bp->ep = add_event(ps_now + bus_delay, END_TRANS, (long *)mp);
	}
	return(OK);
}

/************************************************************************/

SYSCALL	ps_free_buffer(

/* Frees a buffer, returning it to the appropriate buffer pool.		*/

	char	*bp				/* buffer pointer	*/
)
{
	ps_buf_t	*ibp;			/* internal buffer ptr	*/
	ps_pool_t	*pp;			/* pool pointer		*/

	if(((size_t)bp)%4)
		return(BAD_PARAM("bp"));
	ibp = (ps_buf_t *) (bp - 2*sizeof(double));
	if(ibp->signature != BUFFER)
		return(BAD_PARAM("bp"));
	if(ibp->pool < 0 || ibp->pool >= ps_pool_tab.used)
		return(BAD_PARAM("bp"));
	
	pp = pool_ptr(ibp->pool);
	ibp->next = pp->flist;
	pp->flist = ibp;
	return(OK);
}

/************************************************************************/

char	*ps_get_buffer(

/* Gets a buffer from the specified buffer pool.			*/

	long	pool				/* buffer pool id	*/
)
{
	ps_buf_t	*ibp;			/* internal buffer ptr	*/
	ps_pool_t	*pp;			/* pool pointer		*/

	if(pool < 0 || pool >= ps_pool_tab.used)
	    return (char *)(BAD_PARAM("pool"));

	if((ibp = (pp = pool_ptr(pool))->flist) == NULL_BUF_PTR) {
		if((ibp = (ps_buf_t *) malloc(pp->size + 2*sizeof(double)))
		     == (ps_buf_t *) SYSERR) 
			ps_abort("Insufficient memory");
		ibp->signature = BUFFER;
		ibp->pool = pool;
	}
	else 
		pp->flist = ibp->next;

	return(((char *) ibp) + 2*sizeof(double));	
}

/************************************************************************/

SYSCALL	ps_join_port_set(

/* Adds an existing owned port to a specified port set.			*/

	long	port_set,			/* port set port index	*/
	long	port				/* added port index	*/
)
{
	ps_port_t	*psp;			/* port set pointer	*/
	ps_port_t	*pp;			/* port pointer		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/
	long 	surrogate;			/* surrogate task id	*/
	ps_tp_pair_t *pairp;			/* pair pointer		*/
	ps_tp_pair_t *get_pair();		/* tp pair generator	*/

	if(port_set == port)
		return(BAD_CALL("port == port_set"));
	if(port_set < 0 || port_set >= ps_port_tab.tab_size || 
	     port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((psp = port_ptr(port_set))->state != PORT_SET)
		return(BAD_PARAM("port_set"));
	if((pp = port_ptr(port))->state != PORT_USED)
		return(BAD_PARAM("port"));
	if(psp->owner != ps_myself || pp->owner != ps_myself)
		return(BAD_CALL("Caller is not owner"));
	if(port == ps_my_std_port)
		return(BAD_CALL("Port is caller's standard port"));
	sprintf(string,"%s Surrogate", ps_htp->name);
	if((surrogate = ps_create(string, ps_htp->node, ps_htp->host, 
	    port_set_surrogate, MAX_PRIORITY)) == SYSERR) 
		return(OTHER_ERR("creating port set surrogate task"));
	adjust_priority(surrogate, MAX_PRIORITY+4);

	/* WCS - 10 July 1997 - ps_port_tab may grow when surrogate is created, */
	/* because the surrogate is given a standard port.  Does the surrogate  */
	/* need a standard port?  Anyways, psp reflects the pointer before the  */
	/* table grows, so must be updated. 					*/
	psp = port_ptr(port_set);

	pairp = get_pair();
	pairp->task = surrogate;
	pairp->port = port;
	pairp->next = psp->tplist;
	psp->tplist = pairp;

	ps_resume(surrogate);
	ps_pass_port(port, surrogate);
	ps_send(ps_std_port(surrogate), port, "", port_set);

	return(OK);
}

/************************************************************************/

SYSCALL	ps_leave_port_set(

/* Removes a port from the specified port set & returns it to caller.	*/

	long	port_set,			/* port set port index	*/
	long	port				/* removed port	index	*/
)
{
	ps_port_t	*psp;			/* port set pointer	*/
	ps_tp_pair_t *pairp;			/* pair pointer		*/
	ps_tp_pair_t *lastp;			/* last pair pointer	*/
	void	free_pair();			/* tp pair sink		*/
	long	i;				/* loop index		*/
	long	istop;				/* loop stopper		*/
	long	type;				/* message type		*/
	double	ts;				/* message time stamp	*/
	char	*tp;				/* message text pointer	*/
	long	rp;				/* message reply port	*/
	long	oport;				/* message original port*/
	long 	mid;				/* unique message id	*/
	long	did;				/* dye id		*/
/* WCS - 17 July 1997 - added */
	long	save_flag;			/* saves ts_flag	*/

	if(port_set < 0 || port_set >= ps_port_tab.tab_size)
		return(BAD_PARAM("port_set")); 
	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((psp = port_ptr(port_set))->state != PORT_SET)
		return(BAD_PARAM("port_set"));
	if(psp->owner != ps_myself)
		return(BAD_CALL("Caller is not port set owner"));

	pairp = psp->tplist;
	while(pairp) {
		if(pairp->port == port)
			break;
		lastp = pairp;
		pairp = pairp->next;
	}
	if(!pairp)
		return(BAD_CALL("port is not a member of port set"));

	ps_pass_port(port, ps_myself);

/* WCS - 17 July 1997 - added to remove unwanted statements from trace. */
	save_flag = ts_flag;
	ts_flag = FALSE; 

	istop = psp->nmess;
	for(i = 0; i < istop; i++) {
		port_receive(PS_FIFO,port_set,NEVER,&type,&ts,&tp,&rp,&oport,&mid,
		    &did);
		if(oport == port) 
			port_send(port, type, ts, tp, rp, oport, mid, did);
		else
			port_send(port_set, type, ts, tp, rp, oport, mid, did);
	}

	ts_flag = save_flag;

	return(OK);
}

/************************************************************************/

SYSCALL	ps_link_send(

/* Sends a message via "link" to "port". The message type, logical size,*/
/* text, and acknowledge port are passed in the remaining arguments.	*/

	long	link,				/* link used		*/
	long	port,				/* target port		*/
	long	type,				/* message type		*/
	long	size,				/* message size (bytes)	*/
	char	*text,				/* message text	pointer	*/
	long	ack_port			/* acknowledge port	*/
)
{
	ps_link_t	*lp;			/* link pointer		*/
	ps_mess_t	*mp;			/* message pointer	*/
	ps_port_t	*pp;			/* port pointer		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/

	if(link < 0 || link >= ps_link_tab.used)
		return(BAD_PARAM("link"));
	lp = link_ptr(link);
	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state == PORT_FREE)
		return(BAD_PARAM("port"));
	if(lp->snode != ps_htp->node)
		return(BAD_CALL("Sender not connected to link"));
	if(lp->dnode != ps_task_ptr(pp->owner)->node)
		return(BAD_CALL("Port owner not connected to link"));
	if(size < 0)
		return(BAD_PARAM("size"));

	mp = get_mess();
	if(lp->head == NULL_MESS_PTR)
		lp->head = mp;
	else
		lp->tail->next = mp;
	lp->tail = mp;

	mp->sender = ps_myself;
	mp->port = mp->org_port = port;
	mp->ack_port = ack_port;
	mp->type = type;
	mp->size = size;
	mp->text = text;
	mp->c_code = LINK;
	mp->blid = link;
	mp->ts = ps_now;
	mp->next = NULL_MESS_PTR;
	mp->mid = next_mid++;
	if (angio_flag) {
		mp->did = derive_dye (ps_htp->did);
		log_angio_event (ps_htp, dye_ptr(mp->did), "wBegin");
	}
	if(ts_flag) {
		if (pp->state == PORT_SHARED)
			sprintf(string, "sending message %ld to shared port %ld via link %ld",
				mp->mid, port, link);
		else if (ps_task_ptr(pp->owner)->code == port_set_surrogate)
			sprintf(string, "sending message %ld to task %ld via port %ld and link %ld",
				mp->mid, ps_task_ptr(pp->owner)->parent, port, link);
		else  if (pp->owner != 0)
			sprintf(string, "sending message %ld to task %ld via port %ld and link %ld",
				mp->mid, pp->owner, port, link);
		ts_report(ps_htp, string);
	}
	if(lp->state == LINK_IDLE) {
		lp->state = LINK_BUSY;
		if(lp->stat != NULL_STAT)
			ps_record_stat(lp->stat, 1.0);
		lp->ep = add_event(ps_now + link_delay, END_TRANS, (long *)mp);
	}
	return(OK);
}

/************************************************************************/

SYSCALL ps_localcast(		

/* Broadcasts a message to all tasks on caller's node without use of a 	*/
/* bus or a link.							*/

	long	type,				/* message type		*/
	char	*text,				/* message text	pointer	*/
	long	ack_port			/* acknowledge port	*/
)
{
	long	i;

	for(i = 1; i < ps_task_tab.tab_size; i++) 
		if(i != ps_myself && ps_task_ptr(i)->state != TASK_FREE
		    && ps_task_ptr(i)->node == ps_my_node)
			ps_send(ps_std_port(i), type, text, ack_port);
	return(OK);
}

/************************************************************************/

SYSCALL ps_multicast(	

/* Broadcasts a message to descendants only without use of a bus or a 	*/
/* link.								*/

	long	type,				/* message type		*/
	char	*text,				/* message text	pointer	*/
	long	ack_port			/* acknowledge port	*/
)
{
	long	i;

	for(i = 0; i < ps_task_tab.tab_size; i++) 
		if(i != ps_myself && ps_task_ptr(i)->state != TASK_FREE
		    && ancestor(ps_task_ptr(i)))
			ps_send(ps_std_port(i), type, text, ack_port);
	return(OK);
}

/************************************************************************/

SYSCALL	ps_my_ports(	

/* Returns ports owned by caller excluding those in port sets. 		*/

	long	*pcountp,			/* port count pointer	*/
	long	port_array[]			/* port array		*/
)
{
	long	port;				/* port index		*/
	long	pcount;				/* port count		*/

	pcount = 0;
	port = ps_htp->port_list;
	while(port != NULL_PORT) {
		port_array[pcount++] = port;
		port = port_ptr(port)->next;
	}

	*pcountp = pcount;
	return(OK);
}

/************************************************************************/

SYSCALL	ps_owner(

/* Returns the task index of the owner of the specified port.		*/

	long	port				/* port index		*/
)
{
	ps_port_t	*pp;			/* port pointer		*/

	if(port < 0 || port >= ps_port_tab.used)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state == PORT_FREE)
		return(BAD_PARAM("port"));
	
	return(pp->owner);
}

/************************************************************************/

SYSCALL	ps_pass_port( 		

/* Transfers a "port" to another "task".  Caller must be port owner or 	*/
/* an ancestor of the owner.  All existing queued messages are retained.*/
/* "Shared", "standard" and "port set" ports cannot be passed.		*/

	long	port,				/* port id index	*/
	long	task				/* task id index	*/
)
{
	long	cport;				/* current port index	*/
	long	bport;				/* before port index	*/
	ps_port_t	*pp;			/* port pointers	*/
	ps_task_t	*tp1, *tp2;		/* task pointers	*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/
	ps_tp_pair_t *pairp;			/* pair pointer		*/
	ps_tp_pair_t *lastp;			/* last pair pointer	*/
	void	free_pair();			/* tp pair sink		*/
	long	task2;				/* Saves the owner	*/

	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state != PORT_USED)
		return(BAD_PARAM("port"));
	if(!ancestor(tp1 = ps_task_ptr(pp->owner)))
		return(BAD_CALL("Caller is not ancestor of port owner"));
	if(port == tp1->bport)
		return(BAD_CALL("Port is a standard port"));
	if(task >= ps_task_tab.tab_size || task < 0)
		return(BAD_PARAM("task"));
	if((tp2 = ps_task_ptr(task))->state == TASK_FREE)
		return(BAD_PARAM("task"));

/* 	Switch port to other port list					*/
	task2 = pp->owner;
	pp->owner = task;
	cport = tp1->port_list;
	while(cport != port && cport != NULL_PORT) {
		bport = cport;
		cport = port_ptr(cport)->next;
	}
	if(cport == tp1->port_list)
		tp1->port_list = pp->next;
	else
		port_ptr(bport)->next = pp->next;

	pp->next = tp2->port_list;
	tp2->port_list = port;


	if(ts_flag) {
		if (tp1->code == port_set_surrogate) {
			sprintf(string, "removes port %ld from port set %ld", 
			    port, lookup_port_set(tp1->parent, task2));
		}
		else if (tp2->code == port_set_surrogate)
			sprintf(string,"inserts port %ld in port set %ld", port,
			    lookup_port_set(tp2->parent, task));
		else 
			sprintf(string, "passes port %ld to task %ld", port, 
			    task);
		ts_report(ps_htp, string);
	}

/*	Kill original owner if a surrogate owner of a port set port	*/

	cport = (tp2 = ps_task_ptr(tp1->parent))->port_list;
	while(cport != NULL_PORT) {
		if((pp = port_ptr(cport))->state == PORT_SET) {
			pairp = pp->tplist;
			while(pairp) {
				if(pairp->port == port)
					break;
				lastp = pairp;
				pairp = pairp->next;
			}
			if(pairp) {
				if(pairp == pp->tplist)
					pp->tplist = pairp->next;
				else
					lastp->next = pairp->next;
				ps_kill(pairp->task);
				free_pair(pairp);
				break;
			}
		}
		cport = pp->next;
	}

/*	Release original owner if blocked on the passed port		*/

	if(tp1->state == TASK_RECEIVING && tp1->wport == port) {
		remove_event(tp1->rtoep);
		tp1->rtoep = NULL_EVENT_PTR;
		tp1->wport = NULL_PORT;
		find_host(tp1);
	}

	return(OK);
}

/************************************************************************/

SYSCALL ps_receive(

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
)
{
	long	retval;				/* return value		*/
	long	op;				/* original port index	*/
	long	mid;				/* unique message id 	*/
	long	did;				/* dye id		*/

	retval = port_receive(PS_FIFO, port, time_out, typep, tsp, texth, app, 
	    &op, &mid, &did);
	if (retval != SYSERR && angio_flag) {
		end_trace (ps_htp);
		ps_htp->did = did;
	}
	return retval;
}

/************************************************************************/

SYSCALL ps_receive_last(

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
)
{
	long	retval;				/* return value		*/
	long	op;				/* original port index	*/
	long	mid;				/* unique message id 	*/
	long	did;				/* dye id		*/

	retval = port_receive(PS_LIFO, port, time_out, typep, tsp, texth, app, 
	    &op, &mid, &did);
	if (retval != SYSERR && angio_flag) {
		end_trace (ps_htp);
		ps_htp->did = did;
	}
	return retval;
}

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
)
{
	long	retval;				/* return value		*/
	long	op;				/* original port index	*/
	long	mid;				/* unique message id 	*/
	long	did;				/* dye id		*/

	retval = port_receive(PS_RAND, port, time_out, typep, tsp, texth, app, 
	    &op, &mid, &did);
	if (retval != SYSERR && angio_flag) {
		end_trace (ps_htp);
		ps_htp->did = did;
	}
	return retval;
}


/************************************************************************/

SYSCALL ps_receive_priority(

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
)
{
	long	retval;				/* return value		*/
	long	op;				/* original port index	*/
	long	mid;				/* unique message id 	*/
	long	did;				/* dye id		*/

	retval = port_receive(PS_HOL, port, time_out, typep, tsp, texth, app, 
	    &op, &mid, &did);
	if (retval != SYSERR && angio_flag) {
		end_trace (ps_htp);
		ps_htp->did = did;
	}
	return retval;
}

/************************************************************************/

SYSCALL	ps_receive_shared(

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
)
{
	char	name[50];			/* name string		*/
	long	status;				/* receive status	*/
	long	save_flag;			/* saves ts_flag	*/

	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if(port_ptr(port)->state != PORT_SHARED)
		return(BAD_PARAM("port"));
	if(ps_htp->blind_port == NULL_PORT) {
		sprintf(name, "%s - Blind Port", ps_htp->name);
		if((ps_htp->blind_port = ps_allocate_port(name, ps_myself)) 
		    == SYSERR) {
			ps_htp->blind_port = NULL_PORT;
			return(OTHER_ERR("allocating blind port"));
		}
	}

	save_flag = ts_flag;
	ts_flag = FALSE; 
	port_send(port, SP_REQUEST, ps_now, "", ps_htp->blind_port, 0, 0, 0);
	ts_flag = save_flag;
	if(time_out == IMMEDIATE)
		ps_sleep(0.0);
	if((status = ps_receive(ps_htp->blind_port, time_out, typep, tsp, 
	    texth, app)) != OK || *typep == ACK_TIMEOUT) {
		port_send(port, SP_CANCEL, ps_now, "", ps_htp->blind_port, 0, 
		    0, 0);
		if(time_out == IMMEDIATE)
			ps_sleep(0.0);
	}
	return(status);
}
		
/************************************************************************/

SYSCALL	ps_release_port(		

/* Deallocates a specified "port" from a "task".  The target task must	*/
/* be the caller or one of its descendants.  All queued messages are 	*/
/* lost with possible memory leak if recipient was to free text string.	*/
/* If port is a "port set" port, any surrogates are killed along with 	*/
/* all ports and messages within the set.				*/

	long	port				/* port index		*/
)
{
	long	cport;				/* current port index	*/
	long	bport;				/* before port index	*/
	ps_port_t	*pp;			/* port pointers	*/
	ps_mess_t	*mp, *nmp;		/* message pointers	*/
	ps_tp_pair_t *spp;			/* sp pair pointer	*/
	ps_task_t	*tp;			/* task pointer		*/

	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state == PORT_FREE)
		return(OK);
	if(pp->state == PORT_SHARED)
		return(BAD_CALL("Port is a shared port"));
	if(!ancestor(tp = ps_task_ptr(pp->owner)))
		return(BAD_CALL("Caller is not ancestor of port owner"));
	if((port == tp->bport) && (tp == ps_htp))
		return(BAD_CALL("Port is a standard port"));

	pp->state = PORT_FREE;
	free(pp->name);
	mp = pp->first;
	while(mp != NULL_MESS_PTR) {
			nmp = mp->next;
			free_mess(mp);
			mp = nmp;
	}
	spp = pp->tplist;
	while(spp) {
		ps_kill(spp->task);
		spp = spp->next;
	}
	cport = tp->port_list;
	while(cport != port && cport != NULL_PORT) {
		bport = cport;
		cport = port_ptr(cport)->next;
	}
	if(cport == tp->port_list)
		tp->port_list = pp->next;
	else
		port_ptr(bport)->next = pp->next;

	if(tp->bport == port)
		tp->bport = NULL_PORT;

	pp->owner = NULL_TASK;
	free_table_entry(&ps_port_tab, port);

	if(tp->state == TASK_RECEIVING && tp->wport == port) {
		remove_event(tp->rtoep);
		tp->rtoep = NULL_EVENT_PTR;
		tp->wport = NULL_PORT;
		find_host(tp);
	}
	return(OK);
}

/************************************************************************/

SYSCALL	ps_release_shared_port(

/* Deallocates the specified shared "port" by killing its dispatcher 	*/
/* host. To be successful the caller must be the task which allocated	*/
/* the shared port or an ancestor of it. All queued messages are lost	*/
/* with a possible memory leak if the recipient was to free text string.*/

	long	port				/* shared port index	*/
)
{
	ps_port_t	*pp;			/* port pointer		*/

	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state != PORT_SHARED)
		return(BAD_PARAM("port"));

	return(ps_kill(pp->owner));
}

/************************************************************************/

SYSCALL ps_resend(

/* Sends a message to "port" without use of a bus or a link.  The type,	*/
/* timestamp, text, and acknowledge port are passed in the remaining 	*/
/* arguments. Typically used for redirecting a received message and 	*/
/* preserving its timestamp.						*/

	long	port,				/* target port		*/
	long	type,				/* message type		*/
	double	ts,				/* message timestamp	*/
	char	*text,				/* message text pointer	*/
	long	ack_port			/* acknowledge port	*/
)
{
	ps_mess_t	*mp;			/* message pointer	*/
	ps_port_t	*pp;			/* port pointer		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/
	ps_task_t	*tp;			/* task pointer		*/

	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state == PORT_FREE)
		return(BAD_PARAM("port"));

	mp = get_mess();
	if(pp->nmess++) {
		pp->last->next = mp;
		pp->last = mp;
	}
	else 
		pp->first = pp->last = mp;

	mp->sender = ps_myself;
	mp->port = mp->org_port = port;
	mp->ack_port = ack_port;
	mp->c_code = MAGIC;
	mp->type = type;
	mp->text = text;
	mp->ts = ts;
	mp->mid = next_mid ++;
	if (angio_flag) {
		mp->did = derive_dye (ps_htp->did);
		log_angio_event (ps_htp, dye_ptr(ps_htp->did), "wBegin");
	}
	mp->next = NULL_MESS_PTR;
	if(ts_flag && ps_htp != DRIVER_PTR) {
		sprintf(string, "sending message %ld to task %ld via port %ld",
			mp->mid, pp->owner, port);
		ts_report(ps_htp, string);
	}
	tp = ps_task_ptr(pp->owner);
	if((tp->state == TASK_RECEIVING) && (tp->wport == port)) {
		remove_event(tp->rtoep);
		tp->rtoep = NULL_EVENT_PTR;
		tp->wport = NULL_PORT;
		find_host(tp);
	}
	return(OK);
}

/************************************************************************/

SYSCALL	ps_send(

/* Sends a message to "port" without use of a bus or a link.  The type,	*/
/* text, and acknowledge port are passed in the remaining arguments.	*/

	long	port,				/* target port		*/
	long	type,				/* message type		*/
	char	*text,				/* message text pointer	*/
	long	ack_port			/* acknowledge port	*/
)
{
/* WCS - 17 July 1999 - Copied checks so that error reported accurately */
	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((port_ptr(port))->state == PORT_FREE)
		return(BAD_PARAM("port"));

	return ps_send_priority(port, type, text, ack_port, 0);
}


/************************************************************************/

SYSCALL	ps_send_priority(

/* Sends a message to "port" without use of a bus or a link.  The type,	*/
/* text, and acknowledge port are passed in the remaining arguments.	*/

	long	port,				/* target port		*/
	long	type,				/* message type		*/
	char	*text,				/* message text pointer	*/
	long	ack_port,			/* acknowledge port	*/
	long	priority			/* Priority of message	*/
)
{
	ps_mess_t	*mp;			/* message pointer	*/
	ps_port_t	*pp;			/* port pointer		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/
	ps_task_t	*tp;			/* task pointer		*/

	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state == PORT_FREE)
		return(BAD_PARAM("port"));

	mp = get_mess();
	if(pp->nmess++) {
		pp->last->next = mp;
		pp->last = mp;
	}
	else 
		pp->first = pp->last = mp;

	mp->sender = ps_myself;
	mp->port = mp->org_port = port;
	mp->ack_port = ack_port;
	mp->c_code = MAGIC;
	mp->type = type;
	mp->text = text;
	mp->ts = ps_now;
	mp->pri = priority;
	mp->next = NULL_MESS_PTR;
	mp->mid = next_mid++;
	if (angio_flag) {
		mp->did = derive_dye (ps_htp->did);
		log_angio_event (ps_htp, dye_ptr(mp->did), "wBegin");
	}
	if(ts_flag && ps_htp != DRIVER_PTR) {
		if (pp->state == PORT_SHARED) {
			sprintf(string, "sending message %ld to shared port %ld",
				mp->mid, port);
			ts_report(ps_htp, string);
		}
		else if (ps_task_ptr(pp->owner)->code == port_set_surrogate) {
			if (port != ps_std_port(pp->owner)){
				sprintf(string, 
				    "sending message %ld to task %ld via port %ld",
				    mp->mid, ps_task_ptr(pp->owner)->parent, 
				    port);
				ts_report(ps_htp, string);
			}
		}
		else if (pp->owner != 0) {
			sprintf(string, 
			    "sending message %ld to task %ld via port %ld",
			    mp->mid, pp->owner, port);
			ts_report(ps_htp, string);
		}
	}
	tp = ps_task_ptr(pp->owner);
	if((tp->state == TASK_RECEIVING) && (tp->wport == port)) {
		remove_event(tp->rtoep);
		tp->rtoep = NULL_EVENT_PTR;
		tp->wport = NULL_PORT;
		find_host(tp);
	}
	return(OK);
}

/************************************************************************/
/*		Synchronization Related SYSCALLS			*/
/************************************************************************/

SYSCALL	ps_lock(

/* Acquires the specified "lock" waiting by spinning if necessary.	*/

	long	lock				/* lock index		*/
)
{
	long	i;				/* loop index		*/
	ps_lock_t	*lp;			/* lock pointer		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/

	if(lock < 0)
		return(BAD_PARAM("lock"));

	while(lock >= ps_lock_tab.tab_size) {
		ps_lock_tab.used = ps_lock_tab.tab_size;
		if(get_table_entry(&ps_lock_tab) == SYSERR)
			return(OTHER_ERR("growing lock table"));
		for(i = ps_lock_tab.tab_size/2; i < ps_lock_tab.tab_size; i++) {
			lp =  lock_ptr(i);
			lp->state = UNLOCKED;
			lp->owner = lp->queue = NULL_TASK;
			lp->count = 0;
		}
	} 

	if(angio_flag) {
		sprintf(string, "wLockSpinning %ld", lock);
		ps_log_user_event(string);
	}
	while(TRUE) {		/* loop required in case of preemption	*/
		if((lp = lock_ptr(lock))->state == LOCKED) {
			if(lp->owner == ps_myself)
				break;
			ps_htp->next = lp->queue;
			lp->queue = ps_myself;
			lp->count++;
			ps_htp->spin_lock = lock;
			if(ts_flag) {
				sprintf(string, "spinning on lock %ld", lock);
				ts_report(ps_htp, string);
			}
			SET_TASK_STATE(ps_htp, TASK_SPINNING);
			sched();
		}
		else {
			dq_lock(ps_htp);
			lp->state = LOCKED;
			lp->owner = ps_myself;
			lp->next = ps_htp->lock_list;
			ps_htp->lock_list = lock;
			if(ts_flag) {
				sprintf(string, "locking lock %ld", lock);
				ts_report(ps_htp, string);
			}
			break;
		}
	}
	if(angio_flag) {
		sprintf(string, "wLockObtained %ld", lock);
		ps_log_user_event(string);
	}
	return(OK);
}
		
/************************************************************************/

SYSCALL	ps_reset_semaphore(

/* Resets a specified semaphore to a count value clearing task queue.	*/

	long	sid,				/* semaphore index	*/
	long	value				/* semaphore count value*/
)
{
	ps_sema_t 		*sp;		/* semaphore pointer	*/
	long		i;			/* loop index		*/
	char		string[30];		/* trace string		*/
	ps_tp_pair_t	*pp;			/* tp pair pointer	*/
	ps_tp_pair_t	*opp;			/* old tp pair pointer	*/
	void	free_pair();			/* tp pair sink		*/

	if(sid < 0)
		return(BAD_PARAM("sid"));
	if(value < 0)
		return(BAD_PARAM("value"));

	while(sid >= ps_sema_tab.tab_size) {
		ps_sema_tab.used = ps_sema_tab.tab_size;
		if(get_table_entry(&ps_sema_tab) == SYSERR)
			return(OTHER_ERR("growing semaphore table"));
		for(i = ps_sema_tab.tab_size/2; i < ps_sema_tab.tab_size; i++) {
			sp =  sema_ptr(i);
			sp->queue = NULL_PAIR_PTR;
			sp->count = 1;
		}
	}

	if(ts_flag) {
		sprintf(string, "resetting semaphore %ld to value %ld", sid, value);
		ts_report(ps_htp, string);
	} 
	if(angio_flag) {
		sprintf(string, "wSemaReset %ld %ld", sid, value);
		ps_log_user_event(string);
	}

	(sp = sema_ptr(sid))->count = value;
	pp = sp->queue;
	while((opp = pp)) {
		ps_resume(pp->task);
		pp = pp->next;
		free_pair(opp);
	}
	sp->queue = NULL_PAIR_PTR;
	return(OK);
}

/************************************************************************/

SYSCALL ps_signal_semaphore(

/* Signals a specified semaphore possibly resuming a queued task.	*/

	long	sid				/* semaphore index	*/
)
{
	long		i;			/* loop index		*/
	ps_sema_t		*sp;		/* semaphore pointer	*/
	char		string[30];		/* trace string		*/
	ps_tp_pair_t	*qpp;			/* tp queue pointer	*/
	void	free_pair();			/* tp pair sink		*/

	if(sid < 0)
		return(BAD_PARAM("sid"));

	while(sid >= ps_sema_tab.tab_size) {
		ps_sema_tab.used = ps_sema_tab.tab_size;
		if(get_table_entry(&ps_sema_tab) == SYSERR)
			return(OTHER_ERR("growing semaphore table"));
		for(i = ps_sema_tab.tab_size/2; i < ps_sema_tab.tab_size; i++) {
			sp =  sema_ptr(i);
			sp->queue = NULL_PAIR_PTR;
			sp->count = 1;
		}
	} 

	if(ts_flag) {
		sprintf(string, "signalling semaphore %ld", sid);
		ts_report(ps_htp, string);
	}
	if(angio_flag) {
		sprintf(string, "wSemaSignal %ld", sid);
		ps_log_user_event(string);
	}

	if((sp = sema_ptr(sid))->count++ < 0) {
		qpp = sp->queue;
		sp->queue = qpp->next;
		ps_resume(qpp->task);
		free_pair(qpp);
	}
	return(OK);
}	

/************************************************************************/

SYSCALL	ps_unlock(

/* Releases the specified "lock" if held by the caller or one of its 	*/
/* descendants.								*/

	long	lock				/* lock index		*/
)
{	long	clock;				/* current lock index	*/
	ps_lock_t	*lp, *blp;		/* lock pointers	*/
	ps_lock_t	*clp=0;
	ps_task_t	*tp;			/* task pointer		*/
	long	i, n;				/* loop indices		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/

	if(lock < 0 || lock >= ps_lock_tab.tab_size)
		return(BAD_PARAM("lock"));
	if((lp = lock_ptr(lock))->state == UNLOCKED)
		return(OK);
	if(!ancestor((tp = ps_task_ptr(lp->owner))))
		return(BAD_CALL("Caller is not an ancestor of lock owner"));

	clock = tp->lock_list;
	while(clock != NULL_LOCK && (clp = lock_ptr(clock)) != lp) {
		blp = clp;
		clock = clp->next;
	}
	if(clock == tp->lock_list)
		tp->lock_list = clp->next;
	else
		blp->next = clp->next;

	if(ts_flag) {
		sprintf(string, "unlocking lock %ld", lock);
		ts_report(ps_htp, string);
	}
	if(angio_flag) {
		sprintf(string, "wLockUnlock %ld", lock);
		ps_log_user_event(string);
	}

	if(lp->queue == NULL_TASK) {
		lp->owner = NULL_TASK; 
		lp->state = UNLOCKED;
	}
	else {
		tp = ps_task_ptr(lp->queue);
		n = ps_choice(lp->count);
		for(i = 0; i < n; i++) 
			tp = ps_task_ptr(tp->next);
		lp->owner = tid(tp);
		dq_lock(tp);
		lp->next = tp->lock_list;
		tp->lock_list = lock;
		SET_TASK_STATE(tp, TASK_BLOCKED);
		tp->tep = add_event(ps_now, END_BLOCK, (long *)lp->owner);
		if(ts_flag) {
			sprintf(string, "locking lock %ld", lock);
			ts_report(tp, string);
		}
	}

	return(OK);
}
		
/************************************************************************/

SYSCALL	ps_wait_semaphore(

/* Waits on a specified semaphore if its count value is <= 0.		*/

	long	sid				/* semaphore index	*/
)
{
	ps_sema_t 		*sp;		/* semaphore pointer	*/
	long		i;			/* loop index		*/
	char		string[30];		/* trace string		*/
	ps_tp_pair_t	*pp;			/* tp pair pointer	*/
	ps_tp_pair_t	*qpp;			/* tp queue pointer	*/
	ps_tp_pair_t	*opp;			/* old tp pair pointer	*/
	ps_tp_pair_t	*get_pair();		/* tp pair generator	*/

	if(sid < 0)
		return(BAD_PARAM("sid"));

	while(sid >= ps_sema_tab.tab_size) {
		ps_sema_tab.used = ps_sema_tab.tab_size;
		if(get_table_entry(&ps_sema_tab) == SYSERR)
			return(OTHER_ERR("growing semaphore table"));
		for(i = ps_sema_tab.tab_size/2; i < ps_sema_tab.tab_size; i++) {
			sp =  sema_ptr(i);
			sp->queue = NULL_PAIR_PTR;
			sp->count = 1;
		}
	} 

	if(ts_flag) {
		sprintf(string, "waiting on semaphore %ld", sid);
		ts_report(ps_htp, string);
	} 
	if(angio_flag) {
		sprintf(string, "wSemaBlocked %ld", sid);
		ps_log_user_event(string);
	}

	if((sp = sema_ptr(sid))->count-- <= 0) {
		if (ts_flag) {
			sprintf(string, "blocked on semaphore %ld", sid);
			ts_report(ps_htp, string);
		}
		pp = get_pair();
		pp->task = ps_myself;
		pp->next = NULL_PAIR_PTR;
		qpp = sp->queue;
		while(qpp) {
			opp = qpp; 
			qpp = qpp->next;
		}
		if(qpp != sp->queue)
			opp->next = pp;
		else
			sp->queue = pp;
		ps_suspend(ps_myself);
	}
	if(angio_flag) {
		sprintf(string, "wSemaUnblocked %ld", sid);
		ps_log_user_event(string);
	}
	return(OK);
}	

/************************************************************************/
/*		Statistics Related SYSCALLS				*/
/************************************************************************/

SYSCALL	ps_block_stats(

/* Function reports blocked statistics by turning caller into a blocker.*/

	long	nb,				/* number of blocks	*/
	double	delay				/* transient delay	*/
)
{
	long	tid;

	if(bs_time >= 0.0) 
		return(BAD_CALL("ps_block_stats has already been called"));
	if(nb < 2)
		return(BAD_PARAM("nb"));
	if(delay < 0.0 || delay > (ps_run_time - ps_now))
		return(BAD_PARAM("delay"));

	ps_resume (tid = ps_create("Block Stat Collector", 0, ANY_HOST, 
	    block_stats_collector, MAX_PRIORITY));
	adjust_priority(tid, MAX_PRIORITY+4);
	ps_resend(ps_std_port(tid), nb, delay, "", NULL_PORT);

	return(OK);
}

/************************************************************************/

SYSCALL	ps_get_stat(

/* Returns the mean and either the number of observations (SAMPLE) or 	*/
/* the observation period (VARIABLE) of the specified statistic.	*/

	long	stat,				/* statistics index	*/
	double	*meanp,				/* mean pointer		*/
	double	*osp				/* other stat pointer	*/
)
{
	ps_stat_t	*sp;			/* statistics pointer	*/

	if(stat < 0 || stat >= ps_stat_tab.used)
		return(BAD_PARAM("stat"));

	switch((sp = stat_ptr(stat))->type) {

	case SAMPLE:
		
		*osp = sp->values.sam.count;
		if(*osp)
			*meanp = sp->values.sam.sum/ *osp;
		else
			*meanp = 0.0;
		break;

	case VARIABLE:

		*osp = ps_now - sp->values.var.start;
		if(*osp > 0.0) {
			sp->values.var.integral += (ps_now - 
		    	sp->values.var.old_time) *
		    	sp->values.var.old_value;
			sp->values.var.old_time = ps_now;
			*meanp = sp->values.var.integral / *osp;
		}
		else
			*meanp = 0.0;
		break;

	case RATE:
		*meanp = sp->values.rat.count / (ps_now - sp->values.rat.start);
		*osp = (double)sp->values.rat.count;
		break;

	default:
		
		return(BAD_CALL("Invalid statistic type"));
	}

	return(OK);
}

/************************************************************************/

SYSCALL	ps_open_stat(

/* Opens & initializes a statistic. 					*/

	const	char	*name,			/* statistic name	*/
	long	type				/* type of statistic	*/
)
{
	ps_stat_t	*sp;			/* statistic pointer	*/
	long	stat;				/* statistic index	*/

	if(bs_time >= 0.0 && bs_time < ps_now)
		return(BAD_CALL("Doesn't work once block stats is in effect"));
	if(type != SAMPLE && type != VARIABLE && type != RATE)
		return(BAD_PARAM("type"));
	if((stat = get_table_entry(&ps_stat_tab)) == SYSERR)
		return(OTHER_ERR("growing statistics table"));

	sp = stat_ptr(stat);

	if(!(sp->name = (char *) malloc(strlen(name) + 1)))
		ps_abort("Insufficient memory");
	strcpy(sp->name, name);
	sp->resid = 0.0;
	switch (sp->type = type) {

	case SAMPLE:
		sp->values.sam.count = 0;
		sp->values.sam.sum = 0.0;
		break;

	case VARIABLE:
		sp->values.var.start = sp->values.var.old_time = ps_now;
		sp->values.var.old_value = 0.0;
		sp->values.var.integral = 0.0; 
		break;

	case RATE:
		sp->values.rat.start = ps_now;
		sp->values.rat.count = 0;
		break;

	}
	return(stat);
}
	
/************************************************************************/

SYSCALL	para_open_stat(

/* Opens & initializes a statistic. 					*/

	const	char	*name,			/* statistic name	*/
	long	type				/* type of statistic	*/
)
{
	long stat;
	long new_stat = ps_open_stat(name, type);
	if((stat = get_table_entry(&ps_para_stat_tab)) == SYSERR)
		return(OTHER_ERR("growing para stats table"));
	*para_stat_ptr(stat) = new_stat;
	return new_stat;
}
	
/************************************************************************/

SYSCALL	ps_record_rate_stat(

/* Records a statistic sample or value.					*/

	long	stat				/* statistic index	*/
)
{
	ps_stat_t	*sp;			/* statistics pointer	*/

	if(stat < 0 || stat >= ps_stat_tab.used)
		return(BAD_PARAM("stat"));

	if ((sp = stat_ptr(stat))->type != RATE)
		return(BAD_CALL("Only works for RATE statistics"));

	sp->values.rat.count++;

	return(OK);
}


/************************************************************************/

SYSCALL	ps_add_stat(

/* Adds a number to a statistic sample 	*/
/* It is only used to add preemption time to the waiting time. Tao	*/

	long	stat,				/* statistic index	*/
	double	value				/* sample | value	*/
)
{
	ps_stat_t	*sp;			/* statistics pointer	*/
	double 	temp;				/* temporary		*/

	if(stat < 0 || stat >= ps_stat_tab.used)
		return(BAD_PARAM("stat"));

	switch((sp = stat_ptr(stat))->type) {
	
	case SAMPLE:	
		sp->resid += value;
		temp = sp->values.sam.sum + sp->resid;
		sp->resid += (sp->values.sam.sum - temp);
		sp->values.sam.sum = temp;
        break;

	default:
		
		return(BAD_CALL("Only works for SAMPLE statistics"));
	}

	return(OK);
}



/************************************************************************/

SYSCALL	ps_reset_stat(

/* Resets (zeroes) a specified statistic				*/

	long	stat				/* statistics index	*/
)
{
	ps_stat_t	*sp;			/* statistics pointer	*/

	if(stat < 0 || stat >= ps_stat_tab.used)
		return(BAD_PARAM("stat"));

	switch((sp = stat_ptr(stat))->type) {

	case SAMPLE:
		sp->values.sam.count = 0;
		sp->values.sam.sum = sp->resid = 0.0;
		break;

	case VARIABLE:
		sp->values.var.start = sp->values.var.old_time = ps_now;
		sp->values.var.integral = sp->resid = 0.0;
		break;

	case RATE:
		sp->values.rat.start = ps_now;
		sp->values.rat.count = 0;
		break;

	default:
		return(BAD_CALL("Invalid statistic type"));
	}
	return(OK);
}

/************************************************************************/

SYSCALL	ps_reset_all_stats(void)

/* Resets (zeroes) all statistics.					*/
{
	long	stat;				/* loop index		*/

	for(stat = 0; stat < ps_stat_tab.used; stat++) 
		ps_reset_stat(stat);

	return(OK);
}

/************************************************************************/

#if defined(WINNT) || defined(__CYGWIN__)
SYSCALL  ps_record_stat(
/* Records a statistic sample or value.					*/

	long	stat,				/* statistics index	*/
	double	value				/* sample | value	*/
)
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

#if defined(WINNT) || defined(__CYGWIN__)
SYSCALL	ps_record_stat2(

/* Records a statistic sample or value.					*/

	long	stat,				/* statistic index	*/
	double	value,				/* sample | value	*/
	double  start				/* Start time.		*/
)
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

long ps_get_node_stat_index(
    
/* Return the index to the internal node utilization statistic. */

	long node_id				/* Node id 		*/
)
{
	ps_node_t * np = node_ptr( node_id );
	if ( !np )  {
		return BAD_PARAM("node id");
	} else {
		return np->stat;
	}
}


/************************************************************************/

typedef int (*compar)(const void *, const void *);	/* Cast to get rid of compiler warning for qsort */

void	ps_stats(void)

/* Reports all statistics in tabular form.				*/

{
	double	delta;				/* time differential	*/
	long	stat;				/* statistic index	*/
	ps_stat_t	*sp;			/* statistic pointer	*/
	char	name[40];			/* statistic name	*/
	void *	copy;				/* working copy		*/
	long	bytes;				/* size of table	*/

/*	Make a sorted working copy so that we can prlong the stats in	*/
/*	sorted order but don't invalidate any id's.			*/

	bytes = ps_stat_tab.used * ps_stat_tab.entry_size;
	if (!(copy = malloc(bytes)))
		ps_abort("Insufficient Memory");
	memcpy (copy, (void*)ps_stat_tab.base, bytes);
	qsort (copy, ps_stat_tab.used, ps_stat_tab.entry_size,
	    (compar)stat_compare);

	printf("\n\nSimulation statistics for time = %G.\n", ps_now);
	printf("\n Name\t\t\t\t\tType\t  Mean\tObs(#|interval)\n\n");

	for(stat = 0; stat < ps_stat_tab.used; stat++) {
	  sp = (ps_stat_t*)((char *)copy + stat * ps_stat_tab.entry_size);
		padstr(name, sp->name, 38);
		printf("%s", name);

		switch(sp->type) {

		case SAMPLE:
			if(sp->values.sam.count)
				printf("\tSAMPLE\t%8G\t%ld\n", 
				    sp->values.sam.sum/sp->values.sam.count, 
				    sp->values.sam.count);
			else
				printf("\tSAMPLE\t%8G\t%d\n", 0.0, 0);
			break;
	
		case VARIABLE:
			if(ps_now != sp->values.var.old_time) {
				sp->values.var.integral += (ps_now -
				    sp->values.var.old_time) *
				    sp->values.var.old_value;
				sp->values.var.old_time = ps_now;
			}
			if((delta = ps_now - sp->values.var.start) > 0.0) 
				printf("\tVAR\t%8G\t%G\n", 
				   sp->values.var.integral/
				   delta, delta);
			else
				printf("\tVAR\t%8G\t%G\n", 0.0, 0.0);
			break;

		case RATE:
			if (ps_now != sp->values.rat.start)
				printf("\tRATE\t%8G\t%ld\n", 
				    (double)sp->values.rat.count/
				    (ps_now-sp->values.rat.start),
				    sp->values.rat.count);
			else
				printf("\tVAR\t%8G\t%G\n", 0.0, 0.0);
			break;

	
		default:
			warning("Bad stat entry");
		}
	}
	printf("\n\n");


/*	Cleanup our working copy					*/
	free (copy);
}
	


/************************************************************************/
/*		Miscellaneous SYSCALLS					*/
/************************************************************************/

void	ps_abort(

/* Writes PARASOL abort message to stderr and then aborts		*/

	const	char	*string			/* abort string		*/
)
{
	fprintf(stderr, "\n***> %s\n", string);
	exit(0);
}

/************************************************************************/

SYSCALL ps_build_bus(

/* Constructs a named bus connecting two or more specified nodes with a */
/* given transmission rate "trans_rate", and queueing discipline 	*/
/* "discipline".  Queueing may be PS_FIFO or random. Utilization statistics*/
/* are collected if "sf" is set.					*/

	const	char	*name,			/* bus name		*/
	long	ncount,				/* node count		*/
	long	*node_array,			/* array of nodes 	*/
	double	trans_rate,			/* transmission rate	*/
	long	discipline,			/* queueing discipline	*/
	long	sf				/* statistics flag	*/
)
{
	long	i;				/* loop index		*/
	long	*nap;				/* node array pointer	*/
	long	bus;				/* bus id index		*/
	ps_bus_t	*bp;			/* bus pointer		*/
	ps_node_t	*np;			/* node pointer		*/
	ps_comm_t	*cp;			/* comm pointer		*/
	char	stat_name[MAX_STRING_LEN];	/* statistics name	*/

	if(ncount < 2)
		return(BAD_PARAM("ncount"));
	for(i = 0, nap = node_array; i < ncount; i++, nap++)
		if(*nap >= ps_node_tab.used || *nap < 0)
			return(BAD_PARAM("node_array"));
	if(trans_rate <= 0.0)
		return(BAD_PARAM("trans_rate"));
	if((discipline != PS_FIFO) && (discipline != PS_RAND))
		return(BAD_PARAM("discipline"));
	if((bus = get_table_entry(&ps_bus_tab)) == SYSERR)
		return(OTHER_ERR("growing bus table"));

	bp = bus_ptr(bus);
	if(!(bp->name = (char *) malloc(strlen(name) + 1)))
		ps_abort("Insufficient memory");
	sprintf(bp->name, "%s", name);
	bp->state = BUS_IDLE;
	bp->nnodes = ncount;
	if(!(bp->bnode = (long *) malloc(sizeof(long) * ncount)))
		ps_abort("Insufficient memory");
	while(ncount--) {
		bp->bnode[ncount] = *node_array++;
		np = node_ptr(bp->bnode[ncount]);
		if(!(cp = (ps_comm_t *) malloc(sizeof(ps_comm_t))))
			ps_abort("Insufficient memory");
		cp->next = np->bus_list;
		np->bus_list = cp;
		cp->blid = bus;
	}
	bp->trate = trans_rate;
	bp->nqueued = 0;
	bp->discipline = discipline;
	bp->head = bp->tail = NULL_MESS_PTR;
	bp->ep = NULL_EVENT_PTR;
	if(sf) {
		snprintf(stat_name, MAX_STRING_LEN, "%s Utilization", name);
		bp->stat = ps_open_stat(stat_name, VARIABLE);
	}
	else
		bp->stat = NULL_STAT;

	return(bus);
}

/************************************************************************/

SYSCALL ps_build_link( 

/* Constructs a one-way link between "source" and "destination" nodes 	*/
/* with a specified transmission rate "trans_rate".  Queueing is 	*/
/* strictly PS_FIFO.  Utilization statistics are collected if "sf" is set.	*/

	const	char	*name,			/* link name		*/
	long	source,				/* source node id 	*/
	long	destination,			/* destination node id	*/
	double	trans_rate,			/* transmission rate	*/
	long	sf				/* statistics flag	*/
)
{
	long	link;				/* link id index	*/
	ps_link_t	*lp;			/* link pointer		*/
	ps_node_t	*np;			/* node pointer		*/
	ps_comm_t	*cp;			/* comm pointer		*/
	char	stat_name[MAX_STRING_LEN];	/* statistics name	*/

	if(source >= ps_node_tab.used || source < 0)
		return(BAD_PARAM("source"));
	if(destination >= ps_node_tab.used || destination < 0)
		return(BAD_PARAM("destination"));
	if(trans_rate <= 0.0)
		return(BAD_PARAM("trans_rate"));
	if((link = get_table_entry(&ps_link_tab)) == SYSERR)
		return(OTHER_ERR("growing link table"));
	if(source == destination)
		return(BAD_CALL("source == destination"));

	lp = link_ptr(link);
	if(!(lp->name = (char *) malloc(strlen(name) + 1)))
		ps_abort("Insufficient memory");
	sprintf(lp->name, "%s", name);
	lp->state = LINK_IDLE;
	lp->snode = source;
	np = node_ptr(source);
	if(!(cp = (ps_comm_t *) malloc(sizeof(ps_comm_t))))
		ps_abort("Insufficient memory");
	cp->next = np->sl_list;
	cp->blid = link;
	np->sl_list = cp;
	lp->dnode = destination;
	np = node_ptr(destination);
	if(!(cp = (ps_comm_t *) malloc(sizeof(ps_comm_t))))
		ps_abort("Insufficient memory");
	cp->next = np->rl_list;
	cp->blid = link;
	np->rl_list = cp;
	lp->trate = trans_rate;
	lp->head = lp->tail = NULL_MESS_PTR;
	lp->ep = NULL_EVENT_PTR;
	if(sf) {
		snprintf(stat_name, MAX_STRING_LEN, "%s Utilization", name);
		lp->stat = ps_open_stat(stat_name, VARIABLE);
	}
	else
		lp->stat = NULL_STAT;

	return(link);
}

/************************************************************************/

SYSCALL ps_build_node( 

/* Constructs a named node with "ncpu" processors with a speed factor	*/
/* "speed" which scales the durations of TASK_COMPUTING, TASK_SYNC and 	*/
/* TASK_BLOCKED states. The queueing discipline is specified by 	*/
/* "quantum" & "discipline" ( a non-zero quantum specifies the time 	*/
/* slice for Round Robin scheduling). "sl" denotes the level of utiliza-*/
/* tion statistics to gather.						*/

	const	char	*name,			/* node name		*/
	long	ncpu,				/* node size - # cpu's	*/
	double	speed,				/* cpu speed factor	*/
	double	quantum,			/* quantum size		*/
	long	discipline,			/* queueing discipline	*/
	long	sf 				/* statistics flag	*/
)
{
	long	i, j;				/* loop indices		*/
	long	node;				/* node id index	*/
	ps_node_t	*np;			/* node pointer		*/
	char	stat_name[MAX_STRING_LEN];	/* statistics name	*/

	if(ncpu < 1)
		return(BAD_PARAM("ncpu"));
	if(speed <= 0.)
		return(BAD_PARAM("speed"));
	if(quantum < 0.)
		return(BAD_PARAM("quantum"));
	if(discipline < 0 || (discipline > 2 && discipline!=5)) /*5 stand for cfs */
		return(BAD_PARAM("discipline"));
	if((node = get_table_entry(&ps_node_tab)) == SYSERR)
		return(OTHER_ERR("growing node table"));

	np = node_ptr(node);
	if(!(np->name = (char *) malloc(strlen(name) + 1)))
		ps_abort("Insufficient memory");
	sprintf(np->name, "%s", name);
	np->ncpu = np->nfree = ncpu;
	np->build_time = ps_now;
	np->sf = sf;
	if (sf & SF_PER_NODE) {
		snprintf(stat_name, MAX_STRING_LEN, "%s Utilization",
		    name);
		np->stat = ps_open_stat(stat_name, VARIABLE);
	}
	if (sf & SF_PER_TASK_NODE) {
		if (!(np->ts_tab =
		    (ps_table_t*)malloc(sizeof(ps_table_t))))
			ps_abort("Insufficient Memory");
		init_table(np->ts_tab, DEFAULT_MAX_TASKS,
		    sizeof(long));
		for (j = 0; j < DEFAULT_MAX_TASKS; j++)
			*ts_stat_ptr(np->ts_tab, j) = -1;
	}
	if(!(np->cpu = (ps_cpu_t *) malloc(sizeof(ps_cpu_t)*ncpu)))
		ps_abort("Insufficient memory");
	for(i = 0; i < np->ncpu; i++) {
		np->cpu[i].state = CPU_IDLE;
		np->cpu[i].run_task = NULL_TASK;
		np->cpu[i].ts_tab = NULL;
		np->cpu[i].catcher = NULL_TASK;
		np->cpu[i].scheduler = NULL_TASK;
		np->cpu[i].last_task = NULL_TASK;
		np->cpu[i].stat = NULL_STAT;
		np->cpu[i].port_n=0;
		np->cpu[i].group_rq=NULL_CFSRQ_PTR;  /* pointer of group rq */
		if (sf & SF_PER_HOST) {
			snprintf(stat_name, MAX_STRING_LEN, "%s (cpu %ld) Utilization",
			    name, i);
			np->cpu[i].stat = ps_open_stat(stat_name, VARIABLE);
		}
		if (sf & SF_PER_TASK_HOST) {
			if (!(np->cpu[i].ts_tab =
			    (ps_table_t*)malloc(sizeof(ps_table_t))))
				ps_abort("Insufficient Memory");
			init_table(np->cpu[i].ts_tab, DEFAULT_MAX_TASKS,
			    sizeof(long));
			for (j = 0; j < DEFAULT_MAX_TASKS; j++)
				*ts_stat_ptr(np->cpu[i].ts_tab, j) = -1;
		}
 	}

	np->speed = speed;
	np->rtrq = NULL_TASK;
	np->quantum = quantum;
	np->discipline = discipline;
	np->sl_list = NULL_COMM_PTR;
	np->rl_list = NULL_COMM_PTR;
	np->bus_list = NULL_COMM_PTR;
	np->ngroup=0;
	
	/* if the discipline is CFS, construct a cfs-rq for the node 	*/
	if (discipline == PS_CFS)
		ps_build_node_cfs(node);
	else 
		np->host_rq=NULL_CFSRQ_PTR;
	return(node);
}

/************************************************************************/

SYSCALL ps_build_node2(

/* Constructs a named node with "ncpu" processors with a speed factor	*/
/* "speed" which scales the durations of TASK_COMPUTING, TASK_SYNC and 	*/
/* TASK_BLOCKED states. The quantum duration is specified by quantum, 	*/
/* and the user defined scheduling task is specified by "scheduler". 	*/
/* "sl" denotes the level of utilization statistics to gather.		*/  

	const	char	*name, 
	long	ncpu, 
	double	speed, 
 	void 	(*scheduler)(void *),
	long	sf
)
{
	long		nid;			/* node id		*/
	ps_node_t	*np;			/* node ptr		*/
	char		string[TEMP_STR_SIZE];		/* task name buffer	*/
	long		i;			/* loop index		*/

	if ((nid = ps_build_node(name, ncpu, speed, 0.0, PS_PR, sf))
	    == SYSERR)
		return(SYSERR);

	np = node_ptr(nid);

 	for (i = 0; i < ncpu; i++) {
		sprintf(string, "%s Scheduler (cpu %ld)", name, i);
		np->cpu[i].scheduler = ps_create(string, nid, i, scheduler, 1);
		adjust_priority(np->cpu[i].scheduler, MAX_PRIORITY + 3);
		ps_resume(np->cpu[i].scheduler);

		sprintf (string, "%s Catcher (cpu %ld)", name, i);
		np->cpu[i].catcher = ps_create(string, nid, i, catcher, 1);
		adjust_priority(np->cpu[i].catcher, MAX_PRIORITY + 1);
		ps_resume(np->cpu[i].catcher);
	}
	return nid;
}
	
/************************************************************************/
SYSCALL ps_build_node_cfs( 

/* Constructs a cfs_rq for the node whose discipline is CFS.		*/

	long	node				/* node size - # cpu's	*/

)
{
	long	i;				/* loop indices		*/
	struct 	ps_node_t	* np;		/* node pointer		*/
	struct 	ps_cfs_rq_t 	* host_rq;
	
	np = node_ptr(node);
	
	if(!(host_rq = (ps_cfs_rq_t *) malloc(sizeof(ps_cfs_rq_t)*(np->ncpu))))
		ps_abort("Insufficient memory");
	np->host_rq=host_rq;

	for(i=0;i<np->ncpu;i++){
		init_cfs_rq(host_rq+i,node, np->build_time,NULL_SCHED_PTR);
		(host_rq+i)->load=0;
	}

	return(OK);

}
/************************************************************************/
/* WCS - sep, 2008 - Added , build a group */ 
SYSCALL ps_build_group( 

	const	char	*name,			/* group name		*/
	double	group_share,			/* group share		*/
	long	node,				/* node id		*/
	long 	cap				/* cap flag		*/

)
{
	long	i, j;				/* loop indices		*/
	long 	ngroup;				/* number of group	*/
	long 	group;				/* index of group	*/
	struct 	ps_node_t	* np;		/* node pointer		*/
	struct 	ps_cfs_rq_t 	* group_rq ;
	struct 	sched_info 	* group_si;	/* pointer of si	*/
	struct	ps_group_t 	* gp;		/* group pointer	*/
	struct  ps_cpu_t	* hp;
	char	stat_name[MAX_STRING_LEN];	/* statistics name	*/

	if( node >= ps_node_tab.used || node < 0 )
		return(BAD_PARAM("node"));	
	if(group_share < 0.)
		return(BAD_PARAM("group share"));
	if((group = get_table_entry(&ps_group_tab)) == SYSERR)
		return(OTHER_ERR("growing group table"));
	gp = group_ptr(group);
	np = node_ptr(node);
	ngroup=np->ngroup;

	/* initial the group */
	if(!(gp->name = (char *) malloc(strlen(name) + 1)))
		ps_abort("Insufficient memory");
	sprintf(gp->name, "%s", name);

	gp->share=group_share;
	gp->node=node;
	gp->cap=cap;
	gp->ntask=0;	
	gp->group_id2=ngroup;
	if (np->discipline == PS_CFS){

		/* construct the group cfs-rq for each host */
		for(i=0;i<np->ncpu;i++){
			hp=&(np->cpu[i]);
			if(!(group_rq = (ps_cfs_rq_t *) malloc(sizeof(ps_cfs_rq_t)*(ngroup+1))))
				ps_abort("Insufficient memory");
			if(!(group_si = (sched_info *) malloc(sizeof(sched_info))))
				ps_abort("Insufficient memory");

			if (ngroup){
				for(j=0;j<ngroup;j++){
					assign_cfs_rq(&group_rq[j],&hp->group_rq[j]);
					group_rq[j].si->own_rq=&group_rq[j];
				}
				for(j=ngroup-1;j>=0;j--){
					/*free((hp->group_rq+j));  ???? how to free */
				}
				hp->group_rq=NULL_CFSRQ_PTR;
			}
			hp->group_rq=group_rq;
			init_cfs_rq(group_rq+ngroup,node, ps_now,group_si);
			init_sched_info(group_si, ngroup,group_rq+ngroup,np->host_rq+i, group_share);
		}
	}
	(np->ngroup)++;
	if ( SF_PER_NODE) {
		snprintf(stat_name, MAX_STRING_LEN,"%s Utilization",   name);
		gp->stat = ps_open_stat(stat_name, VARIABLE);
	}
	if (SF_PER_TASK_NODE) {
		if (!(gp->ts_tab = (ps_table_t*)malloc(sizeof(ps_table_t))))
			ps_abort("Insufficient Memory");
		init_table(gp->ts_tab, DEFAULT_MAX_TASKS, sizeof(long));
		for (j = 0; j < DEFAULT_MAX_TASKS; j++)
			*ts_stat_ptr(gp->ts_tab, j) = -1;
	}
	return(group);
}

/************************************************************************/
/* WCS - 7 June 1999 - Added */ 
/* WCS - 13 July 1999 - Fixed for idle cpu */

SYSCALL	ps_curr_priority(

/* Returns the current priority of the cpu on the host.		*/

	long	node,				/* node id index	*/
	long	host				/* host id index	*/
)
{
	ps_node_t	*np;			/* node pointer		*/
	long		run_task;

	if(node < 0 || node >= ps_node_tab.used)
		return(BAD_PARAM("node"));
	np = node_ptr(node);
	if((host < 0) || (host >= np->ncpu))
		return(BAD_PARAM("host"));

	if ((run_task = np->cpu[host].run_task) != NULL_TASK) {
		return (ps_task_ptr(run_task)->priority);
	}
	else
		return (MIN_PRIORITY - 1);

}
	
/************************************************************************/

SYSCALL	ps_headroom(void)

/* Checks the headroom remaining on the stack of the caller.		*/

{
#if	!HAVE_SIGALTSTACK || _WIN32 || _WIN64
	long	*context;			/* task context		*/

	context = (long *) &sp_jb[0];
#if	_WIN32 || _WIN64
	setjmp(sp_jb[0]);
#else
	_setjmp(sp_jb[0]);
#endif
	if(sp_dir > 0)
		return(((size_t)ps_htp->stack_limit) - context[sp_ind]);
	else
		return(context[sp_ind] - (size_t)ps_htp->stack_base);
#else
	return 0;
#endif
}

/************************************************************************/

double	ps_erlang(

/* Generates a variate from an Erlang distribution			*/

	double	mean,				/* distribution mean	*/
	long	kernel				/* erlang kernel	*/
)
{
	double	prod;				/* random product	*/
	long	i;				/* loop index		*/

	prod = 1.0;
	for(i = 0; i < kernel; i++ ) 
		prod *= drand48();
	return(-mean*log(prod)/kernel);
}

/************************************************************************/

SYSCALL	ps_run_parasol(

/* Runs PARASOL using supplied parameters and flags.			*/

	double	duration,			/* simulation duration	*/
	long	seed,				/* random number seed	*/
	long	flags				/* run-time flags	*/
)
{
	long	rid;				/* reaper task id	*/

	if(duration <= 0.0)
		return(BAD_PARAM("duration"));
	if(seed < 0)
		return(BAD_PARAM("seed"));

 	

#if !HAVE_SIGALTSTACK || _WIN32 || _WIN64
/*	Test stack for loading direction and jmp_buf index		*/
	if(sp_tester(&sp_dir, &sp_ind, &sp_delta) == SYSERR)
		sp_dss = sp_delta * 20 * sizeof(double);
	else
		sp_dss = get_dss (flags & RPF_TRACE);
#endif

/* 	Initialize PARASOL times, flags and tables			*/

	ps_now = 0.0;

	ps_run_time = duration;
	step_flag = (flags & RPF_STEP) ? TRUE : FALSE;
	break_flag = FALSE;
	ts_flag = (flags & RPF_TRACE) ? TRUE : FALSE;
	w_flag = (flags & RPF_WARNING) ? TRUE : FALSE;
	bs_time = -1.0;
	init_table(&ps_node_tab, DEFAULT_MAX_NODES, sizeof(ps_node_t));
	init_table(&ps_group_tab, DEFAULT_MAX_GROUPS, sizeof(ps_group_t));
	init_table(&ps_bus_tab, DEFAULT_MAX_BUSES, sizeof(ps_bus_t));
	init_table(&ps_link_tab, DEFAULT_MAX_LINKS, sizeof(ps_link_t));
	init_event();
	init_locks();
	init_semaphores();
	init_table(&ps_port_tab, DEFAULT_MAX_PORTS, sizeof(ps_port_t));
	init_table(&ps_task_tab, DEFAULT_MAX_TASKS, sizeof(ps_task_t));
	init_table(&ps_sched_info_tab, DEFAULT_MAX_TASKS, sizeof(sched_info));
	init_table(&ps_stat_tab, DEFAULT_MAX_STATS, sizeof(ps_stat_t));
	init_table(&ps_para_stat_tab, DEFAULT_MAX_STATS, sizeof(long));
	init_table(&ps_pool_tab, DEFAULT_MAX_POOLS, sizeof(ps_buf_t));
	init_table(&ps_dye_tab, DEFAULT_MAX_DYES, sizeof(ps_dye_t));


/*	Seed the random number generator 				*/

	srand48(seed);

/*	Build node 0 & launch reaper (& genesis)			*/

	ps_htp = DRIVER_PTR;
	ps_build_node("PARASOL Node", 1, 1.0, 0.0, PS_PR, FALSE);
	rid = ps_create("Reaper", 0, 0, reaper, MAX_PRIORITY);
	reaper_port = ps_std_port(rid);
	ps_resume(rid);


/*	Drive the simulation to completion				*/

	driver();
	first_run = FALSE;

	/* NOTREACHED */
	return 0;
}		

/************************************************************************/

SYSCALL	ps_schedule(

/* For use by user defined scheduling functions only! Schedules the	*/
/* specified task to run on the specified host (immediately).		*/

	long	task,
	long	host
)
{
	ps_node_t	*np;
	ps_task_t	*tp;
	ps_cpu_t	*hp;

	if (ps_htp->host == ANY_HOST 
	    || ps_myself != (np = 
	    node_ptr(ps_my_node))->cpu[ps_htp->host].scheduler)
		return(BAD_CALL("Caller is not a scheduler task"));
	if (host < 0 || host > np->ncpu)
		return(BAD_PARAM("host"));
	if (task != NULL_TASK 
	    && (task < 0 || task > ps_task_tab.tab_size
	    || ((tp = ps_task_ptr(task))->node != ps_my_node)
	    || (tp->uhost != ANY_HOST && tp->uhost != host)))
		return(BAD_PARAM("task"));

	hp = &np->cpu[host];

 	if (hp->last_task != NULL_TASK) 
		adjust_priority(hp->last_task, 
		    ps_task_ptr(hp->last_task)->upriority);
	
	if (task == NULL_TASK)
		ps_suspend(hp->catcher);
	else {
 		ps_task_ptr(task)->host = host;
		adjust_priority(task, MAX_PRIORITY + 2);
		ps_resume(hp->catcher);
	}
	hp->last_task = task;
	return OK;
}


/************************************************************************/
/*		Angio Tracing Syscalls  				*/
/************************************************************************/

SYSCALL ps_enable_angio_tracing (

/* Enables angio tracing, and opens the specified angio trace file.	*/
/* This function should only be called once, BEFORE any calls to	*/
/* ps_create or ps_create2.						*/

	const	char	*trace_filename		/* Trace file name	*/
)
{
	long		i;
	long		j;
	ps_task_t	*tp;

	if (angio_flag || task_count > 2 || ps_now > 0.0)
		return(BAD_CALL("Must be called before any other functions"));

 	if (trace_filename == NULL || strlen (trace_filename) == 0)
		angio_file = stdout;
	else if ((angio_file = fopen(trace_filename,"w")) == NULL)
		ps_abort ("Can't open angio trace file");
	angio_flag = TRUE;

	ps_toggle_angio_output(TRUE);

	/* Log two late creation events.				*/

	for (i = 0; i < 2; i++) {
		tp = ps_task_ptr(i);
		for (j = 0; tp->name[j]; j++)
			if (isspace(tp->name[j]))
				tp->name[j] = '_';
		log_angio_event(tp, dye_ptr(tp->did), "wBegin");
	}

	return OK;
}

/************************************************************************/

SYSCALL ps_inject_trace_name (

/* Injects a new dye into the calling task.				*/

	const	char	*name			/* base trace name	*/
)
{
	long	i;
	long 	did;				/* New dye id		*/

	if (!angio_flag)
		return(BAD_CALL("Angio tracing not enabled"));
	if(strlen(name) == 0)
		return(BAD_PARAM("name"));
	for (i = 0; name[i]; i++)
		if (isspace(name[i]))
			return(BAD_PARAM("name"));

	
	end_trace (ps_htp);			/* End old trace	*/
	did = create_dye (name);		/* Create new dye	*/
	start_trace (ps_htp, did);		/* Start new trace	*/

	return OK;
}

/************************************************************************/

SYSCALL ps_task_cycle_begin (void)

/* Logs a wCycle event.							*/

{
	return ps_log_user_event("wCycle");
}

/************************************************************************/

SYSCALL ps_toggle_angio_output(

/* Allows the user to turn logging of angio trace events on or off	*/

	long	value				/* flag value		*/
)
{
	if (!angio_flag)
		return(BAD_CALL("Angio tracing not enabled"));
	angio_output = value ? TRUE : FALSE;
	return OK;
}

/************************************************************************/

SYSCALL ps_log_user_event (

/* Logs an event specified by the user, using the calling task's dye.	*/

	const	char	*event			/* event text		*/
)
{
	if (!angio_flag)
		return(BAD_CALL("Angio tracing not enabled"));

	log_angio_event (ps_htp, dye_ptr(ps_htp->did), event);
	return OK;
}	

/************************************************************************/	
/*		Glocal variable related SYSCALLS			*/
/************************************************************************/

SYSCALL ps_allocate_glocal (

/*	Allocates a glocal variable width bytes in size in the		*/
/*	environment with identifier env.  All subscribers to that	*/
/*	environment will get their own copy of the variable.		*/

	long 	env,				/* environment id	*/
	long	width				/* data size in bytes	*/
)
{
	long		gloc;
	ps_env_t	*ep;
	ps_var_t	*vp;

	if(env < 0)
		return(BAD_PARAM("env"));
	if(width <= 0)
		return(BAD_PARAM("width"));

	glocal_init_stuff (env);

	ep = env_ptr(env);
	gloc = get_table_entry (&ep->var_tab);
	vp = var_ptr (ep, gloc);
	vp->width = width;
	vp->used = TRUE;
	if (ep->nallocated)
	if (ep->nallocated != 0) {
		if((vp->data = (void*)malloc (width*ep->nallocated)) == NULL)
			ps_abort("Insufficient Memory");
		memset(vp->data, '\0', width*ep->nallocated);
	}
	return gloc;
}

/************************************************************************/

SYSCALL	ps_share_glocal (

/*	Allows the caller to share a copy of all the glocal variables  	*/
/*	in the environment with identifier env with the given task	*/

	long 		env,			/* environment id	*/
	long		task			/* task 		*/
)
{
	ps_env_t	*ep;
	long		i;
 	long		*temp;

	if(env < 0)
		return(BAD_PARAM("env"));
	if(task < 0)
		return(BAD_PARAM("task"));
	glocal_init_stuff (env);
	if (task >= (ep = env_ptr(env))->ntasks)
		return(BAD_PARAM("task"));
	if (ep->indices[task] == -1)
		return(BAD_CALL("Task is not subscribed to environment"));

	if (ps_myself >= ep->ntasks) {
		if ((temp = (long*)malloc (sizeof(long)*(ps_myself+TASK_DELTA))) 
		    == NULL)
			ps_abort ("Memory allocation error");
		if (ep->ntasks) {
			memcpy (temp, ep->indices, sizeof(long)*ep->ntasks);
			free (ep->indices);
		}
		ep->indices = temp;
		for (i = ep->ntasks; i < ps_myself + TASK_DELTA; i++)
			ep->indices[i] = -1;
		ep->ntasks = ps_myself + TASK_DELTA;
	}
	if (ep->indices[ps_myself] != -1 
	    && ep->indices[ps_myself] != ep->indices[task] ) 
		return(BAD_CALL("Caller is subscribed to the environment"));

	ep->indices[ps_myself] = ep->indices[task];

	return OK;
}

	

/************************************************************************/

SYSCALL	ps_subscribe_glocal (

/*	Gives the caller his own copy of the variable with identifier 	*/
/*	gloc in the environment with identifier env.			*/

	long 		env			/* environment id	*/
)
{
	ps_env_t	*ep;
	ps_var_t	*vp;
	long		i;
	void		*temp;
	long		*temp2;
	long		bytes;

	if (env < 0)
		return(BAD_PARAM("env"));

	glocal_init_stuff (env);

	ep = env_ptr(env);
	if (ps_myself >= ep->ntasks) {
		if ((temp2 = (long*)malloc (sizeof(long)*(ps_myself+TASK_DELTA))) 
		    == NULL)
			ps_abort ("Memory allocation error");
		if (ep->ntasks) {
			memcpy (temp2, ep->indices, sizeof(long)*ep->ntasks);
			free (ep->indices);
		}
		ep->indices = temp2;
		for (i = ep->ntasks; i < ps_myself + TASK_DELTA; i++)
			ep->indices[i] = -1;
		ep->ntasks = ps_myself + TASK_DELTA;
	}
	if (ep->indices[ps_myself] == -1) {
		ep->indices[ps_myself] = ep->nsubscribers++;
		if (ep->nsubscribers > ep->nallocated) {
			for (i = 0; i < ep->var_tab.tab_size; i++) {
				if ((vp = var_ptr(ep, i))->used) {
					bytes = vp->width*(ep->nallocated+SUBSCRIBER_DELTA);
					if (!(temp = (void*)malloc(bytes)))
						ps_abort("Memory allocation error"); 
					memset(temp, '\0', bytes);
					if (ep->nallocated) {
						memcpy (temp, vp->data, vp->width*ep->nallocated);
						free (vp->data);
					}
					vp->data = temp;
				}
			}
			ep->nallocated += SUBSCRIBER_DELTA;
		}
	}
	return OK;	
}		

/************************************************************************/

void *ps_glocal_value (

/*	Returns a pointer to the calling task's copy of the glocal	*/
/*	variable with identifier gloc in the environment env. You may	*/
/*	want to consider implementing this as a macro once everything	*/
/*	is debugged, for obvious performance reasons.			*/

	long		env,			/* environment id	*/	
	long		gloc			/* variable id		*/
)
{
	ps_env_t	*ep;
	ps_var_t	*vp;

	if (env < 0 || gloc < 0 || env >= ps_env_tab.tab_size)
		ps_abort("Bad glocal variable reference 1");
	if (!(ep = env_ptr(env))->used || gloc >= ep->var_tab.tab_size)
		ps_abort("Bad glocal variable reference 2");
	if (!(vp = var_ptr(ep, gloc))->used)
		ps_abort("Bad glocal variable reference 3");
	if (ps_myself >= ep->ntasks || ep->indices[ps_myself] < 0)
		ps_abort("Bad glocal variable reference 4");

	return &(((char*)vp->data)[ep->indices[ps_myself]*vp->width]);

}

/************************************************************************/
/*		Special PARASOL Tasks   				*/
/************************************************************************/

LOCAL	long	nb;				/* number of blocks	*/
LOCAL	double	*sum;				/* sum array		*/
LOCAL	double	*sumsq;				/* sum of squares array	*/

double	blocked_stat_outputer(long stat)

/* Output specified statistic.	*/

{
	char	name[40];			/* statistic name	*/
	double	*ps;				/* sum pointer		*/
	double	*pss;				/* sum of squares prt	*/
	ps_stat_t	*sp;			/* statistics pointer	*/
	double	mean;				/* statistic mean	*/
	double	stddev;				/* statistic std dev	*/
	long	t_index;			/* index for t_tab	*/
	double	t1, t2;				/* Student t values	*/

	t_index = nb-2;				/* find t values	*/
	if(t_index < 30) {
		t1 = t_tab[0][t_index];
		t2 = t_tab[1][t_index];
	}
	else if(t_index < 40) {
		t1 = t_tab[0][29] + (t_tab[0][30] - t_tab[0][29])
			    *(t_index - 29)/10.0;
		t2 = t_tab[1][29] + (t_tab[1][30] - t_tab[1][29])
			    *(t_index - 29)/10.0;
	}
	else if(t_index < 60) {
		t1 = t_tab[0][30] + (t_tab[0][31] - t_tab[0][30])
			    *(t_index - 39)/20.0;
		t2 = t_tab[1][30] + (t_tab[1][31] - t_tab[1][30])
			    *(t_index - 39)/20.0;
	}
	else if(t_index < 120) {
		t1 = t_tab[0][31] + (t_tab[0][32] - t_tab[0][31])
			*(t_index - 59)/60.0;
		t2 = t_tab[1][31] + (t_tab[1][32] - t_tab[1][31])
			    *(t_index - 59)/60.0;
	}
	else {
		t1 = t_tab[0][33];
		t2 = t_tab[1][33];
	}

	sp = stat_ptr(stat);
	ps = sum + stat; 
	pss = sumsq + stat; 
	padstr(name, sp->name, 38);
	printf("%s", name);
	mean = *ps/nb;
	stddev = sqrt(fabs((*pss - (*ps)*(*ps)/nb)/(nb*(nb-1))));
	printf("\t%8G\t%8G\t%8G\n", mean, t1*stddev, t2*stddev);
	return mean;
}

void	para_own_block_stats_outputer()

/* Output of parasol controlled block statistics.	*/

{
	long	para_stat;				/* statistics index	*/

	for(para_stat = 0; para_stat < ps_para_stat_tab.used; para_stat++) {
		blocked_stat_outputer(*para_stat_ptr(para_stat));
	}
}

void	para_block_stats_outputer()

/* Output of block statistics.	*/

{
	long	stat;				/* statistics index	*/

	printf("\n\nBlocked simulation statistics for time = %G.\n", 
	    ps_run_time);
	printf("\n Name\t\t\t\t\t  Mean\t\t95%% Interval\t99%% Interval\n\n");

#ifdef UNDEF
	long	copy;				/* working copy		*/
	long	bytes;				/* size of table	*/

/*	Make a sorted working copy so that we can prlong the stats in	*/
/*	sorted order but don't invalidate any id's.			*/

	bytes = ps_stat_tab.used * ps_stat_tab.entry_size;
	if (!(copy = (long)malloc(bytes)))
		ps_abort("Insufficient Memory");
	memcpy ((void*)copy, (void*)ps_stat_tab.base, bytes);
	qsort ((void*)copy, ps_stat_tab.used, ps_stat_tab.entry_size,
	    stat_compare);
#endif /* UNDEF */

	for(stat = 0; stat < ps_stat_tab.used; stat++) {
		blocked_stat_outputer(stat);
	}
	printf("\n\n");
}

void	block_stats_outputer()

/* Output of block statistics.	*/

{
	para_block_stats_outputer();
}

LOCAL	void	block_stats_collector(void * arg)

/* Code for the task that handles collection of block statistics.	*/

{
	long	nb;				/* number of blocks	*/
	double	delay;				/* transient delay	*/
	long	stat;				/* statistics index	*/
	double	period;				/* block duration	*/
	double	*ps;				/* sum pointer		*/
	double	*pss;				/* sum of squares prt	*/
	long	i;				/* loop index		*/
	double	value;				/* mean observation	*/
	double	junk;				/* junk argument	*/
	char	*text;				/* temp variable	*/
	long	ack_port;			/* temp variable	*/

	ps_receive(ps_my_std_port, NEVER, &nb, &delay, &text, &ack_port);
	bs_time = ps_now + delay;

/* WCS - 12 June 1999 - ps_sync on node 0 can cause no stats report */
/*	period = (ps_run_time++ - ps_now - delay)/nb;                  */
	period = (ps_run_time - ps_now - delay)/nb;
	ps_run_time*=2;

	ps_sleep(delay);
	ps_reset_all_stats();

 	for(i = 0; i < nb; i++) {		/* collect block stats	*/
		ps_sleep(period);
		if (i == 0) {
 			if (!(sum = (double*)malloc(ps_stat_tab.used*sizeof(double))))
				ps_abort("Insufficient Memory");
			memset (sum, '\0', ps_stat_tab.used*sizeof(double));
 			if (!(sumsq = (double*)malloc(ps_stat_tab.used*sizeof(double))))
				ps_abort("Insufficient Memory");
			memset (sumsq, '\0', ps_stat_tab.used*sizeof(double));
		}
		ps = sum;
		pss = sumsq;
		for (stat = 0; stat < ps_stat_tab.used; stat++) {
			ps_get_stat(stat, &value, &junk);
			ps_reset_stat(stat);
			*ps++ += value;
			*pss++ += value*value;
		}
	}
	
/* WCS - 12 June 1999 - ps_sync on node 0 can cause no stats report */
	ps_run_time /= 2;

	block_stats_outputer();

	ps_run_time -= 1.1;  /* where does 1.1 come from ? (WCS) */
	
	ps_sleep(1.0);
}

LOCAL	void	catcher(void * arg)

/* The catcher used for user defined scheduling, all it does is send	*/
/* messages continuously to the user defined scheduler.			*/

{
	long	scheduler;			/* scheduler id		*/

	scheduler = node_ptr(ps_my_node)->cpu[ps_my_host].scheduler;
	while(TRUE)
  		ps_send(ps_std_port(scheduler), SN_IDLE, "", ps_my_host);
}

/************************************************************************/

LOCAL  	void	port_set_surrogate(void * arg)

/* Surrogate task used in port sets to provide concurrent access to 	*/
/* ports in set.							*/

{
	long 	r_port;				/* receive port		*/
	long	type;				/* message type		*/
	double	ts;				/* message timestamp	*/
	char	*tp;				/* message text pointer	*/
	long	rp;				/* message reply port	*/
	long	f_port;				/* forward port		*/
	long     op;				/* original port	*/
	long	mid;				/* unique message id	*/
	long	did;				/* dye id		*/

	ps_receive(ps_my_std_port, NEVER, &r_port, &ts, &tp, &f_port);
	while(TRUE) { 
		if(port_receive(PS_FIFO,r_port, NEVER, &type, &ts, &tp, &rp, &op, 
		    &mid, &did) == SYSERR)
			ps_kill(ps_myself);
		port_send(f_port, type, ts, tp, rp, r_port, mid, did);
	}
}

/************************************************************************/

LOCAL	void	reaper(void * arg)

/* Grim reaper task assists suicides by accepting kill requests.  It is */
/* is the ancestor of all user tasks including genesis which it launches*/
/* before handling requests.						*/

{
	long	type;				/* message type		*/
	long	task;				/* task id		*/
	double	ts;				/* message timestamp	*/
	char	*tp;				/* message text pointer	*/
	char	string[TEMP_STR_SIZE];			/* abort string		*/

	/* WCS - 3 Sept 1997 - Made Genesis stack larger */
	ps_resume(ps_create2("Genesis", 0, 0, ps_genesis, MAX_PRIORITY, 0,4.0));
	while(TRUE) {
		ps_receive(ps_my_std_port, NEVER, &type, &ts, &tp, &task);
		if(type == SUICIDE) {
			if(ps_kill(task) == SYSERR) {
				sprintf(string, "Reaper fails to kill %ld",
				    task);
				warning(string);
			}
		}
		
	}
}

/************************************************************************/

LOCAL 	void 	shared_port_dispatcher(void * arg)

/* Handles shared port messages of three types: requests by users for 	*/
/* messages, request cancelation, and incoming messages directed at the */
/* shared port.								*/

{
	long	queue_port;			/* queue port		*/
	long	type, t1;			/* message type		*/
	double	ts, t2;				/* message timestamp	*/
	char	*tp, *t3;			/* message text pointer	*/
	long	rp, t4;				/* message reply port	*/
	long	state;				/* dispatcher state	*/
	long     op;				/* original port	*/
	long	mid;				/* message id		*/
	long	did;				/* dye id		*/
	
	if((queue_port = ps_allocate_port("Message Queue", ps_myself)) ==
	    SYSERR)
		ps_kill(ps_myself);
	state = 0;
	while(TRUE) {
		port_receive(PS_FIFO, ps_my_std_port, NEVER, &type, &ts, &tp, &rp,
		    &op, &mid, &did);

		switch(type) {

		case SP_REQUEST:
			if(++state > 0) {
				ps_send(queue_port, 0, "", rp);
			}
			else {
				port_receive(PS_FIFO, queue_port, NEVER, &t1, &t2,
				    &t3, &t4, &op, &mid, &did);
				if(port_send(rp, t1, t2, t3, t4, op, mid, did) 
				    == SYSERR) { 
					port_send(queue_port, t1, t2, t3, t4,
					    op, mid, did);
					--state;
				}
			}
			break;

		case SP_CANCEL:
			while(TRUE) {
				ps_receive(queue_port, NEVER, &t1, &t2, &t3, 
				    &t4);
				if(t4 == rp)
					break;
				else
					ps_send(queue_port, 0, "", t4);
			}
			--state;
			break;
			
 		default:
			if(--state < 0) 
				port_send(queue_port, type, ts, tp, rp, op,
				    mid, did);
			else {
				while(state >= 0) {
					ps_receive(queue_port, NEVER, &t1, &t2,
					    &t3, &t4);
					if(port_send(t4, type, ts, tp, rp, op,
					    mid, did) == OK)
						break;
					else 
						--state;
				}						 
			}
			break;
		}
	}
}
	

/************************************************************************/

LOCAL void driver(void)

/* Handles events and advances the global time "now" when appropriate.	*/
/* The debugger is invoked or the simulation is aborted or terminated 	*/
/* as appropriate.							*/ 

{
	ps_event_t	*ep;			/* event pointer	*/
	ps_event_t	*next_event();		/* next event function	*/
	void	(*handler_tab[14])(ps_event_t *ep) = {
			end_sync_handler,
			end_compute_handler, 
			end_quantum_handler,
			end_trans_handler,
			end_sleep_handler,
			end_receive_handler, 
			end_block_handler,
			link_failure_handler,
			link_repair_handler,
			bus_failure_handler,
			bus_repair_handler,
			node_failure_handler,
			node_repair_handler,
			user_event_handler
	};
		
	while(TRUE) {
		if(step_flag)
			debug();
		if(break_flag && (ps_now >= break_time)) {
			break_flag = FALSE;
			step_flag = TRUE;
			debug();
		}
		if((ep = next_event()) == NULL_EVENT_PTR) {
#ifdef STACK_TESTING
		  test_all_stacks();
#endif /* STACK_TESTING */
		  ps_abort("Empty calendar");
		}
		if(ep->time < ps_now)
			ps_abort("Attempt to move time backwards");
		if(ep->time >= ps_run_time) {
			ps_now = ps_run_time;
			return;
		}
		if(ep->time > ps_now) 
			ps_now = ep->time;
		(*handler_tab[ep->type])(ep);
	}
}
 
/************************************************************************/

LOCAL	void	end_block_handler(

/* Handles END_BLOCK events.						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	ps_task_t	*tp;			/* task pointer		*/

	if((tp = ps_task_ptr((size_t)ep->gp))->state != TASK_BLOCKED)
		ps_abort("Bad end block event");
	tp->tep = NULL_EVENT_PTR;
  	ps_htp = tp;

	/* if for CFS task,update block task  */
	if((node_ptr(ps_htp->node))->discipline == PS_CFS){
		update_run_task(ps_htp);
		/*cooling task...; */
	}

	SET_TASK_STATE(ps_htp, TASK_HOT);
	ctxsw(&d_context, &ps_htp->context);
}

/************************************************************************/
 
LOCAL	void	end_compute_handler(

/* Handles END_COMPUTE events						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	ps_task_t	*ctp;			/* current task pointer	*/

	if((ctp = ps_task_ptr((size_t)ep->gp))->state != TASK_COMPUTING)
		ps_abort("Bad computing timeout event");
	ctp->tep = NULL_EVENT_PTR;
	ps_htp = ctp;

	/* if for CFS task,update the computing task  */
	if((node_ptr(ps_htp->node))->discipline == PS_CFS){
		update_run_task(ps_htp);
	}

	SET_TASK_STATE(ps_htp, TASK_HOT);

	ps_htp->end_compute_time = ps_now;	/* tomari quorum */

	ctxsw(&d_context, &ctp->context);
}

/************************************************************************/

LOCAL 	void	end_quantum_handler(

/* Handles END_QUANTUM events.						*/
/* For cfs scheduler, check fair will be called to make sure whether    */
/* this task has fairness to execute in next period.if it can, generate */
/* another end quantum event. if not, find another task.            	*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	ps_task_t	*tp;			/* task pointer		*/
	ps_node_t	*np;			/* node pointer		*/
	ps_cpu_t	*hp;			/* host cpu pointer	*/

	hp = (tp = ps_task_ptr((size_t)ep->gp))->hp;
	np = node_ptr(tp->node);
	tp->qep = NULL_EVENT_PTR;
	switch(tp->state) {
 
	case TASK_SPINNING:
	case TASK_COMPUTING:
		if(np->discipline == PS_FIFO) 
			find_priority(np, hp, MIN_PRIORITY - 1);
		else{
			qxflag = TRUE;
			find_priority(np, hp, tp->priority - 1);
		}
		break;
	
	case TASK_SYNC:
	case TASK_SYNC_SUSPEND:
	case TASK_SYNC_FREE:
		tp->qx = TRUE;
		break;

	default:
		ps_abort("Bad end of quantum event");
	}
}

/************************************************************************/

LOCAL	void	end_receive_handler(

/* Handles END_RECEIVE (receive t/o) events.				*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	ps_task_t	*tp;			/* task pointer		*/

	if((tp = ps_task_ptr((size_t)ep->gp))->state != TASK_RECEIVING)
		ps_abort("Bad receive timeout event");

	tp->rtoep = NULL_EVENT_PTR;
	if(ps_send(tp->wport, ACK_TIMEOUT, "", NULL_PORT) != OK)
		ps_abort("Unable to send receive timeout message");
}

/************************************************************************/

LOCAL	void	end_sleep_handler(

/* Handles END_SLEEP events.						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	ps_task_t	*tp;			/* task pointer		*/
	ps_node_t 	*np;			/* node pointer		*/

	if((tp = ps_task_ptr((size_t)ep->gp))->state != TASK_SLEEPING)  
		ps_abort("Bad end sleep event");
	tp->tep = NULL_EVENT_PTR;
	np=node_ptr(tp->node);

	/* if for CFS task,update fair value of the sleep task */
	if(np->discipline == PS_CFS)
		update_sleep_task(tp);

	find_host(tp);
}

/************************************************************************/

LOCAL	void	end_sync_handler(

/* Handles END_SYNC events						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	ps_task_t	*tp;			/* task pointer		*/
	ps_cpu_t	*hp;			/* host cpu pointer	*/
	ps_node_t	*np;			/* node pointer		*/

	hp = (tp = ps_task_ptr((size_t)(ep->gp)))->hp;
	np = node_ptr(tp->node);
	tp->tep = NULL_EVENT_PTR;
/* update run task and cooling hot task */
	if(np->discipline == PS_CFS){
		update_run_task(tp);
		/*cooling task...; */
	}

	switch(tp->state) {

	case TASK_SYNC_SUSPEND:
		SET_TASK_STATE(tp, TASK_SUSPENDED);
		if(ts_flag)
			ts_report(tp, "suspended");
		tp->hp = NULL_HOST_PTR;
		remove_event(tp->qep);
		tp->qep = NULL_EVENT_PTR;
		release_locks(tp);
		find_ready(np, hp);
		break;

	case TASK_SYNC_FREE:
		SET_TASK_STATE(tp, TASK_COMPUTING);
		ps_kill(tid(tp));
		break;

	case TASK_SYNC:
		if(tp->qx) {
			tp->qx = FALSE;
			qxflag = TRUE;
			find_priority(np, hp, tp->priority - 1);
		}
		else if(np->discipline == PS_PR) {
			find_priority(np, hp, tp->priority);
		}
		else {
			ps_htp = tp;
			SET_TASK_STATE(ps_htp, TASK_HOT);
			ctxsw(&d_context, &ps_htp->context);
		}
		break;

	default:
		ps_abort("Bad sync timeout event");
	}
}

/************************************************************************/

LOCAL	void	end_trans_handler(

/* Handles END_TRANS events						*/

	ps_event_t	*ep			/*event pointer		*/
)
{
	ps_mess_t	*mp;			/* message pointer	*/
	ps_port_t	*pp=0;			/* port pointer		*/
	ps_task_t	*tp=0;			/* task pointer		*/
	ps_bus_t	*bp=0;			/* bus pointer		*/
	ps_link_t	*lp=0;			/* link pointer		*/
	long	i;				/* loop index		*/

	if(((mp = (ps_mess_t *)ep->gp) == NULL_MESS_PTR))
		warning("Empty message transmitted");
	if((mp->port == NULL_PORT)  ||
	    ((pp = port_ptr(mp->port))->state == PORT_FREE))
		warning("Message target port undefined");
	if((pp->owner == NULL_TASK) || ((tp = ps_task_ptr(pp->owner))->state 
	    == TASK_FREE))
		warning("Message target task undefined");
	
	switch(mp->c_code) {
	case BUS:
		bp = bus_ptr(mp->blid);
		if(--(bp->nqueued)) {
			bp->head = bp->head->next;
			bp->ep = add_event(ps_now + bus_delay, END_TRANS,
			    (long *)bp->head);
		}
		else {
			bp->head = bp->tail = NULL_MESS_PTR;
			bp->state = BUS_IDLE;
			if(bp->stat != NULL_STAT)
				ps_record_stat(bp->stat, 0.0);
			bp->ep = NULL_EVENT_PTR;
		}			
		for(i = 0; i < bp->nnodes; i++)
			if(bp->bnode[i] == tp->node)
				break;
		if(i >= bp->nnodes) {
			warning("Message target port not on bus");
			return;
		}
		break;

	case LINK:
		lp = link_ptr(mp->blid);
		if(lp->head->next != NULL_MESS_PTR) {
			
			lp->head = lp->head->next;
			lp->ep = add_event(ps_now + link_delay, END_TRANS,
			    (long *)lp->head);
		}
		else {
			lp->head = lp->tail = NULL_MESS_PTR;
			lp->state = LINK_IDLE;
			if(lp->stat != NULL_STAT)
				ps_record_stat(lp->stat, 0.0);
			lp->ep = NULL_EVENT_PTR;
		}
		if(lp->dnode != tp->node) {
			warning("Message target port not on link");
			return;
		}
		break;

	default:
		return;
	}

/*	Message got thru - queue it on the port and try to start owner	*/

	mp->next = NULL_MESS_PTR;
	if((pp->nmess)++) 
		pp->last = pp->last->next = mp;		
	else
		pp->first = pp->last = mp;

	if(tp->state == TASK_RECEIVING && tp->wport == mp->port) {
		tp->wport = NULL_PORT;
		remove_event(tp->rtoep);
		tp->rtoep = NULL_EVENT_PTR;
		find_host(tp);
	}
}
						
/************************************************************************/
/*		Parasol Scheduler Support Functions			*/
/************************************************************************/
extern
ps_event_t	*add_event(

/* Gets an event struct from the free list and sets its fields.  This	*/
/* event is then merged into the calendar according to event time and	*/
/* a pointer to it is returned. 					*/

	double	time,				/* event time		*/
	long	type,				/* primary event code	*/
	long	*gp				/* generic pointer	*/
)
{
	long	i;				/* loop index		*/
	ps_event_t	*ep, *epf, *epl;	 /* event pointers	*/

	if(event_fl == NULL_EVENT_PTR) {
		if(!(ep = event_fl = (ps_event_t *) malloc(100*sizeof(ps_event_t))))
			ps_abort("Insufficient memory");
		for(i = 1; i < 100; i++) {
			epf = ep++;
			epf->prior = ep;
		}
		ep->prior = NULL_EVENT_PTR;
	}
	ep = event_fl;
	event_fl = ep->prior;
	ep->time = time;
	ep->type = type;
	ep->gp = gp;
	if(time <= (epf = calendar[0].next)->time){
		ep->next = epf;
		ep->prior = epf->prior;
		calendar[0].next = epf->prior = ep;
	}
	else if((time+time) < (epf->time+(epl=calendar[1].prior)->time)) {
		while(time > epf->time) epf = epf->next;	
		ep->next = epf;
		ep->prior = epf->prior;
		epf->prior->next = ep;
		epf->prior = ep;
	}							
	else {
		while(time < epl->time) epl = epl->prior;	
		ep->next = epl->next;
		ep->prior = epl;
		epl->next->prior = ep;
		epl->next = ep;
	}
#if defined(DEBUG)
	print_event( "add_event", ep );
#endif
	return(ep);
}
	
/************************************************************************/

LOCAL	void	dq_ready(

/* Dequeues a given ready task from a given node's ready-to-run queue	*/

	ps_node_t	*np,			/* node pointer		*/
	ps_task_t	*tp			/* task pointer		*/
)
{
	long	task;				/* task index		*/
	ps_task_t	*btp = 0;
	ps_task_t	 *ctp = 0;		/* task pointers	*/
	
	if(np->discipline == PS_CFS){
		task=tid(tp);
		
		dq_cfs_task(tp);

	}
	else{
		task = np->rtrq;
	while(task != NULL_TASK && (ctp = ps_task_ptr(task)) != tp) {
		btp = ctp;
		task = ctp->next;
	}
	if(task == NULL_TASK)
		ps_abort("Ready task missing from ready-to-run queue");
	if(task == np->rtrq) 
		np->rtrq = ctp->next;
	else
		btp->next = ctp->next;
	}
}
	
/************************************************************************/

LOCAL	void	find_host(

/* Finds a host cpu for the given task making the task either		*/
/* TASK_COMPUTING, TASK_HOT, or TASK_BLOCKED depending on whether there	*/
/* is remaining cpu time or if the caller is the simulation driver.	*/
/* This may change the state of a running task to TASK_READY if pre-	*/
/* emption is permitted.  If unable to find a suitable host, the state	*/
/* of the input task is made TASK_READY.*/

	ps_task_t	*tp			/* task pointer		*/
)
{
	ps_node_t	*np;			/* node pointer		*/
	long	host;				/* host cpu id index	*/
	long	i;				/* loop index		*/
	long	minp;				/* min preempt priority	*/
	long	phost;				/* preemptable host	*/
	ps_task_t	*ptp;			/* preempt. task pointer*/
	double	q;				/* quantum		*/
	long	task;				/* task index		*/

	host = NULL_HOST;
	tp->sched_time = ps_now;		/* Ready to run now!	*/
	np=node_ptr(tp->node);
	if(np->discipline == PS_CFS ){
		find_host_cfs(tp);
		return;
	}

	
/* 	First look for a free host					*/
	if((np = node_ptr(tp->node))->nfree
	    && (np->cpu[0].scheduler == NULL_TASK 
	    || tp->priority > MAX_PRIORITY)) {
		if(tp->host == ANY_HOST || 
		   np->cpu[tp->host].state == CPU_IDLE) {
			if(tp->host == ANY_HOST) { 
				for(host = 0; host < np->ncpu; host++) 
					if(np->cpu[host].state == CPU_IDLE)
						break;
			}
			else
				host = tp->host;
			np->cpu[host].state = CPU_BUSY;
			(np->nfree)--;
			if(np->sf & SF_PER_HOST) 
				ps_record_stat(np->cpu[host].stat, 1.0);
			if(np->sf & SF_PER_NODE)
				ps_record_stat(np->stat, np->ncpu - np->nfree);
		}
	}

/* 	If necessary look for host with preemptable task		*/

	phost = NULL_HOST;
	if(host == NULL_HOST && np->discipline == PS_PR
	    && (np->cpu[0].scheduler == NULL_TASK 
	    || tp->priority > MAX_PRIORITY)) {
		if(tp->host == ANY_HOST) {
			minp = tp->priority;
			for(i = 0; i < np->ncpu; i++) {
				ptp = ps_task_ptr(np->cpu[i].run_task); 	
				if((ptp->priority < minp) 
				  && (ptp->state != TASK_SYNC) 
				  && (ptp->state != TASK_SYNC_SUSPEND)
				  && (ptp->state != TASK_SYNC_FREE)) {
					minp = ptp->priority;
					phost = i;
				}
			}
		}
		else {
			ptp = ps_task_ptr(np->cpu[tp->host].run_task);
			if((ptp->priority < tp->priority)
		          && (ptp->state != TASK_SYNC)
			  && (ptp->state != TASK_SYNC_SUSPEND)
			  && (ptp->state != TASK_FREE)) 
				phost = tp->host;
		}

		if(phost != NULL_HOST) {
	
/* 		Found a preemptable task - preempt it			*/

			host = phost;
			ptp = ps_task_ptr(np->cpu[host].run_task);
			remove_event(ptp->qep);
			ptp->qep = NULL_EVENT_PTR;
 			
 			/*Mark the preemption time tag and start to record the preemption time. Tao*/
 			ptp->pt_tag = 1;
 			ptp-> pt_last = ps_now;
 			/*End here*/ 
 			
 			
 			switch(ptp->state) {

			case TASK_COMPUTING:
				ptp->rct = ptp->tep->time - ps_now;
				
			case TASK_BLOCKED:
				remove_event(ptp->tep);
				ptp->tep = NULL_EVENT_PTR;

			case TASK_SPINNING:
				dq_lock(ptp);
				ptp->hp = NULL_HOST_PTR;
				ready(ptp);
				break;
		
			case TASK_HOT:
				ptp->hp = NULL_HOST_PTR;
				ready(ptp);
				ps_htp = tp;
				/* WCS - 25 May 1998 - Added np->ts_tab check below. */
				if (np->cpu[host].ts_tab || np->ts_tab)
					set_run_task(np, &np->cpu[host], 
					    ps_htp);
				else
					np->cpu[host].run_task = ps_myself;
				SET_TASK_STATE(ps_htp, TASK_HOT);
				ps_htp->hp = (ps_cpu_t *)&(np->cpu[host]);
				if(ts_flag)
					ts_report(tp, "executing");
				ctxsw(&ptp->context, &tp->context);
				return;
			}
		}
	}

/*	If host found, make tp runnable					*/

	if(host != NULL_HOST) {

		if(tp->pt_tag == 1)
	    	{	tp->pt_tag = 0;
	    		tp->pt_sum += (ps_now - tp->pt_last);	
	    	}

		if(ts_flag)
			ts_report(tp, "executing");
		task = tid(tp);

		if (np->sf & (SF_PER_TASK_HOST | SF_PER_TASK_NODE))
			set_run_task(np, &np->cpu[host], tp);
		else
			np->cpu[host].run_task = task;
		tp->hp = (ps_cpu_t *)&(np->cpu[host]);
		if((q = np->quantum) > 0.0)
			tp->qep = add_event(ps_now + q, END_QUANTUM, 
			    (long *)task);
		if(tp->rct > 0.0) {
			SET_TASK_STATE(tp, TASK_COMPUTING);
			tp->tep = add_event(ps_now + tp->rct, END_COMPUTE, 
			    (long *)task);
			tp->rct = 0.0;
		}
		else {
			if(ps_htp == DRIVER_PTR) {
				ps_htp = tp;
				SET_TASK_STATE(ps_htp, TASK_HOT);
				ctxsw(&d_context, &tp->context);
			}
			else if(ps_htp != tp) {
				SET_TASK_STATE(tp, TASK_BLOCKED);
				tp->tep = add_event(ps_now, END_BLOCK, 
				    (long *)task);
			}
		}
	} 
	else {
		qxflag = TRUE;
		ready(tp);
		qxflag = FALSE;
		if(tp == ps_htp)
			sched();
	}
}
			
				
/************************************************************************/
LOCAL 	void	find_priority(

/* Finds ready task with priority greater than given priority and makes	*/
/* its state either TASK_COMPUTING, TASK_HOT or TASK_BLOCKED depending	*/
/* on whether there is cpu remaining or the caller is the simulation	*/
/* driver.  If none is found the current task continues as appropriate. */
/* Otherwise, the current task is preempted and made TASK_READY.	*/

	ps_node_t	*np,			/* node pointer		*/
	ps_cpu_t	*hp,			/* host pointer		*/
	long	priority			/* test priority	*/
)
{

	ps_task_t	*btp, *ctp, *tp;	/* task pointers	*/
	long	task;				/* task index		*/
	long	host;				/* host cpu id index	*/
	double	q;				/* cpu quantum		*/
	
	if (np->discipline == PS_CFS){
		find_priority_cfs(np,hp);
		return;
	}
	q = np->quantum;
	host = hid(np, hp);
	tp = ps_task_ptr(hp->run_task);
	task = np->rtrq;
	while(task != NULL_TASK && 
	    ((ctp = ps_task_ptr(task))->priority) > priority) {
		if(ctp->host == ANY_HOST || ctp->host == host)
			break;
		btp = ctp;
		task = ctp->next;
	}
	if(task != NULL_TASK && ctp->priority > priority
	    && (np->cpu[0].scheduler == NULL_TASK 
	    || ctp->priority > MAX_PRIORITY)) {	/* Preempting task found*/
		if(np->rtrq == task)
			np->rtrq = ctp->next;
		else{
			btp->next = ctp->next;
		}	
			/* current task is preempted*/
			tp->pt_tag = 1;
 			tp-> pt_last = ps_now;

			/* Preempting task */
			if(ctp->pt_tag == 1)
	    	{	ctp->pt_tag = 0;
	    		ctp->pt_sum += (ps_now - ctp->pt_last);	
	    	}
		

		if(tp->tep != NULL_EVENT_PTR) {
			tp->rct = tp->tep->time - ps_now;
			remove_event(tp->tep);
			tp->tep = NULL_EVENT_PTR;
		}
		dq_lock(tp);
		ready(tp);
		qxflag = FALSE;
 		if(ts_flag)
			ts_report(ctp, "executing");

		/* WCS - 25 May 1998 - Added np->ts_tab check below. */
		
		if (hp->ts_tab || np->ts_tab)
			set_run_task(np, hp, ps_task_ptr(task));
		else
			hp->run_task = task;
 		ctp->hp = hp;
		if(q > 0.0)
			ctp->qep = add_event(ps_now + q, END_QUANTUM, 
			    (long *)hp->run_task);
		if(ctp->rct > 0.0) {
			ctp->tep = add_event(ps_now + ctp->rct, END_COMPUTE, 
			    (long *)hp->run_task);
 			ctp->rct = 0.0;
			SET_TASK_STATE(ctp, TASK_COMPUTING);
			if(ps_htp == tp) {
				ps_htp = DRIVER_PTR;
				ctxsw(&tp->context, &d_context);
			}
		}
		else if(ps_htp == DRIVER_PTR) {
 			ps_htp = ctp;
			SET_TASK_STATE(ps_htp, TASK_HOT);
			ctxsw(&d_context, &ps_htp->context);
		}
		else if(ps_htp == tp) {
 			ps_htp = ctp;
			SET_TASK_STATE(ps_htp, TASK_HOT);
			ctxsw(&tp->context, &ps_htp->context);
		}
		else {
			SET_TASK_STATE(ctp, TASK_BLOCKED);
			ctp->tep = add_event(ps_now, END_BLOCK, 
			    (long *)hp->run_task);
		}
	}
	else {				/* No preempting task		*/
		qxflag = FALSE;
		if(ps_htp == DRIVER_PTR) {
			if((q > 0.0) && (tp->qep == NULL_EVENT_PTR || 
			    tp->qep->time <= ps_now))
				tp->qep = add_event(ps_now+q, END_QUANTUM, 
				    (long *)hp->run_task);
			else {
 				ps_htp = tp;
				SET_TASK_STATE(ps_htp, TASK_HOT);
				ctxsw(&d_context, &tp->context);
			}
		}
	}
}

/************************************************************************/
LOCAL	void	find_host_cfs(

/* Finds a host cpu for the given task making the task either		*/
/* TASK_COMPUTING, TASK_HOT, or TASK_BLOCKED depending on whether there	*/
/* is remaining cpu time or if the caller is the simulation driver.	*/
/* This may change the state of a running task to TASK_READY if pre-	*/
/* emption is permitted.  If unable to find a suitable host, the state	*/
/* of the input task is made TASK_READY.*/

	ps_task_t	*tp			/* task pointer		*/
)
{
	ps_node_t	*np;			/* node pointer		*/
	long	host;				/* host cpu id index	*/
	double	q;				/* quantum		*/
	long	task;				/* task index		*/

	tp->sched_time = ps_now;		/* Ready to run now!	*/
	np=node_ptr(tp->node);
	/* find a host with a min load */
	if((host=tp->host)==ANY_HOST){
		host=find_min_load_host(np,tp);
	}
	qxflag = TRUE;

	ready_cfs(host,tp);


	qxflag = FALSE;

/*	If the host is free host, make tp runnable			*/

	if(np->cpu[host].state == CPU_IDLE){
		np->cpu[host].state = CPU_BUSY;
		(np->nfree)--;
		if(np->sf & SF_PER_HOST) 
			ps_record_stat(np->cpu[host].stat, 1.0);
		if(np->sf & SF_PER_NODE)
			ps_record_stat(np->stat, np->ncpu - np->nfree);
		
		if(tp->pt_tag == 1)
	    	{	tp->pt_tag = 0;
	    		tp->pt_sum += (ps_now - tp->pt_last);	
	    	}
		if(ts_flag)
			ts_report(tp, "executing");
		task = tid(tp);

		/* set this cfs task to run */	
		set_cfs_task_run(tp);
	
		if (np->sf & (SF_PER_TASK_HOST | SF_PER_TASK_NODE))
			set_run_task(np, &np->cpu[host], tp);
		else
			np->cpu[host].run_task = task;
		tp->hp = (ps_cpu_t *)&(np->cpu[host]);
		if((q = np->quantum) > 0.0)
			tp->qep = add_event(ps_now + q, END_QUANTUM, 
					    (long *)task);
		if(tp->rct > 0.0) {
			SET_TASK_STATE(tp, TASK_COMPUTING);
			tp->tep = add_event(ps_now + tp->rct, END_COMPUTE, 
					    (long *)task);
			tp->rct = 0.0;
		}
		else {
			if(ps_htp == DRIVER_PTR) {
				ps_htp = tp;
				SET_TASK_STATE(ps_htp, TASK_HOT);
				ctxsw(&d_context, &tp->context);
			}
			else if(ps_htp != tp) {
				SET_TASK_STATE(tp, TASK_BLOCKED);
				tp->tep = add_event(ps_now, END_BLOCK, 
						    (long *)task);
			}
		}

	}
	else {
		qxflag = FALSE;
		if(tp == ps_htp)
			sched();
	}
}

/************************************************************************/			
LOCAL 	void	find_priority_cfs(

/* Finds a ready task with larger fair than running task and makes	*/
/* its state either TASK_COMPUTING, TASK_HOT or TASK_BLOCKED depending	*/
/* on whether there is cpu remaining or the caller is the simulation	*/
/* driver.  If none is found the current task continues as appropriate. */
/* Otherwise, the current task is preempted and made TASK_READY.	*/

	ps_node_t	*np,			/* node pointer		*/
	ps_cpu_t	*hp			/* host pointer		*/
)
{

	ps_task_t	*ctp, *tp;	/* task pointers	*/
	ps_group_t 	*gp;			/* group pointer	*/
	long	task;				/* task index		*/
	long	host;				/* host cpu id index	*/
	double	q;				/* cpu quantum		*/
	long 	f;				/* flag			*/

	host = hid(np, hp);
	tp = ps_task_ptr(hp->run_task);

	/* update the running task	*/
	update_run_task(tp);

	/* if the group with cap share	*/	
	if(tp->group!=-1) {
		if((tp->si->parent->fair<0) && ((gp=group_ptr(tp->group))->cap)) {
			cap_handler(tp);
			return;
		}
	}
	q = np->quantum;

	task = find_fair_task(np->host_rq+host);
	f=0;
	if((task != NULL_TASK ) && ((ctp=ps_task_ptr(task))!=tp)){
		update_ready_task(ctp);
		if(check_fair(tp,ctp))
			f=1;
	}

	if(f==1) {	/* Preempting task found*/
		if(tp->tep != NULL_EVENT_PTR) {
			tp->rct = tp->tep->time - ps_now;
			remove_event(tp->tep);
			tp->tep = NULL_EVENT_PTR;
		}
		dq_lock(tp);

		/* cooling this cfs task */
		cooling_cfs_task(tp);
		//SET_TASK_STATE(tp, TASK_READY);
		ready_cfs(host,tp);

		/* current task is preempted*/
			tp->pt_tag = 1;
 			tp-> pt_last = ps_now;

			/* Preempting task */
			if(ctp->pt_tag == 1)
	    	{	ctp->pt_tag = 0;
	    		ctp->pt_sum += (ps_now - ctp->pt_last);	
	    	}
 		if(ts_flag)
			ts_report(ctp, "executing");

		/* set this task to run; */
		set_cfs_task_run(ctp);

		/* WCS - 25 May 1998 - Added np->ts_tab check below. */
		
		if (hp->ts_tab || np->ts_tab)
			set_run_task(np, hp, ps_task_ptr(task));
		else
			hp->run_task = task;
 		ctp->hp = hp;
		if(q > 0.0)
			ctp->qep = add_event(ps_now + q, END_QUANTUM, 
					     (long *)hp->run_task);
		if(ctp->rct > 0.0) {
			ctp->tep = add_event(ps_now + ctp->rct, END_COMPUTE, 
					     (long *)hp->run_task);
 			ctp->rct = 0.0;
			SET_TASK_STATE(ctp, TASK_COMPUTING);
			if(ps_htp == tp) {
				ps_htp = DRIVER_PTR;
				ctxsw(&tp->context, &d_context);
			}
		}
		else if(ps_htp == DRIVER_PTR) {
 			ps_htp = ctp;
			SET_TASK_STATE(ps_htp, TASK_HOT);
			ctxsw(&d_context, &ps_htp->context);
		}
		else if(ps_htp == tp) {
 			ps_htp = ctp;
			SET_TASK_STATE(ps_htp, TASK_HOT);
			ctxsw(&tp->context, &ps_htp->context);
		}
		else {
			SET_TASK_STATE(ctp, TASK_BLOCKED);
			ctp->tep = add_event(ps_now, END_BLOCK, 
					     (long *)hp->run_task);
		}
	}
	else {				/* No preempting task		*/
		qxflag = FALSE;
		
		if(ps_htp == DRIVER_PTR) {
			if((q > 0.0) && (tp->qep == NULL_EVENT_PTR || 
					 tp->qep->time <= ps_now))
				tp->qep = add_event(ps_now+q, END_QUANTUM, 
						    (long *)hp->run_task);
			else {
 				ps_htp = tp;
				SET_TASK_STATE(ps_htp, TASK_HOT);
				ctxsw(&d_context, &tp->context);
			}
		}
	}
}

/************************************************************************/

LOCAL	void	find_ready(
		
/* Finds and dequeues a ready task making it either TASK_COMPUTING,	*/
/* TASK_HOT, or TASK_BLOCKED depending on whether there is remaining 	*/
/* cpu time or if the caller is the simulation driver.  If no runnable	*/
/* task exists, the node and host states are changed as appropriate and	*/
/* a context switch to the simulation driver is	performed(if necessary).*/

	ps_node_t	*np,			/* node pointer		*/
	ps_cpu_t	*hp			/* host cpu pointer	*/
)
{
	long	task;				/* task index		*/
	ps_task_t	*btp, *ctp;		/* task pointers	*/
	long	host;				/* host cpu id index	*/
	double	q;				/* cpu quantum		*/

	if(np->discipline == PS_CFS ){
		find_ready_cfs(np,hp);		
		return;
	}

	host = hid(np, hp);	
	task = np->rtrq;
	while(task != NULL_TASK) {
		if((ctp = ps_task_ptr(task))->host == ANY_HOST 
		    || ctp->host == host)
			break;
		btp = ctp;
		task = ctp->next;
		}
	if(task != NULL_TASK		
	    && (np->cpu[0].scheduler == NULL_TASK 
	    || ctp->priority > MAX_PRIORITY)) {	/* Ready task found	*/
	    
	    	/* Clear the premption tag and add the current to the total preemption time. Tao*/
	    	if(ctp->pt_tag == 1)
	    	{	ctp->pt_tag = 0;
	    		ctp->pt_sum += (ps_now - ctp->pt_last);	
	    	}
	    	/* End here*/ 
	    
		if(ts_flag)
			ts_report(ctp, "executing");
		if(np->rtrq == task)
			np->rtrq = ctp->next;
		else
			btp->next = ctp->next;
		
		/* WCS - 14 April 1998 - Added np->ts_tab check below. */
				
		if (hp->ts_tab || np->ts_tab)
			set_run_task(np, hp, ps_task_ptr(task));
		else
			hp->run_task = task;
 		ctp->hp = hp;
		if((q = np->quantum))
			ctp->qep = add_event(ps_now + q, END_QUANTUM, 
			    (long *)hp->run_task);
		if(ctp->rct > 0.0) {
			SET_TASK_STATE(ctp, TASK_COMPUTING);
 			ctp->tep = add_event(ps_now + ctp->rct, END_COMPUTE,
			    (long *)hp->run_task);
			ctp->rct = 0.0;
			if((ps_htp != DRIVER_PTR) && (ps_htp->state != 
			    TASK_HOT)) {
				btp = ps_htp;
				ps_htp = DRIVER_PTR;
				ctxsw(&btp->context, &d_context);
			}
		}
		else if((ps_htp != DRIVER_PTR) && (ps_htp->state == TASK_HOT)) {
			SET_TASK_STATE(ctp, TASK_BLOCKED);
			ctp->tep = add_event(ps_now, END_BLOCK, 
			    (long *)hp->run_task);
		}
		else {

			btp = ps_htp;
			ps_htp = ctp;
			SET_TASK_STATE(ps_htp, TASK_HOT);
			if(btp == DRIVER_PTR) {
				ctxsw(&d_context, &ps_htp->context);
			}
			else {
				ctxsw(&btp->context, &ps_htp->context);
			}
		}
	}
	else {				/* No ready task available	*/
		(np->nfree)++;
		hp->state = CPU_IDLE;
		if(np->sf & SF_PER_HOST)
			ps_record_stat(hp->stat, 0.0);
		if(np->sf & SF_PER_NODE)
			ps_record_stat(np->stat, np->ncpu - np->nfree);
		if (np->sf & (SF_PER_TASK_NODE | SF_PER_TASK_HOST))
			set_run_task(np, hp, NULL_TASK_PTR);
		else
			hp->run_task = NULL_TASK;
 		if((ps_htp != DRIVER_PTR) && (ps_htp->state != TASK_HOT)) 
			sched();
	}
}

/************************************************************************/
LOCAL	void	find_ready_cfs(
		
/* Finds and dequeues a ready task making it either TASK_COMPUTING,	*/
/* TASK_HOT, or TASK_BLOCKED depending on whether there is remaining 	*/
/* cpu time or if the caller is the simulation driver.  If no runnable	*/
/* task exists, the node and host states are changed as appropriate and	*/
/* a context switch to the simulation driver is	performed(if necessary).*/

	ps_node_t	*np,			/* node pointer		*/
	ps_cpu_t	*hp			/* host cpu pointer	*/

)
{
	long	task;				/* task index		*/
	ps_task_t	*btp, *ctp;		/* task pointers	*/
	long	host;				/* host cpu id index	*/
	double	q;				/* cpu quantum		*/

	host = hid(np, hp);
	task=find_fair_task(np->host_rq+host);

	if(task != NULL_TASK ) {	/* Ready task found	*/
	    /*hp->port_n --; */
	ctp = ps_task_ptr(task);

		/* check whether is a preempted task. */
		if(ctp->pt_tag == 1)
	    {	ctp->pt_tag = 0;
	    	ctp->pt_sum += (ps_now - ctp->pt_last);	
	    }

		if(ts_flag)
			ts_report(ctp, "executing");
		
		/* set the cfs_task to run */
		set_cfs_task_run(ctp);
/* WCS - 14 April 1998 - Added np->ts_tab check below. */
				
		if (hp->ts_tab || np->ts_tab)
			set_run_task(np, hp, ps_task_ptr(task));
		else
			hp->run_task = task;
 		ctp->hp = hp;
		if((q = np->quantum))
			ctp->qep = add_event(ps_now + q, END_QUANTUM, 
			    (long *)hp->run_task);
		if(ctp->rct > 0.0) {
			SET_TASK_STATE(ctp, TASK_COMPUTING);
 			ctp->tep = add_event(ps_now + ctp->rct, END_COMPUTE,
			    (long *)hp->run_task);
			ctp->rct = 0.0;
			if((ps_htp != DRIVER_PTR) && (ps_htp->state != 
			    TASK_HOT)) {
				btp = ps_htp;
				ps_htp = DRIVER_PTR;
				ctxsw(&btp->context, &d_context);
			}
		}
		else if((ps_htp != DRIVER_PTR) && (ps_htp->state == TASK_HOT)) {
			SET_TASK_STATE(ctp, TASK_BLOCKED);
			ctp->tep = add_event(ps_now, END_BLOCK, 
			    (long *)hp->run_task);
		}
		else {
			btp = ps_htp;
			ps_htp = ctp;
			SET_TASK_STATE(ps_htp, TASK_HOT);
			if(btp == DRIVER_PTR) {
				ctxsw(&d_context, &ps_htp->context);
			}
			else {
				ctxsw(&btp->context, &ps_htp->context);
			}
		}
	
	}
	else {				/* No ready task available	*/
		(np->nfree)++;
		hp->state = CPU_IDLE;
		if(np->sf & SF_PER_HOST)
			ps_record_stat(hp->stat, 0.0);
		if(np->sf & SF_PER_NODE)
			ps_record_stat(np->stat, np->ncpu - np->nfree);
		if (np->sf & (SF_PER_TASK_NODE | SF_PER_TASK_HOST))
			set_run_task(np, hp, NULL_TASK_PTR);
		else
			hp->run_task = NULL_TASK;
 		if((ps_htp != DRIVER_PTR) && (ps_htp->state != TASK_HOT)) 
			sched();
	}
}

/************************************************************************/

LOCAL	void	init_event(void)

/* Initializes the event free list to empty and also the event calendar	*/
/* to empty.								*/

{
	ps_event_t	*ep;			/* event pointer	*/

	event_fl = NULL_EVENT_PTR;
	(ep = calendar)->time = -1.0;
	ep->type = CALENDAR;
	ep->prior = NULL_EVENT_PTR;
	ep->next = ep + 1;
	(++ep)->time = MAX_DOUBLE;
	ep->type = CALENDAR;
	ep->prior = ep - 1;
	ep->next = NULL_EVENT_PTR;
}

/************************************************************************/

LOCAL	ps_event_t	*next_event(void)

/* Removes the most imminent event from the calendar returning it to 	*/
/* the free list.  It may be examined by the returned pointer, however,	*/
/* its contents are volatile and cannot be trusted for long.  Should	*/
/* the calendar run dry, the null event pointer is returned.		*/


{
	ps_event_t	*ep, *cep;		/* event pointers	*/

	if((cep = calendar)->next->next == NULL_EVENT_PTR)
		return(NULL_EVENT_PTR);

	ep = cep->next;
	cep->next = ep->next;
	ep->next->prior = cep;
	ep->prior = event_fl;
	event_fl = ep;
	return(ep);
}
	
/************************************************************************/

LOCAL	void	ready(

/* Makes the given task TASK_READY and queues it on the appropriate	*/
/* ready-to-run-queue according to its priority if relevant		*/

	ps_task_t	*tp			/* task pointer		*/
)
{
	long	task;				/* task index		*/
	ps_task_t	*btp;			/* back task pointer	*/
	ps_task_t	*ctp;			/* current task pointer	*/
	ps_node_t	*np;			/* node pointer		*/

	SET_TASK_STATE(tp, TASK_READY);
	tp->hp = NULL_HOST_PTR;
	tp->sched_time = ps_now;
	if(ts_flag)
		ts_report(tp, "ready");
	np = node_ptr(tp->node);
	if(np->discipline == PS_CFS){
		task=tid(tp);
		enqueue_cfs_task(tp);
	}
       else{
	task = np->rtrq;
	if(np->discipline == PS_FIFO) 
		while(task != NULL_TASK) {
			btp = ps_task_ptr(task);
			task = btp->next;
		}
	else {
		if(np->discipline == PS_HOL || qxflag)
			while(task != NULL_TASK && 
		            (ctp = ps_task_ptr(task))->priority >= 
			    tp->priority) {
				btp = ctp;
				task = ctp->next;
			}	
		else 
			while(task != NULL_TASK &&
			    (ctp = ps_task_ptr(task))->priority >
			    tp->priority) {
				btp = ctp;
				task = ctp->next;
			}
	}

	if(task == np->rtrq) {
		tp->next = np->rtrq;
		np->rtrq = tid(tp);
	}
	else {
		tp->next = btp->next;
		btp->next = tid(tp);
	}
      }
	if (np->cpu[0].scheduler != NULL_TASK 
	    && tp->priority <= MAX_PRIORITY
	    && (ps_htp == DRIVER_PTR || ps_htp->priority <= MAX_PRIORITY 
	    || ps_htp->priority == MAX_PRIORITY+2))
 		ps_send(ps_std_port(np->cpu[0].scheduler), SN_READY, "", 
		    tid(tp));
}

/************************************************************************/

LOCAL	void	ready_cfs(

/* Makes the given task TASK_READY and queues it on the appropriate	*/
/* ready-to-run-queue according to its priority if relevant		*/
	long 	host,
	ps_task_t	*tp			/* task pointer		*/
	)
{
	ps_node_t	*np;			/* node pointer		*/
	sched_info 	*si;
	ps_cfs_rq_t	*rq;

	SET_TASK_STATE(tp, TASK_READY);
	
	tp->sched_time = ps_now;/*??? */
	if(ts_flag)
		ts_report(tp, "ready");
	np = node_ptr(tp->node);
	tp->hp = (ps_cpu_t *)&(np->cpu[host]);
	si=tp->si;
	if(np->ngroup){
		rq=tp->hp->group_rq+tp->group;
		si->rq=rq;
		si->parent=rq->si;
	}
	else
		si->rq=np->host_rq+host;

	enqueue_cfs_task(tp);
	/*np->host_rq[host].load++; */


	if (np->cpu[0].scheduler != NULL_TASK 
	    && tp->priority <= MAX_PRIORITY
	    && (ps_htp == DRIVER_PTR || ps_htp->priority <= MAX_PRIORITY 
		|| ps_htp->priority == MAX_PRIORITY+2))
 		ps_send(ps_std_port(np->cpu[0].scheduler), SN_READY, "", 
			tid(tp));
}

/************************************************************************/
extern
void	remove_event(

/* Removes an event from the calendar and adds it to the free list.  It	*/
/* may be examined using its pointer however its contents are valid 	*/
/* only until the struct is reused by an add_event call. If the event	*/
/* pointer is null, nothing happens.  Guard code ensures that the event	*/
/* is in a doubly linked list and aborts the simulation if not.		*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	
	if(ep == NULL_EVENT_PTR)
		return;
	if(ep->prior == NULL_EVENT_PTR ||
	   ep->next == NULL_EVENT_PTR ||
	   ep->next->prior == NULL_EVENT_PTR ||
	   ep->prior->next == NULL_EVENT_PTR ||
	   ep->next->prior != ep ||
	   ep->prior->next != ep)
		ps_abort("Attempting to remove a non calendar event");
	
	ep->prior->next = ep->next;
	ep->next->prior = ep->prior;
	ep->prior = event_fl;
	event_fl = ep;
#if defined(DEBUG)
	print_event( "remove_event", ep );
#endif
}
	
/************************************************************************/

LOCAL	void	sched(void)

/* Performs a context switch between a cooling hot task and a blocked	*/
/* task (if available).  Otherwise, the switch is to the simulation	*/
/* driver.								*/

{
	ps_event_t	*ep;			/* event pointer	*/
	ps_task_t	*tp, *ctp;		/* task pointers	*/

	ep = calendar;
	ep = ep->next;
	while(ep->time == ps_now) {
		if(ep->type == END_BLOCK)
			break;
		ep = ep->next;
	}
	if(ep->time == ps_now && ep->type == END_BLOCK) {
		remove_event(ep);
		if((tp = ps_task_ptr((size_t)ep->gp))->state != TASK_BLOCKED)
			ps_abort("Bad END_BLOCK event");
		tp->tep = NULL_EVENT_PTR;
		ctp = ps_htp;
		ps_htp = tp;
		SET_TASK_STATE(ps_htp, TASK_HOT);
		ctxsw(&ctp->context, &ps_htp->context);
	}
	else {
		tp = ps_htp;
		ps_htp = DRIVER_PTR;
		ctxsw(&tp->context, &d_context);
	}
}
	

	
/************************************************************************/	
/*		PARASOL Dynamic Table Functions				*/
/************************************************************************/

LOCAL	long	free_table_entry(

/*	Frees a specified dynamic table entry for reuse.		*/

	ps_table_t	*tabp,			/* table pointer	*/
	long	id				/* table entry index	*/	
)
{
	if(tabp->signature != TABLE)
		return(SYSERR);
	if(id < 0 || id >= tabp->tab_size)
		return(SYSERR);

	if(*((long *)(tabp->tab+id*tabp->entry_size))) {
		*((long *)(tabp->tab+id*tabp->entry_size)) = (long) 0;
		tabp->used--;
	}
	return(OK);
}

/************************************************************************/

LOCAL	long	get_table_entry(

/*	Gets a free dynamic table entry using a rover to search. Grows	*/
/*	table by factor of 2 in size if necessary.			*/

	ps_table_t	*tabp			/* table pointer	*/
)
{
	long	new_size;			/* new table size	*/
	char	*new_tab;			/* new table pointer	*/
	char	*ncp;				/* new char pointer	*/
	char	*ocp;				/* old char pointer	*/
	long	i;				/* loop index		*/
	long	nbytes;				/* byte count		*/
	long	id;				/* table entry index	*/

	if(tabp->signature != TABLE)
		return(SYSERR);

	if(tabp->used == tabp->tab_size) {
		nbytes = tabp->tab_size * tabp->entry_size;
		new_size = 2 * tabp->tab_size;
		if(!(new_tab =  (char *) 
		    ((double *)malloc(2*nbytes+sizeof(double)))))
			ps_abort("Insufficient memory");
		memset(new_tab, '\0', 2*nbytes);
 		ncp = new_tab;
		ocp = tabp->tab;
		for(i = 0; i < nbytes; i++)
			*ncp++ = *ocp++;
		free(tabp->tab);
		tabp->tab = new_tab;
		tabp->rover = tabp->tab_size;
		tabp->tab_size = new_size;
		tabp->base = tabp->tab + sizeof(double);
	}
	for(id = tabp->rover, i = 0; i < tabp->tab_size; i++) {
		if(!*((long *)(tabp->tab+id*tabp->entry_size)))
			break;
		id = (id + 1) % tabp->tab_size;
	}
	*((long *)(tabp->tab+id*tabp->entry_size)) = (long) 1;
	tabp->used++;
	tabp->rover = (id + 1) % tabp->tab_size;
	return(id);
}

/************************************************************************/

LOCAL	long	init_table(

/*	Initializes a dynamic table of a specified number of entries 	*/
/*	and a specified entry size (in bytes).				*/

	ps_table_t	*tabp,			/* table pointer	*/
	long	tab_size,			/* initial table size	*/
	long	entry_size			/* table entry size	*/
)
{

	if(tab_size < 0 || entry_size < 0)
		return(SYSERR);

	tabp->signature = TABLE;

/*	Adjust entry size for used/free flag				*/

	tabp->used = 0;
	tabp->rover = 0;

	if( first_run ) {
	  tabp->tab_size = tab_size;
	  tabp->entry_size = sizeof(double) + entry_size;
	  if(!(tabp->tab = (char *) ((double *)malloc(tab_size * (tabp->entry_size+sizeof(double))))))
	    ps_abort("Insufficient memory");
	} else { /* reuse existing table if possible  */
	  if( tabp->tab_size != tab_size || tabp->entry_size != sizeof(double) + entry_size ) {
	    if( tabp->tab != NULL ) free( tabp->tab );
	    if(!(tabp->tab = (char *) ((double *)malloc(tab_size * (tabp->entry_size+sizeof(double))))))
	      ps_abort("Insufficient memory");

	    tabp->tab_size = tab_size;
	    tabp->entry_size = sizeof(double) + entry_size;
	  }
	}

	tabp->base = tabp->tab + sizeof(double);
	memset(tabp->tab, '\0', tab_size * tabp->entry_size); 
	return(OK);
}


/************************************************************************/
/*		PARASOL Debugger Functions				*/
/************************************************************************/

LOCAL	void	debug(void)					

/* PARASOL debug interpreter						*/

{
	long	done;				/* completion flag	*/
	long	c;				/* command character	*/

	done = FALSE;
	while(!done) {
		system("stty raw");
		fprintf(stderr, "\r\n(%G)-> ", ps_now);
		c = getchar();
		system("stty -raw");
		switch(c) {

#if HAVE_KILL
		case 3:				/* Interrupt simulation	*/
			kill(getpid(), SIGINT);
			break;
#endif

		case 'Q':			/* abort simulation	*/
		case 'q':
			ps_abort("PARASOL aborted by user");
		
		case 'B':			/* set time break polong	*/	
		case 'b':
			fprintf(stderr, "\n\n---> Break Time: ");
			scanf("%lf", &break_time);
			while(getchar() != '\n');
			if(break_time > ps_now)
				break_flag = TRUE;
			break;

		case 'D':
		case 'd':
			display_breakpoints();
			break;

		case 'K':
		case 'k':
			task_breakpoints();
			break;

		case 'E':			/* event report		*/
		case 'e':
			event_report();
			break;

		case 'G':			/* go - stop stepping	*/
		case 'g':
			step_flag = FALSE;
			done = TRUE;
			break;

		case 'H':			/* hardware report 	*/
		case 'h':
			hard_report();
			break;

		case '?':
			fprintf(stderr, 
		   "\n\n---> P A R A S O L  Debugger Commands\n");
			fprintf(stderr, 
		   "\nb - set time Break\nk - task state break ");
			fprintf(stderr, "\nd - display breakpoints");
			fprintf(stderr, "\ne - show future Events\ng - Go ");
			fprintf(stderr,
		   "\nh - show Hardware status\ns - show Software status ");
			fprintf(stderr,
		   "\nt - Toggle Trace\n? - Help\nq - Abort\nelse - Step time\n");
			break;

		case 'S':			/* software report	*/
		case 's':
			soft_report();
			break;
		
		case 'T':			/* trace toggle		*/
		case 't':
			ts_flag = !ts_flag;
			if(ts_flag)
				fprintf(stderr, "\n---> Trace ON\n");
			else
				fprintf(stderr, "\n---> Trace OFF\n");
			break;			
			
		default:			/* step out		*/
			done = TRUE;
			step_flag = TRUE;
		}
	}
}

/************************************************************************/

LOCAL	void	display_breakpoints(void)

/* Gives a summary of all breakpoints that are set.			*/

{
	long		i;			/* loop index		*/
	char		p1[21];			/* string variables	*/
	ps_task_t	*tp;

	fprintf(stderr, "\n                B R E A K P O I N T   S U M M A R Y");
	if(break_flag)
		fprintf(stderr, "\n\nTime breakpolong set for %G", break_time);

	if(ntask_bps) {
		fprintf(stderr, "\n\nTask State Breakpoints");
 		fprintf(stderr, "\n ID |        Name          | Break on State");
		fprintf(stderr, "\n---------------------------------------------");
		for(i = 0; i < ps_task_tab.tab_size; i++) {
			if((tp = ps_task_ptr(i))->state != TASK_FREE
			    && tp->tbp != -2) {
			    	const char * p2;
				padstr(p1, tp->name, 20);
				if(tp->tbp == -1)
					p2 = "ANYSTATE";
				else
					p2 = task_state_names[tp->tbp];
				fprintf(stderr, "\n%3ld | %s | %s", i, p1, p2);
			}
		}
	}
	fprintf(stderr, "\n\n");
}

/************************************************************************/

LOCAL	 void 	event_report(void)			

/* Reports future calendar events 					*/

{
	ps_event_t	*ep;			/* event pointer	*/
	ps_task_t	*tp;			/* task pointer		*/
	ps_mess_t	*mp;			/* message pointer	*/
	ps_bus_t	*bp;			/* bus pointer		*/
	ps_link_t	*lp;			/* link pointer		*/
	ps_node_t	*np;			/* node pointer		*/
	static	const char	*e_name[] = {		/* event names		*/
		"  End sync  ", "End compute ", "End quantum ", "End transmit",
		" End sleep  ", "Receive t/o ", " End block  ", "", "", "",
		" Link fail  ", "Link restore", "  Bus fail  ", "Bus restore ",
		" Node fail  ", "Node restore", "", "", "", "", 
		" User event "
	};
	
	ep = calendar[0].next;
	if(ep != &calendar[1]) {
		fprintf(stderr, "\n\n       F U T U R E   E V E N T S\n\n");
		fprintf(stderr, "     Time      |     Type     |  ID | Name\n");
		fprintf(stderr,
		    "-----------------------------------------------------\n");
		while(ep != &calendar[1]) {
			fprintf(stderr, "%12.4f   | %s |", ep->time, 
			    e_name[ep->type]);
			switch(ep->type) {

			case END_SYNC:
			case END_COMPUTE:
			case END_QUANTUM:
			case END_SLEEP:
			case END_RECEIVE:
			case END_BLOCK:
				tp = ps_task_ptr((size_t)ep->gp);
				fprintf(stderr, "%4ld | %s\n", (size_t)ep->gp,
					tp->name);
				break;
			
			case END_TRANS:
				mp = (ps_mess_t *) ep->gp;
				tp = ps_task_ptr(port_ptr(mp->port)->owner);
				fprintf(stderr, "%4ld | %s\n", 
				    port_ptr(mp->port)->owner, tp->name);
				break;

			case LINK_FAILURE:
			case LINK_REPAIR:
				lp = (ps_link_t *) ep->gp;
				fprintf(stderr, "%4ld | %s\n", lid(lp),
				    lp->name);
				break;

			case BUS_FAILURE:
			case BUS_REPAIR:
				bp = (ps_bus_t *) ep->gp;
				fprintf(stderr, "%4ld | %s\n", bid(bp),
				    bp->name);
				break;

			case NODE_FAILURE:
			case NODE_REPAIR:
				np = (ps_node_t *) ep->gp;
				fprintf(stderr, "%4ld | %s\n", nid(np),
				    np->name);
				break;

			}
			ep = ep->next;
		}
		fprintf(stderr, "\n");
	}
}

/************************************************************************/

LOCAL 	void 	hard_report(void)

/* Reports hardware status						*/

{
	long	node;				/* node index		*/
	ps_node_t	*np;			/* node pointer		*/
	char	pstring[TEMP_STR_SIZE];			/* prlong string		*/
	long	task;				/* task index		*/
	ps_task_t	*tp;			/* task pointer		*/
	long	bus;				/* bus index		*/
	ps_bus_t	*bp;			/* bus pointer		*/
	ps_mess_t	*mp;			/* message pointer	*/
	long	link;				/* link index		*/
	ps_link_t	*lp;			/* link pointer		*/

	fprintf(stderr, "\n\n               H A R D W A R E   S T A T U S\n\n");
	fprintf(stderr, 
	"               Node                     ||     Tasks Queued\n");
	fprintf(stderr, 
	" ID |        Name          | Busy CPU's ||   ID |      Name\n");
	fprintf(stderr,
	"-------------------------------------------------------------------\n");
	for(node = 0; node < ps_node_tab.used; node++) {
		np = node_ptr(node);
		padstr(pstring, np->name, 20);
		fprintf(stderr,"%3ld | %s |     %3ld    ||", node, 
		    pstring, np->ncpu - np->nfree);
		if((task = np->rtrq) != NULL_TASK) {
			while(task != NULL_TASK) {
				if(task != np->rtrq)
					fprintf(stderr,
	"                                        ||");
				padstr(pstring, (tp = ps_task_ptr(task))->name,
				    20);
				fprintf(stderr," %4ld | %s\n", task, 
				    pstring);
				task = tp->next;
			}
		}
		else
			fprintf(stderr, "      |\n");
	}

	if(ps_bus_tab.used > 0) {
		fprintf(stderr, 
	"\n               Bus                       Messages Queued\n");
		fprintf(stderr, 
	" ID |        Name          | State || Sender | Port | Type\n");
		fprintf(stderr,
	"--------------------------------------------------------------\n");
		for(bus = 0; bus < ps_bus_tab.used; bus++) {
			bp = bus_ptr(bus);
			padstr(pstring, bp->name, 20);
			fprintf(stderr, "%3ld | %s |", bus, pstring);
				switch(bp->state) {
			case BUS_IDLE:
				fprintf(stderr, "  IDLE ");
				break;
			case BUS_BUSY:
				fprintf(stderr, "  BUSY ");
				break;
			case BUS_DOWN:
				fprintf(stderr, "  DOWN ");
			}
			fprintf(stderr, "||");
			if((mp = bp->head)) {
				while(mp != NULL_MESS_PTR) {
					if(mp != bp->head)
						fprintf(stderr, 
	"                                   ||");
					fprintf(stderr, 
					    "   %4ld |  %3ld | %ld\n", 
					    mp->sender, mp->port, 
					    mp->type);
					mp = mp->next;
				}
			}
			else
				fprintf(stderr,"        |      |\n");
		}
	}

	if(ps_link_tab.used > 0) {
		fprintf(stderr, 
	"               Link                      Messages Queued\n");
		fprintf(stderr, 
	" ID |        Name          | State || Sender | Port | Type\n");
		fprintf(stderr,
	"--------------------------------------------------------------\n");
		for(link = 0; link < ps_link_tab.used; link++) {
			lp = link_ptr(link);
			padstr(pstring, lp->name, 20);
			fprintf(stderr, "%3ld | %s |", link, pstring);
			switch(lp->state) {
			case LINK_IDLE:
				fprintf(stderr, "  IDLE ");
				break;
			case LINK_BUSY:
				fprintf(stderr, "  BUSY ");
				break;
			case LINK_DOWN:
				fprintf(stderr, "  DOWN ");
			}
			fprintf(stderr, "||");
			if((mp = lp->head)) {
				while(mp != NULL_MESS_PTR) {
					if(mp != lp->head)
						fprintf(stderr, 
	"                                   ||");
					fprintf(stderr, 
					    "   %4ld |  %3ld | %ld\n", 
					    mp->sender, mp->port, 
					    mp->type);
					mp = mp->next;
				}
			}
			else
				fprintf(stderr,"        |      |\n");
		}
	}
}

/************************************************************************/

long	padstr(

/* Truncates or pads string (with blanks) to specified length		*/

	char	*s1,				/* target string	*/
	const	char	*s2,			/* source string	*/
	long	n				/* string length	*/
)
{
	static char	
           blanks[]="                                                     ";

	strncpy(s1, s2, n);
	if(n -= strlen(s1))  strncat(s1, blanks, n);
	return(strlen(s1));
}

/************************************************************************/

LOCAL 	void 	soft_report(void)

/* Reports software status.						*/

{
	long	task;				/* task index		*/
	ps_task_t	*tp;			/* task pointer		*/
	char	pstring[TEMP_STR_SIZE];			/* prlong string		*/
	long	port;				/* port index		*/
	long	sport;				/* saved port index	*/
	ps_port_t	*pp;			/* port pointer		*/
	ps_mess_t	*mp;			/* message pointer	*/
	long	no_mess;			/* no message flag	*/

	fprintf(stderr, "\n\n               S O F T W A R E   S T A T U S \n\n");
	fprintf(stderr, 
"                      Task                    ||Port|| Messages Queued\n");
	fprintf(stderr, 
" ID |        Name          | Node |   State   || ID || Sender | Type\n");
	fprintf(stderr,
"-------------------------------------------------------------------------\n");
	for(task = 0; task < ps_task_tab.tab_size; task++) {
		if((tp = ps_task_ptr(task))->state != TASK_FREE) {
			padstr(pstring, tp->name, 20);
			fprintf(stderr, "%3ld | %s |%4ld  |", task, pstring, 
			    tp->node);
			switch(tp->state) {
			case TASK_READY:
				fprintf(stderr, "   READY   ||");
				break;
			case TASK_RECEIVING:
				fprintf(stderr, " RECEIVING ||");
				break;
			case TASK_SLEEPING:
				fprintf(stderr, "  SLEEPING ||");
				break;
			case TASK_SPINNING:
				fprintf(stderr, "  SPINNING ||");
				break;
			case TASK_SUSPENDED:
				fprintf(stderr, " SUSPENDED ||");
				break;
			default:
				fprintf(stderr, "  RUNNING  ||");
			}
			port = tp->port_list;
			sport = port;
			no_mess = TRUE;
			while(port != NULL_PORT) {
				if((pp = port_ptr(port))->nmess) {
					no_mess = FALSE;
					if(port != sport)
						fprintf(stderr,
	"    |                      |      |           ||");
					fprintf(stderr,"%3ld ||", pid(pp));
					mp = pp->first;
					while(mp != NULL_MESS_PTR) {
						if(mp != pp->first)
							fprintf(stderr,
	"    |                      |      |           ||    ||");
						fprintf(stderr, "%5ld   | %ld\n", 
						    mp->sender, mp->type);
						mp = mp->next;
					}
				}
				else if(no_mess)
					sport = pp->next;
				port = pp->next;
			}
			if(no_mess)
				fprintf(stderr, "    ||        |\n");
		}
	}
}
	
/************************************************************************/

LOCAL	void	set_task_state(

/* Sets the state of the given task to the given state.  Also checks if	*/
/* there was a breakpolong set on that task's changing state and if so	*/
/* calls debug().							*/

	ps_task_t	*tp,
	ps_task_state_t	state
)
{
	if(tp->state == state)
		return;
	tp->state = state;
	if(tp->tbp == -1 || tp->tbp == state) {
		fprintf(stderr, "\n\n---> Task %ld state changed to %s", tid(tp),
		    task_state_names[state]);
		debug();
	}
}

/************************************************************************/

#define STATE_BUFSIZ 50

LOCAL	void	task_breakpoints(void)

/* Allow user to set and delete task state breakpoints.			*/

{
	ps_task_t	*tp;			/* task ptr.		*/
	long		i, j;			/* loop indices		*/
	long		tid;			/* task id		*/
	char		state[STATE_BUFSIZ];		/* state to break on	*/

	fprintf(stderr, "\n---> Task id: ");
	fgets(state, STATE_BUFSIZ, stdin);
	sscanf(state, "%ld", &tid);
	if (tid < 0 || tid > ps_task_tab.tab_size 
	    || (tp = ps_task_ptr(tid))->state == TASK_FREE) {
		fprintf(stderr, "\nInvalid Task ID");
		return;
	}
	fprintf(stderr, "---> Break when state changes to: ");
	fgets(state, STATE_BUFSIZ, stdin );
	for(i = 0; isspace(state[i]); i++);
	if(i != 0) {
 		for(j = 0; state[i+j]; j++)
			state[j] = state[i+j];
		state[j] = '\0';
	}
	for(i = 0; state[i]; i++) {
		if(isspace(state[i])) {
			state[i] = '\0';
			break;
		}
	}
	if(strlen(state) == 0) {
		if(tp->tbp != -2) {
			fprintf(stderr, "\n---> Task Breakpolong removed");
			tp->tbp = -2;
			ntask_bps--;
		}
		return;
	}
	for(i = 0; task_state_names[i]; i++)
		if(!strcasecmp(task_state_names[i], state))
			break;
	if(!task_state_names[i]) {
		if(!strcasecmp("ANYSTATE", state)) {
			if(tp->tbp == -2)
				ntask_bps++;
			tp->tbp = -1;
			return;
		}
		fprintf(stderr, "\n---> Invalid task state \"%s\".  ", state);
		fprintf(stderr, "State must be one of:");
		for(i = 0; task_state_names[i]; i++)
			fprintf(stderr, "\n---> %s", task_state_names[i]);
		fprintf(stderr, "\n---> ANYSTATE (Any change of state)");
		fprintf(stderr, "\n---> <Return> (Clears Task breakpoint)");
 		return;
	}
	if(tp->tbp == -2) 
		ntask_bps++;
	tp->tbp = i;
}

/************************************************************************/

void	ts_report(

/* PARASOL trace state reporter.					*/

	ps_task_t	*tp,			/* task pointer		*/
	const	char	*sp			/* string pointer	*/
)
{
/* WCS - 30 Aug 1997 - Added.  Should be made more efficient. */
#if	!HAVE_SIGALTSTACK || _WIN32 || _WIN64
	long headroom;
	char string[TEMP_STR_SIZE];  /* is this long enough for any string encounterable? */
	

       	if (w_flag && (ps_htp != DRIVER_PTR))
		if ((headroom = (ps_headroom() - ps_trsct)) < 
		    10*sizeof(double)*sp_delta) {  /* what value to use here ? */
			sprintf(string, "ps_headroom reports %ld for task %ld", 
			    headroom, ps_myself);
			warning(string);
		}
#endif
	
	if (tp->code == reaper || tp->code == port_set_surrogate 
	    || tp->code == shared_port_dispatcher 
	    || tp->code == block_stats_collector )
		return;
	if(strlen(tp->name))
		fprintf(stderr, "\nTime: %.8G; Node: %ld; Task %ld (%s) %s.", 
		    ps_now, tp->node, tid(tp), tp->name, sp);
	else
		fprintf(stderr, "\nTime: %.8G; Node: %ld; Task %ld %s.",
		    ps_now, tp->node, tid(tp), sp);
}
		
/************************************************************************/
/*		 Angio Trace Support Functions				*/
/************************************************************************/

LOCAL	long	create_dye (

/* Creates a new dye with the given base name				*/

	const	char	*base_name		/* dye name		*/
)
{
	long	did;
	ps_dye_t *dye;

	if ((did = get_table_entry(&ps_dye_tab)) == SYSERR)
		ps_abort("Angio Trace Internal Error #1");

	dye = dye_ptr(did);
	dye->occurence = get_occurence_number (base_name, &dye->base_name);
	dye->serialno = async_count++;

	return did;
}

/************************************************************************/

LOCAL	long	derive_dye (

/* Creates a new dye which is a derivative of the original dye, with	*/
/* the same base name and occurence number, but a new serial number.	*/

	long		did
)
{
	ps_dye_t *old_dye;
	ps_dye_t *new_dye;
	long	newid;

	if ((newid = get_table_entry(&ps_dye_tab)) == SYSERR)
		ps_abort("Angio Trace Internal Error #2");

	old_dye = dye_ptr(did);
	new_dye = dye_ptr(newid);

	new_dye->base_name = old_dye->base_name;
	new_dye->occurence = old_dye->occurence;
	new_dye->serialno = async_count++;

	return newid;
}

/************************************************************************/

LOCAL	void 	free_dye (

/* Frees the memory associated with the given dye.			*/

	long		did
)
{
	if (free_table_entry(&ps_dye_tab, did) == SYSERR)
		ps_abort("Angio Trace Internal Error #3");
}

/************************************************************************/

LOCAL 	long	get_occurence_number (

/* Returns a unique occurence number for the given base name by keeping	*/
/* track of all base names along with their occurence numbers. 		*/
/* new_base_name is filled in with a pointer to the shared occurence 	*/
/* name.								*/

	const	char	*base_name, 
	char 		**new_base_name
)
{
	typedef struct name_entry_t {
		char *name;
		long occurence;	
		struct name_entry_t *next;
	} name_entry_t;

	static name_entry_t	*list = NULL; 
	name_entry_t 		*temp;
	long			result;

	temp = list;
	result = 1;
	while (temp != NULL && (result = strcmp(temp->name,base_name)) != 0)
		temp = temp->next;

	if (result == 0)  {	/* Found it */
		*new_base_name = temp->name;
		temp->occurence++;
		return temp->occurence;
	}

	if ((temp = (name_entry_t *)malloc (sizeof(name_entry_t))) == NULL
	    || (temp->name = (char *)malloc(strlen(base_name)+1)) == NULL ) 
		ps_abort("Insufficient Memory");
	
	*new_base_name = temp->name;
	strcpy (temp->name, base_name);
	temp->occurence = 0;
	temp->next = list;
	list = temp;

	return 0;
}

/************************************************************************/

LOCAL	void	start_trace (

/* Starts an angio trace by assigning the given dye to the given task 	*/
/* and logging a wBegin event.						*/

	ps_task_t	*tp,
	long		did
)
{
	tp->did = did;
	log_angio_event (tp, dye_ptr(tp->did), "wBegin");
}

/************************************************************************/

LOCAL	void	end_trace (

/* Ends a single angio trace by logging a wEnd event and freeing the 	*/
/* dye.									*/

	ps_task_t	*tp
)
{
	log_angio_event (tp, dye_ptr(tp->did), "wEnd");
	free_dye (tp->did);
	tp->did = -1;
}

/************************************************************************/

LOCAL	void	log_angio_event (

/* Logs an angio event to the angio trace output file (angio_file).	*/

	ps_task_t	*tp,
	ps_dye_t	*dye,
	const	char	*event
)
{
	if (!angio_flag) 
		ps_abort("Angio tracing internal error");

	if (!angio_output)
		return;

	if (tp->code == reaper || tp->code == port_set_surrogate 
	    || tp->code == shared_port_dispatcher)
		return;

	fprintf (angio_file, "\nW %s %ld %ld ! T %s %ld ! %g ! E %s", 
	    dye->base_name, dye->occurence, dye->serialno, tp->name, tp->tsn, 
	    ps_now, event);
}

/************************************************************************/	
/*		Glocal variable related SYSCALLS			*/
/************************************************************************/

LOCAL	void	glocal_init_stuff (

/*	Initializes the ps_env_tab structure and grow the table if 	*/
/*	necessary.							*/

	long		env			/* environment id	*/
)
{
	long		i;
	static	long	init = FALSE;
	ps_env_t	*ep;

/*	Initialize table if not done yet.				*/

	if (!init) {
		init_table (&ps_env_tab, DEFAULT_MAX_ENVS, sizeof(ps_env_t));
		init = TRUE;
	}

/*	Grow table if to accomodate env if necessary			*/

	while(env >= ps_env_tab.tab_size) {
		ps_env_tab.used = ps_env_tab.tab_size;
		get_table_entry(&ps_env_tab);
		for(i = ps_env_tab.tab_size/2; i < ps_env_tab.tab_size; i++) {
			ep =  env_ptr(i);
			ep->ntasks = 0;
			ep->nsubscribers = 0;
			ep->nallocated = 0;
		}
	} 

/*	Initialize the variable table if necessary			*/

	if (!(ep = env_ptr(env))->used) {
		init_table (&ep->var_tab, DEFAULT_MAX_VARS, sizeof(ps_var_t));
		ep->used = TRUE;
	}
}

/************************************************************************/
/*		Miscellaneous PARASOL Support Functions			*/
/************************************************************************/


LOCAL	void	adjust_priority(

/* Alters the sheduling priority of a specified "task". No error 	*/
/* checking is performed, so priority may be > MAX_PRIORITY or <	*/
/* MIN_PRIORITY. This call may cause instantaneous task resheduling.	*/

	long	task,				/* task id		*/
	long	priority			/* new priority		*/
)
{
	ps_task_t	*tp;			/* task pointer		*/
	ps_node_t	*np;			/* node pointer		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/

	if ((tp = ps_task_ptr(task))->priority == priority)
		return;

 	tp->priority = priority;
	np = node_ptr(tp->node);
	if(ts_flag) {
		sprintf(string, "priority adjusted to %ld", priority);
		ts_report(tp, string);
	}
	switch (tp->state) {

	case TASK_READY:

		if(np->discipline != PS_FIFO) {
			dq_ready(np, tp);
			find_host(tp);
		}
		break;

	case TASK_HOT:
	case TASK_BLOCKED:
	case TASK_COMPUTING:
	case TASK_SPINNING:
		
		if(np->discipline == PS_PR) 
			find_priority(np, tp->hp, priority);
		break;
	}								
 }

/************************************************************************/

LOCAL	long	ancestor(				

/* Tests if the caller is an ancestor of the given task.  Returns TRUE	*/
/* if the simulation driver is the caller or if the caller is the given	*/
/* task.								*/

	ps_task_t	*tp			/* task pointer		*/
)
{
	if (ps_htp == DRIVER_PTR)
		return TRUE;

	while(TRUE) {
		if (ps_htp == tp)
			return TRUE;
		if (tp->parent == DRIVER)
			return FALSE;
		tp = ps_task_ptr(tp->parent);
	}
}

/************************************************************************/

extern	int	bad_param_helper(

/* Prints a message saying function was called with an invalid value	*/
/* for param.								*/

	const char *function, 			/* function name	*/
	const char *param			/* parameter name	*/
)
{
	char	string[100];			/* warning message	*/

	if(w_flag && (ps_htp->priority <= MAX_PRIORITY)) {
		sprintf(string, "%s: Invalid '%s' parameter.", function, param);
		warning(string);
	}
	return(SYSERR);
}

/************************************************************************/

extern int	bad_call_helper(

/* Prints a message saying that something was wrong in a call to a	*/
/* function.								*/

	const char *function,			/* function name	*/
	const char *message			/* error message	*/
)
{
	char	string[100];			/* warning message	*/

	if(w_flag && (ps_htp->priority <= MAX_PRIORITY)) {
		sprintf(string, "%s: %s.", function, message);
		warning(string);
	}
	return(SYSERR);
}

/************************************************************************/

LOCAL	void 	dq_lock(

/* Dequeues the task pointed to by "tp" from the lock on which it is	*/
/* spinning.								*/

	ps_task_t	*tp			/* task pointer		*/
)
{
	ps_lock_t	*lp;			/* lock pointer		*/
	ps_task_t	*btp, *ctp;		/* task pointers	*/

	if(tp->spin_lock != NULL_LOCK) {
		lp = lock_ptr(tp->spin_lock);
		btp = ctp = ps_task_ptr(lp->queue);
		while(ctp != tp) {
			btp = ctp;
			ctp = ps_task_ptr(ctp->next);
		}
		if(ctp == btp)
			lp->queue = ctp->next;
		else
			btp->next = ctp->next;
		lp->count--;
		tp->spin_lock = NULL_LOCK;
	}
}


/************************************************************************/

LOCAL	void	free_mess(

/* Free message envelope.						*/

	ps_mess_t	*mp			/* message pointer	*/
)
{
	mp->next = next_mess;
	next_mess = mp;
}

/************************************************************************/

LOCAL void free_pair(

/* Stuffs a freed tp pair onto the tpflist.				*/

	ps_tp_pair_t	*pp			/* tp pair pointer	*/
)
{	
	pp->next = tpflist;
	tpflist = pp;
}

/************************************************************************/

LOCAL	long	get_dss(
/* Returns a reasonable (I hope) default stack size for the machine we 	*/
/* are running on.  Stack size is truncated so that it is a multiple of	*/
/* sizeof(double) in order to avoid alignment problems.			*/

	long	t_flag
)
{
	long	dss;								

	dss = 15*sizeof(double)*sp_delta;
	if (t_flag) {
		ps_trsct = stack_corruption_tester (trace_rep);
		dss += ps_trsct;
	} else
		dss += stack_corruption_tester (no_trace_rep);

 	return (dss - dss % sizeof(double));
}

/************************************************************************/

LOCAL	ps_mess_t	*get_mess(void)

/* Get message envelope							*/

{
	long	i;				/* loop index		*/
	ps_mess_t	*mp, *nmp;		/* message pointers	*/

	if(next_mess == NULL_MESS_PTR) {
		if(!(mp = next_mess = (ps_mess_t *) malloc(100*sizeof(ps_mess_t))))
			ps_abort("Insufficient memory");
		for(i = 1; i < 100; i++) {
			nmp = mp++;
			nmp->next = mp;
		}
		mp->next = NULL_MESS_PTR;
	}
	mp = next_mess;
	next_mess = next_mess->next;
	return(mp);
}

/************************************************************************/

LOCAL 	ps_tp_pair_t *get_pair(void)

/* Returns a pointer to a tp pair using tpflist if possible		*/

{
	ps_tp_pair_t	*pp;			/* tp pair pointer	*/

	if(tpflist) {
		pp = tpflist;
		tpflist = pp->next;
	}
	else 
		if(!(pp = (ps_tp_pair_t *) malloc(sizeof(ps_tp_pair_t))))
			ps_abort("Insufficient memory");
	return(pp);
}

/************************************************************************/

LOCAL	void	init_locks(void)

/* Initializes the lock table.						*/

{
	long	i;				/* loop index		*/
	ps_lock_t	*lp;			/* lock pointer		*/

	init_table(&ps_lock_tab, DEFAULT_MAX_LOCKS, sizeof(ps_lock_t));
	for( i = 0; i < DEFAULT_MAX_LOCKS; i++) {
		lp = lock_ptr(i);
		lp->state = UNLOCKED;
		lp->owner = NULL_TASK;
		lp->queue = NULL_TASK;
		lp->count = 0;
	}
}

/************************************************************************/

LOCAL	void	init_semaphores(void)

/* Initializes the semaphore table.					*/

{
	long	i;				/* loop index		*/
	ps_sema_t	*sp;			/* semaphore pointer	*/

	init_table(&ps_sema_tab, DEFAULT_MAX_SEMAPHORES, sizeof(ps_sema_t));
	for( i = 0; i < DEFAULT_MAX_SEMAPHORES; i++) {
		sp = sema_ptr(i);
		sp->count = 1;
		sp->queue = NULL_PAIR_PTR;
	}
}

/************************************************************************/

LOCAL	long 	lookup_port_set(

/* Finds the port set belonging to the given task which uses the	*/
/* given surrogate.							*/

	long 	task,
	long 	surrogate
)
{
	ps_tp_pair_t *pairp;
	long port;
	long port_set = NULL_PORT;
	
	port = ps_task_ptr(task)->port_list;
	while (port_set == NULL_PORT && port != NULL_PORT) {
		if (port_ptr(port)->state == PORT_SET) {
			pairp = port_ptr(port)->tplist;
			while (port_set == NULL_PORT && pairp) {
				if (pairp->task == surrogate)
					port_set = port;
				pairp = pairp->next;
			}
		}
		port = port_ptr(port)->next;
	}
	return port_set;
}

/************************************************************************/

LOCAL	void	no_trace_rep(void)

/* This is representative of an application without tracing on		*/

{
	static char 	dummy[40];

	sprintf(dummy, "****%g:%d-Initializing-%d:%d****", 1.234926, 15, 9987, 
	    3456);
}

/************************************************************************/

LOCAL	long	other_err_helper(

/* Prints a message saying that something was wrong in a call to a	*/
/* function.								*/

	const char *function,			/* function name	*/
	const char *message			/* error message	*/
)
{
	char	string[100];			/* warning message	*/

	if(w_flag && (ps_htp->priority <= MAX_PRIORITY)) {
		sprintf(string, "%s: An error occured while %s.", function,
		    message);
		warning(string);
	}
	return(SYSERR);
}

/************************************************************************/

LOCAL	long	port_receive(

/* Receives a message according to port queueing discipline - i.e., 	*/
/* FCFS, PS_LIFO, or RAND.							*/

	long	discipline,			/* queueing discipline	*/
	long	port,				/* port id index	*/
	double	time_out,			/* receive timeout	*/
	long	*typep,				/* type pointer		*/
	double	*tsp,				/* time stamp pointer	*/
	char	**texth,			/* text handle		*/
	long	*app,				/* acknowledge port ptr	*/
	long	*opp,				/* original port ptr	*/
	long	*mid,				/* message id ptr	*/
	long	*did				/* dye id		*/
)
{
	ps_port_t	*pp;			/* port pointer		*/
	ps_node_t	*np;			/* node pointer		*/
	ps_cpu_t	*hp;			/* host cpu pointer	*/
	ps_mess_t	*mp, *op;		/* message pointers	*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/
	long	skip;				/* skip count		*/
	long	max_pri;
	long	pri_skip;

	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state == PORT_FREE)
		return(BAD_PARAM("port"));
	if(pp->owner != ps_myself)
		return(BAD_CALL("Caller is not owner of port"));

	if(!pp->nmess && time_out < 0.0) {
		*typep = ACK_TIMEOUT;
		if(ts_flag) {
			sprintf(string, "times out on port %ld", port);
			ts_report(ps_htp, string);
		}
		return(FALSE);
	}
	if(!pp->nmess && time_out != NEVER) 
		ps_htp->rtoep = add_event(ps_now + time_out, END_RECEIVE, 
		    (long *)ps_myself);

	if(!pp->nmess) { 	/* begin waiting for message		*/	
		if(ts_flag) {
			sprintf(string, "receiving (blocked) on port %ld", 
		 	   port);
			ts_report(ps_htp, string);
		}
		do {		/* do/while required as a waiting task 
					might be suspended temporarily	*/
			np = node_ptr(ps_htp->node);
			hp = ps_htp->hp;
			SET_TASK_STATE(ps_htp, TASK_RECEIVING);
			remove_event(ps_htp->qep);
			ps_htp->qep = NULL_EVENT_PTR;
			ps_htp->wport = port;

			if (np->discipline == PS_CFS){
				dq_cfs_task(ps_htp);
				cooling_cfs_task(ps_htp);
			}

				find_ready(np, hp);
			if((pp = port_ptr(port))->owner != ps_myself)
				return(BAD_CALL("Caller is not owner of port"));

		} while(!pp->nmess);
		remove_event(ps_htp->rtoep);
		ps_htp->rtoep = NULL_EVENT_PTR;

	}

	switch(discipline) {
	    case PS_RAND:
		skip = ps_choice(pp->nmess);
		break;
	    case PS_LIFO:
		skip = pp->nmess -1;
		break;
	    case PS_HOL:
		max_pri = 0;	/* Search for highest priority message */
		skip = 0;
		for ( mp = pp->first, pri_skip = 0; mp; mp = mp->next, pri_skip++ ) {
			if ( mp->pri > max_pri ) {
				max_pri = mp->pri;
				skip = pri_skip;
			}
		}
	        break;
		    
	    case PS_FIFO:
	    default:
		skip = 0;
		break;
	}
	op = mp = pp->first;
	if(!skip) 
		pp->first = mp->next;
	else {
		for(--skip; skip > 0; skip--) 
			op = op->next;
		mp = op->next;
		op->next = mp->next;
	}
	if(pp->last == mp)
		pp->last = op;	
	if(!--(pp->nmess))
		pp->last = NULL_MESS_PTR;

	*typep = mp->type;
	*tsp = mp->ts;
	*texth = mp->text;
	*app = mp->ack_port;
	*opp = mp->org_port;
	*mid = mp->mid;
	if (angio_flag)
		*did = mp->did;
	free_mess(mp);
	if(ts_flag) {
		if(mp->type == ACK_TIMEOUT)
			sprintf(string, "times out on port %ld", port);
		else if (port == ps_htp->blind_port)
			sprintf(string, "receives message %ld on shared port %ld",
			    *mid, *opp); 
		else if (pp->state == PORT_SET)
			sprintf(string, "receives message %ld on port %ld", 
			    *mid, *opp);
		else
			sprintf(string, "receives message %ld on port %ld", 
				*mid, port);
		ts_report(ps_htp, string);
	}
	return ((mp->type == ACK_TIMEOUT) ? FALSE : OK);
}
		
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
	long	mid,          			/* Unique message id	*/   
	long	did				/* dye id		*/
)
{
	ps_mess_t	*mp;			/* message pointer	*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/
	ps_port_t	*pp;			/* port pointer		*/
	ps_task_t	*tp;			/* task pointer		*/

	if(port < 0 || port >= ps_port_tab.tab_size)
		return(BAD_PARAM("port"));
	if((pp = port_ptr(port))->state == PORT_FREE)
		return(BAD_PARAM("port"));

	mp = get_mess();
	if((pp = port_ptr(port))->nmess++) {
		pp->last->next = mp;
		pp->last = mp;
	}
	else 
		pp->first = pp->last = mp;

	mp->sender = ps_myself;
	mp->port = port;
	mp->c_code = MAGIC;
	mp->type = type;
	mp->ts = ts;
	mp->text = text;
	mp->ack_port = ack_port;
	mp->org_port = oport;
	mp->mid = mid;
	if (angio_flag)
		mp->did = did;
	mp->next = NULL_MESS_PTR;
	if(ts_flag && ps_htp != DRIVER_PTR) {
		if (pp->state == PORT_SHARED) {
			sprintf(string, "sending message %ld to shared port %ld",
			    mp->mid, port);
			ts_report(ps_htp, string);
		}
		else if (ps_task_ptr(pp->owner)->code == port_set_surrogate) {
			sprintf(string, "sending message %ld to task %ld via port %ld",
			    mp->mid, ps_task_ptr(pp->owner)->parent, port);
			ts_report(ps_htp, string);
		}
		else  if (pp->owner != 0) {
			sprintf(string, "sending message %ld to task %ld via port %ld",
			    mp->mid, pp->owner, port);
			ts_report(ps_htp, string);
		}
	}
	tp = ps_task_ptr(pp->owner);
	if((tp->state == TASK_RECEIVING) && (tp->wport == port)) {
		remove_event(tp->rtoep);
		tp->rtoep = NULL_EVENT_PTR;
		tp->wport = NULL_PORT;
		find_host(tp);
	}
	return(OK);
}

/************************************************************************/

LOCAL	long	relative(

/* Tests if given task is a relative of the target task.  A relative is	*/
/* defined here as either an ancestor, a sibling, or a sibling of an	*/
/* ancestor.								*/

	ps_task_t	*tp,			/* given task pointer	*/
	ps_task_t	*ttp			/* target task pointer	*/
)
{
	while(TRUE) {
		if(tp == ttp)
			return(TRUE);
		if((tp->son != NULL_TASK) && 
		    relative(ps_task_ptr(tp->son), ttp))
			return(TRUE);
		if(tp->sibling == NULL_TASK)
			return(FALSE);
		tp = ps_task_ptr (tp->sibling);
	}
}

/************************************************************************/

LOCAL	void 	release_locks(

/* Releases the locks held by the specified task (if a descendant).    	*/

	ps_task_t	*tp			/* task pointer		*/
)
{
	long	lock;				/* lock index		*/
	ps_lock_t	*lp;			/* lock pointer		*/
	ps_task_t	*ctp;			/* current task pointer	*/
	long	i,n;				/* loop indices		*/
	char	string[TEMP_STR_SIZE];		/* temp string		*/

	if(!ancestor(tp))
		return;

	lock = tp->lock_list;
	while(lock != NULL_LOCK) {
		lp = lock_ptr(lock);
		if(ts_flag) {
			sprintf(string, "unlocking lock %ld", lock);
			ts_report(ps_htp, string);
		}
		if(lp->queue == NULL_TASK) {
			lp->owner = NULL_TASK;
			lp->state = UNLOCKED;
		}
		else {
			ctp = ps_task_ptr(lp->queue);
			n = ps_choice(lp->count);
			for(i = 0; i < n; i++) 
				ctp = ps_task_ptr(ctp->next);
			lp->owner = tid(ctp);
			dq_lock(ctp);
			lp->next = ctp->lock_list;
			ctp->lock_list = lock;
			SET_TASK_STATE(ctp, TASK_BLOCKED);
			ctp->tep = add_event(ps_now, END_BLOCK, 
			    (long *)lp->owner);
			if(ts_flag) {
				sprintf(string, "locking lock %ld", lock);
				ts_report(ctp, string);
			}
		}

		lock = lp->next;
	}
	tp->lock_list = NULL_LOCK;
}

/************************************************************************/

LOCAL 	void 	release_ports(

/* Releases all ports owned by the task pointed to by "tp". 		*/

	ps_task_t	*tp			/* task pointer		*/
)
{
	long	port;				/* port index		*/

	port = tp->port_list;
	while(port != NULL_PORT) {
		if ( port_ptr(port)->state != PORT_SHARED ) {
			ps_release_port(port);	/**/
		}
		port = port_ptr(port)->next;
	}	
	tp->port_list = NULL_PORT;
}

/************************************************************************/

LOCAL	void	set_run_task(

/* Sets the run_task member of hp to the task id of the task specified	*/
/* by tp (hp->run_task = tid(tp)). The rest of the code is for 		*/
/* gathering statistics on the usage of each CPU on a per task basis.	*/

	ps_node_t	*np,
	ps_cpu_t	*hp,
 	ps_task_t	*tp
)
{
	long		*stat;			/* ptr to stat index	*/
	long		i;			/* loop variable	*/
	long		otask;			/* original task	*/
	long		task;			/* task id		*/
	char		string[TEMP_STR_SIZE];	/* stat name buffer	*/
	ps_group_t	*gp,*ogp;
	ps_task_t 	*otp;

	task = (tp == NULL_TASK_PTR) ? NULL_TASK : tid(tp);
 	otask = hp->run_task;
	hp->run_task = task;


	if (np->sf & SF_PER_TASK_HOST) {
		if (otask != NULL_TASK)	
			/* WCS - 14 April 1998 - Added test for -1 (for port_set_surrogate) */
			if (*(stat = ts_stat_ptr(hp->ts_tab,otask)) != -1)
				ps_record_stat(*stat, 0.0);
		if (task != NULL_TASK) {
			while (task >= hp->ts_tab->tab_size) {
				hp->ts_tab->used = hp->ts_tab->tab_size;
				get_table_entry(hp->ts_tab);
				for(i = hp->ts_tab->tab_size/2; 
				    i < hp->ts_tab->tab_size; i++)
					*ts_stat_ptr(hp->ts_tab, i) = -1;
			}
/* WCS - 6 May 1998 - Added test for port_set_surrogate.  Others ??? */
			if (tp->code != port_set_surrogate) {
	 			stat = ts_stat_ptr(hp->ts_tab, task);
				if (*stat == -1) {
					sprintf(string, "%s (cpu %ld) task %ld Utilization", 
					   np->name, hid(np,hp), task);
					*stat = ps_open_stat(string, VARIABLE);

/* 	Don't try this at home, we adjust the stat we created to take 	*/
/*	into account the difference in time between creation of	the 	*/
/*	node and the first time this task executes on the node.		*/

					stat_ptr(*stat)->values.var.start 
					    = stat_ptr(*stat)->values.var.old_time 
					    = np->build_time;
				}
				ps_record_stat (*stat, 1.0);
			}
		}
	}
	if (np->sf & SF_PER_TASK_NODE) {
		if (otask != NULL_TASK)	
			/* WCS - 14 April 1998 - Added test for -1 (for port_set_surrogate) */
			if (*(stat = ts_stat_ptr(np->ts_tab,otask)) != -1)
				ps_record_stat(*stat, 0.0);

		if (task != NULL_TASK) {
			while (task >= np->ts_tab->tab_size) {
				np->ts_tab->used = np->ts_tab->tab_size;
				get_table_entry(np->ts_tab);
				for(i = np->ts_tab->tab_size/2; 
				    i < np->ts_tab->tab_size; i++)
					*ts_stat_ptr(np->ts_tab, i) = -1;
			}
/* WCS - 6 May 1998 - Added test for port_set_surrogate.  Others ??? */
			if (tp->code != port_set_surrogate) {
	 			stat = ts_stat_ptr(np->ts_tab, task);
				if (*stat == -1) {
					sprintf(string,
					   "%s task %ld Utilization",
					   np->name, task);
					*stat = ps_open_stat(string, VARIABLE);

/* 	Don't try this at home, we adjust the stat we created to take 	*/
/*	into account the difference in time between creation of	the 	*/
/*	node and the first time this task executes on the node.		*/

					stat_ptr(*stat)->values.var.start 
					    = stat_ptr(*stat)->values.var.old_time 
					    = np->build_time;
				}
				ps_record_stat (*stat, 1.0);
			}
		}
	}

	/* collect  stat for groups */
	if( (task!=NULL_TASK) && (tp->group_id >=0)){
		gp=group_ptr(tp->group_id);
		while (task >= gp->ts_tab->tab_size) {
			gp->ts_tab->used = gp->ts_tab->tab_size;
			get_table_entry(gp->ts_tab);
			for(i = gp->ts_tab->tab_size/2; 
			    i < gp->ts_tab->tab_size; i++)
				*ts_stat_ptr(gp->ts_tab, i) = -1;
		}
		if (tp->code != port_set_surrogate) {
	 			stat = ts_stat_ptr(gp->ts_tab, task);
				if (*stat == -1) {
					sprintf(string,
					   "%s task %ld Utilization",
					   gp->name, task);
					*stat = ps_open_stat(string, VARIABLE);

/* 	Don't try this at home, we adjust the stat we created to take 	*/
/*	into account the difference in time between creation of	the 	*/
/*	node and the first time this task executes on the node.		*/

					stat_ptr(*stat)->values.var.start 
					    = stat_ptr(*stat)->values.var.old_time 
					    = np->build_time;
				}
				ps_record_stat (*stat, 1.0);
			}
	}
	if (otask !=NULL_TASK) {
		otp=ps_task_ptr(otask);
		if(otp->group_id >=0){
			ogp=group_ptr(otp->group_id);
			if (*(stat = ts_stat_ptr(ogp->ts_tab,otask)) != -1)
				ps_record_stat(*stat, 0.0);
		}
	}

}

/************************************************************************/

LOCAL	long	sp_tester(

/* Determines direction of stack growth and jmp-`_buf index.		*/

	long	*sp_dirp,			/* sp direction pointer	*/
	long	*sp_indp,			/* sp index pointer	*/
	long	*sp_deltap			/* sp delta pointer	*/
)
{
#if !HAVE_SIGALTSTACK || _WIN32 || _WIN64
	long	i;				/* loop index		*/
	long	istop;				/* loop index stopper	*/
	long	*jbp[3];			/* jmp_buf ptr array	*/

	for(i = 0; i < 3; i++) 
		jbp[i] = (long *) &sp_jb[i];
	istop = sizeof(jmp_buf)/sizeof(long);
	
	sp_winder(2);

	if(sp_yadd[2] > sp_yadd[1])
		*sp_dirp = -1;
	else
		*sp_dirp = 1;
	*sp_deltap = (sp_yadd[1] - sp_yadd[2])*(*sp_dirp);
	for(i = 0; i < istop; i++, jbp[0]++, jbp[1]++, jbp[2]++) 
		if(((*jbp[2]-*jbp[1])==(*jbp[1]-*jbp[0]))
		    &&	((*jbp[2]-*jbp[1])*(*sp_dirp) < 0)
		    &&	((*jbp[1]-sp_yadd[1])*(*sp_dirp) > 0))
			break;
	if(i < istop) {
		*sp_indp = i;
		return(OK);
	}
	else
		return(SYSERR);
#else
	return sp_winder();
#endif
}

/************************************************************************/

#if !HAVE_SIGALTSTACK || _WIN32 || _WIN64
LOCAL	void	sp_winder(

/* Winds stack for sp_tester.						*/

	long	x				/* recursion index	*/
)
{
	long	y;				/* stack marker		*/

#if	_WIN32 || _WIN64
	setjmp(sp_jb[x]);
#else
	_setjmp(sp_jb[x]);
#endif
	sp_yadd[x] = (size_t) &y;
	if(x)
		sp_winder(x-1);
}
#else
LOCAL	long sp_winder()
{
    static char *addr = 0;
    auto char dummy;
    if (addr == 0) {
	addr = &dummy;
	return sp_winder();
    } else {
	return (&dummy > addr) ? 1 : -1;
    }
}
#endif

/************************************************************************/

LOCAL	int	stat_compare(

/* This is a callback function that is passed in to quicksort for 	*/
/* sorting the statistics in alphabetical order by name.		*/

	ps_stat_t 	*s1, 
	ps_stat_t 	*s2
)
{
	return strcmp(s1->name, s2->name);
}

/************************************************************************/

#ifdef STACK_TESTING

void	test_all_stacks(void)

/* 	Prints a summary of stack info					*/

{
	long		task;			/* task id		*/
 
	printf ("\nTesting stacks, this may take a while...");

	for (task = 0; task < ps_task_tab.tab_size; task++)
		if (ps_task_ptr(task)->state != TASK_FREE)
			test_stack(task);

	printf ("\nsp_delta = %ld", sp_delta);
 	printf ("\nsp_dss = %ld", sp_dss);
	printf ("\nmax_stack = %ld\n", max_stack);
}

/************************************************************************/

LOCAL	void	test_stack(

/* Tests the amount of stack space used on all stacks			*/

	long	task				/* task id		*/
)
{
	ps_task_t	*tp;			/* task ptr		*/
	char 		*stack;			/* stack base		*/
	long		stacksize;		/* stack size in bytes	*/
	long		i;			/* loop index		*/
	long		bytes;			/* amount of stack used	*/

	tp = ps_task_ptr(task);
	stack = (char*)tp->stack_base;
	stacksize = ((char*)tp->stack_limit) - stack;

	if (sp_dir > 0) {			/* start at the top	*/
		for (i = stacksize - 1; i >= 0; i--)
			if (stack[i] != 0x55) break;
		bytes = i;
	}
	else {
		for (i = 0; i < stacksize; i++)	/* start at the bottom	*/
			if (stack[i] != 0x55) break;
		bytes = stacksize - i;
	}
	max_stack = max_stack < bytes ? bytes : max_stack;
}

#endif /*STACK_TESTING*/

/************************************************************************/

LOCAL	void	trace_rep(void)

/* This is representative of the PARASOL tracing function ts_report	*/
 
{
	fprintf(stderr, "****%g:%d-Initializing-%d:%d****", 1.234926, 15, 9987, 
	    3456);
}

/************************************************************************/

void	warning(

/* Writes PARASOL warning to stderr					*/

	const	char	*string			/* warning string	*/
)
{
/* WCS - 3 March 1999 - Write current simulation time.	*/

	fprintf(stderr, "\n%.8G ***> %s\n", ps_now, string);
}

/************************************************************************/
/*		Parasol Scheduler Support Functions			*/
/************************************************************************/

#if	HAVE_SIGALTSTACK && !_WIN32 && !_WIN64
static mctx_t mctx_caller;
static sig_atomic_t mctx_called;
static mctx_t  *mctx_creat;
static void (*mctx_creat_func)(void *);
static void *mctx_creat_arg;
static sigset_t mctx_creat_sigs;

void mctx_create_trampoline(int sig);
void mctx_create_boot(void);

void mctx_create( mctx_t *mctx, void (*sf_addr)(void *), void *sf_arg, void *sk_addr, size_t sk_size)
{
	struct sigaction sa;
	struct sigaction osa;
/* #if HAVE_STACK_T */
	stack_t ss;
	stack_t oss;
/*
#else
	struct sigaltstack ss;
	struct sigaltstack oss;
#endif
*/
	sigset_t osigs;
	sigset_t sigs;

	/* Step 1: */
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sigs, &osigs);

	/* Step 2: */
	memset((void *)&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = mctx_create_trampoline;
	sa.sa_flags = SA_ONSTACK;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1, &sa, &osa);

	/* Step 3: */
	ss.ss_sp = sk_addr;
	ss.ss_size = sk_size;
	ss.ss_flags = 0;
	sigaltstack(&ss, &oss);

	/* Step 4: */
	mctx_creat = mctx;
	mctx_creat_func = sf_addr;
	mctx_creat_arg = sf_arg;
	mctx_creat_sigs = osigs;
	mctx_called = FALSE;
	kill(getpid(), SIGUSR1);
	sigfillset(&sigs);
	sigdelset(&sigs, SIGUSR1);
	while (!mctx_called) sigsuspend(&sigs);

	/* Step 6: */
	sigaltstack(NULL, &ss);
	ss.ss_flags = SS_DISABLE;
	sigaltstack(&ss, NULL);
	if (!(oss.ss_flags & SS_DISABLE)) sigaltstack(&oss, NULL);
	sigaction(SIGUSR1, &osa, NULL);
	sigprocmask(SIG_SETMASK, &osigs, NULL);

	/* Step 7 & Step 8: */
	ctxsw(&mctx_caller, mctx);

	/* Step 14: */

	return;
}


void mctx_create_trampoline(int sig)
{
	/* Step 5: */
	if (mctx_save(mctx_creat) == 0) {
		mctx_called = TRUE;
		return;
	}

	/* Step 9: */
	mctx_create_boot();
}


void mctx_create_boot(void)
{
	void (*mctx_start_func)(void *);
	void *mctx_start_arg;
	/* Step 10: */
	sigprocmask(SIG_SETMASK, &mctx_creat_sigs, NULL);

	/* Step 11: */
	mctx_start_func = mctx_creat_func;
	mctx_start_arg = mctx_creat_arg;

	/* Step 12 & Step 13: */
	ctxsw(mctx_creat, &mctx_caller);

	/* The thread "magically: starts... */
	mctx_start_func(mctx_start_arg);

	if(ps_myself < 2)
		ps_suspend(ps_myself);
	ps_kill(ps_myself);

	/* NOTREACHED */
	abort();
}
#else
/************************************************************************/

LOCAL	void	wrapper(

/* Wraps PARASOL tasks to handle running off the end			*/

	mctx_t * newc,
	mctx_t * oldc
)
{
	ctxsw( newc, oldc );
	(* (ps_htp->code))( 0 );		/* 0 can be an arg */
	if(ps_myself < 2)
		ps_suspend(ps_myself);
	ps_kill(ps_myself);
	exit(1);				/* Shouldn't happen	*/
}

#endif
/************************************************************************/
/*		PARASOL Red Black Tree Support Functions		*/
/************************************************************************/

LOCAL struct rb_node * init_rb_node(){
	struct rb_node *n;
	if(!(n=(rb_node *)malloc(sizeof(rb_node)))) 
		ps_abort("Insufficient Memory");

	n->rb_parent=NIL;
	n->rb_left=NIL;
	n->rb_right=NIL;
	n->color=RB_RED;
	n->si=NULL_SCHED_PTR;
	n->key=0.0;
	return n;
}

/************************************************************************/
LOCAL	void  init_cfs_rq(ps_cfs_rq_t * rq,long node,double sched_time,sched_info *si)
{
	rq->root=NIL;
	rq->leftmost=NIL;
	rq->node=node;				
	rq->nready=0;				
	rq->curr=NULL_SCHED_PTR;			
	rq->si=si;
	/* time parameters */
	rq->fair=0.0;				/* fair key*/
	rq->exec_time=0.0;	/*the exec_time*/
	rq->sched_time=sched_time;

}
/************************************************************************/
/* perform rq1=rq2*/
LOCAL void assign_cfs_rq(ps_cfs_rq_t * rq1,ps_cfs_rq_t * rq2){

	rq1->root=rq2->root;
	rq1->leftmost=rq2->leftmost;
	rq1->node=rq2->node;				
	rq1->nready=rq2->nready;				
	rq1->curr=rq2->curr;			
	rq1->si=rq2->si;
	/* time parameters */
	rq1->fair=rq2->fair;				/* fair key*/
	rq1->exec_time=rq2->exec_time;		/*the exec_time*/
	rq1->sched_time=rq2->sched_time;



}
/************************************************************************/

/* node x has two children: left is A and right is y.
   node y has two children: left is B and right is C.
   the result of left rotation:
   node y has two children: left is x and right is C.
   node x has two children: left is A and right is B.
   
            
		  Y   -----right_rotate---->   X
		/   \                        /   \
	       X     C  <----left_rotate--- A     Y
	     /   \                              /   \
	    A     B                            B     C

*/

LOCAL	void  rotate_left( struct ps_cfs_rq_t * rq,struct rb_node * x ) {
	rb_node * y;
	
	y = x->rb_right;/*y is right child of x*/

	/*  y's left sub-tree become x's right sub-tree */
	x->rb_right = y->rb_left;
	if ( y->rb_left != NIL )
		y->rb_left->rb_parent = x;
	/* x's parent become y's  parent   */
	y->rb_parent = x->rb_parent;

	/* if x is at the root */
	if ( x->rb_parent == NIL ) rq->root = y;
	else{
    
		if ( x == (x->rb_parent)->rb_left )
			/* x is on the left child */
			x->rb_parent->rb_left = y;
		else
			/* x is on the right */
			x->rb_parent->rb_right = y;
	}
	/*  put x on y's left */
	y->rb_left = x;
	x->rb_parent = y;

}

/************************************************************************/

/*the  right rotation from Yto X is reverse process of left_rotation.*/

LOCAL	void rotate_right( struct ps_cfs_rq_t *rq, struct rb_node * y ) {
	rb_node * x;

	
	x = y->rb_left;/* x is left child of y */

	/*  x's right sub-tree become y's left sub-tree */
	y->rb_left = x->rb_right;

	if ( x->rb_right != NIL )
		x->rb_right->rb_parent = y;
	/* y's  parent become x's parent   */
	x->rb_parent= y->rb_parent  ;

	/* if y is at the root */
	if ( y->rb_parent ==NIL ) rq->root = x;
	else{
		if ( y == (y->rb_parent)->rb_left )
			/* x is on the left child */
			y->rb_parent->rb_left = x;
		else
			/* x is on the right */
			y->rb_parent->rb_right = x;
	}
	/*  put y on x's right */
	x->rb_right = y;
	y->rb_parent = x;

}
/************************************************************************/


LOCAL void rb_set_data(struct rb_node *n,struct rb_node * p){
	n->si=p->si;
	n->key=p->key;
	n->si->rbnode=n;
}
/************************************************************************/

/* maintain Red Black tree balance after inserting node n */

LOCAL	void insert_rb_color(  struct ps_cfs_rq_t *rq,struct rb_node * n){

	/* check Red-Black properties */
	while (n != rq->root && n->rb_parent->color == RB_RED) {
		/* we have a violation */
		if (n->rb_parent == n->rb_parent->rb_parent->rb_left) {
			rb_node *y = n->rb_parent->rb_parent->rb_right;
			if (y->color == RB_RED) {
 
				/* uncle is RED */
				n->rb_parent->color = RB_BLACK;
				y->color = RB_BLACK;
				n->rb_parent->rb_parent->color = RB_RED;
				n = n->rb_parent->rb_parent;
			} else {
 
				/* uncle is RB_BLACK */
				if (n == n->rb_parent->rb_right) {
					/* make x a left child */
					n = n->rb_parent;
					rotate_left(rq,n);
				}
 
				/* recolor and rotate */
				n->rb_parent->color = RB_BLACK;
				n->rb_parent->rb_parent->color = RB_RED;
				rotate_right(rq,n->rb_parent->rb_parent);
			}
		} else {
 
			/* mirror image of above code */
			rb_node *y = n->rb_parent->rb_parent->rb_left;
			if (y->color == RB_RED) {
 
				/* uncle is RED */
				n->rb_parent->color = RB_BLACK;
				y->color = RB_BLACK;
				n->rb_parent->rb_parent->color = RB_RED;
				n = n->rb_parent->rb_parent;
			} else {
 
				/* uncle is BLACK */
				if (n == n->rb_parent->rb_left) {
					n = n->rb_parent;
					rotate_right(rq,n);
				}
				n->rb_parent->color = RB_BLACK;
				n->rb_parent->rb_parent->color = RB_RED;
				rotate_left(rq,n->rb_parent->rb_parent);
			}
		}
	}
	rq->root->color = RB_BLACK;
}

/************************************************************************/

LOCAL	struct rb_node * insert_rb_node( struct ps_cfs_rq_t *rq,double key){

	struct rb_node *n =  init_rb_node();
	struct rb_node *parent = 0;
	long leftmost=1;
	if (!n){
		return NIL;
	}

	n->key=key;
	if (rq->root==NIL)
		rq->root=n;
		
	else{
		struct rb_node *p=rq->root;
		while(p!=NIL){
			parent=p;
			if (p->key <=key){
				p=p->rb_right;
				leftmost=0;
			}
			else
				p=p->rb_left;
		}
		n->rb_parent=parent;
		if (parent->key>key)
			parent->rb_left=n;
		else 
			parent->rb_right=n;
	}
	if(leftmost) 
		rq->leftmost=n;

	insert_rb_color(  rq, n);
	return n;

}
/************************************************************************/

 LOCAL	void delete_rb_color(

/* maintain Red-Black tree balance after deleting node x */

	ps_cfs_rq_t 	* rq, 
	rb_node   	* x
) 
{
 
	while (x != rq->root && x->color == RB_BLACK) {
		if (x == x->rb_parent->rb_left) {
			rb_node *w = x->rb_parent->rb_right;
			if (w->color == RB_RED) {
				w->color = RB_BLACK;
				x->rb_parent->color = RB_RED;
				rotate_left (rq,x->rb_parent);
				w = x->rb_parent->rb_right;
			}
			if (w->rb_left->color == RB_BLACK && w->rb_right->color == RB_BLACK) {
				w->color = RB_RED;
				x = x->rb_parent;
			} else {
				if (w->rb_right->color == RB_BLACK) {
					w->rb_left->color = RB_BLACK;
					w->color = RB_RED;
					rotate_right (rq,w);
					w = x->rb_parent->rb_right;
				}
				w->color = x->rb_parent->color;
				x->rb_parent->color = RB_BLACK;
				w->rb_right->color = RB_BLACK;
				rotate_left (rq,x->rb_parent);
				x = rq->root;
			}
		} else {
			rb_node *w = x->rb_parent->rb_left;
			if (w->color == RB_RED) {
				w->color = RB_BLACK;
				x->rb_parent->color = RB_RED;
				rotate_right (rq,x->rb_parent);
				w = x->rb_parent->rb_left;
			}
			if (w->rb_right->color == RB_BLACK && w->rb_left->color == RB_BLACK) {
				w->color = RB_RED;
				x = x->rb_parent;
			} else {
				if (w->rb_left->color == RB_BLACK) {
					w->rb_right->color = RB_BLACK;
					w->color = RB_RED;
					rotate_left (rq,w);
					w = x->rb_parent->rb_left;
				}
				w->color = x->rb_parent->color;
				x->rb_parent->color = RB_BLACK;
				w->rb_left->color = RB_BLACK;
				rotate_right (rq,x->rb_parent);
				x = rq->root;
			}
		}
	}
	x->color = RB_BLACK;
}
/************************************************************************/

/*  delete node  from cfs-rq  */
LOCAL	void delete_rb_node(struct ps_cfs_rq_t *rq,rb_node *n) {
	rb_node *x, *y;
	long leftmost=0; 

	if (!n || n == NIL) return;

	if (rq->leftmost ==n){
		leftmost=1;

	}
	/*rq->leftmost =successor(n); */
 
	if (n->rb_left == NIL || n->rb_right == NIL) {
		/* y has a NIL node as a child */
		y = n;
	} else {
		/* find tree successor with a NIL node as a child */
		y = n->rb_right;
		while (y->rb_left != NIL) y = y->rb_left;
	}
 
	/* x is y's only child */
	if (y->rb_left != NIL)
		x = y->rb_left;
	else
		x = y->rb_right;
 
	/* remove y from the parent chain */
	x->rb_parent = y->rb_parent;
	if (y->rb_parent!=NIL)
		if (y == y->rb_parent->rb_left)
			y->rb_parent->rb_left = x;
		else
			y->rb_parent->rb_right = x;
	else
		rq->root = x;
 
	/*if (y != n) n->key = y->key; */
	if (y != n)
		rb_set_data(n,y);
 
	if (y->color == RB_BLACK)
		delete_rb_color (rq,x);
 
	free (y);


	if (leftmost){
		x=rq->root;
		while(x->rb_left!=NIL)
			x=x->rb_left;

		rq->leftmost=x;
	}

}
 
 /************************************************************************/

LOCAL	 rb_node *find_rb_node(struct ps_cfs_rq_t *rq, double key) {
 
	rb_node * n = rq->root;
	while(n != NIL)
		if(key== n->key)
			return (n);
		else
			n = (key<= n->key) ?
				n->rb_left : n->rb_right;
	return(NIL);
}

 /************************************************************************/

void print_rb_node(struct rb_node *n){
/*
  printf("key  %f ", n->key);
  printf("color  %ld ",n->color);
  printf("parent %f ",(n->rb_parent) ? n->rb_parent->key:0);
  printf("leftchild %f ", (n->rb_left) ? n->rb_left->key:0);
  printf("rightchild %f \n",(n->rb_right) ? n->rb_right->key:0);
*/
	printf("task_id:  %ld ,", (n->si) ?n->si->task:0);
	printf("key  %f \n", n->key);

}

/************************************************************************/

void print_rb_tree(struct rb_node * n){
	if (n==NIL) return;
	print_rb_tree(n->rb_left);
	print_rb_node(n);
	print_rb_tree(n->rb_right);
	return;
}

/************************************************************************/

LOCAL struct rb_node *successor(struct rb_node *n) 
{

	rb_node * y ;
	if (n == NIL) return NIL;
	if(n->rb_right != NIL){ /* n has a right child */
		y=n->rb_right ;
		while(y->rb_left !=NIL)
			y=y->rb_left ;
		return y;
	}

	if((y=n->rb_parent )!=NIL){ /* n has no right child and n has a parent */
		if(n==y->rb_left )
			return y;
		if ((y=y->rb_parent)!=NIL)
			return y;
	 
	}
	return NIL;
}

/************************************************************************/
/*		PARASOL CFS scheduler Support Functions			*/
/************************************************************************/


/* find the leftmost node in the ps_cfs_rq_t		*/
LOCAL sched_info * 	find_fair_si(struct ps_cfs_rq_t *rq) {

	if(rq->leftmost==NIL) {
		return NULL_SCHED_PTR;

	}
	return rq->leftmost->si;
}
/************************************************************************/

LOCAL 	long	find_fair_task(struct ps_cfs_rq_t *rq) {

	struct sched_info *si;
	si=find_fair_si(rq);

	if (si) {
		if (si->own_rq) {
			si=find_fair_si(si->own_rq);
			if(si)
				return si->task;
		}
		else{
			return si->task;

		}
	}
	return NULL_TASK;

}
/************************************************************************/

/* find the leftmost node in the ps_cfs_rq_t		*/
LOCAL sched_info * find_next_si(sched_info *si) {

	if (si )
		return successor(si->rbnode)->si;
	return NULL_SCHED_PTR;
	
}

/************************************************************************/

LOCAL 	long 	find_next_task(ps_task_t *tp) {
	
	struct sched_info *si, *psi;
	if(tp){
		si=tp->si;
		psi=find_next_si(si);
		if(psi)
			return psi->task;
		else
			if(si->parent){
				psi=find_next_si(si->parent);
				if (!psi)	
					return NULL_TASK;
				si=find_fair_si(psi->own_rq);
					
				if(si)
					return si->task;
			}
	}
	return NULL_TASK;
}
/************************************************************************/

void dq_cfs_si(sched_info *si){
	rb_node *  n;
	ps_cfs_rq_t *rq;

	if (!(si->on_rq) || ((n=si->rbnode)==NIL)) {

		ps_abort("Ready task missing from CFS run queue");
	}


	/* update cfs_rq */
	rq=si->rq;

	update_cfs_curr(rq);

	if(si!=rq->curr)
		update_ready(si);

	delete_rb_node(rq,n);

	/*si->sched_time=ps_now; */
	si->rbnode=NIL;
	si->on_rq=0;
	rq->nready--;
}

/************************************************************************/

void enque_cfs_si(sched_info * si){
	ps_cfs_rq_t * rq=si->rq;
	rb_node * n;
	double key,delta;

	update_cfs_curr(rq);
	if (si->on_rq || si->rbnode!=NIL) return;

	if ((delta=si->sched_time-ps_now)>0)
		si->fair-=delta*si->weight;

	key= rq->fair - si->fair;

	if ((n=insert_rb_node(rq,key))==NIL){
		ps_abort("Insufficient Memory");
	}

	si->sched_time=ps_now;
	si->rbnode=n;
	n->si=si;
	si->on_rq=1;
	rq->nready++;

}
/************************************************************************/

/*Enqueue ready task into a cfs_rq */
void enqueue_cfs_task(ps_task_t * tp){
	sched_info *si=tp->si;
	/*(tp->hp->port_n)++; */
	while(si){
		if (si->on_rq)
			break;
		enque_cfs_si(si);
		si=si->parent;
	}
	if((si=tp->si)->parent){
		si->rq->load++;
		si=si->parent;
	}
	si->rq->load++;
}

/************************************************************************/
/* Dequeue ready task from a cfs_rq */
void dq_cfs_task(ps_task_t * tp){
	sched_info *si=tp->si;
	/*(tp->hp->port_n)--; */
	while(si){
		dq_cfs_si(si);
		if(si->rq->nready>0)
			break;
		si=si->parent;
	}
	if((si=tp->si)->parent){
		si->rq->load--;
		si=si->parent;
	
	}
	si->rq->load--;
}

/************************************************************************/
SYSCALL check_fair(ps_task_t *run_tp,ps_task_t *ready_tp){

	double gran=0.05;
	if ((tid(ready_tp)==NULL_TASK) || (run_tp==ready_tp))
		return (FALSE);
	if(run_tp->group!=ready_tp->group)
		return(TRUE);
	if(run_tp->si->fair>ready_tp->si->fair+gran)
		return (FALSE);
	else
		return(TRUE);
		
}

/************************************************************************/
void update_run_task(ps_task_t * tp){

/* update the sched_info of the run task. this function will be called	*/
/* at end of running.							*/
	struct sched_info 	*si;
	struct ps_cfs_rq_t 	*rq;
	struct ps_node_t 	*np;
	long host;
	long i;
	/*struct ps_cfs_rq_t *rq=node_ptr(tp->node)->host_rq; */
	si=tp->si;
	if(si->rq->curr !=si)
		ps_abort("Not a run task");
	if(ps_now ==si->sched_time )
		return;
	while(si){
		dq_cfs_si(si);
		enque_cfs_si(si);
		si=si->parent;
	}

	/*update group rq*/
	np=node_ptr(tp->node);
	if(np->ngroup){
		host=hid(np,tp->hp);
		for(i=0;i< np->ngroup; i++){
			rq= &(np->cpu[host].group_rq[i]);
			si=rq->si;
			if(!si->on_rq)
				continue;
			if (si==rq->curr)
				continue;
			update_ready(si);
			dq_cfs_si(si);
			enque_cfs_si(si);
		}
	}
}

/************************************************************************/
void update_ready_task(ps_task_t *tp){

/* update the sched_info of a ready task.		*/
	struct sched_info *si;
	si=tp->si;
	if (si->rq->sched_time!=ps_now )
		update_cfs_curr(si->rq);
	update_ready(si); /*??? */

	/*update group*/

}

/************************************************************************/
void update_sleep_task(ps_task_t *tp){

/* update the sched_info of a waiting task.		*/
	/*double MAX_SLEEP=0.1; */
	struct sched_info *si;
	double delta;

	si=tp->si;
	if ((delta=ps_now-si->sched_time)==0)
		return;

	si->sched_time =ps_now;
	/*if(delta>MAX_SLEEP)
	  delta=MAX_SLEEP;

	  si->fair+=delta;/*???  */

	/*update group*/
}

/************************************************************************/
void print_si(sched_info *si){
	printf("si parameters are:\n");
	printf("si->sched_time =%f   ",si->sched_time);
	printf("si->exec_time =%f   ",si->exec_time);
	printf("si->fair=%f   ",si->fair);
/*printf("si->sleep_time =%f   \n",si->sleep_time); */

	
}

void print_rq( ps_cfs_rq_t *rq){
	printf("rq parameters are:\n");
	printf("rq address is %lx\n",(long)rq);
	printf(" fair of rq =%f, nready =%ld \n",rq->fair,rq->nready);
	printf("rq->exec_time =%f,rq->sched_time =%f \n ", rq->exec_time,rq->sched_time);
}
/************************************************************************/


void update_cfs_curr(ps_cfs_rq_t * cfs_rq){

	struct 	sched_info 	*si;
	double 	delta;
	double 	delta_fair;
	if((delta=ps_now-cfs_rq->sched_time)==0) return;
	if((si=cfs_rq->curr)==NULL_SCHED_PTR){
		cfs_rq->sched_time=ps_now;
		return;
	}
	if(cfs_rq->nready){
		delta_fair=delta /(cfs_rq->nready);
		if(si->weight!=1)
			si->fair-=delta*(1- si->weight);
		else
			si->fair-=delta-delta_fair;
		si->exec_time+=delta;
		si->sched_time=ps_now;
		/* update cfs _rq	*/

		cfs_rq->fair += delta_fair;
		cfs_rq->exec_time+=delta;
		cfs_rq->sched_time=ps_now;
	}
	else{
		si->sched_time=ps_now;
		cfs_rq->sched_time=ps_now;

	}

#if defined(DEBUG)
	print_si(si);
	print_rq(cfs_rq);
#endif

}
/************************************************************************/

void update_ready(sched_info * si){
	double 	delta;
	double 	delta_fair;
	ps_cfs_rq_t * rq;

	rq=si->rq;
	if((delta=rq->sched_time-si->sched_time)==0) return;
	if(rq->curr==si){
		update_cfs_curr(rq);
		return;
	}
	delta_fair=rq->fair-(si->rbnode->key)-(si->fair);
	if (delta_fair==0) 	/* this rq has no running task right now */
		delta_fair=(rq->nready)?(delta /rq->nready):delta;
	if(si->weight!=1)
		si->fair+=delta_fair* si->weight *(rq->nready);
	else
		si->fair+=delta_fair;
	si->sched_time=ps_now;
#if defined(DEBUG)
	print_si(si);
#endif
}
/************************************************************************/

LOCAL void init_sched_info(
	sched_info * si,
	long task,
	ps_cfs_rq_t * own_rq,
	ps_cfs_rq_t * rq,
	double weight
)
{
	si->on_rq  = 0; 
	si->task  = task; 
	si->rbnode = NIL;
	si->own_rq  = own_rq;
	si->rq = rq;
	si->parent = NULL_SCHED_PTR;
	si->fair = 0.0;
	si->key = 0;
	si->q = node_ptr(ps_task_ptr(task)->node)->quantum;
	si->exec_time = 0; 
	si->sched_time = 0;/*???? */
	si->sleep_time = 0;
	si->weight = weight;
}
/************************************************************************/


/* set this cfs task to run */
LOCAL void set_cfs_task_run(ps_task_t *tp){
	sched_info * si=tp->si;
	si->rq->curr=si;
	if(si->parent)
		si->parent->rq->curr=si->parent;
}
/************************************************************************/

double get_quantum(ps_node_t *np,long host,double delta){
	double q,max,min;
	max=np->quantum;
	min=max/5;
	q=delta/(np->host_rq[host].load+1);
	if (q<min)
		q=min;
	else if(q>max)
		q=max;

	return q;
}
/* cooling this cfs task */
LOCAL void cooling_cfs_task(ps_task_t *tp){
	sched_info * si=tp->si;
	si->rq->curr=NULL_SCHED_PTR;
	if(si->parent)
		si->parent->rq->curr=NULL_SCHED_PTR;
}
			
LOCAL long find_min_load_host(
/* find a host with a min load */

	ps_node_t	*np,			/* node pointer		*/
	ps_task_t	*tp			/* task pointer		*/
	)
{
	long host;
	long min,load;
	long i;
	if (np->ncpu==1) return 0;
	host=0;
	if(np->ngroup){
		min=np->cpu[host].group_rq[tp->group].nready;
		for(i=1;i<np->ncpu;i++){
			load=np->cpu[i].group_rq[tp->group].nready;
			if(load<min){
				min=load;
				host=i;
			}
			else if(load==min){
				if (np->host_rq[i].load<np->host_rq[host].load){
					min=load;
					host=i;
				}
			}
		}
	}else{
		min=np->host_rq[host].load;
		for(i=1;i<np->ncpu;i++){
			if ((load=np->host_rq[i].load)<min){
				min=load;
				host=i;
			}
		}
	}
	return host;
}


void cap_handler(ps_task_t *tp){

	double 		delta,fair;
	ps_node_t 	*np;
	ps_cpu_t 	*hp;
	ps_cfs_rq_t 	*rq;
	ps_task_t	*ctp;
	sched_info 	*group_si,*si;
	char		string[40];		/* Output buffer	*/

	np=node_ptr(tp->node);
	group_si=tp->si->parent;
	rq=tp->si->rq;
	fair =0-group_si->fair;
	delta=2*fair/(group_si->weight) ;
	hp=tp->hp;
	while((si=find_fair_si(rq))){
		dq_cfs_si(si);
		ctp=ps_task_ptr(si->task);
		SET_TASK_STATE(ctp, TASK_SLEEPING);
		/* current task is preempted*/
			if (ctp->pt_tag == 0){
				ctp->pt_last=ps_now;
 				ctp-> pt_tag = 1;
			}
		if(ts_flag)
			ts_report(ctp, "sleeping");
		if (angio_flag) {
			sprintf (string, "wDelay %g", delta);
			ps_log_user_event (string);
		}
	
		ctp->qep = NULL_EVENT_PTR;
		if(ctp->tep != NULL_EVENT_PTR) {
			ctp->rct = ctp->tep->time - ps_now;
			remove_event(ctp->tep);
			ctp->tep = NULL_EVENT_PTR;
		}
		ctp->tep = add_event(ps_now+delta, END_SLEEP, (long*)si->task);
	}
	
	rq->curr=NULL_SCHED_PTR;
	rq->load=0;	
	dq_cfs_si(group_si);

	/* cooling the cfs task */
	group_si->rq->curr=NULL_SCHED_PTR;
	group_si->fair=fair;
	group_si->sched_time=ps_now+delta;
	tp->hp = NULL_HOST_PTR;
	find_ready(np, hp);

}

#if defined(DEBUG)
static void 	print_event(
    const char * name,			/* Event operation 	*/
    ps_event_t	*ep )			/* event pointer	*/

/* prints calendar events 					*/

{
	ps_task_t	*tp;			/* task pointer		*/
	ps_mess_t	*mp;			/* message pointer	*/
	ps_bus_t	*bp;			/* bus pointer		*/
	ps_link_t	*lp;			/* link pointer		*/
	ps_node_t	*np;			/* node pointer		*/
	static	char	*e_name[] = {		/* event names		*/
		"  End sync  ", "End compute ", "End quantum ", "End transmit",
		" End sleep  ", "Receive t/o ", " End block  ", "", "", "",
		" Link fail  ", "Link restore", "  Bus fail  ", "Bus restore ",
		" Node fail  ", "Node restore", "", "", "", "", 
		" User event "
	};
	
	fprintf(stderr, "\n%-12s | %12.4f   | %s |", name, ep->time, 
	    e_name[ep->type]);
	switch(ep->type) {

	case END_SYNC:
	case END_COMPUTE:
	case END_QUANTUM:
	case END_SLEEP:
	case END_RECEIVE:
	case END_BLOCK:
		tp = ps_task_ptr((size_t)ep->gp);
		fprintf(stderr, "%4ld | %s", (size_t)ep->gp,
			tp->name);
		break;
	
	case END_TRANS:
		mp = (ps_mess_t *) ep->gp;
		tp = ps_task_ptr(port_ptr(mp->port)->owner);
		fprintf(stderr, "%4ld | %s", 
		    port_ptr(mp->port)->owner, tp->name);
		break;

	case LINK_FAILURE:
	case LINK_REPAIR:
		lp = (ps_link_t *) ep->gp;
		fprintf(stderr, "%4ld | %s", lid(lp),
		    lp->name);
		break;

	case BUS_FAILURE:
	case BUS_REPAIR:
		bp = (ps_bus_t *) ep->gp;
		fprintf(stderr, "%4ld | %s", bid(bp),
		    bp->name);
		break;

	case NODE_FAILURE:
	case NODE_REPAIR:
		np = (ps_node_t *) ep->gp;
		fprintf(stderr, "%4ld | %s", nid(np),
		    np->name);
		break;

	}
}
#endif

