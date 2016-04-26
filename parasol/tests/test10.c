/* $Id: test10.c 9210 2010-02-24 18:59:24Z greg $ */
/************************************************************************/
/* test10.c:	PARASOL test program					*/
/*									*/
/* Created: 	12/06/95 (PRM)						*/
/*									*/
/* Description:	Task creation stress test, tests for memory leaks in 	*/
/*		ps_create/kill.						*/
/* Notes: 	Tests the following:					*/
/*		- explicit suicides (ps_kill(ps_myself))		*/
/*		- ????cide (ps_kill(my_child))				*/
/*		- implicit suicides (return)				*/
/*		- killing tasks with pending messages in their standard	*/
/*		  ports.						*/
/*		- This fails under NeXTStep because wrapper doesn't work*/
/*									*/
/*									*/
/* Bugs:						 		*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include <math.h>
#include "test.h"

  

void ps_genesis(void * arg)
{
	void	stresser(void *);
	void 	task_a(void *);
	void 	task_b(void *);
	void 	task_c(void *);
 	long 	tid, tid2;

 	ps_resume(ps_create("Stresser", 0, ANY_HOST, stresser, 1));

	while (TRUE) {
 		/* explictit suicide 					*/
		ps_resume(ps_create("Task A", 0, ANY_HOST, task_a, 1));

 
 		/* ????cide 						*/
		ps_resume(tid = ps_create("Task B", 0, ANY_HOST, task_b, 1));


		/* ????cide with pending messages on the standard port 	*/
		ps_resume(tid2 = ps_create("Task B", 0, ANY_HOST, task_b, 1));
		ps_send(ps_std_port(tid2),0, "", NULL_PORT);

 		/* implicit suicide					*/
		ps_resume(ps_create("Task C", 0, ANY_HOST, task_c, 1)); 

		ps_sleep(ps_exponential(1.0));
 		ps_kill(tid);
		ps_kill(tid2);
	}
}

void stresser (void * arg)
{
	long 	nid, ntasks, i, inc;
	long 	tids[11];
	void	dummy(void * arg);

	nid = ps_build_node("", 1, 1.0, .1, PR, FALSE);
	ntasks = 0;
	for (i = 0; i < 11; i++)
		tids[i] = -1;

	while (TRUE) {	
		ps_sleep(ps_exponential(.5));
		if ((ntasks < 11 && ps_random < .5) || (ntasks == 0)) {
			inc = i = ps_choice(10) + 1;
			while (tids[i] != -1) i = (i + inc) % 11;
			ps_resume(tids[i] = ps_create("", nid, ANY_HOST, dummy,
			    1));
			ntasks++;
		}
		else {
			inc = i = ps_choice(10) + 1;
			while (tids[i] == -1) i = (i + inc) % 11;
			ps_kill(tids[i]);
			tids[i] = -1;
			ntasks--;
		}
	}
}

void dummy (void * arg)
{
	ps_compute(BIG_TIME);
}


void task_a (void * arg)
{
	ps_sleep (ps_exponential(.90));
	ps_kill(ps_myself);
}

void task_b (void * arg)
{
	ps_sleep(BIG_TIME);
}

void task_c (void * arg)
{
	ps_sleep(ps_exponential(.90));
	return;
}
