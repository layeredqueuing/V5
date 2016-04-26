/* $Id: test06.c 9210 2010-02-24 18:59:24Z greg $ */
/************************************************************************/
/* test06.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	Tests spinlocks						*/
/*									*/
/* Notes:	Ensures that spinlocks do mutual exclusion on a Multi-	*/
/*		processor node.						*/
/*									*/ 
/************************************************************************/
#include <parasol.h>
#include "test.h"

#define LOCK0 0
 
long shared = 0;

void ps_genesis(void * arg)
{
	void locker1(void *);
	long nid, i;

	nid = ps_build_node("", 20, 1.0, 0.0, FCFS, TRUE);
	for (i = 0; i < 20; i++)
		ps_resume(ps_create("Locker", nid, i, locker1, 1));

	ps_suspend(ps_myself);
}
 

void locker1(void * arg)
{
	while (TRUE) {
		ps_lock(LOCK0);
		if (shared != 0)
			ERRABORT("Spinlocks are not working");
		shared = 1;
		ps_compute (ps_exponential(1.0));
		shared = 0;
		ps_unlock(LOCK0);
	}
}

