/* $Id$ */
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
/************************************************************************/

#include <para_internals.h>
#include <para_privates.h>
#if defined(HAVE_FENV_H)
#if defined(__GNUC__) && defined(linux)
#define __USE_GNU
#endif
#if defined(__sparc)
#define __EXTENSIONS__
#endif
#include <fenv.h>
#endif
#if defined(HAVE_IEEEFP_H)
#include <ieeefp.h>
#endif


/************************************************************************/
/*		PARASOL Driver Support Functions			*/
/************************************************************************/

int main(

/* Default PARASOL mainline - scans command line for simulation		*/
/* parameters and flags, and then calls run_parasol to execute the	*/
/* simulation.  Users may replace main with their own wrapper.		*/
 
	int	argc,				/* argument count	*/
	char	*argv[]				/* arguments		*/
)
{
	long	flags = 0;			/* run-time flags	*/
 	double	duration;			/* simulation duration	*/
	long	seed;				/* random number seed	*/

	printf("\n\n\n**************************************************");
	printf("******************************");
	printf("\n*							");
	printf("		       *");
	printf("\n* 			P A R A S O L  (Version %s)   	", 
	    VERSION);
	printf("	       *");
	printf("\n*							");
	printf("		       *");
	printf("\n******************************************************");
	printf("**************************\n\n");

	
/* 	Acquire simulation parameters					*/

	switch(argc) {
	
	case 2:
		if(strcmp(argv[1], "-t") == 0)
			flags |= RPF_TRACE;
		else if(strcmp(argv[1], "-s") == 0) 
			flags |= RPF_TRACE | RPF_STEP;
		else if(strcmp(argv[1], "-w") == 0)
			flags |= RPF_WARNING;
		else
			ps_abort("Invalid PARASOL run-time flag");

	case 1:
		printf("\nEnter random number seed: ");
		scanf("%ld", &seed);
		while(TRUE) {
			printf("\nEnter positive simulation duration: ");
			scanf("%lf", &duration);
			if(duration > 0.0)
				break;
		}
		break;

	case 5:
		if(strcmp(argv[2], "-t") == 0)
			flags |= RPF_TRACE;
		else if(strcmp(argv[2], "-s") == 0)
			flags |= RPF_TRACE | RPF_STEP;
		else if(strcmp(argv[2], "-w") == 0)
			flags |= RPF_WARNING;
		else
			ps_abort("Invalid PARASOL run-time flag");

	case 4:
		if(strcmp(argv[1], "-t") == 0)
			flags |= RPF_TRACE;
		else if(strcmp(argv[1], "-s") == 0)
			flags |= RPF_TRACE | RPF_STEP;
		else if(strcmp(argv[1], "-w") == 0)
			flags |= RPF_WARNING;
		else
			ps_abort("Invalid PARASOL run-time flag");

	case 3:
		sscanf(argv[--argc], "%lf", &duration);
		if(duration <= 0.0)
			ps_abort("Non-positive simulation duration");
		sscanf(argv[--argc],"%ld", &seed);
		break;



	default:
		ps_abort("Invalid # of PARASOL parameters");
	}

/*	Enable floating polong exceptions 				*/
#if defined(HAVE_FENV_H) && defined(HAVE_FEENABLEEXCEPT)
	feenableexcept( FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW );
#elif  defined(HAVE_IEEEFP_H) && defined(HAVE_FPSETMASK)
    fpsetmask( FP_X_INV | FP_X_DZ | FP_X_OFL );
#endif
	
	ps_run_parasol(duration, seed, flags);
	if (ps_stat_tab.used && (bs_time < 0.0))
		ps_stats();
	printf ("\nSimulation Complete\n\n");

#ifdef STACK_TESTING
	test_all_stacks();
#endif /*STACK_TESTING*/

	return 0;
}

/************************************************************************/

void	bus_failure_handler(

/* Handles BUS_FAILURE events.						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	warning("Bus failures not implemented");
}

/************************************************************************/

void	bus_repair_handler(

/* Handles BUS_REPAIR events.						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	warning("Bus repairs not implemented");
}

/************************************************************************/

void	link_failure_handler(

/* Handles LINK_FAILURE events.						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	warning("Link failures not implemented");
}

/************************************************************************/

void	link_repair_handler(

/* Handles LINK_REPAIR events.						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	warning("Link repairs not implemented");
}

/************************************************************************/

void	node_failure_handler(

/* Handles NODE_FAILURE events.		 				*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	warning("Node failures not implemented");
}

/************************************************************************/

void	node_repair_handler(

/* Handles NODE_REPAIR events.						*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	warning("Node repairs not implemented");
}

/************************************************************************/

void	user_event_handler(

/* Stub event handler for USER_EVENT.					*/

	ps_event_t	*ep			/* event pointer	*/
)
{
	warning("Dummy (NULL) user event handler activated");
}
