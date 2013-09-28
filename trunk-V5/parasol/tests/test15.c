/* $Id$ */
/************************************************************************/
/* test15.c:	PARASOL test program					*/
/*									*/
/* Created: 	14/06/95 (PRM)						*/
/*									*/
/* Description:	Tests the fairness of round robin scheduling.		*/	
/*									*/
/************************************************************************/

#include <parasol.h>
#include "test.h"
 
#define NTASKS 10
#define QUANTUM 0.1


void ps_genesis(void * arg)
{
	long i, nid;
	void pr_tester(void *);
	void fcfs_tester(void *);
	void hol_tester(void *);

	nid = ps_build_node("", 1, 1.0, QUANTUM, PR, FALSE);
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("RR", nid, ANY_HOST, pr_tester, 1));

	nid = ps_build_node("", 1, 1.0, QUANTUM, FCFS, FALSE);
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("FCFS", nid, ANY_HOST, fcfs_tester, 1));

	nid = ps_build_node("", 1, 1.0, QUANTUM, HOL, FALSE);
	for (i = 0; i < 10; i++)
		ps_resume(ps_create("HOL", nid, ANY_HOST, hol_tester, 1));

	ps_suspend(ps_myself);
}

void pr_tester(void * arg)
{
	double processing_time = 0.0;

	while (TRUE) {
 		ps_compute(1.0);
		processing_time += 1.0;
		if (ps_now > (double)NTASKS * 1.0) {
			if (processing_time < (ps_now / (double)NTASKS) - 1.0)
				ERRABORT("PR scheduling is not being fair");
			if (processing_time > (ps_now / (double)NTASKS) + 1.0)
				ERRABORT("PR scheduling is not being fair");
		}
	}
}
   
void fcfs_tester(void * arg)
{
	double processing_time = 0.0;

	while (TRUE) {
 		ps_compute(1.0);
		processing_time += 1.0;
		if (ps_now > (double)NTASKS * 1.0) {
			if (processing_time < (ps_now / (double)NTASKS) - 1.0)
				ERRABORT("FCFS scheduling is not being fair");
			if (processing_time > (ps_now / (double)NTASKS) + 1.0)
				ERRABORT("FCFS scheduling is not being fair");
		}
	}
}
   
void hol_tester(void * arg)
{
	double processing_time = 0.0;

	while (TRUE) {
 		ps_compute(1.0);
		processing_time += 1.0;
		if (ps_now > (double)NTASKS * 1.0) {
			if (processing_time < (ps_now / (double)NTASKS) - 1.0)
				ERRABORT("HOL scheduling is not being fair");
			if (processing_time > (ps_now / (double)NTASKS) + 1.0)
				ERRABORT("HOL scheduling is not being fair");
		}
	}
}

