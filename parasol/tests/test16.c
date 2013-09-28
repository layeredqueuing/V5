/* $Id$ */
/************************************************************************/
/* test08.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests ps_sync						*/
/*									*/
/************************************************************************/
#include <parasol.h>
#include "test.h"

void ps_genesis(void * arg)
{
	void 	tester_a(void *);
	void 	dummy(void *);
	void 	syncer_b(void *);
	long	nid, i;

	ps_resume(ps_create("", 0, ANY_HOST, tester_a, 1));

	nid = ps_build_node("", 1, 1.0, .1, FCFS, FALSE);
	for (i = 0; i < 5; i++)
		ps_resume(ps_create("", nid, ANY_HOST, dummy, 1));
	ps_resume(ps_create("", nid, ANY_HOST, syncer_b, 2));

	nid = ps_build_node("", 1, 1.0, 0.0, PR, FALSE);
	for (i = 0; i < 5; i++)
		ps_resume(ps_create("", nid, ANY_HOST, dummy, 1));
	ps_resume(ps_create("", nid, ANY_HOST, syncer_b, 2));

	nid = ps_build_node("", 1, 1.0, .1, HOL, FALSE);
	for (i = 0; i < 5; i++)
		ps_resume(ps_create("", nid, ANY_HOST, dummy, 1));
	ps_resume(ps_create("", nid, ANY_HOST, syncer_b, 2));
			
	ps_suspend(ps_myself);
}

void tester_a (void * arg)
{
	long 	tid, nid;
	void	syncer_a(void *);

	nid = ps_build_node("", 1, 1.0, 0.0, FCFS, FALSE);
	ps_resume(tid = ps_create("", nid, ANY_HOST, syncer_a, 1));
	while (TRUE) {
		ps_suspend(tid);
		ps_sleep(ps_exponential(1.0));
		ps_resume(tid);
		ps_sleep(ps_exponential(1.0));
	}
	ps_suspend(ps_myself);
}

void syncer_a (void * arg)
{
	double delta, mark;

	while(TRUE) {
		delta = ps_exponential(2.0);
		mark = ps_now;
		ps_sync(delta);	
		if (ps_now - mark < delta - TIME_TOLERANCE)
			ERRABORT("sync/suspend not working properly");
	}
}

void dummy(void * arg)
{
	while (TRUE) {
		ps_sleep(ps_exponential(1.0));
		ps_compute(ps_exponential(1.0));
	}
}

void syncer_b (void * arg)
{
	double mark, delta;

	while (TRUE) {
		ps_sleep(ps_exponential(5.0));
		mark = ps_now;
		delta = ps_exponential(1.0);
		ps_sync(delta);
		if (abs(ps_now - mark - delta) > TIME_TOLERANCE) 
 			ERRABORT("ps_sync is allowing preemption");
	}
}
