/* $Id$ */
/************************************************************************/
/* test11.c:	PARASOL test program					*/
/*									*/
/* Created: 	12/06/95 (PRM)						*/
/*									*/
/* Description:	Tests ps_resume, ps_suspend, ps_awaken, ps_sleep,	*/
/*		ps_compute, and the speed parameter of ps_build_node.	*/	
/* Notes: 	Tests the following:					*/	
/*									*/
/*									*/
/* Bugs:						 		*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include <math.h>
#include "test.h"

#define SPEEDUP 5.0  

long data_d;
long data_c;

void ps_genesis(void * arg)
{
	void task_a(void *);
	void task_b(void *);
	void task_c(void *);
	void task_d(void *);
 	long tid, tid2, nid;

	/* Test ps_compute with speedup */
	nid = ps_build_node("Node A", 1, SPEEDUP, 0.0, FCFS, FALSE);
	ps_resume(ps_create("Task A", nid, ANY_HOST, task_a, 1));
	
	/* Tests ps_sleep */
	ps_resume(ps_create("Task B", 0, ANY_HOST, task_b, 1));

	/* Tests ps_sleep and ps_awaken */
	ps_resume(tid = ps_create("Task C", 0, ANY_HOST, task_c, 1));

	/* Tests ps_suspend and ps_resume */
	ps_resume(tid2 = ps_create("Task D", 0, ANY_HOST, task_d, 1));

	while (TRUE) {
		ps_sleep(.5);
		if (data_c != 1)
			ERRABORT("ps_awaken not working properly");
		if (data_d != 1)
			ERRABORT("ps_resume not working properly");
		data_c = 0;
 		data_d = 0;
 		ps_sleep(ps_exponential(1.0));
		if (data_c != 0)
			ERRABORT("ps_sleep not working properly");
		if (data_d != 0) 
			ERRABORT("ps_suspend not working properly");
		ps_awaken(tid);
		ps_resume(tid2);
	}
}
				
 
void task_a (void * arg)
{
	double mark, delta;

	while (TRUE) {
		mark = ps_now;
		delta = ps_exponential(1.0);
		ps_compute(delta);
		delta = ps_now - mark - delta / SPEEDUP;
		delta = (delta < 0.0) ? -delta : delta;  /* |delta| */
		if (delta > 1.0e-4) 
			ERRABORT("ps_compute not working properly");
	}
}

void task_b (void * arg)
{
	double mark, delta;

	while (TRUE) {
		mark = ps_now;
		delta = ps_exponential(1.0);
		ps_sleep(delta);
		delta = ps_now - mark - delta;
		delta = (delta < 0.0) ? -delta : delta;  /* |delta| */
		if (delta > 1.0e-4) 
			ERRABORT("ps_compute not working properly");
	}
}

void task_c (void * arg)
{
	while(TRUE) {
		data_c = 1;
		ps_sleep(BIG_TIME);
		if (data_c != 0)
			ERRABORT("Something wrong with task_c");
	}
}

void task_d (void * arg)
{
	while(TRUE) {
		data_d = 1;
		ps_suspend(ps_myself);
		if (data_d != 0)
			ERRABORT("Something wrong with task_d");
	}
}

