/* $Id$ */
/************************************************************************/
/* test13.c:	PARASOL test program					*/
/*									*/
/* Created: 	12/06/95 (PRM)						*/
/*									*/
/* Description:	Tests that when round robin scheduling is used, that	*/
/*		there isn't more CPU time given out than available.	*/	
/* Notes: 	Tests the following:					*/	
/*									*/
/*									*/
/* Bugs:						 		*/
/*									*/
/************************************************************************/

#include <parasol.h>
#include "test.h"
 
#define NTASKS 10
#define QUANTUM 0.1

double pr_timer = 0.0;
double fcfs_timer = 0.0;
double hol_timer = 0.0;

void ps_genesis(void * arg)
{
	long i, nid;
	void pr_tester(void *);
	void fcfs_tester(void *);
	void hol_tester(void *);

	nid = ps_build_node("PR_Node", 1, 1.0, QUANTUM, PR, 
	    SF_PER_TASK_HOST | SF_PER_NODE | SF_PER_HOST);
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("RR", nid, ANY_HOST, pr_tester, 1));

	nid = ps_build_node("FCFS_Node", 1, 1.0, QUANTUM, FCFS, 2);
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("FCFS", nid, ANY_HOST, fcfs_tester, 1));

	nid = ps_build_node("HOL_Node", 1, 1.0, QUANTUM, HOL, 2);
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("HOL", nid, ANY_HOST, hol_tester, 1));

	ps_suspend(ps_myself);
}

void pr_tester(void * arg)
{
	double delta;

	while (TRUE) {
		delta = ps_exponential(1.0);
		ps_compute(delta);
		pr_timer += delta;
		if (pr_timer > ps_now)
			ERRABORT("More CPU time used than elapsed time");
		if (pr_timer < ps_now - (double)(5*NTASKS))
			printf("Warning: Scheduling looks iffy\n");	
	}
}
 
void fcfs_tester(void * arg)
{
	double delta;

	while (TRUE) {
		delta = ps_exponential(1.0);
		ps_compute(delta);
		fcfs_timer += delta;
		if (fcfs_timer > ps_now)
			ERRABORT("More CPU time used than elapsed time");
		if (fcfs_timer < ps_now - (double)(5*NTASKS))
			printf("Warning: Scheduling looks iffy\n");	
	}
}

void hol_tester(void * arg)
{
	double delta;

	while (TRUE) {
		delta = ps_exponential(1.0);
		ps_compute(delta);
		hol_timer += delta;
		if (hol_timer > ps_now)
			ERRABORT("More CPU time used than elapsed time");
		if (hol_timer < ps_now - (double)(5*NTASKS))
			printf("Warning: Scheduling looks iffy\n");	
	}
}

