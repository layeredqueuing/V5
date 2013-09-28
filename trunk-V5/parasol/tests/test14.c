/* $Id$ */
/************************************************************************/
/* test14.c:	PARASOL test program					*/
/*									*/
/* Created: 	12/06/95 (PRM)						*/
/*									*/
/* Description: Tests preemption and dynamic priorities with PR schedu-	*/
/*		ling.							*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include <math.h>
#include "test.h"
 
#define NTASKS 10

void ps_genesis(void * arg)
{
	long nid, i;
	void pr_tester(void *);
	void pr_tester2(void *);
	void hi_pri(void *);

	ps_resume(ps_create("PR Tester", 0, ANY_HOST, pr_tester, 1));

	nid = ps_build_node ("", 1, 1.0, 0.0, PR, FALSE);
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("PRT2", nid, ANY_HOST, pr_tester2, 1));
	ps_resume(ps_create("Hi Pri.", nid, ANY_HOST, hi_pri, 2));

	ps_suspend(ps_myself);
}

void pr_tester(void * arg)
{
	long nid, i, ti;
	long tasks[NTASKS];
	void dummy(void *);
	ps_task_t *ttp;

	nid = ps_build_node("", 1, 1.0, 0.0, PR, FALSE);
	for (i = 0; i < NTASKS; i++)
		ps_resume(tasks[i] = ps_create("Dummy", nid, ANY_HOST, dummy, 
		    1));

	while (TRUE) {
		for (i = 0; i < NTASKS; i++) {
			ti = ps_choice(NTASKS);
			ttp = ps_task_ptr(tasks[ti]);
			ps_adjust_priority(tasks[ti], 2);
			ps_sleep(1e-6); /* FIXME: Should we have to do this? */
			if (ps_task_state(tasks[ti]) != TASK_COMPUTING)
				ERRABORT("PR scheduling problem");
			ps_sleep(1.0);
			if (ps_task_state(tasks[ti]) != TASK_COMPUTING)
				ERRABORT("PR scheduling problem");
			ps_adjust_priority(tasks[ti], 1);
		}
	}
}

void dummy(void * arg) 
{
	ps_compute (BIG_TIME);
}
  

void pr_tester2(void * arg)
{
	static long runner;

	while (TRUE) {
		runner = ps_myself;
		ps_compute (ps_exponential(1.0));
		if (runner != ps_myself)
			ERRABORT("PR scheduling problem");
		ps_sleep(ps_exponential(2.0));
	}
}

void hi_pri (void * arg)
{
	double mark, delta;

 	while (TRUE) {
		ps_sleep(ps_exponential(5.0));
		mark = ps_now;
		delta = ps_exponential(1.0);
		ps_compute(delta);
		delta = ps_now - mark - delta;
		delta = (delta < 0.0) ? -delta : delta;
		if (delta > 1.0e-6)
			ERRABORT("PR is not being preemptive");
	}
}

