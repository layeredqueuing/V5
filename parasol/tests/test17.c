/* $Id$ */
/************************************************************************/
/* test08.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests ps_migrate					*/
/*									*/
/************************************************************************/
#include <parasol.h>
#include "test.h"


double mark;

void ps_genesis(void * arg)
{
	long 	tn1, tn2, tid, i;
	long	naids[3], nbids[3], tids[3];
	void 	hi_pri(void *);
	void	dummy(void *);
	void	pr_migrater(void *);
	void	pr_computer(void *);
	void	other_migrater(void *);
	void	other_computer(void *);

	/* What if a computing task is migrated from a fast node to a 	*/
	/* slow	node?							*/

	tn1 = ps_build_node("", 1, 1.0, 0.0, PR, FALSE);
	tn2 = ps_build_node("", 1, 0.5, 0.0, PR, FALSE);
	ps_resume(ps_create("", tn1, ANY_HOST, dummy, 1));
	ps_resume(ps_create("", tn2, ANY_HOST, dummy, 1));
	ps_resume(tid = ps_create("", tn1, ANY_HOST, hi_pri, 2));
	ps_sleep(1.0);
	ps_migrate(tid, tn2, ANY_HOST);
	ps_sleep(3.0);
	if (abs(mark - 3.0) > TIME_TOLERANCE) {
		fprintf(stderr, "%g :", mark);
		ERRREPORT("Error migrating from fast node to slow node");
	}

	/* What if a computing task is migrated from a slow node to a	*/
	/* fast node?							*/

	ps_resume(tid = ps_create("", tn2, ANY_HOST, hi_pri, 2));
	ps_sleep(2.0);
	ps_migrate(tid, tn1, ANY_HOST);
	ps_sleep(3.0);
	if (abs(mark - 3.0) > TIME_TOLERANCE) {
		fprintf(stderr, "%g :", mark);
		ERRREPORT("Error migrating from slow node to fast node");
	}

 	naids[0] = ps_build_node("", 1, 1.0, 0.0, PR, FALSE);
 	nbids[0] = ps_build_node("", 1, 1.0, 0.0, PR, FALSE);
 	naids[1] = ps_build_node("", 1, 1.0, 0.0, FCFS, FALSE);
 	nbids[1] = ps_build_node("", 1, 1.0, 0.0, FCFS, FALSE);
 	naids[2] = ps_build_node("", 1, 1.0, 0.0, HOL, FALSE);
 	nbids[2] = ps_build_node("", 1, 1.0, 0.0, HOL, FALSE);
	ps_resume(tids[0] = ps_create("PR Migrater", naids[0], ANY_HOST, 
	    pr_migrater, 2));
	ps_resume(tids[1] = ps_create("FCFS Migrater", naids[1], ANY_HOST, 
	    other_migrater, 2));
	ps_resume(tids[2] = ps_create("HOL Migrater", naids[2], ANY_HOST, 
	    other_migrater, 2));
	ps_resume(ps_create("PR Computer", naids[0], ANY_HOST, pr_computer, 1));
	ps_resume(ps_create("PR Computer", nbids[0], ANY_HOST, pr_computer, 1));
	ps_resume(ps_create("FCFS Computer", naids[1], ANY_HOST, other_computer,
	    1));
	ps_resume(ps_create("FCFS Computer", nbids[1], ANY_HOST, other_computer,
	    1));
	ps_resume(ps_create("HOL Computer", naids[2], ANY_HOST, other_computer, 
	    1));
	ps_resume(ps_create("HOL Computer", nbids[2], ANY_HOST, other_computer, 
	    1));
	while (TRUE) {
		ps_sleep(ps_exponential(1.0));
		for (i = 0; i < 3; i++)
			ps_migrate(tids[i], nbids[i], ANY_HOST);
		ps_sleep(ps_exponential(1.0));
		for (i = 0; i < 3; i++)
			ps_migrate(tids[i], naids[i], ANY_HOST);
	}
}
   

void hi_pri(void * arg)
{
	long	time;

	time = ps_now;
	ps_compute(2.0);
	mark = ps_now - time;
}

void dummy (void * arg)
{
	ps_compute(BIG_TIME);
}

void pr_migrater(void * arg)
{
	double	mark;
	double	delta;

	while (TRUE) {
		mark = ps_now;
		delta = ps_exponential(1.0);
		ps_compute(delta);
		if (abs(ps_now - mark - delta) > TIME_TOLERANCE)
			ERRABORT("ps_migrate with PR scheduling problems");
		mark = ps_now;
		delta = ps_exponential(1.0);
		ps_sleep(delta);
		if (abs(ps_now - mark - delta) > TIME_TOLERANCE)
			ERRABORT("ps_migrate with PR scheduling problems");
	}
}

void pr_computer(void * arg)
{
	while(TRUE) {
		ps_compute(ps_exponential(1.0));
		ps_sleep(ps_exponential(0.2));
	}
}

void other_migrater(void * arg)
{
	while(TRUE) {
		ps_compute(ps_exponential(1.0));
		ps_sleep(ps_exponential(0.2));
	}
}

void other_computer(void * arg)
{
	double	mark;
	double	delta;

	while (TRUE) {
		mark = ps_now;
		delta = ps_exponential(1.0);
		ps_compute(delta);
		if (abs(ps_now - mark - delta) > TIME_TOLERANCE)
			ERRABORT("ps_migrate scheduling problems");
  		ps_sleep(ps_exponential(1.0));
 	}
}

