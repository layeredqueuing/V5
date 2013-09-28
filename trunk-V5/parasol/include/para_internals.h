/* $Id$ */
/************************************************************************/
/*	para_internals.h - PARASOL library internal header file		*/
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
/*	Revised: 21/12/93 (JEN)	Added prototype for wrapper - part of 	*/
/*				run off fix.				*/
/*		 08/04/94 (JEN) Added "port_receive" to support new	*/
/*				receive functionality.			*/
/*		 27/04/94 (JEN)	Added buffer pool support.		*/
/*		 19/07/94 (JEN) Added bs_flag to permit detection of	*/
/*				late ps_open_stat's.			*/
/*		 26/07/94 (JEN) Removed packetizing from delay macros.	*/
/*		 06/10/94 (JEN) Added arguments to wrapper to simply	*/
/*				porting.  Also added global stack 	*/
/*				testing variables and functions.	*/
/*		 07/10/94 (JEN) Modified sp_tester for auto stack siz-	*/
/*				ing. Removed DEFAULT_STACK_SIZE & added */
/*				sp_delta.				*/
/*		 06/12/94 (JEN)	Added _setjmp and _longjmp macros for	*/
/*				Solaris O/S.				*/
/*		 09/12/94 (JEN)	Changed names with ps prefix to minimize*/
/*				name clashes.				*/
/*		 19/12/94 (JEN) Re-installed a mach conditional wrt	*/
/*				MAX_DOUBLE definition.			*/
/*		 14/02/95 (JEN) Added qxflag to fix PR queueing.	*/
/*		 16/02/95 (JEN) Simplified hid macro to correct problem	*/
/*				with ps_migrate.			*/
/*		 11/05/95 (JEN) Added port_send and modified 		*/
/*				port_receive to fix ps_leave_port_set.	*/
/*		 18/05/95 (PRM) Added mid parameter to port_send and	*/
/*				and port_receive. Added mid field to 	*/
/*				ps_mess_t.				*/
/*		 15/06/95 (PRM)	Added prototypes to support stack size	*/
/*				testing (#ifdef STACK_TESTING).		*/
/*		 27/06/95 (PRM) Added #ifdef __cplusplus code for C++	*/
/*				compatability.				*/
/*		 15/06/99 (WCS) Main stuff put in para_main		*/
/*									*/
/************************************************************************/
 
#ifndef	_PARA_INTERNALS
#define	_PARA_INTERNALS

#include	<parasol.h>


/************************************************************************/
/*                 P A R A S O L   P R O T O T Y P E S			*/
/************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* does para_main belong here in this fashion? */


extern void para_own_block_stats_outputer();

extern
double	blocked_stat_outputer(
	
/* Output specified statistic.	*/
	
	long stat				/* statistic identifier */
);
		
/************************************************************************/

extern
void	warning(

/* Writes PARASOL warning to stderr.					*/

	const	char	*string			/* warning string	*/
);

/************************************************************************/

extern
long	padstr(

/* Truncates or pads string (with blanks) to specified length.		*/

	char	*s1,				/* output string	*/
	const	char	*s2,			/* input string		*/
	long	n				/* string length	*/
);

/************************************************************************/
/*		PARASOL Driver Support Functions			*/
/************************************************************************/

extern
void	bus_failure_handler(

/* Handles BUS_FAILURE events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

extern
void	bus_repair_handler(

/* Handles BUS_REPAIR events.						*/

	ps_event_t	*ep			/* event pointer	*/
);


		
/************************************************************************/

extern
void	link_failure_handler(

/* Handles LINK_FAILURE events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

extern
void	link_repair_handler(

/* Handles LINK_REPAIR events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

extern
void	node_failure_handler(

/* Handles NODE_FAILURE events.						*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/

extern
void	node_repair_handler(

/* Handles NODE_REPAIR events						*/

	ps_event_t	*ep 			/* event pointer	*/
);

/************************************************************************/

extern
void	user_event_handler(

/* Stub event handler for USER_EVENT.					*/

	ps_event_t	*ep			/* event pointer	*/
);

/************************************************************************/
/*		PARASOL Scheduler Support Functions			*/
/************************************************************************/

extern
ps_event_t	*add_event(

/* Gets an event struct from the free list and sets its fields.  This	*/
/* event is then merged into the calendar according to event time and	*/
/* a pointer to it is returned. 					*/

	double	time,				/* event time		*/
	long	type,				/* primary event code	*/
	long	*gp				/* generic pointer	*/
);

/************************************************************************/

extern
void	remove_event(

/* Removes an event from the calendar and adds it to the free list.  It	*/
/* may be examined using its pointer however its contents are valid 	*/
/* only until the struct is reused by an add_event call. If the event	*/
/* pointer is null, nothing happens.  Guard code ensures that the event	*/
/* is in a doubly linked list and aborts the simulation if not.		*/

	ps_event_t	*ep			/* event pointer	*/
);


/************************************************************************/

void mctx_create( mctx_t *mctx, void (*sf_addr)(void *), void *sf_arg, void *sk_addr, size_t sk_size);



#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif	/* _PARA_INTERNALS */
