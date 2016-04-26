/* $Id: test18.c 9210 2010-02-24 18:59:24Z greg $ */
/************************************************************************/
/* test18.c:	PARASOL test program					*/
/*									*/
/* Created: 	07/06/95 (PRM)						*/
/*									*/
/* Description:	 							*/
/*									*/
/************************************************************************/
#include <parasol.h>
#include "test.h"

#define NTASKS 3				/* number of tasks	*/
#define SLEEP_TIME 1.0				/* mean sleep time	*/
#define COMPUTE_TIME 1.0			/* mean compute time	*/
#define IDLE 1					/* idle state		*/
#define BUSY 2					/* busy state		*/

void ps_genesis(void * arg)
{
	void 	scheduler (void *);
	void 	task (void *);
	long 	nid, i;
	char	name[20];

	nid = ps_build_node2("NodeA", 1, 1.0, scheduler, 0);
	for (i = 0; i < NTASKS; i++) {
		sprintf (name, "Task %ld", i);
		ps_resume(ps_create(name, nid, ANY_HOST, task, 1));
	}

	ps_suspend(ps_myself);
}

void task (void * arg)
{
	while (TRUE) {
		ps_compute(ps_exponential(COMPUTE_TIME));
		ps_sleep(ps_exponential(SLEEP_TIME));
	}
}

void scheduler (void * arg)
{
	long	type, data, ntasks, rtrq[NTASKS], cpu_state;
	double	ts;
	char	*text;
	
	cpu_state = IDLE;
	while (TRUE) {
		ps_receive(ps_my_std_port, NEVER, &type, &ts, &text, &data);
		switch (type) {
		case SN_IDLE:			/* task done computing	*/
			ntasks = ps_ready_queue(ps_my_node, NTASKS, rtrq);
			if (ntasks > 0) {
				cpu_state = BUSY;
				ps_schedule(rtrq[0], ps_my_host);
			}
			else {
				cpu_state = IDLE;
				ps_schedule(NULL_TASK, ps_my_host);
			}
			break;

 
		case SN_READY:			/* new task in ready q	*/
			if (cpu_state == IDLE) {
				ps_ready_queue(ps_my_node, NTASKS, rtrq);
				ps_schedule(rtrq[0], ps_my_host);
			}
			break;

		default:
			ERRABORT("Invalid Scheduler Notification Message");
			break;
		}
	}
}

