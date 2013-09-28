/* $Id$ */
/************************************************************************/
/* test12.c:	PARASOL test program					*/
/*									*/
/* Created: 	12/06/95 (PRM)						*/
/*									*/
/* Description:	Tests PARASOL task and port information functions and	*/
/*		macros.							*/	
/* Notes: 	Tests the following explicitly:				*/
/*		- ps_myself						*/
/*		- ps_my_parent						*/
/*		- ps_my_node						*/
/*		- ps_my_host						*/
/*		- ps_my_name						*/
/*		- ps_my_priority					*/
/*		- ps_children						*/
/*		- ps_siblings						*/	
/*									*/
/*									*/ 
/************************************************************************/

#include <parasol.h>
#include <math.h>
#include "test.h"

#define NCPUS 10
long genesis_id;				/* genesis task id	*/
long nid;					/* node id		*/
long tasks[NCPUS];				/* task ids		*/

void ps_genesis(void * arg)
{
	void task_a(void *);
	long buf[NCPUS], ports[3];
	long count, i, j;
 
	genesis_id = ps_myself;

	nid = ps_build_node("", NCPUS, 1.0, 0.0, FCFS, FALSE);
	if (ps_idle_cpu(nid) != NCPUS) 
		ERRABORT("ps_idle_cpu not working");
	if (ps_ready_queue(nid, 0, NULL) != 0)
		ERRABORT("ps_ready_queue not working");
	
	for (i = 0; i < NCPUS; i++)
		ps_resume (tasks[i] = ps_create("Task A", nid, i, task_a, i));
 
	ps_siblings(&count, buf);
	if (count != 0) 	
		ERRABORT("ps_siblings not working");

	ps_children(&count, buf);
	if (count != NCPUS)
		ERRABORT("ps_children - count is wrong");

	for (i = 0; i < count; i++) {
		for (j = 0; j < NCPUS; j++)
			if (tasks[i] == buf[j]) break;
		if (j == NCPUS)
			ERRABORT("task missing from sibling list");
 	}
 
	for (i = 0; i < NCPUS; i++)
		if (ps_owner(ps_std_port(tasks[i])) != tasks[i])
			ERRABORT("Problem with ps_std_port or ps_owner");

	ports[0] = ps_my_std_port;
	ports[1] = ps_allocate_port("Private 1", ps_myself);
	ports[2] = ps_allocate_port("Private 2", ps_myself);
	ps_my_ports(&count, buf);
	if (count != 3)
		ERRABORT("ps_my_ports - count is wrong");
	
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			if (ports[i] == buf[j]) break;
		if (j ==3)
			ERRABORT("port not included in port list");
	}
	ps_sleep(5);
	printf ("All tests completed successfully.\n");

	ps_suspend(ps_myself);
}


void task_a(void * arg)
{
	long mypos;
	long buf[NCPUS];
	long count, i, j;

	ps_sleep(0.001);

	for (mypos = 0; mypos < NCPUS; mypos++)
		if (tasks[mypos] == ps_myself) break;

	if (mypos == NCPUS)
		ERRABORT("ps_myself not working");

	if (ps_my_parent != genesis_id )
		ERRABORT("ps_my_parent not working");

	if (ps_my_node != nid)
		ERRABORT("ps_my_node not working");

	if (ps_my_host != mypos)
		ERRABORT("ps_my_host not working");

	if (ps_my_priority != mypos)
		ERRABORT("ps_my_priority not working");

	if (strcmp (ps_my_name, "Task A"))
		ERRABORT("ps_my_name not working");

	ps_children(&count, buf);
	if (count != 0)
		ERRABORT("ps_children not working");

	ps_siblings(&count, buf);
	if (count != NCPUS - 1)
		ERRABORT("ps_siblings not working");

	for (i = 0; i < count; i++) {
		if (i != mypos) {
			for (j = 0; j < NCPUS; j++)
				if (tasks[i] == buf[j]) break;
			if (j == NCPUS)
				ERRABORT("tasks missing from sibling list");
		}
 	}
 	ps_suspend(ps_myself);
}
